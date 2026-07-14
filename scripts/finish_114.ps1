# Finish-only script for Phase 114: assumes libslic3r_from_source is already
# built. Runs the env preamble, then ninja OWzxSlicer + test targets + ctest.
# Single foreground command so there is no separate polling process to abort.
$ErrorActionPreference = 'Continue'

$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) { Write-Error "vcvars64.bat not found"; exit 1 }

# --- PATH sanitize (verbatim from auto_verify_with_vcvars.ps1) ---
$rawPath = $env:PATH
$kept = New-Object System.Collections.Generic.List[string]
foreach ($entry in ($rawPath -split ';')) {
  if ($entry -and ($entry.Contains(' ') -and $entry.Contains('('))) { } else {
    [void]$kept.Add($entry)
  }
}
$env:PATH = ($kept -join ';')

$envDump = cmd /c ('"' + $vcvars + '" >nul & set')
foreach ($line in $envDump) {
  $idx = $line.IndexOf('=')
  if ($idx -gt 0) {
    Set-Item -Path ("env:" + $line.Substring(0, $idx)) -Value $line.Substring($idx + 1)
  }
}

# --- Windows Kits fallback ---
$winKitsRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'
if (-not (Test-Path $winKitsRoot)) { $winKitsRoot = 'C:\Program Files (x86)\Windows Kits\10' }
if ((-not $env:INCLUDE) -or (-not ($env:INCLUDE -match 'ucrt')) -and (Test-Path $winKitsRoot)) {
  $wkVer = (Get-ChildItem -Path (Join-Path $winKitsRoot 'Include') -Directory -ErrorAction SilentlyContinue |
    Sort-Object Name -Descending | Select-Object -First 1).Name
  if ($wkVer) {
    $wkIncBase = Join-Path $winKitsRoot "Include\$wkVer"
    $wkLibBase = Join-Path $winKitsRoot "Lib\$wkVer"
    $missingInc = @('shared', 'ucrt', 'um') | Where-Object {
      $p = Join-Path $wkIncBase $_; (Test-Path $p) -and ($env:INCLUDE -notlike "*$p*") }
    $missingLib = @('ucrt', 'um') | Where-Object {
      $p = Join-Path $wkLibBase "$_\x64"; (Test-Path $p) -and ($env:LIB -notlike "*$p*") }
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
$env:CL = "/bigobj $env:CL"

Write-Host "=== ninja OWzxSlicer ===" -ForegroundColor Cyan
& ninja OWzxSlicer 2>&1 | Write-Host
$exeExit = $LASTEXITCODE
Write-Host "[ninja OWzxSlicer exit=$exeExit]" -ForegroundColor $(if ($exeExit -eq 0) {'Green'} else {'Red'})
if ($exeExit -ne 0) { exit $exeExit }

Write-Host "=== ninja -j2 PartPlateTests QmlUiAuditTests ===" -ForegroundColor Cyan
& ninja -j2 PartPlateTests QmlUiAuditTests 2>&1 | Write-Host
$testBuildExit = $LASTEXITCODE
Write-Host "[ninja tests exit=$testBuildExit]" -ForegroundColor $(if ($testBuildExit -eq 0) {'Green'} else {'Red'})
if ($testBuildExit -ne 0) { exit $testBuildExit }

Write-Host "=== ctest -R PartPlateTests + QmlUiAuditTests ===" -ForegroundColor Cyan
& ctest -C Release --output-on-failure -R "^(PartPlateTests|QmlUiAuditTests)$" 2>&1 | Write-Host
$ctestExit = $LASTEXITCODE
Write-Host "[ctest exit=$ctestExit]" -ForegroundColor $(if ($ctestExit -eq 0) {'Green'} else {'Red'})
if ($ctestExit -ne 0) { exit $ctestExit }

Write-Host "=== FINISH OK (OWzxSlicer + tests + ctest) ===" -ForegroundColor Green
exit 0
