#ifndef UTIL_H
#define UTIL_H
#include <Arduino.h>

// Device naming configuration
#define DEVICE_NAME "LilyGo-MotionController"
#define DEVICE_HOSTNAME "lilygo-motioncontroller"

// Log levels
typedef enum
{
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_DEBUG = 3
} log_level_t;

// Default log level (can be overridden with build flags)
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DEBUG
#endif

// Logging macros
#define LOG_ERROR(fmt, ...) logPrint(LOG_LEVEL_ERROR, __func__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) logPrint(LOG_LEVEL_WARN, __func__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) logPrint(LOG_LEVEL_INFO, __func__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) logPrint(LOG_LEVEL_DEBUG, __func__, fmt, ##__VA_ARGS__)

// Functions
std::string timeToString();
float fmap(float x, float a, float b, float c, float d);
void logPrint(log_level_t level, const char *function, const char *format, ...);

#endif