# Coding Standards for Captain Comic Modernization (C++17)

## Overview

This document defines the C++17 coding conventions for the Captain Comic modernization project. The project recreates the classic 1988 DOS game for modern systems using SDL2 graphics and cross-platform C++17 code.

The goal is readable, maintainable C++ code that is faithful to the original game behavior while leveraging modern language features and best practices.

## General Principles

1. **Clarity over cleverness** - Prefer explicit, readable code
2. **Modern C++17** - Use standard library containers and algorithms
3. **RAII patterns** - Resources tied to object lifetimes
4. **Type safety** - Leverage the type system to prevent bugs
5. **Cross-platform** - No platform-specific code without abstraction

## File Organization

### Project Structure

```
comic-modernization/
├── CMakeLists.txt              # Build configuration
├── MODERNIZATION_PLAN.md       # Project roadmap
├── README.md                   # Build and usage instructions
├── CODING_STANDARDS.md         # This file
├── include/                    # Header files (.h)
│   ├── physics.h
│   ├── graphics.h
│   └── ...
├── src/                        # Implementation files (.cpp)
│   ├── main.cpp
│   ├── physics.cpp
│   ├── graphics.cpp
│   └── ...
├── tests/                      # Test files
│   └── test_main.cpp
├── assets/                     # Game assets (PNG, etc.)
└── build/                      # Build artifacts (generated)
```

### Header Files (.h)

- **Use include guards** (not `#pragma once` for portability):
  ```cpp
  #ifndef GRAPHICS_H
  #define GRAPHICS_H
  // ... content ...
  #endif // GRAPHICS_H
  ```

- **Include order:**
  1. Corresponding .cpp header (for .cpp files only)
  2. System/external headers: `<SDL2/SDL.h>`, `<cstdint>`, `<iostream>`
  3. STL headers: `<vector>`, `<map>`, `<string>`
  4. Local headers: `"physics.h"`, `"graphics.h"`

- **Forward declarations** reduce compilation dependencies:
  ```cpp
  class GraphicsSystem;  // Forward declare instead of including
  ```

- **Document public APIs** with brief descriptions

Example:
```cpp
#ifndef PHYSICS_H
#define PHYSICS_H

#include <cstdint>

// Constants
constexpr int GRAVITY = 5;
constexpr int TERMINAL_VELOCITY = 40;

// Public functions
void apply_gravity(int8_t& velocity);
bool check_collision(int x, int y);

#endif // PHYSICS_H
```

### Source Files (.cpp)

- **Start with brief file header** explaining purpose:
  ```cpp
  // physics.cpp - Player movement, gravity, and collision detection
  ```

- **Include your header first**, then dependencies:
  ```cpp
  #include "../include/physics.h"
  
  #include <SDL2/SDL.h>
  #include <iostream>
  ```

- **Group related functions** with section comments:
  ```cpp
  // ===== Gravity and Velocity =====
  void apply_gravity(int8_t& velocity) { /* ... */ }
  
  // ===== Collision Detection =====
  bool check_collision(int x, int y) { /* ... */ }
  ```

- **Keep functions focused** - single responsibility principle

## Naming Conventions

### Variables

- **Local variables & parameters**: `snake_case`
  ```cpp
  int tile_x = 10;
  uint8_t current_tile_id = level_map[index];
  ```

- **Member variables (class/struct)**: `snake_case`
  ```cpp
  class Player {
  private:
      int position_x;
      uint8_t facing;  // 0 = left, 1 = right
  };
  ```

- **Global variables**: `snake_case` (minimize usage, document in header):
  ```cpp
  extern int comic_x;  // Player X position (game units)
  extern uint8_t key_state_jump;
  ```

- **Constants**: `UPPER_SNAKE_CASE` using `constexpr`:
  ```cpp
  constexpr int RENDER_SCALE = 16;
  constexpr int GRAVITY = 5;
  constexpr int TILE_SIZE = 16;
  ```

### Functions

- **Free functions/methods**: `snake_case`
  ```cpp
  void apply_gravity(int8_t& velocity);
  int load_tileset(const std::string& level_name);
  ```

- **Private/static helper functions**: Prefix with underscores or mark `static`:
  ```cpp
  static void calculate_frame_duration(Animation& anim);
  static TextureInfo load_png(const std::string& filepath);
  ```

### Classes & Structs

