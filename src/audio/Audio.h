#pragma once

#include <SDL2/SDL.h>
#include <cstdint>
#include <queue>

struct Sound {
    uint16_t* data;
    uint32_t length;
    uint8_t priority;
    bool operator<(const Sound& other) const {
        return priority < other.priority;  // For priority queue
    }
};

class Audio {
public:
    Audio();
    ~Audio();

    bool initialize();
    void shutdown();

    // Sound control
    void playSound(const char* name, uint8_t priority);
    void stopSound();
    void setMuted(bool muted);
    bool isMuted() const { return is_muted; }

    // Audio callback for SDL
    static void audioCallback(void* userData, uint8_t* stream, int len);

private:
    SDL_AudioDeviceID audio_device = 0;
    SDL_AudioSpec desired_spec, obtained_spec;
    
    Sound current_sound = {nullptr, 0, 0};
    uint32_t sound_position = 0;
    uint8_t current_priority = 0;
    bool is_muted = false;

    // Synthesize PC speaker sound
    void synthesizeSquareWave(uint8_t* buffer, int length, uint16_t frequency_divisor);
};
