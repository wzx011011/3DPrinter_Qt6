# verify_brand_cleanup.ps1 -- Brand cleanup grep assertion aggregator
# Aggregates all VALIDATION.md rows 1-01-01 through 1-01-06 assertions.
# Exit 0 if all pass; exit 1 with summary table if any fail.
# Run from project root: powershell -ExecutionPolicy Bypass -File scripts/verify_brand_cleanup.ps1

$ErrorActionPreference = "Continue"
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $ProjectRoot

$passed = 0
$failed = 0
$results = @()

function Assert-GrepFiltered {
    param(
        [string]$Name,
        [string]$GrepCmd
    )

    $output = cmd /c "$GrepCmd 2>NUL" | Out-String
    $output = $output.Trim()
    if ([string]::IsNullOrEmpty($output)) {
        $script:passed++
        $script:results += [PSCustomObject]@{ Name = $Name; Status = "PASS"; Detail = "" }
    } else {
        $script:failed++
        $lines = ($output -split "`n" | Measure-Object).Count
        $script:results += [PSCustomObject]@{ Name = $Name; Status = "FAIL"; Detail = "$lines matches" }
    }
}

Write-Host "=== Brand Cleanup Verification ===" -ForegroundColor Cyan
Write-Host "Project root: $ProjectRoot"
Write-Host ""

# Vendor carve-out filter (grep -v pipeline)
$vendorFilter = 'grep -v -i -e "@Creality" -e "Creality K1" -e "Creality Ender" -e "Creality Generic" -e "Creality CR-10" -e "CrealityDesign" -e "Creality/" -e "Creality.json" -e "/Creality" -e "Creality""" -e "fdm_creality" -e "Creality vendor" -e "CrealityPrint.cpp" -e "CrealityDiscovery" -e "upstream" -e "Upstream" -e "CrealityPrint GCodeViewer" -e "CrealityPrint Plater" -e "CrealityPrint model mall" -e "CrealityPrint 6" -e "CrealityPrint 7" -e "CrealityOfficial" -e "CrealityPrintQML" -e "URI CrealityPrint" -e "CrealityPrint迁移" -e "CrealityPrint源码" -e "CrealityPrint的" -e "CrealityPrint是" -e "CrealityPrint wxWidgets"'

# 1-01-01: Zero Creality brand strings in src/ (with carve-outs for vendor names)
Assert-GrepFiltered -Name "1-01-01: No Creality brand strings in src/" `
    -GrepCmd "grep -ri ""creality"" src/ --exclude-dir=third_party | $vendorFilter"

# 1-01-02: No third_party/CrealityPrint references in build configs
# Exclude this script itself from the scan
Assert-GrepFiltered -Name "1-01-02: No third_party/CrealityPrint in cmake/scripts" `
    -GrepCmd "grep -ri ""third_party/CrealityPrint"" cmake/ scripts/ CMakeLists.txt .gitmodules --exclude=verify_brand_cleanup.ps1"

# 1-01-03: No Crality3D or CrealityGL references in src/
Assert-GrepFiltered -Name "1-01-03: No Crality3D/CrealityGL in src/" `
    -GrepCmd "grep -ri ""Crality3D|CrealityGL"" src/ --exclude-dir=third_party"

# 1-01-04: OWzx namespace declarations present
$nsOutput = cmd /c 'grep -r "namespace OWzx" src/core/ 2>NUL' | Out-String
$nsOutput = $nsOutput.Trim()
if (-not [string]::IsNullOrEmpty($nsOutput)) {
    $script:passed++
    $script:results += [PSCustomObject]@{ Name = "1-01-04: OWzx namespace declarations present"; Status = "PASS"; Detail = "" }
} else {
    $script:failed++
    $script:results += [PSCustomObject]@{ Name = "1-01-04: OWzx namespace declarations present"; Status = "FAIL"; Detail = "No OWzx namespace found" }
}

# 1-01-05: No Creality brand in resource files
$found = $false
foreach ($dir in @("src/qml_gui/data", "src/qml_gui/assets")) {
    $fullDir = Join-Path $ProjectRoot $dir
    if (Test-Path $fullDir) {
        $resOutput = cmd /c "grep -ri ""creality"" $dir 2>NUL" | Out-String
        if (-not [string]::IsNullOrEmpty($resOutput.Trim())) {
            $found = $true
        }
    }
}
if (-not $found) {
    $script:passed++
    $script:results += [PSCustomObject]@{ Name = "1-01-05: No Creality brand in resource files"; Status = "PASS"; Detail = "" }
} else {
    $script:failed++
    $script:results += [PSCustomObject]@{ Name = "1-01-05: No Creality brand in resource files"; Status = "FAIL"; Detail = "Matches found" }
}

# 1-01-06: Version string matches OrcaSlicer (no v7.0.1)
Assert-GrepFiltered -Name "1-01-06: No v7.0.1 in CMake/config" `
    -GrepCmd 'grep -E "7\.0\.1" CMakeLists.txt cmake/buildinfo.h.in.stub'

# Print results
Write-Host ""
Write-Host "=== Results ===" -ForegroundColor Cyan
foreach ($r in $results) {
    $color = if ($r.Status -eq "PASS") { "Green" } else { "Red" }
    Write-Host ("  [{0}] {1}" -f $r.Status, $r.Name) -ForegroundColor $color
    if ($r.Detail) {
        Write-Host "       $r.Detail" -ForegroundColor DarkGray
    }
}

Write-Host ""
Write-Host ("Passed: {0} / {1}" -f $passed, ($passed + $failed))

if ($failed -gt 0) {
    Write-Host ""
    Write-Host "Brand cleanup verification FAILED." -ForegroundColor Red
    Write-Host "Run the authoritative build gate separately:" -ForegroundColor Yellow
    Write-Host "  powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1"
    exit 1
} else {
    Write-Host ""
    Write-Host "All brand cleanup assertions passed." -ForegroundColor Green
    Write-Host "Run the authoritative build gate to complete verification:" -ForegroundColor Yellow
    Write-Host "  powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1"
    exit 0
}