- **Types**: `PascalCase`
  ```cpp
  class GraphicsSystem { /* ... */ };
  struct AnimationFrame { /* ... */ };
  struct Tileset { /* ... */ };
  ```

- **Enum types**: `PascalCase`
  ```cpp
  enum class PlayerState {
      Idle,
      Running,
      Jumping,
      Falling
  };
  ```

### Template Parameters

- **Type parameters**: `PascalCase`
  ```cpp
  template<typename Container>
  void process_items(Container& items);
  ```

## Code Formatting

### Indentation

- **4 spaces** per level (no tabs)
- Use editor settings to enforce this automatically

### Braces and Line Breaks

- **Opening brace on same line** (K&R style):
  ```cpp
  void function() {
      // ...
  }
  
  if (condition) {
      // ...
  } else {
      // ...
  }
  ```

- **Always use braces**, even for single statements:
  ```cpp
  // Good
  if (error) {
      return false;
  }
  
  // Avoid
  if (error)
      return false;
  ```

- **Brace placement for classes**:
  ```cpp
  class GraphicsSystem {
  public:
      bool initialize();
      
  private:
      SDL_Renderer* renderer;
  };
  ```

### Line Length

- **Prefer 100 characters max** for readability on modern screens
- Break long lines at logical points:
  ```cpp
  // Good - break at appropriate point
  bool load_sprite(const std::string& sprite_name, 
                   const std::string& direction);
  
  // Avoid - single very long line
  bool load_sprite(const std::string& sprite_name, const std::string& direction, const std::string& additional_param);
  ```

### Spacing

- **After keywords**: `if (`, `while (`, `for (`, `switch (`
- **No space for function calls**: `function(arg)`
- **Space around binary operators**: `a + b`, `x == y`, `ptr = nullptr`
- **No space for unary operators**: `!flag`, `~mask`, `*ptr`, `&reference`
- **Space after commas**: `func(a, b, c)`

## Comments

### File Headers

```cpp
// physics.cpp - Player movement, gravity, and collision
//
// Implements gravity simulation, jumping mechanics, and tile-based
// collision detection for the player character.
```

### Function Comments

```cpp
// Apply gravity to player velocity, with clamping to terminal velocity.
// 
// Parameters:
//   velocity - Current velocity (modified in place)
//
// Notes:
//   Terminal velocity prevents excessive fall speed.
//   See GRAVITY and TERMINAL_VELOCITY constants.
void apply_gravity(int8_t& velocity) {
    // Implementation
}
```

### Inline Comments

- **Explain "why" not "what"** - the code shows what
- **Use `//` for single-line comments**
- **Use for non-obvious logic** or decisions

```cpp
// Clamp velocity to terminal velocity to match original behavior
if (velocity > TERMINAL_VELOCITY) {
    velocity = TERMINAL_VELOCITY;
}

// Update frame timing with cumulative duration tracking
uint32_t elapsed = current_time - anim.frame_start_time;
```

### TODO Comments

```cpp
// TODO: Optimize tile loading with parallel asset loading
// TODO: Add support for more than 64 tiles per tileset
```

## Modern C++17 Practices

### Type Safety

- **Use fixed-width integers**:
  ```cpp
  #include <cstdint>
  
  uint8_t tile_id = 0x3F;
  int8_t velocity = 5;
  uint32_t timestamp = SDL_GetTicks();
  ```

- **Use `nullptr` instead of `NULL`**:
  ```cpp
  Sprite* sprite = nullptr;
  if (sprite == nullptr) { /* ... */ }
  ```

- **Use `constexpr` for compile-time constants** (not `#define`):
  ```cpp
  constexpr int TILE_SIZE = 16;
  constexpr int RENDER_SCALE = 16;
  ```

- **Avoid implicit conversions** - use explicit casts:
  ```cpp
  int frame_index = static_cast<int>(i);
  uint8_t clamped = static_cast<uint8_t>(std::min(value, 255));
  ```

### Containers and Algorithms

- **Use STL containers** instead of C arrays:
  ```cpp
  // Good - bounds checking, clear semantics
  std::vector<uint8_t> tile_map;
  std::map<std::string, Sprite> sprites;
  
  // Avoid - C-style arrays
  uint8_t tile_map[256];
  ```

