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
- **Unit-test helper:** `g_skip_load_on_door` flag allows activate_door() to update
  state without actually loading levels/stages, keeping tests deterministic
  and avoiding reciprocal-door warnings when using minimal test levels.
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

## Developer Tools & Debug Features

**Status:** ✅ COMPLETE
**Implementation Date:** 2026-02-15
**Goal:** Provide development and debugging aids for testing and validation

### CheatSystem Class
A centralized system for managing debug cheats and development tools. Activated via `--debug` command-line flag; all cheats remain silently disabled without it.

**Architecture:**
- Implemented as `CheatSystem` class following RAII pattern (matching `GraphicsSystem`)
- Global instance `g_cheats` initialized at startup
- Input processed during main game loop after regular keyboard handling
- Command-line parsing in `main()` for debug mode activation

**Features:**

1. **Noclip Mode (F1)**
   - Disables collision detection entirely
   - Player walks through all solid tiles and obstacles
   - Useful for: Testing level layouts, debugging physics edge cases, exploring level design

2. **Level Warp (F2)**
   - Interactive two-step menu: select level (0-7), then stage (0-2)
   - Levels: LAKE (0), FOREST (1), SPACE (2), BASE (3), CAVE (4), SHED (5), CASTLE (6), COMP (7)
   - Resets player position to safe spawn (20, 14) on warp
   - Clears velocity and jump state
   - Useful for: Testing specific levels without playing through, accessing late-game content

3. **Position Warp (F4)**
   - Teleport player to specific X,Y coordinates
   - Input validation: X (0-255), Y (0-19)
   - Auto-adjusts camera to keep player on-screen
   - Resets velocity
   - Useful for: Testing collision at specific tiles, placing player near objectives, testing boundaries

4. **Debug Overlay (F3)**
   - Semi-transparent overlay in top-left corner displaying:
     - Player X,Y coordinates (in cyan text)
     - Current level and stage (in cyan text)
     - Visual velocity bar (red, vertical)
     - Visual momentum bar (blue, horizontal)
     - Noclip indicator (green square when active)
   - Uses SDL2_ttf for text rendering with system font fallback chain
   - Useful for: Verifying position during complex maneuvers, monitoring physics state, visual feedback on cheat status

5. **Door Key Toggle (F5)**
   - Toggle the door key inventory item on/off
   - When active (1): Allows opening all doors
   - When inactive (0): Cannot open doors even if they're nearby
   - Useful for: Testing door mechanics, accessing specific areas, bypassing door locks for testing

6. **Item Granting (F6)**
   - Interactive menu to select and grant any item (0-9)
   - Items available:
     - 0 = Corkscrew (fireball vertical oscillation)
     - 1 = Door Key (unlock doors)
     - 2 = Boots (increased jump power)
     - 3 = Lantern (castle lighting)
     - 4 = Teleport Wand (special teleport)
     - 5 = Gems (treasure 1/3)
     - 6 = Crown (treasure 2/3)
     - 7 = Gold (treasure 3/3)
     - 8 = Blastola Cola (increase firepower)
     - 9 = Shield (HP refill)
   - Useful for: Testing item effects without playing through levels, debugging item-dependent mechanics, validating item collection logic

**Console Logging:**
- All cheat activations logged to stdout with `[CHEAT]` prefix
- Input validation feedback (e.g., invalid coordinates)
- Level/stage warp confirmations
- Door key grant/removal notifications
- Users can see cheat activity in terminal even if overlay disabled

**Command-Line Interface:**
```bash
./captain_comic --debug    # Enable all cheats and F-key controls
./captain_comic --help     # Display help message
./captain_comic            # Normal mode (cheats disabled)
```

**Implementation Details:**
- Multi-step input handling for complex cheats (level warp, position warp, item granting)
- Input buffering for coordinate entry with backspace support
- Console prompts guide users through multi-step operations
- Proper cleanup of system resources (SDL_ttf font) in destructor
- Font loading with cross-platform fallback (Menlo, Courier on macOS; DejaVuSansMono on Linux; lucidaconsole on Windows)
- Item granting directly calls `ActorSystem::apply_item_effect()` for all 10 item types

