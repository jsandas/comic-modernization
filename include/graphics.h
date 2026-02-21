#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
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

// Texture rendering structure
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

// Enemy sprite animation data
// Note: Metadata (num_frames, horizontal orientation) is stored in the level's shp_t descriptor.
// This struct only contains the loaded texture data.
struct SpriteAnimationData {
    std::vector<TextureInfo> frames_left;   /* Left-facing animation frames */
    std::vector<TextureInfo> frames_right;  /* Right-facing animation frames */
    std::vector<uint8_t> frame_sequence;    /* Animation order (indexes into frames_*) */
};

// Build animation frame sequence for enemy sprites
std::vector<uint8_t> build_enemy_animation_sequence(
    uint8_t num_distinct_frames,
    uint8_t animation_type
);

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
    
    // Enemy sprite loading
    SpriteAnimationData* load_enemy_sprite(const struct shp_t& sprite_desc);
    
    // Animation management
    Animation create_animation(const std::vector<std::string>& sprite_names, const std::string& direction, int frame_duration_ms, bool looping = true);
    AnimationFrame* get_current_frame(Animation& anim);
    void update_animation(Animation& anim, uint32_t current_time);
    
    // Rendering
    void render_tile(int screen_x, int screen_y, Tileset* tileset, uint8_t tile_id, int scale);
    void render_sprite(int screen_x, int screen_y, const Sprite& sprite, bool flip_h = false);
    void render_sprite_scaled(int screen_x, int screen_y, const Sprite& sprite, int width, int height, bool flip_h = false);
    void render_sprite_centered(int screen_x, int screen_y, const Sprite& sprite, bool flip_h = false);
    void render_sprite_centered_scaled(int screen_x, int screen_y, const Sprite& sprite, int width, int height, bool flip_h = false);
    
    // Text rendering
    void render_text(int screen_x, int screen_y, const std::string& text, SDL_Color color);
    
    // Debug rendering
    void render_debug_overlay();
    
    // Cleanup
    
private:
    void cleanup();
    SDL_Renderer* renderer;
    bool img_inited;
    bool ttf_inited;
    TTF_Font* debug_font;  // Font for debug overlay text
    std::map<std::string, Tileset> tilesets;
    std::map<std::string, Sprite> sprites;
    std::map<std::string, SpriteAnimationData*> enemy_sprites;  // Enemy sprite animation data
    
    // Helper functions
    TextureInfo load_png(const std::string& filepath);
    std::vector<TextureInfo> load_animation_frames(
        const std::string& filepath,
        int expected_frames,
        const std::string& label
    );
    std::string get_asset_path(const std::string& filename);
};

// Global graphics system
extern GraphicsSystem* g_graphics;

#endif // GRAPHICS_H
