# Nanosaur II Android — AAB build and closed-test preparation

> Build instructions only. **The game is not cleared for distribution without a finalized license or a separate written approval explicitly covering the proposed closed test.** Pangea has agreed to review a license, but its review alone is not an executed permission grant.

## Icon asset
- Launcher icon: `app/src/main/res/drawable-nodpi/nanosaur_launcher.webp` (144×144 optimized WebP from the user-selected dinosaur artwork). Used for both launcher and round launcher manifest slots.
- Google Play store-listing icon: upload the **original 512×512 PNG** separately in Play Console. Do not use the reduced launcher asset as the store-listing icon.
- The launcher asset is a square bitmap; validate launcher clipping/appearance on round and adaptive launchers in a physical-device build.

## Signing
The previous `android/nanosaur-test.jks` was committed and subsequently removed from the current tree. Treat the key as compromised; removal from the latest commit **does not purge Git history**. Never use it for production/Play upload signing. Keep a new upload key outside Git and make a secure backup.

From Windows PowerShell, create a new key once:

```powershell
New-Item -ItemType Directory -Force "E:\AndroidSigning" | Out-Null
keytool -genkeypair -v `
  -keystore "E:\AndroidSigning\nanosaur-upload.jks" `
  -alias nanosaur-upload `
  -keyalg RSA -keysize 3072 -validity 10000
```

## Build the complete Android game bundle
```powershell
cd E:\Projects\Game\Nanosaur2-Android
git checkout main
git pull origin main
cd android
.\gradlew.bat :app:clean :app:bundleRelease `
  -PfullGameAssets=true `
  -PgameplayProbe=true `
  -PquickPlay=false `
  -PskipIntro=false
if ($LASTEXITCODE -ne 0) { throw "Release AAB build failed" }

$unsigned = Resolve-Path ".\app\build\outputs\bundle\release\app-release.aab"
$signed = Join-Path $unsigned.Path.Replace("app-release.aab","") "nanosaur2-closed-test-signed.aab"
jarsigner -keystore "E:\AndroidSigning\nanosaur-upload.jks" `
  -signedjar $signed $unsigned.Path nanosaur-upload
if ($LASTEXITCODE -ne 0) { throw "AAB signing failed" }
jarsigner -verify -verbose -certs $signed
if ($LASTEXITCODE -ne 0) { throw "AAB signature invalid" }
Write-Host "Signed AAB: $signed"
```

The above Gradle task is expected to create an unsigned AAB because this repository has no `signingConfigs.release` configured. Sign the AAB using `jarsigner` (not `apksigner`). Use Google Play App Signing for distribution.

## Preflight
- Confirm `targetSdk` and `compileSdk` remain API 36.
- Review Play Console's native 16 KB page-size compatibility results for **all** packaged .so libraries; app bundle packaging alignment is not proof of ELF alignment.
- Ensure release build starts in actual menu (not quick-play/preview), all eggs can be delivered, FIRE/JET and steering work, audio works and game saves persist.
- Capture actual in-app screenshots for Play listing. Promotional artwork must not imply gameplay graphics that do not exist in the app.
- Ensure no compromised keystores, passwords, or private data are in the upload. Rotate any previously exposed credentials.

## Closed test
When distribution rights expressly include closed testing:
1. Play Console → Create app → Complete required setup / app content disclosures.
2. Test and release → Testing → Closed testing → Create track; add testers' Google-account emails and choose countries.
3. Upload the **signed** `.aab`, supply release notes and complete Play review.
4. Share the opt-in link with invited testers **after the track becomes available**.
5. If the account is a personal account created after November 13, 2023, Google requires at least 12 testers opted in continuously for at least 14 days before the production-access application. The requirement depends on developer-account type and eligibility.

See official documentation:
- https://developer.android.com/build/building-cmdline
- https://developer.android.com/guide/practices/page-sizes
- https://support.google.com/googleplay/android-developer/answer/14151465

## 2026-11-22 — Play automatic protection upload rejection

Play Console rejected the previous internal-test bundle because Play automatic protection requires **minSdkVersion >= 24**. The app's minimum API was raised from 23 to 24, with versionCode 20001 and versionName 2.0.1 so it can be submitted as a replacement release. Android 6.0 / API 23 devices will no longer be eligible; compileSdk/targetSdk stay 36. The actual bundle must be rebuilt and signed again; the previous AAB is not fixed by changing Git alone.

To inspect the built AAB, use Android Studio's APK Analyzer or `bundletool dump manifest --bundle <signed.aab> --module base` and verify minSdk 24, targetSdk 36, versionCode 20001 before uploading. Release/rights and real-device checks remain applicable.

## 2026-11-23 — Google Play landscape feature declaration (2.0.6 / 20006)

Play Publishing overview flagged `android.hardware.screen.landscape` as a required hardware feature. This is inferred from `android:screenOrientation="landscape"`. The application's base manifest now explicitly declares:

```xml
<uses-feature android:name="android.hardware.screen.landscape" android:required="false" />
```

Keep `android:screenOrientation="landscape"` for the game's actual landscape UI. The optional hardware feature changes Google Play filtering; it does **not** add portrait layouts or guarantee gameplay on devices without usable landscape screen support. Confirm merged manifest and the signed AAB before uploading. Because versionCode 20005 was uploaded previously, rebuild/sign a new AAB at versionCode 20006. The previous 20005 AAB cannot be modified after upload. Verify the Play Console issue disappears on the new release, then test on a landscape phone/tablet before promoting to Closed testing.
