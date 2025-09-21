#include "WebServer.h"
#include "../Configuration/Configuration.h"
#include "../MotorController/MotorController.h"
#include "../LimitSwitch/LimitSwitch.h"
#include <Arduino.h>

// Global instance
WebServerClass webServer;

WebServerClass::WebServerClass() : server(80), ws("/ws"), initialized(false)
{
}

bool WebServerClass::begin()
{
    Serial.println("Initializing Web Server...");

    // Initialize SPIFFS first
    if (!setupSPIFFS())
    {
        Serial.println("SPIFFS initialization failed");
        return false;
    }

    // Configure WiFiManager
    wm.setConfigPortalTimeout(180); // 3 minutes timeout
    wm.setAPStaticIPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));

    // Try to connect to saved WiFi or start config portal
    if (!wm.autoConnect("LilyGo-MotionController"))
    {
        Serial.println("Failed to connect to WiFi");
        return false;
    }

    Serial.println("WiFi connected successfully");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Setup web server routes and WebSocket
    setupRoutes();
    setupWebSocket();

    // Initialize ElegantOTA
    ElegantOTA.begin(&server);
    ElegantOTA.setAutoReboot(true); // Handle reboots manually

    // Start the server
    server.begin();
    Serial.println("Web Server started");

    initialized = true;
    return true;
}

bool WebServerClass::setupSPIFFS()
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return false;
    }

    Serial.println("SPIFFS mounted successfully");
    return true;
}

void WebServerClass::setupRoutes()
{
    // Serve static files from SPIFFS
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // API endpoints
    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request)
              { handleAPI(request); });

    server.on("/api/move", HTTP_POST, [this](AsyncWebServerRequest *request)
              {
        if (request->hasParam("position", true) && request->hasParam("speed", true)) {
            long position = request->getParam("position", true)->value().toInt();
            int speed = request->getParam("speed", true)->value().toInt();

            if (!limitSwitch.isAnyTriggered()) {
                motorController.moveTo(position, speed);
                request->send(200, "application/json", "{\"status\":\"moving\"}");
            } else {
                request->send(400, "application/json", "{\"error\":\"limit switch triggered\"}");
            }
        } else {
            request->send(400, "application/json", "{\"error\":\"missing parameters\"}");
        } });

    server.on("/api/stop", HTTP_POST, [this](AsyncWebServerRequest *request)
              {
        motorController.emergencyStop();
        request->send(200, "application/json", "{\"status\":\"stopped\"}"); });

    server.on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest *request)
              {
        motorController.clearEmergencyStop();
        request->send(200, "application/json", "{\"status\":\"reset\"}"); });

    // Configuration endpoints
    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        JsonDocument doc;
        doc["maxSpeed"] = config.getMaxSpeed();
        doc["acceleration"] = config.getAcceleration();
        doc["minLimit"] = config.getMinLimit();
        doc["maxLimit"] = config.getMaxLimit();

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response); });
}

void WebServerClass::setupWebSocket()
{
    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len)
               { onWebSocketEvent(server, client, type, arg, data, len); });

    server.addHandler(&ws);
}

void WebServerClass::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                                      AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        // Send current status to new client
        broadcastStatus();
        break;

    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;

    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;

    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}

void WebServerClass::handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {
        data[len] = 0; // Null terminate

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, (char *)data);

        if (error)
        {
            Serial.printf("WebSocket JSON parse error: %s\n", error.c_str());
            return;
        }

        String command = doc["command"];

        if (command == "move")
        {
            if (doc["position"].is<long>() && doc["speed"].is<int>())
            {
                long position = doc["position"];
                int speed = doc["speed"];

                if (!limitSwitch.isAnyTriggered())
                {
                    motorController.moveTo(position, speed);
                }
                else
                {
                    ws.textAll("{\"error\":\"limit switch triggered\"}");
                }
            }
        }
        else if (command == "stop")
        {
            motorController.emergencyStop();
        }
        else if (command == "reset")
        {
            motorController.clearEmergencyStop();
        }
        else if (command == "status")
        {
            broadcastStatus();
        }
    }
}

void WebServerClass::handleAPI(AsyncWebServerRequest *request)
{
    JsonDocument doc;
    doc["position"] = motorController.getCurrentPosition();
    doc["isMoving"] = motorController.isMoving();
    doc["emergencyStop"] = motorController.isEmergencyStopActive();
    doc["limitSwitches"]["min"] = limitSwitch.isMinTriggered();
    doc["limitSwitches"]["max"] = limitSwitch.isMaxTriggered();
    doc["limitSwitches"]["any"] = limitSwitch.isAnyTriggered();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerClass::broadcastStatus()
{
    if (!initialized)
        return;

    JsonDocument doc;
    doc["type"] = "status";
    doc["position"] = motorController.getCurrentPosition();
    doc["isMoving"] = motorController.isMoving();
    doc["emergencyStop"] = motorController.isEmergencyStopActive();
    doc["limitSwitches"]["min"] = limitSwitch.isMinTriggered();
    doc["limitSwitches"]["max"] = limitSwitch.isMaxTriggered();
    doc["limitSwitches"]["any"] = limitSwitch.isAnyTriggered();

    String message;
    serializeJson(doc, message);
    ws.textAll(message);
}

void WebServerClass::broadcastPosition(long position)
{
    if (!initialized)
        return;

    JsonDocument doc;
    doc["type"] = "position";
    doc["position"] = position;

    String message;
    serializeJson(doc, message);
    ws.textAll(message);
}

void WebServerClass::update()
{
    if (!initialized)
        return;

    // Handle ElegantOTA
    ElegantOTA.loop();

    // Cleanup disconnected WebSocket clients
    ws.cleanupClients();
}