- **Use range-based for loops**:
  ```cpp
  // Good - modern, clear
  for (const auto& tile : tileset.tiles) {
      process(tile);
  }
  
  // Avoid - index-based loops (unless you need the index)
  for (int i = 0; i < tiles.size(); i++) {
      process(tiles[i]);
  }
  ```

- **Use `auto` for type inference** when type is obvious:
  ```cpp
  auto it = sprites.find("comic_standing");  // iterator type is clear from context
  auto current_frame = get_current_frame(animation);  // return type is documented
  ```

### Resource Management (RAII)

- **Manage resources through constructors/destructors**:
  ```cpp
  class GraphicsSystem {
  public:
      GraphicsSystem(SDL_Renderer* renderer) 
          : renderer(renderer), initialized(false) {}
      
      ~GraphicsSystem() {
          cleanup();  // Automatic cleanup
      }
      
      bool initialize() {
          if (initialized) return true;
          // ... init code ...
          initialized = true;
          return true;
      }
      
  private:
      void cleanup() {
          if (!initialized) return;
          // ... cleanup code ...
          initialized = false;
      }
      
      SDL_Renderer* renderer;
      bool initialized;
  };
  ```

### Error Handling

- **Return `bool` for success/failure** from initialization functions:
  ```cpp
  bool load_tileset(const std::string& level_name) {
      // ... loading code ...
      if (error_condition) {
          std::cerr << "Failed to load tileset: " << level_name << std::endl;
          return false;
      }
      return true;
  }
  ```

- **Check SDL return values**:
  ```cpp
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
      std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
      return 1;
  }
  ```

- **Use exceptions sparingly** - mostly in constructors:
  ```cpp
  class Image {
  public:
      Image(const std::string& path) {
          SDL_Surface* surf = IMG_Load(path.c_str());
          if (!surf) {
              throw std::runtime_error("Failed to load image: " + path);
          }
          texture_data = surf;
      }
  };
  ```

## SDL2-Specific Guidelines

### Initialization Pattern

```cpp
// Initialize SDL subsystems
if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
    return 1;
}

// Create window and renderer
SDL_Window* window = SDL_CreateWindow(...);
SDL_Renderer* renderer = SDL_CreateRenderer(...);

// Always clean up before exit
SDL_DestroyRenderer(renderer);
SDL_DestroyWindow(window);
SDL_Quit();
```

### Texture Management

- **Load textures once, cache in containers**:
  ```cpp
  std::map<std::string, Sprite> sprite_cache;
  
  Sprite* get_or_load_sprite(const std::string& name) {
      auto it = sprite_cache.find(name);
      if (it != sprite_cache.end()) {
          return &it->second;
      }
      // Load new sprite and cache it
  }
  ```

- **Always destroy textures before cleanup**:
  ```cpp
  for (auto& pair : sprite_cache) {
      if (pair.second.texture.texture) {
          SDL_DestroyTexture(pair.second.texture.texture);
      }
  }
  ```

### Rendering Pipeline

```cpp
// Each frame:
1. SDL_RenderClear(renderer);        // Clear screen
2. render_all_game_objects();         // Render sprites/tiles
3. SDL_RenderPresent(renderer);       // Display
4. SDL_Delay(16);                     // ~60 FPS
```

## Testing

### Test Framework

- **Use simple assertions** from `<cassert>`:
  ```cpp
  #include <cassert>
  
  int main() {
      uint8_t tile = 0x3F;
      assert(tile <= 0x3F);
      
      int8_t velocity = 0;
      apply_gravity(velocity);
      assert(velocity == GRAVITY);
      
      std::cout << "All tests passed!" << std::endl;
      return 0;
  }
  ```

- **Register tests in CMakeLists.txt**:
  ```cmake
  enable_testing()
  
  add_executable(comic_tests tests/test_main.cpp)
  target_link_libraries(comic_tests ${SDL2_LIBRARIES} PkgConfig::SDL2_IMAGE)
  
  add_test(NAME comic_tests_all COMMAND comic_tests)
  add_test(NAME comic_tests_jump_height COMMAND comic_tests --filter jump_height)
  ```

- **Run with CTest**:
  ```bash
  cd build && ctest --output-on-failure
  ```

- **Run a single test**:
  ```bash
  cd build && ctest -R comic_tests_jump_height --output-on-failure
  ```

## Common Pitfalls to Avoid

