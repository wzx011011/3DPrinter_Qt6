# generate_baseline.ps1 — Run E2E tests and generate performance baseline
# Usage: powershell -ExecutionPolicy Bypass -File scripts/generate_baseline.ps1

$ErrorActionPreference = "Continue"
$buildDir = "build"
$testExe = "$buildDir\E2EPipelineTests.exe"
$outputDir = "tests\output"
$baselineDir = "tests\data\baseline"

if (-not (Test-Path $testExe)) {
    Write-Host "[ERROR] E2EPipelineTests.exe not found. Build first with auto_verify_with_vcvars.ps1" -ForegroundColor Red
    exit 1
}

# Ensure output directory exists
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

Write-Host "`n=== Generating Performance Baseline ===" -ForegroundColor Cyan

# Run the timing collection test
& $testExe test_slice_timing_collection -outputdir $outputDir 2>&1

if (-not (Test-Path "$outputDir\timing_hotend.json")) {
    Write-Host "[ERROR] timing_hotend.json was not generated" -ForegroundColor Red
    exit 1
}

# Copy output to baseline
New-Item -ItemType Directory -Force -Path $baselineDir | Out-Null
Copy-Item "$outputDir\timing_hotend.json" "$baselineDir\slice_timing_baseline.json" -Force

Write-Host "`n[Baseline] Updated: $baselineDir\slice_timing_baseline.json" -ForegroundColor Green
Write-Host "[Baseline] Review the file and commit if the timings look correct." -ForegroundColor Yellow

# Display the baseline
$baseline = Get-Content "$baselineDir\slice_timing_baseline.json" -Raw | ConvertFrom-Json
Write-Host "`n  Total elapsed: $($baseline.totalElapsedMs) ms"
Write-Host "  G-code size:   $($baseline.gcodeSizeBytes) bytes"
Write-Host "  Layer count:   $($baseline.layerCount)"
Write-Host "  Stages:"
$baseline.stages.PSObject.Properties | ForEach-Object {
    if ($_.Value -gt 0) {
        Write-Host ("    {0,-25} {1,8} ms" -f $_.Name, $_.Value)
    }
}
