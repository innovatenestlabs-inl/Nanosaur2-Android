# Nanosaur 2 Android — Original-asset preview (Milestone 2A)

**Purpose:** Validate the original game's resource pipeline before attempting
the legacy desktop OpenGL renderer. The original game has **not** been ported
or linked into the APK yet.

## Verified earlier

On a physical Android phone, the original SDL3 + OpenGL ES 3.0 native
bootstrap compiled and displayed a solid teal screen (Milestone 1).

## Current change

- The Gradle build copies the **original** `Data/Sprites/maps/level1.jpg`
  into Android APK assets at `Sprites/maps/level1.jpg`.
- The existing upstream `Source/Headers/stb_image.h` decodes JPEG in memory.
- The native app uses `SDL_LoadFile` and a minimal GLES3 shader/quad to draw
  the original map, maintaining its aspect ratio against a teal background.
- No source asset conversion, duplicate assets, game logic modification or
  desktop renderer replacement was necessary for this test.
- This should display a **level map image**, not playable Nanosaur gameplay.

## Setup and build (Windows PowerShell)

From the repository root:

```powershell
git checkout android-port
git pull origin android-port
powershell -ExecutionPolicy Bypass -File android/setup-sdl.ps1
cd android
```

`android/local.properties` is intentionally **not versioned**. A previous
commit accidentally tracked it. After pulling a commit that untracks it,
recreate your local file as follows:

```powershell
$sdk = "$env:LOCALAPPDATA\Android\Sdk"
if (!(Test-Path $sdk)) { throw "Android SDK not found: $sdk" }
"sdk.dir=$($sdk -replace '\\','/')" |
    Set-Content -Encoding ASCII .\local.properties
.\gradlew.bat :app:assembleDebug
adb install -r .\app\build\outputs\apk\debug\app-debug.apk
adb shell am force-stop com.innovatenestlabs.nanosaur2
adb shell am start -n com.innovatenestlabs.nanosaur2/.NanosaurActivity
```

When successful, the screen displays an **original Nanosaur level map** rather
than a plain teal screen. If still teal or crashing, capture Android Logcat.
Native log markers:

- `Loaded original Nanosaur image`
- `Nanosaur Android original-asset preview started`
- `Couldn't open bundled Nanosaur asset`
- `Original game JPEG decoding failed`
- `Shader error` or `Shader link error`

Windows ADB log capture:

```powershell
adb logcat -c
adb shell am force-stop com.innovatenestlabs.nanosaur2
adb shell am start -n com.innovatenestlabs.nanosaur2/.NanosaurActivity
adb logcat -d -s SDL:V SDL/APP:V AndroidRuntime:E libc:E
```

## Next planned milestone

Port Pomme's required functionality and replace fixed-function OpenGL
calls in `Source/3D/OGL_Support.c`, `Source/3D/MetaObjects.c` and
`Source/3D/Atlas.c` with GLES-compatible rendering. Game asset packaging
will expand as the rest of the engine becomes functional.

## Licensing

Nanosaur II © Pangea Software; modern port © Iliyas Jorio.
Upstream: https://github.com/jorio/Nanosaur2
Existing CC BY-NC-SA 4.0 licensing and attribution are preserved.
This experimental Android port is for private, noncommercial evaluation.

## Experimental complete resource packaging

The current APK includes one unmodified original level-map image for the
verified GLES3 preview. To package the **entire** original `Data/` tree
for later engine integration, use:

```powershell
cd E:\Projects\Game\Nanosaur2-Android\android
.\gradlew.bat :app:assembleDebug -PfullGameAssets=true
```

This package preserves the original directory tree under `assets/Data/`.
It also keeps the standalone preview image under `assets/Sprites/maps/`.
Use `jar tf .\app\build\outputs\apk\debug\app-debug.apk` to inspect
packaged files. **Including the resources does not make the full game run.**
Pomme's original filesystem APIs cannot yet treat Android APK assets as
ordinary desktop files; file virtualization/extraction is still needed.
Do not publish this resource-containing APK commercially.

## Remaining full-game port blockers (not yet implemented)

