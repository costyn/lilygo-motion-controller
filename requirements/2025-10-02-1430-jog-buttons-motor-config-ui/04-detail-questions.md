# Expert Detail Questions

**Phase:** Technical Requirements Clarification
**Date:** 2025-10-02

These questions clarify expected system behavior based on deep codebase analysis.

---

## Q1: Should the jog distance scale with the configured limit range, or use a fixed step size?

**Context**: Current jog moves 100 steps per press. If limit range is -5000 to +5000 (10,000 total), 100 steps is 1%. But if limits are -500 to +500 (1,000 total), 100 steps is 10% of range.

**Options**:
- **A**: Fixed distance (e.g., 500 steps regardless of limits)
- **B**: Percentage-based (e.g., 5% of limit range)
- **C**: Multiple jog speeds (small/medium/large buttons)

**Default if unknown:** Fixed distance of 500 steps (simple, predictable, 5x current distance for smoother motion)

---

## Q2: Should the Motor Config dialog use Radix Dialog (modal overlay) or a separate full-page view?

**Context**: You mentioned "button at the top to a 2nd page would also be acceptable." This could mean:
- **A**: Modal dialog overlay (stays on same page, dims background)
- **B**: Full-page replacement (navigate to /config route or conditional render)
- **C**: Slide-in panel from side

**Default if unknown:** Radix Dialog modal (simpler implementation, no routing needed, matches existing UI patterns, mobile-friendly)

---

## Q3: Should limit positions (minLimit/maxLimit) be editable in the config UI, or only via the limit switch learning process?

**Context**: Backend supports setting limits via setConfig command, but README.md describes "limit switch learning" as the intended workflow (move motor to positions, trigger switches, positions auto-saved).

**Allowing manual limit editing could**:
- ✅ Let users fine-tune without physically moving motor
- ⚠️ Risk setting invalid limits that break motor operation

**Default if unknown:** No - Keep limits read-only in UI, only editable via physical limit switch learning (safer, matches documented workflow in README.md section "Limit Switch Learning")

---

## Q4: Should the config UI show a preview of what will change before applying, or just highlight modified fields?

**Context**: Form will have current values pre-filled. User might change multiple fields.

**Options**:
- **A**: Just show modified fields in different color/style
- **B**: Show "Current" vs "New" side-by-side for changed values
- **C**: Simple form, no preview (user sees values they entered)

**Default if unknown:** Simple form with no preview (Option C) - keeps UI clean, values are self-explanatory, "Revert" button provides safety net

---

## Q5: Should config validation errors prevent the Apply button from being enabled, or show warnings but allow submission?

**Context**: Frontend will validate values (e.g., maxSpeed must be 1000-12000). If user enters invalid value:

**Options**:
- **A**: Disable Apply button until all fields valid (strict)
- **B**: Show warning but allow Apply (trust backend validation)
- **C**: Validate on Apply click, show errors inline

**Default if unknown:** Option A - Disable Apply button with red error text below invalid fields (prevents invalid submissions, clear feedback, matches modern form UX)

---

## Notes
All questions have smart defaults based on:
- Existing codebase patterns
- User's stated preferences (no clutter, mobile-friendly)
- Safety considerations (motor control)
- UX best practices (Radix UI patterns, accessible forms)
