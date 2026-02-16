#include "../include/graphics.h"
#include "../include/cheats.h"
#include "../include/physics.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <iomanip>
#include <sstream>

// External game state (defined in main.cpp)
extern int comic_x;
extern int comic_y;
extern int8_t comic_y_vel;
extern int8_t comic_x_momentum;
extern uint8_t comic_facing;
extern uint8_t current_level_number;
extern uint8_t current_stage_number;
extern int camera_x;

// Global graphics system
GraphicsSystem* g_graphics = nullptr;

GraphicsSystem::GraphicsSystem(SDL_Renderer* renderer) : renderer(renderer), img_inited(false), debug_font(nullptr) {}

GraphicsSystem::~GraphicsSystem() {
    cleanup();
}

bool GraphicsSystem::initialize() {
    // Initialize SDL_image
    int flags = IMG_INIT_PNG;
    if ((IMG_Init(flags) & flags) != flags) {
        std::cerr << "SDL_image initialization failed: " << IMG_GetError() << std::endl;
        return false;
    }
    img_inited = true;
    
    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        std::cerr << "SDL_ttf initialization failed: " << TTF_GetError() << std::endl;
        IMG_Quit();
        img_inited = false;
        return false;
    }
    
    // Try to load a monospace font for debug overlay
    // Try multiple possible font paths and names
    std::string font_candidates[] = {
        // macOS system fonts
        "/System/Library/Fonts/Menlo.ttc",
        "/System/Library/Fonts/Courier.ttc",
        "/System/Library/Fonts/SFNSMono.ttf",
        "/Library/Fonts/Menlo.ttc",
        
        // Linux fonts
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        
        // Windows fonts
        "C:\\Windows\\Fonts\\lucidaconsole.ttf",
        "C:\\Windows\\Fonts\\consola.ttf",
        
        // Bundled fonts
        "assets/fonts/monospace.ttf"
    };
    
    const int debug_font_size = 12;
    for (const auto& font_path : font_candidates) {
        debug_font = TTF_OpenFont(font_path.c_str(), debug_font_size);
        if (debug_font != nullptr) {
            break;
        }
    }
    
    if (debug_font == nullptr) {
        std::cerr << "Warning: Could not load debug font, debug overlay will not display coordinates" << std::endl;
        std::cerr << "  Tried: Menlo, Courier, DejaVuSansMono, LiberationMono, and others" << std::endl;
    }
    
    return true;
}

std::string GraphicsSystem::get_asset_path(const std::string& filename) {
    // Try multiple possible locations for assets:
    // 1. ./assets/ (from current working directory)
    // 2. ../assets/ (from build directory)
    // 3. ../../assets/ (from nested build directory)

    std::string paths[] = {
        "assets/" + filename,
        "../assets/" + filename,
        "../../assets/" + filename
    };

    for (const auto& path : paths) {
        std::ifstream f(path);
        if (f.good()) {
            return path;
        }
    }

    return paths[0];
}

TextureInfo GraphicsSystem::load_png(const std::string& filepath) {
    TextureInfo info = {nullptr, 0, 0};
    static std::unordered_set<std::string> logged_load_failures;
    
    // Try multiple possible paths
    std::string possible_paths[] = {
        filepath,
        "../" + filepath,
        "../../" + filepath
    };
    
    SDL_Surface* surface = nullptr;
    
    for (const auto& path : possible_paths) {
        // Check if file exists
        std::ifstream f(path);
        if (f.good()) {
            surface = IMG_Load(path.c_str());
            if (surface) {
                break;
            }
            if (logged_load_failures.insert(path).second) {
                std::cerr << "Warning: Failed to load PNG: " << path
                          << " (" << IMG_GetError() << ")" << std::endl;
            }
        }
    }
    
    if (surface == nullptr) {
        // Silently return null texture - caller will handle missing assets
        return info;
    }
    
    info.texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (info.texture == nullptr) {
        std::cerr << "Failed to create texture from surface: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return info;
    }
    
    info.width = surface->w;
    info.height = surface->h;
    
    SDL_FreeSurface(surface);
    return info;
}