**Technical Specs:**
- Dependencies: SDL2_ttf (added to CMakeLists.txt)
- Code Coverage:
  - New files: `include/cheats.h`, `src/cheats.cpp`
  - Modified: `src/main.cpp` (command-line parsing, cheat input integration)
  - Modified: `src/physics.cpp`, `include/physics.h` (noclip flag implementation)
  - Modified: `src/graphics.cpp`, `include/graphics.h` (debug overlay rendering, text support)
  - Modified: `CMakeLists.txt` (cheats.cpp, SDL2_ttf)

**Testing:**
- Builds without warnings or errors ✅
- All existing tests pass ✅
- Cheats silently ignored when --debug not specified ✅
- Debug overlay font loads successfully ✅
- Position validation prevents out-of-bounds warping ✅

**Future Enhancements:**
- God mode (invincibility) - deferred to when damage/HP system exists
- Enemy spawner/despawner controls
- Speed-up/slow-down time controls
- Collision visualization (show collision boxes)

---

### Phase 5: Actor System
**Status:** Complete
**Current Stage:** Phase 5.6 - Item System ✅ COMPLETE
**Completion Date:** 2026-02-22
**Goal:** Implement enemies, fireballs, and items

**Phase 5.1: Enemy System Core ✅ COMPLETE**
**Objective:** Port complete enemy spawning, AI behaviors, collision, and despawning systems from reference C code

**Completed:**
- [x] Created `ActorSystem` class with core architecture
  - [x] Enemy data structure (`enemy_t`) with position, velocity, animation, behavior, state
  - [x] Enemy spawning logic: off-screen spawn positions, spawn rate limiting (1 per tick)
  - [x] All 5 AI behaviors fully implemented:
    - [x] Bounce: Diagonal bouncing with independent X/Y velocities
    - [x] Leap: Jumping arc with gravity (Bug-eyes, Beach Ball)
    - [x] Roll: Ground-following locomotion (Glow Globe)
    - [x] Seek: Pathfinding toward player (Killer Bee)
    - [x] Shy: Fleeing when player faces, approaching otherwise (Shy Bird)
  - [x] Enemy-player collision detection (2×4 unit collision box)
  - [x] Enemy despawn logic: distance-based and bottom-of-screen despawn
  - [x] Death animation states: white spark (despawned), red spark (hit by player)
  - [x] Respawn cycling: 20→40→60→80→100→20 tick intervals
  - [x] Movement throttling: alternating motion for slow enemies, every-tick for fast
  - [x] Animation frame handling and looping
- [x] Created `include/actors.h` header with complete API
- [x] Created `src/actors.cpp` implementation (1000+ lines of AI logic)
- [x] Updated `CMakeLists.txt` to build actors.cpp
- [x] Compilation successful, all tests passing

**Phase 5.2: Sprite/Animation Loading ✅ COMPLETE**
- [x] Implemented `load_enemy_sprite()` in `GraphicsSystem` with GIF-based asset loading
- [x] Load left/right facing animation frames using `IMG_LoadAnimation()` (SDL_image)
- [x] Handle `ENEMY_HORIZONTAL_DUPLICATED` (mirror left frames for right direction)
- [x] Handle `ENEMY_HORIZONTAL_SEPARATE` (load independent right-facing GIF)
- [x] Handle `ENEMY_ANIMATION_LOOP` (0,1,2,0,1,... forward loop)
- [x] Handle `ENEMY_ANIMATION_ALTERNATE` (0,1,2,1,0,... ping-pong)
- [x] `frame_sequence` vector added to `SpriteAnimationData` struct
- [x] `build_enemy_animation_sequence()` free function generates frame index sequences
- [x] Composite cache key (`name:num_frames:horizontal:animation`) avoids redundant loads
- [x] Added `test_enemy_animation_sequence` unit test; all tests passing

**Phase 5.3: Enemy Rendering ✅ COMPLETE**
- [x] Implemented `render_enemies()` in `ActorSystem`
- [x] Renders each spawned enemy using current `frame_sequence` index
- [x] Uses `frames_right` for `ENEMY_HORIZONTAL_SEPARATE` + right-facing, otherwise flips `frames_left`
- [x] Camera culling: skips enemies whose X position is outside `[camera_x - 2, camera_x + PLAYFIELD_WIDTH + 2)` game units
- [x] Enemies rendered before player sprite (correct draw order)

