#include "audio.h"

#if defined(HAVE_SDL2_MIXER)

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <vector>

namespace {

// ===== Audio Constants =====
constexpr int AUDIO_SAMPLE_RATE = 22050;
constexpr int AUDIO_CHANNELS = 1;
constexpr int AUDIO_CHUNK_SIZE = 1024;
constexpr int SFX_CHANNEL = 0;

// ===== Shared Musical Notes (Hz) =====
// Use these for melodic sequences where exact note names are intended.
// Keep PIT-derived effect tones as dedicated FREQ_* constants for original fidelity.
constexpr int NOTE_C4 = 261;
constexpr int NOTE_D4 = 293;
constexpr int NOTE_E4 = 330;
constexpr int NOTE_E5 = 659;
constexpr int NOTE_F4 = 349;
constexpr int NOTE_FS4 = 370;
constexpr int NOTE_G4 = 392;
constexpr int NOTE_A4 = 440;
constexpr int NOTE_B3 = 247;
constexpr int NOTE_C5 = 523;
constexpr int NOTE_G5 = 784;

// ===== Frequency Definitions (PIT divisors converted to Hz) =====
// Conversion formula: Frequency = 1193182 Hz / divisor
// These match the original PC speaker implementation

// Door sound: 0x0f00, 0x0d00, 0x0c00, 0x0b00, 0x0a00, 0x0b00, 0x0c00, 0x0d00, 0x0f00
constexpr int FREQ_DOOR_1 = 310;   // 0x0f00
constexpr int FREQ_DOOR_2 = 358;   // 0x0d00
constexpr int FREQ_DOOR_3 = 388;   // 0x0c00
constexpr int FREQ_DOOR_4 = 423;   // 0x0b00
constexpr int FREQ_DOOR_5 = 466;   // 0x0a00

// Fire sound: 0x2000, 0x1e00
constexpr int FREQ_FIRE_1 = 145;   // 0x2000
constexpr int FREQ_FIRE_2 = 155;   // 0x1e00

// Hit enemy sound: 0x0800, 0x0400
constexpr int FREQ_HIT_1 = 582;    // 0x0800
constexpr int FREQ_HIT_2 = 1165;   // 0x0400

// Collect item: 0x0fda, 0x0c90, 0x0a91, 0x0800
constexpr int FREQ_COLLECT_1 = 294;   // 0x0fda
constexpr int FREQ_COLLECT_2 = 371;   // 0x0c90
constexpr int FREQ_COLLECT_3 = 441;   // 0x0a91
constexpr int FREQ_COLLECT_4 = 582;   // 0x0800

// Teleport: 0x2000, 0x1e00, 0x1c00, 0x1a00, 0x1c00, 0x1e00, 0x2000
constexpr int FREQ_TELEPORT_1 = 145;   // 0x2000
constexpr int FREQ_TELEPORT_2 = 155;   // 0x1e00
constexpr int FREQ_TELEPORT_3 = 166;   // 0x1c00
constexpr int FREQ_TELEPORT_4 = 179;   // 0x1a00

// Death sound: descending tones   0x3000, 0x3800, 0x4000, 0x0800, 0x1000, 0x1800
constexpr int FREQ_DEATH_1 = 97;    // 0x3000
constexpr int FREQ_DEATH_2 = 83;    // 0x3800
constexpr int FREQ_DEATH_3 = 72;    // 0x4000
constexpr int FREQ_DEATH_4 = 582;   // 0x0800
constexpr int FREQ_DEATH_5 = 291;   // 0x1000
constexpr int FREQ_DEATH_6 = 194;   // 0x1800



// ===== Sound Definition Structure =====
struct FrequencyNote {
    int frequency_hz;
    uint16_t duration_ticks;  // Duration in ticks (at ~18.2 Hz = ~55ms per tick)
};

// ===== Sound Sequences (matching original PC speaker sounds) =====
// Each sound is a sequence of frequencies played in order

static const std::vector<FrequencyNote> SOUND_FIRE_SEQUENCE = {
    {FREQ_FIRE_1, 1}, {FREQ_FIRE_2, 1}
};

static const std::vector<FrequencyNote> SOUND_ITEM_COLLECT_SEQUENCE = {
    {FREQ_COLLECT_1, 2}, {FREQ_COLLECT_2, 2}, {FREQ_COLLECT_3, 2}, {FREQ_COLLECT_4, 2}
};

static const std::vector<FrequencyNote> SOUND_DOOR_OPEN_SEQUENCE = {
    {FREQ_DOOR_1, 1}, {FREQ_DOOR_2, 1}, {FREQ_DOOR_3, 1}, {FREQ_DOOR_4, 1}, {FREQ_DOOR_5, 1},
    {FREQ_DOOR_4, 1}, {FREQ_DOOR_3, 1}, {FREQ_DOOR_2, 1}, {FREQ_DOOR_1, 1}
};

static const std::vector<FrequencyNote> SOUND_STAGE_TRANSITION_SEQUENCE = {
    {NOTE_C4, 3}, {NOTE_D4, 3}, {NOTE_F4, 6}, {NOTE_F4, 6},
    {NOTE_G4, 3}, {NOTE_A4, 6}, {NOTE_G4, 6}
};

static const std::vector<FrequencyNote> SOUND_ENEMY_HIT_SEQUENCE = {
    {FREQ_HIT_1, 1}, {FREQ_HIT_2, 1}
};

static const std::vector<FrequencyNote> SOUND_PLAYER_HIT_SEQUENCE = {
    {FREQ_DEATH_1, 2}, {FREQ_DEATH_2, 2}, {FREQ_DEATH_3, 2}
};

static const std::vector<FrequencyNote> SOUND_PLAYER_DIE_SEQUENCE = {
    {FREQ_DEATH_1, 1}, {FREQ_DEATH_2, 1}, {FREQ_DEATH_3, 1}, 
    {FREQ_DEATH_4, 1}, {FREQ_DEATH_5, 1}, {FREQ_DEATH_6, 2}
};

static const std::vector<FrequencyNote> SOUND_GAME_OVER_SEQUENCE = {
    {NOTE_B3, 2}, {NOTE_C4, 4}, {NOTE_D4, 2}, {NOTE_E4, 6}, {NOTE_G4, 7},
    {NOTE_FS4, 5}, {NOTE_E4, 2}, {NOTE_D4, 4}, {NOTE_E4, 15}
};

static const std::vector<FrequencyNote> SOUND_TELEPORT_SEQUENCE = {
    {FREQ_TELEPORT_1, 2}, {FREQ_TELEPORT_2, 2}, {FREQ_TELEPORT_3, 2}, {FREQ_TELEPORT_4, 2},
    {FREQ_TELEPORT_3, 2}, {FREQ_TELEPORT_2, 2}, {FREQ_TELEPORT_1, 2}
};

// ===== Loaded Sound Structure =====
struct LoadedSound {
    Mix_Chunk* chunk;
    uint16_t total_duration_ms;
    uint8_t priority;
};

std::array<LoadedSound, static_cast<size_t>(GameSound::COUNT)> g_sounds{};
bool g_audio_initialized = false;
// Track whether we called SDL_InitSubSystem(SDL_INIT_AUDIO)
// so shutdown can undo it. SDL may have been initialized by caller.
bool g_sdl_audio_initialized = false;
uint8_t g_current_priority = 0;
uint32_t g_current_sound_end_tick = 0;

// ===== Sound Priorities =====
constexpr std::array<uint8_t, static_cast<size_t>(GameSound::COUNT)> SOUND_PRIORITIES = {{
    0,   // UNUSED_0 (no jump sound in original game)
    5,   // FIRE
    6,   // ITEM_COLLECT (includes treasures and power-ups)
    5,   // DOOR_OPEN
    3,   // STAGE_TRANSITION
    4,   // ENEMY_HIT
    8,   // PLAYER_HIT
    9,   // PLAYER_DIE
    2,   // GAME_OVER
    7,   // TELEPORT
}};


/**
 * Convert game ticks to milliseconds
 * Original PC speaker rate: ~18.2 Hz game ticks (~55 ms per tick)
 */
static uint16_t ticks_to_ms(uint16_t ticks) {
    return static_cast<uint16_t>((static_cast<uint64_t>(ticks) * 55) / 1);
}

/**
 * Synthesize a complete sound from a frequency sequence
 * 
 * Concatenates synthesized square waves for each frequency/duration pair
 * into a single SDL_Chunk.
 */
Mix_Chunk* create_sound_sequence_chunk(const std::vector<FrequencyNote>& sequence) {
    if (sequence.empty()) {
        return nullptr;
    }

    // Calculate total sample count needed
    uint32_t total_samples = 0;
    for (const auto& note : sequence) {
        uint16_t duration_ms = ticks_to_ms(note.duration_ticks);
        uint32_t note_samples = (static_cast<uint64_t>(AUDIO_SAMPLE_RATE) * duration_ms) / 1000;
        total_samples += note_samples;
    }

    if (total_samples == 0) {
        return nullptr;
    }

    uint32_t byte_count = total_samples * sizeof(int16_t);
    auto* sample_buffer = static_cast<int16_t*>(SDL_malloc(byte_count));
    if (!sample_buffer) {
        return nullptr;
    }

    // Generate waveform for each frequency note
    uint32_t current_sample = 0;
    constexpr int16_t amplitude = 9000;

    for (const auto& note : sequence) {
        if (note.frequency_hz <= 0) {
            continue;
        }

        uint16_t duration_ms = ticks_to_ms(note.duration_ticks);
        uint32_t note_samples = (static_cast<uint64_t>(AUDIO_SAMPLE_RATE) * duration_ms) / 1000;
        int period = std::max(1, AUDIO_SAMPLE_RATE / note.frequency_hz);
        int half_period = std::max(1, period / 2);

        for (uint32_t i = 0; i < note_samples; ++i) {
            if (current_sample < total_samples) {
                int phase = static_cast<int>((current_sample) % period);
                sample_buffer[current_sample] = (phase < half_period) ? amplitude : -amplitude;
                current_sample++;
            }
        }
    }

    // Wrap in Mix_Chunk structure
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
        loaded_sound.total_duration_ms = 0;
        loaded_sound.priority = 0;
    }
}

