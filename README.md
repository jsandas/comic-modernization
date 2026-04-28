# Captain Comic Modernization

A modern recreation of the classic 1988 DOS game *The Adventures of Captain Comic* using SDL2 and C++.

## Project Status

**Current Phase:** Game Loop Integration (Phase 8 - In Progress)

✅ Foundation complete (SDL2 setup, build system, basic game loop)  
✅ Core physics complete (gravity, jumping, collision, stage transitions)  
✅ Rendering system complete (tiles, sprites, animations, camera)  
✅ Level system complete (all 8 levels, doors, stage transitions)  
✅ Actor system complete (enemies, fireballs, items — Phase 5)  
✅ Audio system complete (SFX + music — Phase 6)  
✅ UI/Menus complete (title sequence, HUD, menus, animations — Phase 7)  
🔄 Game loop integration in progress (Phase 8)

## About

This project ports the game to modern systems while maintaining behavioral fidelity to the original. We're using the [C refactor by jsandas](https://github.com/jsandas/comic-c) as the primary reference, which provides well-structured game logic translated from the original x86-16 assembly.

**Technology Stack:**
- **SDL2** - Graphics, audio, and input
- **C++17** - Modern language features
- **CMake** - Cross-platform builds

**Target Platforms:**
- Windows
- macOS
- Linux

## Building

### Prerequisites

