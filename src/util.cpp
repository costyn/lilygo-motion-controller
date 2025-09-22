#include "util.h"

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