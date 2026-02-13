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

#endif /* LEVEL_LOADER_H */
