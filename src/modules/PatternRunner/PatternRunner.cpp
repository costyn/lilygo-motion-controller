#include "PatternRunner.h"
#include "../MotorController/MotorController.h"
#include "../Configuration/Configuration.h"
#include "util.h"

PatternRunner patternRunner;

// Guidelines from the lamp's description, in % of travel. Its blades turn 72° from closed
// to open, assumed here to be 20° to 92°.
static const char DEFAULT_PROGRAM[] = R"({
"presets":[0,10,25,45,75,85,90,100],
"patterns":[
{"name":"Butterfly","repeat":0,"segments":[
{"to":97,"speed":50,"ramp":0.5,"hold":2},
{"to":28,"speed":50,"ramp":0.5,"hold":2}]},
{"name":"Breath","repeat":0,"segments":[
{"to":100,"speed":20,"ramp":0.2,"hold":2},
{"to":35,"speed":20,"ramp":0.2,"hold":4}]}]})";

PatternRunner::PatternRunner()
    : requested(nullptr), active(nullptr), running(false), startRequested(false), round(0),
      generation(0), nextSegment(-1), holdSeconds(0), stoppedAt(0)
{
    program.presetCount = 0;
    program.patternCount = 0;
}

void PatternRunner::begin()
{
    String saved = config.getProgramJson();
    JsonDocument doc;
    if (saved.length() > 0 && !deserializeJson(doc, saved) && !parseProgram(doc.as<JsonVariantConst>(), staging))
    {
        program = staging;
        LOG_INFO("Loaded %u patterns and %u presets", program.patternCount, program.presetCount);
        return;
    }

    if (saved.length() > 0)
    {
        LOG_WARN("Saved patterns are invalid, using the defaults");
    }
    doc.clear();
    deserializeJson(doc, DEFAULT_PROGRAM);
    const char *error = parseProgram(doc.as<JsonVariantConst>(), staging);
    if (error)
    {
        LOG_ERROR("Default patterns are invalid: %s", error);
        return;
    }
    program = staging;
}

static bool readNumber(JsonVariantConst value, float minimum, float maximum, float &out)
{
    if (!value.is<float>())
        return false;
    out = value.as<float>();
    return out >= minimum && out <= maximum;
}

const char *PatternRunner::parseProgram(JsonVariantConst source, Program &target)
{
    JsonArrayConst presets = source["presets"].as<JsonArrayConst>();
    JsonArrayConst patterns = source["patterns"].as<JsonArrayConst>();
    if (presets.isNull() || patterns.isNull())
        return "Patterns need a presets list and a patterns list";
    if (presets.size() > MAX_PRESETS)
        return "Too many presets, 12 at most";
    if (patterns.size() > MAX_PATTERNS)
        return "Too many patterns, 8 at most";

    target.presetCount = 0;
    for (JsonVariantConst preset : presets)
    {
        if (!readNumber(preset, 0, 100, target.presets[target.presetCount]))
            return "Presets must be between 0 and 100";
        target.presetCount++;
    }

    target.patternCount = 0;
    for (JsonVariantConst source : patterns)
    {
        Pattern &pattern = target.patterns[target.patternCount];

        const char *name = source["name"] | "";
        if (strlen(name) == 0 || strlen(name) > MAX_NAME_LENGTH)
            return "Each pattern needs a name of 1 to 24 characters";
        strncpy(pattern.name, name, MAX_NAME_LENGTH);
        pattern.name[MAX_NAME_LENGTH] = '\0';

        float repeat = 0;
        if (!source["repeat"].isNull() && !readNumber(source["repeat"], 0, 100, repeat))
            return "Repeat must be between 0 (until stopped) and 100";
        pattern.repeat = (uint8_t)repeat;

        JsonArrayConst segments = source["segments"].as<JsonArrayConst>();
        if (segments.isNull() || segments.size() == 0 || segments.size() > MAX_SEGMENTS)
            return "Each pattern needs 1 to 8 segments";

        pattern.segmentCount = 0;
        for (JsonVariantConst item : segments)
        {
            Segment &segment = pattern.segments[pattern.segmentCount];
            segment.seconds = 0;
            segment.speed = 0;
            segment.ramp = DEFAULT_RAMP;
            segment.hold = 0;

            if (!readNumber(item["to"], 0, 100, segment.to))
                return "Each segment needs a \"to\" between 0 and 100";
            if (!item["seconds"].isNull() && !readNumber(item["seconds"], 0.1f, 3600, segment.seconds))
                return "Seconds must be between 0.1 and 3600";
            if (!item["speed"].isNull() && !readNumber(item["speed"], 0.1f, 100, segment.speed))
                return "Speed must be between 0.1 and 100 (% of Max Speed)";
            if (segment.seconds == 0 && segment.speed == 0)
                return "Each segment needs seconds or speed";
            if (!item["ramp"].isNull() && !readNumber(item["ramp"], MIN_RAMP, MAX_RAMP, segment.ramp))
                return "Ramp must be between 0.05 and 0.5";
            if (!item["hold"].isNull() && !readNumber(item["hold"], 0, 3600, segment.hold))
                return "Hold must be between 0 and 3600 seconds";

            pattern.segmentCount++;
        }
        target.patternCount++;
    }
    return nullptr;
}

