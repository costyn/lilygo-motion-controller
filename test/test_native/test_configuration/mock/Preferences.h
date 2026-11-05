#pragma once

#include <map>
#include <string>

// Global storage for mock preferences (persists across instances)
static std::map<std::string, long> globalLongValues;
static std::map<std::string, bool> globalBoolValues;
static std::map<std::string, std::string> globalStringValues;

// Stand-in for Arduino's String
struct String : std::string {
    using std::string::string;
    String(const std::string& value) : std::string(value) {}
};

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

    String getString(const char* key, const String& defaultValue = String()) {
        auto it = globalStringValues.find(key);
        if (it != globalStringValues.end()) {
            return it->second;
        }
        return defaultValue;
    }

    void putString(const char* key, const String& value) {
        globalStringValues[key] = value;
    }

    bool remove(const char* key) {
        return globalStringValues.erase(key) > 0;
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
        globalStringValues.clear();
    }

    // Test helpers
    bool hasKey(const char* key) {
        return globalLongValues.find(key) != globalLongValues.end() ||
               globalBoolValues.find(key) != globalBoolValues.end();
    }
};
