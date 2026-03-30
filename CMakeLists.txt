cmake_minimum_required(VERSION 3.10)
project(breakout_week2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

find_library(RAYLIB_LIBRARY raylib REQUIRED)

add_executable(breakout_week2
    main.cpp
    Game.cpp
    Ball.cpp
    Paddle.cpp
    Brick.cpp
    PowerUp.cpp
)

set_target_properties(breakout_week2 PROPERTIES OUTPUT_NAME game)

target_link_libraries(breakout_week2
    PRIVATE
    ${RAYLIB_LIBRARY}
    GL
    m
    pthread
    dl
    rt
    X11
)

add_executable(collision_test
    tests/collision_test.cpp
)

target_link_libraries(collision_test
    PRIVATE
    ${RAYLIB_LIBRARY}
    GL
    m
    pthread
    dl
    rt
    X11
)

add_test(NAME collision_test COMMAND collision_test)
