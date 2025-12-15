#include "Graphics.h"
#include <iostream>

Graphics::Graphics() 
    : window(nullptr), renderer(nullptr) {
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

void Graphics::drawTile(int x, int y, uint8_t tile_id) {
    // Placeholder for tile rendering
    // Will be implemented when asset loading is complete
}

void Graphics::drawSprite(int x, int y, const char* sprite_name) {
    // Placeholder for sprite rendering
    // Will be implemented when asset loading is complete
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
