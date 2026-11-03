$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location (Join-Path $repoRoot 'build')

# The build does not deploy Qt next to OWzxSlicer.exe; put the Qt 6.12 bin
# directory (same root auto_verify_with_vcvars.ps1 builds against) on PATH,
# otherwise the exe exits immediately on the missing Qt DLLs.
$qtBin = Join-Path $repoRoot '.deps\Qt6.12\6.12.0\msvc2022_64\bin'
if (-not (Test-Path (Join-Path $qtBin 'Qt6Core.dll'))) {
    Write-Host "[quick_run] Qt 6.12 runtime not found at $qtBin" -ForegroundColor Red
    exit 1
}
$env:PATH = $qtBin + ';' + $env:PATH

# RDP does not support OpenGL hardware acceleration
$sessionName = [System.Environment]::GetEnvironmentVariable('SESSIONNAME')
if ($sessionName -match 'RDP') {
    Write-Host "[RDP detected] Using software rendering"
    $env:QT_QUICK_BACKEND = 'software'
} else {
    Write-Host "[Local session] Using hardware rendering"
}

& ".\OWzxSlicer.exe"
