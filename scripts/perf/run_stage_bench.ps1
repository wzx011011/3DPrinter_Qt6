# Efficiency self-test harness: runs the stage-level PerfBench against the
# synthetic benchmark models and writes a machine-readable report.
#
# Usage (from anywhere):
#   powershell -NoProfile -ExecutionPolicy Bypass -File scripts/perf/run_stage_bench.ps1
#
# Env overrides:
#   PERF_BENCH_TIERS   default "400k,2m" (append ",5m,grid" for the heavy tiers)
#   PERF_BENCH_SLICE   default "1" (slice+preview-parse the 400k tier)
#
# Standing rule (docs/perf-baseline.md): re-run this after perf-relevant
# changes; per-stage wall time must not regress beyond noise (~10%).

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$buildDir = Join-Path $repoRoot 'build'
$exe = Join-Path $buildDir 'PerfBench.exe'
$modelsDir = Join-Path $buildDir 'perf_models'
$reportPath = Join-Path $buildDir 'perf_out\stage_report.json'

if (-not (Test-Path $exe)) {
    Write-Error "PerfBench.exe not found at $exe -- run the canonical build first: scripts/auto_verify_with_vcvars.ps1"
}

if (-not (Test-Path (Join-Path $modelsDir 'bench_sphere_400k.stl'))) {
    Write-Host '[perf] generating benchmark models...'
    python (Join-Path $repoRoot 'scripts\perf\gen_benchmark_models.py') $modelsDir
    if ($LASTEXITCODE -ne 0) { Write-Error 'gen_benchmark_models.py failed' }
}

$env:OWZX_PERF_LOG = '1'
$env:PERF_BENCH_MODELS_DIR = $modelsDir
$env:PERF_BENCH_REPORT = $reportPath
if (-not $env:PERF_BENCH_TIERS) { $env:PERF_BENCH_TIERS = '400k,2m' }

# PerfBench links Qt6Test (the deployed app runtime does not carry it);
# resolve the Qt bin dir from the build's CMake cache.
$qtBin = (Get-Content (Join-Path $buildDir 'CMakeCache.txt') |
    Select-String -Pattern '^Qt6_DIR:[^=]*=(.*)$').Matches[0].Groups[1].Value |
    Split-Path -Parent | Split-Path -Parent | Split-Path -Parent
$qtBin = Join-Path $qtBin 'bin'
$env:PATH = "$qtBin;$env:PATH"
Write-Host "[perf] Qt bin: $qtBin"

Write-Host "[perf] running $exe (tiers=$env:PERF_BENCH_TIERS slice=$env:PERF_BENCH_SLICE)"
# Qt logging may not reach a redirected console for this target; the JSON
# stage report is the source of truth. Print it after the run so the
# baseline numbers are visible in the transcript.
& $exe | Out-Null
if ($LASTEXITCODE -ne 0) { Write-Error "PerfBench exited with code $LASTEXITCODE (see $reportPath)" }
Write-Host "[perf] report: $reportPath"
$report = Get-Content $reportPath -Raw | ConvertFrom-Json
foreach ($model in $report.models.PSObject.Properties) {
    Write-Host ("[perf] == {0} ==" -f $model.Name)
    $model.Value.PSObject.Properties | ForEach-Object {
        Write-Host ("[perf]   {0,-26} {1}" -f $_.Name, $_.Value)
    }
}