**Phase 5.4: Integration ✅ COMPLETE**
- [x] `ActorSystem` instantiated and initialized in `main()`
- [x] `setup_enemies_for_stage()` called after level load and on level/stage change
- [x] `actor_system.update()` called each physics tick with Comic position, facing, tiles, camera
- [x] `actor_system.render_enemies()` called each frame before player rendering
- [x] Level/stage change detection triggers enemy re-setup on transitions

**Phase 5.5: Fireball System ✅ COMPLETE**
**Objective:** Port fireball spawning, movement, collision, and rendering from reference C code

**Remaining Tasks:**
- [x] Port fireball system from actors.c
  - [x] Fireball data structure (`fireball_t`: y, x, vel, corkscrew_phase, animation)
  - [x] Fireball constants (MAX_NUM_FIREBALLS=5, FIREBALL_DEAD=0xFF, FIREBALL_VELOCITY=2)
  - [x] Fireball spawning (`try_to_fire()`: find open slot, set position/velocity/phase)
  - [x] Horizontal movement (±2 game units per tick)
  - [x] Corkscrew motion (Y oscillation when `comic_has_corkscrew` is set)
  - [x] Fireball meter management (2-tick counter cycle: decrements when firing, increments when not)
  - [x] Fireball-enemy collision (0–1 vertical + ±1 horizontal box → ENEMY_STATE_WHITE_SPARK)
  - [x] Fireball deactivation (off-screen left/right bounds)
  - [x] Fireball rendering (16×8 px sprites scaled 2× per game unit)
  - [x] Sprite loading (`sprite-fireball_0.png`, `sprite-fireball_1.png`)
  - [x] Fire key input wired to Left/Right Ctrl in game loop

**Phase 5.6: Item System ✅ COMPLETE**
**Objective:** Port item collection, animation, and effect application from reference C code

**Completed:**
- [x] Item data structures and tracking
  - [x] Item collection state per [level][stage] (8×3 array)
  - [x] Current item position and type from stage data
  - [x] Item animation counter (toggles 0→1 each tick)
  - [x] Item sprite storage for all item types (even/odd frames)
- [x] Item sprite loading
  - [x] Load 16×16 pixel sprites for all item types
  - [x] Two-frame animation (even/odd frames)
  - [x] Sprite names: `item_<name>_0`, `item_<name>_1`
- [x] Item animation system
  - [x] Two-frame toggle each game tick
  - [x] Simple alternation: 0→1→0→1
- [x] Item collision detection
  - [x] Horizontal tolerance: ±1 game unit
  - [x] Vertical tolerance: 0 to 4 game units
  - [x] Check only when item visible in playfield
  - [x] Mark as collected on collision
- [x] Item collection effects
  - [x] Blastola Cola: increment firepower (max 5)
  - [x] Corkscrew: enable fireball vertical oscillation
  - [x] Boots: increase jump power (4→5)
  - [x] Lantern: castle lighting flag
  - [x] Shield: HP refill (placeholder for future HP system)
  - [x] Door Key: unlock doors
  - [x] Teleport Wand: special teleport ability
  - [x] Gems, Crown, Gold: treasure tracking (victory at 3/3)
- [x] Item rendering
  - [x] Render 16×16 pixel sprites at item position
  - [x] Skip if already collected
  - [x] Camera culling (off-screen items not rendered)
  - [x] Scale to render_scale per game unit
- [x] Integration with game loop
  - [x] Call `handle_item()` each tick in `ActorSystem::update()`
  - [x] Call `render_item()` each frame before player rendering
  - [x] Update `comic_jump_power` from `get_jump_power()` (boots effect)
  - [x] Pass level_index to `setup_enemies_for_stage()` for item tracking
- [x] Unit tests
  - [x] Test item collection tracking
  - [x] Test Blastola Cola firepower increase (max 5)
  - [x] Test Boots jump power increase
  - [x] Test Corkscrew flag
  - [x] Test treasure counting (3 treasures)
  - [x] Test Door Key, Teleport Wand, Lantern flags
  - [x] All 6 item tests passing

**Reference Code:**
- `src/actors.c`: Lines 1-1800+ (all actor systems)
- `include/actors.h`: Data structures and constants

