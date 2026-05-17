#ifndef GPS_EXTRA_H
#define GPS_EXTRA_H

#include <Arduino.h>

// Massimo satelliti tracciabili
#define MAX_SATS 32

// Costellazione
enum SatConstellation {
    SAT_GPS,       // GP
    SAT_BEIDOU,    // BD
    SAT_GLONASS,   // GL
    SAT_UNKNOWN
};

// Dati singolo satellite (da GSV)
struct SatInfo {
    uint8_t prn;           // PRN / ID satellite
    uint8_t elevation;     // Elevazione 0-90°
    uint16_t azimuth;      // Azimuth 0-359°
    uint8_t snr;           // SNR/CN0 in dB-Hz (0 = non tracciato)
    SatConstellation constellation;
    bool tracked;          // SNR > 0 → satellite usato
};

// Parser GSV per dati satellitari estesi
class GpsExtra {
public:
    GpsExtra();

    // Alimentare con ogni carattere ricevuto dal GPS (stessi dati di TinyGPS)
    void encode(char c);

    // Dati satellitari
    int getSatsInView() const { return _satsInView; }
    int getSatsTracked() const;  // Con SNR > 0
    const SatInfo* getSatInfo() const { return _sats; }
    int getSatCount() const { return _satCount; }

    // HDOP (da GGA/GSA)
    float getHdop() const { return _hdop; }
    float getPdop() const { return _pdop; }
    float getVdop() const { return _vdop; }

    // Altitudine (da GGA, più precisa di RMC)
    float getAltitude() const { return _altitude; }

    // Fix quality (0=no, 1=GPS, 2=DGPS)
    int getFixQuality() const { return _fixQuality; }

    // Aggiornato? (reset dopo lettura)
    bool isGsvUpdated();
    bool isGgaUpdated();

private:
    // Buffer sentenza NMEA
    char _sentence[128];
    int _sentenceIdx;
    bool _inSentence;

    // Satelliti
    SatInfo _sats[MAX_SATS];
    SatInfo _satsBuild[MAX_SATS];  // Buffer costruzione durante multi-GSV
    int _satCount;
    int _satBuildCount;
    int _satsInView;
    int _gsvMsgTotal;
    int _gsvMsgCurrent;
    SatConstellation _currentConstellation;

    // DOP
    float _hdop, _pdop, _vdop;

    // GGA
    float _altitude;
    int _fixQuality;

    // Flag aggiornamento
    bool _gsvUpdated;
    bool _ggaUpdated;

    void _processSentence();
    void _parseGSV();
    void _parseGGA();
    void _parseGSA();
    int _getField(const char* sentence, int fieldNum, char* buf, int bufLen);
};

#endif