❌ **Don't use C-style casts** - Use `static_cast<>`, `reinterpret_cast<>`, `const_cast<>`
  ```cpp
  // Avoid
  int* ptr = (int*)void_ptr;
  
  // Good
  int* ptr = static_cast<int*>(void_ptr);
  ```

❌ **Don't leak SDL resources** - Always call destroy/cleanup functions
  ```cpp
  // Bad - memory leak
  SDL_Renderer* r = SDL_CreateRenderer(window, -1, 0);
  // ... no cleanup ...
  
  // Good - cleanup on exit
  SDL_DestroyRenderer(renderer);
  ```

❌ **Don't ignore return values** - Always check for errors
  ```cpp
  // Bad
  load_tileset("forest");
  
  // Good
  if (!load_tileset("forest")) {
      std::cerr << "Failed to load forest tileset" << std::endl;
      return false;
  }
  ```

❌ **Don't use `#define` for constants** - Use `constexpr`
  ```cpp
  // Avoid
  #define TILE_SIZE 16
  
  // Good
  constexpr int TILE_SIZE = 16;
  ```

❌ **Don't use `NULL`** - Use `nullptr`

❌ **Don't use C-style arrays** - Use `std::vector` or `std::array`

❌ **Don't forget to initialize members** - Use member initializers in constructors

❌ **Don't mix concerns** - Keep classes focused and cohesive

## Build and Compilation

### CMake Configuration

```bash
# Configure with Debug
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Configure with Release
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Run tests
cd build && ctest --output-on-failure
```

### Compiler Standards

- **Minimum C++17** - required for modern features
- **Enable all warnings** - typically `-Wall -Wextra -Wpedantic`
- **Fix all warnings** - warnings are future bugs

## Reference Materials