**Success Criteria:**
- All enemy behaviors work correctly ✅
- Enemies spawn at proper distances ✅
- Fireballs kill enemies on contact ✅
- Items grant correct abilities ✅
- Treasure collection triggers win sequence (victory logic pending)

---

### Phase 6: Audio System
**Status:** In Progress
**Current Stage:** Phase 6.2 - Sound Definitions & Event Mapping ✅ COMPLETE
**Completion Date:** 2026-02-27
**Goal:** Implement sound effects and music

**Phase 6.1: Audio Foundation ✅ COMPLETE**
- [x] Set up SDL_mixer for audio
- [x] Implement foundational `AudioSystem` module (`include/audio.h`, `src/audio.cpp`)
- [x] Add synthesized PC-speaker-style square-wave SFX generation (SDL_mixer `Mix_Chunk`)
- [x] Implement single-channel SFX priority system (higher priority interrupts lower)
- [x] Integrate core gameplay SFX triggers
  - [x] Fire
  - [x] Item collect
  - [x] Door open
  - [x] Stage edge transition

**Phase 6.2: Sound Definitions & Event Mapping ✅ COMPLETE**
**Objective:** Port core game sound definitions with frequency sequences and map each to in-game events

**Completed:**
- [x] Extended audio system to support multi-frequency sound sequences
  - [x] Created `FrequencyNote` structure for frequency/duration pairs
  - [x] Implemented `create_sound_sequence_chunk()` to synthesize complete frequency sequences
  - [x] Updated SDL_mixer chunk synthesis to handle frequency changes per note
- [x] Ported 10 core PC-speaker sound definitions
  - [x] FIRE: Two-note fireball launch (145→155 Hz)
  - [x] ITEM_COLLECT: All item pickups including treasures, powerups, shield (294→371→441→582 Hz)
  - [x] DOOR_OPEN: Nine-note door sequence (310↔466 Hz palindrome)
  - [x] STAGE_TRANSITION: Stage crossing motif (see `src/audio.cpp` for exact note sequence)
  - [x] ENEMY_HIT: Two-note enemy collision (582→1165 Hz)
  - [x] PLAYER_HIT: Three-note damage sound (97→83→72 Hz descending)
  - [x] PLAYER_DIE: Six-note death sequence (97→83→72→582→291→194 Hz)
  - [x] GAME_OVER: Jingle sequence (B3→C4→D4→E4→G4)
  - [x] TELEPORT: Seven-note teleport palindrome (145Hz oscillating)
  - [x] UNUSED_0: Reserved (no jump sound in original game)
- [x] Integrated sound triggers into game events
  - [x] FIRE trigger: `actors.cpp` - `try_to_fire()` on fireball spawn
  - [x] ITEM_COLLECT trigger: `actors.cpp` - `collect_item()` on collision (all item types including treasures and shield)
  - [x] ENEMY_HIT trigger: `actors.cpp` - `handle_fireballs()` on fireball-enemy impact
  - [x] DOOR_OPEN trigger: `doors.cpp` - `activate_door()`
  - [x] STAGE_TRANSITION trigger: `physics.cpp` - stage boundary transitions
- [x] Updated `include/audio.h` with comprehensive documentation
- [x] Fixed SDL_mixer audio synthesis for multi-frequency sequences
- [x] All sounds use priority system (0-9); active sounds span 2-9, with `UNUSED_0` reserved at 0
- [x] Tick-to-millisecond conversion using original 18.2 Hz game tick rate (55ms/tick)

**Technical Details:**
- Audio frequency conversion: Original PIT divisors → modern Hz values
- Multi-frequency synthesis: Concatenates square waves for each note in sequence
- Duration handling: Game ticks (55ms/tick) → SDL milliseconds
- Priority system: Higher priority sounds interrupt lower priority ones
- Channel management: Single SFX channel with non-preemptable higher-priority sounds

**Reference Implementation:**
- Based on jsandas/comic-c sound.c and sound_data.c
- Maintains original frequency sequences and timing relationships
- PC speaker square-wave synthesis recreated in SDL2 audio

**Phase 6.3: Music System ✅ COMPLETE**
**Objective:** Port music playback from reference C code with full looping support

**Completed:**
- [x] Ported title music melody (100+ notes, ~15 second loop)
  - [x] Full note sequence from jsandas/comic-c SOUND_TITLE
  - [x] Perfect fidelity to original PC speaker implementation
