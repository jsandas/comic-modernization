# Test Guide

This document describes the test suite for Captain Comic modernization. Tests are implemented in `tests/test_main.cpp` and can be run with `./comic_tests`.

## Running Tests

List all available tests:
```bash
./comic_tests --list
```

Run all tests:
```bash
./comic_tests
```

Run tests matching a pattern:
```bash
./comic_tests --filter door    # Run all door-related tests
./comic_tests --filter jump    # Run all jump-related tests
./comic_tests --filter physics # Run all physics tests
./comic_tests --filter stage   # Run all stage-related tests
```

## Test Categories

### Physics & Collision (1 test)

#### test_physics_tiles
**Purpose:** Verify tile collision detection and terrain metadata.

**What it tests:**
- Tile lookup at map coordinates returns correct tile ID
- Ground floor tiles are marked as solid (tile 0x3F)
- Out-of-bounds positions return passable tile (0)
- Tile solidity check correctly identifies solid vs passable tiles (threshold at 0x3E/0x3F)

**Why it matters:** Core collision detection depends on accurate tile identification and solidity checks. This test ensures the foundation for all physics is correct.

---

### Animation System (3 tests)

#### test_animation_looping
**Purpose:** Verify animation frames advance correctly and loop when enabled.

**What it tests:**
- Animation frame index starts at 0
- Frame advances at correct time intervals based on frame duration
- Animation loops back to frame 0 after final frame when `looping=true`

**Why it matters:** Visual feedback is critical for game feel. Looping animations (idle, running) must cycle smoothly. Incorrect timing makes the game feel janky or unresponsive.

---

#### test_animation_non_looping
**Purpose:** Verify one-shot animations reach final frame and clamp there.

