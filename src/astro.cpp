#include "astro.h"
#include <math.h>

// Gradi → radianti
static float degToRad(float d) { return d * M_PI / 180.0f; }
static float radToDeg(float r) { return r * 180.0f / M_PI; }

// Giorno giuliano da data
static float julianDay(int y, int m, int d) {
    if (m <= 2) { y--; m += 12; }
    int A = y / 100;
    int B = 2 - A + A / 4;
    return (int)(365.25f * (y + 4716)) + (int)(30.6001f * (m + 1)) + d + B - 1524.5f;
}

SunTimes astro_sun_times(int year, int month, int day, float lat, float lon, float tzOffset) {
    SunTimes result = {6, 0, 18, 0, 720};

    float jd = julianDay(year, month, day);
    float jc = (jd - 2451545.0f) / 36525.0f;

    // Longitudine media del sole
    float L0 = fmod(280.46646f + jc * (36000.76983f + 0.0003032f * jc), 360.0f);
    // Anomalia media
    float M = fmod(357.52911f + jc * (35999.05029f - 0.0001537f * jc), 360.0f);
    // Equazione del centro
    float C = (1.914602f - jc * (0.004817f + 0.000014f * jc)) * sin(degToRad(M))
            + (0.019993f - 0.000101f * jc) * sin(degToRad(2 * M))
            + 0.000289f * sin(degToRad(3 * M));
    // Longitudine vera
    float sunLon = L0 + C;
    // Longitudine apparente
    float omega = 125.04f - 1934.136f * jc;
    float lambda = sunLon - 0.00569f - 0.00478f * sin(degToRad(omega));

    // Obliquità eclittica
    float obliq = 23.0f + (26.0f + (21.448f - jc * (46.815f + jc * (0.00059f - jc * 0.001813f))) / 60.0f) / 60.0f;
    obliq += 0.00256f * cos(degToRad(omega));

    // Declinazione solare
    float sinDecl = sin(degToRad(obliq)) * sin(degToRad(lambda));
    float decl = radToDeg(asin(sinDecl));

    // Equazione del tempo (minuti)
    float y2 = tan(degToRad(obliq / 2.0f));
    y2 *= y2;
    float eqTime = 4.0f * radToDeg(
        y2 * sin(2 * degToRad(L0))
        - 2 * sin(degToRad(M)) * 0.01671f  // eccentricità
        + 4 * 0.01671f * y2 * sin(degToRad(M)) * cos(2 * degToRad(L0))
        - 0.5f * y2 * y2 * sin(4 * degToRad(L0))
        - 1.25f * 0.01671f * 0.01671f * sin(2 * degToRad(M))
    );

    // Angolo orario alba/tramonto (zenith 90.833°)
    float zenith = 90.833f;
    float cosHA = (cos(degToRad(zenith)) - sin(degToRad(lat)) * sinDecl)
                / (cos(degToRad(lat)) * cos(degToRad(decl)));

    if (cosHA > 1.0f) {
        // Sole mai sorge (notte polare)
        result.dayLengthMin = 0;
        return result;
    }
    if (cosHA < -1.0f) {
        // Sole mai tramonta (giorno polare)
        result.dayLengthMin = 1440;
        return result;
    }

    float HA = radToDeg(acos(cosHA));

    // Mezzogiorno solare (minuti da mezzanotte UTC)
    float solarNoon = (720.0f - 4.0f * lon - eqTime) / 1440.0f;
    float sunriseUTC = solarNoon * 1440.0f - HA * 4.0f;
    float sunsetUTC = solarNoon * 1440.0f + HA * 4.0f;

    // Applicare timezone
    float sunrise = sunriseUTC + tzOffset * 60.0f;
    float sunset = sunsetUTC + tzOffset * 60.0f;

    if (sunrise < 0) sunrise += 1440;
    if (sunrise >= 1440) sunrise -= 1440;
    if (sunset < 0) sunset += 1440;
    if (sunset >= 1440) sunset -= 1440;

    result.sunriseHour = (int)sunrise / 60;
    result.sunriseMin = (int)sunrise % 60;
    result.sunsetHour = (int)sunset / 60;
    result.sunsetMin = (int)sunset % 60;
    result.dayLengthMin = (int)(sunset - sunrise);
    if (result.dayLengthMin < 0) result.dayLengthMin += 1440;

    return result;
}

int astro_moon_phase(int year, int month, int day) {
    // Algoritmo semplificato basato sul ciclo sinodale
    // Luna nuova di riferimento: 6 gennaio 2000
    float jd = julianDay(year, month, day);
    float refJd = julianDay(2000, 1, 6);
    float daysSinceNew = jd - refJd;
    float synodicMonth = 29.53058867f;
    float phase = fmod(daysSinceNew, synodicMonth);
    if (phase < 0) phase += synodicMonth;
    return (int)(phase + 0.5f) % 30;
}

const char* astro_moon_name(int phase) {
    if (phase <= 1 || phase >= 29) return "Nuova";
    if (phase <= 6) return "Crescente";
    if (phase <= 8) return "1o Quarto";
    if (phase <= 13) return "Gibbosa+";
    if (phase <= 16) return "Piena";
    if (phase <= 21) return "Gibbosa-";
    if (phase <= 23) return "Ult.Quarto";
    return "Calante";
}

char astro_moon_icon(int phase) {
    if (phase <= 1 || phase >= 29) return 'O';  // Nuova
    if (phase <= 8) return 'D';  // Crescente / 1° quarto
    if (phase <= 16) return '@'; // Piena
    return 'C';                  // Calante
}
