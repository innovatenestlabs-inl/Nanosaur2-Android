# Nanosaur II Android — Phase 7 Acceptance and Release Gate

> **Status: Development / testing only.** No claim that all gameplay modes,
> save/load, or release stability has been verified. The original game assets
> and code retain their licensing terms. Commercial publication requires
> separate written commercial distribution rights.

## Branches and local workflow

- `master` remains the unchanged upstream desktop source fork.
- `android-port` contains Android development and real-device testing.
- Reserve a separate `main` branch for **fully validated** Android release
  after Phase 7; do not create it prematurely.
- GitHub Actions have been removed; testing is **local only**.

### Run a local build and basic APK/runtime preflight

```powershell
cd E:\Projects\Game\Nanosaur2-Android
git checkout android-port
git pull origin android-port
powershell -ExecutionPolicy Bypass -File .\android\verify-phase7.ps1
```

For a direct first-level smoke test (skips title/menu):

```powershell
powershell -ExecutionPolicy Bypass -File .\android\verify-phase7.ps1 -QuickPlay
```

The script installs the game and writes:
- `android/nanosaur-local-test.log` — Android startup/error messages
- `android/phase7-local-report.json` — APK assets and observed startup

**A passed preflight is not a full release sign-off.** In particular, the
script does not play levels, press on-screen controls, test audio quality,
verify save/load correctness, or certify phone hardware performance.

## Manual acceptance checklist

### A. Android controls and game UI
- [ ] Right-hand floating joystick moves smoothly: up=climb, down=descend
- [ ] Left FIRE fires on a quick tap and remains usable while steering
- [ ] Holding FIRE charges Sonic Scream, releasing fires a stronger shot
- [ ] Left JET works while steering and firing
- [ ] MORE expands NEXT, DROP, VIEW, PAUSE, and LESS hides those touch targets
- [ ] VIEW never unexpectedly hides the dinosaur
- [ ] HUD is visible, legible and unclipped on at least two phone sizes
- [ ] Main menu navigates through touchscreen taps
- [ ] Screens displaying TAP ANYWHERE advance without a keyboard
- [ ] Original pause menu works and resuming leaves no stuck finger/button
- [ ] App switch, lock/unlock and call interruptions do not corrupt controls

### B. Original gameplay completeness
- [ ] Adventure level 1: entry, eggs, enemies, checkpoint, death and respawn
- [ ] Adventure level 2: level load, textures, gameplay and progression
- [ ] Adventure level 3: level load, textures, gameplay and ending
- [ ] Story/title/menu, transition/credits and end-game screens work
- [ ] Alternate single-player modes still load, if exposed by the menu
- [ ] No blank screens, GL4ES fatal alerts, persistent `0x0502`, or crashes
- [ ] Restart and replay following death/game-over work correctly

### C. Persistence and audio
- [ ] Save Adventure progress to each save slot
- [ ] Force-stop and relaunch, load the save with level/health/items intact
- [ ] Save/delete/overwrite slot flows work without corruption
- [ ] Preferences persist between launches (volume, graphics/settings)
- [ ] Music, ambient SFX, weapons and voice/effect playback work
- [ ] Background/resume recovers audio and does not leave channels running
- [ ] No permissions or external/public storage access are needed for saves

### D. Performance, devices, production preparation
- [ ] Test at least two ARM64 physical Android devices and screen geometries
- [ ] Run 20+ minutes of mixed gameplay; no thermal shutdown or memory crash
- [ ] Measure frame pacing/FPS in all Adventure levels and during transitions
- [ ] Validate landscape rendering, notches/cutouts and Android navigation
- [ ] Review NDK/SDL3/GL4ES/Pomme security and licensing updates
- [ ] Set appropriate production versionCode/versionName (currently dev build)
- [ ] Test a properly signed release APK/AAB on clean devices
- [ ] Obtain written commercial distribution rights before Play Store release
- [ ] Only after all applicable checks pass, create `main` from `android-port`
      while leaving `master` unchanged.

## Known development limitations

- Native GL4ES diagnostics are still present; avoid claiming zero runtime GL
  errors without reviewing device logcat from a playthrough.
- The game began life as a desktop game; multiplayer and gamepad variants
  need their own QA even if Android menus expose them.
- The APK built by `test-game.ps1` is a **debug** APK. It is not a signed,
  licensed, production-ready release.
- `android/app/build.gradle` blocks incomplete release variants to prevent
  accidentally distributing the old asset-preview binary.

## Phase 7 outstanding verification — 21 November 2026

The user reports that the Android game boots and controls work and the
latest local build succeeds. **The following remain unverified in supplied
logs:** all three Adventure levels and endings, save/load across force-stop,
complete audio, stability across Android background/resume, and multi-device
performance. A production-ready verdict cannot be issued on a successful
startup log alone.

Pending Android-only changes to test:
- Resume/submenu taps must never select a second menu entry unintentionally.
- A touch-up is required before another selection; a short selection debounce
  and larger pause-menu row spacing are enabled.
