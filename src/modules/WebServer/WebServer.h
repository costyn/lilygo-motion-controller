#pragma once

#include <WiFiManager.h> // MUST be included BEFORE ESPAsyncWebServer.h
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <SPIFFS.h>
#include <ElegantOTA.h>
#include <ArduinoJson.h>

class WebServerClass
{
private:
    AsyncWebServer server;
    AsyncWebSocket ws;
    WiFiManager wm;
    bool initialized;

    // WebSocket handlers
    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
    void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                          AwsEventType type, void *arg, uint8_t *data, size_t len);

    // HTTP handlers
    void handleRoot(AsyncWebServerRequest *request);
    void handleAPI(AsyncWebServerRequest *request);

    // Configuration
    void setupRoutes();
    void setupWebSocket();
    bool setupSPIFFS();

public:
    WebServerClass();
    bool begin();
    void update();
    void broadcastStatus();
    void broadcastPosition(long position);
};

extern WebServerClass webServer;