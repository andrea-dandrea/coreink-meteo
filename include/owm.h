#ifndef OWM_H
#define OWM_H

#include <Arduino.h>

// Dati meteo da OpenWeatherMap
struct OwmData {
    float temp;          // Temperatura °C
    float feels_like;    // Percepita °C
    float humidity;      // Umidità %
    float pressure;      // Pressione hPa
    float wind_speed;    // Vento m/s
    float wind_gust;     // Raffica m/s
    int wind_deg;        // Direzione vento (gradi)
    int weather_id;      // Codice condizione OWM (200-804)
    char description[32]; // Descrizione testuale (es. "cielo sereno")
    char icon[4];        // Icona OWM (es. "01d")
    float uvi;           // Indice UV (da One Call, -1 se non disponibile)
    float clouds;        // Nuvolosità %
    float pop;           // Probabilità pioggia (0-1)
    float rain_3h;       // Pioggia ultimi 3h (mm)
    unsigned long timestamp; // Epoch dell'ultimo aggiornamento riuscito
    bool valid;          // Dati validi (almeno un fetch riuscito)
};

// Inizializza il modulo OWM (chiama in setup dopo NVS)
void owm_init();

// Aggiorna dati OWM se scaduto l'intervallo (chiama nel loop)
// Ritorna true se ha fatto un fetch (riuscito o meno)
bool owm_update(unsigned long now_ms);

// Forza un aggiornamento immediato
bool owm_fetch_now();

// Dati correnti (sempre accessibile, controlla .valid)
const OwmData& owm_get_data();

// Stato: ultima volta che il fetch è riuscito (millis)
unsigned long owm_last_success();

// Configurato? (API key presente in NVS)
bool owm_is_configured();

// --- Forecast 5-day/3h ---
struct OwmForecastSlot {
    float temp_min;
    float temp_max;
    char description[24];
    int hour;       // Ora locale (0-23)
    int day;        // Giorno del mese
    bool valid;
};

#define OWM_FORECAST_SLOTS 5  // +3h, +6h, +9h, domani matt, domani pom

// Dati forecast (4 slot)
const OwmForecastSlot* owm_get_forecast();

// Forecast valido?
bool owm_forecast_valid();

#endif // OWM_H
