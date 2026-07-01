$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) {
  Write-Error "vcvars64.bat not found: $vcvars"
  exit 1
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

Set-Location 'e:/ai/3DPrinter_Qt6/build'

Stop-Process -Name 'OWzxSlicer' -Force -ErrorAction SilentlyContinue
Stop-Process -Name 'cmake', 'ninja', 'link' -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

$env:CMAKE_PREFIX_PATH = "E:\Qt6.10"
$env:Qt6_DIR = "E:\Qt6.10"
$env:PATH = "E:\Qt6.10\bin;$env:PATH"

$renderBenchEnabled = $env:OWZX_RENDER_BENCH -match '^(1|ON|TRUE|YES)$'
$renderBenchSegments = if ($env:OWZX_RENDER_BENCH_SEGMENTS) { $env:OWZX_RENDER_BENCH_SEGMENTS } else { '1000000' }
$renderBenchFrames = if ($env:OWZX_RENDER_BENCH_FRAMES) { $env:OWZX_RENDER_BENCH_FRAMES } else { '240' }
$renderBenchBackend = if ($env:OWZX_RENDER_BENCH_BACKEND) { $env:OWZX_RENDER_BENCH_BACKEND } else { 'auto' }

$cmakeArgs = @(
  '-S', '..',
  '-B', '.',
  '-G', 'Ninja',
  '-DCMAKE_BUILD_TYPE=Release',
  '-DBUILD_LIBSLIC3R=ON',
  '-DLIBSLIC3R_FROM_SOURCE=ON',
  '-DOWZX_QML_GUI=ON',
  '-DBUILD_CLI=ON',
  "-DOWZX_RENDER_BENCH=$(if ($renderBenchEnabled) { 'ON' } else { 'OFF' })",
  '-DQT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT=3.21',
  '-DQt6_DIR=E:/Qt6.10'
)

$configureSucceeded = $false
for ($attempt = 1; $attempt -le 3; ++$attempt) {
  $configureOutput = & cmake @cmakeArgs 2>&1
  $configureText = ($configureOutput | Out-String)
  $lastConfigureExitCode = $LASTEXITCODE
  $configureOutput | Write-Host

  if ($lastConfigureExitCode -eq 0) {
    $configureSucceeded = $true
    break
  }

  if ($configureText -notmatch 'failed recompaction: Permission denied' -or $attempt -eq 3) {
    exit $lastConfigureExitCode
  }

  Write-Host ("CMAKE_RETRY_ATTEMPT=" + $attempt)
  Stop-Process -Name 'OWzxSlicer' -Force -ErrorAction SilentlyContinue
  Stop-Process -Name 'cmake', 'ninja', 'link' -Force -ErrorAction SilentlyContinue
  Start-Sleep -Seconds 2
}

if (-not $configureSucceeded) { exit $lastConfigureExitCode }

# Reduce MSVC memory pressure in large TUs/autogen files
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

Invoke-NinjaTarget 'OWzxSlicer.exe'

# Build test targets (if BUILD_TESTING is ON)
Invoke-NinjaTarget 'E2EWorkflowTests'
Invoke-NinjaTarget 'ViewModelSmokeTests'
Invoke-NinjaTarget 'PrepareSceneDataTests'
Invoke-NinjaTarget 'QmlUiAuditTests'
Invoke-NinjaTarget 'PartPlateTests'
Invoke-NinjaTarget 'owzx-cli'
Invoke-NinjaTarget 'CliTests'
Invoke-NinjaTarget 'test-slice-direct' $false
if ($renderBenchEnabled) {
  Invoke-NinjaTarget 'owzx-render-bench'
}

# Deploy Qt runtime DLLs if not already present
if (-not (Test-Path './Qt6Core.dll')) {
  & 'E:/Qt6.10/bin/windeployqt.exe' --release --qmldir '../src' --no-translations --no-system-d3d-compiler --no-opengl-sw './OWzxSlicer.exe' 2>$null
}

# Deploy MSVC runtime DLLs (vcruntime140, msvcp140, etc.) for standalone execution.
# windeployqt does NOT copy these — they are only available via vcvars PATH.
$msvcRedist = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Redist\MSVC\14.50.35710\x64\Microsoft.VC145.CRT'
if (Test-Path $msvcRedist) {
  Get-ChildItem -Path $msvcRedist -Filter '*.dll' | ForEach-Object {
    if (-not (Test-Path (Join-Path '.' $_.Name))) {
      Copy-Item $_.FullName -Destination '.' -Force
    }
  }
}

# Deploy OCCT (OpenCASCADE) DLLs — the exe imports TK*.dll at load time.
# Source: OrcaSlicer dependency bundle at DEPS_PREFIX/bin/occt/
$occtBinDir = 'E:\ai\3D-Printer\deps\build\OrcaSlicer_dep\usr\local\bin\occt'
if (Test-Path $occtBinDir) {
  Get-ChildItem -Path $occtBinDir -Filter 'TK*.dll' | ForEach-Object {
    if (-not (Test-Path (Join-Path '.' $_.Name))) {
      Copy-Item $_.FullName -Destination '.' -Force
    }
  }
}

# cr_tpms_library.dll is delay-loaded and unused; remove to avoid missing DLL warnings.
Remove-Item 'cr_tpms_library.dll' -ErrorAction SilentlyContinue

if ($renderBenchEnabled) {
  Write-Host "`n[RenderBench] Running Qt RHI render benchmark..." -ForegroundColor Cyan
  $benchExe = './owzx-render-bench.exe'
  if (Test-Path $benchExe) {
    & $benchExe --segments $renderBenchSegments --frames $renderBenchFrames --backend $renderBenchBackend 2>&1 | ForEach-Object { Write-Host "  $_" }
    if ($LASTEXITCODE -ne 0) {
      Write-Host "[RenderBench] Benchmark failed" -ForegroundColor Red
      exit $LASTEXITCODE
    }
    Write-Host "[RenderBench] Benchmark completed" -ForegroundColor Green
  } else {
    Write-Host "[RenderBench] owzx-render-bench.exe not found" -ForegroundColor Red
    exit 1
  }
}

# Run static UI audit tests before launching the app.
Write-Host "`n[PrepareScene] Running Prepare scene data tests..." -ForegroundColor Cyan
$prepareSceneDataExe = './PrepareSceneDataTests.exe'
if (Test-Path $prepareSceneDataExe) {
  & $prepareSceneDataExe 2>&1 | ForEach-Object { Write-Host "  $_" }
  if ($LASTEXITCODE -ne 0) {
    Write-Host "[PrepareScene] Prepare scene data tests failed" -ForegroundColor Red
    exit $LASTEXITCODE
  }
  Write-Host "[PrepareScene] Prepare scene data tests passed" -ForegroundColor Green
} else {
  Write-Host "[PrepareScene] PrepareSceneDataTests.exe not found" -ForegroundColor Red
  exit 1
}

Write-Host "`n[PartPlate] Running PartPlate geometry + arrangement tests..." -ForegroundColor Cyan
$partPlateExe = './PartPlateTests.exe'
if (Test-Path $partPlateExe) {
  & $partPlateExe 2>&1 | ForEach-Object { Write-Host "  $_" }
  if ($LASTEXITCODE -ne 0) {
    Write-Host "[PartPlate] PartPlate tests failed" -ForegroundColor Red
    exit $LASTEXITCODE
  }
  Write-Host "[PartPlate] PartPlate tests passed" -ForegroundColor Green
} else {
  Write-Host "[PartPlate] PartPlateTests.exe not found" -ForegroundColor Red
  exit 1
}

Write-Host "`n[ViewModel] Running viewmodel smoke tests..." -ForegroundColor Cyan
$viewModelSmokeExe = './ViewModelSmokeTests.exe'
if (Test-Path $viewModelSmokeExe) {
  & $viewModelSmokeExe 2>&1 | ForEach-Object { Write-Host "  $_" }
  if ($LASTEXITCODE -ne 0) {
    Write-Host "[ViewModel] ViewModel smoke tests failed" -ForegroundColor Red
    exit $LASTEXITCODE
  }
  Write-Host "[ViewModel] ViewModel smoke tests passed" -ForegroundColor Green
} else {
  Write-Host "[ViewModel] ViewModelSmokeTests.exe not found" -ForegroundColor Red
  exit 1
}

Write-Host "`n[UI] Running QML UI audit tests..." -ForegroundColor Cyan
$uiAuditExe = './QmlUiAuditTests.exe'
if (Test-Path $uiAuditExe) {
  & $uiAuditExe 2>&1 | ForEach-Object { Write-Host "  $_" }
  if ($LASTEXITCODE -ne 0) {
    Write-Host "[UI] QML UI audit tests failed" -ForegroundColor Red
    exit $LASTEXITCODE
  }
  Write-Host "[UI] QML UI audit tests passed" -ForegroundColor Green
} else {
  Write-Host "[UI] QmlUiAuditTests.exe not found" -ForegroundColor Red
  exit 1
}

# Deploy vcpkg runtime DLLs for libs linked via vcpkg (windeployqt doesn't see these).
# noise.dll (libnoise) and draco.dll are imported by libslic3r_from_source.
$vcpkgBin = 'E:\vcpkg\installed\x64-windows\bin'
if (Test-Path $vcpkgBin) {
  foreach ($dll in @('noise.dll', 'draco.dll')) {
    $src = Join-Path $vcpkgBin $dll
    if ((Test-Path $src) -and -not (Test-Path (Join-Path '.' $dll))) {
      Copy-Item $src -Destination '.' -Force
    }
  }
}

$p = Start-Process -FilePath './OWzxSlicer.exe' -WorkingDirectory (Get-Location) -PassThru
Start-Sleep -Seconds 5
if ($p.HasExited) {
  Write-Host ("APP_EXIT_CODE=" + $p.ExitCode)
  exit 1
}
Write-Host ("APP_RUNNING_PID=" + $p.Id)
Stop-Process -Id $p.Id -Force

# ── Run E2E Pipeline Tests (non-blocking) ──
Write-Host "`n[E2E] Running pipeline tests..." -ForegroundColor Cyan
$e2eExe = './E2EWorkflowTests.exe'
if (Test-Path $e2eExe) {
  & $e2eExe 2>&1 | ForEach-Object { Write-Host "  $_" }
  if ($LASTEXITCODE -ne 0) {
    Write-Host "[E2E] Pipeline tests reported failures (non-blocking)" -ForegroundColor Yellow
  } else {
    Write-Host "[E2E] All pipeline tests passed" -ForegroundColor Green
  }
} else {
  Write-Host "[E2E] E2EPipelineTests.exe not found, skipping" -ForegroundColor DarkGray
}

exit 0
