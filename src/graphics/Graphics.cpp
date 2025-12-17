#include "Graphics.h"
#include "assets/AssetManager.h"
#include "game/GameState.h"
#include <iostream>

Graphics::Graphics() 
    : window(nullptr), renderer(nullptr), assetManager(nullptr) {
}

Graphics::~Graphics() {
    shutdown();
}

bool Graphics::initialize() {
    // Create window
    window = SDL_CreateWindow(
        "Captain Comic",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        GameConstants::SCREEN_WIDTH * 2,  // Scale 2x for visibility
        GameConstants::SCREEN_HEIGHT * 2,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!window) {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create renderer with vsync
    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer) {
        std::cerr << "Failed to create SDL renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        return false;
    }

    // Set logical size for scaling
    SDL_RenderSetLogicalSize(renderer, GameConstants::SCREEN_WIDTH, GameConstants::SCREEN_HEIGHT);

    initializePalette();

    std::cout << "Graphics system initialized" << std::endl;
    return true;
}

void Graphics::shutdown() {
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
}

void Graphics::clear() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void Graphics::present() {
    SDL_RenderPresent(renderer);
}

void Graphics::drawTile(int px, int py, uint8_t tile_id, const std::string& tileset_name) {
    if (!assetManager || !renderer) {
        return;
    }

    // Load the tileset texture for this specific tile
    // Tileset naming convention: "tileset-id" (e.g., "forest.tt2-00")
    std::string tile_name = tileset_name + "-" + 
                           (tile_id < 16 ? "0" : "") + 
                           (char)(tile_id < 10 ? ('0' + tile_id) : ('a' + tile_id - 10));
    
    SDL_Texture* texture = assetManager->getTexture(tile_name);
    if (!texture) {
        return;
    }

    SDL_Rect dest_rect;
    dest_rect.x = px;
    dest_rect.y = py;
    dest_rect.w = GameConstants::TILE_SIZE;
    dest_rect.h = GameConstants::TILE_SIZE;

    SDL_RenderCopy(renderer, texture, nullptr, &dest_rect);
}

void Graphics::drawSprite(int px, int py, SDL_Texture* texture, int width, int height) {
    if (!texture || !renderer) {
        return;
    }

    SDL_Rect dest_rect;
    dest_rect.x = px;
    dest_rect.y = py;
    
    // If width/height not specified, query texture size
    if (width == 0 || height == 0) {
        SDL_QueryTexture(texture, nullptr, nullptr, &dest_rect.w, &dest_rect.h);
    } else {
        dest_rect.w = width;
        dest_rect.h = height;
    }

    SDL_RenderCopy(renderer, texture, nullptr, &dest_rect);
}

void Graphics::drawSpriteByName(int px, int py, const std::string& sprite_name, int width, int height) {
    if (!assetManager) {
        return;
    }

    // Get cached texture info to avoid redundant SDL_QueryTexture calls
    TextureInfo info = assetManager->getTextureInfo(sprite_name);
    if (info.texture) {
        // Use cached dimensions if width/height not specified
        int final_width = (width > 0) ? width : info.width;
        int final_height = (height > 0) ? height : info.height;
        drawSprite(px, py, info.texture, final_width, final_height);
    }
}

void Graphics::renderTileMap(const TileMap& tilemap, int camera_x, int camera_y, const std::string& tileset_name) {
    if (!assetManager || !renderer) {
        return;
    }

    // Calculate which tiles are visible on screen
    int start_tile_x = camera_x / GameConstants::TILE_SIZE;
    int start_tile_y = camera_y / GameConstants::TILE_SIZE;
    
    int tiles_wide = (GameConstants::SCREEN_WIDTH / GameConstants::TILE_SIZE) + 2;
    int tiles_high = (GameConstants::SCREEN_HEIGHT / GameConstants::TILE_SIZE) + 2;
    
    // Clamp to map bounds
    start_tile_x = (start_tile_x < 0) ? 0 : start_tile_x;
    start_tile_y = (start_tile_y < 0) ? 0 : start_tile_y;
    
    int end_tile_x = start_tile_x + tiles_wide;
    int end_tile_y = start_tile_y + tiles_high;
    
    if (end_tile_x > GameConstants::SCREEN_WIDTH_TILES) {
        end_tile_x = GameConstants::SCREEN_WIDTH_TILES;
    }
    if (end_tile_y > GameConstants::SCREEN_HEIGHT_TILES) {
        end_tile_y = GameConstants::SCREEN_HEIGHT_TILES;
    }

    // Render visible tiles
    for (int ty = start_tile_y; ty < end_tile_y; ++ty) {
        for (int tx = start_tile_x; tx < end_tile_x; ++tx) {
            int tile_idx = ty * GameConstants::SCREEN_WIDTH_TILES + tx;
            
            if (tile_idx >= 0 && tile_idx < static_cast<int>(tilemap.tiles.size())) {
                uint8_t tile_id = tilemap.tiles[tile_idx];
                
                // Calculate screen position
                int screen_x = (tx * GameConstants::TILE_SIZE) - camera_x;
                int screen_y = (ty * GameConstants::TILE_SIZE) - camera_y;
                
                drawTile(screen_x, screen_y, tile_id, tileset_name);
            }
        }
    }
}

void Graphics::initializePalette() {
    // Convert EGA palette to SDL_Color format
    for (int i = 0; i < 16; ++i) {
        uint32_t color = GameConstants::EGA_PALETTE[i];
        palette[i].r = (color >> 16) & 0xFF;
        palette[i].g = (color >> 8) & 0xFF;
        palette[i].b = color & 0xFF;
        palette[i].a = 255;
    }
}
