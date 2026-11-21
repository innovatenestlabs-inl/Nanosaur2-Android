# Nanosaur II Android 2.0.0 — source-level production-readiness audit

Date: 2026-11-21
Branch audited: `android-port` (do not modify `master`).
Decision: **NOT YET VERIFIED FOR PRODUCTION / PUBLIC DISTRIBUTION**

## Scope and evidence

Code inspected: Android Gradle, manifest, native CMake/GL4ES/SDL/Pomme
entry points, resource extraction, original Adventure main loop, egg
collection, exit wormhole, player movement, saved games, pause/menu input,
minimap/infobar, level intros and original credits. The user supplied
successful real-device boot and graphical gameplay tests. This inspection
is **not** a substitute for compiling the latest changes and playing all
levels on physical devices. Additional unrelated desktop modules were not
exhaustively reviewed, and multiplayer is not validated.

## What the source supports

### Adventure level completion

`Source/Items/Eggs.c` increments `gNumEggsSaved` only once an egg
finishes entering an egg-recovery wormhole. When the required eggs of
every color are saved, `gOpenPlayerWormhole` becomes true.

`Source/Items/Wormhole.c` then transforms a relevant egg wormhole into
an **exit wormhole**, or spawns an exit when its item is loaded. This is
**not necessarily the wormhole where the dinosaur originally entered
the level**. For an exit to capture the player, the player must be within
1000 world units and in front of the wormhole mouth. When captured,
the dinosaur changes to the `PLAYER_ANIM_ENTERWORMHOLE` animation.

`Source/Player/Player_Terrain.c` completes the exit motion and calls
`StartLevelCompletion(2.0)`. `Source/System/Main.c` then exits the
current level, cleans it up and moves to the next Adventure level:
Forest -> Desert -> Swamp; after Level 3 it runs the win screen.
These code paths exist; **they have not been proven reliable end-to-end
on Android**. No guarantee of 100% reachability can be made without
the three physical-device playthroughs.

### Level map and progression information

The existing `Source/Screens/Infobar.c` already draws an in-level
overhead/minimap and the per-color egg tally. Android now additionally
shows `LEVEL n/3 - BIOME  EGGS x/y` in the Adventure HUD. The original
game shows `ENTER WORMHOLE` when the exit opens.

Android's Play menu now contains `LEVEL GUIDE`, a **read-only**
Adventure listing (Forest, Desert, Swamp); it does not unlock chapters.
Normal progression remains sequential or via the original saved-game
slots. An account-wide, persisted map of which entire chapters have
been completed is **not implemented**; do not imply that it is.

### Movement

`Source/Headers/player.h`: Android `PLAYER_NORMAL_MAX_SPEED`
reduced from 900 to 850 game units (~5.6%). The original desktop game
remains 900, and jetpack maximum remains 2000. Speed changes must be
tested near enemies, checkpoints and the exit wormhole.

## Fixes made during this audit

- Reduce Android-only normal flight speed to 850.
- Show level title and current egg count on-screen.
- Add read-only Level Guide and preserve original overhead minimap.
- Log all-eggs completion, exit creation, player entering exit and
  actual level completion for Android runtime diagnosis.
- Hash each packaged original asset's contents during the Android
  Gradle build, and require a matching extraction fingerprint on-device.
  Previously, updating an asset *without changing its filename* could
  leave stale extracted resources after an APK update.
- Advertise GLES 2.0 as the minimum for GL4ES gameplay (rather than
  GLES 3.0 solely because the old optional preview uses GLES3).
- Keep `master` unchanged. GitHub Actions remain disabled.

## Production gates / unresolved concerns

**P0 — Licensing/rights:** Original code and assets are supplied under
CC BY-NC-SA 4.0; a free price does *not* guarantee a branded corporate
distribution is NonCommercial. Obtain rights-holder confirmation and
review obligations for original work, adaptations, SDL/Pomme/GL4ES and
other embedded libraries before a public release. The app's
`Android Version Prepared by Innovate Nest Labs` credit is independent
and must not imply official endorsement.

**P1 — Full-game acceptance:** All 3 Adventure levels must be finished
on a physical phone, with collected eggs, the opened exit wormhole,
successful level changes, win screen, game-over/restart, and no hidden
GL4ES errors. The reported real-device success so far is startup and
playable dinosaur/graphics, not 3 completed levels.

