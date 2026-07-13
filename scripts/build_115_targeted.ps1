# Targeted build for Phase 115 (MEASURE-04): compile + link just the touched
# objects (EditorViewModel, RhiViewport) and the owzx_app_core / OWzxSlicer /
# QmlUiAuditTests targets without a full clean rebuild. Phase 115 adds NO new
# source files (it wires existing files), so no CMake reconfigure is needed
# unless the build dir is stale. Reuses the env preamble (PATH sanitize +
# Windows-Kits fallback) verbatim from auto_verify_with_vcvars.ps1.
#
# Usage:  powershell -ExecutionPolicy Bypass -File scripts/build_115_targeted.ps1

$ErrorActionPreference = 'Continue'

$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) {
  Write-Error "vcvars64.bat not found: $vcvars"
  exit 1
}

# --- PATH sanitize (verbatim from auto_verify_with_vcvars.ps1) ---
$rawPath = $env:PATH
$kept = New-Object System.Collections.Generic.List[string]
$dropped = New-Object System.Collections.Generic.List[string]
foreach ($entry in ($rawPath -split ';')) {
  if ($entry -and ($entry.Contains(' ') -and $entry.Contains('('))) {
    [void]$dropped.Add($entry)
  } else {
    [void]$kept.Add($entry)
  }
}
if ($dropped.Count -gt 0) {
  $env:PATH = ($kept -join ';')
  Write-Host ('[vcvars] Dropped ' + $dropped.Count + ' PATH entr' + $(if ($dropped.Count -eq 1) { 'y' } else { 'ies' }) + ' with spaces+parens that break vcvars64.bat batch parsing:') -ForegroundColor Yellow
  foreach ($d in $dropped) { Write-Host ('[vcvars]   - ' + $d) -ForegroundColor DarkGray }
}

$envDump = cmd /c ('"' + $vcvars + '" >nul & set')
foreach ($line in $envDump) {
  $idx = $line.IndexOf('=')
  if ($idx -gt 0) {
    $name = $line.Substring(0, $idx)
    $value = $line.Substring($idx + 1)
    Set-Item -Path ("env:" + $name) -Value $value
  }
}

# --- Windows Kits fallback (verbatim from auto_verify_with_vcvars.ps1) ---
$winKitsRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'
if (-not (Test-Path $winKitsRoot)) { $winKitsRoot = 'C:\Program Files (x86)\Windows Kits\10' }
if ((-not $env:INCLUDE) -or (-not ($env:INCLUDE -match 'ucrt')) -and (Test-Path $winKitsRoot)) {
  $wkVer = (Get-ChildItem -Path (Join-Path $winKitsRoot 'Include') -Directory -ErrorAction SilentlyContinue |
    Sort-Object Name -Descending | Select-Object -First 1).Name
  if ($wkVer) {
    $wkIncBase = Join-Path $winKitsRoot "Include\$wkVer"
    $wkLibBase = Join-Path $winKitsRoot "Lib\$wkVer"
    $missingInc = @('shared', 'ucrt', 'um') | Where-Object {
      $p = Join-Path $wkIncBase $_
      (Test-Path $p) -and ($env:INCLUDE -notlike "*$p*")
    }
    $missingLib = @('ucrt', 'um') | Where-Object {
      $p = Join-Path $wkLibBase "$_\x64"
      (Test-Path $p) -and ($env:LIB -notlike "*$p*")
    }
    if ($missingInc) {
      $env:INCLUDE = (($missingInc | ForEach-Object { Join-Path $wkIncBase $_ }) -join ';') + ';' + $env:INCLUDE
    }
    if ($missingLib) {
      $env:LIB = (($missingLib | ForEach-Object { Join-Path $wkLibBase "$_\x64" }) -join ';') + ';' + $env:LIB
    }
    $wkBin = Join-Path $winKitsRoot "Bin\$wkVer\x64"
    if ((Test-Path $wkBin) -and ($env:PATH -notlike "*$wkBin*")) {
      $env:PATH = "$wkBin;$env:PATH"
    }
    Write-Host ('[vcvars] Patched Windows Kits ' + $wkVer + ' paths into INCLUDE/LIB (vcvars SDK detection failed)') -ForegroundColor Yellow
  }
}

Set-Location 'e:/ai/3DPrinter_Qt6/build'

Stop-Process -Name 'OWzxSlicer' -Force -ErrorAction SilentlyContinue
Stop-Process -Name 'cmake', 'ninja', 'link' -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

$env:CMAKE_PREFIX_PATH = "E:\Qt6.10"
$env:Qt6_DIR = "E:\Qt6.10"
$env:PATH = "E:\Qt6.10\bin;$env:PATH"

# Reduce MSVC memory pressure in large TUs/autogen files
$env:CL = "/Zm300 /bigobj $env:CL"

Write-Host "=== Phase 115 targeted build: owzx_app_core + OWzxSlicer + QmlUiAuditTests ===" -ForegroundColor Cyan

# No CMakeLists.txt change in Phase 115 (no new sources -- wiring only), so
# skip the cmake reconfigure. If the build dir is stale, reconfigure manually.
Write-Host "=== ninja -j2 owzx_app_core ===" -ForegroundColor Cyan
& ninja -j2 owzx_app_core 2>&1 | Write-Host
$appCoreExit = $LASTEXITCODE
if ($appCoreExit -ne 0) {
  Write-Host "[Build] ninja owzx_app_core FAILED (exit $appCoreExit)" -ForegroundColor Red
  exit $appCoreExit
}

Write-Host "=== ninja OWzxSlicer ===" -ForegroundColor Cyan
& ninja OWzxSlicer 2>&1 | Write-Host
$exeExit = $LASTEXITCODE
if ($exeExit -ne 0) {
  Write-Host "[Build] ninja OWzxSlicer FAILED (exit $exeExit)" -ForegroundColor Red
  exit $exeExit
}

Write-Host "=== ninja -j2 PartPlateTests QmlUiAuditTests ===" -ForegroundColor Cyan
& ninja -j2 PartPlateTests QmlUiAuditTests 2>&1 | Write-Host
$testBuildExit = $LASTEXITCODE
if ($testBuildExit -ne 0) {
  Write-Host "[Build] ninja test targets FAILED (exit $testBuildExit)" -ForegroundColor Red
  exit $testBuildExit
}

Write-Host "=== ctest -R PartPlateTests + QmlUiAuditTests ===" -ForegroundColor Cyan
& ctest -C Release --output-on-failure -R "^(PartPlateTests|QmlUiAuditTests)$" 2>&1 | Write-Host
$ctestExit = $LASTEXITCODE
if ($ctestExit -ne 0) {
  Write-Host "[Build] ctest FAILED (exit $ctestExit)" -ForegroundColor Red
  exit $ctestExit
}

Write-Host "=== BUILD OK (owzx_app_core + OWzxSlicer + tests + ctest) ===" -ForegroundColor Green
exit 0
