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
- `level_data.c` - All 8 levels (LAKE, FOREST, SPACE, BASE, CAVE, SHED, CASTLE, COMP)
- `file_loaders.c` - Asset loading (.PT, .TT2, .SHP, .EGA files)

### Headers (include/)
- `globals.h` - Shared constants and state
- `physics.h` - Physics constants (GRAVITY=5, ACCELERATION=7, etc.)
- `actors.h` - Enemy behaviors, fireball mechanics
- `level_data.h` - Level structures and data

## Implementation Phases

### Phase 1: Foundation ✅ COMPLETE
**Goal:** Set up development environment and basic infrastructure

- [x] Initialize git repository
- [x] Create CMake build system
- [x] Install SDL2 dependencies
- [x] Create basic window and event loop
- [x] Implement input handling (keyboard)
- [x] Verify SDL2 linkage and execution

**Deliverable:** Application opens window, processes input, exits cleanly

---

### Phase 2: Core Physics 🔄 IN PROGRESS
**Goal:** Implement player movement and physics

**Tasks:**
- [x] Port physics constants from physics.h (GRAVITY, TERMINAL_VELOCITY, JUMP_ACCELERATION)
- [x] Implement `handle_fall_or_jump()` from physics.c
  - [x] Gravity application
  - [x] Jump counter logic
  - [x] Velocity integration
  - [ ] Ceiling collision
  - [x] Ground collision (simple placeholder)
- [ ] Implement `move_left()` and `move_right()` from physics.c
  - [ ] Wall collision detection
  - [ ] Camera following
  - [ ] Stage transitions
- [ ] Port tile map system
  - [ ] `address_of_tile_at_coordinates()`
  - [ ] `is_tile_solid()`
  - [ ] Tile data structures

**Reference Code:**
- `src/physics.c`: Lines 59-398 (handle_fall_or_jump, movement functions)
- `include/physics.h`: Constants and declarations
- `include/globals.h`: MAP_WIDTH, PLAYFIELD_WIDTH, etc.

**Success Criteria:**
- Player falls realistically with gravity
- Jumping height matches original (constants validated)
- Wall collision prevents movement through walls
- Floor detection works accurately

---

### Phase 3: Rendering System
**Goal:** Display game graphics

**Tasks:**
- [ ] Port tile rendering from graphics.c
  - [ ] Load .TT2 tileset files
  - [ ] Render tile maps to screen
  - [ ] Implement double-buffering
- [ ] Port sprite system from graphics.c
  - [ ] Load .SHP sprite files
  - [ ] Blit sprites with masking
  - [ ] Animation frame handling
- [ ] Implement camera system
  - [ ] Viewport scrolling
  - [ ] Player-centered camera
  - [ ] Clamping to stage bounds
- [ ] Port EGA palette to SDL
  - [ ] Convert 16-color EGA to RGB
  - [ ] Palette effects (fade, darken)
- [ ] Render player sprite
  - [ ] Standing, running, jumping animations
  - [ ] Direction facing

**Reference Code:**
- `src/graphics.c`: Lines 1-1200+ (rendering functions)
- `src/sprite_data.c`: Sprite definitions
- `include/graphics.h`: Video buffer constants

**Data Files Needed:**
- `*.TT2` - Tileset graphics (16x16 tiles)
- `*.PT` - Stage tile maps (128x10 tiles)
- `*.SHP` - Sprite files (player, enemies, effects)
- `*.EGA` - Fullscreen graphics (title, menus)

**Success Criteria:**
- Tiles render correctly from .TT2 files
- Sprites display with proper masking
- Animations play smoothly
- Camera follows player appropriately

---

### Phase 4: Level System
**Goal:** Load and manage game levels

**Tasks:**
- [ ] Port level data structures from level_data.h
  - [ ] `level_t` structure
  - [ ] `stage_t` structure
  - [ ] Door definitions
- [ ] Implement level loading from level_data.c
  - [ ] All 8 levels (LAKE through COMP)
  - [ ] Stage data (3 stages per level)
  - [ ] Enemy spawn data
  - [ ] Item placement
- [ ] Port file loaders from file_loaders.c
  - [ ] .PT file loader (tile maps)
  - [ ] .TT2 file loader (tilesets)
  - [ ] .SHP file loader (sprites)
  - [ ] .EGA file loader (fullscreen)
- [ ] Implement door system from doors.c
  - [ ] Door collision detection
  - [ ] Key checking
  - [ ] Level/stage transitions

**Reference Code:**
- `src/level_data.c`: All level definitions
- `src/file_loaders.c`: Asset loading
- `src/doors.c`: Door mechanics

**Success Criteria:**
- All 8 levels load correctly
- Stage transitions work (left/right boundaries)
- Doors function properly with keys
- Level data matches original exactly

---

### Phase 5: Actor System
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
│   ├── main.cpp             # Entry point
│   ├── game.cpp             # Main game class
│   ├── physics.cpp          # Physics engine
│   ├── actors.cpp           # Enemies, fireballs, items
│   ├── graphics.cpp         # Rendering
│   ├── audio.cpp            # Sound and music
│   ├── level.cpp            # Level loading and management
│   ├── input.cpp            # Input handling
│   └── ...                  # Additional modules
│
├── include/                 # Header files
│   ├── game.h
│   ├── physics.h
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

## Current Status (2024-02-07)

### Completed ✅
- Phase 1: Foundation (100%)
  - SDL2 setup
  - CMake build system
  - Window and event loop
  - Input handling

- Phase 2: Core Physics (30%)
  - Physics constants ported
  - Basic gravity and jumping
  - Simple ground collision
  - Jump counter logic

### In Progress 🔄
- Phase 2: Core Physics (remaining 70%)
  - Next: Ceiling collision
  - Next: Horizontal movement with walls
  - Next: Tile map integration

### Not Started ⏸️
- Phase 3-10 (pending completion of physics)

### Recent Accomplishments
- Created modernization plan with comprehensive phases
- Established SDL2 development environment
- Implemented basic player movement and physics
- Set up cross-platform build system
- Fixed compilation warnings (data type issues)

### Next Steps (Immediate)
1. Complete `handle_fall_or_jump()` ceiling collision
2. Implement tile map data structures
3. Port `move_left()` and `move_right()` with collision
4. Create simple test level with walls
5. Validate physics constants against original

### Blockers
- None currently

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
