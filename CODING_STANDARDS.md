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
├── docs/                       # Project documentation
│   ├── MODERNIZATION_PLAN.md   # Project roadmap
│   └── ...
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
- [MODERNIZATION_PLAN.md](docs/MODERNIZATION_PLAN.md) - Project roadmap
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
