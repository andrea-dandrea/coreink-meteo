#ifndef APRS_H
#define APRS_H

#include <Arduino.h>

/**
 * Converte un locatore Maidenhead in coordinate decimali (lat, lon).
 * Supporta locatori a 4 o 6 caratteri.
 * Restituisce true se la conversione è riuscita.
 */
bool maidenhead_to_latlon(const char* locator, float &lat, float &lon);

/**
 * Converte le coordinate decimali nel formato APRS (DDMM.MM)
 */
String aprs_format_lat(float lat);
String aprs_format_lon(float lon);

/**
 * Costruisce un pacchetto APRS weather report completo.
 * Formato: callsign>APRS,TCPIP*:@DDHHMMz lat[table]lon[symbol]_.../...gXXXtXXXhXXbXXXXX
 *
 * locator: locatore Maidenhead della stazione
 * symbolTable: tabella del simbolo ('/' primaria, '\\' alternativa)
 * symbolCode: codice simbolo (es. '_' per WX)
 * temp_c: temperatura in Celsius
 * humidity: umidità relativa (0-100)
 * pressure_hpa: pressione in hPa
 */
String aprs_build_weather_packet(const char* callsign, int ssid,
                                  const char* locator,
                                  char symbolTable, char symbolCode,
                                  float temp_c, float humidity,
                                  float pressure_hpa);

/**
 * Versione con coordinate decimali dirette (usata quando il GPS è disponibile).
 * lat/lon sovrascrivono il locatore Maidenhead.
 */
String aprs_build_weather_packet(const char* callsign, int ssid,
                                  float lat, float lon,
                                  char symbolTable, char symbolCode,
                                  float temp_c, float humidity,
                                  float pressure_hpa);

#endif
