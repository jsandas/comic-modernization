#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <SDL2/SDL.h>
#include <cstdint>
#include <string>
#include <map>
#include <vector>

// Forward declarations
struct Texture;
struct Tileset;
struct SpriteSheet;

// Tile size in pixels (16x16 in original EGA)
constexpr int TILE_SIZE = 16;

// Text rendering structure
struct TextureInfo {
    SDL_Texture* texture;
    int width;
    int height;
};

// Tileset: contains multiple tile graphics (16x16 pixels each)
struct Tileset {
    std::map<uint8_t, TextureInfo> tiles;  // tile_id -> TextureInfo
    void cleanup();
};

// Sprite structure for player and actors
struct Sprite {
    TextureInfo texture;
    int width;
    int height;
};

// Sprite animation frame
struct AnimationFrame {
    Sprite sprite;
    int duration_ms;  // How long to display this frame
};

// Animation sequence
struct Animation {
    std::vector<AnimationFrame> frames;
    int current_frame;
    uint32_t frame_start_time;
    bool looping;
    
    Animation() : current_frame(0), frame_start_time(0), looping(true) {}
};

// Graphics system
class GraphicsSystem {
public:
    GraphicsSystem(SDL_Renderer* renderer);
    ~GraphicsSystem();
    
    // Initialize - must be called after construction
    bool initialize();
    
    // Tileset loading/management
    bool load_tileset(const std::string& level_name);
    Tileset* get_tileset(const std::string& level_name);
    
    // Sprite loading
    bool load_sprite(const std::string& sprite_name, const std::string& direction);
    Sprite* get_sprite(const std::string& sprite_name, const std::string& direction);
    
    // Animation management
    Animation create_animation(const std::vector<std::string>& sprite_names, const std::string& direction, int frame_duration_ms, bool looping = true);
    AnimationFrame* get_current_frame(Animation& anim);
    void update_animation(Animation& anim, uint32_t current_time);
    
    // Rendering
    void render_tile(int screen_x, int screen_y, Tileset* tileset, uint8_t tile_id, int scale);
    void render_sprite(int screen_x, int screen_y, const Sprite& sprite, bool flip_h = false);
    void render_sprite_centered(int screen_x, int screen_y, const Sprite& sprite, bool flip_h = false);
    
    // Cleanup
    void cleanup();
    
private:
    SDL_Renderer* renderer;
    std::map<std::string, Tileset> tilesets;
    std::map<std::string, Sprite> sprites;
    
    // Helper functions
    TextureInfo load_png(const std::string& filepath);
    std::string get_asset_path(const std::string& filename);
};

// Global graphics system
extern GraphicsSystem* g_graphics;

#endif // GRAPHICS_H