bool GraphicsSystem::load_tileset(const std::string& level_name) {
    if (tilesets.find(level_name) != tilesets.end()) {
        return true;
    }
    
    Tileset tileset;
    int missing_count = 0;
    int loaded_count = 0;
    std::string first_missing;
    
    // Load all tiles (0x00-0x7F / 0-127) for the level
    // Some levels have up to 87 tiles, so we try to load 128 to be safe
    // Missing tiles beyond what exists is expected and not an error
    for (int i = 0; i < 128; i++) {
        char tile_name[64];
        std::snprintf(tile_name, sizeof(tile_name), "%s.tt2-%02x.png", level_name.c_str(), i);
        
        std::string filepath = get_asset_path(tile_name);
        TextureInfo texture = load_png(filepath);
        
        if (texture.texture != nullptr) {
            tileset.tiles[i] = texture;
            loaded_count++;
        } else {
            missing_count++;
            if (first_missing.empty()) {
                first_missing = tile_name;
            }
        }
    }
    
    if (tileset.tiles.empty()) {
        std::cerr << "Error: Failed to load any tiles for tileset: " << level_name << std::endl;
        return false;
    }

    if (missing_count > 0) {
        std::cerr << "Warning: Tileset '" << level_name << "' missing "
                  << missing_count << " tile(s)"
                  << " (loaded " << loaded_count << ")"
                  << (first_missing.empty() ? "" : ", e.g. ")
                  << (first_missing.empty() ? "" : first_missing)
                  << std::endl;
    }
    
    tilesets[level_name] = tileset;
    return true;
}

Tileset* GraphicsSystem::get_tileset(const std::string& level_name) {
    auto it = tilesets.find(level_name);
    if (it != tilesets.end()) {
        return &it->second;
    }
    return nullptr;
}

bool GraphicsSystem::load_sprite(const std::string& sprite_name, const std::string& direction) {
    std::string key = sprite_name + "_" + direction;
    if (sprites.find(key) != sprites.end()) {
        return true;
    }
    
    std::string filename = "sprite-" + sprite_name + "_" + direction + ".png";
    std::string filepath = get_asset_path(filename);
    
    TextureInfo texture = load_png(filepath);
    if (texture.texture == nullptr) {
        std::cerr << "Warning: Missing sprite asset: " << filename << std::endl;
        return false;
    }
    
    Sprite sprite;
    sprite.texture = texture;
    sprite.width = texture.width;
    sprite.height = texture.height;
    
    sprites[key] = sprite;
    return true;
}

Sprite* GraphicsSystem::get_sprite(const std::string& sprite_name, const std::string& direction) {
    std::string key = sprite_name + "_" + direction;
    auto it = sprites.find(key);
    if (it != sprites.end()) {
        return &it->second;
    }
    return nullptr;
}

Animation GraphicsSystem::create_animation(const std::vector<std::string>& sprite_names, const std::string& direction, int frame_duration_ms, bool looping) {
    Animation anim;
    anim.looping = looping;
    anim.frame_start_time = SDL_GetTicks();
    int safe_duration_ms = frame_duration_ms > 0 ? frame_duration_ms : 1;
    
    for (const auto& sprite_name : sprite_names) {
        if (load_sprite(sprite_name, direction)) {
            Sprite* sprite = get_sprite(sprite_name, direction);
            if (sprite) {
                AnimationFrame frame;
                frame.sprite = *sprite;
                frame.duration_ms = safe_duration_ms;
                anim.frames.push_back(frame);
            }
        }
    }
    
    return anim;
}

AnimationFrame* GraphicsSystem::get_current_frame(Animation& anim) {
    if (anim.frames.empty()) {
        return nullptr;
    }
    return &anim.frames[anim.current_frame];
}

void GraphicsSystem::update_animation(Animation& anim, uint32_t current_time) {
    if (anim.frames.empty()) {
        return;
    }

    int total_duration = 0;
    for (const auto& frame : anim.frames) {
        total_duration += frame.duration_ms > 0 ? frame.duration_ms : 1;
    }

    if (total_duration <= 0) {
        anim.current_frame = 0;
        return;
    }

    uint32_t elapsed = current_time - anim.frame_start_time;
    if (anim.looping) {
        elapsed %= static_cast<uint32_t>(total_duration);
    } else if (elapsed >= static_cast<uint32_t>(total_duration)) {
        anim.current_frame = static_cast<int>(anim.frames.size()) - 1;
        return;
    }

    uint32_t cursor = 0;
    for (size_t i = 0; i < anim.frames.size(); i++) {
        int frame_duration = anim.frames[i].duration_ms > 0 ? anim.frames[i].duration_ms : 1;
        cursor += static_cast<uint32_t>(frame_duration);
        if (elapsed < cursor) {
            anim.current_frame = static_cast<int>(i);
            return;
        }
    }

    anim.current_frame = static_cast<int>(anim.frames.size()) - 1;
}

void GraphicsSystem::render_tile(int screen_x, int screen_y, Tileset* tileset, uint8_t tile_id, int scale) {
    if (tileset == nullptr) return;
    
    auto it = tileset->tiles.find(tile_id);
    if (it == tileset->tiles.end()) return;
    
    const TextureInfo& texture = it->second;
    int pixel_size = scale * 2; // 2 game units per tile
    SDL_Rect dst_rect = {screen_x, screen_y, pixel_size, pixel_size};
    SDL_RenderCopy(renderer, texture.texture, nullptr, &dst_rect);
}

