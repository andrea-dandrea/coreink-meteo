#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

// Eventi sonori
enum BuzzerEvent {
    BUZZ_TX_OK,
    BUZZ_TX_FAIL,
    BUZZ_GPS_FIX,
    BUZZ_GPS_LOST,
    BUZZ_WIFI_OK,
    BUZZ_WIFI_LOST,
    BUZZ_BAT_LOW,
    BUZZ_PAGE,
    BUZZ_CONFIRM,
    BUZZ_BOOT
};

void buzzer_init();
void buzzer_play_event(BuzzerEvent event);
void buzzer_play_melody(int melodyId);
void buzzer_tone(int freq, int duration, int volume = -1);
void buzzer_set_enabled(bool enabled);
void buzzer_set_volume(int volume);
bool buzzer_is_enabled();

#endif