const char *PatternRunner::setProgram(JsonVariantConst source)
{
    if (isRunning())
        return "Stop the pattern before changing patterns";

    const char *error = parseProgram(source, staging);
    if (error)
        return error;

    String json;
    serializeJson(source, json);
    if (json.length() > MAX_PROGRAM_BYTES)
        return "Patterns are too long to save";

    program = staging;
    config.saveProgramJson(json);
    return nullptr;
}

const char *PatternRunner::resetProgram()
{
    if (isRunning())
        return "Stop the pattern before changing patterns";

    config.saveProgramJson("");
    begin();
    return nullptr;
}

void PatternRunner::toJson(JsonObject target) const
{
    JsonArray presets = target["presets"].to<JsonArray>();
    for (uint8_t i = 0; i < program.presetCount; i++)
        presets.add(program.presets[i]);

    JsonArray patterns = target["patterns"].to<JsonArray>();
    for (uint8_t i = 0; i < program.patternCount; i++)
    {
        const Pattern &pattern = program.patterns[i];
        JsonObject item = patterns.add<JsonObject>();
        item["name"] = pattern.name;
        item["repeat"] = pattern.repeat;
        JsonArray segments = item["segments"].to<JsonArray>();
        for (uint8_t j = 0; j < pattern.segmentCount; j++)
        {
            const Segment &segment = pattern.segments[j];
            JsonObject out = segments.add<JsonObject>();
            out["to"] = segment.to;
            if (segment.seconds > 0)
                out["seconds"] = segment.seconds;
            else
                out["speed"] = segment.speed;
            out["ramp"] = segment.ramp;
            out["hold"] = segment.hold;
        }
    }
}

const char *PatternRunner::getName() const
{
    if (startRequested && requested)
        return requested->name;
    return running && active ? active->name : "";
}

uint8_t PatternRunner::getRepeat() const
{
    const Pattern *pattern = startRequested ? requested : active;
    return isRunning() && pattern ? pattern->repeat : 0;
}

const char *PatternRunner::startPattern(int index)
{
    if (index < 0 || index >= program.patternCount)
        return "No such pattern";
    return start(&program.patterns[index]);
}

const char *PatternRunner::startCycles(int count, int speed)
{
    if (count < 1 || count > MAX_CYCLES)
        return "Cycles must be between 1 and 10";
    if (isRunning())
        return "Stop the pattern first";

    long maxSpeed = config.getMaxSpeed();
    float percent = maxSpeed > 0 ? 100.0f * speed / maxSpeed : 100;

    strcpy(cyclePattern.name, "Cycle");
    cyclePattern.repeat = count;
    cyclePattern.segmentCount = 2;
    // A negative ramp keeps the configured Acceleration, as for any other move
    cyclePattern.segments[0] = {100, 0, percent, -1, 0};
    cyclePattern.segments[1] = {0, 0, percent, -1, 0};
    return start(&cyclePattern);
}

const char *PatternRunner::start(const Pattern *pattern)
{
    if (isRunning())
        return "Stop the pattern first";

    const char *error = motorController.beginProgram();
    if (error)
        return error;

    requested = pattern;
    round = 1;
    startRequested = true;
    LOG_INFO("Starting pattern %s", pattern->name);
    return nullptr;
}

void PatternRunner::update()
{
    if (startRequested)
    {
        active = requested;
        startRequested = false;
        if (!motorController.isProgramActive())
        {
            return; // Stopped before it began
        }
        running = true;
        round = 1;
        generation = generation + 1;
        nextSegment = -1;
        advance();
        return;
    }

    if (!running)
        return;

    if (!motorController.isProgramActive())
    {
        running = false;
        LOG_INFO("Pattern %s stopped", active->name);
        return;
    }

    if (motorController.isMoving() || motorController.isCorrectionPending())
    {
        stoppedAt = 0;
        return;
    }

    unsigned long now = millis();
    if (stoppedAt == 0)
    {
        stoppedAt = now == 0 ? 1 : now;
    }

    unsigned long pause = (unsigned long)(holdSeconds * 1000.0f);
    if (pause < MIN_PAUSE_MS)
        pause = MIN_PAUSE_MS;
    if (now - stoppedAt >= pause)
    {
        advance();
    }
}

void PatternRunner::advance()
{
    if (nextSegment >= active->segmentCount)
    {
        if (active->repeat > 0 && round >= active->repeat)
        {
            finish();
            return;
        }
        round = round + 1;
        nextSegment = 0;
    }

    // First, move to where the last segment ends, so the first round is complete
    const Segment &segment = active->segments[nextSegment < 0 ? active->segmentCount - 1 : nextSegment];
    nextSegment++;

    MovePlan plan = planSegment(segment, motorController.getCurrentPosition(), config.getMinLimit(),
                                config.getMaxLimit(), config.getMaxSpeed(), MotorController::MAX_ACCELERATION);
    holdSeconds = segment.hold;
    stoppedAt = 0;

    if (!motorController.startProgramMove(plan.target, plan.speed, plan.acceleration))
    {
        running = false;
        LOG_WARN("Pattern %s stopped: the motor can't move", active->name);
    }
}

void PatternRunner::finish()
{
    running = false;
    motorController.endProgram();
    LOG_INFO("Pattern %s finished after %u rounds", active->name, round);
}
