#include "gps_extra.h"
#include <string.h>
#include <stdlib.h>

GpsExtra::GpsExtra() {
    _sentenceIdx = 0;
    _inSentence = false;
    _satCount = 0;
    _satBuildCount = 0;
    _satsInView = 0;
    _gsvMsgTotal = 0;
    _gsvMsgCurrent = 0;
    _currentConstellation = SAT_UNKNOWN;
    _hdop = _pdop = _vdop = 99.9f;
    _altitude = 0;
    _fixQuality = 0;
    _gsvUpdated = false;
    _ggaUpdated = false;
    memset(_sats, 0, sizeof(_sats));
    memset(_satsBuild, 0, sizeof(_satsBuild));
}

void GpsExtra::encode(char c) {
    if (c == '$') {
        _sentenceIdx = 0;
        _inSentence = true;
        _sentence[_sentenceIdx++] = c;
        return;
    }

    if (!_inSentence) return;

    if (c == '\r' || c == '\n') {
        _sentence[_sentenceIdx] = '\0';
        _inSentence = false;
        if (_sentenceIdx > 10) {
            _processSentence();
        }
        return;
    }

    if (_sentenceIdx < (int)sizeof(_sentence) - 1) {
        _sentence[_sentenceIdx++] = c;
    } else {
        _inSentence = false;
    }
}

int GpsExtra::getSatsTracked() const {
    int count = 0;
    for (int i = 0; i < _satCount; i++) {
        if (_sats[i].snr > 0) count++;
    }
    return count;
}

bool GpsExtra::isGsvUpdated() {
    bool v = _gsvUpdated;
    _gsvUpdated = false;
    return v;
}

bool GpsExtra::isGgaUpdated() {
    bool v = _ggaUpdated;
    _ggaUpdated = false;
    return v;
}

// Estrarre campo N dalla sentenza NMEA (separatore virgola)
int GpsExtra::_getField(const char* sentence, int fieldNum, char* buf, int bufLen) {
    int field = 0;
    int i = 0;
    int start = 0;

    // Saltare il '$' iniziale e il talker+type (es. $GPGSV,)
    for (i = 0; sentence[i] != '\0'; i++) {
        if (sentence[i] == ',') {
            if (field == fieldNum) {
                int len = i - start;
                if (len >= bufLen) len = bufLen - 1;
                strncpy(buf, &sentence[start], len);
                buf[len] = '\0';
                return len;
            }
            field++;
            start = i + 1;
        }
    }
    // Ultimo campo (prima del checksum *)
    if (field == fieldNum) {
        // Cercare * per checksum
        int end = i;
        for (int j = start; j < i; j++) {
            if (sentence[j] == '*') { end = j; break; }
        }
        int len = end - start;
        if (len >= bufLen) len = bufLen - 1;
        strncpy(buf, &sentence[start], len);
        buf[len] = '\0';
        return len;
    }
    buf[0] = '\0';
    return 0;
}

void GpsExtra::_processSentence() {
    // Identificare tipo sentenza: $GPGSV, $BDGSV, $GLGSV, $GPGGA, $GPGSA, $GNGGA, $GNGSA
    if (strstr(_sentence, "GSV")) {
        // Determinare costellazione dal talker ID
        if (_sentence[1] == 'G' && _sentence[2] == 'P') {
            _currentConstellation = SAT_GPS;
        } else if (_sentence[1] == 'B' && _sentence[2] == 'D') {
            _currentConstellation = SAT_BEIDOU;
        } else if (_sentence[1] == 'G' && _sentence[2] == 'L') {
            _currentConstellation = SAT_GLONASS;
        } else {
            _currentConstellation = SAT_GPS;  // Default
        }
        _parseGSV();
    } else if (strstr(_sentence, "GGA")) {
        _parseGGA();
    } else if (strstr(_sentence, "GSA")) {
        _parseGSA();
    }
}

