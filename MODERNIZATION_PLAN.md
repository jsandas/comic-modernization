# Captain Comic Modernization Plan

## Project Overview

This project modernizes the classic 1988 DOS game *The Adventures of Captain Comic* for modern systems. The goal is to recreate the game using modern technologies while maintaining behavioral fidelity to the original.

**Original Game:**
- Platform: DOS (x86-16 assembly)
- Graphics: EGA 320x200, 16 colors
- Input: Keyboard/joystick
- Audio: PC speaker

**Target Platforms:**
- Windows, macOS, Linux
- Modern resolutions with optional scaling
- Modern input devices
- Cross-platform audio

## Technology Stack

### Core Libraries
- **SDL2**: Graphics, audio, input handling
  - Chosen for performance, broad compatibility, and low-level control
  - Better for faithful DOS emulation than SFML
  - Extensive features and mature ecosystem

- **C++17**: Modern language features while maintaining C compatibility
  - Easier to port from C refactor
  - Object-oriented design for maintainability

- **CMake**: Cross-platform build system
  - Simple configuration
  - IDE integration

### Reference Materials

Primary reference: [jsandas/comic-c](https://github.com/jsandas/comic-c)
- C refactor of the original assembly (Phase 4 complete)
- Structured, readable game logic
- Complete implementation of core systems
- Documents: REFACTOR_PLAN.md, GAME_LOOP_FLOW.md, CODING_STANDARDS.md

Secondary reference: Assembly disassembly
- Low-level details for fidelity
- Exact constants and edge cases
- Located in reference/disassembly/R5sw1991.asm

## Reference Code Structure

From jsandas/comic-c, key modules to port:

### Core Systems (src/)
- `game_main.c` - Entry point, game loop, level loading
- `physics.c` - Gravity, jumping, movement, collision
- `actors.c` - Enemies, fireballs, items
- `graphics.c` - Rendering, sprites, video buffers
- `sound.c` / `music.c` - PC speaker sound, music playback
- `doors.c` - Door system, level transitions
- `level_data.c` - All 8 levels: LAKE (0), FOREST (1 - first playable), SPACE, BASE, CAVE, SHED, CASTLE, COMP
- `file_loaders.c` - Asset loading (.PT, .TT2, .SHP, .EGA files)

### Headers (include/)
- `globals.h` - Shared constants and state
- `physics.h` - Physics constants (GRAVITY=5, ACCELERATION=7, etc.)
- `actors.h` - Enemy behaviors, fireball mechanics
- `level_data.h` - Level structures and data

## Implementation Phases

### Phase 1: Foundation ✅ COMPLETE
**Status:** Complete
**Completion Date:** 2026-02-12
**Goal:** Set up development environment and basic infrastructure

- [x] Initialize git repository
- [x] Create CMake build system
- [x] Install SDL2 dependencies
- [x] Create basic window and event loop
- [x] Implement input handling (keyboard)
- [x] Verify SDL2 linkage and execution

**Deliverable:** Application opens window, processes input, exits cleanly

---

### Phase 2: Core Physics ✅ COMPLETE
**Status:** Complete
**Completion Date:** 2026-02-12
**Goal:** Implement player movement and physics

**Tasks:**
- [x] Port physics constants from physics.h (GRAVITY, TERMINAL_VELOCITY, JUMP_ACCELERATION)
- [x] Implement `handle_fall_or_jump()` from physics.c
  - [x] Gravity application
  - [x] Jump counter logic
  - [x] Velocity integration
  - [x] Ceiling collision
  - [x] Ground collision with tile detection
- [x] Implement `move_left()` and `move_right()` from physics.c
  - [x] Wall collision detection
  - [x] Camera following
  - [x] Movement mechanics (stage transitions deferred to Phase 4)
- [x] Port tile map system
  - [x] `address_of_tile_at_coordinates()` (implemented as `get_tile_at()`)
  - [x] `is_tile_solid()`
  - [x] Tile data structures
  - [x] Test level with platforms and walls

**Reference Code:**
- `src/physics.c`: Lines 59-398 (handle_fall_or_jump, movement functions)
- `include/physics.h`: Constants and declarations
- `include/globals.h`: MAP_WIDTH, PLAYFIELD_WIDTH, etc.

**Success Criteria:**
- ✅ Player falls realistically with gravity
- ✅ Jumping height matches original (constants validated)
- ✅ Wall collision prevents movement through walls
- ✅ Floor detection works accurately
- ✅ Camera follows player smoothly
- ✅ Ceiling collision stops upward movement
- [ ] Stage transitions at boundaries (deferred to Phase 4)

---

### Phase 3: Rendering System ✅ COMPLETE
**Status:** Complete
**Completion Date:** 2026-02-12
**Goal:** Display game graphics

**Tasks:**
- [x] Port tile rendering from graphics.c
  - [x] Load PNG tileset files (converted from .TT2)
  - [x] Render tile maps to screen
  - [x] Implemented with SDL2 textures (hardware-accelerated)
- [x] Port sprite system from graphics.c
  - [x] Load PNG sprite files
  - [x] Blit sprites with proper rendering
  - [x] Animation frame handling with frame timing
- [x] Implement camera system
  - [x] Viewport scrolling with camera position
  - [x] Player-centered camera (camera follows player)
  - [x] Tile culling for performance (only render visible tiles)
- [x] Port EGA palette to SDL
  - [x] PNG files already in RGB format (no conversion needed)
- [x] Render player sprite
  - [x] Standing animation (idle)
  - [x] Running animation (3-frame cycle)
  - [x] Jumping animation
  - [x] Direction facing based on input

**Implementation Details:**
- Created `graphics.h` and `graphics.cpp` with `GraphicsSystem` class
- `GraphicsSystem` handles texture loading, sprite management, and animation
- Asset loader supports multiple search paths for flexibility
- SDL2_image (libpng) for PNG loading
- Animation system with frame-based timing
- Tile culling: only renders tiles within camera viewport

**Reference Code Used:**
- Asset organization from jsandas/comic-c assets
- Sprite naming conventions: `sprite-{name}_{direction}.png`
- Tileset naming: `{level}.tt2-{id}.png` (0x00-0x3F per level)

**Data Files Used:**
- Pre-converted PNG tilesets for all 8 levels (lake, forest, space, base, cave, shed, castle, comp)
- PNG sprite files for player animations (left/right facing)
- Player sprites: standing, running (3 frames), jumping

**Success Criteria:**
- ✅ Tiles render correctly from PNG files
- ✅ Sprites display with proper positioning
- ✅ Animations play smoothly with configurable frame timing
- ✅ Camera follows player appropriately, showing 24x20 game unit viewport
- ✅ Player sprite changes based on state (idle/running/jumping)
- ✅ Player sprite faces direction of movement
- ✅ No rendering artifacts or flicker (hardware-accelerated)

---

### Phase 4: Level System ✅ COMPLETE
**Status:** Complete
**Completion Date:** 2026-02-12
**Goal:** Load and manage game levels

**Tasks:**
- [x] Port level data structures from level_data.h
  - [x] `level_t` structure (with filenames, door tiles, sprite descriptors)
  - [x] `stage_t` structure (with items, exits, doors, enemies)
  - [x] Door definitions
  - [x] Enemy record definitions
  - [x] Sprite descriptor (`shp_t`) definitions
- [x] Implement level loading from level_data.c
  - [x] All 8 levels (LAKE through COMP) with complete data
  - [x] Stage data (3 stages per level) with exact values
  - [x] Enemy spawn data for all stages
  - [x] Item placement for all stages
- [x] Port file loaders from file_loaders.c (NO LONGER NEEDED)
  - [x] ~~.PT file loader~~ → Tile maps compiled as C++ hex arrays
  - [x] ~~.TT2 file loader~~ → Tilesets converted to PNG
  - [x] ~~.SHP file loader~~ → Sprites converted to PNG/GIF
  - [x] ~~.EGA file loader~~ → System graphics converted to PNG
  - **Note:** All binary formats pre-converted to modern formats (PNG/GIF)
  - **Note:** Tile data embedded in executable as C++ arrays (stage 2 migration)
- [x] Implement door system from doors.c
  - [x] Door collision detection
  - [x] Key checking
  - [x] Level/stage transitions (stubs for now)

**Reference Code:**
- `src/level_data.c`: All level definitions
- `src/file_loaders.c`: Asset loading
- `src/doors.c`: Door mechanics
- `src/physics.c`: Stage boundary transition logic

**Success Criteria:**
- ✅ All 8 levels load correctly
- ✅ Stage transitions work (left/right boundaries)
- ✅ Doors function properly with keys
- ✅ Level data matches original exactly

**Implementation Notes:**
- Door collision detection checks Y coordinate (exact) and X coordinate (within 3 units)
- Door Key requirement implemented
- Door transitions call load_new_level() or load_new_stage() as appropriate
- Source door tracking for reciprocal positioning (entering target stage via door)
- Alt key (SDL KMOD_ALT modifier) opens doors; 'K' key grants door key (debug)
- Level/stage loading system integrated:
  - load_new_level() sets current_level_ptr and loads tileset graphics
  - load_new_stage() loads stage tiles into physics system (current_tiles)
  - Reciprocal door search finds matching door in destination stage
  - Camera positioned relative to Comic on stage load
  - Initial spawn: Forest stage 0 at position (14, 12)
- Stage boundary transitions:
  - Triggered at left edge (comic_x == 0) or right edge (comic_x >= 254)
  - Checks stage->exit_l or stage->exit_r for target stage
  - If EXIT_UNUSED (0xFF), stops movement
  - Otherwise, transitions to new stage at opposite edge
  - Checkpoint updated to preserve Y position and edge position
  - source_door_level_number = -1 marks as boundary (not door) transition

---

### Phase 5: Actor System
**Status:** In Progress
**Completion Date:** TBD
**Goal:** Implement enemies, fireballs, and items

**Tasks:**
- [ ] Port enemy system from actors.c
  - [ ] Enemy spawning logic
  - [ ] 5 AI behaviors:
    - [ ] Bounce (Fire Ball, Brave Bird)
    - [ ] Leap (Bug-eyes, Beach Ball)
    - [ ] Roll (Glow Globe)
    - [ ] Seek (Killer Bee)
    - [ ] Shy (Shy Bird, Spinner)
  - [ ] Enemy-player collision
  - [ ] Enemy despawn logic
- [ ] Port fireball system from actors.c
  - [ ] Fireball spawning
  - [ ] Horizontal movement
  - [ ] Corkscrew motion (when item collected)
  - [ ] Fireball-enemy collision
  - [ ] Fireball meter depletion
- [ ] Port item system from actors.c
  - [ ] Item animation
  - [ ] Item collision detection
  - [ ] Item effects:
    - [ ] Blastola Cola (firepower)
    - [ ] Corkscrew (fireball motion)
    - [ ] Boots (jump power)
    - [ ] Lantern (castle lighting)
    - [ ] Shield (protection)
    - [ ] Gems, Crown, Gold (treasures)
    - [ ] Door Key, Teleport Wand

**Reference Code:**
- `src/actors.c`: Lines 1-1800+ (all actor systems)
- `include/actors.h`: Data structures and constants

**Success Criteria:**
- All enemy behaviors work correctly
- Enemies spawn at proper distances
- Fireballs kill enemies on contact
- Items grant correct abilities
- Treasure collection triggers win sequence

---

### Phase 6: Audio System
**Status:** Not Started
**Completion Date:** TBD
**Goal:** Implement sound effects and music

**Tasks:**
- [ ] Set up SDL_mixer for audio
- [ ] Port PC speaker sound effects from sound.c
  - [ ] Convert frequency/duration to SDL format
  - [ ] 13 game sounds (jump, fire, collect, etc.)
  - [ ] Sound priority system
- [ ] Port music system from music.c
  - [ ] Music playback
  - [ ] Looping
  - [ ] Start/stop controls

**Reference Code:**
- `src/sound.c`: PC speaker sound implementation
- `src/music.c`: Music playback
- `include/sound_data.h`: Sound effect data

**Success Criteria:**
- All sound effects play correctly
- Music loops properly
- Sound priority works (higher priority interrupts lower)
- Audio quality is acceptable

---

### Phase 7: UI and Menus
**Status:** Not Started
**Completion Date:** TBD
**Goal:** Implement game menus and HUD

**Tasks:**
- [ ] Port UI rendering from game_main.c
  - [ ] Score display
  - [ ] Inventory display (keys, wand)
  - [ ] HP/shield meter
  - [ ] Fireball power meter
  - [ ] Lives counter
- [ ] Implement menus from game_main.c
  - [ ] Startup notice
  - [ ] Title sequence
  - [ ] Story screen
  - [ ] Items screen
  - [ ] High scores
  - [ ] Keyboard setup
  - [ ] Pause menu
- [ ] Port special sequences
  - [ ] Beam-in animation
  - [ ] Beam-out animation
  - [ ] Death animation
  - [ ] Victory sequence
  - [ ] Game over screen

**Reference Code:**
- `src/game_main.c`: UI and menu functions
- `src/graphics.c`: Text rendering, fullscreen graphics

**Success Criteria:**
- HUD displays correctly during gameplay
- All menus are functional
- Animations play smoothly
- High score system works

---

### Phase 8: Game Loop Integration
**Status:** Not Started
**Completion Date:** TBD
**Goal:** Complete game flow and state management

**Tasks:**
- [ ] Port main game loop from game_main.c
  - [ ] Tick-based timing (60 Hz)
  - [ ] Input processing order
  - [ ] Physics update
  - [ ] Actor update
  - [ ] Rendering
- [ ] Implement game states
  - [ ] Menu state
  - [ ] Playing state
  - [ ] Paused state
  - [ ] Game over state
  - [ ] Victory state
- [ ] Port special mechanics
  - [ ] Teleportation
  - [ ] Door system
  - [ ] Stage boundaries
  - [ ] Fall death
  - [ ] Life loss

**Reference Code:**
- `src/game_main.c`: Lines 3725-3960 (game_loop function)
- `docs/GAME_LOOP_FLOW.md`: Flow documentation

**Success Criteria:**
- Game loop runs at consistent 60 FPS
- All game states transition correctly
- Full playthrough possible from start to end
- Win/lose conditions work properly

---

### Phase 9: Polish and Testing
**Status:** Not Started
**Completion Date:** TBD
**Goal:** Ensure quality and faithfulness

**Tasks:**
- [ ] Validate physics against original
  - [ ] Jump height measurements
  - [ ] Fall speed verification
  - [ ] Movement speed checks
- [ ] Test all levels and stages
  - [ ] Completability
  - [ ] No softlocks
  - [ ] Item accessibility
- [ ] Performance optimization
  - [ ] 60 FPS on target hardware
  - [ ] Memory usage profiling
  - [ ] Asset loading optimization
- [ ] Bug fixes and edge cases
  - [ ] Collision edge cases
  - [ ] Enemy behavior quirks
  - [ ] Save/load if implemented
- [ ] Quality of life improvements
  - [ ] Configurable controls
  - [ ] Resolution options
  - [ ] Fullscreen toggle
  - [ ] Volume controls

**Testing Resources:**
- DOSBox for side-by-side comparison
- Save states for scenario testing
- Assembly reference for exact behavior

**Success Criteria:**
- Game feels identical to original
- No major bugs or crashes
- Performance meets targets
- Playable on all target platforms

---

### Phase 10: Documentation and Release
**Status:** Not Started
**Completion Date:** TBD
**Goal:** Prepare for distribution

**Tasks:**
- [ ] Write comprehensive README
  - [ ] Building instructions
  - [ ] Running the game
  - [ ] Controls
  - [ ] Credits
- [ ] Create build scripts for all platforms
  - [ ] Windows (Visual Studio / MinGW)
  - [ ] macOS (Xcode / clang)
  - [ ] Linux (GCC)
- [ ] Package assets
  - [ ] Include original game assets (if permissible)
  - [ ] Asset extraction tools if needed
- [ ] License considerations
  - [ ] Code licensing (MIT/GPL)
  - [ ] Asset ownership clarification
- [ ] Distribution
  - [ ] GitHub releases
  - [ ] Binaries for each platform
  - [ ] Installation instructions

**Deliverables:**
- Complete, playable game
- Source code on GitHub
- Binary releases
- Documentation

---

## Development Workflow

### Iteration Strategy
1. Implement small, testable modules
2. Validate against C refactor reference
3. Compare behavior to original (DOSBox)
4. Fix discrepancies before moving on
5. Commit working increments frequently

### Testing Approach
- **Unit Testing:** Individual functions (physics, collision)
- **Integration Testing:** Subsystems working together
- **Scenario Testing:** Specific game situations
- **Playthrough Testing:** Full level completion
- **Regression Testing:** Ensure fixes don't break existing features

### Reference Workflow
1. Read function in `src/*.c` from comic-c
2. Understand logic and data structures
3. Port to modern C++ with SDL2
4. Cross-reference assembly for edge cases
5. Test behavior matches original

## Key Design Decisions

### Architecture
- **Modular Design:** Separate physics, rendering, actors, audio
- **Data-Driven:** Levels defined in data structures, not code
- **Modern Idioms:** Use C++ features for clarity (classes, vectors)
- **Compatibility Layer:** Abstract DOS-specific code (video memory, interrupts)

### Rendering
- **Native Resolution:** Render at original 320x200 internally
- **Scaling:** Scale up to modern resolutions (640x480, 1280x960, etc.)
- **Filtering:** Nearest-neighbor for pixel-perfect look
- **SDL_Renderer:** Use hardware acceleration when available

### Input
- **Keyboard Primary:** Arrow keys + Space (jump), Alt (open), etc.
- **Configurable:** Allow remapping
- **Gamepad Support:** Optional, map to modern controllers

### Audio
- **SDL_mixer:** Modern audio library
- **Synthesized Sounds:** Recreate PC speaker beeps/tones
- **Music Playback:** Port original music data or recreate

### Performance
- **Target:** Solid 60 FPS on modest hardware
- **Optimization:** Only where needed (profiling-driven)
- **Memory:** Keep original footprint small, modern extras optional

## File Organization

```
comic-modernization/
├── CMakeLists.txt           # Build configuration
├── README.md                # User documentation
├── MODERNIZATION_PLAN.md    # This file
├── .gitignore               # Git ignore rules
│
├── src/                     # Source code
│   ├── main.cpp             # Entry point and game loop
│   ├── physics.cpp          # Physics engine (COMPLETE)
│   ├── game.cpp             # Main game class (TODO)
│   ├── actors.cpp           # Enemies, fireballs, items (TODO)
│   ├── graphics.cpp         # Rendering (TODO)
│   ├── audio.cpp            # Sound and music (TODO)
│   ├── level.cpp            # Level loading and management (TODO)
│   ├── input.cpp            # Input handling (TODO)
│   └── ...                  # Additional modules
│
├── include/                 # Header files
│   ├── physics.h            # Physics constants and functions (COMPLETE)
│   ├── game.h
│   ├── actors.h
│   ├── graphics.h
│   ├── audio.h
│   ├── level.h
│   ├── input.h
│   └── ...
│
├── assets/                  # Game assets
│   ├── levels/              # Level data files
│   ├── graphics/            # Tilesets, sprites
│   ├── audio/               # Sound effects, music
│   └── ...
│
├── build/                   # Build output (generated)
│   └── captain_comic        # Executable
│
└── docs/                    # Additional documentation
    ├── PHYSICS.md           # Physics system details
    ├── RENDERING.md         # Rendering system details
    └── ...
```

## Success Criteria

### Functional Requirements
- ✅ Game launches on Windows, macOS, Linux
- [ ] All 8 levels playable from start to finish
- [ ] Physics matches original (jump height, fall speed, movement)
- [ ] All enemies behave correctly (5 AI types)
- [ ] All items work (firepower, treasures, keys, etc.)
- [ ] Sound effects and music play
- [ ] Menus and UI function properly
- [ ] Win/lose conditions trigger correctly

### Quality Requirements
- [ ] Runs at 60 FPS on target hardware
- [ ] No crashes or major bugs
- [ ] Clean, maintainable code
- [ ] Comprehensive documentation

### Fidelity Requirements
- [ ] Game looks like original (pixel-perfect at native res)
- [ ] Game feels like original (physics constants exact)
- [ ] Game sounds like original (PC speaker recreation)
- [ ] Level layouts identical
- [ ] Enemy behavior identical

## Resources

### Primary References
- [jsandas/comic-c](https://github.com/jsandas/comic-c) - C refactor (main reference)
- Assembly disassembly in reference/ folder - Low-level details

### Documentation
- [docs/REFACTOR_PLAN.md](https://github.com/jsandas/comic-c/blob/main/docs/REFACTOR_PLAN.md)
- [docs/GAME_LOOP_FLOW.md](https://github.com/jsandas/comic-c/blob/main/docs/GAME_LOOP_FLOW.md)
- [docs/CODING_STANDARDS.md](https://github.com/jsandas/comic-c/blob/main/docs/CODING_STANDARDS.md)

### Libraries
- [SDL2](https://www.libsdl.org/) - Graphics, audio, input
- [SDL_mixer](https://www.libsdl.org/projects/SDL_mixer/) - Audio playback
- [CMake](https://cmake.org/) - Build system

### Tools
- DOSBox - Running original for comparison
- Compiler: GCC/Clang/MSVC (C++17)
- IDE: VS Code, Visual Studio, Xcode, CLion

## Notes

### Design Philosophy
- **Fidelity First:** Match original behavior exactly before adding enhancements
- **Modern Standards:** Use modern C++ practices, not C-style code
- **Incremental Progress:** Small, tested changes over large refactors
- **Documentation:** Comment why, not just what

### Known Challenges
- PC speaker sound recreation requires synthesis
- EGA palette conversion to modern color spaces
- Timing differences between DOS and modern systems
- Handling DOS-specific file formats

### Future Enhancements (Post-MVP)
- Save/load system
- Level editor
- Mod support
- Enhanced graphics mode (HD sprites)
- Network multiplayer (?)
- Speedrun timer
- Achievements

## Timeline Estimate

**Phase 2 (Core Physics):** 1 week
**Phase 3 (Rendering):** 2 weeks  
**Phase 4 (Level System):** 1 week
**Phase 5 (Actor System):** 2 weeks
**Phase 6 (Audio):** 1 week
**Phase 7 (UI/Menus):** 1 week
**Phase 8 (Game Loop):** 1 week
**Phase 9 (Polish/Testing):** 2 weeks
**Phase 10 (Documentation):** 1 week

**Total Estimated Time:** ~12 weeks (3 months)

*Note: Estimates assume part-time development (10-15 hours/week)*

## Contributing

This is a learning/modernization project. Contributions welcome after MVP completion.

### How to Contribute
1. Read this plan and understand the architecture
2. Check current status and pick a task
3. Reference the C refactor code for implementation
4. Write tests for your changes
5. Submit pull request with clear description

### Coding Standards
- Follow C++17 best practices
- Use consistent naming (snake_case for functions, PascalCase for classes)
- Comment complex logic
- Keep functions focused and small
- Write self-documenting code

---

**Last Updated:** 2024-02-07  
**Author:** Development Team  
**Project:** Captain Comic Modernization
