// Temporary Nanosaur II Android touch controller for Phase 5 testing.
// SDL touch coordinates are normalized to 0..1. Multi-finger controls:
//   Lower-left: steering joystick (centre x=.23, y=.68)
//   Lower-right: fire (x=.50-.78) / jetpack (x=.78-1)
//   Middle-right: drop / camera
//   Upper-right: next weapon / pause
// Keyboard and gamepad inputs continue to work alongside these zones.
#include "game.h"
#include <math.h>

#if defined(__ANDROID__) && defined(NANOSAUR_ANDROID_QUICKPLAY)

#define MAX_ANDROID_TOUCH_FINGERS 10
typedef struct
{
    bool active;
    SDL_FingerID id;
    float x;
    float y;
} AndroidTouch;

static AndroidTouch gTouches[MAX_ANDROID_TOUCH_FINGERS];

void NanosaurAndroidTouchEvent(const SDL_TouchFingerEvent* finger, bool released)
{
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

    gTouches[slot].active = !released;
    gTouches[slot].id = finger->fingerID;
    gTouches[slot].x = finger->x;
    gTouches[slot].y = finger->y;
}

static float ClampStrength(float strength)
{
    return strength >= 1.0f ? 1.0f : strength;
}

float NanosaurAndroidTouchNeedAnalog(int need)
{
    float strength = 0.0f;
    for (int i = 0; i < MAX_ANDROID_TOUCH_FINGERS; ++i)
    {
        if (!gTouches[i].active)
            continue;

        const float x = gTouches[i].x;
        const float y = gTouches[i].y;

        if (x < .5f && y >= .32f)
        {
            const float dx = (x - .23f) / .18f;
            const float dy = (y - .68f) / .22f;

            if (need == kNeed_YawLeft && dx < -.16f)
                strength = GAME_MAX(strength, ClampStrength(-dx));
            if (need == kNeed_YawRight && dx > .16f)
                strength = GAME_MAX(strength, ClampStrength(dx));
            if (need == kNeed_PitchUp && dy < -.16f)
                strength = GAME_MAX(strength, ClampStrength(-dy));
            if (need == kNeed_PitchDown && dy > .16f)
                strength = GAME_MAX(strength, ClampStrength(dy));
        }
        else if (x >= .5f)
        {
            int action = 0;
            if (y >= .55f)
                action = x < .78f ? kNeed_Fire : kNeed_Jetpack;
            else if (y >= .28f)
                action = x < .78f ? kNeed_Drop : kNeed_CameraMode;
            else
                action = x < .78f ? kNeed_NextWeapon : kNeed_UIPause;

            if (need == action)
                strength = 1.0f;
        }
    }
    return strength;
}

void NanosaurAndroidTouchReset(void)
{
    SDL_memset(gTouches, 0, sizeof(gTouches));
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
        glVertex2f(x + cosf(angle) * r, y + sinf(angle) * r);
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
    AndroidHudCircle(.23f, .68f, .17f, .07f, .12f, .16f, .28f);
    AndroidHudCircle(.23f, .68f, .145f, .62f, .87f, .98f, .17f);
    float thumbX = .23f;
    float thumbY = .68f;
    for (int i = 0; i < MAX_ANDROID_TOUCH_FINGERS; ++i)
        if (gTouches[i].active && gTouches[i].x < .5f && gTouches[i].y >= .32f)
        {
            float dx = gTouches[i].x - .23f;
            float dy = gTouches[i].y - .68f;
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

    // Right action zones: match the current tested multitouch mapping.
    static const struct {
        float x, y, radius;
        int need;
        const char* label;
    } buttons[] = {
        {.63f, .78f, .065f, kNeed_Fire, "FIRE"},
        {.89f, .78f, .065f, kNeed_Jetpack, "JET"},
        {.63f, .41f, .050f, kNeed_Drop, "DROP"},
        {.89f, .41f, .050f, kNeed_CameraMode, "CAM"},
        {.63f, .14f, .045f, kNeed_NextWeapon, "NEXT"},
        {.89f, .14f, .045f, kNeed_UIPause, "PAUSE"}
    };

    for (size_t i = 0; i < SDL_arraysize(buttons); ++i)
    {
        const float pressed = NanosaurAndroidTouchNeedAnalog(buttons[i].need);
        AndroidHudCircle(buttons[i].x, buttons[i].y, buttons[i].radius,
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

    for (size_t i = 0; i < SDL_arraysize(buttons); ++i)
        AndroidHudLabel(buttons[i].label, buttons[i].x - .023f, buttons[i].y - .014f);
}

#endif
