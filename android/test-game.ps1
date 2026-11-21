# Android local-device build/test runner. No GitHub Actions.
# Example: powershell -ExecutionPolicy Bypass -File .\android\test-game.ps1
#         powershell -ExecutionPolicy Bypass -File .\android\test-game.ps1 -SkipIntro
#         powershell -ExecutionPolicy Bypass -File .\android\test-game.ps1 -QuickPlay
param(
    [switch]$QuickPlay,
    [switch]$SkipIntro,
    [int]$LogSeconds = 15
)

$ErrorActionPreference = 'Stop'
$repoAndroid = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $repoAndroid

if (!(Get-Command adb -ErrorAction SilentlyContinue)) {
    throw 'adb is not available in PATH. Install Android platform-tools first.'
}
$devices = @(adb devices | Select-String '^\S+\s+device$')
if ($LASTEXITCODE -ne 0 -or $devices.Count -eq 0) {
    throw 'No authorized Android device found. Check adb devices and USB/wireless debugging.'
}

Write-Host 'Building original Nanosaur II Android game ...'
$quick = if ($QuickPlay) { 'true' } else { 'false' }
$skip = if ($SkipIntro) { 'true' } else { 'false' }
& .\gradlew.bat ':app:assembleDebug' '-PfullGameAssets=true' '-PgameplayProbe=true' "-PquickPlay=$quick" "-PskipIntro=$skip"
if ($LASTEXITCODE -ne 0) {
    throw 'Native game build FAILED. Refusing to install an older APK.'
}

$apk = Join-Path $repoAndroid 'app\build\outputs\apk\debug\app-debug.apk'
if (!(Test-Path $apk)) { throw "APK missing: $apk" }

adb install -r $apk
if ($LASTEXITCODE -ne 0) { throw 'ADB install FAILED' }

adb logcat -c
if ($LASTEXITCODE -ne 0) { throw 'Unable to clear Android logcat' }
adb shell am force-stop com.innovatenestlabs.nanosaur2
adb shell am start -n com.innovatenestlabs.nanosaur2/.NanosaurActivity
if ($LASTEXITCODE -ne 0) { throw 'Android app launch FAILED' }

Write-Host 'Inspect the phone: original intro/menu or quickplay, then flight controls and camera.'
if ($LogSeconds -gt 0) { Start-Sleep -Seconds $LogSeconds }
$logFile = Join-Path $repoAndroid 'nanosaur-local-test.log'
adb logcat -d -s SDL:V SDL/APP:V AndroidRuntime:E libc:E | Set-Content -Encoding UTF8 $logFile
Write-Host "Saved app logs: $logFile"
Get-Content $logFile | Select-String 'Nanosaur Android|GL4ES|Fatal|error|exception' | Select-Object -Last 75
