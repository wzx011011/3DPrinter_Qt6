function Resolve-VcVars64 {
  $candidates = New-Object System.Collections.Generic.List[string]
  $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
  if (Test-Path $vswhere) {
    $installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null |
      Select-Object -First 1
    if ($LASTEXITCODE -eq 0 -and $installPath) {
      [void]$candidates.Add((Join-Path $installPath 'VC\Auxiliary\Build\vcvars64.bat'))
    }
  }

  foreach ($edition in @('Enterprise', 'Professional', 'Community', 'BuildTools')) {
    [void]$candidates.Add("C:\Program Files\Microsoft Visual Studio\2022\$edition\VC\Auxiliary\Build\vcvars64.bat")
  }
  [void]$candidates.Add('C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat')

  foreach ($candidate in ($candidates | Select-Object -Unique)) {
    if ($candidate -and (Test-Path $candidate)) {
      return $candidate
    }
  }

  return $null
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir '..')
$buildDir = Join-Path $repoRoot 'build'

$vcvars = Resolve-VcVars64
if (-not $vcvars) {
  Write-Error "vcvars64.bat not found. Install Visual Studio 2022 C++ tools or ensure vswhere can find Microsoft.VisualStudio.Component.VC.Tools.x86.x64."
  exit 1
}

# Sanitize PATH before invoking vcvars64.bat. vcvars is a batch script and
# parses the inherited PATH with weak quoting; entries that contain BOTH spaces
# and parentheses (e.g. "F:\Program Files (x86)\VMware\VMware Workstation\bin")
# break its parser and abort before INCLUDE/LIB are exported, which surfaces as
# C1083 "cannot open <vector>" on the first real C++ compile (libslic3r_cgal).
# Drop only those offending entries; the system PATH itself is left untouched.
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
$vcvarsPath = $null
foreach ($line in $envDump) {
  $idx = $line.IndexOf('=')
  if ($idx -gt 0) {
    $name = $line.Substring(0, $idx)
    $value = $line.Substring($idx + 1)
    if ($name -ieq 'PATH') {
      # The Codex shell can expose both PATH and Path; prefer vcvars' uppercase PATH.
      if (($name -ceq 'PATH') -or (-not $vcvarsPath)) {
        $vcvarsPath = $value
      }
      continue
    }
    Set-Item -Path ("env:" + $name) -Value $value
  }
}
if ($vcvarsPath) {
  Set-Item -Path 'env:PATH' -Value $vcvarsPath
  Set-Item -Path 'env:Path' -Value $vcvarsPath
}

$vsInstallRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $vcvars)))
$vsNinjaDir = Join-Path $vsInstallRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja'
if (Test-Path (Join-Path $vsNinjaDir 'ninja.exe')) {
  $pathWithoutVsNinja = $env:PATH -split ';' | Where-Object { $_ -and ($_ -ine $vsNinjaDir) }
  $env:PATH = "$vsNinjaDir;" + ($pathWithoutVsNinja -join ';')
}

# Windows Kits fallback. When vcvars64.bat trips on a polluted PATH (see the
# sanitize block above), its findstr-based Windows SDK detection silently
# fails and the UCRT/UM headers (<stdio.h>, <Windows.h>) are NOT added to
# INCLUDE/LIB even though the MSVC STL (<vector>) is. That surfaces as C1083
# "cannot open <stdio.h>" on the first C source (mcut/shewchuk.c). Detect the
# gap and append the installed Windows Kits paths explicitly so the build does
# not depend on vcvars's fragile SDK auto-detection.
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

