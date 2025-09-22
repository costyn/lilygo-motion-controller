#include "util.h"
#include <stdarg.h>

// Forward declaration for WebServer debug streaming
// This will be linked when WebServer module is included
extern void broadcastDebugMessage(const String& message) __attribute__((weak));

std::string timeToString()
{
    char myString[20];
    unsigned long nowMillis = millis();
    unsigned int seconds = nowMillis / 1000;
    unsigned int remainder = nowMillis % 1000;

    // Simplified format: HH:MM:SS.mmm (no days, easier to parse)
    byte hours = (seconds / 3600) % 24; // Wrap at 24 hours
    seconds %= 3600;
    byte minutes = seconds / 60;
    seconds %= 60;

    snprintf(myString, 20, "%02d:%02d:%02d.%03d", hours, minutes, seconds, remainder);
    return std::string(myString);
}

float fmap(float x, float a, float b, float c, float d)
{
    float f = x / (b - a) * (d - c) + c;
    return f;
}

void logPrint(log_level_t level, const char* function, const char* format, ...)
{
    // Skip if log level is below threshold
    if (level > LOG_LEVEL) {
        return;
    }

    // Get log level string
    const char* levelStr;
    switch (level) {
        case LOG_LEVEL_ERROR: levelStr = "ERROR"; break;
        case LOG_LEVEL_WARN:  levelStr = "WARN";  break;
        case LOG_LEVEL_INFO:  levelStr = "INFO";  break;
        case LOG_LEVEL_DEBUG: levelStr = "DEBUG"; break;
        default:              levelStr = "UNKNOWN"; break;
    }

    // Format the user message
    char userMessage[256];
    va_list args;
    va_start(args, format);
    vsnprintf(userMessage, sizeof(userMessage), format, args);
    va_end(args);

    // Create the full log message
    char logMessage[512];
    snprintf(logMessage, sizeof(logMessage), "[%s] [%s] [%s]: %s",
             timeToString().c_str(), levelStr, function, userMessage);

    // Output to Serial
    Serial.println(logMessage);

    // Output to debug WebSocket if available (weak linkage)
    if (broadcastDebugMessage) {
        broadcastDebugMessage(String(logMessage));
    }
}