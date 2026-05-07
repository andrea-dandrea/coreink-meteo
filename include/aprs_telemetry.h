#ifndef APRS_TELEMETRY_H
#define APRS_TELEMETRY_H

#include <Arduino.h>

/**
 * Modulo telemetria APRS.
 *
 * Gestisce l'invio di:
 * - Pacchetti T# (dati telemetrici, 5 canali analogici + 8 bit digitali)
 * - Pacchetti PARM (nomi dei parametri)
 * - Pacchetti UNIT (unità di misura)
 * - Pacchetti EQNS (equazioni di conversione)
 *
 * Canali utilizzati:
 *   A1: Tensione batteria (mV)
 *   A2: RSSI WiFi (valore + 100 per renderlo positivo)
 *   A3: Uptime (minuti)
 *   A4: Satelliti GPS
 *   A5: (riservato)
 *   D1-D8: bit di stato
 */

/**
 * Costruisce il pacchetto dati telemetrici T#seq,a1,a2,a3,a4,a5,dddddddd
 *
 * seq: contatore sequenziale (0-999)
 * batMv: tensione batteria in mV
 * rssi: RSSI WiFi in dBm (valore negativo)
 * uptimeMin: uptime in minuti
 * satellites: numero satelliti GPS (0 se disabilitato)
 * digitalBits: 8 bit di stato
 */
String aprs_build_telemetry_data(const char* callsign, int ssid,
                                  int seq, int batMv, int rssi,
                                  int uptimeMin, int satellites,
                                  uint8_t digitalBits);

/**
 * Costruisce il pacchetto definizione parametri (PARM)
 */
String aprs_build_telemetry_parm(const char* callsign, int ssid);

/**
 * Costruisce il pacchetto unità di misura (UNIT)
 */
String aprs_build_telemetry_unit(const char* callsign, int ssid);

/**
 * Costruisce il pacchetto equazioni di conversione (EQNS)
 */
String aprs_build_telemetry_eqns(const char* callsign, int ssid);

/**
 * Costruisce un pacchetto posizione APRS (senza dati meteo).
 * Usato come beacon di localizzazione separato dal weather report.
 *
 * comment: testo aggiuntivo (es. stato batteria)
 */
String aprs_build_position_packet(const char* callsign, int ssid,
                                   float lat, float lon,
                                   char symbolTable, char symbolCode,
                                   const char* comment);

String aprs_build_position_packet(const char* callsign, int ssid,
                                   const char* locator,
                                   char symbolTable, char symbolCode,
                                   const char* comment);

#endif