# MSVC compiler fallback. vsdevcmd.bat (invoked by vcvars64.bat) aborts and
# rolls back the WHOLE environment when any optional ext extension fails
# (clang_cl.bat / cmake.bat / ConnectionManagerExe.bat routinely fail when
# those components are absent or conflict with a standalone CMake on PATH).
# The rollback leaves cl.exe out of PATH, so `where cl` returns empty and a
# from-scratch CMake configure (no cached CMAKE_C_COMPILER) fails with
# "No CMAKE_C_COMPILER could be found". When a valid cache exists this is
# masked, but a clean configure exposes it. Detect the gap and inject the
# installed MSVC Hostx64/x64 bin + INCLUDE/LIB so the build does not depend
# on vcvars's all-or-nothing extension chain.
$clCheck = cmd /c 'where cl 2>nul & echo CL_EXIT=%ERRORLEVEL%'
if (($clCheck -join "`n") -notmatch '[\\/]cl\.exe') {
  $msvcToolsRoot = Join-Path $vsInstallRoot 'VC\Tools\MSVC'
  $msvcVer = (Get-ChildItem -Path $msvcToolsRoot -Directory -ErrorAction SilentlyContinue |
    Sort-Object Name -Descending | Select-Object -First 1).Name
  if ($msvcVer) {
    $msvcBin = Join-Path $msvcToolsRoot "$msvcVer\bin\Hostx64\x64"
    $msvcInc = Join-Path $msvcToolsRoot "$msvcVer\include"
    $msvcLib = Join-Path $msvcToolsRoot "$msvcVer\lib\x64"
    if ((Test-Path (Join-Path $msvcBin 'cl.exe')) -and ($env:PATH -notlike "*$msvcBin*")) {
      $env:PATH = "$msvcBin;$env:PATH"
    }
    if ((Test-Path $msvcInc) -and ($env:INCLUDE -notlike "*$msvcInc*")) {
      $env:INCLUDE = "$msvcInc;$env:INCLUDE"
    }
    if ((Test-Path $msvcLib) -and ($env:LIB -notlike "*$msvcLib*")) {
      $env:LIB = "$msvcLib;$env:LIB"
    }
    # Drop Strawberry Perl's bin from PATH. Its GNU ld.exe shadows the MSVC
    # link.exe discovery during CMakeTestCCompiler (CMake invokes `vs_link_exe`
    # which finds ld.exe first and fails with "/nologo: No such file"). Perl is
    # not needed for the C++ build, so it is safe to remove here.
    $strawberryBins = @('C:\Strawberry\c\bin', 'C:\Strawberry\perl\site\bin', 'C:\Strawberry\perl\bin')
    $pathParts = $env:PATH -split ';' | Where-Object {
      $_ -and ($strawberryBins -notcontains $_) -and
      ($_ -notlike 'C:\Strawberry\c\bin*') -and ($_ -notlike 'C:\Strawberry\perl*')
    }
    $env:PATH = $pathParts -join ';'
    Write-Host ('[vcvars] Patched MSVC ' + $msvcVer + ' bin/include/lib into PATH/INCLUDE/LIB and dropped Strawberry Perl bins (vsdevcmd ext rollback dropped cl.exe; Strawberry ld.exe shadowed MSVC link.exe)') -ForegroundColor Yellow
  }
}

if (-not (Test-Path $buildDir)) {
  New-Item -ItemType Directory -Path $buildDir | Out-Null
}
Set-Location $buildDir

function Write-Stage([string]$Stage, [string]$Message) {
  Write-Host ("[" + $Stage + "] " + $Message) -ForegroundColor Cyan
}

function Fail-Stage([string]$Stage, [string]$Message, [int]$ExitCode = 1) {
  Write-Host ("[" + $Stage + "] " + $Message) -ForegroundColor Red
  exit $ExitCode
}

Stop-Process -Name 'OWzxSlicer' -Force -ErrorAction SilentlyContinue
Stop-Process -Name 'cmake', 'ninja', 'link' -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

$defaultUpstreamRoot = Join-Path $repoRoot 'third_party\OrcaSlicer'
$localUpstreamRoot = 'D:\work\OrcaSlicer'
$upstreamRoot = $defaultUpstreamRoot
if ((-not (Test-Path (Join-Path $upstreamRoot 'src\libslic3r\libslic3r_version.h.in'))) -and
    (Test-Path (Join-Path $localUpstreamRoot 'src\libslic3r\libslic3r_version.h.in'))) {
  $upstreamRoot = $localUpstreamRoot
}

