#include "audio.h"

#if defined(HAVE_SDL2_MIXER)

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <algorithm>
#include <array>
#include <iostream>

namespace {

constexpr int AUDIO_SAMPLE_RATE = 22050;
constexpr int AUDIO_CHANNELS = 1;
constexpr int AUDIO_CHUNK_SIZE = 1024;
constexpr int SFX_CHANNEL = 0;

struct SoundDefinition {
    int frequency_hz;
    uint16_t duration_ms;
    uint8_t priority;
};

struct LoadedSound {
    Mix_Chunk* chunk;
    uint16_t duration_ms;
    uint8_t priority;
};

std::array<LoadedSound, static_cast<size_t>(GameSound::COUNT)> g_sounds{};
bool g_audio_initialized = false;
uint8_t g_current_priority = 0;
uint32_t g_current_sound_end_tick = 0;

constexpr std::array<SoundDefinition, static_cast<size_t>(GameSound::COUNT)> SOUND_DEFINITIONS = {{
    {392, 120, 4},
    {784, 70, 5},
    {988, 150, 6},
    {330, 140, 5},
    {196, 180, 3},
    {523, 110, 4},
    {165, 140, 8},
    {110, 260, 9},
    {659, 140, 5},
    {1047, 200, 7},
    {1175, 170, 7},
    {247, 130, 5},
    {1319, 320, 10},
}};

Mix_Chunk* create_square_wave_chunk(int frequency_hz, uint16_t duration_ms) {
    if (frequency_hz <= 0 || duration_ms == 0) {
        return nullptr;
    }

    const uint32_t sample_count =
        static_cast<uint32_t>((static_cast<uint64_t>(AUDIO_SAMPLE_RATE) * duration_ms) / 1000);
    const uint32_t byte_count = sample_count * sizeof(int16_t);
    if (sample_count == 0 || byte_count == 0) {
        return nullptr;
    }

    auto* sample_buffer = static_cast<int16_t*>(SDL_malloc(byte_count));
    if (!sample_buffer) {
        return nullptr;
    }

    const int period = std::max(1, AUDIO_SAMPLE_RATE / frequency_hz);
    const int half_period = std::max(1, period / 2);
    constexpr int16_t amplitude = 9000;

    for (uint32_t sample_index = 0; sample_index < sample_count; ++sample_index) {
        int phase = static_cast<int>(sample_index % period);
        sample_buffer[sample_index] = (phase < half_period) ? amplitude : -amplitude;
    }

    auto* chunk = static_cast<Mix_Chunk*>(SDL_malloc(sizeof(Mix_Chunk)));
    if (!chunk) {
        SDL_free(sample_buffer);
        return nullptr;
    }

    chunk->allocated = 1;
    chunk->abuf = reinterpret_cast<uint8_t*>(sample_buffer);
    chunk->alen = byte_count;
    chunk->volume = MIX_MAX_VOLUME;
    return chunk;
}

void free_loaded_sounds() {
    for (auto& loaded_sound : g_sounds) {
        if (loaded_sound.chunk) {
            Mix_FreeChunk(loaded_sound.chunk);
            loaded_sound.chunk = nullptr;
        }
        loaded_sound.duration_ms = 0;
        loaded_sound.priority = 0;
    }
}

} // namespace

bool initialize_audio_system() {
    if (g_audio_initialized) {
        return true;
    }

    if (Mix_OpenAudio(AUDIO_SAMPLE_RATE, AUDIO_S16SYS, AUDIO_CHANNELS, AUDIO_CHUNK_SIZE) < 0) {
        std::cerr << "Failed to initialize SDL_mixer audio: " << Mix_GetError() << std::endl;
        return false;
    }

    Mix_AllocateChannels(8);

    for (size_t index = 0; index < SOUND_DEFINITIONS.size(); ++index) {
        const SoundDefinition& definition = SOUND_DEFINITIONS[index];
        Mix_Chunk* chunk = create_square_wave_chunk(definition.frequency_hz, definition.duration_ms);
        if (!chunk) {
            std::cerr << "Failed to synthesize sound effect index " << index << std::endl;
            free_loaded_sounds();
            Mix_CloseAudio();
            return false;
        }

        g_sounds[index].chunk = chunk;
        g_sounds[index].duration_ms = definition.duration_ms;
        g_sounds[index].priority = definition.priority;
    }

    g_current_priority = 0;
    g_current_sound_end_tick = 0;
    g_audio_initialized = true;
    return true;
}

void shutdown_audio_system() {
    if (!g_audio_initialized) {
        return;
    }

    Mix_HaltChannel(SFX_CHANNEL);
    free_loaded_sounds();
    Mix_CloseAudio();

    g_audio_initialized = false;
    g_current_priority = 0;
    g_current_sound_end_tick = 0;
}

bool is_audio_system_ready() {
    return g_audio_initialized;
}

bool play_game_sound(GameSound sound) {
    if (!g_audio_initialized) {
        return false;
    }

    size_t sound_index = static_cast<size_t>(sound);
    if (sound_index >= g_sounds.size()) {
        return false;
    }

    const LoadedSound& requested_sound = g_sounds[sound_index];
    if (!requested_sound.chunk) {
        return false;
    }

    uint32_t now = SDL_GetTicks();
    bool channel_is_busy = Mix_Playing(SFX_CHANNEL) != 0 && now < g_current_sound_end_tick;
    if (channel_is_busy && requested_sound.priority < g_current_priority) {
        return false;
    }

    if (channel_is_busy) {
        Mix_HaltChannel(SFX_CHANNEL);
    }

    if (Mix_PlayChannel(SFX_CHANNEL, requested_sound.chunk, 0) < 0) {
        return false;
    }

    g_current_priority = requested_sound.priority;
    g_current_sound_end_tick = now + requested_sound.duration_ms;
    return true;
}

#else

bool initialize_audio_system() {
    return false;
}

void shutdown_audio_system() {
}

bool is_audio_system_ready() {
    return false;
}

bool play_game_sound(GameSound sound) {
    (void)sound;
    return false;
}

#endif
