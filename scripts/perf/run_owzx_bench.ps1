# OWzx CLI slice benchmark, the symmetric counterpart of run_orca_bench.ps1
# for the perf evaluation (docs/perf-baseline.md). Slices a model headless
# via build/owzx-cli.exe (real SliceService path) and reports wall time +
# peak working set. Machine-readable line:
#   [OWZX] model=... wall_s=... peak_mib=... gcode_bytes=... exit=...
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File scripts/perf/run_owzx_bench.ps1 `
#       -Model build/perf_models/bench_sphere_400k.stl

param(
    [Parameter(Mandatory = $true)][string]$Model,
    [string]$Exe = "build/owzx-cli.exe",
    [string]$OutDir = "build/perf_out",
    [int]$TimeoutSec = 600,
    # Fairness: pin layer height 0.2 + auto line widths on both sides.
    # Orca's default profile is layer 0.2 with every *_line_width = 0 (auto);
    # the OWzx default process (0.12) carries fixed widths that trip the
    # extrusion_width<=layer_height check at 0.2. owzx-cli expects underscore
    # keys and validates them against the print config schema.
    [string[]]$ExtraArgs = @(
        '--layer_height', '0.2',
        '--line_width', '0',
        '--initial_layer_line_width', '0',
        '--outer_wall_line_width', '0',
        '--inner_wall_line_width', '0',
        '--sparse_infill_line_width', '0',
        '--top_surface_line_width', '0',
        '--support_line_width', '0'
    )
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSCommandPath))
$exeFull = if ([IO.Path]::IsPathRooted($Exe)) { $Exe } else { Join-Path $repo $Exe }
$modelFull = (Resolve-Path $Model).Path
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
Get-ChildItem -Path $OutDir -Filter "*.gcode" -File -ErrorAction SilentlyContinue |
    Remove-Item -Force

$qtBin = "E:\Qt6.10\bin"
$env:Path = "$qtBin;" + $env:Path

Write-Host "[OWZX] exe=$exeFull"
$sw = [Diagnostics.Stopwatch]::StartNew()
$cliOut = Join-Path $OutDir "owzx_cli_stdout.txt"
$cliErr = Join-Path $OutDir "owzx_cli_stderr.txt"
$proc = Start-Process -FilePath $exeFull `
    -ArgumentList (@('--load', $modelFull, '--slice', '0', '--output-dir', $OutDir) + $ExtraArgs) `
    -PassThru -WindowStyle Hidden -RedirectStandardOutput $cliOut -RedirectStandardError $cliErr `
 -WorkingDirectory $repo
$peakBytes = 0L
while (-not $proc.HasExited) {
    if ($sw.Elapsed.TotalSeconds -gt $TimeoutSec) {
        Write-Host "[OWZX] TIMEOUT after ${TimeoutSec}s"
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
$produced = Get-ChildItem -Path $OutDir -Filter "*.gcode" -File -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending
if ($produced) { $gcodeFile = $produced[0].FullName; $gcodeBytes = $produced[0].Length }

Write-Host ("[OWZX] model={0} wall_s={1:F2} peak_mib={2:F1} gcode_bytes={3} exit={4} gcode={5}" -f `
    $modelFull, $sw.Elapsed.TotalSeconds, ($peakBytes / 1MB), $gcodeBytes, $exit, $gcodeFile)
if ($exit -ne 0 -or $gcodeBytes -eq 0) {
    Write-Host "[OWZX] CLI stdout:"
    if (Test-Path $cliOut) { Get-Content $cliOut | Select-Object -Last 15 | ForEach-Object { Write-Host "  $_" } }
    Write-Host "[OWZX] CLI stderr:"
    if (Test-Path $cliErr) { Get-Content $cliErr | Select-Object -Last 15 | ForEach-Object { Write-Host "  $_" } }
    exit 1
}