$defaultDepsPrefix = 'E:\ai\3D-Printer\deps\build\OrcaSlicer_dep\usr\local'
$depsPrefix = $defaultDepsPrefix
$upstreamDepsPrefix = Join-Path $upstreamRoot 'deps\build\OrcaSlicer_dep\usr\local'
if ((-not (Test-Path $depsPrefix)) -and (Test-Path $upstreamDepsPrefix)) {
  $depsPrefix = $upstreamDepsPrefix
}

$upstreamRootCmake = $upstreamRoot -replace '\\', '/'
$depsPrefixCmake = $depsPrefix -replace '\\', '/'
Write-Host ("[Build] Upstream root: " + $upstreamRoot)
Write-Host ("[Build] Deps prefix: " + $depsPrefix)

$qtRootCandidates = New-Object System.Collections.Generic.List[string]
if ($env:OWZX_QT_ROOT) {
  [void]$qtRootCandidates.Add($env:OWZX_QT_ROOT)
}
[void]$qtRootCandidates.Add((Join-Path $repoRoot '.deps\Qt6.10\6.10.0\msvc2022_64'))
[void]$qtRootCandidates.Add((Join-Path $repoRoot '.deps\Qt6.10'))
[void]$qtRootCandidates.Add('E:\Qt6.10')
[void]$qtRootCandidates.Add('D:\Qt6.10')

$qtRoot = $null
foreach ($candidate in ($qtRootCandidates | Select-Object -Unique)) {
  if ($candidate -and (Test-Path (Join-Path $candidate 'lib\cmake\Qt6\Qt6Config.cmake'))) {
    $qtRoot = $candidate
    break
  }
}
if (-not $qtRoot) {
  $qtRoot = Join-Path $repoRoot '.deps\Qt6.10'
}
$qt6ConfigDir = Join-Path $qtRoot 'lib\cmake\Qt6'
$qt6ConfigDirCmake = $qt6ConfigDir -replace '\\', '/'
Write-Host ("[Build] Qt root: " + $qtRoot)

$env:CMAKE_PREFIX_PATH = "$qtRoot;$depsPrefix"
$env:Qt6_DIR = $qt6ConfigDir
$env:PATH = (Join-Path $qtRoot 'bin') + ";$env:PATH"
$clPath = (where.exe cl | Select-Object -First 1)
$ninjaPath = (where.exe ninja | Select-Object -First 1)
$clPathCmake = $clPath -replace '\\', '/'
$ninjaPathCmake = $ninjaPath -replace '\\', '/'
Write-Host ("[Build] Repo root: " + $repoRoot)
Write-Host ("[Build] Build dir: " + $buildDir)
Write-Host ("[Build] MSVC compiler: " + $clPath)
Write-Host ("[Build] Ninja: " + $ninjaPath)

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
  "-DDEPS_PREFIX=$depsPrefixCmake",
  "-DOWZX_UPSTREAM_ROOT=$upstreamRootCmake",
  "-DCMAKE_C_COMPILER=$clPathCmake",
  "-DCMAKE_CXX_COMPILER=$clPathCmake",
  "-DCMAKE_MAKE_PROGRAM=$ninjaPathCmake",
  "-DOWZX_RENDER_BENCH=$(if ($renderBenchEnabled) { 'ON' } else { 'OFF' })",
  '-DQT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT=3.21',
  "-DQt6_DIR=$qt6ConfigDirCmake"
)

$configureSucceeded = $false
for ($attempt = 1; $attempt -le 3; ++$attempt) {
  Write-Stage 'Configure' ("CMake configure attempt " + $attempt)
  $configureOutput = & cmake @cmakeArgs 2>&1
  $configureText = ($configureOutput | Out-String)
  $lastConfigureExitCode = $LASTEXITCODE
  $configureOutput | Write-Host

  if ($lastConfigureExitCode -eq 0) {
    $configureSucceeded = $true
    break
  }

  if ($configureText -notmatch 'failed recompaction: Permission denied' -or $attempt -eq 3) {
    Fail-Stage 'Configure' ("CMake configure failed with exit code " + $lastConfigureExitCode) $lastConfigureExitCode
  }

  Write-Stage 'Configure' ("Retrying after permission-denied configure failure on attempt " + $attempt)
  Stop-Process -Name 'OWzxSlicer' -Force -ErrorAction SilentlyContinue
  Stop-Process -Name 'cmake', 'ninja', 'link' -Force -ErrorAction SilentlyContinue
  Start-Sleep -Seconds 2
}

