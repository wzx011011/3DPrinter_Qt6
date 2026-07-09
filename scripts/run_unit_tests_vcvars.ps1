# One-off helper (Phase 91): source vcvars and incrementally build + run the
# unit-test targets WITHOUT re-running cmake configure (the verify script
# reconfigures every run, which invalidates libslic3r_from_source and pushes
# the full sequence past the 10-min wrapper budget). This is NOT a replacement
# for auto_verify_with_vcvars.ps1 (the canonical build per AGENTS.md); it only
# builds the already-configured test targets incrementally and runs them.
$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) { Write-Error "vcvars64.bat not found"; exit 1 }

$rawPath = $env:PATH
$kept = New-Object System.Collections.Generic.List[string]
foreach ($entry in ($rawPath -split ';')) {
  if ($entry -and ($entry.Contains(' ') -and $entry.Contains('('))) { } else { [void]$kept.Add($entry) }
}
$env:PATH = ($kept -join ';')

$envDump = cmd /c ('"' + $vcvars + '" >nul & set')
foreach ($line in $envDump) {
  $idx = $line.IndexOf('=')
  if ($idx -gt 0) { Set-Item -Path ("env:" + $line.Substring(0, $idx)) -Value $line.Substring($idx + 1) }
}

$winKitsRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'
if (-not (Test-Path $winKitsRoot)) { $winKitsRoot = 'C:\Program Files (x86)\Windows Kits\10' }
if ((-not $env:INCLUDE) -or (-not ($env:INCLUDE -match 'ucrt')) -and (Test-Path $winKitsRoot)) {
  $wkVer = (Get-ChildItem -Path (Join-Path $winKitsRoot 'Include') -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1).Name
  if ($wkVer) {
    $wkIncBase = Join-Path $winKitsRoot "Include\$wkVer"
    $wkLibBase = Join-Path $winKitsRoot "Lib\$wkVer"
    $missingInc = @('shared', 'ucrt', 'um') | Where-Object { $p = Join-Path $wkIncBase $_; (Test-Path $p) -and ($env:INCLUDE -notlike "*$p*") }
    $missingLib = @('ucrt', 'um') | Where-Object { $p = Join-Path $wkLibBase "$_\x64"; (Test-Path $p) -and ($env:LIB -notlike "*$p*") }
    if ($missingInc) { $env:INCLUDE = (($missingInc | ForEach-Object { Join-Path $wkIncBase $_ }) -join ';') + ';' + $env:INCLUDE }
    if ($missingLib) { $env:LIB = (($missingLib | ForEach-Object { Join-Path $wkLibBase "$_\x64" }) -join ';') + ';' + $env:LIB }
    $wkBin = Join-Path $winKitsRoot "Bin\$wkVer\x64"
    if ((Test-Path $wkBin) -and ($env:PATH -notlike "*$wkBin*")) { $env:PATH = "$wkBin;$env:PATH" }
  }
}

Set-Location 'e:/ai/3DPrinter_Qt6/build'
$env:CMAKE_PREFIX_PATH = "E:\Qt6.10"
$env:Qt6_DIR = "E:\Qt6.10"
$env:PATH = "E:\Qt6.10\bin;$env:PATH"
$env:CL = "/Zm300 /bigobj $env:CL"

$targets = @('PrepareSceneDataTests', 'ViewModelSmokeTests', 'QmlUiAuditTests', 'PartPlateTests', 'PreviewParserTests')
Write-Host "[run_unit_tests] Building test targets incrementally (no reconfigure)..."
& ninja -j16 $targets 2>&1 | ForEach-Object { Write-Host $_ }
if ($LASTEXITCODE -ne 0) { Write-Host "[run_unit_tests] BUILD FAILED (exit $LASTEXITCODE)" -ForegroundColor Red; exit $LASTEXITCODE }
Write-Host "[run_unit_tests] Build OK" -ForegroundColor Green

foreach ($t in $targets) {
  $exe = "./$t.exe"
  Write-Host "`n[run_unit_tests] Running $t..." -ForegroundColor Cyan
  if (Test-Path $exe) {
    & $exe 2>&1 | ForEach-Object { Write-Host "  $_" }
    if ($LASTEXITCODE -ne 0) { Write-Host "[run_unit_tests] $t FAILED (exit $LASTEXITCODE)" -ForegroundColor Red; exit $LASTEXITCODE }
    Write-Host "[run_unit_tests] $t PASSED" -ForegroundColor Green
  } else {
    Write-Host "[run_unit_tests] $exe not found" -ForegroundColor Red; exit 1
  }
}
Write-Host "`n[run_unit_tests] All test targets built and passed." -ForegroundColor Green
exit 0
