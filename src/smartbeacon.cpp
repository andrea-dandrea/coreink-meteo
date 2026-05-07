#include "smartbeacon.h"

SmartBeacon::SmartBeacon(float fastSpeed, int fastRate,
                         float slowSpeed, int slowRate,
                         int turnMinAngle, int turnSlope, int turnTime)
    : _fastSpeed(fastSpeed), _fastRate(fastRate),
      _slowSpeed(slowSpeed), _slowRate(slowRate),
      _turnMinAngle(turnMinAngle), _turnSlope(turnSlope), _turnTime(turnTime),
      _lastCourse(0), _lastBeaconTime(0), _lastTurnTime(0), _firstFix(true) {}

bool SmartBeacon::shouldBeacon(float speed_kmh, float course) {
    unsigned long now = millis();

    // Primo fix: inviare subito
    if (_firstFix) {
        _firstFix = false;
        _lastCourse = course;
        _lastBeaconTime = now;
        _lastTurnTime = now;
        return true;
    }

    unsigned long elapsed = (now - _lastBeaconTime) / 1000;
    int interval = _calcInterval(speed_kmh);

    // Controllo intervallo basato su velocità
    if ((int)elapsed >= interval) {
        _lastCourse = course;
        _lastBeaconTime = now;
        return true;
    }

    // Controllo cambio di rotta (solo se in movimento)
    if (speed_kmh > _slowSpeed) {
        float diff = _courseDiff(course, _lastCourse);

        // Soglia dinamica: angolo minimo + slope/velocità
        float threshold = (float)_turnMinAngle + (float)_turnSlope / speed_kmh;

        unsigned long turnElapsed = (now - _lastTurnTime) / 1000;

        if (diff >= threshold && (int)turnElapsed >= _turnTime) {
            _lastCourse = course;
            _lastBeaconTime = now;
            _lastTurnTime = now;
            return true;
        }
    }

    return false;
}

void SmartBeacon::reset() {
    _lastBeaconTime = millis();
    _firstFix = false;
}

unsigned long SmartBeacon::secondsSinceLastBeacon() {
    return (millis() - _lastBeaconTime) / 1000;
}

int SmartBeacon::_calcInterval(float speed_kmh) {
    if (speed_kmh <= _slowSpeed) {
        return _slowRate;
    }
    if (speed_kmh >= _fastSpeed) {
        return _fastRate;
    }
    // Interpolazione lineare tra slow e fast
    float ratio = (speed_kmh - _slowSpeed) / (_fastSpeed - _slowSpeed);
    return _slowRate - (int)(ratio * (_slowRate - _fastRate));
}

float SmartBeacon::_courseDiff(float a, float b) {
    float diff = fabs(a - b);
    if (diff > 180.0) diff = 360.0 - diff;
    return diff;
}
