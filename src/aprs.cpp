#include "aprs.h"
#include <time.h>
#include <ctype.h>

bool maidenhead_to_latlon(const char* locator, float &lat, float &lon) {
    int len = strlen(locator);
    if (len < 4 || len == 5 || len == 7) return false;

    char loc[9];
    strncpy(loc, locator, 8);
    loc[8] = '\0';

    // Primo coppia: campo (A-R) -> 20 gradi lon, 10 gradi lat
    if (!isalpha(loc[0]) || !isalpha(loc[1])) return false;
    int lon_field = toupper(loc[0]) - 'A';
    int lat_field = toupper(loc[1]) - 'A';

    // Seconda coppia: quadrato (0-9) -> 2 gradi lon, 1 grado lat
    if (!isdigit(loc[2]) || !isdigit(loc[3])) return false;
    int lon_square = loc[2] - '0';
    int lat_square = loc[3] - '0';

    // Terza coppia: sottoquadrato (a-x) -> 5 min lon, 2.5 min lat
    float lon_sub = 0.0, lat_sub = 0.0;
    if (len >= 6) {
        if (!isalpha(loc[4]) || !isalpha(loc[5])) return false;
        lon_sub = (toupper(loc[4]) - 'A') * (2.0 / 24.0);
        lat_sub = (toupper(loc[5]) - 'A') * (1.0 / 24.0);
    } else {
        // Centro del quadrato per locatori a 4 caratteri
        lon_sub = 1.0;
        lat_sub = 0.5;
    }

    // Quarta coppia: estensione (0-9) -> 1/120 grado lon, 1/240 grado lat
    float lon_ext = 0.0, lat_ext = 0.0;
    if (len >= 8) {
        if (!isdigit(loc[6]) || !isdigit(loc[7])) return false;
        lon_ext = (loc[6] - '0') * (2.0 / 240.0) + (1.0 / 240.0);
        lat_ext = (loc[7] - '0') * (1.0 / 240.0) + (0.5 / 240.0);
    } else if (len >= 6) {
        // Centro del sottoquadrato per locatori a 6 caratteri
        lon_ext = 1.0 / 24.0;
        lat_ext = 0.5 / 24.0;
    }

    lon = (lon_field * 20.0) + (lon_square * 2.0) + lon_sub + lon_ext - 180.0;
    lat = (lat_field * 10.0) + (lat_square * 1.0) + lat_sub + lat_ext - 90.0;

    return true;
}

String aprs_format_lat(float lat) {
    char ns = (lat >= 0) ? 'N' : 'S';
    lat = fabs(lat);
    int deg = (int)lat;
    float min = (lat - deg) * 60.0;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d%05.2f%c", deg, min, ns);
    return String(buf);
}

String aprs_format_lon(float lon) {
    char ew = (lon >= 0) ? 'E' : 'W';
    lon = fabs(lon);
    int deg = (int)lon;
    float min = (lon - deg) * 60.0;
    char buf[16];
    snprintf(buf, sizeof(buf), "%03d%05.2f%c", deg, min, ew);
    return String(buf);
}

String aprs_build_weather_packet(const char* callsign, int ssid,
                                  const char* locator,
                                  char symbolTable, char symbolCode,
                                  float temp_c, float humidity,
                                  float pressure_hpa) {
    // Convertire locatore Maidenhead in coordinate decimali
    float lat, lon;
    if (!maidenhead_to_latlon(locator, lat, lon)) {
        lat = 0.0;
        lon = 0.0;
    }

    return aprs_build_weather_packet(callsign, ssid, lat, lon,
                                     symbolTable, symbolCode,
                                     temp_c, humidity, pressure_hpa);
}

String aprs_build_weather_packet(const char* callsign, int ssid,
                                  float lat, float lon,
                                  char symbolTable, char symbolCode,
                                  float temp_c, float humidity,
                                  float pressure_hpa) {
    // Ottenere timestamp UTC (APRS richiede UTC con suffisso 'z')
    time_t now = time(nullptr);
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    char timestamp[8];
    if (now > 100000) {  // NTP sincronizzato
        snprintf(timestamp, sizeof(timestamp), "%02d%02d%02dz",
                 timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min);
    } else {
        snprintf(timestamp, sizeof(timestamp), "000000z");
    }

    // Convertire temperatura in Fahrenheit (formato APRS)
    int temp_f = (int)round(temp_c * 9.0 / 5.0 + 32.0);

    // Umidità: 00 = 100%
    int hum = (int)round(humidity);
    if (hum >= 100) hum = 0;

    // Costruire posizione APRS
    String lat_str = aprs_format_lat(lat);
    String lon_str = aprs_format_lon(lon);

    // Costruire header: CALLSIGN-SSID>APRS,TCPIP*:
    char header[64];
    snprintf(header, sizeof(header), "%s-%d>APRS,TCPIP*:", callsign, ssid);

    // Costruire dati meteo APRS
    // Formato: @timestamp lat[symbolTable]lon[symbolCode] vento/vel raffica temp umidita [pressione]
    // Il campo b (pressione) viene omesso se il valore e' fuori range valido (500-1100 hPa)
    char weather[128];
    if (pressure_hpa >= 500.0f && pressure_hpa <= 1100.0f) {
        long press_tenth = (long)round(pressure_hpa * 10.0);
        snprintf(weather, sizeof(weather),
                 "@%s%s%c%s%c.../...g...t%03dh%02db%05ld",
                 timestamp,
                 lat_str.c_str(),
                 symbolTable,
                 lon_str.c_str(),
                 symbolCode,
                 temp_f,
                 hum,
                 press_tenth);
    } else {
        snprintf(weather, sizeof(weather),
                 "@%s%s%c%s%c.../...g...t%03dh%02d",
                 timestamp,
                 lat_str.c_str(),
                 symbolTable,
                 lon_str.c_str(),
                 symbolCode,
                 temp_f,
                 hum);
    }

    return String(header) + String(weather);
}
