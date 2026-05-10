#include "test_helpers.h"
#include "test_cases.h"
#include <filesystem>
#include <fstream>

void test_animation_looping() {
    reset_physics_state();
    GraphicsSystem graphics(nullptr);
    Animation anim = make_animation({100, 200, 100}, true);

    graphics.update_animation(anim, 50);
    check(anim.current_frame == 0, "looping: frame at 50ms should be 0");

    graphics.update_animation(anim, 150);
    check(anim.current_frame == 1, "looping: frame at 150ms should be 1");

    graphics.update_animation(anim, 310);
    check(anim.current_frame == 2, "looping: frame at 310ms should be 2");

    graphics.update_animation(anim, 450);
    check(anim.current_frame == 0, "looping: frame at 450ms should loop to 0");
}

void test_animation_non_looping() {
    reset_physics_state();
    GraphicsSystem graphics(nullptr);
    Animation anim = make_animation({100, 200, 100}, false);

    graphics.update_animation(anim, 50);
    check(anim.current_frame == 0, "non-looping: frame at 50ms should be 0");

    graphics.update_animation(anim, 150);
    check(anim.current_frame == 1, "non-looping: frame at 150ms should be 1");

    graphics.update_animation(anim, 450);
    check(anim.current_frame == 2, "non-looping: frame at 450ms should clamp to 2");
}

void test_animation_zero_duration() {
    reset_physics_state();
    GraphicsSystem graphics(nullptr);
    Animation anim = make_animation({0, 0}, true);

    graphics.update_animation(anim, 1);
    check(anim.current_frame >= 0 && anim.current_frame < static_cast<int>(anim.frames.size()),
          "zero duration: frame index should stay in range");
}

void test_enemy_animation_sequence() {
    reset_physics_state();
    std::vector<uint8_t> loop_sequence = build_enemy_animation_sequence(3, ENEMY_ANIMATION_LOOP);
    check(loop_sequence == std::vector<uint8_t>({0, 1, 2}),
        "enemy loop sequence should be 0,1,2 for 3 frames");

    std::vector<uint8_t> alternate_sequence = build_enemy_animation_sequence(3, ENEMY_ANIMATION_ALTERNATE);
    check(alternate_sequence == std::vector<uint8_t>({0, 1, 2, 1}),
        "enemy alternate sequence should be 0,1,2,1 for 3 frames");

    std::vector<uint8_t> alternate_sequence_four = build_enemy_animation_sequence(4, ENEMY_ANIMATION_ALTERNATE);
    check(alternate_sequence_four == std::vector<uint8_t>({0, 1, 2, 3, 2, 1}),
        "enemy alternate sequence should be 0,1,2,3,2,1 for 4 frames");

    std::vector<uint8_t> empty_sequence = build_enemy_animation_sequence(0, ENEMY_ANIMATION_LOOP);
    check(empty_sequence.empty(), "enemy sequence should be empty for 0 frames");
}

void test_asset_path_resolution() {
    reset_physics_state();
    namespace fs = std::filesystem;
    // create a temporary directory tree mimicking the asset layout
    fs::path base = fs::temp_directory_path() / "comic_assets_test";
    fs::remove_all(base);
    fs::create_directories(base / "assets" / "sprites");
    fs::create_directories(base / "assets" / "sounds");
    fs::create_directories(base / "assets" / "maps");
    fs::create_directories(base / "assets" / "tiles");
    fs::create_directories(base / "assets" / "shp");
    fs::create_directories(base / "assets" / "graphics");

    // touch one file of each type
    std::ofstream((base / "assets" / "sprites" / "sprite-test.png").string()).close();
    std::ofstream((base / "assets" / "sounds" / "sound-test.wav").string()).close();
    std::ofstream((base / "assets" / "maps" / "foo.pt.png").string()).close();
    std::ofstream((base / "assets" / "tiles" / "foo.tt2-00.png").string()).close();
    std::ofstream((base / "assets" / "shp" / "foo.shp").string()).close();
    std::ofstream((base / "assets" / "graphics" / "sys000.ega").string()).close();

    // remember cwd so we can restore later
    fs::path orig_cwd = fs::current_path();
    fs::current_path(base);

    GraphicsSystem g(nullptr);
    std::string p;
    p = g.get_asset_path("sprite-test.png");
    check(p.find("assets/sprites/sprite-test.png") != std::string::npos,
          "get_asset_path should find sprite in sprites subdir");
    p = g.get_asset_path("sound-test.wav");
    check(p.find("assets/sounds/sound-test.wav") != std::string::npos,
          "get_asset_path should find sound in sounds subdir");
    p = g.get_asset_path("foo.pt.png");
    check(p.find("assets/maps/foo.pt.png") != std::string::npos,
          "get_asset_path should find map in maps subdir");
    p = g.get_asset_path("foo.tt2-00.png");
    check(p.find("assets/tiles/foo.tt2-00.png") != std::string::npos,
          "get_asset_path should find tileset in tiles subdir");
    p = g.get_asset_path("foo.shp");
    check(p.find("assets/shp/foo.shp") != std::string::npos,
          "get_asset_path should find shp in shp subdir");
    p = g.get_asset_path("sys000.ega");
    check(p.find("assets/graphics/sys000.ega") != std::string::npos,
          "get_asset_path should find raw graphics in graphics subdir");

    // cleanup
    fs::current_path(orig_cwd);
    fs::remove_all(base);
}

void test_runtime_level_tiles_populated() {
    reset_physics_state();
    initialize_level_data();
    current_level_number = LEVEL_NUMBER_FOREST;
    current_stage_number = 0;
    source_door_level_number = -1;
    source_door_stage_number = -1;
    current_level_ptr = nullptr;

    load_new_level();
    check(current_level_ptr != nullptr, "current_level_ptr should be set after load_new_level");

    bool any_non_zero = false;
    const uint8_t* tiles = current_level_ptr->stages[current_stage_number].tiles;
    for (int i = 0; i < 128 * 10; ++i) {
        if (tiles[i] != 0) {
            any_non_zero = true;
            break;
        }
    }

    check(any_non_zero, "current level tiles should be populated (non-zero)");
    reset_door_state();
}
