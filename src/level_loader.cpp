/**
 * level_loader.cpp - Runtime level data initialization
 * 
 * Initializes level data with pre-generated tile data from level_tiles.cpp.
 * Tile data is compiled directly into the executable as C++ hex arrays,
 * eliminating all runtime file I/O dependencies.
 */

#include "level_loader.h"
#include "level_tiles.h"
#include <cstring>
#include <iostream>

/* Runtime mutable copies of level data */
static level_t runtime_levels[8];
static bool levels_initialized = false;

/* Names corresponding to level numbers */
static const char* level_names[] = {
    "lake", "forest", "space", "base", "cave", "shed", "castle", "comp"
};

/* Pointers to source const data */
static const level_t* source_levels[8] = {
    &level_data_lake,
    &level_data_forest,
    &level_data_space,
    &level_data_base,
    &level_data_cave,
    &level_data_shed,
    &level_data_castle,
    &level_data_comp
};

/* Pre-generated tile data arrays (from level_tiles.cpp) */
static const uint8_t* tile_arrays[8][3] = {
    {lake_stage_0_tiles, lake_stage_1_tiles, lake_stage_2_tiles},
    {forest_stage_0_tiles, forest_stage_1_tiles, forest_stage_2_tiles},
    {space_stage_0_tiles, space_stage_1_tiles, space_stage_2_tiles},
    {base_stage_0_tiles, base_stage_1_tiles, base_stage_2_tiles},
    {cave_stage_0_tiles, cave_stage_1_tiles, cave_stage_2_tiles},
    {shed_stage_0_tiles, shed_stage_1_tiles, shed_stage_2_tiles},
    {castle_stage_0_tiles, castle_stage_1_tiles, castle_stage_2_tiles},
    {comp_stage_0_tiles, comp_stage_1_tiles, comp_stage_2_tiles}
};

void set_level_asset_path(const std::string& path) {
    /* No longer needed - all tile data is compiled-in */
    (void)path;
}

bool initialize_level_data() {
    if (levels_initialized) {
        return true;
    }
    
    /* Copy const level data into runtime structures and populate with tile data */
    for (int level_num = 0; level_num < 8; level_num++) {
        /* Copy const level data into runtime mutable copy */
        std::memcpy(&runtime_levels[level_num], source_levels[level_num], 
                    sizeof(level_t) - (128 * 10 * 3)); /* Don't copy tiles yet */
        
        /* Copy pre-generated tile data for each of the 3 stages */
        for (int stage_num = 0; stage_num < 3; stage_num++) {
            std::memcpy(runtime_levels[level_num].stages[stage_num].tiles,
                       tile_arrays[level_num][stage_num], 
                       128 * 10);
        }
    }
    
    levels_initialized = true;
    return true;
}

level_t* get_level_data(const std::string& level_name) {
    if (!levels_initialized) {
        std::cerr << "Error: Level data not initialized. Call initialize_level_data() first." << std::endl;
        return nullptr;
    }
    
    const level_t* result = get_level_by_name(level_name);
    if (!result) {
        std::cerr << "Error: Unknown level: " << level_name << std::endl;
        return nullptr;
    }
    
    /* Find which level this is */
    for (int i = 0; i < 8; i++) {
        if (source_levels[i] == result) {
            return &runtime_levels[i];
        }
    }
    
    return nullptr;
}