/**
 * Get the appropriate sound sequence for a game sound
 */
static const std::vector<FrequencyNote>* get_sound_sequence(GameSound sound) {
    switch (sound) {
        case GameSound::UNUSED_0:
            return nullptr;  // No jump sound in original game
        case GameSound::FIRE:
            return &SOUND_FIRE_SEQUENCE;
        case GameSound::ITEM_COLLECT:
            return &SOUND_ITEM_COLLECT_SEQUENCE;
        case GameSound::DOOR_OPEN:
            return &SOUND_DOOR_OPEN_SEQUENCE;
        case GameSound::STAGE_TRANSITION:
            return &SOUND_STAGE_TRANSITION_SEQUENCE;
        case GameSound::ENEMY_HIT:
            return &SOUND_ENEMY_HIT_SEQUENCE;
        case GameSound::PLAYER_HIT:
            return &SOUND_PLAYER_HIT_SEQUENCE;
        case GameSound::PLAYER_DIE:
            return &SOUND_PLAYER_DIE_SEQUENCE;
        case GameSound::GAME_OVER:
            return &SOUND_GAME_OVER_SEQUENCE;
        case GameSound::TELEPORT:
            return &SOUND_TELEPORT_SEQUENCE;
        default:
            return nullptr;
    }
}

/**
 * Calculate total duration for a sound sequence in milliseconds
 */
