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
├── assets/                        # GAME ASSETS AND EXTRACTION
│   ├── extraction/
│   │   ├── Makefile               # Asset extraction orchestration
│   │   ├── tools/
│   │   │   ├── fetch_orig_version.sh
│   │   │   └── rebuild_r1sw1988.sh
│   │   └── programs/              # Extraction tools (Go programs)
│   │       ├── unpack-ega/        # EGA graphics extraction
│   │       ├── unpack-pt/         # Map data extraction
│   │       ├── unpack-shp/        # Enemy sprite extraction
│   │       ├── unpack-tt2/        # Tileset extraction
│   │       ├── unpack-game-graphic/
│   │       ├── extract-sound/
│   │       └── ... (other tools)
│   │
│   ├── original/                  # ORIGINAL GAME FILES
│   │   └── R3sw1989/
│   │       ├── COMIC.EXE
│   │       ├── *.EGA (graphics)
│   │       ├── *.PT (maps)
│   │       ├── *.SHP (sprites)
│   │       ├── *.TT2 (tilesets)
│   │       └── ... (other files)
│   │
│   ├── extracted/                 # GENERATED ASSETS (gitignored)
│   │   ├── R3sw1989/
│   │   │   ├── *.png (extracted graphics)
│   │   │   ├── *.wav (extracted sounds)
│   │   │   └── *.gif (extracted animations)
│   │   └── zip/ (archived downloads)
│   │
│   └── downloaded/                # DOWNLOADED FILES (gitignored)
│       └── [game archives from archive.org]
│
├── reference/                     # REFERENCE MATERIALS
│   ├── disassembly/
│   │   ├── R3sw1989.asm           # Original DOS assembly code
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
1. **Original files**: `assets/original/R3sw1989/` contains the original DOS game files
2. **Extraction**: `assets/extraction/Makefile` orchestrates asset extraction via Go tools
3. **Generated assets**: `assets/extracted/R3sw1989/` contains PNG, WAV, GIF files
4. **Build copy**: CMake copies assets to `build/data/` for the executable

### Git Tracking
- **Committed**: Source code (`src/`), build configuration, original game files, extraction tools
- **Ignored**: Build artifacts (`build/`), extracted assets (`assets/extracted/`), downloaded files

### Reference Materials
- Original DOS assembly code in `reference/disassembly/`
- DOS linker (djlink) source code in `reference/disassembly/djlink/`
- These are for understanding the original implementation, not used for building the port

### Extraction Tools
Located in `assets/extraction/programs/`:
- `unpack-ega/` - Extracts EGA full-screen graphics to PNG
- `unpack-tt2/` - Extracts tileset graphics to PNG (one per tile)
- `unpack-pt/` - Extracts level maps and renders preview PNG
- `unpack-shp/` - Extracts enemy sprites to animated GIF
- `unpack-game-graphic/` - Extracts in-game graphics from executable
- `extract-sound/` - Extracts PC speaker sounds to WAV format

### Build Process
1. `cmake ..` - Reads CMakeLists.txt
2. `make` - Executes:
   - `build_assets` target runs `make -C assets/extraction`
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
