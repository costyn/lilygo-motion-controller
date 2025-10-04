// Mock implementations for testing
#include <string>
#include <cstdarg>

typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_DEBUG = 3
} log_level_t;

// Provide actual implementation for linking
void logPrint(log_level_t level, const char* function, const char* format, ...) {
    // No-op for tests
}

std::string timeToString() {
    return "00:00:00.000";
}

float fmap(float x, float a, float b, float c, float d) {
    return 0.0f;
}
