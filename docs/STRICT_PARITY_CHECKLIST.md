# Strict Parity Checklist

Date: 2026-04-24

Purpose: hard checklist for verifying outcome parity against original behavior, while keeping modern implementation architecture.

How to use:
- Mark each item Pass/Fail.
- For Fail, record file/line and follow-up issue.
- Do not merge parity work until all Critical items pass.

## Critical (Must Pass)

### C1. Player Landing Snap
- [ ] Landing after downward collision resolves to original-equivalent even boundary behavior.
- [ ] No one-unit oscillation after repeated short hops on flat ground.
- [ ] Edge landings near tile boundaries match expected settle height.
Evidence:
- Current implementation point: [src/physics.cpp](../src/physics.cpp#L348)

### C2. Tick Ordering Around Physics and Inputs
- [ ] Per-tick ordering of jump, physics, door, teleport, floor checks matches intended original outcome.
- [ ] In-air gating prevents door/teleport checks when original flow would skip them.
- [ ] Floor-loss transition starts falling on the same effective tick as original.
Evidence:
- Tick loop: [src/main.cpp](../src/main.cpp#L980)
- Ordering points: [src/main.cpp](../src/main.cpp#L996), [src/main.cpp](../src/main.cpp#L999), [src/main.cpp](../src/main.cpp#L1003), [src/main.cpp](../src/main.cpp#L1024)
- Assembly anchors: [reference/R5sw1991.asm](../reference/R5sw1991.asm#L3951), [reference/R5sw1991.asm](../reference/R5sw1991.asm#L3980), [reference/R5sw1991.asm](../reference/R5sw1991.asm#L4010), [reference/R5sw1991.asm](../reference/R5sw1991.asm#L4274)

### C3. Teleport Timing and Destination Application
- [ ] Destination position applies on teleport frame 3.
- [ ] Source/destination FX phase offset matches original feel.
- [ ] Camera shift during teleport is bounded and parity-tested at map edges.
Evidence:
- Destination timing: [src/player_teleport.cpp](../src/player_teleport.cpp#L9)
- Main teleport handling: [src/main.cpp](../src/main.cpp#L1008), [src/main.cpp](../src/main.cpp#L1282)

### C4. Core Physics Constants
- [ ] Gravity normal = 5.
- [ ] Gravity space = 3.
- [ ] Terminal velocity = 23.
- [ ] Jump acceleration = 7.
Evidence:
- [include/physics.h](../include/physics.h#L8), [include/physics.h](../include/physics.h#L9), [include/physics.h](../include/physics.h#L10), [include/physics.h](../include/physics.h#L13)

## High (Should Pass Before Release)

### H1. Door Interaction and Transition Semantics
- [ ] Door activation requires open input, exact Y, and X window parity.
- [ ] Door key requirement enforced exactly.
- [ ] Transition target level/stage mapping is reciprocal and stable.
Evidence:
- [src/doors.cpp](../src/doors.cpp#L57), [src/doors.cpp](../src/doors.cpp#L76), [src/doors.cpp](../src/doors.cpp#L84), [src/doors.cpp](../src/doors.cpp#L119)

### H2. Door Animation Sequence Parity
- [ ] Entry/exit door animation sequence exists and matches original ordering.
- [ ] Comic and door layer ordering matches intended frame narrative.
Evidence:
- Current gap location: [src/doors.cpp](../src/doors.cpp#L119)

### H3. Enemy Behavior Coverage and Data Mapping
- [ ] Leap, roll, seek, shy, bounce all produce expected movement families.
- [ ] Stage enemy behavior codes (including fast flag) map correctly from level data.
Evidence:
- Behaviors: [src/actors.cpp](../src/actors.cpp#L869), [src/actors.cpp](../src/actors.cpp#L1010), [src/actors.cpp](../src/actors.cpp#L1085), [src/actors.cpp](../src/actors.cpp#L1169)
- Data mapping: [src/level_data.cpp](../src/level_data.cpp#L276), [src/level_data.cpp](../src/level_data.cpp#L636), [reference/R3_levels.asm](../reference/R3_levels.asm#L263), [reference/R3_levels.asm](../reference/R3_levels.asm#L652)

### H4. Fast Enemy Bug Compatibility Decision
- [ ] Project explicitly chooses one mode:
  - strict legacy bug-compatible tempo, or
  - modernized fixed tempo.
- [ ] Choice is documented and tested for long-run behavior.
Evidence:
- Initialization and restraint transitions: [src/actors.cpp](../src/actors.cpp#L610), [src/actors.cpp](../src/actors.cpp#L947)

## Medium (Authenticity / Polish)

### M1. Item and Player Layering
- [ ] Visual overlap order between item and player is intentionally chosen and tested in overlap scenes.
Evidence:
- Item render: [src/main.cpp](../src/main.cpp#L1266)
- Player render: [src/main.cpp](../src/main.cpp#L1269)

### M2. Title and Beam Sequence Timing
- [ ] Fade-step cadence and beam visibility thresholds match expected outcome.
Evidence:
- Fade timing: [src/title_sequence.cpp](../src/title_sequence.cpp#L18)
- Beam thresholds: [src/main.cpp](../src/main.cpp#L663), [src/main.cpp](../src/main.cpp#L860)

### M3. Camera Follow and Stage Edge Feel
- [ ] Camera threshold and one-step increments feel consistent during sustained run and edge transitions.
Evidence:
- [src/physics.cpp](../src/physics.cpp#L449), [src/physics.cpp](../src/physics.cpp#L517)

## Verification Matrix
For each test, mark Pass/Fail and attach clip or notes.

1. Flat-ground jump spam for 30s (landing stability).
2. Jump at door boundary while pressing open.
3. Walk off ledge while holding left/right, verify immediate fall transition.
4. Teleport at left and right stage extremes.
5. Enemy stress test with fast variants over 3 minutes.
6. Door transition visual sequence capture.
7. Item overlap with player in same coordinates.

## Release Gate
- [ ] All Critical items Pass.
- [ ] All High items Pass, or explicit waived exceptions approved.
- [ ] Medium items reviewed and accepted as intentional differences.