1. **Rendering compatibility:** Original `OGL_Support.c`, `MetaObjects.c`
   and `Atlas.c` use desktop OpenGL fixed-function/immediate-mode calls
   unavailable on Android GLES3. Evaluate a GL4ES compatibility approach
   (https://github.com/ptitSeb/gl4es, MIT-licensed) versus an explicit GLES
   shader rewrite before selecting either. Both need real device validation.
2. **Pomme / filesystem:** Compile the pinned `extern/Pomme` submodule for
   Android and map Macintosh-style FSSpec paths onto accessible game files.
3. **Engine entry:** Build and link original C/C++ sources into an Android
   native shared library with appropriate SDL3 lifecycle and GLES setup.
4. **Runtime:** Validate splash/menu, music, collision, terrain, 3D entities,
   levels and save-state behavior on hardware.
5. **Mobile controls:** Integrate accelerometer flight, recalibration,
   touchscreen buttons and navigation.
6. **Verification:** Full gameplay testing, frame pacing, crash reporting,
   rotation/pause/resume, memory use and APK checks.

**Status:** Milestone 1 passed on a physical phone. Milestone 2A map asset
loading was tested by the user; a full game build has not been produced.
The optional full asset archive is a development facility, not proof of a
working engine. Keep all work on `android-port`.

## Milestone 2B — Pomme filesystem bridge (experimental)

The `-PfullGameAssets=true` build now generates
`assets/nanosaur-data-files.txt` with sorted original resource filenames.
When launching this build, `PommeResources.cpp` extracts the original
`assets/Data/**` files into the app's **private internal storage**. An
installed-manifest marker avoids repeating extraction when resources are
unchanged. The native program calls `Pomme::Init()` and checks that Pomme
can resolve the original `:System:gamecontrollerdb.txt` FSSpec.

The original level-map preview remains the fallback; there is **no playable
original gameplay yet** and the rest of the desktop engine isn't linked.

Build and launch on Windows:

```powershell
git pull origin android-port
cd android
.\gradlew.bat :app:assembleDebug -PfullGameAssets=true
adb install -r .\app\build\outputs\apk\debug\app-debug.apk
adb logcat -c
adb shell am force-stop com.innovatenestlabs.nanosaur2
adb shell am start -n com.innovatenestlabs.nanosaur2/.NanosaurActivity
Start-Sleep -Seconds 8
adb logcat -d -s SDL:V SDL/APP:V AndroidRuntime:E
```

Verify these native log markers:

- `Nanosaur extracted N original game resources` (initial launch)
- `Nanosaur game assets already installed; extraction skipped` (next launch)
- `Nanosaur Pomme filesystem ready`
- `Nanosaur Android Pomme filesystem probe: READY`
- `Loaded original Nanosaur image`

If Pomme reports a failed FSSpec lookup, the filesystem bridge isn't complete.
Android's app-private path does **not** imply that the desktop engine has been
ported; rendering, input, audio and engine boot still remain separate work.

## Milestone 4 — Original engine/GL4ES compile probes (experimental)

The **verified preview** remains the default build:

```powershell
.\gradlew.bat :app:assembleDebug -PfullGameAssets=true
```

The original engine's files can be compiled as a separate CMake object target
using `-PengineProbe=true`. The original engine still isn't linked to the
preview executable. The CI workflow explicitly builds `nanosaur-engine-probe`
to expose missing desktop OpenGL APIs/other Android issues:

```powershell
.\gradlew.bat :app:assembleDebug -PfullGameAssets=true -PengineProbe=true
```

A separate experimental compatibility library checkout is available:

```powershell
powershell -ExecutionPolicy Bypass -File .\setup-gl4es.ps1
.\gradlew.bat :app:assembleDebug -PfullGameAssets=true -PrendererProbe=true
```

GL4ES: <https://github.com/ptitSeb/gl4es>, MIT license; pinned checkout
`8b1a3da5d2331d0f88b70be25963182d46eed857` is ignored by Git.
This experimental target compiles GL4ES for ARM64/GLES2 but does **not**
substitute it for the GLES3 preview renderer or link it to the game yet.
The engine and GL4ES probes may initially fail until Android compatibility
changes are made. CI marks these audits experimental while keeping the
known-good APK build as a required step.

**Do not install an APK produced by an unsuccessful audit as if the audit
succeeded.** The original 3D game cannot be exercised until the render bridge,
game entry point, Pomme audio, controls, and engine integration are finished.

## Local-only Android gameplay testing policy (21 Nov 2026)

Android development occurs exclusively on `android-port`. **Do not modify
`master`**: it must remain the upstream desktop fork. The `main` branch is
reserved for the fully tested, production-ready Android version after Phase 7;
do not create or merge to it prematurely.

All GitHub Actions workflows were removed from `android-port` at the user's
request. The developer tests builds and runtime directly on Windows and a
physical Android handset. Do not claim CI passed for subsequent commits.

The experimental original gameplay build is:

```powershell
cd E:\Projects\Game\Nanosaur2-Android
git checkout android-port
git pull origin android-port
cd android
.\gradlew.bat :app:assembleDebug -PfullGameAssets=true -PgameplayProbe=true
if ($LASTEXITCODE -ne 0) { throw 'Android game build failed; APK not installed' }
adb install -r .\app\build\outputs\apk\debug\app-debug.apk
adb logcat -c
adb shell am force-stop com.innovatenestlabs.nanosaur2
adb shell am start -n com.innovatenestlabs.nanosaur2/.NanosaurActivity
Start-Sleep -Seconds 12
adb logcat -d -s SDL:V SDL/APP:V AndroidRuntime:E libc:E
```

**Preferences fix:** Android startup now creates
`<SDL_GetAndroidInternalStoragePath()>/Preferences` and assigns it to
`XDG_CONFIG_HOME` before `GameMain()`. Pomme's `FindFolder` expects
this directory to exist even on the first launch. Do not bypass the
original save/load code or store prefs in external public storage.

**Scene diagnostics:** `Source/System/Main.c` includes Android-only log
markers for Toolbox, warmup, global assets, legal screen, intro and menu.
These determine where a dark screen begins. A successful native APK
build is not proof of visible gameplay. The full Phase 4/5 still
requires device verification and functioning controls.

## Phase 5: Android Quick Play / first level test (experimental)

For the first **3D gameplay** test, the `-PgameplayProbe=true` build
previously enabled `NANOSAUR_ANDROID_QUICKPLAY` by default. This bypasses the long
legal/story presentation and the desktop main menu. It launches the actual
original Adventure Level 1 through `PlayGame_Adventure()`, then the
original `InitLevel()` and `PlayLevel()` functions. This is a diagnostic
shortcut, not a finished Android menu or a completed level.

To reproduce the older direct-Level-1 test, pass `-PquickPlay=true` alongside
`-PgameplayProbe=true`. Normal gameplay builds now start at the original menu.

### Temporary touchscreen layout (landscape orientation)

All coordinates refer to proportions of the *screen*, not world space:

- Lower-left half: analog steering. Neutral point near 23% from left,
  68% from top. Drag left/right to yaw and up/down to pitch.
- Right lower-middle: FIRE (right half x=50–78%, lower 45% of screen)
- Far right lower: JETPACK (x=78–100%, lower 45%)
- Right middle: DROP (x=50–78%, y=28–55%), CAMERA (x=78–100%, y=28–55%)
- Right upper: NEXT WEAPON (x=50–78%, y=0–28%), PAUSE (x=78–100%, y=0–28%)

These are **temporary invisible hit zones** to test the native game loop.
Multi-touch is supported. Bluetooth controllers/keyboard remain available.
Visible buttons, calibration, safe-area layout and accessibility are Phase 6.

### Local PowerShell build and runtime test

```powershell
cd E:\Projects\Game\Nanosaur2-Android
git checkout android-port
git pull origin android-port
cd android
.\gradlew.bat :app:assembleDebug -PfullGameAssets=true -PgameplayProbe=true
if ($LASTEXITCODE -ne 0) { throw 'Build failed — do not install old APK' }
adb install -r .\app\build\outputs\apk\debug\app-debug.apk
if ($LASTEXITCODE -ne 0) { throw 'APK install failed' }
adb logcat -c
adb shell am force-stop com.innovatenestlabs.nanosaur2
adb shell am start -n com.innovatenestlabs.nanosaur2/.NanosaurActivity
Start-Sleep -Seconds 15
adb logcat -d -s SDL:V SDL/APP:V AndroidRuntime:E libc:E
```

Look for `Nanosaur Android QUICKPLAY: starting Adventure Level 1`,
`Nanosaur Android level: InitLevel complete`, `Nanosaur Android level:
PlayLevel started` and `Nanosaur Android level: first 3D frame submitted`.
Logs do **not** prove the image is visible or the game is playable. A
real Android device must display the terrain and dinosaur and respond
to steering/fire before Phase 5 can be considered complete.

All GitHub Actions workflows are removed. Keep `master` unchanged and
do not create the production `main` branch until Phase 7 is tested.

## Phase 6: Visible Android touchscreen controls (experimental)

After the user verified that Adventure Level 1 renders in full color and
responds to the existing touch zones, the gameplay build gained a visible
translucent overlay drawn by GL4ES. It uses the original game's font and
retains the previously tested multi-finger input logic.

- Left lower steering pad: drag around its center to pitch and yaw.
- FIRE and JET: press/hold on the lower-right side.
- DROP and CAM: center-right actions.
- NEXT and PAUSE: upper-right actions.
- Button indicators brighten when their touch zone is pressed.
- Finger state clears if the app loses focus or enters the background.

The HUD is rendered during gameplay from the Android level callback; the
original desktop renderer and input mapping remain unchanged. The touch
layout is temporary and still requires safe-area, tablet/phone, pause/resume,
and real-device verification. In particular, the HUD rendering must not
leave native GL client-array state altered after drawing.

### Local test

```powershell
cd E:\Projects\Game\Nanosaur2-Android
git checkout android-port
git pull origin android-port
cd android
.\gradlew.bat :app:assembleDebug -PfullGameAssets=true -PgameplayProbe=true
if ($LASTEXITCODE -ne 0) { throw 'Build failed — do not install the old APK' }
adb install -r .\app\build\outputs\apk\debug\app-debug.apk
if ($LASTEXITCODE -ne 0) { throw 'Install failed' }
adb logcat -c
adb shell am force-stop com.innovatenestlabs.nanosaur2
adb shell am start -n com.innovatenestlabs.nanosaur2/.NanosaurActivity
```

Verify full-color terrain/objects and translucent control circles/labels.
Test steering and simultaneous FIRE + JET, then NEXT, CAM, DROP and PAUSE.
Background the app and resume, checking there are no stuck buttons or
graphics errors. Capture logs with:

```powershell
adb logcat -d -s SDL:V SDL/APP:V AndroidRuntime:E libc:E
```

No GitHub Actions validation is performed. Production `main` is reserved
until Phase 7; `master` remains the unchanged desktop fork.

## Phase 6 normal Android startup and controls — November 2026

The gameplay build now **defaults to original Nanosaur 2 startup**:
opening screens, intro and original title/menu, followed by the Adventure
campaign when selected. The desktop code and other game modes are not changed.
Tap original menu entries using SDL3's synthesized touch-to-mouse clicks.
The Android touch controls are compiled independently of Quick Play, so they
also work after using the normal menu to start a game.

For fast local testing, `-PskipIntro=true` omits only startup opening
cutscenes, but still shows the original menu. `-PquickPlay=true` is an
**explicit debug-only** shortcut directly into Adventure Level 1. These
flags are **OFF** by default even if an earlier CMake build cached them,
because Gradle passes both flags explicitly.

The Android chase camera keeps the dinosaur visible at all times when
switching VIEW, alternating between near/far chase views. A pre-existing
Z-axis camera overshoot typo is fixed for Android only. Transformation and
death animations may still temporarily hide the character intentionally.

The Android controller has a floating left joystick (first touch establishes
neutral), nonlinear deadzone and frame-based smoothing. Right action buttons
require contact near their displayed control, reducing accidental activation.
Touch ownership is independent for multi-finger fire/jet/steering, and touch
state resets on background/focus change. The pause menu and title menu use
normal mouse/menu input rather than synthetic flight controls.

To test normal startup on Windows:

```powershell
cd E:\Projects\Game\Nanosaur2-Android
git checkout android-port
git pull origin android-port
cd android
.\gradlew.bat :app:assembleDebug -PfullGameAssets=true -PgameplayProbe=true
if ($LASTEXITCODE -ne 0) { throw "BUILD FAILED: do not install stale APK" }
adb install -r .\app\build\outputs\apk\debug\app-debug.apk
if ($LASTEXITCODE -ne 0) { throw "APK INSTALL FAILED" }
adb logcat -c
adb shell am force-stop com.innovatenestlabs.nanosaur2
adb shell am start -n com.innovatenestlabs.nanosaur2/.NanosaurActivity
```

Once the original menu appears, tap Play Game > Adventure, test flying with a
floating left thumb, simultaneous FIRE/JET, pause/resume, VIEW zoom and death
respawn. Observe stability after Android app background/resume. Test the same
APK on more than one phone/tablet screen size if available.

**Validation gate:** Neither a successful Gradle build nor source inspection
proves Phase 6 is final. The user must verify the actual menu, gameplay HUD,
steering, scene rendering, pause/resume, and camera behavior on physical
devices before we mark Phase 6 complete. CI remains disabled. The untouched
desktop branch is `master`; `main` remains reserved for the completed
Phase 7 Android release.

### Repeatable local test runner

A local PowerShell runner builds, installs, launches and writes filtered
device logs without GitHub Actions:

```powershell
cd E:\Projects\Game\Nanosaur2-Android
git pull origin android-port
powershell -ExecutionPolicy Bypass -File .\android\test-game.ps1
```

For a faster menu-only test, add `-SkipIntro`. To debug the older
direct-to-Level-1 flow, add `-QuickPlay`. Neither option is enabled in
the default Android build. The output `android/nanosaur-local-test.log`
is ignored by Git. The command deliberately aborts if build/install
fails, preventing an old APK from being mistaken for a successful build.
