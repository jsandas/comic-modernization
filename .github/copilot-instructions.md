# Agent Instructions for Captain Comic Modernization

## Project Overview

This project modernizes The Adventures of Captain Comic (1988 DOS game) for modern systems using C++17 and SDL2. The goal is to recreate the game faithfully while leveraging modern development practices and cross-platform portability.

**Reference:** [jsandas/comic-c](https://github.com/jsandas/comic-c) - C refactor with well-structured game logic
**Target Platforms:** Windows, macOS, Linux
**Technology:** SDL2, SDL2_image, C++17, CMake 3.16+

## Key Principles

### 1. Modern C++ Standards (C++17)

**Use Modern C++ Features:**
- `constexpr` for compile-time constants
- `nullptr` instead of NULL
- Range-based for loops: `for (const auto& item : container)`
- STL containers: `std::vector`, `std::map`, `std::string`
- Smart pointers when ownership is complex (prefer RAII patterns)
- `auto` for type inference when type is obvious
- Structured bindings: `auto [key, value] = pair;`

**Type Safety:**
- Use fixed-width integer types: `uint8_t`, `uint16_t`, `int8_t`, `int32_t`
- Use `size_t` for sizes and indices
- Avoid implicit conversions; use `static_cast<>` when necessary
- Prefer `enum class` over plain `enum`

**Example:**
```cpp
// Good - modern C++17
constexpr int TILE_SIZE = 16;
std::vector<uint8_t> tiles;
for (const auto& tile : tiles) {
    if (tile > 0x3E) { /* ... */ }
}

// Avoid - C-style
#define TILE_SIZE 16
uint8_t tiles[100];
for (int i = 0; i < 100; i++) {
    if (tiles[i] > 0x3E) { /* ... */ }
}
```

### 2. Resource Management (RAII)

**SDL2 Resource Lifecycle:**
- Initialize resources in constructors or dedicated init functions
- Clean up in destructors (RAII pattern)
- Make cleanup idempotent (safe to call multiple times)
- Track initialization state with boolean flags

**Pattern:**
```cpp
class GraphicsSystem {
private:
    SDL_Renderer* renderer;
    bool initialized;
    
public:
    GraphicsSystem(SDL_Renderer* r) : renderer(r), initialized(false) {}
    
    ~GraphicsSystem() {
        cleanup();  // Safe to call multiple times
    }
    
    bool initialize() {
        if (initialized) return true;
        // ... initialization code ...
        initialized = true;
        return true;
    }
    
private:
    void cleanup() {
        if (!initialized) return;
        // ... cleanup code ...
        initialized = false;
    }
};
```

### 3. Error Handling

**Validate and Report Errors:**
- Check return values from SDL functions
- Log errors to `std::cerr` with context
- Return `bool` for success/failure from functions
- Exit early on critical failures (missing assets, init failures)
- Provide clear error messages identifying what failed

**Example:**
```cpp
bool load_tileset(const std::string& level_name) {
    if (!graphics->load_tileset(level_name)) {
        std::cerr << "Failed to load tileset: " << level_name << std::endl;
        return false;
    }
    return true;
}
```

### 4. Code Organization

**File Structure:**
```
comic-modernization/
├── .github/                # GitHub-specific files
│   ├── copilot-instructions.md  # Instructions for GitHub Copilot
│   ├── dependabot.yml          # Dependabot configuration
│   └── workflows/              # GitHub Actions workflows
├── CMakeLists.txt          # Build configuration
├── MODERNIZATION_PLAN.md   # Project roadmap
├── README.md               # Setup and usage
├── include/                # Header files (.h)
│   ├── physics.h
│   ├── graphics.h
│   └── ...
├── src/                    # Implementation files (.cpp)
│   ├── main.cpp
│   ├── physics.cpp
│   ├── graphics.cpp
│   └── ...
├── tests/                  # Test files
│   └── test_main.cpp
├── assets/                 # Game assets (PNG, etc.)
└── build/                  # Build artifacts (generated)
```

**Header Guards:**
```cpp
#ifndef GRAPHICS_H
#define GRAPHICS_H
// ... content ...
#endif // GRAPHICS_H
```

**Include Order:**
1. Corresponding header (for .cpp files)
2. System headers (`<SDL2/SDL.h>`, `<cstdint>`)
3. STL headers (`<vector>`, `<map>`, `<string>`)
4. Local headers (`"physics.h"`, `"graphics.h"`)

### 5. SDL2 Best Practices

**Initialization Pattern:**
```cpp
// Initialize SDL subsystems
if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
    return 1;
}

// Always clean up
SDL_Quit();  // Call before exit
```

**Texture Management:**
- Load textures once, cache in containers
- Use `SDL_DestroyTexture()` before cleanup
- Check for nullptr after loading
- Track loaded resources to avoid duplicates

**Rendering Pattern:**
1. Clear screen: `SDL_RenderClear(renderer)`
2. Render all sprites/tiles
3. Present: `SDL_RenderPresent(renderer)`
4. Delay for frame rate: `SDL_Delay(16)` for ~60 FPS

### 6. Build System (CMake)

**CMakeLists.txt conventions:**
- Minimum version: `cmake_minimum_required(VERSION 3.16)`
- Use `find_package()` for dependencies
- Use `pkg-config` with `IMPORTED_TARGET` for libraries like SDL2_image
- Set C++ standard: `set(CMAKE_CXX_STANDARD 17)`
- Enable testing: `enable_testing()`

**Building:**
```bash
mkdir build && cd build
cmake ..
cmake --build .
ctest  # Run tests
```

### 7. Testing

**Test Framework:**
- Use simple assertions with `assert()` from `<cassert>`
- Create test executable in `tests/`
- Register tests with `add_test()` in CMakeLists.txt
- Run with `ctest --output-on-failure`

**Test Example:**
```cpp
#include <cassert>
#include "../include/physics.h"

int main() {
    // Test tile collision detection
    init_test_level();
    
    // Get tile at a specific position
    uint8_t tile = get_tile_at(0, 0);
    
    // Test solid tile detection (tile_id > 0x3E is solid)
    assert(is_tile_solid(0x40) == true);
    assert(is_tile_solid(0x3F) == false);
    
    // Verify gravity constant
    static_assert(COMIC_GRAVITY == 5, "Gravity should be 5");
    
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
```

## Implementation Guidelines

### Current Phase: Phase 3 (Rendering System)

See [MODERNIZATION_PLAN.md](../MODERNIZATION_PLAN.md) for complete roadmap.

**Priority:**
1. Implement graphics system (textures, sprites, animations)
2. Implement rendering pipeline (tiles, player sprite)
3. Add animation state machine
4. Test with forest level assets

### Common Patterns

**Animation State Machine:**
```cpp
// Track current animation
Animation* current_animation = nullptr;
Animation* previous_animation = nullptr;

// Update based on state
previous_animation = current_animation;
if (is_jumping) {
    current_animation = &jump_animation;
} else if (is_running) {
    current_animation = &run_animation;
} else {
    current_animation = &idle_animation;
}

// Reset on state change
if (current_animation != previous_animation) {
    current_animation->current_frame = 0;
    current_animation->frame_start_time = SDL_GetTicks();
}
```

**Tileset Loading:**
```cpp
// Load all tiles for a level
for (int i = 0; i < 64; i++) {
    char filename[64];
    std::snprintf(filename, sizeof(filename), "%s.tt2-%02x.png", 
                  level_name.c_str(), i);
    
    if (!load_tile(filename, i)) {
        // Track missing tiles, warn but continue
    }
}
```

### Code Style

**Naming Conventions:**
- Variables: `snake_case` (e.g., `comic_x`, `tile_size`)
- Functions: `snake_case` (e.g., `load_tileset()`, `handle_jump()`)
- Classes/Structs: `PascalCase` (e.g., `GraphicsSystem`, `AnimationFrame`)
- Constants: `UPPER_SNAKE_CASE` (e.g., `RENDER_SCALE`, `GRAVITY`)
- Member variables: `snake_case` (e.g., `current_frame`, `renderer`)

**Formatting:**
- Indentation: 4 spaces (no tabs)
- Braces: Opening brace on same line
- Line length: Prefer under 100 characters
- Spaces around operators: `x = y + z`, not `x=y+z`

**Comments:**
- Use `//` for single-line comments
- Document public APIs with brief descriptions
- Explain "why" not "what" for complex logic
- Mark TODOs with `// TODO: description`

**Example:**
```cpp
// Check if a tile is solid (passable or obstacle)
// Tiles with ID > 0x3E are solid obstacles
bool is_tile_solid(uint8_t tile_id) {
    constexpr uint8_t TILESET_LAST_PASSABLE = 0x3E;
    return tile_id > TILESET_LAST_PASSABLE;
}

// Get tile ID at game coordinates
// Converts game units to tile coordinates and looks up in map
uint8_t get_tile_at(uint8_t x, uint8_t y) {
    // Each tile is 2 game units (16 pixels at 16px/unit)
    uint8_t tile_x = x / 2;
    uint8_t tile_y = y / 2;
    
    if (tile_x >= MAP_WIDTH_TILES || tile_y >= MAP_HEIGHT_TILES) {
        return 0;  // Out of bounds = passable
    }
    
    uint16_t index = tile_y * MAP_WIDTH_TILES + tile_x;
    return current_level_map[index];
}
```

## Common Pitfalls to Avoid

❌ **Don't use C-style casts** - Use `static_cast<>`, `reinterpret_cast<>` instead  
❌ **Don't use raw pointers for ownership** - Use RAII patterns  
❌ **Don't leak SDL resources** - Always call cleanup/destroy functions  
❌ **Don't ignore return values** - Check for errors from SDL and file operations  
❌ **Don't use `#define` for constants** - Use `constexpr` instead  
❌ **Don't use `NULL`** - Use `nullptr`  
❌ **Don't use C arrays** - Use `std::vector` or `std::array`  
❌ **Don't forget to initialize variables** - Always initialize members  

## Reference Materials

**Primary Reference:**
- [jsandas/comic-c](https://github.com/jsandas/comic-c) - C implementation with complete game logic
  - `src/physics.c` - Movement, collision, gravity
  - `src/graphics.c` - Rendering, video management
  - `src/level_data.c` - All 8 game levels
  - `include/globals.h` - Constants and data structures

**Documentation:**
- [MODERNIZATION_PLAN.md](../MODERNIZATION_PLAN.md) - Project roadmap and status
- [README.md](../README.md) - Build instructions and overview
- SDL2 Documentation: https://wiki.libsdl.org/

## When Stuck or Uncertain

1. **Check the C reference** in jsandas/comic-c for algorithm details
2. **Consult SDL2 documentation** for API usage patterns
3. **Test incrementally** - Build and test after small changes
4. **Look at existing code** - Follow established patterns in the codebase
5. **Document uncertainties** - Add TODO comments for unclear behavior
6. **Validate behavior** - Compare with reference implementation
