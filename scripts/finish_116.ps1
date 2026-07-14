# Phase 116-01 targeted finisher: replicates the remaining steps of
# scripts/auto_verify_with_vcvars.ps1 AFTER the main build + most test targets
# linked. Build-rules compliant: same vcvars preamble (PATH-sanitize + Windows-
# Kits fallback copied verbatim from the canonical script), same ninja targets,
# same regression ctest sequence, same 5-second launch liveness check.
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
  Write-Host ('[vcvars] Dropped ' + $dropped.Count + ' PATH entries with spaces+parens') -ForegroundColor Yellow
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
$needKits = ((-not $env:INCLUDE) -or (-not ($env:INCLUDE -match 'ucrt')))
if ($needKits -and (Test-Path $winKitsRoot)) {
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
    Write-Host ('[vcvars] Patched Windows Kits ' + $wkVer) -ForegroundColor Yellow
  }
}

Set-Location 'e:/ai/3DPrinter_Qt6/build'
Stop-Process -Name 'OWzxSlicer' -Force -ErrorAction SilentlyContinue
Stop-Process -Name 'cmake', 'ninja', 'link' -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

$env:CMAKE_PREFIX_PATH = "E:\Qt6.10"
$env:Qt6_DIR = "E:\Qt6.10"
$env:PATH = "E:\Qt6.10\bin;$env:PATH"
$env:CL = "/Zm300 /bigobj $env:CL"

