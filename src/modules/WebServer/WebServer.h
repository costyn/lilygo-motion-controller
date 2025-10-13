#pragma once

#include <WiFiManager.h> // MUST be included BEFORE ESPAsyncWebServer.h
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <SPIFFS.h>
#include <ElegantOTA.h>
#include <ArduinoJson.h>
#include <mdns.h>

// Simple circular buffer for debug messages
#define DEBUG_BUFFER_SIZE 100

// Broadcast timing intervals (milliseconds)
#define POSITION_BROADCAST_INTERVAL_MS 100
#define STATUS_BROADCAST_INTERVAL_MS 500

class DebugBuffer {
private:
    String buffer[DEBUG_BUFFER_SIZE];
    int head;
    int tail;
    bool full;

public:
    DebugBuffer();
    void add(const String& message);
    void sendHistoryTo(AsyncWebSocketClient* client);
    bool isEmpty() const;
};

class WebServerClass
{
private:
    AsyncWebServer server;
    AsyncWebSocket ws;           // Main control WebSocket at /ws
    AsyncWebSocket debugWs;      // Debug WebSocket at /debug
    WiFiManager wm;
    bool initialized;
    DebugBuffer debugBuffer;

    // Broadcast timing state
    unsigned long lastPositionBroadcast;
    unsigned long lastStatusBroadcast;
    bool wasMovingLastUpdate;
    long lastBroadcastPosition;  // Track last position to avoid sending duplicates

    // WebSocket handlers
    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
    void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                          AwsEventType type, void *arg, uint8_t *data, size_t len);

    // Command handlers (dispatch table pattern)
    void handleMoveCommand(JsonDocument& doc);
    void handleJogStartCommand(JsonDocument& doc);
    void handleJogStopCommand(JsonDocument& doc);
    void handleEmergencyStopCommand(JsonDocument& doc);
    void handleResetCommand(JsonDocument& doc);
    void handleStatusCommand(JsonDocument& doc);
    void handleGetConfigCommand(JsonDocument& doc);
    void handleSetConfigCommand(JsonDocument& doc);

    // Debug WebSocket handlers
    void onDebugWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                               AwsEventType type, void *arg, uint8_t *data, size_t len);

    // HTTP handlers
    void handleRoot(AsyncWebServerRequest *request);
    void handleAPI(AsyncWebServerRequest *request);

    // Configuration
    void setupRoutes();
    void setupWebSocket();
    void setupDebugWebSocket();
    bool setupSPIFFS();
    bool setupMDNS();

public:
    WebServerClass();
    bool begin();
    void update();
    void broadcastStatus();
    void broadcastConfig();
    void broadcastPosition(long position);
    void broadcastDebugMessage(const String& message);
};

extern WebServerClass webServer;

// Global function for util.cpp weak linkage
void broadcastDebugMessage(const String& message);