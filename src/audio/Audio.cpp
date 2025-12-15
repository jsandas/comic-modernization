#include "Audio.h"
#include <iostream>
#include <cstring>

Audio::Audio() {
    SDL_zero(desired_spec);
    SDL_zero(obtained_spec);
}

Audio::~Audio() {
    shutdown();
}

bool Audio::initialize() {
    // Set up audio spec for playback
    desired_spec.freq = 44100;
    desired_spec.format = AUDIO_F32;
    desired_spec.channels = 1;
    desired_spec.samples = 4096;
    desired_spec.callback = audioCallback;
    desired_spec.userdata = this;

    audio_device = SDL_OpenAudioDevice(nullptr, 0, &desired_spec, &obtained_spec, 0);
    if (audio_device == 0) {
        std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
        return false;
    }

    std::cout << "Audio system initialized at " << obtained_spec.freq << " Hz" << std::endl;
    return true;
}

void Audio::shutdown() {
    if (audio_device != 0) {
        SDL_CloseAudioDevice(audio_device);
        audio_device = 0;
    }
}

void Audio::playSound(const char* name, uint8_t priority) {
    if (priority < current_priority) {
        return;  // Don't interrupt higher priority sound
    }

    // Placeholder for sound loading and playing
    // Will be implemented with asset manager integration
}

void Audio::stopSound() {
    current_sound.data = nullptr;
    current_sound.length = 0;
    current_priority = 0;
    sound_position = 0;
}

void Audio::setMuted(bool muted) {
    is_muted = muted;
    SDL_PauseAudioDevice(audio_device, muted ? 1 : 0);
}

void Audio::audioCallback(void* userData, uint8_t* stream, int len) {
    Audio* self = reinterpret_cast<Audio*>(userData);
    
    // Clear buffer
    std::memset(stream, 0, len);

    if (self->is_muted || self->current_sound.data == nullptr) {
        return;
    }

    // Placeholder: actual audio playback will be implemented
    // when we integrate with sound data from assets
}

void Audio::synthesizeSquareWave(uint8_t* buffer, int length, uint16_t frequency_divisor) {
    // Convert DOS timer frequency divisor to Hz
    // DOS timer frequency: 1193182 Hz
    float frequency = 1193182.0f / frequency_divisor;

    // Generate square wave at the given frequency
    // Placeholder for implementation
}
