# Captain Comic Modernization Parity Findings Report

Date: 2026-04-24

## Goal
This report compares the current C++ implementation with:
- C reference: jsandas/comic-c
- Original assembly behavior: reference/R5sw1991.asm and reference/R3_levels.asm

Primary success metric: gameplay outcome parity (feel and player-visible result), not instruction-level parity.

## Executive Summary
Overall parity is strong in core physics constants, enemy behavior coverage, teleport timing, and stage data encoding. The highest-impact gaps are:
1. Player landing snap formula in physics.
2. Tick-order differences around door/teleport/physics gating.
3. Missing full door animation sequence parity.
4. Policy decision needed for legacy fast-enemy restraint overflow bug compatibility.

## Scope Reviewed
- Rendering and composition: [src/main.cpp](../src/main.cpp), [src/title_sequence.cpp](../src/title_sequence.cpp), [src/graphics.cpp](../src/graphics.cpp), [src/ui_system.cpp](../src/ui_system.cpp)
- Game loop and sequencing: [src/main.cpp](../src/main.cpp)
- Player physics and movement: [src/physics.cpp](../src/physics.cpp), [src/doors.cpp](../src/doors.cpp), [src/player_teleport.cpp](../src/player_teleport.cpp)
- Enemy/actor logic and data: [src/actors.cpp](../src/actors.cpp), [include/actors.h](../include/actors.h), [src/level_data.cpp](../src/level_data.cpp)
- Assembly anchors: [reference/R5sw1991.asm](../reference/R5sw1991.asm), [reference/R3_levels.asm](../reference/R3_levels.asm)

## Detailed Findings

