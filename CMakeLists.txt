cmake_minimum_required(VERSION 3.16)
project(CaptainComic)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(captain_comic 
    src/main.cpp
    src/physics.cpp
)

target_link_libraries(captain_comic ${SDL2_LIBRARIES})