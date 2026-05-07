#include "aprs_is.h"

AprsIs::AprsIs(const char* server, uint16_t port, const char* callsign,
               int ssid, const char* passcode)
    : _server(server), _port(port), _callsign(callsign),
      _ssid(ssid), _passcode(passcode) {}

bool AprsIs::connect() {
    if (_client.connected()) {
        return true;
    }

    Serial.printf("[APRS-IS] Connecting to %s:%d...\n", _server, _port);

    if (!_client.connect(_server, _port)) {
        Serial.println("[APRS-IS] Connection failed");
        return false;
    }

    // Esperar banner del servidor
    unsigned long start = millis();
    while (!_client.available() && millis() - start < 5000) {
        delay(100);
    }

    if (_client.available()) {
        String banner = _client.readStringUntil('\n');
        Serial.printf("[APRS-IS] Server: %s\n", banner.c_str());
    }

    // Enviar login
    String login = _buildLoginLine();
    Serial.printf("[APRS-IS] Login: %s\n", login.c_str());
    _client.println(login);

    // Esperar respuesta de login
    start = millis();
    while (!_client.available() && millis() - start < 5000) {
        delay(100);
    }

    if (_client.available()) {
        String response = _client.readStringUntil('\n');
        Serial.printf("[APRS-IS] Response: %s\n", response.c_str());

        if (response.indexOf("verified") == -1 && response.indexOf("unverified") == -1) {
            Serial.println("[APRS-IS] Unexpected login response");
        }
    }

    return _client.connected();
}

void AprsIs::disconnect() {
    if (_client.connected()) {
        _client.stop();
    }
    Serial.println("[APRS-IS] Disconnected");
}

bool AprsIs::isConnected() {
    return _client.connected();
}

bool AprsIs::sendPacket(const String& packet) {
    if (!_client.connected()) {
        Serial.println("[APRS-IS] Not connected, reconnecting...");
        if (!connect()) {
            return false;
        }
    }

    Serial.printf("[APRS-IS] TX: %s\n", packet.c_str());
    _client.println(packet);
    return true;
}

String AprsIs::_buildLoginLine() {
    char buf[128];
    snprintf(buf, sizeof(buf), "user %s-%d pass %s vers CoreInkMeteo 1.0",
             _callsign, _ssid, _passcode);
    return String(buf);
}