**What it tests:**
- Animation frame index advances correctly through all frames
- Animation clamps at final frame (doesn't loop) when `looping=false`
- Frame index stays valid throughout playback

**Why it matters:** One-shot animations (jump, hit reactions) should play once then hold the final frame. They must never loop or go out of bounds.

---

#### test_animation_zero_duration
**Purpose:** Verify animation system handles edge case of zero-duration frames gracefully.

**What it tests:**
- Zero-duration frames don't cause crashes or infinite loops
- Frame index stays within valid range
- Animation system can handle degenerate input

**Why it matters:** Defensive programming. Even though frames should always have positive duration, the system should not crash or misbehave if given zero duration.

---

### Jump Physics (3 tests)

#### test_jump_edge_trigger
**Purpose:** Verify jump only initiates on key press edge, not while held.

**What it tests:**
- Jump starts when key transitions from not-pressed to pressed
- Jump does NOT retrigger while key is held continuously
- Jump input uses edge detection, not level detection

**Why it matters:** This prevents the player from jumping every frame while holding the jump key. Edge triggering is essential for responsive, arcade-like jump controls. Without this, the game would be unplayable.

---

#### test_jump_recharge
**Purpose:** Verify jump counter recharges to full when player releases jump key.

**What it tests:**
- Jump counter is depleted (set to 1) when jump holds expire
- Jump counter recharges to `comic_jump_power` when jump key is released and player is on ground
- Player can jump again after releasing and pressing jump key

**Why it matters:** The jump system uses a "jump counter" that decays during a jump, controlling jump height. It must fully recharge when released so the player can jump again. Without this, the player would only be able to jump once per level.

---

#### test_jump_height
**Purpose:** Verify jump height matches design values for default and boosted scenarios.

**What it tests:**
- Default jump height is 7 units (measured from ground to apex)
- Jump height with boots is 9 units (2 units higher) 
- Jump physics produces deterministic, consistent height

**Why it matters:** Jump height is a core mechanic that affects level design and difficulty. Tests verify that power-ups work correctly and that heights match the original game design. This ensures the game feels right.

---

### Door System (6 tests)

#### test_door_activation_alignment_x
**Purpose:** Verify doors only activate when Comic is positioned correctly horizontally.

**What it tests:**
- Door activates at exact door X coordinate
- Door activates 1 unit offset to the right of door X
- Door activates 2 units offset to the right (within 3-unit tolerance)
- Door does NOT activate 3+ units away
- Door does NOT activate to the left of its position (negative offset)

**Why it matters:** This implements the precise collision behavior required for door activation. Comic must be closely aligned with the door. Incorrect alignment would allow opening doors from impossible positions or prevent legitimate activation.

**Key detail:** Comic's width is 2 units, and doors are 2 units wide. The 3-unit tolerance (0, 1, 2 units offset) allows Comic to stand in the door or adjacent to it. Offsets cannot be negative.

---

#### test_door_activation_alignment_y
**Purpose:** Verify doors only activate when Comic is at the exact vertical position.

**What it tests:**
- Door activates only when Comic's Y exactly matches door Y
- Door does NOT activate when Comic is 1 unit above (y-1)
- Door does NOT activate when Comic is 1 unit below (y+1)

**Why it matters:** Unlike X alignment which uses a tolerance, Y alignment must be exact. This prevents activating doors from awkward heights and maintains tight positioning requirements.

---

#### test_door_key_requirement
**Purpose:** Verify doors cannot be opened without the Door Key item.

**What it tests:**
- Door does NOT activate when `comic_has_door_key = 0`
- Door DOES activate when `comic_has_door_key = 1`
- Key requirement is enforced before door transition

**Why it matters:** Keys are a core progression mechanic. Without this check, players could bypass locked doors. This test ensures the game progresses in the intended order.

---

#### test_door_open_key_requirement
**Purpose:** Verify doors require the player to press the open key.

**What it tests:**
- `check_door_activation()` returns 0 when not pressing open key (`key_state_open = 0`)
- `check_door_activation()` returns 1 when open key is pressed (`key_state_open = 1`)
- Player cannot accidentally trigger doors (must explicitly press open key)

**Why it matters:** This ensures doors are not opened by proximity alone. The player must actively press the open key. This prevents frustrating accidental transitions and maintains player control.

---

#### test_door_state_update_same_level
**Purpose:** Verify door transitions within the same level correctly update all state variables.

**What it tests:**
- `source_door_level_number` is set to the origin level (for reciprocal door lookup)
- `source_door_stage_number` is set to the origin stage
- `current_stage_number` is updated to the target stage
- `current_level_number` remains unchanged (same level)
- Activation succeeds when all conditions are met

**Critical detail:** The source level/stage are saved because when entering a door, the destination stage must find the reciprocal (return) door. Without this, the player couldn't exit back through the same door.

**Why it matters:** Stage-to-stage transitions establish the gameplay loop. Players cross different stages within a level (lake0 → lake1 → lake2), and must be able to return. This test verifies the machinery is in place.

---

#### test_door_state_update_different_level
**Purpose:** Verify door transitions to a different level correctly update all state variables.

**What it tests:**
- `source_door_level_number` is set to the origin level
- `source_door_stage_number` is set to the origin stage
- `current_level_number` is updated to the target level
- `current_stage_number` is updated to the target stage
- Activation succeeds (which triggers `load_new_level()`)

**Why it matters:** Level-to-level progression is the primary narrative progression (lake → forest → space → ...). This test ensures that cross-level doors work and that the system correctly tracks where the player came from (for returning through reciprocal doors).

---

### Stage Exits (4 tests)

#### test_stage_left_exit_blocked
**Purpose:** Verify normal left movement works when not at the stage boundary.

**What it tests:**
- Comic moves left normally when positioned away from the left edge
- Left movement does not trigger a stage exit when not at the boundary

**Why it matters:** Stage exits should only trigger at the boundary. Regular movement must remain unaffected in the interior of the stage.

---

#### test_stage_right_exit_blocked
**Purpose:** Verify normal right movement works when not at the stage boundary.

**What it tests:**
- Comic moves right normally when positioned away from the right edge
- Right movement does not trigger a stage exit when not at the boundary

**Why it matters:** Prevents false positives where normal movement would accidentally cause a stage transition.

---

#### test_stage_left_edge_detection
**Purpose:** Verify left edge exit logic is blocked if the level pointer is missing.

**What it tests:**
- When Comic is at the left edge and `current_level_ptr` is null, stage does not change
- Leftward momentum is cleared when blocked at the edge

**Why it matters:** Defensive handling avoids crashes or undefined behavior when stage data is unavailable.

---

#### test_stage_right_edge_detection
**Purpose:** Verify right edge exit logic is blocked if the level pointer is missing.

**What it tests:**
- When Comic is at the right edge and `current_level_ptr` is null, stage does not change
- Rightward momentum is cleared when blocked at the edge

**Why it matters:** Mirrors the left edge guard path to keep boundary transitions safe and deterministic.

---

## Test Infrastructure

### Helper Functions

**`reset_physics_state()`** - Initializes a default physics state:
- Position: (4, 14) on the test level
- All velocities and states zeroed
- Full jump charge
- Camera at (0, 0)

**`reset_door_state()`** - Initializes default door system state:
- No door key collected
- Open key not pressed
- Forest zone, stage 0
- Source door cleared (-1)

**`create_test_level_with_door(x, y, target_level, target_stage)`** - Constructs a minimal test level with a single door at specified coordinates and target.

### Test Framework

Tests use a simple assertion macro:
```cpp
check(condition, "message")
```

If `condition` is false, it prints "FAIL: message" and increments a failure counter. At the end, the test harness reports total failures.

---

## Coverage Summary

| Category | Count | Status |
|----------|-------|--------|
| Physics & Collision | 1 | ✓ Implemented |
| Animation | 3 | ✓ Implemented |
| Jump Physics | 3 | ✓ Implemented |
| Door System | 6 | ✓ Implemented |
| Stage Exits | 4 | ✓ Implemented |
| **Total** | **17** | ✓ All Passing |

---

## Future Test Additions

Areas that would benefit from additional tests:

1. **Door Reciprocal Positioning** - Verify that `load_new_stage()` correctly positions Comic in front of the reciprocal door
2. **Item Collection** - Test item pickup and inventory tracking
3. **Enemy Behavior** - Test enemy spawning, movement, and collision
4. **Level Boundary Transitions** - Test stage exits (left/right boundaries)
5. **Camera System** - Verify camera follows Comic smoothly and respects boundaries
6. **Tileset Loading** - Test that tilesets load correctly for each level
7. **Checkpoint/Respawn** - Test that dying/boundary crossings respawn Comic at correct position

---

## Running Tests During Development

When making changes to core systems (physics, doors, animation), always run the relevant tests:

```bash
# After changing physics
./comic_tests --filter physics

# After changing animation
./comic_tests --filter animation

# After changing door logic
./comic_tests --filter door

# After changing stage exits
./comic_tests --filter stage

# After any change
./comic_tests
```

Tests should pass before committing changes. Failing tests indicate a regression that must be fixed before merging.
