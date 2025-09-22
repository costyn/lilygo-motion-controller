#include "WebServer.h"
#include "../Configuration/Configuration.h"
#include "../MotorController/MotorController.h"
#include "../LimitSwitch/LimitSwitch.h"
#include "util.h"
#include <Arduino.h>

// Global instance
WebServerClass webServer;

WebServerClass::WebServerClass() : server(80), ws("/ws"), initialized(false)
{
}

bool WebServerClass::begin()
{
    constexpr const char *SGN = "WebServerClass::begin()";
    Serial.println("Initializing Web Server...");

    // Initialize SPIFFS first
    if (!setupSPIFFS())
    {
        Serial.println("SPIFFS initialization failed");
        return false;
    }

    // Configure WiFiManager
    wm.setConfigPortalTimeout(30); // 3 minutes timeout

    // Try to connect to saved WiFi or start config portal
    if (!wm.autoConnect("LilyGo-MotionController"))
    {
        Serial.println("Failed to connect to WiFi");
        return false;
    }

    Serial.printf("%s: %s: Connected to wifi! IP: %s\n", SGN, timeToString().c_str(), WiFi.localIP());

    // Setup web server routes and WebSocket
    setupRoutes();
    setupWebSocket();

    // Initialize ElegantOTA
    ElegantOTA.begin(&server);
    ElegantOTA.setAutoReboot(true); // Handle reboots manually

    // Start the server
    server.begin();
    Serial.printf("%s: %s: Webserver started. URL http://%s/\n", timeToString().c_str(), SGN, WiFi.localIP());

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
        doc["useStealthChop"] = config.getUseStealthChop();

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response); });

    server.on("/api/config", HTTP_POST, [this](AsyncWebServerRequest *request)
              {
        bool updated = false;
        JsonDocument response;

        if (request->hasParam("maxSpeed", true)) {
            long maxSpeed = request->getParam("maxSpeed", true)->value().toInt();
            config.setMaxSpeed(maxSpeed);
            motorController.setMaxSpeed(maxSpeed);
            updated = true;
        }

        if (request->hasParam("acceleration", true)) {
            long acceleration = request->getParam("acceleration", true)->value().toInt();
            config.setAcceleration(acceleration);
            motorController.setAcceleration(acceleration);
            updated = true;
        }

        if (request->hasParam("useStealthChop", true)) {
            bool useStealthChop = request->getParam("useStealthChop", true)->value() == "true";
            config.setUseStealthChop(useStealthChop);
            motorController.setTMCMode(useStealthChop);
            updated = true;
        }

        if (updated) {
            config.saveConfiguration();
            response["status"] = "success";
            response["message"] = "Configuration updated";
        } else {
            response["status"] = "error";
            response["message"] = "No valid parameters provided";
        }

        String responseStr;
        serializeJson(response, responseStr);
        request->send(200, "application/json", responseStr); });
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
        else if (command == "getConfig")
        {
            JsonDocument configDoc;
            configDoc["type"] = "config";
            configDoc["maxSpeed"] = config.getMaxSpeed();
            configDoc["acceleration"] = config.getAcceleration();
            configDoc["minLimit"] = config.getMinLimit();
            configDoc["maxLimit"] = config.getMaxLimit();
            configDoc["useStealthChop"] = config.getUseStealthChop();

            String configMessage;
            serializeJson(configDoc, configMessage);
            ws.textAll(configMessage);
        }
        else if (command == "setConfig")
        {
            bool updated = false;

            if (doc["maxSpeed"].is<long>())
            {
                config.setMaxSpeed(doc["maxSpeed"]);
                motorController.setMaxSpeed(doc["maxSpeed"]);
                updated = true;
            }

            if (doc["acceleration"].is<long>())
            {
                config.setAcceleration(doc["acceleration"]);
                motorController.setAcceleration(doc["acceleration"]);
                updated = true;
            }

            if (doc["useStealthChop"].is<bool>())
            {
                config.setUseStealthChop(doc["useStealthChop"]);
                motorController.setTMCMode(doc["useStealthChop"]);
                updated = true;
            }

            if (updated)
            {
                config.saveConfiguration();
                ws.textAll("{\"type\":\"configUpdated\",\"status\":\"success\"}");

                // Broadcast updated config to all clients
                JsonDocument configDoc;
                configDoc["type"] = "config";
                configDoc["maxSpeed"] = config.getMaxSpeed();
                configDoc["acceleration"] = config.getAcceleration();
                configDoc["minLimit"] = config.getMinLimit();
                configDoc["maxLimit"] = config.getMaxLimit();
                configDoc["useStealthChop"] = config.getUseStealthChop();

                String configMessage;
                serializeJson(configDoc, configMessage);
                ws.textAll(configMessage);
            }
            else
            {
                ws.textAll("{\"type\":\"error\",\"message\":\"Invalid configuration parameters\"}");
            }
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