// Temporary Nanosaur II Android touch controller for Phase 5 testing.
// SDL touch coordinates are normalized to 0..1. Multi-finger controls:
//   Right: user-selectable floating joystick or four-arrow directional pad.
//   Left: FIRE/JET remain visible; MORE reveals DROP, PAUSE, NEXT and VIEW.
//   Top right: switch between joystick and directional arrows.
// Keyboard and external gamepads remain independently supported.
// Keyboard and gamepad inputs continue to work alongside these zones.
#include "game.h"
#include <math.h>

#if defined(__ANDROID__) && defined(NANOSAUR_ANDROID_TOUCH_CONTROLS)

#define MAX_ANDROID_TOUCH_FINGERS 10
typedef struct
{
    bool active;
    SDL_FingerID id;
    float x;
    float y;
} AndroidTouch;

static AndroidTouch gTouches[MAX_ANDROID_TOUCH_FINGERS];

// Floating joystick prevents sudden steering when the user first touches.
// Actions use discrete hit targets rather than triggering across an entire
// right-side region. Gamepad and keyboard input are unaffected.
static bool gAndroidGameplayActive = false;
static bool gAndroidActionsExpanded = false;
static bool gToggleFingerHeld = false;
static SDL_FingerID gToggleFingerID = 0;
static bool gModeFingerHeld = false;
static SDL_FingerID gModeFingerID = 0;
static bool gAndroidUseDpad = false;
static const float gModeCenterX = .78f, gModeCenterY = .13f;
static const float gModeRadiusX = .105f, gModeRadiusY = .105f;

// Only FIRE/JET are visible by default. Secondary actions expand on demand.
// Movement mode persists while game input resets on pause/backgrounding.
static const float gToggleCenterX = .34f;
static const float gToggleCenterY = .14f;
static const float gToggleRadiusX = .072f;
static const float gToggleRadiusY = .095f;
static bool gSteeringFingerActive = false;
static SDL_FingerID gSteeringFinger = 0;
static float gStickCenterX = .77f, gStickCenterY = .68f;
static float gSmoothYaw = 0.0f, gSmoothPitch = 0.0f;
static Uint64 gLastSteerTick = 0;

void NanosaurAndroidTouchReset(void);

void NanosaurAndroidTouchSetGameplayActive(bool active)
{
    if (gAndroidGameplayActive == active)
        return;
    gAndroidGameplayActive = active;
    NanosaurAndroidTouchReset();
}

static float AndroidLimit(float value, float lo, float hi)
{
    return value < lo ? lo : (value > hi ? hi : value);
}

static float AndroidSteerCurve(float normalized)
{
    const float deadzone = .14f; // Less jitter around neutral on phones
    const float sign = normalized < 0.0f ? -1.0f : 1.0f;
    float mag = fabsf(normalized);
    if (mag <= deadzone)
        return 0.0f;
    mag = AndroidLimit((mag - deadzone) / (1.0f - deadzone), 0.0f, 1.0f);
    // Mild quadratic acceleration improves fine steering around center.
    return sign * (.60f * mag + .40f * mag * mag);
}


