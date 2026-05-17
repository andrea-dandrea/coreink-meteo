#ifndef BLE_OTA_H
#define BLE_OTA_H

#include "config.h"

#if BLE_OTA_ENABLED

// UUID del servizio e delle caratteristiche BLE OTA
// Servizio custom per evitare conflitti con servizi standard
#define BLE_OTA_SERVICE_UUID        "fb1e4001-54ae-4a28-9f74-dfccb248601d"
#define BLE_OTA_CHAR_CMD_UUID       "fb1e4002-54ae-4a28-9f74-dfccb248601d"  // Comandi (write)
#define BLE_OTA_CHAR_DATA_UUID      "fb1e4003-54ae-4a28-9f74-dfccb248601d"  // Dati firmware (write-no-response)
#define BLE_OTA_CHAR_STATUS_UUID    "fb1e4004-54ae-4a28-9f74-dfccb248601d"  // Stato (notify)

// Comandi inviati alla caratteristica CMD
#define BLE_OTA_CMD_START   0x01   // Payload: 4 byte (uint32 little-endian) = dimensione firmware
#define BLE_OTA_CMD_END     0x02   // Fine trasmissione, verifica e applica
#define BLE_OTA_CMD_ABORT   0x03   // Annulla aggiornamento in corso

// Codici di stato notificati via STATUS
#define BLE_OTA_STATUS_READY    0x00
#define BLE_OTA_STATUS_BUSY     0x01
#define BLE_OTA_STATUS_SUCCESS  0x02
#define BLE_OTA_STATUS_ERROR    0x03

/// Inizializza il servizio BLE OTA (da chiamare dopo BLEDevice::init)
void ble_ota_begin(void* pServer);

/// Da chiamare nel loop per gestire il reboot post-update
void ble_ota_handle();

/// Ritorna true se un aggiornamento BLE è in corso
bool ble_ota_in_progress();

#endif // BLE_OTA_ENABLED
#endif // BLE_OTA_H
