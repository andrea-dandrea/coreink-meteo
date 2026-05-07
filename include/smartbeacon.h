#ifndef SMARTBEACON_H
#define SMARTBEACON_H

#include <Arduino.h>

/**
 * Algoritmo SmartBeaconing per APRS.
 * Adatta l'intervallo di beacon in base a:
 * - Velocità: più veloce = beacon più frequenti
 * - Cambio di rotta: svolta significativa = beacon immediato
 */
class SmartBeacon {
public:
    SmartBeacon(float fastSpeed, int fastRate,
                float slowSpeed, int slowRate,
                int turnMinAngle, int turnSlope, int turnTime);

    /**
     * Aggiorna lo stato con i dati GPS correnti.
     * Restituisce true se è il momento di inviare un beacon.
     *
     * speed_kmh: velocità attuale in km/h
     * course: rotta attuale in gradi (0-360)
     */
    bool shouldBeacon(float speed_kmh, float course);

    /** Forza il reset del timer (dopo un invio manuale) */
    void reset();

    /** Restituisce i secondi trascorsi dall'ultimo beacon */
    unsigned long secondsSinceLastBeacon();

private:
    float _fastSpeed;
    int _fastRate;
    float _slowSpeed;
    int _slowRate;
    int _turnMinAngle;
    int _turnSlope;
    int _turnTime;

    float _lastCourse;
    unsigned long _lastBeaconTime;
    unsigned long _lastTurnTime;
    bool _firstFix;

    int _calcInterval(float speed_kmh);
    float _courseDiff(float a, float b);
};

#endif
