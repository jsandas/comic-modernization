#pragma once

#include <SDL2/SDL.h>
#include <memory>
#include "game/Constants.h"

class Graphics {
public:
    Graphics();
    ~Graphics();

    bool initialize();
    void shutdown();

    // Rendering operations
    void clear();
    void present();
    void drawTile(int x, int y, uint8_t tile_id);
    void drawSprite(int x, int y, const char* sprite_name);

    // Get SDL objects
    SDL_Window* getWindow() const { return window; }
    SDL_Renderer* getRenderer() const { return renderer; }

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    std::array<SDL_Color, 16> palette;

    void initializePalette();
};