**Primary Reference:**
- [jsandas/comic-c](https://github.com/jsandas/comic-c) - C implementation with complete game logic
  - Game physics and collision algorithms
  - Level data and tile structures
  - Enemy behaviors and state machines

**Documentation:**
- [MODERNIZATION_PLAN.md](MODERNIZATION_PLAN.md) - Project roadmap
- [README.md](README.md) - Build instructions
- [AGENT_INSTRUCTIONS.md](agent/AGENT_INSTRUCTIONS.md) - AI agent guidelines

**External References:**
- [C++ Reference](https://en.cppreference.com/) - Standard library documentation
- [SDL2 Documentation](https://wiki.libsdl.org/) - Graphics and input APIs
- [CMake Documentation](https://cmake.org/documentation/) - Build system

## Summary

Key takeaways:
- Use **modern C++17** features (`constexpr`, containers, range-based loops)
- Follow **RAII patterns** for safe resource management
- Prioritize **clarity and type safety** over cleverness
- **Test thoroughly** with unit tests
- **Document APIs** with clear comments
- Keep **functions focused** and maintainable

## General Principles

1. **Clarity over cleverness** - Prefer explicit, readable code
2. **Bug-for-bug compatibility** - Preserve original behavior, even bugs
3. **Document DOS-specific assumptions** - We're in 16-bit real-mode
4. **Test everything** - No change is too small to test

## File Organization

### Entry Point and Initialization

As of 2026-01-03, `src/game_main.c` is the primary entry point containing:
- `main()` - Application entry point (replaces assembly initialization)
- Initialization functions: `disable_pc_speaker()`, `calibrate_joystick()`, `check_ega_support()`, etc.
- Menu/UI functions: `display_startup_notice()`, `setup_keyboard_interactive()`, etc.
- Cleanup functions: `terminate_program()`, `display_ega_error()`
- Game stubs: `game_entry()`, `load_new_level()`, `game_loop_iteration()` skeleton

### Header Files

- Use include guards: `#ifndef FILENAME_H` / `#define FILENAME_H` / `#endif`
- Order: system includes → local includes → types → constants → declarations
- **Note**: Assembly-specific pragmas (e.g., `#pragma aux`) are legacy and should be avoided unless absolutely necessary to interface with the original assembly; prefer pure C solutions
- Most new code will be implemented in C without assembly dependencies

Example (Pure C):
```c
#ifndef PHYSICS_H
#define PHYSICS_H

#include <stdint.h>

int check_collision(uint8_t x, uint8_t y);
void handle_player_physics(void);

#endif /* PHYSICS_H */
```

### Source Files

- Start with header comment explaining file purpose
- Include system headers first (`<dos.h>`, `<conio.h>`, etc.)
- Then include own header, then local dependencies
- Group related functions together with section comments
- Keep functions focused and cohesive

Example (Pure C):
```c
/*
 * collision.c - Collision detection routines
 * 
 * Implements tile-based collision detection for player and enemies.
 * All logic in C, no assembly dependencies.
 */

#include <stdint.h>
#include "collision.h"
#include "globals.h"

/* Helper functions */
static uint16_t calculate_tile_offset(uint8_t x, uint8_t y);
static int is_tile_solid(uint16_t offset);

/* Public functions */
int check_collision(uint8_t x, uint8_t y);
```

## Naming Conventions

### Variables

- **Global variables**: Match assembly names exactly (e.g., `comic_x`, `comic_y`)
- **Local variables**: Descriptive snake_case (e.g., `tile_offset`, `is_solid`)
- **Constants**: SCREAMING_SNAKE_CASE (e.g., `MAP_WIDTH_TILES`)
- **Function parameters**: Descriptive snake_case

### Functions

- **Legacy assembly labels** are documented in the reference disassembly (e.g., `load_new_level`); prefer descriptive C function names and implement behavior in C (e.g., `load_new_level`)
- **C functions**: Descriptive snake_case (e.g., `calculate_tile_offset`, `is_tile_solid`)
- **Static helpers**: Prefix with `static` keyword

### Types

- Use explicit integer types: `uint8_t`, `uint16_t`, `int8_t`, `int16_t`
- **Never use `int`** - size is ambiguous in 16-bit DOS (it's 16-bit)
- Struct types: `typedef struct { ... } name_t;` with `_t` suffix

Example:
```c
typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t facing;
} player_state_t;
```

## Code Formatting

### Indentation

- **4 spaces** per level (no tabs)
- Align related elements vertically when it improves readability

### Braces

- **K&R style** for functions: opening brace on same line
- Always use braces for control structures, even single statements

```c
void function_name(uint8_t param)
{
    if (condition) {
        do_something();
    } else {
        do_something_else();
    }
    
    for (i = 0; i < count; i++) {
        process(i);
    }
}
```

### Line Length

- Prefer **80 characters** max for readability
- Break long lines at logical points (after operators, commas)

### Spacing

- Space after keywords: `if (`, `while (`, `for (`
- No space for function calls: `function(arg)`
- Space around binary operators: `a + b`, `x == y`
- No space around unary operators: `!flag`, `~mask`, `*ptr`

## Comments

### File Headers

```c
/*
 * filename.c - Brief description
 * 
 * Longer explanation if needed.
 * Reference to original assembly: R5sw1991.asm lines XXXX-YYYY
 */
```

### Function Comments

```c
/*
 * Brief one-line summary
 * 
 * Longer explanation of what the function does, edge cases,
 * and any DOS-specific assumptions.
 * 
 * Parameters:
 *   x - Description of x
 *   y - Description of y
 * 
 * Returns:
 *   0 on success, -1 on error
 * 
 * Assembly reference: R5sw1991.asm:5500 (check_vertical_enemy_map_collision)
 */
int check_collision(uint8_t x, uint8_t y)
{
    /* Implementation */
}
```

### Inline Comments

- Use `/* */` for block comments
- Use `//` for end-of-line comments (Open Watcom C supports C++ style)
- Explain **why**, not **what** (code shows what)
- Document deviations from expected behavior or bug-for-bug compatibility

```c
/* Bug-for-bug compatibility: Original assembly incorrectly uses SUB instead of ADD
 * See R5sw1991.asm:5632 - joystick_x_low calculation bug */
threshold = (zero_value - extreme_value) / 2;  // Wrong but intentional!
```

## DOS-Specific Considerations

### Memory Model

- We use **large model** (`-ml` flag)
- Code and data in separate segments
- Far pointers for code, far pointers for data
- DS register should point to the data segment as required by the large model; verify this when working with low-level code
- Avoid modifying DS unless absolutely necessary and document any changes carefully

### No Standard Library Assumptions

- **Avoid `malloc`/`free`** - prefer static allocation or other deterministic storage strategies (do not rely on assembly-managed memory)
- **Avoid `printf`** - use direct video memory manipulation or C wrappers; avoid using assembly functions for I/O
- **File I/O** - can use `fopen`/`fread`/`fclose` (Open Watcom provides DOS versions)
- **String functions** - can use `strcmp`, `strcpy`, etc. (they work in DOS)

### Video Memory Access

- Video memory at segment `0xa000`
- Prefer pure C implementations for direct video access; inline assembly is permitted only when absolutely necessary and must be reviewed
- Never cache video memory pointers across function calls

Example:
```c
/* Write directly to video memory using C helper */
void write_pixel(uint16_t x, uint16_t y, uint8_t color)
{
    uint16_t offset = (y * 320 + x) / 8;
    /* Implemented in C: prefer C-based helpers over assembly */
    write_video_byte(offset, color);  // Implemented in C
}
```

### Inline Assembly (rare)

- Inline assembly is strongly discouraged; prefer pure C implementations. Inline assembly is permitted only when absolutely necessary and must be carefully reviewed and documented.

## Type Definitions

### Standard Types

```c
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed long int32_t;
```

### Game-Specific Types

```c
/* Tile coordinate (game units, not pixels) */
typedef struct {
    uint8_t x;
    uint8_t y;
} tile_pos_t;

/* Velocity in fixed-point (1/8 game unit per tick) */
typedef int8_t velocity_t;
```

## Error Handling

- **Return codes**: 0 = success, -1 = error (Unix convention)
- **Booleans**: `0 = false`, `non-zero = true` (C89 convention)
- **NULL checks**: Always validate pointers before dereferencing

```c
int load_file(const char* filename, void* buffer)
{
    if (!filename || !buffer) {
        return -1;  // Invalid parameters
    }
    
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        return -1;  // File not found
    }
    
    /* ... */
    
    fclose(fp);
    return 0;  // Success
}
```

## Assembly Reference

The original assembly is preserved in `reference/disassembly/` as an implementation reference only. The preferred approach is to reimplement behavior in C. If interfacing with the original assembly is unavoidable, document the justification, follow calling conventions, and write thorough tests; avoid adding new assembly where possible.
## Testing Requirements

### Function Testing

- Every converted function must have a test scenario
- Document test in `tests/scenarios/`
- Create save state if needed
- Verify with DOSBox-X

### Documentation

- Mark conversions with assembly line references:
```c
/* Converted from R5sw1991.asm:5500-5550 (check_vertical_enemy_map_collision) */
```

- Document any behavioral changes or uncertainties:
```c
/* TODO: Verify this matches assembly behavior at R5sw1991.asm:5532
 * The original code seems to have an off-by-one bug here */
```

## Example: Complete Function

```c
/*
 * check_vertical_collision - Check if player would collide vertically
 * 
 * Checks the map tile at player's position to determine if vertical
 * movement would result in a collision with a solid tile.
 * 
 * Parameters:
 *   x - X coordinate in game units
 *   y - Y coordinate in game units
 * 
 * Returns:
 *   1 if collision (solid tile), 0 if passable
 * 
 * Assembly reference: R5sw1991.asm:5500-5550 (check_vertical_enemy_map_collision)
 */
int check_vertical_collision(uint8_t x, uint8_t y)
{
    uint16_t tile_offset;
    uint8_t tile_id;
    
    /* Calculate tile offset from coordinates */
    /* Each tile is 16x16 pixels = 2 game units */
    tile_offset = (y / 2) * MAP_WIDTH_TILES + (x / 2);
    
    /* Safety check: ensure offset is within map bounds */
    if (tile_offset >= MAP_WIDTH_TILES * MAP_HEIGHT_TILES) {
        return 1;  // Out of bounds = solid
    }
    
    /* Read tile ID from map */
    tile_id = current_tiles_ptr[tile_offset];
    
    /* Compare to passable threshold */
    /* Tiles with ID > tileset_last_passable are solid */
    if (tile_id > tileset_last_passable) {
        return 1;  // Solid tile
    }
    
    return 0;  // Passable tile
}
```

## When In Doubt

1. Check the original assembly for exact behavior
2. Prefer explicit over implicit
3. Document unusual patterns
4. Test thoroughly
5. Ask for review if uncertain

## Tools and Validation

- **Compiler**: `wcc -ml -s -0` (large model, no stack checks, 8086 target)
- **Warnings**: Enable all warnings, fix them
- **Static analysis**: Use Open Watcom's built-in checks
- **Testing**: DOSBox-X with save states for validation