**Required:**
- CMake 3.16+
- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- SDL2 development libraries (SDL2, SDL2_image, SDL2_ttf, SDL2_mixer)
- Original game files (https://archive.org/download/TheAdventuresOfCaptainComic/AdventuresOfCaptainComicEpisode1The-PlanetOfDeathR5sw1991michaelA.Denioaction.zip)

### Assets

The build requires the original DOS game data to extract graphics, maps,
sprites and sounds.  Revision 5 of *Captain Comic* is used by the
`tools/extract_assets.py` script, which places the converted PNGs into
`assets/` subfolders.

1. Download the ZIP archive from Archive.org and unpack it into the
   `original/` directory (the script will look there):

```bash
curl -L -o comic_r5.zip \
  https://archive.org/download/TheAdventuresOfCaptainComic/AdventuresOfCaptainComicEpisode1The-PlanetOfDeathR5sw1991michaelA.Denioaction.zip
unzip comic_r5.zip -d original
rm comic_r5.zip
```

2. Install the Python dependencies and run the extractor:

```bash
pip install -r tools/requirements.txt
python3 tools/extract_assets.py --orig original
```

After extraction the `assets/` folder will contain neatly categorized
subdirectories (`sprites`, `tiles`, `maps`, `shp`, `sounds`, etc.) that
are referenced by the C++ code.

### macOS

```bash
# Install SDL2
brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer

# Build
mkdir build && cd build
cmake ..
make

# Run
./captain_comic
```

### Linux

```bash
# Install SDL2 (Ubuntu/Debian)
sudo apt-get install cmake g++ libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev

# Or Fedora
sudo dnf install cmake gcc-c++ sdl2-compat-devel SDL2_image-devel SDL2_ttf-devel SDL2_mixer-devel

# Build
mkdir build && cd build
cmake ..
make

# Run
./captain_comic
```

### Windows

```bash
vcpkg install sdl2 sdl2-image sdl2-ttf sdl2-mixer

# Build with Visual Studio or MinGW
mkdir build && cd build
cmake ..
cmake --build .

# Run
.\captain_comic.exe
```

## Current Features

- ✅ SDL2 window and event loop
- ✅ Keyboard input handling (arrow keys, space, Ctrl to fire)
- ✅ Complete physics system:
  - ✅ Gravity and terminal velocity
  - ✅ Jumping with original constants (GRAVITY=5, ACCELERATION=7)
  - ✅ Ceiling, floor, and wall collision detection
  - ✅ Mid-air momentum and drag
  - ✅ Stage boundary transitions (left/right edge)
  - ✅ Camera following with viewport scrolling
- ✅ Full rendering system:
  - ✅ Tile rendering from converted PNG tilesets (all 8 levels)
  - ✅ Player sprite with idle, run (3-frame), and jump animations
  - ✅ Direction-aware sprite rendering (left/right facing)
  - ✅ Enemy sprite rendering with GIF-based animation (loop and ping-pong)
  - ✅ Fireball rendering (2-frame animated sprites)
  - ✅ Hardware-accelerated (SDL2 renderer)
- ✅ Level system:
  - ✅ All 8 levels (LAKE, FOREST, SPACE, BASE, CAVE, SHED, CASTLE, COMP)
  - ✅ 3 stages per level with complete tile, door, and enemy data
  - ✅ Door system with key requirement and level/stage transitions
  - ✅ Stage transitions at left/right boundaries
- ✅ Enemy system (Actor System):
  - ✅ All 5 AI behaviors: Bounce, Leap, Roll, Seek, Shy
  - ✅ Enemy spawning, despawning, and respawn cycling
  - ✅ Enemy-player collision (damage trigger)
  - ✅ Death animations: white spark (killed by fireball), red spark (hit player)
- ✅ Fireball system:
  - ✅ Up to 5 simultaneous fireballs (based on Blastola Cola count)
  - ✅ Horizontal movement (±2 units/tick) in facing direction
  - ✅ Corkscrew motion when Corkscrew item is held
  - ✅ Fireball-enemy collision detection and kill
  - ✅ Fireball meter with 2-tick charge/discharge rate
- ✅ Item system:
  - ✅ Item placement and rendering (16×16 px, 2-frame animation)
  - ✅ Collision detection and collection
  - ✅ Blastola Cola: increase firepower (max 5)
  - ✅ Corkscrew: fireball vertical oscillation
  - ✅ Boots: increased jump power (4→5)
  - ✅ Lantern: castle lighting flag
  - ✅ Shield: HP refill (placeholder)
  - ✅ Door Key: unlock doors
  - ✅ Teleport Wand: special teleport ability
  - ✅ Treasures (Gems, Crown, Gold): victory tracking
- ✅ Audio system:
  - ✅ PC-speaker-style square-wave SFX synthesis (SDL2_mixer)
  - ✅ Single-channel priority SFX system (higher priority interrupts lower)
  - ✅ 10 sound effects: fire, item collect, door open, stage transition, enemy hit, player hit, player die, game over, teleport
  - ✅ Title music with full looping (100-note melody)
  - ✅ Dedicated music channel independent from SFX
- ✅ Title sequence:
  - ✅ Title screen (SYS000.EGA) with 6-step palette fade-in
  - ✅ Story screen (SYS001.EGA) and items/controls screen (SYS004.EGA)
  - ✅ Title music playback during sequence
  - ✅ Letterbox rendering for 320×200 EGA content
  - ✅ `--skip-title` flag to bypass sequence during development
- ✅ HUD/UI system:
  - ✅ Score display (6-digit base-100 encoding)
  - ✅ Lives counter (0-5 icons, bright/dark states)
  - ✅ HP meter (6 cells)
  - ✅ Fireball meter (6 cells, full/half/empty states)
  - ✅ Full inventory grid (9 items, 3×3 layout, 2-frame animation)
  - ✅ Menus: pause, high scores, keyboard setup, startup notice
  - ✅ Beam-in/beam-out, death, and victory animations
- ✅ **Debug/Cheat System** (development tool, `--debug` flag required):
  - ✅ Noclip mode (F1)
  - ✅ Level/stage warp (F2)
  - ✅ Debug overlay — coordinates, velocity, level/stage (F3)
  - ✅ Position warp (F4)
  - ✅ Door key toggle (F5)
  - ✅ Item granting (F6) — grant any item for testing effects

## Roadmap

See [MODERNIZATION_PLAN.md](docs/MODERNIZATION_PLAN.md) for the complete 10-phase implementation plan:

1. ✅ **Foundation** - SDL2 setup, build system
2. ✅ **Core Physics** - Gravity, jumping, collision, stage transitions
3. ✅ **Rendering** - Tiles, sprites, animations, hardware acceleration
4. ✅ **Level System** - All 8 levels, doors, stage transitions
5. ✅ **Actors** - Enemies, fireballs, items
6. ✅ **Audio** - SFX (10 sounds), title music, priority system
7. ✅ **UI/Menus** - Title sequence, HUD, menus, death/victory animations
8. 🔄 **Game Loop** - Complete game flow, states
9. ⏸️ **Polish** - Testing, optimization
10. ⏸️ **Release** - Packaging, distribution
## Project Structure

```
comic-modernization/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── docs/                       # Project documentation and roadmap
├── include/                    # Header files
├── src/                        # Game source
├── tests/                      # Test suite
├── tools/                      # Development tools
├── original/                   # Original game assets (local)
├── assets/                     # Modernized assets (in progress)
└── build/                      # Build output (generated)
```

## Controls

- **Arrow Keys** - Move left/right
- **Space** - Jump
- **Left/Right Ctrl** - Fire fireball
- **Alt** - Open doors

### Debug Mode (with `--debug` flag)

- **F1** - Toggle noclip (walk through walls)
- **F2** - Level warp (interactive menu to select level 0-7 and stage 0-2)
- **F3** - Toggle debug overlay (shows X/Y coordinates, velocity, level/stage)
- **F4** - Position warp (teleport to specific coordinates)
- **F5** - Toggle door key (grant/remove ability to open doors)
- **F6** - Grant item (select any item to test effects: Blastola Cola, Boots, Corkscrew, etc.)

## Development

### Reference Materials

- [jsandas/comic-c](https://github.com/jsandas/comic-c) - Primary reference (C refactor)
- Assembly disassembly - For low-level details and validation

### Workflow

1. Port logic from C refactor reference
2. Validate physics constants (GRAVITY=5, ACCELERATION=7, etc.)
3. Test behavior against original (DOSBox)
4. Commit working increments

### Testing

Automated unit tests cover physics, actors, items, UI helpers, and audio. Run with:

```bash
cd build && ctest --output-on-failure
```

### Tools

Generate compiled-in tile data from the original PT files:

```bash
g++ -std=c++17 -o tools/generate_tiles tools/generate_tiles.cpp
./tools/generate_tiles            # Run from repo root
./tools/generate_tiles /path/to/comic-modernization
```

The tool expects the original data in the `original/` directory and writes
`src/level_tiles.cpp`.

## Contributing

This is an active development project. Contributions welcome once core systems are complete.

### How to Help

- Test builds on different platforms
- Report bugs and issues
- Suggest improvements (after MVP)
- Documentation improvements

## License

Code: TBD (likely MIT or GPL)  
Assets: Original game assets © Michael Denio - consult original licensing

## Credits

- **Original Game**: Michael Denio (1988)
- **C Refactor**: [jsandas/comic-c](https://github.com/jsandas/comic-c)
- **Modernization**: This project's contributors

## Links

- Original Game: [Wikipedia](https://en.wikipedia.org/wiki/The_Adventures_of_Captain_Comic)
- C Refactor: [github.com/jsandas/comic-c](https://github.com/jsandas/comic-c)
- SDL2: [libsdl.org](https://www.libsdl.org/)

---

**Last Updated:** 2026-04-26  
**Status:** Active Development (Phase 8 of 10)