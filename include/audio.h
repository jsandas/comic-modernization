#ifndef AUDIO_H
#define AUDIO_H

#include <cstdint>

enum class GameSound : uint8_t {
    JUMP = 0,
    FIRE,
    ITEM_COLLECT,
    DOOR_OPEN,
    STAGE_TRANSITION,
    ENEMY_HIT,
    PLAYER_HIT,
    PLAYER_DIE,
    POWERUP,
    TREASURE,
    TELEPORT,
    SHIELD,
    VICTORY,
    COUNT
};

bool initialize_audio_system();
void shutdown_audio_system();
bool is_audio_system_ready();
bool play_game_sound(GameSound sound);

#endif // AUDIO_H
