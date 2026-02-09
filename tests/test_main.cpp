#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "../include/physics.h"
#include "../include/graphics.h"

// Provide required globals from main.cpp for physics.cpp linkage.
int comic_x = 0;
int comic_y = 0;
int8_t comic_y_vel = 0;
int8_t comic_x_momentum = 0;
uint8_t comic_facing = 1;
uint8_t comic_animation = 0;
uint8_t comic_is_falling_or_jumping = 0;
uint8_t comic_jump_counter = 0;
uint8_t comic_jump_power = JUMP_POWER_DEFAULT;
uint8_t key_state_jump = 0;
uint8_t previous_key_state_jump = 0;
uint8_t key_state_left = 0;
uint8_t key_state_right = 0;
int camera_x = 0;

static int failures = 0;

static void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << std::endl;
        failures++;
    }
}

static Animation make_animation(const std::vector<int>& durations, bool looping) {
    Animation anim;
    anim.looping = looping;
    anim.frame_start_time = 0;
    anim.current_frame = 0;
    anim.frames.clear();
    for (int duration : durations) {
        AnimationFrame frame;
        frame.duration_ms = duration;
        anim.frames.push_back(frame);
    }
    return anim;
}

static void test_physics_tiles() {
    init_test_level();

    uint8_t ground_tile = get_tile_at(0, 18); // tile_y = 9 (ground row)
    check(ground_tile == 0x3F, "ground tile should be 0x3F");

    uint8_t out_of_bounds = get_tile_at(255, 30); // tile_y = 15 (out of bounds)
    check(out_of_bounds == 0, "out-of-bounds tile should be 0");

    check(is_tile_solid(0x3F), "tile 0x3F should be solid");
    check(!is_tile_solid(0x3E), "tile 0x3E should be passable");
}

static void test_animation_looping() {
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

static void test_animation_non_looping() {
    GraphicsSystem graphics(nullptr);
    Animation anim = make_animation({100, 200, 100}, false);

    graphics.update_animation(anim, 50);
    check(anim.current_frame == 0, "non-looping: frame at 50ms should be 0");

    graphics.update_animation(anim, 150);
    check(anim.current_frame == 1, "non-looping: frame at 150ms should be 1");

    graphics.update_animation(anim, 450);
    check(anim.current_frame == 2, "non-looping: frame at 450ms should clamp to 2");
}

static void test_animation_zero_duration() {
    GraphicsSystem graphics(nullptr);
    Animation anim = make_animation({0, 0}, true);

    graphics.update_animation(anim, 1);
    check(anim.current_frame >= 0 && anim.current_frame < static_cast<int>(anim.frames.size()),
          "zero duration: frame index should stay in range");
}

static void reset_physics_state() {
    init_test_level();
    comic_x = 4;
    comic_y = 14;
    comic_y_vel = 0;
    comic_x_momentum = 0;
    comic_facing = 1;
    comic_animation = 0;
    comic_is_falling_or_jumping = 0;
    comic_jump_power = JUMP_POWER_DEFAULT;
    comic_jump_counter = comic_jump_power;
    key_state_jump = 0;
    previous_key_state_jump = 0;
    key_state_left = 0;
    key_state_right = 0;
    camera_x = 0;
}

static void simulate_tick() {
    if (comic_is_falling_or_jumping == 0 &&
        key_state_jump && !previous_key_state_jump &&
        comic_jump_counter == comic_jump_power) {
        comic_is_falling_or_jumping = 1;
    }

    previous_key_state_jump = key_state_jump;
    handle_fall_or_jump();
}

static int measure_jump_height(uint8_t jump_power) {
    reset_physics_state();
    comic_jump_power = jump_power;
    comic_jump_counter = comic_jump_power;

    int start_y = comic_y;
    int min_y = comic_y;

    key_state_jump = 1;
    simulate_tick();

    for (int tick = 0; tick < 200; ++tick) {
        simulate_tick();
        min_y = std::min(min_y, comic_y);
        if (comic_is_falling_or_jumping == 0 && tick > 0) {
            break;
        }
    }

    return start_y - min_y;
}

static void test_jump_edge_trigger() {
    reset_physics_state();

    key_state_jump = 1;
    simulate_tick();
    check(comic_is_falling_or_jumping == 1, "jump should start on key press edge");

    comic_is_falling_or_jumping = 0;
    comic_y = 14;
    comic_y_vel = 0;
    key_state_jump = 1;
    simulate_tick();
    check(comic_is_falling_or_jumping == 0, "holding jump should not retrigger");
}

static void test_jump_recharge() {
    reset_physics_state();

    comic_jump_counter = 1;
    key_state_jump = 0;
    simulate_tick();
    check(comic_jump_counter == comic_jump_power, "jump counter should recharge on release");
}

static void test_jump_height() {
    int default_height = measure_jump_height(JUMP_POWER_DEFAULT);
    int boots_height = measure_jump_height(JUMP_POWER_WITH_BOOTS);

    check(default_height == 7,
          "default jump height should be 7 units (got " + std::to_string(default_height) + ")");
    check(boots_height == 9,
          "boots jump height should be 9 units (got " + std::to_string(boots_height) + ")");
}

struct TestCase {
    const char* name;
    void (*run)();
};

static const std::vector<TestCase>& test_registry() {
    static const std::vector<TestCase> tests = {
        {"physics_tiles", test_physics_tiles},
        {"animation_looping", test_animation_looping},
        {"animation_non_looping", test_animation_non_looping},
        {"animation_zero_duration", test_animation_zero_duration},
        {"jump_edge_trigger", test_jump_edge_trigger},
        {"jump_recharge", test_jump_recharge},
        {"jump_height", test_jump_height}
    };
    return tests;
}

static bool matches_filter(const std::string& name, const std::string& filter) {
    if (filter.empty()) {
        return true;
    }
    return name.find(filter) != std::string::npos;
}

static int run_tests(const std::string& filter) {
    int tests_run = 0;
    for (const auto& test : test_registry()) {
        if (!matches_filter(test.name, filter)) {
            continue;
        }
        tests_run++;
        test.run();
    }

    if (tests_run == 0) {
        std::cerr << "No tests match filter: " << filter << std::endl;
        return 1;
    }

    if (failures == 0) {
        std::cout << "All tests passed." << std::endl;
        return 0;
    }

    std::cerr << failures << " test(s) failed." << std::endl;
    return 1;
}

static void print_usage(const char* argv0) {
    std::cout << "Usage: " << argv0 << " [--list] [--filter NAME]" << std::endl;
}

int main(int argc, char** argv) {
    std::string filter;
    bool list_only = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--list") {
            list_only = true;
        } else if (arg == "--filter" && i + 1 < argc) {
            filter = argv[++i];
        } else if (arg.rfind("--filter=", 0) == 0) {
            filter = arg.substr(std::string("--filter=").size());
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    if (list_only) {
        for (const auto& test : test_registry()) {
            std::cout << test.name << std::endl;
        }
        return 0;
    }

    return run_tests(filter);
}
