# Captain Comic - C++ SDL2 Port

A modern port of the 1988 DOS platformer Captain Comic to C++ using SDL2.

## Project Status

Initial implementation phase - basic framework and build system established.

### Completed
- [x] CMake 3.10+ build system
- [x] SDL2 integration (video, audio, input)
- [x] Asset extraction pipeline (PNG graphics, WAV audio)
- [x] Core game data structures (GameState, Enemy, Fireball, TileMap)
- [x] Graphics system with EGA palette support (320×200)
- [x] Input handling system with configurable keymaps
- [x] Audio framework with PC speaker synthesis
- [x] Asset manager with filesystem::path cross-platform support
- [x] Fixed-timestep game loop (9.1 Hz tick rate)
- [x] Successful build with all assets copied to data/

### Next Steps
- [x] Implement tile and sprite rendering
- [ ] Port game physics and collision detection
- [ ] Implement enemy AI behaviors (5 types: bounce, leap, roll, seek, shy)
- [ ] Implement item collection and door mechanics
- [ ] Audio playback integration
- [ ] Game state management and level loading
- [ ] Player movement and animation
- [ ] UI rendering (HUD, score, lives)

## Build Instructions

### Prerequisites
- CMake 3.10 or later
- C++17 compatible compiler (AppleClang, GCC, Clang)
- SDL2 development libraries
- SDL2_image development libraries (optional, for PNG loading)

### Building

```bash
cd /path/to/comic-c
mkdir build
cd build
cmake ..
make
```

The build system will:
1. Extract game assets from the original game data via `make -C deriv`
2. Copy extracted assets to `build/data/`
3. Compile C++ source to `build/comic-c` executable

### Running

```bash
cd build
./comic-c
```

The executable will look for assets in `./data/` relative to its location.

## Directory Structure

```
.
├── CMakeLists.txt              # Top-level CMake configuration
├── src/
│   ├── CMakeLists.txt          # Source build configuration
│   ├── main.cpp                # Entry point and game loop
│   ├── assets/                 # Asset loading system
│   │   ├── AssetManager.h/cpp
│   ├── graphics/               # Rendering system
│   │   ├── Graphics.h/cpp
│   ├── audio/                  # Sound system
│   │   ├── Audio.h/cpp
│   ├── input/                  # Input handling
│   │   ├── Input.h/cpp
│   └── game/                   # Game logic
│       ├── Constants.h         # Game constants (from assembly)
│       ├── GameState.h/cpp     # Game state structures
├── deriv/                      # Asset extraction (Makefile)
├── orig/                       # Original game files (from archive.org)
├── disassembly/                # Original DOS assembly code
└── build/                      # Build artifacts (generated)
    ├── comic-c                 # Executable
    └── data/                   # Extracted assets (PNG, WAV, GIF)
```

## Game Architecture

### Fixed-Timestep Game Loop

The game runs at a fixed 9.1 Hz (matching the original DOS PC speaker interrupt rate), independent of rendering:

- **Game Logic**: Fixed 9.1 Hz tick rate for deterministic gameplay
- **Rendering**: Variable frame rate (60+ Hz with vsync) for smooth visual output
- **Input**: Polled per frame, state updated each tick
- **Audio**: SDL audio callback for continuous sound synthesis

### Game Constants

Key constants from the original assembly are defined in [src/game/Constants.h](src/game/Constants.h):

- **Resolution**: 320×200 pixels, 16-color EGA palette
- **Map Size**: 128×10 tiles, 16×16 pixels per tile
- **Game Mechanics**:
  - Max 4 enemies per stage
  - Max 5 fireballs
  - Max 3 doors per stage
  - Gravity: 5 units (3 in space level)
  - Terminal velocity: 23 units (1/8 pixel per tick)

### Data Structures

**GameState** - Main game state container
- Player data (position, velocity, HP, items, lives)
- Current level and stage
- Enemies and fireballs
- Map and tiles
- Score and game flags

