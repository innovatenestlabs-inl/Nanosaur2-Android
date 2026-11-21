package com.innovatenestlabs.nanosaur2;

import org.libsdl.app.SDLActivity;

/**
 * Thin Android entry point. SDL3 handles JNI, lifecycle and input forwarding.
 * The native 'main' library launches the original engine with gameplayProbe=true.
 * The default smoke-test build is retained solely for native integration testing.
 */
public final class NanosaurActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] {"SDL3", "main"};
    }
}
