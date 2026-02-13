/**
 * level_loader.cpp - Runtime level data initialization
 * 
 * Loads tile data from binary PT files into level structures.
 * This bridges the gap between code-based level metadata and binary tile data,
 * with a path toward full code-based data migration.
 */

#include "level_loader.h"
#include "file_loader.h"
#include <cstring>
#include <iostream>
#include <map>

/* Runtime mutable copies of level data */
static level_t runtime_levels[8];
static bool levels_initialized = false;
static std::string level_asset_path = "original/";

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

void set_level_asset_path(const std::string& path) {
    level_asset_path = path;
    if (!level_asset_path.empty() && level_asset_path.back() != '/') {
        level_asset_path += '/';
    }
    set_asset_path(level_asset_path);
}

bool initialize_level_data() {
    if (levels_initialized) {
        return true;
    }
    
    /* Copy const level data into runtime structures */
    bool has_errors = false;
    for (int level_num = 0; level_num < 8; level_num++) {
        /* Copy const level data into runtime mutable copy */
        std::memcpy(&runtime_levels[level_num], source_levels[level_num], 
                    sizeof(level_t) - (128 * 10 * 3)); /* Don't copy tiles yet */
        
        /* Load tile data for each of the 3 stages */
        for (int stage_num = 0; stage_num < 3; stage_num++) {
            std::string filename = std::string(level_names[level_num]);
            filename += std::to_string(stage_num) + ".PT";
            
            pt_file_t pt_file;
            if (!load_pt_file(filename, pt_file)) {
                std::cerr << "Warning: Failed to load tile data for " 
                          << level_names[level_num] << " stage " << stage_num << std::endl;
                has_errors = true;
                /* Initialize with empty tiles */
                std::memset(runtime_levels[level_num].stages[stage_num].tiles, 0, 128 * 10);
            } else {
                /* Copy tile data into stage */
                std::memcpy(runtime_levels[level_num].stages[stage_num].tiles,
                           pt_file.tiles, 128 * 10);
            }
        }
    }
    
    levels_initialized = true;
    return !has_errors;
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