**P1 — Saving and persistence:** `SaveGame` and `LoadSavedGame`
exist and write user data through the configured app-private preferences
folder, but correctness after force-stop, app update, and save-slot
overwrite/delete is not verified on Android.

**P1 — Audio/lifecycle/performance:** Verify effects, music, audio
focus/background handling, screen locks, interruptions, frame timing,
long sessions, memory, temperature, and at least two Android devices.
Touch HUD, safe-area/cutout and pause confirmation on small screens need
real-device checks.

**P1 — Signed distribution build:** The delivered test is assembled via
`:app:assembleDebug`. The release Gradle configuration has no
user-supplied signing credentials, no verified signed release APK/AAB,
and no documented clean install/update test. A versionName of
`2.0.0` does not make it production-ready.

**P2 — Asset extraction recovery:** Content hash fixes same-filename
update detection, but interruptions, low storage, and corrupt
app-private assets should be fault-injected before claiming robustness.

**P2 — Alternate modes:** Versus/Race/Battle/Capture the Flag
and gamepad behavior are exposed in source but have not been verified
as playable with Android touch alone. Evaluate whether to hide
unsupported modes in a public Android release.

## Local verification

```powershell
cd E:\Projects\Game\Nanosaur2-Android
git checkout android-port
git pull origin android-port
powershell -ExecutionPolicy Bypass -File .\android\test-game.ps1 -SkipIntro
powershell -ExecutionPolicy Bypass -File .\android\verify-phase7.ps1
```

Run the full-game test. Once the eggs are delivered, capture:

```powershell
adb logcat -d -s SDL:V SDL/APP:V AndroidRuntime:E libc:E |
    Select-String 'Nanosaur Android progression|InitLevel started|Fatal|exception'
```

Required logs: `all required eggs collected`, `exit wormhole
spawned`, `player entered exit wormhole`, `Level n completed`,
then `InitLevel started (level=1)` for Desert and level=2 for Swamp.

The logs should be corroborated by the actual visible exit animation
and next-level screens. These tests have not been reported as passing.

## Production branch gate

Do **not** create `main` from `android-port` until the relevant
P0/P1 items are resolved and acceptance evidence is supplied.
Preserve `master` exactly as the original desktop fork.

## 2026-11-21 — Android phone feedback follow-up

- **APK attribution preflight:** the test failed because `app/build.gradle`
  referenced `app/NOTICE-ANDROID.txt` rather than
  `android/NOTICE-ANDROID.txt`. The Gradle Copy input is now
  `file('../NOTICE-ANDROID.txt')`. The verifier still requires the exact
  `assets/licenses/NOTICE-ANDROID.txt` APK entry. A new local device
  build/preflight is required to confirm the fix.
- **Egg collection verification:** original `MoveEgg_IntoWormhole`
  calls `EggWasRetrieved` when the egg reaches the last joint or shrinks
  away. `EggWasRetrieved` increments the saved count and deletes that egg;
  its Adventure branch unlocks `gOpenPlayerWormhole` once every required
  color reaches its quota. The original exit spawning, capture, fade and
  next-level functions remain intact. This supports the algorithm but
  does not prove real-device completion of all three stages.
- **Lives indicator:** `Infobar_DrawLives` already renders one life
  sprite per free life. Android additionally renders `LIVES: N` to help
  players understand remaining lives on small screens. Confirm on
  device that it doesn't overlap controls or crop with display cutouts.
- **Multiplayer trap:** the Android title Play menu now hides the
  two-controller versus entry because two-player touch HUD support is
  not implemented. If another path reaches LocalGather without two
  controllers, Android returns to menu instead of trapping the user.
  Desktop behavior remains untouched.
- **Mobile prompts:** Android's ESC-to-go-back string now displays
  `TAP BACK TO RETURN` and missing controllers prompt is reworded.
  All actual back affordances require device testing; a translated
  string alone is not evidence of a usable touch-back button.
- **Branding:** app label `Nanosaur II: Hatchling`, custom Android vector
  launcher icon, and existing versionCode 20000/versionName 2.0.0.
  This branding preserves the original Nanosaur identity and doesn't
  represent original copyrights as an Innovate Nest Labs creation.

**Decision remains: not Play Store ready.** A signed AAB, rights review,
actual three-level physical-device progression, save/load, and lifecycle
performance acceptance are outstanding.
