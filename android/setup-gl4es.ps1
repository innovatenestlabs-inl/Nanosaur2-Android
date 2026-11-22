# Run from any directory: powershell -ExecutionPolicy Bypass -File android/setup-gl4es.ps1
# Experimental desktop OpenGL -> OpenGL ES compatibility dependency.
# This does NOT enable the original game renderer by itself.
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$gl4es = Join-Path $root "gl4es"
$expected = "8b1a3da5d2331d0f88b70be25963182d46eed857"

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "Git must be installed and available in PATH."
}
if (-not (Test-Path (Join-Path $gl4es ".git"))) {
    if (Test-Path $gl4es) {
        throw "android/gl4es exists but is not a Git checkout; move or remove it first."
    }
    git clone https://github.com/ptitSeb/gl4es.git $gl4es
    if ($LASTEXITCODE -ne 0) { throw "GL4ES clone failed." }
}
$actual = (git -C $gl4es rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw "GL4ES revision inspection failed." }
if ($actual -ne $expected) {
    git -C $gl4es fetch --depth 1 origin $expected
    if ($LASTEXITCODE -ne 0) { throw "Cannot fetch pinned GL4ES revision." }
    git -C $gl4es checkout --detach $expected
    if ($LASTEXITCODE -ne 0) { throw "Cannot check out pinned GL4ES revision." }
    $actual = (git -C $gl4es rev-parse HEAD).Trim()
}
if ($actual -ne $expected) {
    throw "Unexpected GL4ES revision: $actual (expected $expected)."
}

# Apply audited GL4ES source corrections, without suppressing compiler diagnostics.
$python = Get-Command py -ErrorAction SilentlyContinue
if ($python) {
    & py -3 (Join-Path $root "tools/fix_gl4es_warnings.py")
} else {
    $python = Get-Command python -ErrorAction SilentlyContinue
    if (-not $python) { throw "Python 3 is required to apply pinned GL4ES source fixes." }
    & python (Join-Path $root "tools/fix_gl4es_warnings.py")
}
if ($LASTEXITCODE -ne 0) { throw "Pinned GL4ES warning corrections failed" }

Write-Host "GL4ES source ready at pinned commit $actual (warnings corrected; renderer unchanged)"