void NanosaurAndroidTouchEvent(const SDL_TouchFingerEvent* finger, bool released)
{
    if (!gAndroidGameplayActive || gGamePaused)
        return;

    int slot = -1;
    for (int i = 0; i < MAX_ANDROID_TOUCH_FINGERS; ++i)
        if (gTouches[i].active && gTouches[i].id == finger->fingerID)
        {
            slot = i;
            break;
        }

    // Consume toggling fingers until release. Dragging from a toggle must
    // never trigger another action or steering.
    if (gToggleFingerHeld && gToggleFingerID == finger->fingerID)
    {
        if (released)
            gToggleFingerHeld = false;
        return;
    }
    if (gModeFingerHeld && gModeFingerID == finger->fingerID)
    {
        if (released)
            gModeFingerHeld = false;
        return;
    }

    if (slot < 0 && !released)
    {
        const float modeDx = (finger->x - gModeCenterX) / gModeRadiusX;
        const float modeDy = (finger->y - gModeCenterY) / gModeRadiusY;
        if (modeDx*modeDx + modeDy*modeDy <= 1.0f)
        {
            gAndroidUseDpad = !gAndroidUseDpad;
            gModeFingerHeld = true;
            gModeFingerID = finger->fingerID;
            gSteeringFingerActive = false;
            gSmoothYaw = gSmoothPitch = 0.0f;
            gLastSteerTick = 0;
            // Discard old finger presses so changing modes never sticks input.
            SDL_memset(gTouches, 0, sizeof(gTouches));
            SDL_Log("Nanosaur Android controls: direction mode %s",
                    gAndroidUseDpad ? "arrows" : "joystick");
            return;
        }
        const float dx = (finger->x - gToggleCenterX) / gToggleRadiusX;
        const float dy = (finger->y - gToggleCenterY) / gToggleRadiusY;
        if (dx*dx + dy*dy <= 1.0f)
        {
            gAndroidActionsExpanded = !gAndroidActionsExpanded;
            gToggleFingerHeld = true;
            gToggleFingerID = finger->fingerID;
            SDL_Log("Nanosaur Android controls: %s",
                    gAndroidActionsExpanded ? "expanded" : "collapsed");
            return;
        }
        for (int i = 0; i < MAX_ANDROID_TOUCH_FINGERS; ++i)
            if (!gTouches[i].active)
            {
                slot = i;
                break;
            }
    }

    if (slot < 0)
        return;

    // The first lower-right touch claims steering; other fingers may operate
    // action buttons simultaneously without hijacking the joystick.
    if (!gAndroidUseDpad && !released && !gSteeringFingerActive && finger->x > .51f && finger->y > .30f)
    {
        gSteeringFinger = finger->fingerID;
        gSteeringFingerActive = true;
        gStickCenterX = AndroidLimit(finger->x, .62f, .88f);
        gStickCenterY = AndroidLimit(finger->y, .49f, .82f);
        gSmoothYaw = 0.0f;
        gSmoothPitch = 0.0f;
        gLastSteerTick = SDL_GetTicks();
    }

    gTouches[slot].active = !released;
    gTouches[slot].id = finger->fingerID;
    gTouches[slot].x = finger->x;
    gTouches[slot].y = finger->y;

    if (released && gSteeringFingerActive && finger->fingerID == gSteeringFinger)
    {
        gSteeringFingerActive = false;
        gSmoothYaw = 0.0f;
        gSmoothPitch = 0.0f;
        gLastSteerTick = 0;
    }
}

void NanosaurAndroidTouchTick(void)
{
    if (!gAndroidGameplayActive || gGamePaused)
        return;

    float targetYaw = 0.0f;
    float targetPitch = 0.0f;
    if (!gAndroidUseDpad && gSteeringFingerActive)
    {
        for (int i = 0; i < MAX_ANDROID_TOUCH_FINGERS; ++i)
            if (gTouches[i].active && gTouches[i].id == gSteeringFinger)
            {
                // X/Y radii account for a landscape phone's aspect ratio.
                targetYaw = AndroidSteerCurve((gTouches[i].x - gStickCenterX) / .15f);
                targetPitch = AndroidSteerCurve((gTouches[i].y - gStickCenterY) / .25f);
                break;
            }
    }

    const Uint64 now = SDL_GetTicks();
    float dt = gLastSteerTick ? (float)(now - gLastSteerTick) * .001f : .016f;
    gLastSteerTick = now;
    dt = AndroidLimit(dt, .001f, .06f);
    const float yawAlpha = 1.0f - expf(-17.0f * dt);
    // Never carry an opposite vertical command into a quick direction change.
    // Keep enough smoothing for precise egg/wormhole approaches.
    if (targetPitch * gSmoothPitch < 0.0f)
        gSmoothPitch = 0.0f;
    const float pitchAlpha = 1.0f - expf(-28.0f * dt);
    gSmoothYaw += (targetYaw - gSmoothYaw) * yawAlpha;
    gSmoothPitch += (targetPitch - gSmoothPitch) * pitchAlpha;
    if (!gSteeringFingerActive)
    {
        gSmoothYaw = 0.0f;
        gSmoothPitch = 0.0f;
    }
}

