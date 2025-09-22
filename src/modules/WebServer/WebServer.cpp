#include "WebServer.h"
#include "../Configuration/Configuration.h"
#include "../MotorController/MotorController.h"
#include "../LimitSwitch/LimitSwitch.h"
#include "util.h"
#include <Arduino.h>

// Global instance
WebServerClass webServer;

// DebugBuffer implementation
DebugBuffer::DebugBuffer() : head(0), tail(0), full(false) {}

void DebugBuffer::add(const String &message)
{
    buffer[head] = message;
    head = (head + 1) % DEBUG_BUFFER_SIZE;

    if (full)
    {
        tail = (tail + 1) % DEBUG_BUFFER_SIZE;
    }

    if (head == tail)
    {
        full = true;
    }
}

void DebugBuffer::sendHistoryTo(AsyncWebSocketClient *client)
{
    if (isEmpty())
    {
        return;
    }

    int start = full ? tail : 0;
    int end = head;

    for (int i = start; i != end; i = (i + 1) % DEBUG_BUFFER_SIZE)
    {
        client->text(buffer[i]);
    }
}

bool DebugBuffer::isEmpty() const
{
    return (!full && (head == tail));
}

// Global function for util.cpp weak linkage
void broadcastDebugMessage(const String &message)
{
    webServer.broadcastDebugMessage(message);
}

WebServerClass::WebServerClass() : server(80), ws("/ws"), debugWs("/debug"), initialized(false)
{
}

bool WebServerClass::begin()
{
    constexpr const char *SGN = "WebServerClass::begin()";
    LOG_INFO("Initializing Web Server...");

    // Initialize SPIFFS first
    if (!setupSPIFFS())
    {
        LOG_WARN("SPIFFS initialization failed");
        return false;
    }

    // Configure WiFiManager
    wm.setConfigPortalTimeout(30); // 3 minutes timeout

    // Try to connect to saved WiFi or start config portal
    if (!wm.autoConnect("LilyGo-MotionController"))
    {
        LOG_WARN("Failed to connect to WiFi");
        return false;
    }

    LOG_INFO("Connected to WiFi! IP: %s", WiFi.localIP().toString().c_str());

    // Setup mDNS
    if (!setupMDNS())
    {
        LOG_WARN("mDNS setup failed, device won't be accessible via hostname");
    }

    // Setup web server routes and WebSockets
    setupRoutes();
    setupWebSocket();
    setupDebugWebSocket();

    // Initialize ElegantOTA
    ElegantOTA.begin(&server);
    ElegantOTA.setAutoReboot(true); // Handle reboots manually

    // Start the server
    server.begin();
    LOG_INFO("Web server started. URLs: http://%s/ and http://%s.local/", WiFi.localIP().toString().c_str(), DEVICE_HOSTNAME);

    initialized = true;
    return true;
}

bool WebServerClass::setupSPIFFS()
{
    if (!SPIFFS.begin(true))
    {
        LOG_WARN("An Error has occurred while mounting SPIFFS");
        return false;
    }

    LOG_INFO("SPIFFS mounted successfully");
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

void WebServerClass::setupDebugWebSocket()
{
    debugWs.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client,
                           AwsEventType type, void *arg, uint8_t *data, size_t len)
                    { onDebugWebSocketEvent(server, client, type, arg, data, len); });

    server.addHandler(&debugWs);
}

bool WebServerClass::setupMDNS()
{
    esp_err_t err = mdns_init();
    if (err)
    {
        LOG_ERROR("mDNS Init failed: %d", err);
        return false;
    }

    mdns_hostname_set(DEVICE_HOSTNAME);
    mdns_instance_name_set(DEVICE_NAME);
    mdns_service_add(DEVICE_HOSTNAME, "_http", "_tcp", 80, NULL, 0);

    LOG_INFO("mDNS initialized. Device accessible at http://%s.local/", DEVICE_HOSTNAME);
    return true;
}

void WebServerClass::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                                      AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        LOG_INFO("WebSocket client #%u connected from %s", client->id(), client->remoteIP().toString().c_str());
        // Send current status to new client
        broadcastStatus();
        break;

    case WS_EVT_DISCONNECT:
        LOG_INFO("WebSocket client #%u disconnected", client->id());
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
            LOG_ERROR("WebSocket JSON parse error: %s", error.c_str());
            return;
        }

        // Log incoming command for debugging
        LOG_DEBUG("WebSocket raw data received: %s", (char *)data);

        // Check for command field (support both "command" and "cmd" for compatibility)
        String command;
        if (doc["command"].is<const char *>() || doc["command"].is<String>())
        {
            command = doc["command"].as<String>();
        }
        else if (doc["cmd"].is<const char *>() || doc["cmd"].is<String>())
        {
            command = doc["cmd"].as<String>();
            LOG_DEBUG("Using legacy 'cmd' field, consider updating webapp to use 'command'");
        }
        else
        {
            LOG_WARN("WebSocket message missing 'command' or 'cmd' field in: %s", (char *)data);
            return;
        }

        LOG_INFO("Processing WebSocket command: %s", command.c_str());

        if (command == "move" || command == "goto")
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

void WebServerClass::onDebugWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                                           AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        LOG_INFO("Debug WebSocket client #%u connected from %s", client->id(), client->remoteIP().toString().c_str());
        // Send a simple welcome message instead of history (to prevent flooding)
        client->text("[DEBUG] Debug WebSocket connected - showing real-time logs only");
        break;

    case WS_EVT_DISCONNECT:
        LOG_INFO("Debug WebSocket client #%u disconnected", client->id());
        break;

    case WS_EVT_DATA:
        // Debug WebSocket is read-only, ignore incoming messages
        break;

    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}

void WebServerClass::broadcastDebugMessage(const String &message)
{
    // Skip circular buffer entirely to prevent flooding
    // debugBuffer.add(message);

    // Simple real-time broadcast only
    if (debugWs.count() > 0)
    {
        debugWs.textAll(message);
    }
}

void WebServerClass::update()
{
    if (!initialized)
        return;

    // Handle ElegantOTA
    ElegantOTA.loop();

    // Cleanup disconnected WebSocket clients
    ws.cleanupClients();
    debugWs.cleanupClients();
}