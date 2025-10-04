# Initial Request

**Date:** 2025-10-04
**Session:** Tech Debt Review & Code Quality Improvements

## User Request

Hello. During this session we are going to be handling some tech debt that has built up during the project. During code review I was doing I spotted some issues I'd like fixed. And I would also like you to do a code review of each module one by one (the C++ project as well as the webapp) and see what you think could be improved. These are the issues I spotted. Please see various .md files for more information about the project and previous requirements sessions in requirements/

## Scope
- Address identified tech debt issues
- Conduct comprehensive code review of C++ project modules
- Conduct comprehensive code review of webapp
- Identify improvement opportunities module-by-module

## User-Spotted Issues

1. **Abstraction layer needed**: DataModel between WebController and MotorController
   - WebController is doing too much calculations and knows too much about MotorController

2. **Hardware button logic in main.cpp**: Needs to be moved to separate component

3. **Duplicate/similar code (DRY violations)**:
   - `onSwitch1Pressed` and `onSwitch2Pressed` are very similar

4. **Naming inconsistency**: `stopGently()` has no corresponding JSON command but is just called `jogStop` (may be confusing)

5. **Code smell in handleWebSocketMessage()**:
   - Massive chains of if-else statements
   - Code duplication of log messages
   - `broadcastStatus()` calls littered around
