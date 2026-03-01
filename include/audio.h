#ifndef AUDIO_H
#define AUDIO_H

#include <cstdint>

enum class GameSound : uint8_t {
    UNUSED_0 = 0,       // Reserved (no jump sound in original game)
    FIRE,               // Fireball launch
    ITEM_COLLECT,       // Item pickup (includes treasures and power-ups)
    DOOR_OPEN,          // Door opening
    STAGE_TRANSITION,   // Stage edge crossing
    ENEMY_HIT,          // Fireball hits enemy
    PLAYER_HIT,         // Player takes damage
    PLAYER_DIE,         // Player death
    GAME_OVER,          // Game over jingle
    TELEPORT,           // Teleport action
    COUNT
};

/**
 * Initialize the audio subsystem
 * 
 * Sets up SDL_mixer and synthesizes all game sound effects.
 * Call once at startup before calling play_game_sound().
 * 
 * @return true if initialization successful, false on error
 */
bool initialize_audio_system();

/**
 * Shutdown the audio subsystem
 * 
 * Stops any playing sounds and frees all audio resources.
 * Call once at program shutdown.
 */
void shutdown_audio_system();

/**
 * Check if audio system is ready
 * 
 * @return true if initialized and ready to play sounds
 */
bool is_audio_system_ready();

/**
 * Play a game sound effect
 * 
 * Plays the specified sound effect. If a lower-priority sound is
 * already playing, it will be interrupted. If a higher-priority
 * sound is playing, the new sound will be ignored.
 * 
 * @param sound The game sound to play
 * @return true if sound was played, false if rejected or error
 */
bool play_game_sound(GameSound sound);

#endif // AUDIO_H
