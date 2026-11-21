// Temporary Nanosaur II Android touch controller for Phase 5 testing.
// SDL touch coordinates are normalized to 0..1. Multi-finger controls:
//   Lower-left: steering joystick (centre x=.23, y=.68)
//   Lower-right: fire (x=.50-.78) / jetpack (x=.78-1)
//   Middle-right: drop / camera
//   Upper-right: next weapon / pause
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
static bool gSteeringFingerActive = false;
static SDL_FingerID gSteeringFinger = 0;
static float gStickCenterX = .23f, gStickCenterY = .68f;
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
    const float deadzone = .12f;
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

    if (slot < 0 && !released)
        for (int i = 0; i < MAX_ANDROID_TOUCH_FINGERS; ++i)
            if (!gTouches[i].active)
            {
                slot = i;
                break;
            }

    if (slot < 0)
        return;

    // The first lower-left touch claims steering; other fingers may operate
    // action buttons simultaneously without hijacking the joystick.
    if (!released && !gSteeringFingerActive && finger->x < .49f && finger->y > .30f)
    {
        gSteeringFinger = finger->fingerID;
        gSteeringFingerActive = true;
        gStickCenterX = AndroidLimit(finger->x, .12f, .38f);
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
    if (gSteeringFingerActive)
    {
        for (int i = 0; i < MAX_ANDROID_TOUCH_FINGERS; ++i)
            if (gTouches[i].active && gTouches[i].id == gSteeringFinger)
            {
                // X/Y radii account for a landscape phone's aspect ratio.
                targetYaw = AndroidSteerCurve((gTouches[i].x - gStickCenterX) / .11f);
                targetPitch = AndroidSteerCurve((gTouches[i].y - gStickCenterY) / .21f);
                break;
            }
    }

    const Uint64 now = SDL_GetTicks();
    float dt = gLastSteerTick ? (float)(now - gLastSteerTick) * .001f : .016f;
    gLastSteerTick = now;
    dt = AndroidLimit(dt, .001f, .06f);
    const float alpha = 1.0f - expf(-22.0f * dt);
    gSmoothYaw += (targetYaw - gSmoothYaw) * alpha;
    gSmoothPitch += (targetPitch - gSmoothPitch) * alpha;
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
} gAndroidActions[] = {
    {.63f, .78f, .12f, .15f, kNeed_Fire, "FIRE"},
    {.89f, .78f, .11f, .15f, kNeed_Jetpack, "JET"},
    {.63f, .41f, .12f, .125f, kNeed_Drop, "DROP"},
    {.89f, .41f, .11f, .125f, kNeed_CameraMode, "VIEW"},
    {.63f, .14f, .12f, .12f, kNeed_NextWeapon, "NEXT"},
    {.89f, .14f, .11f, .12f, kNeed_UIPause, "PAUSE"}
};

float NanosaurAndroidTouchNeedAnalog(int need)
{
    if (!gAndroidGameplayActive || gGamePaused)
        return 0.0f;

    switch (need)
    {
        case kNeed_YawLeft: return fmaxf(0.0f, -gSmoothYaw);
        case kNeed_YawRight: return fmaxf(0.0f, gSmoothYaw);
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
            if (button->need != need)
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

    // Steering pad (lower-left) and thumb indicator.
    const float padX = gSteeringFingerActive ? gStickCenterX : .23f;
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

    // Render actions from the same hit-target table used by input.
    for (size_t i = 0; i < SDL_arraysize(gAndroidActions); ++i)
    {
        const struct AndroidActionButton* button = &gAndroidActions[i];
        const float pressed = NanosaurAndroidTouchNeedAnalog(button->need);
        AndroidHudCircle(button->x, button->y, button->radiusY * .56f,
                         pressed > 0 ? .10f : .03f,
                         pressed > 0 ? .75f : .20f,
                         pressed > 0 ? .30f : .28f,
                         pressed > 0 ? .70f : .35f);
    }

    // Label glyphs use the original game's atlas/font. Restore our HUD
    // state and leave OGL_DrawString to save/restore its own matrices.
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
    glMatrixMode(GL_MODELVIEW);

    for (size_t i = 0; i < SDL_arraysize(gAndroidActions); ++i)
        AndroidHudLabel(gAndroidActions[i].label,
                        gAndroidActions[i].x - .025f,
                        gAndroidActions[i].y - .014f);
}

#endif