static const struct AndroidActionButton {
    float x, y, radiusX, radiusY;
    int need;
    const char* label;
    bool expandedOnly;
} gAndroidActions[] = {
    // Clear bottom row for frequent two-thumb actions; compact secondary row.
    {.34f, .82f, .095f, .115f, kNeed_Fire, "FIRE", false},
    {.13f, .82f, .090f, .115f, kNeed_Jetpack, "JET", false},
    // Secondary actions are all hidden until MORE is selected.
    {.13f, .59f, .080f, .090f, kNeed_Drop, "DROP", true},
    {.13f, .36f, .080f, .090f, kNeed_UIPause, "PAUSE", true},
    {.34f, .36f, .080f, .090f, kNeed_NextWeapon, "NEXT", true},
    {.34f, .59f, .080f, .090f, kNeed_CameraMode, "VIEW", true}
};

// Screen-space D-pad targets (not tied to the floating stick).
// A second finger can hold another arrow to steer diagonally.
static const struct AndroidDpadButton {
    float x, y, radiusX, radiusY;
    int need;
    const char* label;
} gAndroidDpad[] = {
    {.78f, .48f, .090f, .085f, kNeed_PitchUp, "UP"},
    {.78f, .86f, .090f, .085f, kNeed_PitchDown, "DN"},
    {.64f, .67f, .090f, .090f, kNeed_YawLeft, "LEFT"},
    {.92f, .67f, .075f, .090f, kNeed_YawRight, "RIGHT"}
};

static float AndroidDpadNeedAnalog(int need)
{
    for (int i = 0; i < MAX_ANDROID_TOUCH_FINGERS; ++i)
    {
        if (!gTouches[i].active)
            continue;
        for (size_t j = 0; j < SDL_arraysize(gAndroidDpad); ++j)
        {
            const struct AndroidDpadButton* button = &gAndroidDpad[j];
            if (button->need != need)
                continue;
            const float dx = (gTouches[i].x - button->x) / button->radiusX;
            const float dy = (gTouches[i].y - button->y) / button->radiusY;
            if (dx*dx + dy*dy <= 1.0f)
                return 1.0f;
        }
    }
    return 0.0f;
}

float NanosaurAndroidTouchNeedAnalog(int need)
{
    if (!gAndroidGameplayActive || gGamePaused)
        return 0.0f;

    if (gAndroidUseDpad)
    {
        switch (need)
        {
            case kNeed_YawLeft:
            case kNeed_YawRight:
            case kNeed_PitchUp:
            case kNeed_PitchDown:
                return AndroidDpadNeedAnalog(need);
            default:
                break;
        }
    }

    switch (need)
    {
        case kNeed_YawLeft: return fmaxf(0.0f, -gSmoothYaw);
        case kNeed_YawRight: return fmaxf(0.0f, gSmoothYaw);
        // The original GetNeedAnalogSteering(PitchUp, PitchDown) produces
        // negative for climb and positive for dive. Screen Y increases down,
        // so a downward drag must trigger PitchDown, not PitchUp.
        case kNeed_PitchUp: return fmaxf(0.0f, -gSmoothPitch);
        case kNeed_PitchDown: return fmaxf(0.0f, gSmoothPitch);
        default: break;
    }

    for (int i = 0; i < MAX_ANDROID_TOUCH_FINGERS; ++i)
    {
        if (!gTouches[i].active || (gSteeringFingerActive && gTouches[i].id == gSteeringFinger))
            continue;

        for (size_t j = 0; j < SDL_arraysize(gAndroidActions); ++j)
        {
            const struct AndroidActionButton* button = &gAndroidActions[j];
            if (button->need != need || (button->expandedOnly && !gAndroidActionsExpanded))
                continue;
            const float dx = (gTouches[i].x - button->x) / button->radiusX;
            const float dy = (gTouches[i].y - button->y) / button->radiusY;
            if (dx * dx + dy * dy <= 1.0f)
                return 1.0f;
        }
    }
    return 0.0f;
}

