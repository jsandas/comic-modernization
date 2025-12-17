#pragma once

#include <SDL2/SDL.h>
#include <memory>
#include <string>
#include <cstdint>
#include "game/Constants.h"

// Forward declarations
class AssetManager;

class Graphics {
public:
    Graphics();
    ~Graphics();

    bool initialize();
    void shutdown();
    void setAssetManager(AssetManager* assets) { assetManager = assets; }

    // Rendering operations
    void clear();
    void present();
    
    // Tile rendering - draws a single tile from tileset at screen position (px, py)
    void drawTile(int px, int py, uint8_t tile_id, const std::string& tileset_name);
    
    // Sprite rendering - draws a sprite texture at screen position (px, py)
    // If width/height are 0, uses texture size
    void drawSprite(int px, int py, SDL_Texture* texture, int width = 0, int height = 0);
    void drawSpriteByName(int px, int py, const std::string& sprite_name, int width = 0, int height = 0);
    
    // Full map rendering - renders tilemap at given offset
    void renderTileMap(const class TileMap& tilemap, int camera_x, int camera_y, const std::string& tileset_name);
    
    // Get SDL objects
    SDL_Window* getWindow() const { return window; }
    SDL_Renderer* getRenderer() const { return renderer; }

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    AssetManager* assetManager = nullptr;
    std::array<SDL_Color, 16> palette;

    void initializePalette();
};
