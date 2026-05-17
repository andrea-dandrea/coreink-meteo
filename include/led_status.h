#ifndef LED_STATUS_H
#define LED_STATUS_H

#include <Arduino.h>

enum LedState {
    LED_OFF,        // Spento: offline/standby
    LED_SOLID,      // Acceso fisso: WiFi connesso, tutto OK
    LED_SLOW,       // Lampeggio lento (1s): in attesa
    LED_FAST        // Lampeggio rapido (100ms): errore/batteria bassa
};

void led_init();
void led_set_state(LedState state);
void led_update();  // Chiamare nel loop per gestire lampeggio
void led_flash_tx();  // Flash singolo TX (override momentaneo)
void led_set_enabled(bool enabled);
bool led_is_enabled();

#endif