### 1) Rendering
What aligns:
- Tick cadence target mirrors DOS odd-IRQ cadence via constants and accumulator in [src/main.cpp](../src/main.cpp#L534), [src/main.cpp](../src/main.cpp#L535), [src/main.cpp](../src/main.cpp#L536), [src/main.cpp](../src/main.cpp#L980).
- Title fade pacing explicitly tracks one-tick style transitions in [src/title_sequence.cpp](../src/title_sequence.cpp#L17), [src/title_sequence.cpp](../src/title_sequence.cpp#L18).
- Beam in/out comic visibility thresholds are now aligned in [src/main.cpp](../src/main.cpp#L663), [src/main.cpp](../src/main.cpp#L860).
- Teleport destination applies on frame 3 in [src/player_teleport.cpp](../src/player_teleport.cpp#L9).

Potential outcome drift:
- Item is rendered before player in main render path: [src/main.cpp](../src/main.cpp#L1266) then player at [src/main.cpp](../src/main.cpp#L1269).
- Original loop structure is map/comic then non-player actors: [reference/R5sw1991.asm](../reference/R5sw1991.asm#L4100), [reference/R5sw1991.asm](../reference/R5sw1991.asm#L4104).
- Door visuals: current C++ door handling is logic-focused without original multi-frame door animation parity in [src/doors.cpp](../src/doors.cpp#L119).

Impact:
- Mostly visual overlap/authenticity differences.
- Door transition visuals are the largest rendering authenticity gap.

### 2) Game Loop
What aligns:
- Core assembly flow checkpoints exist and are traceable: [reference/R5sw1991.asm](../reference/R5sw1991.asm#L3861), [reference/R5sw1991.asm](../reference/R5sw1991.asm#L3951), [reference/R5sw1991.asm](../reference/R5sw1991.asm#L3980), [reference/R5sw1991.asm](../reference/R5sw1991.asm#L4010).
- C++ uses a bounded accumulator and stable per-tick updates: [src/main.cpp](../src/main.cpp#L980).
- Teleport path has special-case handling and skip behavior in [src/main.cpp](../src/main.cpp#L1008).

Potential outcome drift:
- Current order in C++ does jump/door/teleport checks before physics each tick: [src/main.cpp](../src/main.cpp#L996), [src/main.cpp](../src/main.cpp#L999), [src/main.cpp](../src/main.cpp#L1003), then [src/main.cpp](../src/main.cpp#L1024).
- Original/C-reference sequencing gates door/teleport/floor checks differently around in-air paths.

Impact:
- Edge-case timing differences near doors, floor edges, and jump transitions.
- Most visible in precision movement and speedrun-style routing.

### 3) Enemy Logic
What aligns:
- All major behavior families are implemented:
  - leap: [src/actors.cpp](../src/actors.cpp#L869)
  - roll: [src/actors.cpp](../src/actors.cpp#L1010)
  - seek: [src/actors.cpp](../src/actors.cpp#L1085)
  - shy: [src/actors.cpp](../src/actors.cpp#L1169)
- Assembly behavior anchors for leap/bounce are present: [reference/R5sw1991.asm](../reference/R5sw1991.asm#L4887), [reference/R5sw1991.asm](../reference/R5sw1991.asm#L5054).
- Fast flag data encoding appears consistent between C++ and asm data:
  - C++ examples: [src/level_data.cpp](../src/level_data.cpp#L276), [src/level_data.cpp](../src/level_data.cpp#L636)
  - ASM examples: [reference/R3_levels.asm](../reference/R3_levels.asm#L263), [reference/R3_levels.asm](../reference/R3_levels.asm#L652)

Policy gap (intentional modernization candidate):
- Fast-enemy restraint overflow bug compatibility. C++ initializes fast restraint in [src/actors.cpp](../src/actors.cpp#L610) with clean state transitions; assembly has documented quirks and overflow side effects.

Impact:
- If strict historical bug parity is required, long-run enemy tempo may differ.
- If modernized fairness is preferred, current behavior is likely acceptable.

### 4) Player Logic
What aligns:
- Constants match expected values:
  - gravity: [include/physics.h](../include/physics.h#L8), [include/physics.h](../include/physics.h#L9)
  - terminal velocity: [include/physics.h](../include/physics.h#L10)
  - jump acceleration: [include/physics.h](../include/physics.h#L13)
- Edge-trigger jump input is implemented in [src/physics.cpp](../src/physics.cpp#L227), [src/physics.cpp](../src/physics.cpp#L229), [src/physics.cpp](../src/physics.cpp#L230).
- Vertical integration and ceiling-stick behavior are present in [src/physics.cpp](../src/physics.cpp#L262), [src/physics.cpp](../src/physics.cpp#L267), [src/physics.cpp](../src/physics.cpp#L327).
- Stage-edge transitions and camera movement are present in [src/physics.cpp](../src/physics.cpp#L414), [src/physics.cpp](../src/physics.cpp#L480), [src/physics.cpp](../src/physics.cpp#L449), [src/physics.cpp](../src/physics.cpp#L517).
- Door activation bounds match original intent in [src/doors.cpp](../src/doors.cpp#L57), [src/doors.cpp](../src/doors.cpp#L76), [src/doors.cpp](../src/doors.cpp#L84).

Highest-impact mismatch:
- Landing snap formula in [src/physics.cpp](../src/physics.cpp#L348) uses tile-row arithmetic. Assembly/C-reference parity behavior typically snaps via even-boundary masking after contact check.

Impact:
- This directly affects platforming settle position and can accumulate into feel differences.

## Priority Ranking (Outcome Impact)
1. Landing snap parity in player physics.
2. Tick-order parity around physics vs door/teleport/floor gate checks.
3. Door entry/exit animation sequencing parity.
4. Explicit policy choice for fast-enemy legacy bug compatibility.
5. Optional: item/player render order for overlap authenticity.

## Recommended Direction
Given the project intent (replicate original feel on modern systems):
- Treat landing snap and tick-order gating as mandatory parity fixes.
- Treat door animation parity as high-value authenticity work.
- Make fast-enemy bug parity a documented, deliberate project toggle/policy.
- Keep modern architecture (SDL2, accumulator loop) while matching gameplay outcomes at tick boundaries.

## Phase 8 QA Addendum (2026-05-10)

The following gameplay issues were reported during Phase 8 QA and are now addressed:

1. Extra life not awarded every 50,000 points:
- Root cause: `award_points()` no longer tracked ten-thousands carry cadence.
- Fix: restored `score_10000_counter` carry tracking and 5-step rollover extra-life award path.
- Evidence: [src/actors.cpp](../src/actors.cpp), [src/main.cpp](../src/main.cpp), [tests/test_ui.cpp](../tests/test_ui.cpp)

2. Door animation opening looked like black replacement instead of opening leaves:
- Root cause: overlay renderer only painted black rectangles for open states.
- Fix: render door tile halves shifted away from the center line for half/full open states.
- Evidence: [src/main.cpp](../src/main.cpp)

3. Enemies spawning inside solid tiles:
- Root cause: simplified spawn Y selection only checked up to two positions.
- Fix: spawn search now mirrors assembly flow (find solid anchor tile, then first non-solid above).
- Evidence: [src/actors.cpp](../src/actors.cpp), [tests/test_actors.cpp](../tests/test_actors.cpp)

4. White spark displayed when enemies fell into pits:
- Root cause: pit-fall sentinel state reached render path for one frame and was rendered with the normal spark effect.
- Fix: keep the one-frame pit-fall sentinel state, but suppress spark rendering in that state; the enemy then despawns on the next update tick.
- Evidence: [src/actors.cpp](../src/actors.cpp), [tests/test_actors.cpp](../tests/test_actors.cpp)

5. End-to-end victory trigger timing after 3rd treasure:
- Validation: gameplay run confirms collecting the 3rd treasure starts the `win_counter` countdown, then transitions into the victory sequence (not immediate handoff).
- Evidence: Phase 8 manual QA run (2026-05-11).
