# GL4ES warning corrections (pinned dependency)

The generated native build warnings originate in the local checkout at `android/gl4es`, not files tracked in this repository. That checkout is pinned to `8b1a3da5d2331d0f88b70be25963182d46eed857` and is intentionally untracked. `android/setup-gl4es.ps1` now applies the audited line-level corrections using Python 3 after pinning the revision.

Corrections: clarify dangling-else ownership with braces, fix `!(need->need_texs & (1<<i))` so texture-stage filtering actually works, use double-precision numeric constants before integer conversion, and avoid changing any renderer behavior beyond the identified bug. The updater verifies each original line against Git's pristine pinned source, refuses unknown revisions or unrelated local edits, and is idempotent.

Run from the repository root before rebuilding:
```powershell
powershell -ExecutionPolicy Bypass -File .\android\setup-gl4es.ps1
cd android
.\gradlew.bat :app:clean :app:assembleDebug -PfullGameAssets=true -PgameplayProbe=true -PquickPlay=false -PskipIntro=false
```

Java deprecation notices originate in SDL3/Android Java sources and are independent of the GL4ES corrections. The Gradle incubating problems-report notice is also independent of game rendering.

**No physical-device runtime test or clean Android Gradle build was executed when committing this change.** Rebuild and compare warnings; specifically test textures, fog, lighting, HUD, controls and all levels before public distribution.
