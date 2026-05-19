#include "aprs_telemetry.h"
#include "aprs.h"

String aprs_build_telemetry_data(const char* callsign, int ssid,
                                  int seq, int batMv, int rssi,
                                  int uptimeMin, int satellites,
                                  uint8_t digitalBits) {
    // Header
    char header[64];
    snprintf(header, sizeof(header), "%s-%d>APRS,TCPIP*:", callsign, ssid);

    // Canale A1: tensione batteria (mV), range 0-8191
    int a1 = constrain(batMv, 0, 8191);
    // Canale A2: RSSI + 100 (es. -70 dBm -> 30), range 0-255
    int a2 = constrain(rssi + 100, 0, 255);
    // Canale A3: uptime in minuti, range 0-8191
    int a3 = constrain(uptimeMin, 0, 8191);
    // Canale A4: satelliti GPS, range 0-255
    int a4 = constrain(satellites, 0, 255);
    // Canale A5: riservato
    int a5 = 0;

    // Formato bit digitali
    char bits[9];
    for (int i = 7; i >= 0; i--) {
        bits[7 - i] = (digitalBits & (1 << i)) ? '1' : '0';
    }
    bits[8] = '\0';

    // Sequenza 0-999
    int s = seq % 1000;

    char payload[64];
    snprintf(payload, sizeof(payload), "T#%03d,%03d,%03d,%03d,%03d,%03d,%s",
             s, a1, a2, a3, a4, a5, bits);

    return String(header) + String(payload);
}

String aprs_build_telemetry_parm(const char* callsign, int ssid) {
    char header[64];
    snprintf(header, sizeof(header), "%s-%d>APRS,TCPIP*:", callsign, ssid);

    // Messaggio diretto a se stesso con definizione parametri
    // Il destinatario APRS deve essere callsign-ssid paddato a 9 caratteri totali
    char full_call[16];
    snprintf(full_call, sizeof(full_call), "%s-%d", callsign, ssid);
    char dest[16];
    snprintf(dest, sizeof(dest), "%-9s", full_call);

    char payload[128];
    snprintf(payload, sizeof(payload), ":%s:PARM.Vbat,RSSI,Uptime,Sat,Rsvd,GPS,WiFi,Chg,TX,Err,LoRa,R2,R3",
             dest);

    return String(header) + String(payload);
}

String aprs_build_telemetry_unit(const char* callsign, int ssid) {
    char header[64];
    snprintf(header, sizeof(header), "%s-%d>APRS,TCPIP*:", callsign, ssid);

    char full_call[16];
    snprintf(full_call, sizeof(full_call), "%s-%d", callsign, ssid);
    char dest[16];
    snprintf(dest, sizeof(dest), "%-9s", full_call);

    char payload[128];
    snprintf(payload, sizeof(payload), ":%s:UNIT.mV,dBm,min,n,,,,,,,,",
             dest);

    return String(header) + String(payload);
}

String aprs_build_telemetry_eqns(const char* callsign, int ssid) {
    char header[64];
    snprintf(header, sizeof(header), "%s-%d>APRS,TCPIP*:", callsign, ssid);

    char full_call[16];
    snprintf(full_call, sizeof(full_call), "%s-%d", callsign, ssid);
    char dest[16];
    snprintf(dest, sizeof(dest), "%-9s", full_call);

    // Equazioni: valore = a*x^2 + b*x + c
    // A1: Vbat -> a=0, b=1, c=0 (diretto in mV)
    // A2: RSSI -> a=0, b=1, c=-100 (sottrae 100 per avere dBm)
    // A3: Uptime -> a=0, b=1, c=0 (diretto in minuti)
    // A4: Sat -> a=0, b=1, c=0 (diretto)
    // A5: riservato -> a=0, b=0, c=0
    char payload[128];
    snprintf(payload, sizeof(payload), ":%s:EQNS.0,1,0,0,1,-100,0,1,0,0,1,0,0,0,0",
             dest);

    return String(header) + String(payload);
}

String aprs_build_position_packet(const char* callsign, int ssid,
                                   float lat, float lon,
                                   char symbolTable, char symbolCode,
                                   const char* comment) {
    // Header
    char header[64];
    snprintf(header, sizeof(header), "%s-%d>APRS,TCPIP*:", callsign, ssid);

    // Posizione APRS
    String lat_str = aprs_format_lat(lat);
    String lon_str = aprs_format_lon(lon);

    // Formato: !lat[table]lon[symbol] commento
    char payload[128];
    snprintf(payload, sizeof(payload), "!%s%c%s%c%s",
             lat_str.c_str(), symbolTable,
             lon_str.c_str(), symbolCode,
             comment ? comment : "");

    return String(header) + String(payload);
}

String aprs_build_position_packet(const char* callsign, int ssid,
                                   const char* locator,
                                   char symbolTable, char symbolCode,
                                   const char* comment) {
    float lat, lon;
    if (!maidenhead_to_latlon(locator, lat, lon)) {
        lat = 0.0;
        lon = 0.0;
    }
    return aprs_build_position_packet(callsign, ssid, lat, lon,
                                       symbolTable, symbolCode, comment);
}
