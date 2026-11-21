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

// Access GL4ES's internal compatibility entry points, not SDL's native
// GLES2 functions: otherwise fixed-function material selection becomes
// out-of-sync and surfaces render white or without their textures.
extern void gl4es_glActiveTexture(unsigned int texture);
extern void gl4es_glClientActiveTexture(unsigned int texture);

void NanosaurAndroidSelectTextureUnit(unsigned int texture)
{
    gl4es_glActiveTexture(texture);
    gl4es_glClientActiveTexture(texture);
}
