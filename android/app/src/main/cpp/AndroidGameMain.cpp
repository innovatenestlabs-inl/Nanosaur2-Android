// Experimental native entry point for the full Nanosaur II game engine.
// This file is compiled ONLY with -PgameplayProbe=true. The normal preview
// app remains the default and the original desktop Boot.cpp is untouched.
// All original game and resource rights remain with their respective owners.
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <Pomme.h>
#include <PommeInit.h>
#include <PommeFiles.h>
#include <gl4esinit.h>
#include <exception>
#include <filesystem>
#include <cstdlib>
#include <system_error>
#include <stdexcept>
#include <string>

extern "C" {
#include "game.h"
SDL_Window* gSDLWindow = nullptr;
FSSpec gDataSpec = {};
int gCurrentAntialiasingLevel = 0;
}

extern bool InitializePommeResources(bool retainPomme);

static void ShutdownNativeGame()
{
    Pomme::Shutdown();
    if (gSDLWindow) {
        SDL_DestroyWindow(gSDLWindow);
        gSDLWindow = nullptr;
    }
    SDL_Quit();
}

int main(int, char**)
{
    SDL_SetAppMetadata("Nanosaur II Android Engine Test", "0.3.0-dev",
                       "com.innovatenestlabs.nanosaur2");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Game SDL init: %s", SDL_GetError());
        return 1;
    }

    // Legacy OpenGL entry points are emulated by GL4ES atop GLES2.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    if (!InitializePommeResources(true)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Full game resources unavailable");
        SDL_Quit();
        return 1;
    }

    gSDLWindow = SDL_CreateWindow("Nanosaur II Android Engine Test", 1280, 720,
                                 SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
    if (!gSDLWindow) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Game window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    const char* storage = SDL_GetAndroidInternalStoragePath();
    if (!storage) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Game storage: %s", SDL_GetError());
        SDL_DestroyWindow(gSDLWindow);
        SDL_Quit();
        return 1;
    }

    try {
        // Pomme::FindFolder(kPreferencesFolderType) uses XDG_CONFIG_HOME
        // on Unix-like systems. Android has neither HOME nor XDG_CONFIG_HOME
        // guaranteed, so give it an existing app-private preferences root.
        const std::filesystem::path prefsRoot =
            std::filesystem::path(storage) / "Preferences";
        std::error_code prefsError;
        std::filesystem::create_directories(prefsRoot, prefsError);
        if (prefsError) {
            throw std::runtime_error("Cannot create Android preferences folder: "
                                     + prefsError.message());
        }
        if (setenv("XDG_CONFIG_HOME", prefsRoot.string().c_str(), 1) != 0) {
            throw std::runtime_error("Cannot configure Android preferences directory");
        }
        SDL_Log("Nanosaur Android preferences root ready: %s",
                prefsRoot.string().c_str());

        // InitializePommeResources(true) already initialized Pomme.
        // Reinitialization allocates the system refnum twice and throws.
        const std::string systemDir = std::string(storage) + "/Data/System";
        gDataSpec = Pomme::Files::HostPathToFSSpec(::fs::path(systemDir));
        SDL_Log("Android original game entry initialized; starting GameMain");
        // OGL_CreateDrawContext initializes GL4ES after SDL makes the GLES
        // context current. Calling it here would be too early.
        GameMain();
        close_gl4es();
        ShutdownNativeGame();
        return 0;
    } catch (const Pomme::QuitRequest&) {
        SDL_Log("Nanosaur II clean exit");
    } catch (const std::exception& e) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Nanosaur II game exception: %s", e.what());
    } catch (...) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Nanosaur II unknown engine exception");
    }
    ShutdownNativeGame();
    return 1;
}