void GraphicsSystem::render_sprite(int screen_x, int screen_y, const Sprite& sprite, bool flip_h) {
    SDL_Rect dst_rect = {screen_x, screen_y, sprite.width, sprite.height};
    SDL_RendererFlip flip = flip_h ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderCopyEx(renderer, sprite.texture.texture, nullptr, &dst_rect, 0, nullptr, flip);
}

void GraphicsSystem::render_sprite_scaled(int screen_x, int screen_y, const Sprite& sprite, int width, int height, bool flip_h) {
    SDL_Rect dst_rect = {screen_x, screen_y, width, height};
    SDL_RendererFlip flip = flip_h ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderCopyEx(renderer, sprite.texture.texture, nullptr, &dst_rect, 0, nullptr, flip);
}

void GraphicsSystem::render_sprite_centered(int screen_x, int screen_y, const Sprite& sprite, bool flip_h) {
    int x = screen_x - sprite.width / 2;
    int y = screen_y - sprite.height / 2;
    render_sprite(x, y, sprite, flip_h);
}

void GraphicsSystem::render_sprite_centered_scaled(int screen_x, int screen_y, const Sprite& sprite, int width, int height, bool flip_h) {
    int x = screen_x - width / 2;
    int y = screen_y - height / 2;
    render_sprite_scaled(x, y, sprite, width, height, flip_h);
}

void GraphicsSystem::render_text(int screen_x, int screen_y, const std::string& text, SDL_Color color) {
    if (debug_font == nullptr) {
        return;  // Font not available
    }
    
    SDL_Surface* surface = TTF_RenderText_Solid(debug_font, text.c_str(), color);
    if (surface == nullptr) {
        return;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == nullptr) {
        SDL_FreeSurface(surface);
        return;
    }
    
    SDL_Rect dst_rect = {screen_x, screen_y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &dst_rect);
    
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void GraphicsSystem::render_debug_overlay() {
    // Draw a semi-transparent debug indicator in top-left corner
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    // Background box
    SDL_Rect bg_rect = {5, 5, 200, 100};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);  // Semi-transparent black
    SDL_RenderFillRect(renderer, &bg_rect);
    
    // Border
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);  // Yellow border
    SDL_RenderDrawRect(renderer, &bg_rect);
    
    // Draw noclip indicator if active
    if (cheat_noclip) {
        SDL_Rect noclip_indicator = {10, 10, 20, 20};
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);  // Green square for noclip
        SDL_RenderFillRect(renderer, &noclip_indicator);
    }
    
    // Draw simple bars to visualize velocity
    // Y velocity bar (vertical)
    int vel_bar_height = std::abs(comic_y_vel) * 2;
    if (vel_bar_height > 50) vel_bar_height = 50;
    SDL_Rect vel_bar = {40, 50 - vel_bar_height / 2, 10, vel_bar_height};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Red for velocity
    SDL_RenderFillRect(renderer, &vel_bar);
    
    // X momentum bar (horizontal)
    int momentum_bar_width = std::abs(comic_x_momentum) * 3;
    if (momentum_bar_width > 50) momentum_bar_width = 50;
    SDL_Rect momentum_bar = {60, 40, momentum_bar_width, 10};
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);  // Blue for momentum
    SDL_RenderFillRect(renderer, &momentum_bar);
    
    // Reset blend mode
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    
    // Render coordinate information as text
    if (debug_font != nullptr) {
        std::ostringstream coord_text;
        coord_text << "X: " << static_cast<int>(comic_x) << " Y: " << static_cast<int>(comic_y);
        render_text(10, 70, coord_text.str(), {0, 255, 255, 255});  // Cyan text
        
        std::ostringstream level_text;
        level_text << "L" << static_cast<int>(current_level_number) 
                   << " S" << static_cast<int>(current_stage_number);
        render_text(10, 85, level_text.str(), {0, 255, 255, 255});  // Cyan text
    }
}

void GraphicsSystem::cleanup() {
    // Clean up tilesets
    for (auto& pair : tilesets) {
        pair.second.cleanup();
    }
    tilesets.clear();
    
    // Clean up sprites
    for (auto& pair : sprites) {
        if (pair.second.texture.texture) {
            SDL_DestroyTexture(pair.second.texture.texture);
        }
    }
    sprites.clear();
    
    // Clean up fonts
    if (debug_font != nullptr) {
        TTF_CloseFont(debug_font);
        debug_font = nullptr;
    }
    if (TTF_WasInit() != 0) {
        TTF_Quit();
    }
    
    // Only quit SDL_image once to prevent double-quit errors
    if (img_inited) {
        IMG_Quit();
        img_inited = false;
    }
}

void Tileset::cleanup() {
    for (auto& pair : tiles) {
        if (pair.second.texture) {
            SDL_DestroyTexture(pair.second.texture);
        }
    }
    tiles.clear();
}
