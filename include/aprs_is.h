#ifndef APRS_IS_H
#define APRS_IS_H

#include <Arduino.h>
#include <WiFiClient.h>

class AprsIs {
public:
    AprsIs(const char* server, uint16_t port, const char* callsign,
           int ssid, const char* passcode);

    bool connect();
    void disconnect();
    bool isConnected();
    bool sendPacket(const String& packet);

private:
    const char* _server;
    uint16_t _port;
    const char* _callsign;
    int _ssid;
    const char* _passcode;
    WiFiClient _client;

    String _buildLoginLine();
};

#endif