- [x] Ported game over music (reference-only, currently uses GAME_OVER SFX)
  - [x] Short 9-note jingle sequence
  - [x] Available for future use if game over becomes a music track
- [x] Music playback system separate from SFX
  - [x] Dedicated music channel (channel 1) independent from SFX channel (channel 0)
  - [x] Infinite loop support: `Mix_PlayChannel(..., chunk, -1)`
  - [x] Priority handling: music plays on separate channel, no priority conflicts
- [x] Music control functions API
  - [x] `play_game_music(GameMusic music)` - Start playing a music track
  - [x] `stop_game_music()` - Stop current music
  - [x] `is_game_music_playing()` - Query playback status
  - [x] `get_current_music()` - Return currently playing track
- [x] Full note frequency coverage
  - [x] Added missing note constants (NOTE_C3-NOTE_B4, NOTE_C5, etc.)
  - [x] Covers full range from C3 (131 Hz) to G5 (784 Hz)
- [x] Music initialization and cleanup
  - [x] Music loaded during `initialize_audio_system()`
  - [x] Music cleanup in `shutdown_audio_system()`
  - [x] Proper channel management and halt on shutdown

**Implementation Details:**
- Title music: 100 notes, ~15.2 seconds per loop (no priority; plays on independent music channel)
- Game over music: 9 notes, ~1.5 seconds (reference for future enhancement)
- Music data structure: `LoadedMusic` with chunk pointer and duration
- Looping: Uses SDL_mixer's -1 loop count (infinite loop) vs SFX's 0 (one-shot)
- Channel allocation: Music automatically allocated to channel 1 on first play
- Synthesis: Same square-wave synthesis as SFX (see `create_sound_sequence_chunk()`)

**Technical Specs:**
- Dependencies: SDL2_mixer (same as SFX)
- Code Coverage:
  - Modified: `include/audio.h` (added GameMusic enum, 5 new functions)
  - Modified: `src/audio.cpp` (music sequences, LoadedMusic struct, playback functions)
  - Plus existing SFX infrastructure for synthesis and timing
- No terminal calls needed - pure API-based integration

**Usage Example (for future Title Sequence):**
```cpp
// Play title music during title screen
play_game_music(GameMusic::TITLE);  // Starts infinite loop

// Wait for user input...

// Stop music when transitioning to gameplay
stop_game_music();
```

**Usage Example (for future Victory/End-Game Sequence):**
```cpp
// When player collects 3rd treasure
if (comic_num_treasures == 3) {
    // Play beam-out animation
    // ...
    
    // Play title music during victory sequence
    play_game_music(GameMusic::TITLE);  // Reuses title music as per original
    
    // Award points while music plays
    // ...
    
    // Stop music before returning to main menu
    stop_game_music();
}
```

**Pending (Phase 7+):**
- [ ] Title sequence screen and menus (Phase 7)
- [ ] Victory/end-game sequence (Phase 7-8)
- [ ] Integration of title music into these sequences
- [ ] Optional: Additional music tracks for different game states

**Post-completion enhancements (non-original behavior):**
- [ ] Add distinct item-category SFX (POWERUP, SHIELD, TREASURE) as optional enhancements
- [ ] Add separate GameSound enum entries for enhancement-only item SFX
- [ ] Map item collection triggers to enhancement-only item SFX when enabled

**Todos:**
- [ ] Test all 10 implemented sounds with actual gameplay
- [ ] Adjust sound frequencies/durations if needed based on player feedback

---

### Phase 7: UI and Menus
**Status:** In Progress
**Current Stage:** Phase 7.1 - Title Sequence System ✅ COMPLETE
**Completion Date (7.1):** 2026-03-07
**Goal:** Implement game menus and HUD

**Phase 7.1: Title Sequence System ✅ COMPLETE**
**Objective:** Port complete title sequence from original game with proper visual transitions and asset loading

**Completed:**
- [x] Title sequence function structure
  - [x] Load and display title screen (SYS000.EGA) with palette fade-in
  - [x] Load and display story screen (SYS001.EGA) with palette fade-in
  - [x] Load game UI background (SYS003.EGA) into HUD texture for gameplay
  - [x] Load and display items/controls screen (SYS004.EGA)
  - [x] Proper blocking on keypresses between screens
  - [x] Title music playback (integrated with Phase 6 audio system)
