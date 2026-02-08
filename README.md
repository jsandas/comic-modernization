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
- SDL2 development libraries

### macOS

```bash
# Install SDL2
brew install sdl2

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
sudo apt-get install libsdl2-dev

# Or Fedora
sudo dnf install SDL2-devel

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
10. ├── main.cpp                # Entry point and game loop
│   └── physics.cpp             # Physics engine (complete)
├── include/
│   └── physics.h               # Physics constants and functions
## Project Structure

```
comic-modernization/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── MODERNIZATION_PLAN.md       #  (on ground or in air)
- **Space** - Jump (hold for higher jump)
- **Close Window** - Exit game Entry point and game loop
├── include/                    # Headers (TBD)
├── assets/                     # Game assets (not in repo)
└── build/                      # Build output (generated)
```

## Controls (Current)

- **Arrow Keys** - Move left/right
- **Space** - Jump
- **ESC** - (Not yet implemented)

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
- **C RefactPhase 2 Nearly Complete (85% - Physics System)

**What's Working:**
- Player falls with gravity and lands on platforms
- Jump with space bar (hold for higher jumps)
- Move left/right with arrow keys
- Collision with walls, floors, and ceilings
- Camera follows player as they move
- Test level with ground, platforms, and walls(https://github.com/jsandas/comic-c)
- **Modernization**: This project's contributors

## Links

- Original Game: [Wikipedia](https://en.wikipedia.org/wiki/The_Adventures_of_Captain_Comic)
- C Refactor: [github.com/jsandas/comic-c](https://github.com/jsandas/comic-c)
- SDL2: [libsdl.org](https://www.libsdl.org/)

---

**Last Updated:** 2024-02-07  
**Status:** Early Development (Phase 2 of 10)