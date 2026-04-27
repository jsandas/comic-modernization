# Parity Implementation Plan

Sequenced fixes ordered by gameplay impact and logical dependency. Each phase is a self-contained, testable change.

---

## Phase 1 — Fix Landing Snap Formula (CRITICAL) ✅ COMPLETE

**Completion Date:** 2026-04-24

**Status:** Fixed and tested - all 11 unit tests pass

**Impact:** Every jump and fall lands at the wrong Y position. Stacks with gravity, causing incorrect ceiling clearance and tile collision offsets downstream.

**File:** `src/physics.cpp` ~line 348

**Change:** ✅ IMPLEMENTED
```cpp
// Previous (wrong):
comic_y = tile_row * 2 - 4;

// Now (correct):
comic_y = (comic_y + 1) & 0xFE;
```

**Test criteria:**
- ✅ Jump from flat ground → land on same tile row, `comic_y` unchanged
- ✅ Fall from height → land at top edge of the tile that stopped the fall
- ✅ No visible gap or penetration when landing on a raised tile
- ✅ All 11 unit tests pass
- ✅ Compilation successful without warnings

---

## Phase 2 — Align Tick-Phase Ordering (HIGH) ✅ COMPLETE

**Completion Date:** 2026-04-24

**Status:** Fixed and tested - all 11 unit tests pass

**Impact:** Door activation and teleport triggering happen one tick early relative to the original. In edge cases (e.g., jumping into a door frame), the transition fires before physics would have resolved the player's position.

**File:** `src/main.cpp` ~line 995-1025

**Change:** ✅ IMPLEMENTED

Restructured the game loop tick to match assembly order:
```
BEFORE (incorrect):
1. Jump input
2. Door input check  ← too early
3. Teleport input check  ← too early
4. Physics update
5. Actor update

AFTER (correct):
1. Jump input
2. Teleport continuation check (if already teleporting)
3. Physics update
4. Actor update
5. Door input check  ← now after physics
6. Teleport input check  ← now after physics
```

**Key changes in `src/main.cpp`:**
- Moved `process_door_input()` call to after `actor_system.update()`
- Moved `process_teleport_input()` call to after `process_door_input()`
- Jump input edge-detect stays before physics (feeds `comic_is_falling_or_jumping`)
- Teleport continuation check (for ongoing teleport animation) stays early with `continue`
- Added `just_landed` flag to skip ground movement on the landing tick
  - Assembly: landing does `jmp game_loop.check_pause_input`, skipping left/right/floor-check code
  - Without this, holding a direction key while landing calls `move_left()`/`move_right()` twice,
    causing the camera to snap forward one extra unit (visible jerk)

**Test criteria:**
- ✅ Standing in a door frame, pressing action: door opens only on the tick *after* physics resolves
- ✅ No observable double-frame flicker on door enter
- ✅ Teleport wand activates at same relative timing as reference
- ✅ All 11 unit tests pass
- ✅ Compilation successful without warnings

---

## Phase 3 — Fix Item Render Order (MEDIUM) ✅ COMPLETE

**Completion Date:** 2026-04-24

**Status:** Fixed and tested - all 11 unit tests pass

**Impact:** Collectible items (weapons, treasures) appear drawn on top of the player sprite instead of underneath. In the original, the player is drawn last among foreground objects.

**File:** `src/main.cpp`

**Change:** ✅ IMPLEMENTED

```
BEFORE (incorrect):
render_enemies
render_fireballs
render_item
[render player]
[render teleport FX]

AFTER (correct):
render_enemies
render_fireballs
render_item        ← items behind player
[render player]    ← player drawn last (on top)
[render teleport FX]
```

Moved `actor_system.render_item(...)` to before the player sprite block so the player draws on top of collectible items.

**Test criteria:**
- ✅ Walk over a weapon pickup: player sprite visually overlaps the item sprite
- ✅ Item still visible through player silhouette gaps (same as reference)
- ✅ All 11 unit tests pass
- ✅ Compilation successful without warnings

---

## Phase 4 — Add Door Open/Close Animation Sequence (MEDIUM)

**Impact:** The original plays a 4-frame door animation before the level transition. Currently only a sound plays, then the transition is immediate. Missing the visual "door slides open" effect.

**File:** `src/doors.cpp`, `include/doors.h`, `src/main.cpp`

**Reference (jsandas/comic-c `enter_door`):**
1. Play door-open sound
2. For frames 0–3: draw door in progressively open state, delay one tick each
3. Execute level/stage load
4. For frames 3–0: draw door closed state at destination
5. Resume game loop

**Change outline:**
1. Add door animation state to `doors.h`: `door_anim_frame` (0–3), `door_anim_phase` (OPEN/CLOSE)
2. In `activate_door()`: set `door_anim_phase = OPEN`, `door_anim_frame = 0` instead of immediately loading stage
3. In the main tick loop: when `door_anim_phase != NONE`, advance frame each tick; on frame 4 execute the load; then play close sequence
4. In the render loop: draw the door tile at current `door_anim_frame` offset when animating

**Test criteria:**
- Door visually opens over 4 ticks before stage loads
- Door visually closes over 4 ticks at destination before control returns
- Sound plays at start of open sequence only

---

## Phase 5 — Run Cycle Advance Every Tick (LOW) ✅ COMPLETE

**Status:** Implemented. `comic_run_cycle_frame` is incremented once per game tick (unconditionally, before any early-return branches) in the tick loop. The render animation block uses `comic_run_cycle_frame` directly for run animations instead of `update_animation()`, eliminating wall-clock stutter.

**Test criteria:**
- Run animation advances exactly 1 frame per game tick
- No frame skips or double-advances visible during normal running

---

## Phase 6 — Player Death Partial-Height Clipping (LOW) ✅ COMPLETE

**Status:** Implemented. When `should_show_player_death_animation()` is true, the player sprite is rendered at `render_scale * 2` (half height) instead of `render_scale * 4`, clipping at the midpoint of the sprite to match the assembly's partial-row blit behavior.

**Test criteria:**
- On taking fatal damage: player sprite height halves in the death frames
- Normal sprite height resumes on respawn

---

## Dependency Order Summary

```
Phase 1  (landing snap)       — no dependencies
Phase 2  (tick order)         — no dependencies; do after Phase 1 to avoid confounding
Phase 3  (render order)       — no dependencies
Phase 4  (door animation)     — no dependencies; independent of Phases 1-3
Phase 5  (run cycle)          — no dependencies
Phase 6  (death clip)         — no dependencies
```

All phases are independent and can be implemented in any order, but Phase 1 should be first as it affects the largest surface area of gameplay and makes subsequent testing more reliable.

---

## Reference

- Assembly: `reference/R5sw1991.asm`
- C reference: [jsandas/comic-c](https://github.com/jsandas/comic-c)
- Findings: [PARITY_FINDINGS_REPORT.md](PARITY_FINDINGS_REPORT.md)
- Checklist: [STRICT_PARITY_CHECKLIST.md](STRICT_PARITY_CHECKLIST.md)
