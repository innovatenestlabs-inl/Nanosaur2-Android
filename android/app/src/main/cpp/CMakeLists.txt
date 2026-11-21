cmake_minimum_required(VERSION 3.22.1)
project(Nanosaur2AndroidBootstrap LANGUAGES C CXX)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
option(NANOSAUR_ENGINE_PROBE "Compile original Nanosaur game source without linking into the preview" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../SDL")
get_filename_component(GAME_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../../" ABSOLUTE)
if(NOT EXISTS "${SDL_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "SDL3 source missing. Run android/setup-sdl.ps1 first.")
endif()
if(NOT EXISTS "${GAME_ROOT}/Source/Headers/stb_image.h")
    message(FATAL_ERROR "Original Nanosaur 2 image decoder missing.")
endif()

set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${SDL_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/SDL" EXCLUDE_FROM_ALL)


# Compile the pinned Pomme filesystem/runtime core independently of the
# desktop OpenGL renderer. These flags are intentionally a *probe* rather
# than the game's final audio/graphics configuration.
get_filename_component(POMME_SOURCE_DIR "${GAME_ROOT}/extern/Pomme" ABSOLUTE)
if(NOT EXISTS "${POMME_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Pomme submodule missing. Run git submodule update --init --recursive")
endif()
add_compile_definitions(POMME_CASE_SENSITIVE_FSSPEC=1)
set(POMME_NO_GRAPHICS TRUE)
set(POMME_NO_INPUT TRUE)
set(POMME_NO_QD3D TRUE)
set(POMME_NO_VIDEO TRUE)
set(POMME_NO_MP3 TRUE)
set(POMME_NO_SOUND_MIXER TRUE)
set(POMME_NO_SOUND_FORMATS TRUE)
add_subdirectory("${POMME_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/Pomme" EXCLUDE_FROM_ALL)

add_library(main SHARED smoke_main.cpp PommeResources.cpp)
# Compile/link Pomme now so missing Android platform facilities fail in CI.
target_link_libraries(main PRIVATE Pomme)

target_include_directories(main PRIVATE "${GAME_ROOT}/Source/Headers")
find_library(GLES3_LIB GLESv3 REQUIRED)
find_library(LOG_LIB log REQUIRED)
target_link_libraries(main PRIVATE SDL3::SDL3 ${GLES3_LIB} ${LOG_LIB})

# Compile-only audit of the original game engine for Android. Keeps current
# working preview untouched. The engine is NOT linked or executed until its
# desktop OpenGL dependencies are successfully ported.
if(NANOSAUR_ENGINE_PROBE)
    file(GLOB_RECURSE NANOSAUR_ENGINE_SOURCES CONFIGURE_DEPENDS
        "${GAME_ROOT}/Source/*.c"
        "${GAME_ROOT}/Source/*.cpp")
    list(FILTER NANOSAUR_ENGINE_SOURCES EXCLUDE REGEX "/Source/Boot[.]cpp$")
    add_library(nanosaur-engine-probe OBJECT ${NANOSAUR_ENGINE_SOURCES})
    target_include_directories(nanosaur-engine-probe PRIVATE "${GAME_ROOT}/Source/Headers")
    target_link_libraries(nanosaur-engine-probe PRIVATE SDL3::SDL3 Pomme)
    target_compile_definitions(nanosaur-engine-probe PRIVATE GL_SILENCE_DEPRECATION)
    add_dependencies(main nanosaur-engine-probe)
endif()