- [x] Palette fade effects
  - [x] 6-step palette transition sequence mimicking DOS palette_fade_in()
  - [x] EGA color conversion (6-bit → 8-bit RGB)
  - [x] Multi-step color animation for smooth visual transitions
  - [x] Non-fatal handling for missing/invalid palette data (continue without fade)
- [x] Asset loading
  - [x] Load paletted PNG files (320×200 EGA graphics)
  - [x] Preserve indexed color format with palette
  - [x] Convert to RGBA textures for SDL rendering
  - [x] Proper error handling and logging
- [x] Timing and input handling
  - [x] Title screen auto-advances after ~770 ms (14 ticks × 55 ms)
  - [x] Story and items screens wait for keypress (blocking)
  - [x] Event polling during fade sequences allows SDL_QUIT detection
  - [x] Letterbox rendering for 320×200 content on modern resolutions
- [x] Integration with game loop
  - [x] `run_title_sequence()` called at startup
  - [x] `get_hud_texture()` returns preserved SYS003.EGA for in-game HUD rendering
  - [x] HUD texture rendered as background layer during gameplay
  - [x] Playfield viewport (8,8 offset, 192×160 size) matches original HUD layout
  - [x] `--skip-title` command-line flag bypasses sequence for development speed
  - [x] `cleanup_title_sequence()` properly releases loaded surfaces and textures
- [x] Created `include/title_sequence.h` and `src/title_sequence.cpp`
- [x] Created `TITLE_SEQUENCE.md` with detailed documentation

**Technical Implementation:**
- Paletted PNG loading preserves EGA palette data (8-bit indexed color)
- Letterbox calculation centralized in `GraphicsSystem::compute_letterbox_rect()`
- Fade sequence renders 6 frames with 55ms delays (matching original tick rate)
- Multi-step palette color transitions:
  - Step 1: All colors → dark gray (0x18)
  - Step 2: All colors → light gray (0x07)
  - Step 3: Background stays light gray, items/title → white (0x1f)
  - Step 4: Background → dark green (0x02), items → bright green (0x1a)
  - Step 5: Title → bright red (0x1c), background/items restored
  - Step 6: All restored to original colors
- Surface format conversion: paletted → RGBA32 for SDL rendering

**Reference Implementation:**
- Based on jsandas/comic-c `game_main.c:title_sequence()`
- Original palette fade sequence fully replicated
- Asset timing matches original (~770ms title display, instant story/items)

**Remaining Tasks (Phase 7.2+):**
- [ ] Port UI rendering from game_main.c
  - [ ] Score display (3-digit score at top)
  - [ ] Lives counter (life icons showing player lives)
  - [ ] HP/shield meter (health bar display)
  - [ ] Fireball power meter (charge indicator)
  - [ ] Inventory display (keys, wand, other items)
- [ ] Implement menus from game_main.c
  - [ ] Startup notice (initial acknowledgement screen)
  - [ ] High scores screen (leaderboard display)
  - [ ] Keyboard setup menu (control configuration)
  - [ ] Pause menu (in-game pause functionality)
- [ ] Port special sequences
  - [ ] Beam-in animation (entering level/stage)
  - [ ] Beam-out animation (exiting level/stage to victory)
  - [ ] Death animation (player death sequence)
  - [ ] Victory sequence (treasure collection celebration)
  - [ ] Game over screen (game-over state display)

**Reference Code:**
- `src/game_main.c`: UI and menu functions
- `src/graphics.c`: Text rendering, fullscreen graphics

**Success Criteria Phase 7.1:**
- ✅ Title sequence displays in correct order
- ✅ Palette fades smoothly (6 steps at 55ms each)
- ✅ All screens display correctly
- ✅ Title music plays during title screen
- ✅ Input advances story and items screens
- ✅ HUD texture preserved for gameplay rendering
- ✅ No crashes or memory leaks
- ✅ Seamless transition to gameplay

**Success Criteria (Full Phase 7):**
- HUD displays correctly during gameplay with all elements
- All menus are functional and match original
- Animations play smoothly
- High score system works
- All game states transition correctly

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
