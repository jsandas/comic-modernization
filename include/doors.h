#ifndef DOORS_H
#define DOORS_H

#include <cstdint>
#include "level.h"

/**
 * doors.h - Door system for level transitions
 * 
 * Implements door collision detection, key requirement checking, and
 * level/stage transitions via doors.
 * 
 * Matches the original assembly behavior where doors are positioned on the map
 * and require the Door Key to open. Entering a door transitions to a new level
 * or stage.
 * 
 * Reference: jsandas/comic-c src/doors.c
 */

/**
 * check_door_activation - Check if Comic is in front of a door and the open key is pressed
 * 
 * When the player presses the open key:
 * 1. Iterate through all doors in the current stage
 * 2. For each door, check:
 *    - Comic's Y coordinate matches door Y coordinate exactly (both in game units)
 *    - Comic's X coordinate is within 3 units of door X (0, 1, or 2 units offset)
 * 3. If a door matches and the player has the Door Key, activate the door
 * 
 * Door positioning uses game units (same as Comic's position):
 * - X ranges from 0-256 (128 tiles * 2)
 * - Y ranges from 0-20 (10 tiles * 2)
 * 
 * Input:
 *   Requires external variables:
 *   - comic_x, comic_y: Player position in game units
 *   - key_state_open: 1 if open key is pressed, 0 otherwise
 *   - comic_has_door_key: 1 if player has door key, 0 otherwise
 *   - current_level_ptr, current_stage_number: Current level/stage
 * 
 * Output:
 *   Returns 1 if a door was activated, 0 otherwise
 *   If activated, may load new level/stage data
 */
uint8_t check_door_activation();

/**
 * activate_door - Perform door transition to target level/stage
 * 
 * Input:
 *   door: pointer to door descriptor with target level/stage
 * 
 * Marks the entry point as coming from a door, sets current level/stage
 * to the door's target, and loads the new level or stage as appropriate.
 * 
 * If the target is in a different level, loads entire level data.
 * If the target is in the same level, loads only the stage data.
 */
void activate_door(const door_t *door);

#endif /* DOORS_H */