void GpsExtra::_parseGSV() {
    // $GPGSV,totMsg,msgNum,satsInView,prn1,elev1,az1,snr1,...*cs
    char buf[16];

    // Campo 1: totale messaggi GSV
    _getField(_sentence, 1, buf, sizeof(buf));
    int totMsg = atoi(buf);

    // Campo 2: numero messaggio corrente
    _getField(_sentence, 2, buf, sizeof(buf));
    int msgNum = atoi(buf);

    // Campo 3: satelliti in vista
    _getField(_sentence, 3, buf, sizeof(buf));
    int satsInView = atoi(buf);

    if (msgNum == 1) {
        // Primo messaggio di una serie: resettare buffer costruzione
        _satBuildCount = 0;
        _gsvMsgTotal = totMsg;
        _satsInView = satsInView;
    }

    // Ogni messaggio GSV contiene fino a 4 satelliti (campi 4-7, 8-11, 12-15, 16-19)
    for (int i = 0; i < 4; i++) {
        if (_satBuildCount >= MAX_SATS) break;

        int base = 4 + i * 4;

        // PRN
        _getField(_sentence, base, buf, sizeof(buf));
        if (buf[0] == '\0') break;  // Nessun altro satellite
        int prn = atoi(buf);

        // Elevazione
        _getField(_sentence, base + 1, buf, sizeof(buf));
        int elev = (buf[0] != '\0') ? atoi(buf) : 0;

        // Azimuth
        _getField(_sentence, base + 2, buf, sizeof(buf));
        int az = (buf[0] != '\0') ? atoi(buf) : 0;

        // SNR (può essere vuoto se non tracciato)
        _getField(_sentence, base + 3, buf, sizeof(buf));
        int snr = (buf[0] != '\0') ? atoi(buf) : 0;

        _satsBuild[_satBuildCount].prn = prn;
        _satsBuild[_satBuildCount].elevation = elev;
        _satsBuild[_satBuildCount].azimuth = az;
        _satsBuild[_satBuildCount].snr = snr;
        _satsBuild[_satBuildCount].constellation = _currentConstellation;
        _satsBuild[_satBuildCount].tracked = (snr > 0);
        _satBuildCount++;
    }

    // Se è l'ultimo messaggio della serie, copiare nel buffer principale
    if (msgNum == totMsg) {
        memcpy(_sats, _satsBuild, sizeof(SatInfo) * _satBuildCount);
        _satCount = _satBuildCount;
        _gsvUpdated = true;
    }
}

void GpsExtra::_parseGGA() {
    // $GPGGA,time,lat,N,lon,E,quality,numSats,hdop,alt,M,geoid,M,...*cs
    char buf[16];

    // Campo 6: fix quality
    _getField(_sentence, 6, buf, sizeof(buf));
    _fixQuality = (buf[0] != '\0') ? atoi(buf) : 0;

    // Campo 8: HDOP
    _getField(_sentence, 8, buf, sizeof(buf));
    if (buf[0] != '\0') _hdop = atof(buf);

    // Campo 9: altitudine
    _getField(_sentence, 9, buf, sizeof(buf));
    if (buf[0] != '\0') _altitude = atof(buf);

    _ggaUpdated = true;
}

void GpsExtra::_parseGSA() {
    // $GPGSA,A,3,prn,prn,...,pdop,hdop,vdop*cs
    // Campi 15,16,17: PDOP, HDOP, VDOP
    char buf[16];

    // Cercare gli ultimi 3 campi numerici prima del checksum
    // In GSA standard: campo 15=PDOP, 16=HDOP, 17=VDOP
    _getField(_sentence, 15, buf, sizeof(buf));
    if (buf[0] != '\0') _pdop = atof(buf);

    _getField(_sentence, 16, buf, sizeof(buf));
    if (buf[0] != '\0') _hdop = atof(buf);

    _getField(_sentence, 17, buf, sizeof(buf));
    if (buf[0] != '\0') _vdop = atof(buf);
}
