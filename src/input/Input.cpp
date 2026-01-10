#include "Input.h"
#include <iostream>
#include <fstream>

Input::Input() {
    key_state.fill(0);
    prev_key_state.fill(0);
}

Input::~Input() {
}

void Input::initialize() {
    loadKeymap();
    std::cout << "Input system initialized" << std::endl;
}

void Input::handleEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_KEYDOWN:
            key_state[event.key.keysym.scancode] = 1;
            break;
        case SDL_KEYUP:
            key_state[event.key.keysym.scancode] = 0;
            break;
        default:
            break;
    }
}

void Input::update() {
    prev_key_state = key_state;
}

bool Input::isPressed(SDL_Scancode key) const {
    return key_state[key] != 0;
}

bool Input::isKeyDown(SDL_Scancode key) const {
    return key_state[key] != 0 && prev_key_state[key] == 0;
}

bool Input::isKeyUp(SDL_Scancode key) const {
    return key_state[key] == 0 && prev_key_state[key] != 0;
}

bool Input::moveLeft() const {
    // Support arrow key *and* A for left
    return isPressed(key_left) || isPressed(SDL_SCANCODE_A);
}

bool Input::moveRight() const {
    // Support arrow key *and* D for right
    return isPressed(key_right) || isPressed(SDL_SCANCODE_D);
}

bool Input::jump() const {
    return isPressed(key_jump);
}

bool Input::fire() const {
    return isPressed(key_fire);
}

bool Input::open() const {
    return isPressed(key_open);
}

bool Input::teleport() const {
    return isPressed(key_teleport);
}

void Input::loadKeymap() {
    // Try to load KEYS.DEF from data directory
    // Default key mappings are already set in header
    // TODO: Load from data/KEYS.DEF if it exists
}

void Input::saveKeymap() {
    // TODO: Save current key mapping to data/KEYS.DEF
}