void NanosaurAndroidTouchReset(void)
{
    SDL_memset(gTouches, 0, sizeof(gTouches));
    gAndroidActionsExpanded = false;
    gToggleFingerHeld = false;
    gToggleFingerID = 0;
    gModeFingerHeld = false;
    gModeFingerID = 0;
    gSteeringFingerActive = false;
    gSmoothYaw = gSmoothPitch = 0.0f;
    gLastSteerTick = 0;
}

/*
 * Phase 6 touch HUD. Render with the game's fixed-function GL4ES context,
 * after 3D gameplay, before the back-buffer swap. All drawing is enclosed
 * in GL attribute/matrix saves so terrain and game material states are
 * restored for the next frame.
 */
static void AndroidHudCircle(float x, float y, float r, float red, float green, float blue, float alpha)
{
    glColor4f(red, green, blue, alpha);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 32; ++i)
    {
        const float angle = (float) i * (6.28318530718f / 32.0f);
        // Normalize the X radius so circles remain round on landscape phones.
        const float aspect = gGameWindowWidth > 0 ? (float)gGameWindowHeight / (float)gGameWindowWidth : .5625f;
        glVertex2f(x + cosf(angle) * r * aspect, y + sinf(angle) * r);
    }
    glEnd();
}

// Render an actual directional arrow with GL geometry rather than a text label.
// Local coordinates are corrected for the landscape screen aspect ratio,
// so the arrowhead and shaft keep their intended proportions.
static void AndroidHudArrow(float x, float y, float dx, float dy, bool pressed)
{
    const float aspect = gGameWindowWidth > 0
        ? (float) gGameWindowHeight / (float) gGameWindowWidth : .5625f;
    const float forward[] = { .045f, .004f, .004f, -.032f, -.032f, .004f, .004f };
    const float side[] = { 0.0f, .032f, .013f, .013f, -.013f, -.013f, -.032f };
    glColor4f(pressed ? 1.0f : .90f, pressed ? 1.0f : .95f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i < 7; ++i)
    {
        const float tx = dx * forward[i] - dy * side[i];
        const float ty = dy * forward[i] + dx * side[i];
        glVertex2f(x + tx * aspect, y + ty);
    }
    glEnd();
}

static void AndroidHudLabel(const char* label, float x, float y)
{
    // The original game's debug font already supports a virtual 640x480
    // coordinate system, avoiding a second native text renderer.
    OGL_DrawString(label, (GLint) (x * 640), (GLint) (y * 480));
}