static uint16_t calculate_sequence_duration_ms(const std::vector<FrequencyNote>& sequence) {
    uint16_t total_ms = 0;
    for (const auto& note : sequence) {
        total_ms += ticks_to_ms(note.duration_ticks);
    }
    return total_ms;
}

} // namespace

bool initialize_audio_system() {
    if (g_audio_initialized) {
        return true;
    }

    // Ensure SDL audio subsystem is initialized; SDL_mixer requires this.
    if ((SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            std::cerr << "Failed to init SDL audio subsystem: " << SDL_GetError() << std::endl;
            return false;
        }
        // track that we initialized it so shutdown can clean up
        g_sdl_audio_initialized = true;
    }

    if (Mix_OpenAudio(AUDIO_SAMPLE_RATE, AUDIO_S16SYS, AUDIO_CHANNELS, AUDIO_CHUNK_SIZE) < 0) {
        std::cerr << "Failed to initialize SDL_mixer audio: " << Mix_GetError() << std::endl;
        // if we initialized the subsystem just above, undo it so callers can retry
        if (g_sdl_audio_initialized) {
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
            g_sdl_audio_initialized = false;
        }
        return false;
    }

    Mix_AllocateChannels(8);

    // Load all game sounds (skip UNUSED_0 which has no sequence)
    for (size_t index = 0; index < SOUND_PRIORITIES.size(); ++index) {
        GameSound sound = static_cast<GameSound>(index);
        const std::vector<FrequencyNote>* sequence = get_sound_sequence(sound);
        
        // Skip unused sound slots
        if (!sequence) {
            continue;
        }
        
        if (sequence->empty()) {
            std::cerr << "Error: Sound #" << index << " has empty sequence" << std::endl;
            free_loaded_sounds();
            Mix_CloseAudio();
            if (g_sdl_audio_initialized) {
                SDL_QuitSubSystem(SDL_INIT_AUDIO);
                g_sdl_audio_initialized = false;
            }
            return false;
        }

        Mix_Chunk* chunk = create_sound_sequence_chunk(*sequence);
        if (!chunk) {
            std::cerr << "Failed to synthesize sound #" << index << std::endl;
            free_loaded_sounds();
            Mix_CloseAudio();
            if (g_sdl_audio_initialized) {
                SDL_QuitSubSystem(SDL_INIT_AUDIO);
                g_sdl_audio_initialized = false;
            }
            return false;
        }

        g_sounds[index].chunk = chunk;
        g_sounds[index].total_duration_ms = calculate_sequence_duration_ms(*sequence);
        g_sounds[index].priority = SOUND_PRIORITIES[index];
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

    if (g_sdl_audio_initialized) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        g_sdl_audio_initialized = false;
    }
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
    g_current_sound_end_tick = now + requested_sound.total_duration_ms;
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
