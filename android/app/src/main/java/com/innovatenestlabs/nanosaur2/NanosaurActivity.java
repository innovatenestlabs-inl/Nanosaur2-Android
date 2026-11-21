package com.innovatenestlabs.nanosaur2;

import org.libsdl.app.SDLActivity;

/**
 * Thin Android entry point. SDL3 handles JNI, lifecycle and input forwarding.
 * The native 'main' library currently renders a smoke-test screen, not the game.
 */
public final class NanosaurActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] {"SDL3", "main"};
    }
}
