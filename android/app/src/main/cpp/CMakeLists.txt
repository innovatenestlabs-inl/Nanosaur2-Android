cmake_minimum_required(VERSION 3.22.1)
project(Nanosaur2AndroidBootstrap LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../SDL")
if(NOT EXISTS "${SDL_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "SDL3 source missing. Run android/setup-sdl.ps1 first.")
endif()

# Build the same SDL3 revision that supplies Android's Java SDLActivity.
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${SDL_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/SDL" EXCLUDE_FROM_ALL)

add_library(main SHARED smoke_main.cpp)
find_library(GLES3_LIB GLESv3 REQUIRED)
find_library(LOG_LIB log REQUIRED)
target_link_libraries(main PRIVATE SDL3::SDL3 ${GLES3_LIB} ${LOG_LIB})