function Invoke-NinjaTarget([string]$Target, [bool]$Required = $true) {
  $knownTargets = ninja -t targets all 2>$null
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  $targetPrefix = $Target + ':'
  $targetExists = $false
  foreach ($knownTarget in $knownTargets) {
    if ($knownTarget.StartsWith($targetPrefix)) {
      $targetExists = $true
      break
    }
  }
  if (-not $targetExists) {
    if ($Required) {
      Write-Host ("[Build] Required target not found: " + $Target) -ForegroundColor Red
      exit 1
    }
    Write-Host ("[Build] Optional target not found, skipping: " + $Target) -ForegroundColor DarkGray
    return
  }
  ninja -j16 $Target
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

# Build remaining test targets (OWzxSlicer.exe already linked in the prior run).
Invoke-NinjaTarget 'PartPlateTests'
Invoke-NinjaTarget 'PreviewParserTests'
Invoke-NinjaTarget 'owzx-cli'
Invoke-NinjaTarget 'CliTests'

# Deploy Qt runtime DLLs if not already present.
if (-not (Test-Path './Qt6Core.dll')) {
  & 'E:/Qt6.10/bin/windeployqt.exe' --release --qmldir '../src' --no-translations --no-system-d3d-compiler --no-opengl-sw './OWzxSlicer.exe' 2>$null
}
$msvcRedist = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Redist\MSVC\14.50.35710\x64\Microsoft.VC145.CRT'
if (Test-Path $msvcRedist) {
  Get-ChildItem -Path $msvcRedist -Filter '*.dll' | ForEach-Object {
    if (-not (Test-Path (Join-Path '.' $_.Name))) {
      Copy-Item $_.FullName -Destination '.' -Force
    }
  }
}
$occtBinDir = 'E:\ai\3D-Printer\deps\build\OrcaSlicer_dep\usr\local\bin\occt'
if (Test-Path $occtBinDir) {
  Get-ChildItem -Path $occtBinDir -Filter 'TK*.dll' | ForEach-Object {
    if (-not (Test-Path (Join-Path '.' $_.Name))) {
      Copy-Item $_.FullName -Destination '.' -Force
    }
  }
}
Remove-Item 'cr_tpms_library.dll' -ErrorAction SilentlyContinue
$vcpkgBin = 'E:\vcpkg\installed\x64-windows\bin'
if (Test-Path $vcpkgBin) {
  foreach ($dll in @('noise.dll', 'draco.dll')) {
    $src = Join-Path $vcpkgBin $dll
    if ((Test-Path $src) -and -not (Test-Path (Join-Path '.' $dll))) {
      Copy-Item $src -Destination '.' -Force
    }
  }
}

# --- Regression ctest sequence (verbatim order from auto_verify_with_vcvars.ps1) ---
Write-Host "`n[PrepareScene] Running Prepare scene data tests..." -ForegroundColor Cyan
$exe = './PrepareSceneDataTests.exe'
if (Test-Path $exe) {
  & $exe 2>&1 | ForEach-Object { Write-Host "  $_" }
  if ($LASTEXITCODE -ne 0) { Write-Host "[PrepareScene] FAILED" -ForegroundColor Red; exit 1 }
  Write-Host "[PrepareScene] tests passed" -ForegroundColor Green
} else { Write-Host "[PrepareScene] exe not found" -ForegroundColor Red; exit 1 }

Write-Host "`n[PartPlate] Running PartPlate tests..." -ForegroundColor Cyan
$exe = './PartPlateTests.exe'
if (Test-Path $exe) {
  & $exe 2>&1 | ForEach-Object { Write-Host "  $_" }
  if ($LASTEXITCODE -ne 0) { Write-Host "[PartPlate] FAILED" -ForegroundColor Red; exit 1 }
  Write-Host "[PartPlate] tests passed" -ForegroundColor Green
} else { Write-Host "[PartPlate] exe not found" -ForegroundColor Red; exit 1 }

Write-Host "`n[ViewModel] Running ViewModel smoke tests..." -ForegroundColor Cyan
$exe = './ViewModelSmokeTests.exe'
if (Test-Path $exe) {
  & $exe 2>&1 | ForEach-Object { Write-Host "  $_" }
  if ($LASTEXITCODE -ne 0) { Write-Host "[ViewModel] FAILED" -ForegroundColor Red; exit 1 }
  Write-Host "[ViewModel] tests passed" -ForegroundColor Green
} else { Write-Host "[ViewModel] exe not found" -ForegroundColor Red; exit 1 }

Write-Host "`n[UI] Running QML UI audit tests..." -ForegroundColor Cyan
$exe = './QmlUiAuditTests.exe'
if (Test-Path $exe) {
  & $exe 2>&1 | ForEach-Object { Write-Host "  $_" }
  if ($LASTEXITCODE -ne 0) { Write-Host "[UI] FAILED" -ForegroundColor Red; exit 1 }
  Write-Host "[UI] tests passed" -ForegroundColor Green
} else { Write-Host "[UI] exe not found" -ForegroundColor Red; exit 1 }

Write-Host "`n[PreviewParser] Running PreviewParser tests..." -ForegroundColor Cyan
$exe = './PreviewParserTests.exe'
if (Test-Path $exe) {
  & $exe 2>&1 | ForEach-Object { Write-Host "  $_" }
  if ($LASTEXITCODE -ne 0) { Write-Host "[PreviewParser] FAILED" -ForegroundColor Red; exit 1 }
  Write-Host "[PreviewParser] tests passed" -ForegroundColor Green
} else { Write-Host "[PreviewParser] exe not found" -ForegroundColor Red; exit 1 }

# --- 5-second launch liveness (verbatim from auto_verify_with_vcvars.ps1) ---
$p = Start-Process -FilePath './OWzxSlicer.exe' -WorkingDirectory (Get-Location) -PassThru
Start-Sleep -Seconds 5
if ($p.HasExited) {
  Write-Host ("APP_EXIT_CODE=" + $p.ExitCode) -ForegroundColor Red
  exit 1
}
Write-Host ("APP_RUNNING_PID=" + $p.Id) -ForegroundColor Green
Stop-Process -Id $p.Id -Force

# --- E2E (non-blocking, mirrors canonical script) ---
Write-Host "`n[E2E] Running pipeline tests..." -ForegroundColor Cyan
$exe = './E2EWorkflowTests.exe'
if (Test-Path $exe) {
  & $exe 2>&1 | ForEach-Object { Write-Host "  $_" }
  if ($LASTEXITCODE -ne 0) {
    Write-Host "[E2E] Pipeline tests reported failures (non-blocking)" -ForegroundColor Yellow
  } else {
    Write-Host "[E2E] All pipeline tests passed" -ForegroundColor Green
  }
}

exit 0
