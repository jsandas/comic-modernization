#ifndef LEVEL_LOADER_H
#define LEVEL_LOADER_H

#include "level.h"
#include <string>

/**
 * Initialize all level data by loading tile maps from PT files
 * 
 * This function populates the tile data for all stages from binary PT files.
 * Call this once at game startup before using any level data.
 * 
 * Returns true if successful, false if critical file loading fails
 */
bool initialize_level_data();

/**
 * Get runtime-initialized level data by name (case-insensitive)
 * Returns nullptr if level not found
 * 
 * The returned level_t contains fully loaded tile data for all stages.
 */
level_t* get_level_data(const std::string& level_name);

/**
 * Set the asset path for loading PT files
 * Default is "original/"
 */
void set_level_asset_path(const std::string& path);

/**
 * load_new_level - Load a new level's data and assets
 * 
 * Loads the tileset, enemy sprites, and initializes the first stage.
 * Called when a door leads to a different level.
 * 
 * TODO: Implement full level loading
 */
void load_new_level();

/**
 * load_new_stage - Load a new stage within the current level
 * 
 * Loads the stage tile map, enemies, and items.
 * Called for stage transitions within same level or at startup.
 * 
 * TODO: Implement stage loading
 */
void load_new_stage();

#endif /* LEVEL_LOADER_H */
