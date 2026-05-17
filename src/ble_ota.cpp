#include "ble_ota.h"

#if BLE_OTA_ENABLED

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Update.h>

static BLECharacteristic* pStatusChar = nullptr;
static bool otaInProgress = false;
static bool otaRebootPending = false;
static uint32_t otaExpectedSize = 0;
static uint32_t otaReceived = 0;

static void notifyStatus(uint8_t status) {
    if (pStatusChar) {
        pStatusChar->setValue(&status, 1);
        pStatusChar->notify();
    }
}

// === Callback per la caratteristica COMMAND ===
class OtaCmdCallback : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pChar) override {
        std::string val = pChar->getValue();
        if (val.empty()) return;

        uint8_t cmd = val[0];

        switch (cmd) {
            case BLE_OTA_CMD_START: {
                if (val.size() < 5) {
                    Serial.println("[BLE-OTA] CMD_START: mancano byte dimensione");
                    notifyStatus(BLE_OTA_STATUS_ERROR);
                    return;
                }
                otaExpectedSize = (uint32_t)val[1]
                                | ((uint32_t)val[2] << 8)
                                | ((uint32_t)val[3] << 16)
                                | ((uint32_t)val[4] << 24);
                otaReceived = 0;

                Serial.printf("[BLE-OTA] Inizio update, dimensione: %u byte\n", otaExpectedSize);

                if (!Update.begin(otaExpectedSize)) {
                    Serial.println("[BLE-OTA] Update.begin() fallito!");
                    Update.printError(Serial);
                    notifyStatus(BLE_OTA_STATUS_ERROR);
                    return;
                }
                otaInProgress = true;
                notifyStatus(BLE_OTA_STATUS_BUSY);
                break;
            }
            case BLE_OTA_CMD_END: {
                if (!otaInProgress) return;
                if (Update.end(true)) {
                    Serial.printf("[BLE-OTA] Update completato! Ricevuti %u/%u byte\n",
                                  otaReceived, otaExpectedSize);
                    notifyStatus(BLE_OTA_STATUS_SUCCESS);
                    otaInProgress = false;
                    otaRebootPending = true;
                } else {
                    Serial.println("[BLE-OTA] Update.end() fallito!");
                    Update.printError(Serial);
                    notifyStatus(BLE_OTA_STATUS_ERROR);
                    otaInProgress = false;
                }
                break;
            }
            case BLE_OTA_CMD_ABORT: {
                if (otaInProgress) {
                    Update.abort();
                    otaInProgress = false;
                    otaReceived = 0;
                    Serial.println("[BLE-OTA] Update annullato");
                    notifyStatus(BLE_OTA_STATUS_READY);
                }
                break;
            }
        }
    }
};

// === Callback per la caratteristica DATA ===
class OtaDataCallback : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pChar) override {
        if (!otaInProgress) return;

        std::string val = pChar->getValue();
        if (val.empty()) return;

        size_t written = Update.write((uint8_t*)val.data(), val.size());
        if (written != val.size()) {
            Serial.printf("[BLE-OTA] Errore scrittura: %u/%u\n", written, val.size());
            Update.abort();
            otaInProgress = false;
            notifyStatus(BLE_OTA_STATUS_ERROR);
            return;
        }
        otaReceived += written;

        // Log progresso ogni 10KB
        if (otaReceived % 10240 < val.size()) {
            Serial.printf("[BLE-OTA] Progresso: %u/%u (%u%%)\n",
                          otaReceived, otaExpectedSize,
                          (otaReceived * 100) / otaExpectedSize);
        }
    }
};

void ble_ota_begin(void* pServer) {
    BLEServer* server = (BLEServer*)pServer;

    BLEService* pService = server->createService(BLE_OTA_SERVICE_UUID);

    // Caratteristica COMMAND (write)
    BLECharacteristic* pCmdChar = pService->createCharacteristic(
        BLE_OTA_CHAR_CMD_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pCmdChar->setCallbacks(new OtaCmdCallback());

    // Caratteristica DATA (write without response per velocità)
    BLECharacteristic* pDataChar = pService->createCharacteristic(
        BLE_OTA_CHAR_DATA_UUID,
        BLECharacteristic::PROPERTY_WRITE_NR
    );
    pDataChar->setCallbacks(new OtaDataCallback());

    // Caratteristica STATUS (notify)
    pStatusChar = pService->createCharacteristic(
        BLE_OTA_CHAR_STATUS_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pStatusChar->addDescriptor(new BLE2902());
    uint8_t ready = BLE_OTA_STATUS_READY;
    pStatusChar->setValue(&ready, 1);

    pService->start();
    Serial.printf("[BLE-OTA] Servizio avviato (porta %s)\n", BLE_OTA_SERVICE_UUID);
}

void ble_ota_handle() {
    if (otaRebootPending) {
        Serial.println("[BLE-OTA] Riavvio in 2 secondi...");
        delay(2000);
        ESP.restart();
    }
}

bool ble_ota_in_progress() {
    return otaInProgress;
}

#endif // BLE_OTA_ENABLED