void NanosaurAndroidDrawTouchHUD(void)
{
    if (!gAndroidGameplayActive || gGamePaused)
        return;
    // Save all desktop OpenGL fixed-function state and matrices before HUD.
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 1, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    OGL_ActiveTextureUnit(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    // Do not disable client vertex arrays here: glPushAttrib does not save
    // GL_CLIENT_VERTEX_ARRAY_BIT, and leaving them disabled would corrupt
    // subsequent native 3D draws when the HUD has been rendered.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // The user only sees the currently chosen direction controller.
    if (gAndroidUseDpad)
    {
        for (size_t i = 0; i < SDL_arraysize(gAndroidDpad); ++i)
        {
            const struct AndroidDpadButton* button = &gAndroidDpad[i];
            const float pressed = AndroidDpadNeedAnalog(button->need);
            AndroidHudCircle(button->x, button->y, .060f,
                             pressed > 0 ? .12f : .05f,
                             pressed > 0 ? .70f : .27f,
                             pressed > 0 ? .35f : .45f,
                             pressed > 0 ? .85f : .52f);
            float dx = 0.0f, dy = 0.0f;
            switch (button->need)
            {
                case kNeed_PitchUp: dy = -1.0f; break;
                case kNeed_PitchDown: dy = 1.0f; break;
                case kNeed_YawLeft: dx = -1.0f; break;
                case kNeed_YawRight: dx = 1.0f; break;
                default: break;
            }
            AndroidHudArrow(button->x, button->y, dx, dy, pressed > 0.0f);
        }
    }
    else
    {
        // Floating joystick and thumb indicator.
        const float padX = gSteeringFingerActive ? gStickCenterX : .77f;
        const float padY = gSteeringFingerActive ? gStickCenterY : .68f;
        AndroidHudCircle(padX, padY, .17f, .07f, .12f, .16f, .28f);
        AndroidHudCircle(padX, padY, .145f, .62f, .87f, .98f, .17f);
        float thumbX = padX;
        float thumbY = padY;
        for (int i = 0; i < MAX_ANDROID_TOUCH_FINGERS; ++i)
            if (gSteeringFingerActive && gTouches[i].active && gTouches[i].id == gSteeringFinger)
            {
                float dx = (gTouches[i].x - padX) * 0.55f;
                float dy = gTouches[i].y - padY;
                const float len = sqrtf(dx*dx + dy*dy);
                if (len > .12f)
                {
                    dx *= .12f / len;
                    dy *= .12f / len;
                }
                thumbX += dx;
                thumbY += dy;
                break;
            }
        AndroidHudCircle(thumbX, thumbY, .045f, .70f, .95f, 1.0f, .75f);
    }

    // Render actions from the same hit-target table used by input.
    for (size_t i = 0; i < SDL_arraysize(gAndroidActions); ++i)
    {
        const struct AndroidActionButton* button = &gAndroidActions[i];
        if (button->expandedOnly && !gAndroidActionsExpanded)
            continue;
        const float pressed = NanosaurAndroidTouchNeedAnalog(button->need);
        AndroidHudCircle(button->x, button->y, button->radiusY * .47f,
                         pressed > 0 ? .10f : .03f,
                         pressed > 0 ? .75f : .20f,
                         pressed > 0 ? .30f : .28f,
                         pressed > 0 ? .70f : .35f);
    }

    // Separate control-mode toggle (upper right) and action expansion (left).
    AndroidHudCircle(gToggleCenterX, gToggleCenterY, .040f,
                     .08f, .25f, .38f, .65f);
    AndroidHudCircle(gModeCenterX, gModeCenterY, .050f,
                     .08f, .25f, .38f, .65f);

    // Label glyphs use the original game's atlas/font. Restore our HUD
    // state and leave OGL_DrawString to save/restore its own matrices.
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
    glMatrixMode(GL_MODELVIEW);

    for (size_t i = 0; i < SDL_arraysize(gAndroidActions); ++i)
    {
        if (gAndroidActions[i].expandedOnly && !gAndroidActionsExpanded)
            continue;
        AndroidHudLabel(gAndroidActions[i].label,
                        gAndroidActions[i].x - .025f,
                        gAndroidActions[i].y - .014f);
    }
    AndroidHudLabel(gAndroidActionsExpanded ? "LESS" : "MORE",
                    gToggleCenterX - .025f, gToggleCenterY - .014f);
    AndroidHudLabel(gAndroidUseDpad ? "STICK" : "ARROWS",
                    gModeCenterX - .035f, gModeCenterY - .014f);
    // D-pad direction glyphs are rendered by AndroidHudArrow above,
    // using vector geometry rather than the game's limited text font.
}

#endif
