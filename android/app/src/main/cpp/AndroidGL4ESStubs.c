// Minimal GL4ES GLX/GBM entry points for SDL-managed Android GLES contexts.
// GL4ES builds without its desktop GLX windowing module on Android. The
// functions below are only relevant to GLX/pixmap/GBM paths that SDL does
// not use. SDL3 owns context creation and buffer swapping.
//
// Important: do not enable these in the preview-only build.
#include <stddef.h>
#include <SDL3/SDL.h>
#include <gl4esinit.h>

static SDL_Window* gAndroidGLWindow = NULL;

static void* AndroidGLESProcAddress(const char* name)
{
    return SDL_GL_GetProcAddress(name);
}

static void AndroidFramebufferSize(int* width, int* height)
{
    int w = 0;
    int h = 0;
    if (gAndroidGLWindow)
        SDL_GetWindowSizeInPixels(gAndroidGLWindow, &w, &h);
    if (width) *width = w;
    if (height) *height = h;
}

void NanosaurAndroidInitializeGL4ES(SDL_Window* window)
{
    gAndroidGLWindow = window;
    // SDL has created the GLES context. Hand its native symbols and
    // framebuffer dimensions to the GL4ES fixed-function translator.
    set_getprocaddress(AndroidGLESProcAddress);
    set_getmainfbsize(AndroidFramebufferSize);
    initialize_gl4es();
    SDL_Log("Nanosaur Android GL4ES initialized with SDL-managed GLES context");
}


void glx_init(void) {}
void FreeFBVisual(void) {}
void BlitEmulatedPixmap(int window) { (void) window; }
void refreshMainFBO(void) {}
void gl4es_SwapBuffers_currentContext(void) {}
