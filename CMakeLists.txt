cmake_minimum_required(VERSION 3.14)
project(breakout_week2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG v1.3.18
)
FetchContent_MakeAvailable(enet)

if(TARGET enet_static)
    set(ENET_TARGET enet_static)
elseif(TARGET enet)
    set(ENET_TARGET enet)
else()
    message(FATAL_ERROR "ENet target not found")
endif()

find_package(raylib 5.0 QUIET)
if(NOT raylib_FOUND)
    find_library(RAYLIB_LIBRARY raylib QUIET)
endif()

if(NOT raylib_FOUND AND NOT RAYLIB_LIBRARY)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.0
        GIT_SHALLOW TRUE
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
    set(RAYLIB_TARGET raylib)
elseif(raylib_FOUND)
    set(RAYLIB_TARGET raylib)
else()
    set(RAYLIB_TARGET ${RAYLIB_LIBRARY})
endif()

add_executable(breakout_week2
    src/main.cpp
    src/Game.cpp
    src/Ball.cpp
    src/Paddle.cpp
    src/Brick.cpp
    src/PowerUp.cpp
    src/PowerUpEffect.cpp
    src/Networking.cpp
)

target_include_directories(breakout_week2
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${enet_SOURCE_DIR}/include
)

set_target_properties(breakout_week2 PROPERTIES OUTPUT_NAME game)

target_link_libraries(breakout_week2
    PRIVATE
    ${RAYLIB_TARGET}
    nlohmann_json::nlohmann_json
    ${ENET_TARGET}
)

if(WIN32)
    target_link_libraries(breakout_week2 PRIVATE winmm ws2_32 opengl32 gdi32 user32 shell32)
elseif(APPLE)
    target_link_libraries(breakout_week2 PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
else()
    target_link_libraries(breakout_week2 PRIVATE GL m pthread dl rt X11)
endif()

add_executable(collision_test
    tests/collision_test.cpp
    src/Ball.cpp
    src/Brick.cpp
)

target_include_directories(collision_test
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(collision_test
    PRIVATE
    ${RAYLIB_TARGET}
)

if(WIN32)
    target_link_libraries(collision_test PRIVATE winmm opengl32 gdi32 user32 shell32)
elseif(APPLE)
    target_link_libraries(collision_test PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
else()
    target_link_libraries(collision_test PRIVATE GL m pthread dl rt X11)
endif()

add_test(NAME collision_test COMMAND collision_test)