**Enemy** - Enemy entity
- Position and velocity
- Behavior type (bounce, leap, roll, seek, shy)
- Animation frame and facing direction
- Spawn timer and restraint (for movement speed variation)

**Fireball** - Player projectile
- Position and velocity
- Facing direction

**TileMap** - Level map
- 128×10 tile IDs
- Solidity data per tile

## Graphics Rendering System

The rendering system provides tile and sprite drawing with efficient texture caching:

### Tile Rendering
- Individual tiles are loaded from tileset PNG files on-demand and cached
- Naming convention: `tileset-id` (e.g., `forest.tt2-00`, `forest.tt2-1a`)
- `renderTileMap()` renders the visible map with camera offset support
- Handles out-of-screen culling to reduce draw calls

### Sprite Rendering
- Sprites are loaded from the `/data/sprite-*.png` files
- Both texture object and texture info (dimensions) are cached
- `drawSprite()` renders sprites at specific screen coordinates
- `drawSpriteByName()` is a convenience wrapper for loading by name

### Asset Caching
- Textures are cached in-memory after first load
- `TextureInfo` struct stores texture pointer, width, and height
- Efficient `getTextureInfo()` method for dimension queries

## Asset Pipeline

Assets are extracted from the original DOS game via the existing tools:

1. **[deriv/Makefile](deriv/Makefile)** - Orchestrates extraction:
   - Unpacks executable (exepack)
   - Extracts EGA graphics (*.EGA → PNG)
   - Extracts tileset graphics (*.TT2 → PNG)
   - Extracts map data (*.PT → PNG preview + binary)
   - Extracts sprite animations (*.SHP → GIF)
   - Extracts sounds (→ WAV)

2. **Asset types**:
   - PNG images (tiles, backgrounds, UI)
   - WAV audio files (sound effects and music)
   - GIF animations (enemy sprites)

3. **Caching**: [AssetManager](src/assets/AssetManager.h) caches loaded assets in memory during gameplay

## Code Organization

### Constants
- Game mechanics and physics constants
- EGA palette definition
- Level and enemy enumeration

### Graphics
- SDL window and renderer setup
- Palette initialization (16 EGA colors)
- Tile and sprite rendering (placeholders for full implementation)

### Audio
- SDL audio device initialization
- Sound playback queue with priority system
- PC speaker square wave synthesis
- Frequency divider conversion (1193182 Hz timer frequency)

### Input
- SDL keyboard event handling
- Configurable key bindings
- Support for KEYS.DEF configuration file
- Input state tracking (current and previous frame)

### Asset Management
- Filesystem path resolution relative to executable
- PNG texture loading (via SDL2_image)
- WAV audio loading (via SDL2)
- Resource caching with STL containers

## Original Assembly Reference

The original DOS assembly code is available in [disassembly/R3sw1989.asm](disassembly/R3sw1989.asm) with extensive comments documenting:
- Game loop structure and fixed-timestep timing
- Graphics rendering using EGA planar format
- Sound system via PC speaker (INT 8 and INT 3 handlers)
- Keyboard input via INT 9 handler
- Data structure layouts and field meanings
- Physics calculations and collision detection
- Enemy AI behaviors

## Building for Different Platforms

### macOS
```bash
# Install SDL2 via Homebrew
brew install sdl2 sdl2_image
cd build
cmake ..
make
```

### Linux (Ubuntu/Debian)
```bash
sudo apt-get install libsdl2-dev libsdl2-image-dev
cd build
cmake ..
make
```

### Windows (MSVC)
```bash
# Install SDL2 development libraries
# Then build with Visual Studio:
cmake -G "Visual Studio 16 2019" ..
cmake --build . --config Release
```

## Notes

- The executable expects assets in `./data/` relative to its working directory
- The game runs at the original 9.1 Hz tick rate, matching DOS gameplay timing
- Graphics are rendered at the original 320×200 resolution, scaled 2× by default for visibility
- All game logic maintains the original fixed-point math and physics calculations
- SDL_WINDOW_ALLOW_HIGHDPI enables proper scaling on high-DPI displays (Retina, 4K, etc.)
