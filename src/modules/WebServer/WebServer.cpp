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
    lastPositionBroadcast = 0;
    lastStatusBroadcast = 0;
    wasMovingLastUpdate = false;
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

    // Read-only REST API endpoints (monitoring/debugging only)
    // For control operations, use WebSocket interface at /ws
    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request)
              { handleAPI(request); });

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

                    // Immediate status broadcast on movement start
                    LOG_INFO("Movement started to position %ld - broadcasting initial status", position);
                    broadcastStatus();
                    lastPositionBroadcast = millis();
                    lastStatusBroadcast = millis();
                }
                else
                {
                    ws.textAll("{\"error\":\"limit switch triggered\"}");
                }
            }
        }
        else if (command == "stop")
        {
            motorController.stop();

            // Immediate broadcast of emergency stop state
            LOG_WARN("Emergency stop triggered - broadcasting status");
            broadcastStatus();
        }
        else if (command == "jogStart")
        {
            if (doc["direction"].is<const char *>() || doc["direction"].is<String>())
            {
                String direction = doc["direction"].as<String>();

                if (!limitSwitch.isAnyTriggered() && !motorController.isEmergencyStopActive())
                {
                    // Calculate jog speed (30% of max speed)
                    int jogSpeed = config.getMaxSpeed() * 0.3;

                    if (direction == "forward")
                    {
                        // Move to max limit at jog speed
                        long targetPosition = config.getMaxLimit();
                        motorController.moveTo(targetPosition, jogSpeed);
                        LOG_INFO("Jog started: forward to %ld at speed %d", targetPosition, jogSpeed);
                    }
                    else if (direction == "backward")
                    {
                        // Move to min limit at jog speed
                        long targetPosition = config.getMinLimit();
                        motorController.moveTo(targetPosition, jogSpeed);
                        LOG_INFO("Jog started: backward to %ld at speed %d", targetPosition, jogSpeed);
                    }

                    // Broadcast status to show movement started
                    broadcastStatus();
                    lastPositionBroadcast = millis();
                    lastStatusBroadcast = millis();
                }
                else
                {
                    ws.textAll("{\"type\":\"error\",\"message\":\"Cannot jog: limit or emergency stop active\"}");
                }
            }
        }
        else if (command == "jogStop")
        {
            motorController.stopGently();
            LOG_INFO("Jog stopped");
            broadcastStatus();
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

            if (doc["minLimit"].is<long>())
            {
                config.setLimitPos1(doc["minLimit"]);
                updated = true;
            }

            if (doc["maxLimit"].is<long>())
            {
                config.setLimitPos2(doc["maxLimit"]);
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

    // Automatic status broadcasting during movement
    // Only broadcast if actually moving (not stopped by emergency stop)
    bool isCurrentlyMoving = motorController.isMoving() && !motorController.isEmergencyStopActive();
    unsigned long currentMillis = millis();

    if (isCurrentlyMoving)
    {
        // Send position updates every 100ms during movement
        if (currentMillis - lastPositionBroadcast >= POSITION_BROADCAST_INTERVAL_MS)
        {
            LOG_DEBUG("Broadcasting position update: %ld", motorController.getCurrentPosition());
            broadcastPosition(motorController.getCurrentPosition());
            lastPositionBroadcast = currentMillis;
        }

        // Send full status every 500ms during movement
        if (currentMillis - lastStatusBroadcast >= STATUS_BROADCAST_INTERVAL_MS)
        {
            LOG_DEBUG("Broadcasting full status (movement active)");
            broadcastStatus();
            lastStatusBroadcast = currentMillis;
        }

        // Update movement tracking for next iteration
        wasMovingLastUpdate = true;
    }
    else if (wasMovingLastUpdate)
    {
        // Movement just completed - send final status
        LOG_INFO("Movement completed - sending final status");
        broadcastStatus();
        wasMovingLastUpdate = false;
    }
}