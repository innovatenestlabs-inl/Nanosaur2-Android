# Nanosaur II Android — Production Readiness Review (2026-11-23)

**Assessment: NOT PRODUCTION-READY YET.** This is not a compile failure. The provided Android debug build (v2.0.4) succeeded with 37 executed Gradle tasks, but there is no validated device test for the latest gameplay changes. No release AAB has been built or tested in this review.

## Build warnings assessed

1. GL4ES: dangling else (15+ diagnostics). Pinned local source is corrected by `android/tools/fix_gl4es_warnings.py` invoked by `android/setup-gl4es.ps1`. Each transformation validates the pinned Git revision and exact upstream line before touching the untracked local checkout.
2. GL4ES: `!need->need_texs&(1<<i)` uses the wrong operator precedence. Corrected to `!(need->need_texs & (1<<i))`; this is a real behavior fix and must be renderer-tested.
3. GL4ES: 2147483647 integer constants promoted to float with precision loss. Use double literals before conversion in the integer-query and selection paths.
4. Java: SDL3's Android integration emits a nonfatal deprecated-API note. We have not replaced SDL3 APIs or suppressed the diagnostic. Run `-Xlint:deprecation` if you need detailed Java call sites.
5. Gradle incubating problems-report message is not itself a game compiler warning. Open `android/build/reports/problems/problems-report.html` for details.

## Source checks and remaining risks

- **Egg reset:** `FindAllEggItems` clears `gNumEggsSaved` when a level initializes; no per-egg checkpoint is written. Verify on a real device after force-stop.
- **Level unlock:** `FileJ` (save slot 9) records highest unlocked Adventure level; `CONTINUE ADVENTURE` and replay selection read this. Physical force-stop/relaunch, reinstall/update preservation and invalid-save behavior are not yet verified.
- **End game:** `DoAndroidEndGameMenu` offers retry/return on normal game-over and victory; this cannot intercept a native crash. Test multiple consecutive retries, victory slideshow, and reentering menu.
- **Minimap:** nearest unsaved-egg marker runs in the HUD renderer; verify location and rotation across Forest, Desert and Swamp, and confirm it does not obscure controls.
- **Assets:** 333 resources staged in the supplied build log. This proves packaging preparation, not correctness of every mission or enemy spawn.
- **Versioning:** `versionCode 20005` / `versionName 2.0.5` for the next testing bundle; API min 24 / target 36; ARM64 only.
- **Release signing:** release build is unsigned by design until signed separately with your current private upload key. Do not reuse an old exposed test key.
- **Android 16 KB pages:** linker flags are configured but every final `.so`'s ELF alignment and final AAB packaging must be checked; Google Play's review is a separate gate.
- **Legal:** retain a fully executed rights agreement, any other required port/asset permissions, and the privacy policy/store-listing compliance records. A friendly email or unsigned agreement is not equivalent to full rights clearance.

## Required real-device acceptance

- [ ] Fresh install: Forest is available, higher levels are hidden, Continue is hidden
- [ ] Kill after collecting eggs in Forest without finishing; reopen: Forest with zero delivered eggs
- [ ] Complete Forest; kill app; reopen: Desert unlocked and Continue starts Desert
- [ ] Kill during partially collected Desert; reopen: Desert with zero delivered eggs
- [ ] Replay Forest after Desert unlocked: full egg reset, Desert remains unlocked
- [ ] Complete Desert; reopen: Swamp unlocked, Forest and Desert replay available
- [ ] Complete Swamp: victory slideshow and "PLAY LEVEL AGAIN / MAIN MENU" shown, no process exit
- [ ] Lose all lives in each level: retry and menu options work; repeated retry has no resource leak
- [ ] Combat FIRE/JET, movement, touch pointer ownership, camera, pause and sound work on small and large screens
- [ ] Enemy dinosaur spawns and egg counts allow reaching each mission objective
- [ ] Egg direction marker tracks unsaved nests in all levels; recovered eggs disappear
- [ ] Background/resume, repeated app relaunch, low-memory termination and rotation do not corrupt progress
- [ ] Debug and signed-release builds both pass on hardware
- [ ] Verify every packaged .so meets the 16 KB page-size requirements
- [ ] Legal authorization and Play Console disclosures verified before general distribution

## Local Windows rebuild

```powershell
cd E:\Projects\Game\Nanosaur2-Android
git checkout main
git pull --ff-only origin main
powershell -ExecutionPolicy Bypass -File .\android\setup-gl4es.ps1
if ($LASTEXITCODE -ne 0) { throw 'GL4ES setup failed' }
cd android
.\gradlew.bat :app:clean :app:assembleDebug -PfullGameAssets=true -PgameplayProbe=true -PquickPlay=false -PskipIntro=false
if ($LASTEXITCODE -ne 0) { throw 'Android debug build failed' }
```

To check on-device fatal signals and progress logs, capture after reproducing:

```powershell
adb logcat -d -s SDL:V SDL/APP:V AndroidRuntime:E libc:E DEBUG:E | Select-String 'Nanosaur|progress|Fatal|signal|exception|retry'
```

**No GitHub Actions, Gradle, or hardware tests were run in the change-authoring environment. Do not mark any of the device boxes checked until tested.**
