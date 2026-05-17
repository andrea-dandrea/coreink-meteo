#include "buzzer.h"
#include "config.h"
#include "nvs_config.h"

static bool _enabled = true;
static int _volume = BUZZER_DEFAULT_VOLUME;
static bool _eventFlags[10] = {true, true, true, true, true, true, true, true, true, true};

void buzzer_init() {
    ledcSetup(0, 2000, 8);  // Channel 0, 2kHz default, 8-bit
    ledcAttachPin(BUZZER_PIN, 0);
    ledcWrite(0, 0);  // Silenzio

    _enabled = nvs_load_int(NVS_KEY_BUZZER_ENABLED, 1) != 0;
    _volume = nvs_load_int(NVS_KEY_BUZZER_VOLUME, BUZZER_DEFAULT_VOLUME);
}

void buzzer_set_enabled(bool enabled) {
    _enabled = enabled;
    nvs_save_int(NVS_KEY_BUZZER_ENABLED, enabled ? 1 : 0);
}

void buzzer_set_volume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    _volume = volume;
    nvs_save_int(NVS_KEY_BUZZER_VOLUME, volume);
}

bool buzzer_is_enabled() { return _enabled; }

void buzzer_tone(int freq, int duration, int volume) {
    if (!_enabled) return;
    int vol = (volume >= 0) ? volume : _volume;
    int duty = (vol * 127) / 100;
    ledcWriteTone(0, freq);
    ledcWrite(0, duty);
    delay(duration);
    ledcWrite(0, 0);
}

void buzzer_play_event(BuzzerEvent event) {
    if (!_enabled || !_eventFlags[event]) return;

    switch (event) {
        case BUZZ_TX_OK:
            buzzer_tone(2000, 100);
            break;
        case BUZZ_TX_FAIL:
            buzzer_tone(800, 200);
            delay(100);
            buzzer_tone(800, 200);
            break;
        case BUZZ_GPS_FIX:
            buzzer_tone(500, 80);
            buzzer_tone(1000, 80);
            buzzer_tone(1500, 80);
            break;
        case BUZZ_GPS_LOST:
            buzzer_tone(1500, 80);
            buzzer_tone(1000, 80);
            buzzer_tone(500, 80);
            break;
        case BUZZ_WIFI_OK:
            buzzer_tone(1500, 60);
            delay(40);
            buzzer_tone(1500, 60);
            delay(40);
            buzzer_tone(1500, 60);
            break;
        case BUZZ_WIFI_LOST:
            buzzer_tone(600, 300);
            break;
        case BUZZ_BAT_LOW:
            buzzer_tone(1000, 500);
            break;
        case BUZZ_PAGE:
            buzzer_tone(4000, 50);
            break;
        case BUZZ_CONFIRM:
            buzzer_tone(1500, 150);
            break;
        case BUZZ_BOOT:
            buzzer_play_melody(nvs_load_int(NVS_KEY_BOOT_MELODY, 2));
            break;
    }
}

void buzzer_play_melody(int melodyId) {
    if (!_enabled) return;

    switch (melodyId) {
        case 0: // Bip singolo
            buzzer_tone(2000, 100);
            break;
        case 1: // Weather Chime: Do-Mi-Sol-Do
            buzzer_tone(523, 100);  // Do5
            buzzer_tone(659, 100);  // Mi5
            buzzer_tone(784, 100);  // Sol5
            buzzer_tone(1047, 150); // Do6
            break;
        case 2: // Sailor's Call: Sol-Do-Mi + Sol-Do
            buzzer_tone(392, 150);  // Sol4
            buzzer_tone(523, 150);  // Do5
            buzzer_tone(659, 150);  // Mi5
            delay(80);
            buzzer_tone(784, 100);  // Sol5
            buzzer_tone(1047, 200); // Do6
            break;
        case 3: // Sunrise: Do-Re-Mi-Sol-Do
            buzzer_tone(262, 80);   // Do4
            buzzer_tone(294, 80);   // Re4
            buzzer_tone(330, 80);   // Mi4
            buzzer_tone(392, 80);   // Sol4
            buzzer_tone(523, 120);  // Do5
            break;
        case 4: // Radio Check: La-La-La-La alta
            buzzer_tone(440, 200);  // La4
            delay(100);
            buzzer_tone(440, 100);  // La4
            buzzer_tone(440, 100);  // La4
            buzzer_tone(880, 200);  // La5
            break;
        case 5: // Jingle minimale: Do-Mi-Sol
            buzzer_tone(523, 80);   // Do5
            delay(40);
            buzzer_tone(659, 80);   // Mi5
            delay(40);
            buzzer_tone(784, 200);  // Sol5
            break;
    }
}
