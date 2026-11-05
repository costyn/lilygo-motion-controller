#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "PatternPlan.h"

/**
 * PatternRunner - movement patterns and position presets
 *
 * A pattern is a list of segments, each a move to a % of travel with its own timing and
 * easing, then a hold. It starts by moving to its last segment's target, so every round
 * is complete, then repeats a set number of times or until stopped. Cycling between Min
 * and Max is a pattern built on request.
 *
 * Runs from InputTask every 100ms on the controller, so a pattern carries on with no
 * browser connected. Any other move, jog or stop ends it, through MotorController.
 *
 * Presets and patterns are saved together as JSON in NVRAM, with defaults built in.
 */
class PatternRunner
{
public:
    struct Program
    {
        float presets[MAX_PRESETS];
        uint8_t presetCount;
        Pattern patterns[MAX_PATTERNS];
        uint8_t patternCount;
    };

    static constexpr int MAX_CYCLES = 10;

private:
    Program program;
    Program staging; // Parsed into first, so a bad program changes nothing
    Pattern cyclePattern;

    const Pattern *volatile requested; // Set by startPattern(), picked up by update()
    const Pattern *active;
    volatile bool running;
    volatile bool startRequested;
    volatile uint8_t round;    // From 1
    volatile uint8_t generation; // Changes on each start, so a restart is broadcast
    int8_t nextSegment;        // -1 for the move to the start
    float holdSeconds;
    unsigned long stoppedAt;

    // Gives ClosedLoopTask time to check each move and correct lost steps
    static constexpr unsigned long MIN_PAUSE_MS = 500;
    static constexpr size_t MAX_PROGRAM_BYTES = 3500; // NVRAM strings top out near 4000

    const char *parseProgram(JsonVariantConst source, Program &target);
    const char *start(const Pattern *pattern);
    void advance();
    void finish();

public:
    PatternRunner();

    // Loads the saved program, or the defaults
    void begin();

    // Called from InputTask
    void update();

    // Return an error message, or nullptr
    const char *startPattern(int index);
    const char *startCycles(int count, int speed);
    const char *setProgram(JsonVariantConst source);
    const char *resetProgram();

    void toJson(JsonObject target) const;

    bool isRunning() const { return running || startRequested; }
    const char *getName() const;
    uint8_t getRound() const { return round; }
    uint8_t getRepeat() const;
    uint8_t getGeneration() const { return generation; }
};

extern PatternRunner patternRunner;
