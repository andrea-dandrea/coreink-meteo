#ifndef ASTRO_H
#define ASTRO_H

#include <Arduino.h>

// Risultato calcolo solare
struct SunTimes {
    int sunriseHour, sunriseMin;
    int sunsetHour, sunsetMin;
    int dayLengthMin;             // Durata del giorno in minuti
};

// Calcola alba/tramonto per una data e posizione (algoritmo NOAA semplificato)
// lat/lon in gradi decimali, tzOffset in ore (es. +1 per CET, +2 per CEST)
SunTimes astro_sun_times(int year, int month, int day, float lat, float lon, float tzOffset);

// Fase lunare (0-29, ciclo sinodale ~29.53 giorni)
// 0=luna nuova, 7=primo quarto, 15=luna piena, 22=ultimo quarto
int astro_moon_phase(int year, int month, int day);

// Nome fase lunare
const char* astro_moon_name(int phase);

// Icona fase lunare (carattere ASCII)
char astro_moon_icon(int phase);

#endif
