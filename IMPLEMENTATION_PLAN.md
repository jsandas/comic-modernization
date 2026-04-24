# Parity Implementation Plan

Sequenced fixes ordered by gameplay impact and logical dependency. Each phase is a self-contained, testable change.

---

## Phase 1 — Fix Landing Snap Formula (CRITICAL) [IN PROGRESS]

**Impact:** Every jump and fall lands at the wrong Y position. Stacks with gravity, causing incorrect ceiling clearance and tile collision offsets downstream.

**File:** `src/physics.cpp` ~line 348

**Change:**
```cpp
// Current (wrong):
comic_y = tile_row * 2 - 4;

// Correct (matches assembly mask at R5sw1991.asm):
comic_y = (comic_y + 1) & 0xFE;
```

**Why:** The original clears the low bit of `(comic_y + 1)` to snap to the nearest even boundary below the foot probe position. The current formula subtracts a hardcoded 4, which gives the wrong result whenever `comic_y` is not exactly at a 2-unit boundary.

**Test criteria:**
- Jump from flat ground → land on same tile row, `comic_y` unchanged
- Fall from height → land at top edge of the tile that stopped the fall
- No visible gap or penetration when landing on a raised tile

---

## Phase 2 — Align Tick-Phase Ordering (HIGH)

**Impact:** Door activation and teleport triggering happen one tick early relative to the original. In edge cases (e.g., jumping into a door frame), the transition fires before physics would have resolved the player's position.

**File:** `src/main.cpp` ~line 995-1025

**Assembly order (R5sw1991.asm):**
1. Physics update (`update_physics`)
2. Actor/enemy update
3. Door check
4. Teleport check

**Current C++ order:**
1. Jump input
2. Door input check  ← too early
3. Teleport input check  ← too early
4. Physics update

**Change:** Move `process_door_input()` and `process_teleport_input()` to after `update_physics()` and actor updates, matching the assembly phase order. Jump input edge-detect can stay before physics (it feeds `comic_is_falling_or_jumping`).

**Test criteria:**
- Standing in a door frame, pressing action: door opens only on the tick *after* physics resolves
- No observable double-frame flicker on door enter compared to reference
- Teleport wand activates at same relative timing as reference

---

## Phase 3 — Fix Item Render Order (MEDIUM)

**Impact:** Collectible items (weapons, treasures) appear drawn on top of the player sprite instead of underneath. In the original, the player is drawn last among foreground objects.

**File:** `src/main.cpp` ~line 1264-1269

**Current order:**
```
render_enemies
render_fireballs
render_item        ← items on top of player
[render player]
```

**Correct order:**
```
render_enemies
render_fireballs
[render player]
render_item        ← items behind player (player drawn last)
```

**Change:** Move `actor_system.render_item(...)` call to after the player sprite render block (~line 1278).

**Test criteria:**
- Walk over a weapon pickup: player sprite visually overlaps the item sprite
- Item still visible through player silhouette gaps (same as reference)

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

## Phase 5 — Run Cycle Advance Every Tick (LOW)

**Impact:** The assembly advances `comic_run_cycle` (animation frame index) exactly once per game tick while running. The current SDL animation system uses a wall-clock timer, which can advance 0 or 2 frames in a single tick at tick boundaries. Causes minor animation stutter.

**File:** `src/main.cpp` (animation update block), `include/graphics.h`

**Change:** When `comic_is_falling_or_jumping == 0` and horizontal movement is non-zero, increment the run animation frame counter by 1 inside the tick loop unconditionally, rather than relying on `SDL_GetTicks()` comparison in `get_current_frame`.

**Test criteria:**
- Run animation advances exactly 1 frame per game tick
- No frame skips or double-advances visible during normal running

---

## Phase 6 — Player Death Partial-Height Clipping (LOW)

**Impact:** On death, the original renders the player at half sprite height (clipped at midpoint), simulating the character "melting" into the floor. Currently the full sprite is shown until the death sequence ends.

**File:** `src/main.cpp` (death render block), `src/graphics.h`

**Change:** When the death animation is active and `comic_health == 0`, clip the destination rect height to `render_scale * 2` (half normal height) during the sprite blit.

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
