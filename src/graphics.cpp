#include "../include/graphics.h"
#include <SDL2/SDL_image.h>
#include <iostream>
#include <fstream>

// Global graphics system
GraphicsSystem* g_graphics = nullptr;

GraphicsSystem::GraphicsSystem(SDL_Renderer* renderer) : renderer(renderer) {}

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
    return true;
}

std::string GraphicsSystem::get_asset_path(const std::string& filename) {
    // Try multiple possible locations for assets:
    // 1. ./assets/ (from current working directory)
    // 2. ../assets/ (from build directory)
    // 3. ../../assets/ (from nested build directory)

    // Note: In a full implementation, we might want to check if the file actually exists 
    // at these paths and return the first valid one. For now, we'll just return the first 
    // path and assume the user runs from the correct directory.
    // std::string paths[] = {
    //     "assets/" + filename,
    //     "../assets/" + filename,
    //     "../../assets/" + filename
    // };
    
    // For now, just return the first path - the user should run from the correct directory
    return "assets/" + filename;
}

TextureInfo GraphicsSystem::load_png(const std::string& filepath) {
    TextureInfo info = {nullptr, 0, 0};
    
    // Try multiple possible paths
    std::string possible_paths[] = {
        filepath,
        "../" + filepath,
        "../../" + filepath
    };
    
    SDL_Surface* surface = nullptr;
    std::string loaded_from;
    
    for (const auto& path : possible_paths) {
        // Check if file exists
        std::ifstream f(path);
        if (f.good()) {
            surface = IMG_Load(path.c_str());
            if (surface) {
                loaded_from = path;
                break;
            }
        }
    }
    
    if (surface == nullptr) {
        std::cerr << "Failed to load image: " << filepath << " - " << IMG_GetError() << std::endl;
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
    
    // Load all 64 tiles (0x00-0x3F) for the level
    for (int i = 0; i < 64; i++) {
        char tile_name[64];
        snprintf(tile_name, sizeof(tile_name), "%s.tt2-%02x.png", level_name.c_str(), i);
        
        std::string filepath = get_asset_path(tile_name);
        TextureInfo texture = load_png(filepath);
        
        if (texture.texture != nullptr) {
            tileset.tiles[i] = texture;
        }
    }
    
    if (tileset.tiles.empty()) {
        std::cerr << "Failed to load any tiles for tileset: " << level_name << std::endl;
        return false;
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
    
    for (const auto& sprite_name : sprite_names) {
        if (load_sprite(sprite_name, direction)) {
            Sprite* sprite = get_sprite(sprite_name, direction);
            if (sprite) {
                AnimationFrame frame;
                frame.sprite = *sprite;
                frame.duration_ms = frame_duration_ms;
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
    
    uint32_t elapsed = current_time - anim.frame_start_time;
    int current_frame = (elapsed / anim.frames[0].duration_ms) % anim.frames.size();
    
    if (!anim.looping && current_frame >= anim.frames.size() - 1) {
        current_frame = anim.frames.size() - 1;
    }
    
    anim.current_frame = current_frame;
}

void GraphicsSystem::render_tile(int screen_x, int screen_y, Tileset* tileset, uint8_t tile_id) {
    if (tileset == nullptr) return;
    
    auto it = tileset->tiles.find(tile_id);
    if (it == tileset->tiles.end()) return;
    
    const TextureInfo& texture = it->second;
    SDL_Rect dst_rect = {screen_x, screen_y, texture.width, texture.height};
    SDL_RenderCopy(renderer, texture.texture, nullptr, &dst_rect);
}

void GraphicsSystem::render_sprite(int screen_x, int screen_y, const Sprite& sprite, bool flip_h) {
    SDL_Rect dst_rect = {screen_x, screen_y, sprite.width, sprite.height};
    SDL_RendererFlip flip = flip_h ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderCopyEx(renderer, sprite.texture.texture, nullptr, &dst_rect, 0, nullptr, flip);
}

void GraphicsSystem::render_sprite_centered(int screen_x, int screen_y, const Sprite& sprite, bool flip_h) {
    int x = screen_x - sprite.width / 2;
    int y = screen_y - sprite.height / 2;
    render_sprite(x, y, sprite, flip_h);
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
    
    IMG_Quit();
}

void Tileset::cleanup() {
    for (auto& pair : tiles) {
        if (pair.second.texture) {
            SDL_DestroyTexture(pair.second.texture);
        }
    }
    tiles.clear();
}