if (-not $configureSucceeded) {
  Fail-Stage 'Configure' ("CMake configure failed after retries with exit code " + $lastConfigureExitCode) $lastConfigureExitCode
}

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
      Fail-Stage 'Build' ("Required target not found: " + $Target)
    }
    Write-Stage 'Build' ("Optional target not found, skipping: " + $Target)
    return
  }

  Write-Stage 'Build' ("Building target " + $Target)
  ninja -j6 $Target
  if ($LASTEXITCODE -ne 0) {
    Fail-Stage 'Build' ("Target build failed: " + $Target + " (exit code " + $LASTEXITCODE + ")") $LASTEXITCODE
  }
}

Invoke-NinjaTarget 'OWzxSlicer.exe'

# Build test targets (if BUILD_TESTING is ON)
Invoke-NinjaTarget 'E2EWorkflowTests'
Invoke-NinjaTarget 'ViewModelSmokeTests'
Invoke-NinjaTarget 'PrepareSceneDataTests'
Invoke-NinjaTarget 'QmlUiAuditTests'
Invoke-NinjaTarget 'PartPlateTests'
# Phase 55-01: PreviewParserTests target (parser/role/mode coverage scaffold).
Invoke-NinjaTarget 'PreviewParserTests'
Invoke-NinjaTarget 'owzx-cli'
Invoke-NinjaTarget 'CliTests'
Invoke-NinjaTarget 'test-slice-direct' $false
if ($renderBenchEnabled) {
  Invoke-NinjaTarget 'owzx-render-bench'
}

# Deploy Qt runtime DLLs if not already present
if (-not (Test-Path './Qt6Core.dll')) {
  Write-Stage 'Deploy' ("Deploying Qt runtime DLLs from " + $qtRoot)
  $windeployqt = Join-Path $qtRoot 'bin\windeployqt.exe'
  if (-not (Test-Path $windeployqt)) {
    Fail-Stage 'Deploy' ("windeployqt.exe not found: " + $windeployqt)
  }
  & $windeployqt --release --qmldir '../src' --no-translations --no-system-d3d-compiler --no-opengl-sw './OWzxSlicer.exe' 2>$null
  if ($LASTEXITCODE -ne 0) {
    Fail-Stage 'Deploy' ("Qt runtime deployment failed with exit code " + $LASTEXITCODE) $LASTEXITCODE
  }
}

# Deploy MSVC runtime DLLs (vcruntime140, msvcp140, etc.) for standalone execution.
# windeployqt does NOT copy these — they are only available via vcvars PATH.
function Resolve-MsvcRedistDir {
  $candidates = New-Object System.Collections.Generic.List[string]
  if ($env:VCToolsRedistDir) {
    [void]$candidates.Add((Join-Path $env:VCToolsRedistDir 'x64\Microsoft.VC145.CRT'))
    [void]$candidates.Add((Join-Path $env:VCToolsRedistDir 'x64\Microsoft.VC143.CRT'))
  }

  $vcRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $vcvars))
  $redistRoot = Join-Path $vcRoot 'Redist\MSVC'
  if (Test-Path $redistRoot) {
    Get-ChildItem -Path $redistRoot -Directory -ErrorAction SilentlyContinue |
      Sort-Object Name -Descending |
      ForEach-Object {
        [void]$candidates.Add((Join-Path $_.FullName 'x64\Microsoft.VC145.CRT'))
        [void]$candidates.Add((Join-Path $_.FullName 'x64\Microsoft.VC143.CRT'))
      }
  }

  foreach ($candidate in ($candidates | Select-Object -Unique)) {
    if ($candidate -and (Test-Path $candidate)) {
      return $candidate
    }
  }

  return $null
}

