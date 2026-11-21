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
