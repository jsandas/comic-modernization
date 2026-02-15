# Captain Comic Modernization

A modern recreation of the classic 1988 DOS game *The Adventures of Captain Comic* using SDL2 and C++.

## Project Status

**Current Phase:** Core Physics Implementation (Phase 2 - 85% complete)

✅ Foundation complete (SDL2 setup, build system, basic game loop)  
✅ Physics system nearly complete (gravity, jumping, tile collision, camera)  
🔄 Final physics polish (stage transitions deferred to Phase 4)  
⏸️ Remaining phases pending

See [MODERNIZATION_PLAN.md](MODERNIZATION_PLAN.md) for complete roadmap and status.

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

- CMake 3.16+
- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- SDL2 development libraries (SDL2, SDL2_image, SDL2_ttf)

### macOS

```bash
# Install SDL2
brew install sdl2 sdl2_image sdl2_ttf

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
sudo apt-get install cmake g++ libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev

# Or Fedora
sudo dnf install cmake gcc-c++ sdl2-compat-devel SDL2_image-devel SDL2_ttf-devel

# Build
mkdir build && cd build
cmake ..
make

# Run
./captain_comic
```

### Windows

```bash
# Install SDL2 (download from libsdl.org or use vcpkg)
vcpkg install sdl2

# Build with Visual Studio or MinGW
mkdir build && cd build
cmake ..
cmake --build .

# Run
.\captain_comic.exe
```

## Current Features

- ✅ SDL2 window and event loop
- ✅ Keyboard input handling (arrow keys, space)
- ✅ Complete physics system:
  - ✅ Gravity and terminal velocity
  - ✅ Jumping with original constants (GRAVITY=5, ACCELERATION=7)
  - ✅ Ceiling collision detection
  - ✅ Floor/ground collision with tiles
  - ✅ Wall collision detection
  - ✅ Mid-air momentum and drag
  - ✅ Camera following with viewport scrolling
- ✅ Tile-based collision system
- ✅ Test level with platforms and walls
- ✅ Player rendering (yellow rectangle, 2x4 game units)
- ✅ Tile rendering (gray blocks for solid tiles)
- ✅ Modular architecture (separate physics module)
- ✅ **Debug/Cheat System** (development tool):
  - ✅ Noclip mode (walk through walls)
  - ✅ Level/stage warping (teleport to any level)
  - ✅ Position warping (teleport to coordinates)
  - ✅ Debug overlay (display coordinates, velocity, active cheats)
  - Toggled via `--debug` flag; all cheats disabled without it

## Roadmap

See [MODERNIZATION_PLAN.md](MODERNIZATION_PLAN.md) for the complete 10-phase implementation plan:

1. ✅ **Foundation** - SDL2 setup, build system
2. 🔄 **Core Physics** - Movement, collision, camera
3. ⏸️ **Rendering** - Tiles, sprites, animations
4. ⏸️ **Level System** - 8 levels, stages, doors
5. ⏸️ **Actors** - Enemies, fireballs, items
6. ⏸️ **Audio** - Sound effects, music
7. ⏸️ **UI/Menus** - HUD, title, high scores
8. ⏸️ **Game Loop** - Complete flow, states
9. ⏸️ **Polish** - Testing, optimization
10. ⏸️ **Release** - Packaging, distribution
## Project Structure

```
comic-modernization/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── MODERNIZATION_PLAN.md       # Roadmap and status
├── include/                    # Header files
├── src/                        # Game source
├── tests/                      # Test suite
├── tools/                      # Development tools
├── original/                   # Original game assets (local)
├── assets/                     # Modernized assets (in progress)
└── build/                      # Build output (generated)
```

## Controls (Current)

- **Arrow Keys** - Move left/right
- **Space** - Jump
- **Alt** - Open doors
- **ESC** - (Not yet implemented)

### Debug Mode (with `--debug` flag)

- **F1** - Toggle noclip (walk through walls)
- **F2** - Level warp (interactive menu to select level 0-7 and stage 0-2)
- **F3** - Toggle debug overlay (shows X/Y coordinates, velocity, level/stage)
- **F4** - Position warp (teleport to specific coordinates)
- **F5** - Toggle door key (grant/remove ability to open doors)

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

Currently manual testing. Automated tests planned for Phase 9.

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

**Last Updated:** 2026-02-15  
**Status:** Active Development (Phase 4 of 10)