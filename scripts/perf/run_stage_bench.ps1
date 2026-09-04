# Stage-level OWzx pipeline benchmark runner (docs/perf-baseline.md).
# Builds nothing itself: run the canonical build first, then this script
# executes build/PerfBench.exe with OWZX_PERF_LOG=1 against the generated
# benchmark models and prints the [BENCH]/[PERF] output.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File scripts/perf/run_stage_bench.ps1 `
#       [-Tiers "400k,2m"] [-IncludeSlice 1]

param(
    [string]$Tiers = "400k,2m",
    [int]$IncludeSlice = 1,
    [string]$Report = "build/perf_out/stage_report.json"
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSCommandPath))
Set-Location $repo

if (-not (Test-Path "build/PerfBench.exe")) {
    Write-Host "[StageBench] build/PerfBench.exe not found - run the canonical build first"
    exit 3
}
if (-not (Test-Path "build/perf_models/bench_sphere_400k.stl")) {
    Write-Host "[StageBench] benchmark models missing - run scripts/perf/gen_benchmark_models.py first"
    exit 3
}

$env:Path = "E:\Qt6.10\bin;" + $env:Path
$env:OWZX_PERF_LOG = "1"
$env:PERF_BENCH_TIERS = $Tiers
$env:PERF_BENCH_SLICE = "$IncludeSlice"
$env:PERF_BENCH_REPORT = $Report

# qInfo output is unreliable through PowerShell console redirection; QtTest's
# own -o file capture (includes QINFO lines, i.e. all [BENCH]/[PERF] rows) is
# the authoritative capture.
$qtTestOut = Join-Path (Split-Path -Parent $Report) "stage_bench_qtest.txt"
& (Resolve-Path "build/PerfBench.exe") -o "$qtTestOut,txt" | Out-Null
$exitCode = $LASTEXITCODE
if (Test-Path $qtTestOut) {
    Write-Host "--- qtest output ($qtTestOut) ---"
    Get-Content $qtTestOut -Encoding UTF8 |
        Where-Object { $_ -match '\[BENCH\]|\[PERF\]|Totals|FAIL' } |
        ForEach-Object { Write-Host $_ }
}
exit $exitCode
