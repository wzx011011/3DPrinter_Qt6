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

$cmakeArgs = @(
  '-S', '..',
  '-B', '.',
  '-G', 'Ninja',
  '-DCMAKE_BUILD_TYPE=Release',
  '-DBUILD_LIBSLIC3R=ON',
  '-DLIBSLIC3R_FROM_SOURCE=ON',
  '-DOWZX_QML_GUI=ON',
  '-DBUILD_CLI=ON',
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

ninja -j16 OWzxSlicer.exe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Build test targets (if BUILD_TESTING is ON)
ninja -j16 E2EWorkflowTests 2>$null
ninja -j16 ViewModelSmokeTests 2>$null
ninja -j16 owzx-cli 2>$null
ninja -j16 CliTests 2>$null
ninja -j16 test-slice-direct 2>$null

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
