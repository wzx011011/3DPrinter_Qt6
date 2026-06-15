# Quick incremental build — skips cmake configure, smoke test, and test builds.
# Usage: powershell -ExecutionPolicy Bypass -File scripts/quick_build.ps1 [-run] [-j N]
#   -run    Launch app after build succeeds
#   -j N    Parallel jobs (default: 16)

param([switch]$run, [int]$jobs = 16)

$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) {
  Write-Error "vcvars64.bat not found: $vcvars"
  exit 1
}

$envDump = cmd /c ('"' + $vcvars + '" >nul & set')
foreach ($line in $envDump) {
  $idx = $line.IndexOf('=')
  if ($idx -gt 0) {
    Set-Item -Path ("env:" + $line.Substring(0, $idx)) -Value $line.Substring($idx + 1)
  }
}

Set-Location 'e:/ai/3DPrinter_Qt6/build'

Stop-Process -Name 'OWzxSlicer' -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 300

$env:CL = "/Zm300 /bigobj $env:CL"

Write-Host "[quick_build] ninja -j$jobs OWzxSlicer.exe" -ForegroundColor Cyan
& ninja "-j$jobs" OWzxSlicer.exe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "[quick_build] done ($('{0:N1}' -f ((Get-ChildItem OWzxSlicer.exe).Length / 1MB)) MB)" -ForegroundColor Green

if ($run) {
  Write-Host "[quick_build] launching..." -ForegroundColor Cyan
  Start-Process -FilePath './OWzxSlicer.exe' -WorkingDirectory (Get-Location)
}
