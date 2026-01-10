# Project Structure

This document describes the organization of the Captain Comic C++ SDL2 port.

## Directory Layout

```
.
├── README.md                      # Project overview
├── README_BUILD.md                # Build instructions and architecture
├── CMakeLists.txt                 # Top-level CMake configuration
├── .gitignore                     # Git ignore patterns
│
├── src/                           # C++ SOURCE CODE
│   ├── CMakeLists.txt             # Source build configuration
│   ├── main.cpp                   # Game entry point and main loop
│   ├── assets/
│   │   ├── AssetManager.h/cpp     # Resource loading and caching
│   ├── graphics/
│   │   ├── Graphics.h/cpp         # SDL2 rendering system
│   ├── audio/
│   │   ├── Audio.h/cpp            # Sound system (PC speaker synthesis)
│   ├── input/
│   │   ├── Input.h/cpp            # Keyboard input handling
│   └── game/
│       ├── Constants.h            # Game constants (from assembly)
│       ├── GameState.h/cpp        # Game state structures
│
├── reference/                        # GAME ASSETS AND EXTRACTION
│   ├── assets/                 # GENERATED ASSETS (gitignored)
│   │   ├── *.png (extracted graphics)
│   │   ├── *.wav (extracted sounds)
│   │   └── *.gif (extracted animations)
│   │   └── zip/ (archived downloads)
│   ├── original/                  # ORIGINAL GAME FILES
│   │   ├── COMIC.EXE
│   │   ├── *.EGA (graphics)
│   │   ├── *.PT (maps)
│   │   ├── *.SHP (sprites)
│   │   ├── *.TT2 (tilesets)
│   │   └── ... (other files)
│   ├── disassembly/
│   │   ├── R5sw1991.asm           # Original DOS assembly code
│   │   ├── R3_levels.asm
│   │   ├── gen_levels.go
│   │   ├── README
│   │   └── djlink/                # DOS linker source code
│   │       ├── *.cc, *.h          # C++ source
│   │       ├── djlink (executable)
│   │       └── makefile
│   └── docs/                      # Documentation (future)
│
└── build/                         # BUILD ARTIFACTS (gitignored)
    ├── comic-c                    # Compiled executable
    ├── data/                      # Copied extracted assets
    ├── CMakeFiles/
    ├── Makefile
    └── src/
```

## Key Points

### Assets Workflow
1. **Original files**: `reference/assets/original/R5sw1991/` contains the original DOS game files
2. **Extraction**: `reference/assets/extraction/Makefile` orchestrates asset extraction via Go tools
3. **Generated assets**: `reference/assets/extracted` contains PNG, WAV, GIF files
4. **Build copy**: CMake copies assets to `build/data/` for the executable

> Tip: The game searches upwards from the current working directory to find `data/`. Either run the executable from `build/` or set `COMIC_DATA` to the full path of the data directory (for example: `export COMIC_DATA=$(pwd)/build/data`).

### Git Tracking
- **Committed**: Source code (`src/`), build configuration, original game files, extraction tools
- **Ignored**: Build artifacts (`build/`), extracted assets (`reference/assets/extracted/`), downloaded files

### Reference Materials
- Original DOS assembly code in `reference/disassembly/`
- DOS linker (djlink) source code in `reference/disassembly/djlink/`
- These are for understanding the original implementation, not used for building the port

### Extraction Tools
Located in `reference/assets/extraction/programs/`:
- `unpack-ega/` - Extracts EGA full-screen graphics to PNG
- `unpack-tt2/` - Extracts tileset graphics to PNG (one per tile)
- `unpack-pt/` - Extracts level maps and renders preview PNG
- `unpack-shp/` - Extracts enemy sprites to animated GIF
- `unpack-game-graphic/` - Extracts in-game graphics from executable
- `extract-sound/` - Extracts PC speaker sounds to WAV format

### Build Process
1. `cmake ..` - Reads CMakeLists.txt
2. `make` - Executes:
   - `build_assets` target copies `reference/assets` to the build data directory
   - Asset extraction runs Go tools
   - Generated assets copied to `build/data/`
   - C++ source compiled and linked
   - Final executable at `build/comic-c`

## Rationale

**Before Reorganization**:
- `deriv/` mixed extraction rules with generated assets
- `disassembly/` and `programs/` scattered across root
- Hard to distinguish reference materials from active code

**After Reorganization**:
- Clear separation of concerns
- Easy to understand asset pipeline
- Reference materials clearly separated
- Simpler .gitignore with predictable paths
- Scalable for supporting multiple game versions