- The original legal/opening animation remains intact. The **independent**
  Innovate Nest Labs adaptation credit appears afterwards during normal
  startup (not when the debug `-SkipIntro` shortcut is enabled).
- Android package versionName is `2.0.0`, versionCode `20000`.
- APK includes upstream CC BY-NC-SA 4.0 license and attribution notice in
  `assets/licenses/` for offline reference.

**Release rights warning:** free distribution does not automatically imply
NonCommercial use; business attribution that doubles as promotion can
constitute commercial advantage. Obtain rights-holder advice/permission
before public release if the use is not clearly noncommercial, and preserve
original copyright notices and ShareAlike source distribution requirements.
`main` must not be created until QA and rights checks are resolved.

## 21 November source-level Adventure and release-readiness audit

See [`RELEASE_AUDIT_2.0.0.md`](RELEASE_AUDIT_2.0.0.md) for detailed
source evidence, status, and production blockers.

- Android normal flight speed has been reduced from 900 to **850**. The
  original desktop game remains 900; jetpack maximum speed is unchanged.
- Adventure HUD shows `LEVEL n/3 - BIOME  EGGS x/y` and retains the
  existing in-level overhead minimap and egg icons.
- `Play Game > LEVEL GUIDE` lists Forest, Desert and Swamp; it is a
  read-only guide, not an unlock-all or persistent completed-chapters map.
- A completed egg quota unlocks an **exit wormhole at an egg wormhole**,
  not necessarily the entry wormhole. The player must approach and enter
  the opened exit to finish the level. Debug logs record this progression.
- The Android APK includes a content SHA-256 signature of the game assets;
  an update with changed asset contents re-extracts game resources even if
  the filenames are identical.
- GL4ES gameplay uses GLES 2.0, so Android's manifest requirement has
  been aligned to GLES 2.0; the optional separate old preview still uses
  a GLES 3 rendering path.
- **Release decision: not yet production-ready.** Full Adventure
  playthrough, saves, audio, performance, signed release artifact,
  third-party notices, and public distribution rights remain open.
- Continue only on `android-port`; keep `master` unchanged, and
  reserve `main` until all mandatory checks have passed.

### Post-egg progression log

After delivering all eggs, enter the new wormhole, then run:

```powershell
adb logcat -d -s SDL:V SDL/APP:V AndroidRuntime:E libc:E |
    Select-String 'Nanosaur Android progression|InitLevel started|Fatal|exception'
```

The sequence should include all-eggs completion, exit spawn, player
entry, level completion, and then a next-level InitLevel (level=1 for
Desert, level=2 for Swamp). The real screen must also show the
progression; a log line alone does not certify a complete level.

## Dual touch control acceptance (2026-11-21)
- [ ] Right top "ARROWS" switches to four direction buttons; "STICK" switches back
- [ ] Default joystick: up climbs, down dives, left/right turn; release neutral
- [ ] D-pad: visibly drawn arrowheads ↑ ↓ ← → point in the correct directions; tapping each drives the corresponding direction
- [ ] D-pad: holding two arrows simultaneously gives diagonal steering
- [ ] Switching modes while touching controls does not cause stuck input
- [ ] FIRE and JET remain visible, responsive and usable with either controller
- [ ] MORE expands DROP, PAUSE, NEXT, VIEW; LESS hides all four
- [ ] MORE/LESS and mode switches do not inadvertently activate gameplay buttons
- [ ] Screen cutouts, landscape aspect ratios, 3D scene and HUD remain readable
- [ ] Pause/resume and Android app background/foreground clear touch input safely

## D-pad visual correction (2026-11-21)
- Direction labels are now vector-drawn arrow icons instead of textual UP/DN/LEFT/RIGHT. Rendering uses the existing OpenGL/GL4ES HUD pass, with screen aspect correction. Touch hit boxes and gameplay direction mapping are unchanged.
- Requires local native compile and on-device visual/gameplay verification before production signoff.

## Mobile gameplay regression verification (2026-11-21)
- [ ] FIRE/JET respond to first press and stay active while fingers drift beyond button centers
- [ ] FIRE and JET work with simultaneous steering; both release immediately on finger-up
- [ ] Pausing/returning from background does not leave stuck actions
- [ ] MORE/LESS does not steal button presses
- [ ] A new Android Adventure starts with 7 spare lives, shield/fuel preserved
- [ ] Android Adventure receives 75% of normal damage; saved games retain their own life counts
- [ ] Egg delivery, exit wormhole, all three levels, audio and saves still work

## 2026-11-23 Android progression and victory regression checks
- [ ] Complete Forest; restart app; CONTINUE ADVENTURE opens Desert
- [ ] Complete Desert; restart app; CONTINUE ADVENTURE opens Swamp
- [ ] Replaying Forest does not overwrite a higher unlocked-level checkpoint
- [ ] No checkpoint → CONTINUE safely stays in the menu
- [ ] End of Swamp → victory and menu, no shutdown
- [ ] QuickPlay ending returns to menu rather than closing Activity
- [ ] Remaining egg radar marker correctly indicates the nearest unsaved nest
- [ ] App update retains on-device checkpoint
