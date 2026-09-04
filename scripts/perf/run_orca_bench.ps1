# OrcaSlicer / Bambu Studio CLI slice benchmark for the OWzx perf evaluation
# (docs/perf-baseline.md). Slices a model headless via the installed slicer's
# CLI and reports wall time + peak working set. Output line is machine
# readable: [RIVAL] model=... wall_s=... peak_mib=... gcode_bytes=... exit=...
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File scripts/perf/run_orca_bench.ps1 `
#       -Model build/perf_models/bench_sphere_400k.stl [-Exe "C:\Program Files\OrcaSlicer\orca-slicer.exe"]

param(
    [Parameter(Mandatory = $true)][string]$Model,
    [string]$Exe = "C:\Program Files\OrcaSlicer\orca-slicer.exe",
    [string]$OutDir = "build/perf_out",
    [int]$TimeoutSec = 600,
    # Fairness: pin layer height 0.2 on both sides (OWzx default process is
    # 0.12mm, Orca default 0.2mm) and satisfy the relative-E validate guard.
    # The regex (Print.cpp:1058) accepts the space-free "G92E0" form, which
    # avoids Start-Process argument splitting.
    [string[]]$ExtraArgs = @('--layer-change-gcode=G92E0', '--layer-height=0.2')
)

$ErrorActionPreference = "Stop"
$modelFull = (Resolve-Path $Model).Path
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
# Orca CLI names its product plate_<n>.gcode; clearing stale gcode files makes
# the artifact scan unambiguous.
Get-ChildItem -Path $OutDir -Filter "*.gcode" -File -ErrorAction SilentlyContinue |
    Remove-Item -Force
$modelName = [IO.Path]::GetFileNameWithoutExtension($modelFull)
$exeName = [IO.Path]::GetFileNameWithoutExtension($Exe) -replace '[^A-Za-z]', ''
$gcode = Join-Path $OutDir ("{0}_{1}.gcode" -f $exeName, $modelName)
if (Test-Path $gcode) { Remove-Item $gcode -Force }

Write-Host "[RIVAL] exe=$Exe"
$version = ""
try {
    $version = (@(& $Exe --help 2>&1) | Select-Object -First 1) -join ""
} catch { $version = "(unavailable)" }
Write-Host "[RIVAL] banner=$version"

$sw = [Diagnostics.Stopwatch]::StartNew()
$cliOut = Join-Path $OutDir "cli_stdout.txt"
$cliErr = Join-Path $OutDir "cli_stderr.txt"
# OrcaSlicer CLI has no --output; it writes <model>.gcode into --outputdir
# (upstream OrcaSlicer.cpp:1101 reads "outputdir"; the "output|o" option is
# commented out in PrintConfig.cpp:7341-7344).
$proc = Start-Process -FilePath $Exe -ArgumentList (@('--slice', '0', '--outputdir', $OutDir) + $ExtraArgs + @($modelFull)) `
    -PassThru -WindowStyle Hidden -RedirectStandardOutput $cliOut -RedirectStandardError $cliErr
$peakBytes = 0L
while (-not $proc.HasExited) {
    if ($sw.Elapsed.TotalSeconds -gt $TimeoutSec) {
        Write-Host "[RIVAL] TIMEOUT after ${TimeoutSec}s"
        Stop-Process -Id $proc.Id -Force
        exit 2
    }
    try {
        $p = Get-Process -Id $proc.Id -ErrorAction SilentlyContinue
        if ($p) {
            $p.Refresh()
            if ($p.PeakWorkingSet64 -gt $peakBytes) { $peakBytes = $p.PeakWorkingSet64 }
        }
    } catch { }
    Start-Sleep -Milliseconds 100
}
$sw.Stop()
# HasExited=true does not guarantee ExitCode is populated (.NET pitfall);
# WaitForExit() flushes it.
$proc.WaitForExit()
$exit = $proc.ExitCode
$gcodeBytes = 0L
$gcodeFile = ""
# With --outputdir, Orca writes <model_basename>.gcode next to the model's
# name inside the directory.
$expected = Join-Path $OutDir ([IO.Path]::GetFileNameWithoutExtension($modelFull) + ".gcode")
if (Test-Path $expected) { $gcodeFile = $expected; $gcodeBytes = (Get-Item $expected).Length }
else {
    $produced = Get-ChildItem -Path $OutDir -Filter "*.gcode" -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending
    if ($produced) { $gcodeFile = $produced[0].FullName; $gcodeBytes = $produced[0].Length }
}

Write-Host ("[RIVAL] model={0} wall_s={1:F2} peak_mib={2:F1} gcode_bytes={3} exit={4} gcode={5}" -f `
    $modelFull, $sw.Elapsed.TotalSeconds, ($peakBytes / 1MB), $gcodeBytes, $exit, $gcodeFile)
if ($exit -ne 0 -or $gcodeBytes -eq 0) {
    Write-Host "[RIVAL] CLI stdout:"
    if (Test-Path $cliOut) { Get-Content $cliOut | Select-Object -First 25 | ForEach-Object { Write-Host "  $_" } }
    Write-Host "[RIVAL] CLI stderr:"
    if (Test-Path $cliErr) { Get-Content $cliErr | Select-Object -First 25 | ForEach-Object { Write-Host "  $_" } }
    exit 1
}
