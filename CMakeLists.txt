cmake_minimum_required(VERSION 3.28)

project(codename_hover VERSION 0.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL/CMakeLists.txt")
    message(FATAL_ERROR
        "SDL is missing. Run: git submodule update --init --recursive"
    )
endif()

# SDL is part of the game build, but its tests, examples, shared library, and
# install targets are not. Keep these cache values above add_subdirectory().
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(external/SDL EXCLUDE_FROM_ALL)

function(hover_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

add_library(hover_math STATIC src/hover_math.cpp)
target_include_directories(hover_math PUBLIC src)
hover_enable_warnings(hover_math)

add_library(hover_core STATIC
    src/core/fixed_step.cpp
    src/core/launch_options.cpp
)
target_include_directories(hover_core PUBLIC src)
hover_enable_warnings(hover_core)

add_library(hover_input STATIC
    src/input/player_input.cpp
    src/platform/sdl_input.cpp
)
target_include_directories(hover_input PUBLIC src)
target_link_libraries(hover_input PUBLIC SDL3::SDL3)
hover_enable_warnings(hover_input)

add_library(hover_render STATIC src/render/gpu_mesh.cpp)
target_sources(hover_render PRIVATE src/render/engine_pulse.cpp src/render/vehicle_presentation.cpp)
target_include_directories(hover_render PUBLIC src)
target_link_libraries(hover_render PUBLIC SDL3::SDL3 hover_math)
hover_enable_warnings(hover_render)

add_library(hover_assets STATIC
    src/assets/generated/engine_pulse_mesh.cpp
    src/assets/generated/mesh_builder.cpp
    src/assets/generated/presentation_pad.cpp
    src/assets/generated/prototype_01_mesh.cpp
    src/assets/generated/track_surface_mesh.cpp
)
target_include_directories(hover_assets PUBLIC src)
target_link_libraries(hover_assets PUBLIC hover_math)
hover_enable_warnings(hover_assets)

add_library(hover_game STATIC
    src/game/track.cpp
    src/game/track_vehicle_state.cpp
    src/game/ship_definition.cpp
    src/game/ships/prototype_01.cpp
    src/game/tracks/oval_track.cpp
    src/game/vehicle_simulation.cpp
)
target_include_directories(hover_game PUBLIC src)
target_link_libraries(hover_game PUBLIC hover_input hover_math)
hover_enable_warnings(hover_game)
target_link_libraries(hover_assets PUBLIC hover_game)

add_executable(codename_hover src/main.cpp)
target_link_libraries(codename_hover PRIVATE
    SDL3::SDL3
    hover_assets
    hover_core
    hover_game
    hover_input
    hover_render
)
hover_enable_warnings(codename_hover)

include(CTest)
if(BUILD_TESTING)
    add_executable(hover_math_tests tests/math_tests.cpp)
    target_link_libraries(hover_math_tests PRIVATE hover_math)
    hover_enable_warnings(hover_math_tests)
    add_test(NAME hover_math_tests COMMAND hover_math_tests)

    add_executable(hover_ship_tests tests/ship_tests.cpp)
    target_link_libraries(hover_ship_tests PRIVATE hover_assets hover_game)
    hover_enable_warnings(hover_ship_tests)
    add_test(NAME hover_ship_tests COMMAND hover_ship_tests)

    add_executable(hover_input_tests tests/input_tests.cpp)
    target_link_libraries(hover_input_tests PRIVATE hover_input)
    hover_enable_warnings(hover_input_tests)
    add_test(NAME hover_input_tests COMMAND hover_input_tests)

    add_executable(hover_simulation_tests tests/simulation_tests.cpp)
    target_link_libraries(hover_simulation_tests PRIVATE hover_core hover_game)
    hover_enable_warnings(hover_simulation_tests)
    add_test(NAME hover_simulation_tests COMMAND hover_simulation_tests)

    add_executable(hover_launch_options_tests tests/launch_options_tests.cpp)
    target_link_libraries(hover_launch_options_tests PRIVATE hover_core)
    hover_enable_warnings(hover_launch_options_tests)
    add_test(NAME hover_launch_options_tests COMMAND hover_launch_options_tests)

    add_executable(hover_track_tests tests/track_tests.cpp)
    target_link_libraries(hover_track_tests PRIVATE hover_assets hover_game)
    hover_enable_warnings(hover_track_tests)
    add_test(NAME hover_track_tests COMMAND hover_track_tests)

    add_executable(hover_track_vehicle_state_tests tests/track_vehicle_state_tests.cpp)
    target_link_libraries(hover_track_vehicle_state_tests PRIVATE hover_game)
    hover_enable_warnings(hover_track_vehicle_state_tests)
    add_test(NAME hover_track_vehicle_state_tests COMMAND hover_track_vehicle_state_tests)

    add_executable(hover_engine_pulse_tests tests/engine_pulse_tests.cpp)
    target_link_libraries(hover_engine_pulse_tests PRIVATE hover_render)
    hover_enable_warnings(hover_engine_pulse_tests)
    add_test(NAME hover_engine_pulse_tests COMMAND hover_engine_pulse_tests)
endif()

include(cmake/HoverShaders.cmake)

hover_add_shader(
    NAME vehicle_vertex
    SOURCE shaders/vehicle.vert.hlsl
    STAGE vertex
)

hover_add_shader(
    NAME vehicle_fragment
    SOURCE shaders/vehicle.frag.hlsl
    STAGE fragment
)

hover_add_shader(
    NAME engine_pulse_fragment
    SOURCE shaders/engine_pulse.frag.hlsl
    STAGE fragment
)

add_dependencies(codename_hover hover_shaders)