$msvcRedist = Resolve-MsvcRedistDir
if ($msvcRedist) {
  Write-Stage 'Deploy' ("Copying MSVC runtime DLLs from " + $msvcRedist)
  Get-ChildItem -Path $msvcRedist -Filter '*.dll' | ForEach-Object {
    if (-not (Test-Path (Join-Path '.' $_.Name))) {
      Copy-Item $_.FullName -Destination '.' -Force
    }
  }
} else {
  Write-Host "[Deploy] MSVC redist directory not found; relying on PATH/runtime installation" -ForegroundColor Yellow
}

# Deploy OCCT (OpenCASCADE) DLLs – the exe imports TK*.dll at load time.
# Source: OrcaSlicer dependency bundle at DEPS_PREFIX/bin/occt/
$occtBinDir = Join-Path $depsPrefix 'bin\occt'
if (Test-Path $occtBinDir) {
  Write-Stage 'Deploy' ("Copying OCCT DLLs from " + $occtBinDir)
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

Write-Host "`n[PreviewParser] Running G-code parser/role/mode tests..." -ForegroundColor Cyan
$previewParserExe = './PreviewParserTests.exe'
if (Test-Path $previewParserExe) {
  & $previewParserExe 2>&1 | ForEach-Object { Write-Host "  $_" }
  if ($LASTEXITCODE -ne 0) {
    Write-Host "[PreviewParser] PreviewParser tests failed" -ForegroundColor Red
    exit $LASTEXITCODE
  }
  Write-Host "[PreviewParser] PreviewParser tests passed" -ForegroundColor Green
} else {
  Write-Host "[PreviewParser] PreviewParserTests.exe not found" -ForegroundColor Red
  exit 1
}

# Deploy vcpkg runtime DLLs for libs linked via vcpkg (windeployqt doesn't see these).
# noise.dll (libnoise) and draco.dll are imported by libslic3r_from_source.
$runtimeBinDirs = @(
  (Join-Path $depsPrefix 'bin'),
  'E:\vcpkg\installed\x64-windows\bin'
) | Select-Object -Unique
foreach ($runtimeBinDir in $runtimeBinDirs) {
  if (Test-Path $runtimeBinDir) {
    Write-Stage 'Deploy' ("Copying vcpkg runtime DLLs from " + $runtimeBinDir)
    foreach ($dll in @('noise.dll', 'draco.dll')) {
      $src = Join-Path $runtimeBinDir $dll
      if ((Test-Path $src) -and -not (Test-Path (Join-Path '.' $dll))) {
        Copy-Item $src -Destination '.' -Force
      }
    }
  }
}

Write-Stage 'Launch' "Starting OWzxSlicer.exe"
# v5.7 Phase 211: D3D11 is the safe default (D3D12 crashes at QQuickWindow
# swapchain init on AMD Radeon APU). OWZX_VERIFY_RHI_BACKEND lets a host force
# a specific backend for the launch liveness gate (e.g. d3d12 on a verified
# discrete-GPU host); unset uses the app's D3D11-first default.
if ($env:OWZX_VERIFY_RHI_BACKEND) {
  Write-Host ("[Launch] OWZX_VERIFY_RHI_BACKEND=" + $env:OWZX_VERIFY_RHI_BACKEND + " (forcing backend for this host)") -ForegroundColor Yellow
  $env:OWZX_RHI_RENDERER = $env:OWZX_VERIFY_RHI_BACKEND
}
$p = Start-Process -FilePath './OWzxSlicer.exe' -WorkingDirectory (Get-Location) -PassThru
Start-Sleep -Seconds 5
if ($p.HasExited) {
  Fail-Stage 'Launch' ("OWzxSlicer exited early with code " + $p.ExitCode)
}
Write-Host ("[Launch] APP_RUNNING_PID=" + $p.Id)
Stop-Process -Id $p.Id -Force

# ── Run E2E Pipeline Tests (non-blocking) ──
Write-Stage 'E2E' "Running pipeline tests (non-blocking)"
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
