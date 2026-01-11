#include <iostream>
#include <SDL2/SDL.h>
#include "game/GameState.h"
#include "input/Input.h"

int main() {
    // 1) Item collection: lantern
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        auto lvl = std::make_unique<Level>();
        lvl->stages.resize(1);
        lvl->stage_items.resize(1);

        Item it{};
        it.x = 0; it.y = 0; it.type = GameConstants::ITEM_LANTERN; it.collected = false;
        lvl->stage_items[0].push_back(it);

        g.current_level = std::move(lvl);
        g.current_stage_number = 0;
        g.comic_x = 0; g.comic_y = 0;

        Input input;
        g.update(input);

        if (!g.comic_has_lantern) { std::cerr << "Item collection test: lantern not collected\n"; return 1; }
        if (!g.current_level->stage_items[0][0].collected) { std::cerr << "Item collection test: item flag not set\n"; return 1; }
    }

    // 1b) Item collection: corkscrew
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        auto lvl = std::make_unique<Level>();
        lvl->stages.resize(1);
        lvl->stage_items.resize(1);

        Item it{};
        it.x = 0; it.y = 0; it.type = GameConstants::ITEM_CORKSCREW; it.collected = false;
        lvl->stage_items[0].push_back(it);

        g.current_level = std::move(lvl);
        g.current_stage_number = 0;
        g.comic_x = 0; g.comic_y = 0;

        Input input;
        g.update(input);

        if (!g.comic_has_corkscrew) { std::cerr << "Item collection test: corkscrew not collected\n"; return 1; }
    }

    // 1c) Item collection: Blastola Cola (firepower)
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        auto lvl = std::make_unique<Level>();
        lvl->stages.resize(1);
        lvl->stage_items.resize(1);

        Item it{};
        it.x = 0; it.y = 0; it.type = GameConstants::ITEM_BLASTOLA_COLA; it.collected = false;
        lvl->stage_items[0].push_back(it);

        g.current_level = std::move(lvl);
        g.current_stage_number = 0;
        g.comic_x = 0; g.comic_y = 0;

        Input input;
        g.update(input);

        if (g.comic_firepower == 0) { std::cerr << "Item collection test: blastola cola did not increment firepower\n"; return 1; }
    }

    // 1d) Item collection: Shield
    {
        // Case: not full HP => should restore to MAX
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        auto lvl = std::make_unique<Level>();
        lvl->stages.resize(1);
        lvl->stage_items.resize(1);

        Item it{};
        it.x = 0; it.y = 0; it.type = GameConstants::ITEM_SHIELD; it.collected = false;
        lvl->stage_items[0].push_back(it);

        g.current_level = std::move(lvl);
        g.current_stage_number = 0;
        g.comic_x = 0; g.comic_y = 0;
        g.comic_hp = 1;

        Input input;
        g.update(input);

        if (g.comic_hp != GameConstants::MAX_HP) { std::cerr << "Shield test: did not restore HP to max\n"; return 1; }

        // Case: full HP => should award a life (if available)
        GameState g2;
        g2.current_map = std::make_unique<TileMap>();
        auto lvl2 = std::make_unique<Level>();
        lvl2->stages.resize(1);
        lvl2->stage_items.resize(1);
        Item it2{};
        it2.x = 0; it2.y = 0; it2.type = GameConstants::ITEM_SHIELD; it2.collected = false;
        lvl2->stage_items[0].push_back(it2);
        g2.current_level = std::move(lvl2);
        g2.current_stage_number = 0;
        g2.comic_x = 0; g2.comic_y = 0;
        g2.comic_hp = GameConstants::MAX_HP;
        g2.comic_num_lives = 1;

        Input input2;
        g2.update(input2);

        if (g2.comic_num_lives == 1) { std::cerr << "Shield test: did not award life when HP full\n"; return 1; }
    }

    // 1e) Item collection: door key
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        auto lvl = std::make_unique<Level>();
        lvl->stages.resize(1);
        lvl->stage_items.resize(1);

        Item it{};
        it.x = 0; it.y = 0; it.type = GameConstants::ITEM_DOOR_KEY; it.collected = false;
        lvl->stage_items[0].push_back(it);

        g.current_level = std::move(lvl);
        g.current_stage_number = 0;
        g.comic_x = 0; g.comic_y = 0;

        Input input;
        g.update(input);

        if (!g.comic_has_door_key) { std::cerr << "Item collection test: door key not collected\n"; return 1; }
    }

    // 2) Door mechanics: entering a door should change level
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        auto lvl = std::make_unique<Level>();
        lvl->stages.resize(1);
        lvl->stage_doors.resize(1);

        Door d{};
        d.x = 0; d.y = 0; d.destination_level = static_cast<uint8_t>(GameConstants::LEVEL_NUMBER_SPACE); d.destination_stage = 0;
        lvl->stage_doors[0].push_back(d);

        g.current_level = std::move(lvl);
        g.current_stage_number = 0;
        g.comic_x = 0; g.comic_y = 0;
        g.dataPath = std::filesystem::path("."); // safe placeholder

        Input input;
        g.update(input);

        if (g.current_level_number != GameConstants::LEVEL_NUMBER_SPACE) { std::cerr << "Door test: level not changed on door overlap\n"; return 1; }
        if (g.current_stage_number != 0) { std::cerr << "Door test: destination stage not set correctly\n"; return 1; }
    }

    // 3) Loading items from files: ensure parser reads sidecar files
    {
        GameState g;
        std::filesystem::path dpath = std::filesystem::path("reference") / "assets";
        // If the tests are run from the build directory (ctest), the relative path
        // may not point to the repository source tree. Prefer using the compile-time
        // PROJECT_SOURCE_DIR if available (set in CMake), otherwise fall back to
        // deriving the source path from __FILE__.
#ifndef PROJECT_SOURCE_DIR
        auto src_root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
#else
        auto src_root = std::filesystem::path(PROJECT_SOURCE_DIR);
#endif
        if (!std::filesystem::exists(dpath)) {
            dpath = src_root / "reference" / "assets";
        }
        std::vector<Item> items;
        std::vector<Door> doors;
        std::filesystem::path p = dpath / (std::string("forest0.pt") + std::string(".items"));
        GameState::loadStageSidecarFile(p, items, doors);
        if (items.empty()) { std::cerr << "Expected items parsed from file: " << p << "\n"; return 1; }
        if (doors.empty()) { std::cerr << "Expected doors parsed from file: " << p << "\n"; return 1; }
    }

    // 4) Ensure loadLevel finds sidecar files in reference/levels even when dataPath doesn't include them
    {
        GameState g;
        // Use an empty/nonexistent dataPath so loader must fall back to reference/levels
        g.dataPath = std::filesystem::path("/nonexistent-data-path");
        bool ok = g.loadLevel(GameConstants::LEVEL_NUMBER_FOREST, g.dataPath, 0);
        if (!ok) { std::cerr << "loadLevel failed for forest (expected success)\n"; return 1; }
        if (g.current_level->stage_items.size() <= 0) { std::cerr << "No stages found in loaded level\n"; return 1; }
        if (g.current_level->stage_items[0].empty()) { std::cerr << "Expected items loaded from reference/levels for stage 0\n"; return 1; }
    }

    std::cout << "Item and door mechanic tests passed" << std::endl;
    return 0;
}
