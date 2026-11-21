# Nanosaur II Android — local Phase 7 APK and device preflight.
# No CI or GitHub Actions. Does NOT assert full game completion.
# Run from repo root:
#   powershell -ExecutionPolicy Bypass -File .\android\verify-phase7.ps1
# Optional direct Level 1 smoke test:
#   powershell -ExecutionPolicy Bypass -File .\android\verify-phase7.ps1 -QuickPlay
param(
    [switch]$QuickPlay,
    [int]$LogSeconds = 20
)

$ErrorActionPreference = 'Stop'
$androidRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repo = Split-Path -Parent $androidRoot
Set-Location $repo

$currentBranch = (& git branch --show-current).Trim()
if ($LASTEXITCODE -ne 0 -or $currentBranch -ne 'android-port') {
    throw "Refusing Phase 7 test outside android-port. Current branch: $currentBranch"
}
$commit = (& git rev-parse HEAD).Trim()
$testRunner = Join-Path $androidRoot 'test-game.ps1'
if (!(Test-Path $testRunner)) { throw "Local test runner missing: $testRunner" }

$runnerArgs = @('-ExecutionPolicy', 'Bypass', '-File', $testRunner, '-LogSeconds', "$LogSeconds")
if ($QuickPlay) {
    $runnerArgs += '-QuickPlay'
} else {
    $runnerArgs += '-SkipIntro'
}
& powershell @runnerArgs
if ($LASTEXITCODE -ne 0) { throw 'Local game build/install/startup failed' }

$apk = Join-Path $androidRoot 'app\build\outputs\apk\debug\app-debug.apk'
if (!(Test-Path $apk)) { throw "APK not found: $apk" }
if (!(Get-Command jar -ErrorAction SilentlyContinue)) {
    throw 'JDK jar executable is unavailable in PATH'
}
$archiveEntries = @(& jar tf $apk)
if ($LASTEXITCODE -ne 0) { throw 'Cannot inspect the APK archive' }
$required = @(
    'assets/Data/System/gamecontrollerdb.txt',
    'assets/nanosaur-data-files.txt',
    'assets/Sprites/maps/level1.jpg',
    'lib/arm64-v8a/libmain.so'
)
foreach ($entry in $required) {
    if ($archiveEntries -notcontains $entry) {
        throw "Incomplete game APK: missing $entry"
    }
}

$logFile = Join-Path $androidRoot 'nanosaur-local-test.log'
$logLines = @(Get-Content $logFile)
$crashes = @($logLines | Select-String -Pattern 'Fatal Alert|FATAL EXCEPTION|Nanosaur II game exception|SIGSEGV|SIGABRT')
$hasMenu = [bool]($logLines | Select-String -Pattern 'Nanosaur Android stage: Main menu starting')
$hasLevel = [bool]($logLines | Select-String -Pattern 'Nanosaur Android level: first 3D frame submitted')
$hasPomme = [bool]($logLines | Select-String -Pattern 'Nanosaur Pomme filesystem ready')
$hasGL4ES = [bool]($logLines | Select-String -Pattern 'Nanosaur Android GL4ES initialized')
$report = [ordered]@{
    timestamp_utc = [DateTime]::UtcNow.ToString('o')
    git_branch = $currentBranch
    git_commit = $commit
    apk_path = $apk
    apk_bytes = (Get-Item $apk).Length
    required_archive_entries_present = $true
    pomme_ready_in_log = $hasPomme
    gl4es_initialized_in_log = $hasGL4ES
    menu_observed_in_log = $hasMenu
    first_3d_frame_observed_in_log = $hasLevel
    fatal_messages_in_log = @($crashes | ForEach-Object { $_.Line })
    physical_device_full_playthrough_passed = $null
    manual_saves_audio_pause_resume_tested = $null
    commercial_rights_obtained = $null
    production_ready = $false
}
$out = Join-Path $androidRoot 'phase7-local-report.json'
$report | ConvertTo-Json -Depth 4 | Set-Content -Encoding UTF8 $out
Write-Host "Phase 7 local preflight report: $out"
Write-Host "Archive checks PASS; Pomme=$hasPomme GL4ES=$hasGL4ES Menu=$hasMenu First3DFrame=$hasLevel"
if ($crashes.Count -gt 0) {
    Write-Warning "Detected $($crashes.Count) fatal log messages; inspect $logFile"
}
Write-Host 'Full levels, touch actions, saving/loading, audio and release rights MUST be manually validated.'
