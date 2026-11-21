cmake_minimum_required(VERSION 3.22.1)
project(Nanosaur2AndroidBootstrap LANGUAGES C CXX)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
option(NANOSAUR_ENGINE_PROBE "Compile original Nanosaur game source without linking into the preview" OFF)
option(NANOSAUR_RENDERER_PROBE "Compile GL4ES for Android without changing preview renderer" OFF)
option(NANOSAUR_GAMEPLAY_PROBE "Experimental complete game engine link and startup" OFF)
if(NANOSAUR_GAMEPLAY_PROBE)
    set(NANOSAUR_RENDERER_PROBE ON)
endif()

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

# Pomme's sound mixer uses find_package(SDL3 REQUIRED), while this project
# builds SDL3 as an existing CMake target rather than installing it.
# Export a tiny local package config that resolves to that existing target.
# No system-wide SDL3 installation is required.
set(POMME_SDL3_CONFIG_DIR "${CMAKE_CURRENT_BINARY_DIR}/pomme-sdl3-config")
file(MAKE_DIRECTORY "${POMME_SDL3_CONFIG_DIR}")
file(WRITE "${POMME_SDL3_CONFIG_DIR}/SDL3Config.cmake"
     "if(NOT TARGET SDL3::SDL3)\n"
     "  message(FATAL_ERROR \"SDL3 target must be configured before Pomme\")\n"
     "endif()\n"
     "set(SDL3_FOUND TRUE)\n")
set(SDL3_DIR "${POMME_SDL3_CONFIG_DIR}" CACHE PATH "Internal SDL3 package configuration for Pomme" FORCE)



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
if(NANOSAUR_GAMEPLAY_PROBE)
    set(POMME_NO_MP3 FALSE)
    set(POMME_NO_SOUND_MIXER FALSE)
    set(POMME_NO_SOUND_FORMATS FALSE)
else()
    set(POMME_NO_MP3 TRUE)
    set(POMME_NO_SOUND_MIXER TRUE)
    set(POMME_NO_SOUND_FORMATS TRUE)
endif()
add_subdirectory("${POMME_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/Pomme" EXCLUDE_FROM_ALL)

if(NANOSAUR_GAMEPLAY_PROBE)
    file(GLOB_RECURSE NANOSAUR_GAME_SOURCES CONFIGURE_DEPENDS
        "${GAME_ROOT}/Source/*.c"
        "${GAME_ROOT}/Source/*.cpp")
    list(FILTER NANOSAUR_GAME_SOURCES EXCLUDE REGEX "/Source/Boot[.]cpp$")
    add_library(main SHARED AndroidGameMain.cpp AndroidGL4ESStubs.c PommeResources.cpp ${NANOSAUR_GAME_SOURCES})
    target_include_directories(main PRIVATE "${CMAKE_CURRENT_LIST_DIR}" "${GAME_ROOT}/Source/Headers")
    target_compile_definitions(main PRIVATE GL_SILENCE_DEPRECATION)
    # Development-only route to the first original 3D level; not a release mode.
    option(NANOSAUR_ANDROID_QUICKPLAY "Skip Android intro/menu and enter original Level 1" ON)
    if(NANOSAUR_ANDROID_QUICKPLAY)
        target_compile_definitions(main PRIVATE NANOSAUR_ANDROID_QUICKPLAY=1)
    endif()
else()
    add_library(main SHARED smoke_main.cpp PommeResources.cpp)
endif()
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

# Standalone GL4ES feasibility build. This compiles the compatibility library
# without replacing GLES3 in the verified asset preview. GL4ES has its own
# MIT license; the original Nanosaur II noncommercial restrictions remain.
if(NANOSAUR_RENDERER_PROBE)
    set(GL4ES_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../gl4es")
    if(NOT EXISTS "${GL4ES_ROOT}/src/CMakeLists.txt")
        message(FATAL_ERROR "GL4ES missing. Run android/setup-gl4es.ps1 before enabling the renderer probe.")
    endif()
    # GL4ES must use its Android/no-GBM build path. Merely using the NDK
    # toolchain does not guarantee its own ANDROID option is enabled.
    set(ANDROID ON CACHE BOOL "Build GL4ES for Android" FORCE)
    set(NO_GBM ON CACHE BOOL "No desktop GBM display under SDL/Android" FORCE)
    set(STATICLIB ON CACHE BOOL "Build GL4ES as static library" FORCE)
    set(NOX11 ON CACHE BOOL "SDL owns the Android context" FORCE)
    set(NOEGL ON CACHE BOOL "SDL owns the EGL context" FORCE)
    set(NO_LOADER ON CACHE BOOL "Avoid dynamic GL loader in probe" FORCE)
    set(NO_INIT_CONSTRUCTOR ON CACHE BOOL "" FORCE)
    set(DEFAULT_ES 2 CACHE STRING "GL4ES GLES2 backend" FORCE)
    include_directories("${GL4ES_ROOT}/include")
    add_subdirectory("${GL4ES_ROOT}/src" "${CMAKE_CURRENT_BINARY_DIR}/gl4es" EXCLUDE_FROM_ALL)
    # We add the GL4ES src/ subdirectory directly, not its root CMakeLists.
    # Therefore its root options do not inject C compiler preprocessor defines.
    # Set Android/GLES2/no-GLX flags explicitly on the library target.
    target_compile_definitions(GL PRIVATE
        ANDROID NOX11 NO_GBM NOEGL NO_LOADER NO_INIT_CONSTRUCTOR
        STATICLIB DEFAULT_ES=2)
    add_dependencies(main GL)
    if(NANOSAUR_GAMEPLAY_PROBE)
        target_link_libraries(main PRIVATE GL)
    endif()
endif()
