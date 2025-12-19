include(FetchContent)

# Fetch Catch2 (single library with main)
FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(catch2)

# Add test executable
add_executable(collision_unit_tests
    collision_tests.cpp
    ../game/Collision.cpp
)

target_include_directories(collision_unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(collision_unit_tests PRIVATE Catch2::Catch2WithMain)

target_compile_definitions(collision_unit_tests PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

add_test(NAME collision_unit_tests COMMAND $<TARGET_FILE:collision_unit_tests>)

# AI / Fireball tests
add_executable(ai_fireball_unit_tests
    ai_fireball_tests.cpp
    ../game/EnemyLogic.cpp
    ../game/FireballLogic.cpp
)

target_include_directories(ai_fireball_unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(ai_fireball_unit_tests PRIVATE Catch2::Catch2WithMain)

target_compile_definitions(ai_fireball_unit_tests PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

add_test(NAME ai_fireball_unit_tests COMMAND $<TARGET_FILE:ai_fireball_unit_tests>)

# CI comparison helper: run both C++ and ASM harness under Unicorn and fail on mismatch
add_executable(asm_ci_compare
    asm_ci_compare.cpp
    ../game/Collision.cpp
    ../game/EnemyLogic.cpp
    ../game/FireballLogic.cpp
)

target_include_directories(asm_ci_compare PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_compile_definitions(asm_ci_compare PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

add_test(NAME asm_ci_compare COMMAND $<TARGET_FILE:asm_ci_compare>)
