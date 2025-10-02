# Expert Detail Answers

**Phase:** Technical Requirements Clarification
**Date:** 2025-10-02

---

## Q1: Should the jog distance scale with the configured limit range, or use a fixed step size?

**Answer:** User clarified this question was based on misunderstanding. The requirement is for **continuous movement while button held** (Option B from context discussion), not incremental steps.

**Selected Approach**: Implement true continuous jogging with backend support:
- New WebSocket commands: `jogStart` (with direction: forward/backward) and `jogStop`
- Backend implements continuous movement in specified direction
- Movement continues until `jogStop` received or limit switch triggered
- Requires both C++ backend and TypeScript frontend changes

User confirmed: "I like option B a lot better. Backend is in this project codebase, so shouldn't be too hard to fix. Go for option B"

---

## Q2: Should the Motor Config dialog use Radix Dialog (modal overlay) or a separate full-page view?

**Answer:** Default - Radix Dialog modal overlay (Option A)
- Stays on same page with dimmed background
- Simpler implementation, no routing needed
- Matches existing UI patterns
- Mobile-friendly

---

## Q3: Should limit positions (minLimit/maxLimit) be editable in the config UI, or only via the limit switch learning process?

**Answer:** Default - No, keep limits read-only in UI
- Only editable via physical limit switch learning
- Safer approach prevents invalid limits
- Matches documented workflow in README.md

**Important Note**: User mentioned limit switch poller is currently commented out due to wiring/connector issues. Leave it commented out - do not modify limit switch code.

---

## Q4: Should the config UI show a preview of what will change before applying, or just highlight modified fields?

**Answer:** Option C - Simple form with no preview
- Keep UI clean
- Values are self-explanatory
- "Revert" button provides safety net

---

## Q5: Should config validation errors prevent the Apply button from being enabled, or show warnings but allow submission?

**Answer:** Option A - Disable Apply button until all fields valid
- Red error text below invalid fields
- Prevents invalid submissions
- Clear feedback
- Matches modern form UX

**Action Item**: User needs help determining sane min/max validation values for maxSpeed, acceleration, etc. This should be researched and documented during implementation based on:
- TMC2209 driver capabilities
- Motor specifications (17HS19-2004S1)
- Encoder resolution (MT6816)
- Safe operational ranges

---

