#include "led_status.h"
#include "config.h"
#include "nvs_config.h"

#if defined(BOARD_COREINK)
#define LED_PIN 10
#define LED_ON  LOW   // Attivo basso
#define LED_OFF_VAL HIGH
#else
#define LED_PIN -1
#define LED_ON  LOW
#define LED_OFF_VAL HIGH
#endif

static LedState _state = LED_OFF;
static bool _enabled = true;
static unsigned long _lastToggle = 0;
static bool _ledIsOn = false;
static bool _txFlash = false;
static unsigned long _txFlashStart = 0;

void led_init() {
#if defined(BOARD_COREINK)
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LED_OFF_VAL);
#endif
    _enabled = nvs_load_int(NVS_KEY_LED_ENABLED, 1) != 0;
}

void led_set_enabled(bool enabled) {
    _enabled = enabled;
    nvs_save_int(NVS_KEY_LED_ENABLED, enabled ? 1 : 0);
    if (!enabled) {
#if defined(BOARD_COREINK)
        digitalWrite(LED_PIN, LED_OFF_VAL);
#endif
    }
}

bool led_is_enabled() { return _enabled; }

void led_set_state(LedState state) {
    _state = state;
}

void led_flash_tx() {
    _txFlash = true;
    _txFlashStart = millis();
#if defined(BOARD_COREINK)
    if (_enabled) digitalWrite(LED_PIN, LED_ON);
#endif
}

void led_update() {
#if defined(BOARD_COREINK)
    if (!_enabled) {
        digitalWrite(LED_PIN, LED_OFF_VAL);
        return;
    }

    // TX flash override (200ms)
    if (_txFlash) {
        if (millis() - _txFlashStart >= 200) {
            _txFlash = false;
        } else {
            digitalWrite(LED_PIN, LED_ON);
            return;
        }
    }

    unsigned long now = millis();
    unsigned long interval;

    switch (_state) {
        case LED_OFF:
            digitalWrite(LED_PIN, LED_OFF_VAL);
            break;
        case LED_SOLID:
            digitalWrite(LED_PIN, LED_ON);
            break;
        case LED_SLOW:
            interval = 1000;
            if (now - _lastToggle >= interval) {
                _ledIsOn = !_ledIsOn;
                digitalWrite(LED_PIN, _ledIsOn ? LED_ON : LED_OFF_VAL);
                _lastToggle = now;
            }
            break;
        case LED_FAST:
            interval = 100;
            if (now - _lastToggle >= interval) {
                _ledIsOn = !_ledIsOn;
                digitalWrite(LED_PIN, _ledIsOn ? LED_ON : LED_OFF_VAL);
                _lastToggle = now;
            }
            break;
    }
#endif
}
