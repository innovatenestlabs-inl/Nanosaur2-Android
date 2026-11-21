// Minimal GL4ES GLX/GBM entry points for SDL-managed Android GLES contexts.
// GL4ES builds without its desktop GLX windowing module on Android. The
// functions below are only relevant to GLX/pixmap/GBM paths that SDL does
// not use. SDL3 owns context creation and buffer swapping.
//
// Important: do not enable these in the preview-only build.
#include <stddef.h>

void glx_init(void) {}
void FreeFBVisual(void) {}
void BlitEmulatedPixmap(int window) { (void) window; }
void refreshMainFBO(void) {}
void gl4es_SwapBuffers_currentContext(void) {}
