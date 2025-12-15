#pragma once

#include <SDL2/SDL.h>
#include <array>

class Input {
public:
    Input();
    ~Input();

    void initialize();
    void handleEvent(const SDL_Event& event);
    void update();

    // Query input state
    bool isPressed(SDL_Scancode key) const;
    bool isKeyDown(SDL_Scancode key) const;
    bool isKeyUp(SDL_Scancode key) const;

    // Game action queries
    bool moveLeft() const;
    bool moveRight() const;
    bool jump() const;
    bool fire() const;
    bool open() const;
    bool teleport() const;

    // Configuration
    void loadKeymap();
    void saveKeymap();

private:
    std::array<uint8_t, SDL_NUM_SCANCODES> key_state = {};
    std::array<uint8_t, SDL_NUM_SCANCODES> prev_key_state = {};

    // Key mappings
    SDL_Scancode key_jump = SDL_SCANCODE_SPACE;
    SDL_Scancode key_fire = SDL_SCANCODE_INSERT;
    SDL_Scancode key_left = SDL_SCANCODE_LEFT;
    SDL_Scancode key_right = SDL_SCANCODE_RIGHT;
    SDL_Scancode key_open = SDL_SCANCODE_LALT;
    SDL_Scancode key_teleport = SDL_SCANCODE_CAPSLOCK;
};
