# Run from any directory: powershell -ExecutionPolicy Bypass -File android/setup-sdl.ps1
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$sdl = Join-Path $root "SDL"
$tag = "release-3.2.8"
$expected = "f6864924f76e1a0b4abaefc76ae2ed22b1a8916e"

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "Git must be installed and available in PATH."
}

if (-not (Test-Path (Join-Path $sdl ".git"))) {
    if (Test-Path $sdl) {
        throw "android/SDL exists but is not a Git checkout; remove or rename it first."
    }
    git clone --branch $tag --depth 1 https://github.com/libsdl-org/SDL.git $sdl
    if ($LASTEXITCODE -ne 0) { throw "SDL3 clone failed." }
}

$actual = (git -C $sdl rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $actual -ne $expected) {
    throw "Unexpected SDL3 revision: $actual. Expected $expected ($tag)."
}

Write-Host "SDL3 ready at pinned commit $actual"
