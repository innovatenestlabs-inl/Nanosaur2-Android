# Nanosaur 2 Android — Milestone 1 bootstrap

This is a **native SDL3/OpenGL ES smoke test**, not yet a playable Android port.
The desktop game code, resources, copyrights and `master` branch are unchanged.

## Prerequisites

- Windows 11, Git, Android Studio and Java 17 (Android Studio's bundled JDK is fine)
- Android SDK Platform 35, SDK Build Tools, NDK and CMake **3.22.1**
- Android Gradle Plugin 8.7.3 and **Gradle 8.9** (locally installed for first-time wrapper setup)
- Internet access on the first build to fetch Gradle/Android dependencies and SDL3

## Fetch SDL3

From the repository root in PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File android/setup-sdl.ps1
```

The script clones SDL3 `release-3.2.8` into ignored `android/SDL` and verifies its commit SHA.
Do **not** copy the desktop prebuilt SDL3 DLL or framework into the Android project.

## Build

Open the `android` directory in Android Studio and allow Gradle to sync. If Android
Studio prompts you to configure the Gradle distribution, select **Gradle 8.9**.
Alternatively, with Gradle 8.9 installed:

```powershell
cd android
gradle wrapper --gradle-version 8.9
.\gradlew.bat :app:assembleDebug
```

The Gradle wrapper JAR is not checked in yet; generating a wrapper with Gradle 8.9
is required for command-line-only bootstrapping.

APK: `android/app/build/outputs/apk/debug/app-debug.apk`

Install with `adb install -r app/build/outputs/apk/debug/app-debug.apk`,
or use Android Studio's Run button on an ARM64 emulator/device supporting GLES 3.0.

## Expected behavior

A **solid teal landscape screen** confirms Android Java entry, SDL3 JNI,
OpenGL ES context and basic draw loop. You will **not** see the dinosaur yet.
If startup fails, collect Android Studio Logcat lines tagged `SDL` or `libc`
and the full Gradle build output.

## Next milestone

Audit and adapt fixed-function OpenGL calls in `Source/3D/OGL_Support.c`,
`Source/3D/MetaObjects.c` and `Source/3D/Atlas.c`. Then link the original
game code, integrate Pomme, map desktop `Data/` into Android asset/file
access and add accelerometer input. Do not copy all game assets into this smoke
test unnecessarily; no game asset is used by the bootstrap screen.

## Copyright

Nanosaur II © Pangea Software; modern port © Iliyas Jorio.
Upstream project: https://github.com/jorio/Nanosaur2
Original CC BY-NC-SA 4.0 license is retained in the repository.
This experimental Android project is for private, noncommercial use.
