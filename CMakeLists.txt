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

add_executable(codename_hover src/main.cpp)
target_link_libraries(codename_hover PRIVATE SDL3::SDL3 hover_math)
hover_enable_warnings(codename_hover)

include(CTest)
if(BUILD_TESTING)
    add_executable(hover_math_tests tests/math_tests.cpp)
    target_link_libraries(hover_math_tests PRIVATE hover_math)
    hover_enable_warnings(hover_math_tests)
    add_test(NAME hover_math_tests COMMAND hover_math_tests)
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

add_dependencies(codename_hover hover_shaders)
