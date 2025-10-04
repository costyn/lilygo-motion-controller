#pragma once

#include <map>
#include <string>

// Global storage for mock preferences (persists across instances)
static std::map<std::string, long> globalLongValues;
static std::map<std::string, bool> globalBoolValues;

// Mock Preferences class for testing (replaces ESP32 Preferences)
class Preferences {
private:
    bool isOpen = false;

public:
    bool begin(const char* name, bool readOnly) {
        isOpen = true;
        return true;
    }

    void end() {
        isOpen = false;
    }

    long getLong(const char* key, long defaultValue = 0) {
        auto it = globalLongValues.find(key);
        if (it != globalLongValues.end()) {
            return it->second;
        }
        return defaultValue;
    }

    bool getBool(const char* key, bool defaultValue = false) {
        auto it = globalBoolValues.find(key);
        if (it != globalBoolValues.end()) {
            return it->second;
        }
        return defaultValue;
    }

    void putLong(const char* key, long value) {
        globalLongValues[key] = value;
    }

    void putBool(const char* key, bool value) {
        globalBoolValues[key] = value;
    }

    void clear() {
        globalLongValues.clear();
        globalBoolValues.clear();
    }

    // Test helpers
    bool hasKey(const char* key) {
        return globalLongValues.find(key) != globalLongValues.end() ||
               globalBoolValues.find(key) != globalBoolValues.end();
    }
};
