// Temporary Nanosaur II Android touch controller for Phase 5 testing.
// SDL touch coordinates are normalized to 0..1. Multi-finger controls:
//   Lower-left: steering joystick (centre x=.23, y=.68)
//   Lower-right: fire (x=.50-.78) / jetpack (x=.78-1)
//   Middle-right: drop / camera
//   Upper-right: next weapon / pause
// Keyboard and gamepad inputs continue to work alongside these zones.
#include "game.h"

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

#endif
