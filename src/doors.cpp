#include "../include/doors.h"
#include "../include/physics.h"
#include <cstdint>

/**
 * doors.cpp - Door system implementation
 * 
 * Handles door collision detection, key checking, and level/stage transitions.
 * Matches the original assembly behavior for door mechanics.
 * 
 * Reference: jsandas/comic-c src/doors.c
 */

/* External game state variables (defined in main.cpp) */
extern int comic_x;
extern int comic_y;
extern uint8_t comic_has_door_key;
extern uint8_t key_state_open;
extern uint8_t current_level_number;
extern uint8_t current_stage_number;
extern const level_t* current_level_ptr;

/* Source door tracking for proper reciprocal door positioning */
extern int8_t source_door_level_number;
extern int8_t source_door_stage_number;

/* External functions for level loading (defined in level_loader.cpp) */
extern void load_new_level();
extern void load_new_stage();

/**
 * check_door_activation - Check if Comic is in front of a door and open key is pressed
 * 
 * Scans all doors in the current stage looking for one that matches:
 * - Comic's Y coordinate (exact match)
 * - Comic's X coordinate (within 3 units)
 * - Player has the Door Key
 * 
 * If found, activates the door and returns 1. Otherwise returns 0.
 * 
 * Assembly reference: R5sw1991.asm:3952-3973
 */
uint8_t check_door_activation() {
    /* Validate preconditions */
    if (!current_level_ptr) {
        return 0;  /* No level loaded */
    }
    
    if (current_stage_number >= 3) {
        return 0;  /* Invalid stage number */
    }
    
    if (key_state_open != 1) {
        return 0;  /* Open key not pressed */
    }
    
    /* Get current stage data */
    const stage_t* current_stage = &current_level_ptr->stages[current_stage_number];
    
    /* Check all doors in the current stage */
    for (int i = 0; i < MAX_NUM_DOORS; i++) {
        const door_t* door = &current_stage->doors[i];
        
        /* Door is unused if x or y is 0xff (DOOR_UNUSED) */
        if (door->x == DOOR_UNUSED || door->y == DOOR_UNUSED) {
            continue;
        }
        
        /* Check Y coordinate: must be exact match
         * Both comic_y and door->y are in game units (same coordinate system).
         * Door activation only occurs when Comic's Y exactly equals the door's Y. */
        if (comic_y != door->y) {
            continue;
        }
        
        /* Check X coordinate: must be within 3 units
         * Both comic_x and door->x are in game units
         * Comic is 2 units wide, so allows adjacent positioning */
        int x_offset = comic_x - door->x;
        if (x_offset < 0 || x_offset > 2) {
            continue;
        }
        
        /* Check if player has the Door Key */
        if (comic_has_door_key != 1) {
            continue;  /* Door is locked, skip to next door */
        }
        
        /* All checks passed: activate this door */
        activate_door(door);
        return 1;  /* Door was activated */
    }
    
    /* No doors activated */
    return 0;
}

/**
 * activate_door - Perform door transition to target level/stage
 * 
 * Plays door entry animation, updates level/stage numbers, and loads the
 * new level or stage data as appropriate.
 * 
 * If the target is in a different level, calls load_new_level() which
 * loads tileset, enemy sprites, and all stage data for the new level.
 * 
 * If the target is in the same level, calls load_new_stage() which
 * only loads the new stage tiles, enemies, and items.
 * 
 * The source level/stage are saved so that the destination stage can
 * find the reciprocal door and position Comic there.
 * 
 * Assembly reference: R5sw1991.asm:4618-4643
 */
void activate_door(const door_t *door) {
    /* Validate door target values before proceeding */
    if (door->target_stage >= 3) {
        return;  /* Invalid target stage */
    }
    
    if (door->target_level >= 8) {
        return;  /* Invalid target level */
    }
    
    /* Save the source level and stage so the destination can find the
     * reciprocal door and position Comic at the matching door */
    source_door_level_number = current_level_number;
    source_door_stage_number = current_stage_number;
    
    /* Set the current level/stage to wherever the door leads */
    current_stage_number = door->target_stage;
    uint8_t target_level = door->target_level;
    
    /* If targeting a different level, load the entire level (tileset + stages)
     * Otherwise, just load the stage within the current level */
    if (target_level != source_door_level_number) {
        /* Different level: update level number and load new level */
        current_level_number = target_level;
        load_new_level();
    } else {
        /* Same level: only load the new stage (tiles, enemies, items) */
        current_level_number = target_level;
        load_new_stage();
    }
}
