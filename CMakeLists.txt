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

add_executable(codename_hover src/main.cpp)
target_link_libraries(codename_hover PRIVATE SDL3::SDL3)

if(MSVC)
    target_compile_options(codename_hover PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(codename_hover PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

include(cmake/HoverShaders.cmake)

hover_add_shader(
    NAME triangle_vertex
    SOURCE shaders/triangle.vert.hlsl
    STAGE vertex
)

hover_add_shader(
    NAME triangle_fragment
    SOURCE shaders/triangle.frag.hlsl
    STAGE fragment
)

add_dependencies(codename_hover hover_shaders)
