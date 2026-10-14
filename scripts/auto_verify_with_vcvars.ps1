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

# Stale processes from an interrupted verify keep their exe files locked
# (LNK1104 "cannot open file"), so kill them before linking — but scoped to
# THIS repo (Q4). The old bare-name kill of 'cmake'/'ninja'/'link' was
# machine-wide and took down other sessions'/projects' builds.
function Stop-RepoProcesses {
  # (1) Anything whose executable lives under build\ — the app, test/bench/
  #     cli exes all output there, so this covers every current and future
  #     test target without a name list.
  $buildPrefix = $buildDir.TrimEnd('\') + '\'
  Get-Process -ErrorAction SilentlyContinue | Where-Object {
    try { $_.Path -and $_.Path.StartsWith($buildPrefix, [System.StringComparison]::OrdinalIgnoreCase) } catch { $false }
  } | Stop-Process -Force -ErrorAction SilentlyContinue

  # (2) Tool processes (cmake/ninja/link) only when their command line
  #     references this repo's root — our configure passes absolute repo
  #     paths (-DOWZX_UPSTREAM_ROOT/-DQt6_DIR/...). Those tools' executables
  #     are never installed under build\, so exe-path scoping cannot reach
  #     them; a repo path in the command line is the honest ownership signal.
  #     (A stale `ninja -j6 <target>` spawned from build\ uses relative
  #     arguments and stays unidentifiable after the fact — accepted: the
  #     LNK1104 lock holders are the exes under (1), and the configure retry
  #     covers the "failed recompaction" race.)
  Get-CimInstance Win32_Process -Filter "Name='cmake.exe' OR Name='ninja.exe' OR Name='link.exe'" -ErrorAction SilentlyContinue |
    Where-Object { $_.CommandLine -and ($_.CommandLine.IndexOf($repoRoot, [System.StringComparison]::OrdinalIgnoreCase) -ge 0) } |
    ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
}
Stop-Process -Name 'OWzxSlicer' -Force -ErrorAction SilentlyContinue
Stop-RepoProcesses
# Belt and braces: this repo's test exe names are unique machine-wide, so the
# name list is zero-collateral and also catches exes launched from copies.
Stop-Process -Name 'E2EWorkflowTests', 'ViewModelSmokeTests', 'PrepareSceneDataTests',
    'ObjectPickingTests', 'NavigatorCubeTests', 'CameraParityTests',
    'ViewportContextMenuTests', 'QmlUiAuditTests', 'PartPlateTests', 'AppToolTests',
    'PreviewParserTests', 'GizmoGeometryTests', 'CliTests', 'owzx-cli',
    'owzx-render-bench', 'PerfBench', 'QmlSmokeTests' -Force -ErrorAction SilentlyContinue
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
# Qt 6.12 LTS anchor (M2.5 G1, 2026-10-13; 6.10 candidates retired in G5,
# 2026-10-14). Single-version discipline: only 6.12 candidates here — if the
# 6.12 root is missing the build must fail loudly instead of silently
# downgrading (mixed-version hazard). Never re-add fallback candidates from a
# retired base; rollback history lives in docs/qt612-upgrade-notes.md.
[void]$qtRootCandidates.Add((Join-Path $repoRoot '.deps\Qt6.12\6.12.0\msvc2022_64'))
[void]$qtRootCandidates.Add((Join-Path $repoRoot '.deps\Qt6.12'))

$qtRoot = $null
foreach ($candidate in ($qtRootCandidates | Select-Object -Unique)) {
  if ($candidate -and (Test-Path (Join-Path $candidate 'lib\cmake\Qt6\Qt6Config.cmake'))) {
    $qtRoot = $candidate
    break
  }
}
if (-not $qtRoot) {
  $qtRoot = Join-Path $repoRoot '.deps\Qt6.12'
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

# Build-speed fast path (2026-09-05): CMake configure costs minutes on this
# project (FindBoost, Qt shader/tooling discovery). Skip it entirely when the
# existing build.ninja is NEWER than every CMake input that could change it
# (root/cmake CMakeLists + the script's own -D switches are fixed values).
# Q4 (2026-10-11): patches/orcaslicer/*.patch are applied onto the OrcaSlicer
# submodule working tree AT CONFIGURE TIME (ApplyOrcaSlicerPatches.cmake) and
# the submodule gitlink decides which upstream tree gets built. Neither was
# in this input set, so a patch-only or pin-only commit skipped configure and
# ran the suites on stale, unpatched binaries (false green). The patches join
# the mtime inputs; the pin has no file mtime at all (the gitlink moves
# without touching any tracked file's mtime), so it is recorded after each
# successful configure into build\ and compared by CONTENT on the fast path.
# Q4a hardening (2026-10-11, round 2): the mtime listing only sees files that
# EXIST. A deleted or renamed patch simply vanishes from the input list while
# every remaining input stays older than build.ninja — configure would be
# skipped on a tree still carrying the deleted patch's edits. The whole patch
# series is therefore ALSO fingerprinted by content (name + SHA256 per patch,
# sorted) and compared exactly like the pin, so add/modify/delete/rename of
# any patch all force a real configure.
$patchSeriesFiles = @(
  Get-ChildItem '..\patches\orcaslicer' -Filter '*.patch' -File -ErrorAction SilentlyContinue | Sort-Object Name
)
$cmakeInputs = @(
  '..\CMakeLists.txt'
) + (
  (Get-ChildItem '..\cmake' -Filter '*.cmake' -File -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName }) +
  @($patchSeriesFiles | ForEach-Object { $_.FullName })
)
$patchSeriesFingerprint = ($patchSeriesFiles | ForEach-Object {
  "{0}:{1}" -f $_.Name, (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
}) -join "`n"
$patchSeriesFile = Join-Path (Get-Location) 'owzx_patch_series.txt'
# Normalize CRLF->LF on read: Set-Content writes platform EOLs, the in-memory
# fingerprint joins with LF only.
$patchSeriesMatches = $false
if (Test-Path $patchSeriesFile) {
  $recordedSeries = [string](Get-Content $patchSeriesFile -Raw)
  if (($recordedSeries -replace "`r`n", "`n").Trim() -eq $patchSeriesFingerprint) { $patchSeriesMatches = $true }
}
$submodulePinFile = Join-Path (Get-Location) 'owzx_orcaslicer_pin.txt'
$submoduleHead = $null
try {
  $submoduleHead = (& git -C (Join-Path $repoRoot 'third_party\OrcaSlicer') rev-parse HEAD 2>$null | Select-Object -First 1)
  if (-not $submoduleHead) { $submoduleHead = $null }
} catch {
  $submoduleHead = $null
}
# An unreadable pin (git missing, submodule not initialized) must NOT allow
# the fast path — force a real configure so the failure surfaces there.
$submodulePinMatches = $false
if ($null -ne $submoduleHead -and (Test-Path $submodulePinFile)) {
  $pinnedHead = (Get-Content $submodulePinFile -Raw).Trim()
  if ($pinnedHead -eq $submoduleHead.Trim()) { $submodulePinMatches = $true }
}
$buildNinja = Join-Path (Get-Location) 'build.ninja'
$skipConfigure = $false
if ((Test-Path $buildNinja) -and ($cmakeInputs.Count -gt 0) -and $submodulePinMatches -and $patchSeriesMatches) {
  $ninjaTime = (Get-Item $buildNinja).LastWriteTimeUtc
  $newestInput = ($cmakeInputs | Get-Item | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1)
  if ($null -ne $newestInput -and $newestInput.LastWriteTimeUtc -le $ninjaTime) {
    $skipConfigure = $true
    $configureSucceeded = $true
    Write-Stage 'Configure' ("Skipped (build.ninja up to date vs " + (Split-Path $newestInput.FullName -Leaf) + "; OrcaSlicer pin " + $submoduleHead + " + patch series unchanged)")
  }
}

if (-not $skipConfigure) {
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
  Stop-RepoProcesses
  Start-Sleep -Seconds 2
}

if (-not $configureSucceeded) {
  Fail-Stage 'Configure' ("CMake configure failed after retries with exit code " + $lastConfigureExitCode) $lastConfigureExitCode
}

# Record the submodule pin and patch-series fingerprint this build.ninja was
# configured against (Q4): the fast path above compares them by content so a
# pin-only or patch-only change (add, modify, delete, rename) cannot skip
# configure on the next run. Only written after a successful configure.
if ($null -ne $submoduleHead) {
  Set-Content -Path $submodulePinFile -Value $submoduleHead
}
Set-Content -Path $patchSeriesFile -Value $patchSeriesFingerprint
}  # end of skip-configure fast path

# Q13 (2026-10-12): kill the stale-moc/mixed-object failure class at its
# mechanism. AUTOMOC's per-build moc regeneration is mtime-tracked per target
# ("weak moc tracking", documented on the single-file QtTest targets in
# CMakeLists.txt), and since the 2026-09-05 configure fast path a
# source-only change no longer forces a configure-time automoc rescan. A
# binary can then mix objects compiled against different Q_OBJECT layouts
# (stale mocs_compilation.obj vs fresh consumers) -- the deterministic
# import-time heap-corruption family that made AppToolTests /
# ViewportContextMenuTests flake-crash between build layouts (Q13 audit:
# 0xC0000005/0xC0000409 during the first initTestCase slot). Deleting the
# autogen timestamp probes forces CMake's RERUN_AUTOGEN step on the next
# ninja invocation of THIS run: moc outputs are regenerated and any
# mismatching object rebuilds, so a canonical run can no longer emit a
# mixed-layout binary. Cost: an automoc rescan (seconds) per verify run.
Get-ChildItem -Path . -Filter '*_autogen' -Directory -ErrorAction SilentlyContinue | ForEach-Object {
  $ts = Join-Path $_.FullName 'timestamp'
  if (Test-Path $ts) { Remove-Item $ts -Force }
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

# G-12 followup (2026-09-05): redirect test temp files to an ISOLATED E:
# directory. QTemporaryDir (Qt tests, E2E fixtures) resolves from TMP/TEMP;
# the shared C: Temp is contended by parallel automation sessions whose
# multi-GB writes made QSaveFile targets transiently unwritable
# (store_bbs_3mf dirWritable=0 flake). E: has its own headroom.
$testTemp = Join-Path $BuildDir 'test_temp'
New-Item -ItemType Directory -Force -Path $testTemp | Out-Null
$env:TMP = $testTemp
$env:TEMP = $testTemp
Write-Host "[TestEnv] TMP/TEMP -> $testTemp"

# Build test targets (if BUILD_TESTING is ON)
Invoke-NinjaTarget 'E2EWorkflowTests'
Invoke-NinjaTarget 'ViewModelSmokeTests'
Invoke-NinjaTarget 'PrepareSceneDataTests'
Invoke-NinjaTarget 'ObjectPickingTests'
Invoke-NinjaTarget 'NavigatorCubeTests'
Invoke-NinjaTarget 'CameraParityTests'
Invoke-NinjaTarget 'ViewportContextMenuTests'
Invoke-NinjaTarget 'QmlUiAuditTests'
# Q14: QML behavior smoke suite (engine-instantiated main.qml + 10 pages;
# suite count 12 -> 13 in the canonical gate).
Invoke-NinjaTarget 'QmlSmokeTests'
# Phase 66 GGEO: gizmo geometry snapshot tests + the P15.11 sidebar-hint
# builders (buildSidebarHintVertices axis/regression coverage).
Invoke-NinjaTarget 'GizmoGeometryTests'
Invoke-NinjaTarget 'PartPlateTests'
Invoke-NinjaTarget 'AppToolTests'
# Phase 55-01: PreviewParserTests target (parser/role/mode coverage scaffold).
Invoke-NinjaTarget 'PreviewParserTests'
# PERF-STAGE-BENCH: efficiency self-test exe (not part of the pass/fail
# suites; run via scripts/perf/run_stage_bench.ps1 after the gate).
Invoke-NinjaTarget 'PerfBench'
Invoke-NinjaTarget 'owzx-cli'
Invoke-NinjaTarget 'CliTests'
Invoke-NinjaTarget 'test-slice-direct' $false
if ($renderBenchEnabled) {
  Invoke-NinjaTarget 'owzx-render-bench'
}

# S8 (2026-10-14): QML lint gate — qmllint over the OWzx qt_add_qml_module
# module tree, 0-error 口径. Levels are pinned in src/qml_gui/.qmllint.ini
# (all reviewed categories stay at warning; only error-level findings —
# today the QML compile/syntax-failure class that historically zeroed engine
# root objects — fail the build). Type-resolution breakage that qmllint keeps
# at warning is caught by the QmlSmokeTests component-creation asserts.
Write-Host "`n[QmlLint] Running qmllint over the OWzx QML module (0-error gate)..." -ForegroundColor Cyan
& cmake --build . --target all_qmllint 2>&1 | ForEach-Object { Write-Host "  $_" }
if ($LASTEXITCODE -ne 0) {
  Fail-Stage 'QmlLint' ("qmllint gate failed with exit code " + $LASTEXITCODE + " -- see src/qml_gui/.qmllint.ini for the level contract") $LASTEXITCODE
}
Write-Host "[QmlLint] qmllint gate passed" -ForegroundColor Green

# Deploy Qt runtime DLLs if not already present
if (-not (Test-Path './Qt6Core.dll')) {
  Write-Stage 'Deploy' ("Deploying Qt runtime DLLs from " + $qtRoot)
  $windeployqt = Join-Path $qtRoot 'bin\windeployqt.exe'
  if (-not (Test-Path $windeployqt)) {
    Fail-Stage 'Deploy' ("windeployqt.exe not found: " + $windeployqt)
  }
  # S8: the QML module tree lives at build/OWzx (generated qmldir + copied
  # QML sources) — windeployqt scans it to map imports to plugins; the OWzx
  # module itself is embedded in the exe (NO_PLUGIN) and needs no deployment.
  & $windeployqt --release --qmldir '../build/OWzx' --no-translations --no-system-d3d-compiler --no-opengl-sw './OWzxSlicer.exe' 2>$null
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

function Invoke-QtTestReport([string]$Exe, [string]$ReportName) {
  $reportPath = Join-Path (Get-Location) $ReportName
  $nativeLogPath = $reportPath + '.native.log'
  Remove-Item $reportPath, $nativeLogPath -Force -ErrorAction SilentlyContinue
  & $Exe -o ($reportPath + ',txt') *> $nativeLogPath
  $exitCode = $LASTEXITCODE
  if (Test-Path $nativeLogPath) {
    Get-Content $nativeLogPath | ForEach-Object { Write-Host "  $_" }
  }
  if (Test-Path $reportPath) {
    Get-Content $reportPath | ForEach-Object { Write-Host "  $_" }
  }
  return $exitCode
}

# Run static UI audit tests before launching the app.
Write-Host "`n[PrepareScene] Running Prepare scene data tests..." -ForegroundColor Cyan
$prepareSceneDataExe = './PrepareSceneDataTests.exe'
if (Test-Path $prepareSceneDataExe) {
  $prepareSceneExitCode = Invoke-QtTestReport $prepareSceneDataExe 'PrepareSceneDataTests.verify.txt'
  if ($prepareSceneExitCode -ne 0) {
    Write-Host "[PrepareScene] Prepare scene data tests failed" -ForegroundColor Red
    exit $prepareSceneExitCode
  }
  Write-Host "[PrepareScene] Prepare scene data tests passed" -ForegroundColor Green
} else {
  Write-Host "[PrepareScene] PrepareSceneDataTests.exe not found" -ForegroundColor Red
  exit 1
}

Write-Host "`n[PartPlate] Running PartPlate geometry + arrangement tests..." -ForegroundColor Cyan
$partPlateExe = './PartPlateTests.exe'
if (Test-Path $partPlateExe) {
  $partPlateExitCode = Invoke-QtTestReport $partPlateExe 'PartPlateTests.verify.txt'
  if ($partPlateExitCode -ne 0) {
    Write-Host "[PartPlate] PartPlate tests failed" -ForegroundColor Red
    exit $partPlateExitCode
  }
  Write-Host "[PartPlate] PartPlate tests passed" -ForegroundColor Green
} else {
  Write-Host "[PartPlate] PartPlateTests.exe not found" -ForegroundColor Red
  exit 1
}

# AI control surface tests (OWzx-only, docs/ai-control.md). BLOCKING again as
# of 2026-10-12 (Q13): the 2026-08-31 import-period heap-corruption was
# attributed to the stale-moc/mixed-object build class (see the autogen
# timestamp cleanup above) -- a consistent-epoch rebuild passes 3/3 and the
# canonical run can no longer emit a mixed-layout binary. The suite also
# installs CrashHandlerWin in initTestCase, so any future crash lands in
# build/crash_dumps/crash_stack.log + a minidump instead of exiting silently.
Write-Host "`n[AppTool] Running AI control surface tests..." -ForegroundColor Cyan
$appToolExe = './AppToolTests.exe'
if (Test-Path $appToolExe) {
  $appToolExitCode = Invoke-QtTestReport $appToolExe 'AppToolTests.verify.txt'
  if ($appToolExitCode -ne 0) {
    Write-Host "[AppTool] AI control surface tests failed" -ForegroundColor Red
    exit $appToolExitCode
  }
  Write-Host "[AppTool] AI control surface tests passed" -ForegroundColor Green
} else {
  Write-Host "[AppTool] AppToolTests.exe not found" -ForegroundColor Red
  exit 1
}

Write-Host "`n[ObjectPicking] Running precise object picking tests..." -ForegroundColor Cyan
$objectPickingExe = './ObjectPickingTests.exe'
if (Test-Path $objectPickingExe) {
  $objectPickingExitCode = Invoke-QtTestReport $objectPickingExe 'ObjectPickingTests.verify.txt'
  if ($objectPickingExitCode -ne 0) {
    Write-Host "[ObjectPicking] Object picking tests failed" -ForegroundColor Red
    exit $objectPickingExitCode
  }
  Write-Host "[ObjectPicking] Object picking tests passed" -ForegroundColor Green
} else {
  Write-Host "[ObjectPicking] ObjectPickingTests.exe not found" -ForegroundColor Red
  exit 1
}

Write-Host "`n[ViewModel] Running viewmodel smoke tests..." -ForegroundColor Cyan
$viewModelSmokeExe = './ViewModelSmokeTests.exe'
if (Test-Path $viewModelSmokeExe) {
  $viewModelExitCode = Invoke-QtTestReport $viewModelSmokeExe 'ViewModelSmokeTests.verify.txt'
  if ($viewModelExitCode -ne 0) {
    Write-Host "[ViewModel] ViewModel smoke tests failed" -ForegroundColor Red
    exit $viewModelExitCode
  }
  Write-Host "[ViewModel] ViewModel smoke tests passed" -ForegroundColor Green
} else {
  Write-Host "[ViewModel] ViewModelSmokeTests.exe not found" -ForegroundColor Red
  exit 1
}

Write-Host "`n[UI] Running QML UI audit tests..." -ForegroundColor Cyan
$uiAuditExe = './QmlUiAuditTests.exe'
if (Test-Path $uiAuditExe) {
  $uiAuditExitCode = Invoke-QtTestReport $uiAuditExe 'QmlUiAuditTests.verify.txt'
  if ($uiAuditExitCode -ne 0) {
    Write-Host "[UI] QML UI audit tests failed" -ForegroundColor Red
    exit $uiAuditExitCode
  }
  Write-Host "[UI] QML UI audit tests passed" -ForegroundColor Green
} else {
  Write-Host "[UI] QmlUiAuditTests.exe not found" -ForegroundColor Red
  exit 1
}

# Q14 (2026-10-12): QML behavior smoke suite, blocking from day one. First
# engine-instantiated QML coverage (QmlUiAuditTests is source-string locks):
# loads the shipped qrc tree (main.qml + 10 pages) under the minimal QPA
# platform, asserting zero QML errors, readable context properties and
# per-page instantiation. Behavioral safety net for S1 (NOTIFY re-pointing)
# and X2 (mass QML rewrite). Runs headless — no GPU, no window exposure.
Write-Host "`n[QmlSmoke] Running QML behavior smoke tests..." -ForegroundColor Cyan
$qmlSmokeExe = './QmlSmokeTests.exe'
if (Test-Path $qmlSmokeExe) {
  $qmlSmokeExitCode = Invoke-QtTestReport $qmlSmokeExe 'QmlSmokeTests.verify.txt'
  if ($qmlSmokeExitCode -ne 0) {
    Write-Host "[QmlSmoke] QML behavior smoke tests failed" -ForegroundColor Red
    exit $qmlSmokeExitCode
  }
  Write-Host "[QmlSmoke] QML behavior smoke tests passed" -ForegroundColor Green
} else {
  Write-Host "[QmlSmoke] QmlSmokeTests.exe not found" -ForegroundColor Red
  exit 1
}

Write-Host "`n[GizmoGeometry] Running gizmo geometry + sidebar hint tests..." -ForegroundColor Cyan
$gizmoGeometryExe = './GizmoGeometryTests.exe'
if (Test-Path $gizmoGeometryExe) {
  $gizmoGeometryExitCode = Invoke-QtTestReport $gizmoGeometryExe 'GizmoGeometryTests.verify.txt'
  if ($gizmoGeometryExitCode -ne 0) {
    Write-Host "[GizmoGeometry] Gizmo geometry tests failed" -ForegroundColor Red
    exit $gizmoGeometryExitCode
  }
  Write-Host "[GizmoGeometry] Gizmo geometry tests passed" -ForegroundColor Green
} else {
  Write-Host "[GizmoGeometry] GizmoGeometryTests.exe not found" -ForegroundColor Red
  exit 1
}

Write-Host "`n[NavigatorCube] Running 3D navigator cube math tests..." -ForegroundColor Cyan
$navigatorCubeExe = './NavigatorCubeTests.exe'
if (Test-Path $navigatorCubeExe) {
  $navigatorCubeExitCode = Invoke-QtTestReport $navigatorCubeExe 'NavigatorCubeTests.verify.txt'
  if ($navigatorCubeExitCode -ne 0) {
    Write-Host "[NavigatorCube] Navigator cube tests failed" -ForegroundColor Red
    exit $navigatorCubeExitCode
  }
  Write-Host "[NavigatorCube] Navigator cube tests passed" -ForegroundColor Green
} else {
  Write-Host "[NavigatorCube] NavigatorCubeTests.exe not found" -ForegroundColor Red
  exit 1
}

Write-Host "`n[CameraParity] Running upstream camera parity tests..." -ForegroundColor Cyan
$cameraParityExe = './CameraParityTests.exe'
if (Test-Path $cameraParityExe) {
  $cameraParityExitCode = Invoke-QtTestReport $cameraParityExe 'CameraParityTests.verify.txt'
  if ($cameraParityExitCode -ne 0) {
    Write-Host "[CameraParity] Camera parity tests failed" -ForegroundColor Red
    exit $cameraParityExitCode
  }
  Write-Host "[CameraParity] Camera parity tests passed" -ForegroundColor Green
} else {
  Write-Host "[CameraParity] CameraParityTests.exe not found" -ForegroundColor Red
  exit 1
}

# Viewport context menu tests. BLOCKING again as of 2026-10-12 (Q13): the
# "import-path heap-corruption family" that silenced this suite since
# 2026-08-31 (silent 0xC0000409 fast-fail during the first initTestCase slot,
# floating between build layouts) was the same stale-moc/mixed-object class as
# AppToolTests -- see the autogen timestamp cleanup above. Consistent-epoch
# rebuilds pass 3/3 and the canonical run can no longer emit a mixed-layout
# binary; initTestCase installs CrashHandlerWin so a future crash leaves a
# stack + minidump in build/crash_dumps/.
Write-Host "`n[ViewportContext] Running viewport context menu tests..." -ForegroundColor Cyan
$viewportContextMenuExe = './ViewportContextMenuTests.exe'
if (Test-Path $viewportContextMenuExe) {
  $viewportContextExitCode = Invoke-QtTestReport $viewportContextMenuExe 'ViewportContextMenuTests.verify.txt'
  if ($viewportContextExitCode -ne 0) {
    Write-Host "[ViewportContext] Viewport context menu tests failed" -ForegroundColor Red
    exit $viewportContextExitCode
  }
  Write-Host "[ViewportContext] Viewport context menu tests passed" -ForegroundColor Green
} else {
  Write-Host "[ViewportContext] ViewportContextMenuTests.exe not found" -ForegroundColor Red
  exit 1
}

Write-Host "`n[PreviewParser] Running G-code parser/role/mode tests..." -ForegroundColor Cyan
$previewParserExe = './PreviewParserTests.exe'
if (Test-Path $previewParserExe) {
  $previewParserExitCode = Invoke-QtTestReport $previewParserExe 'PreviewParserTests.verify.txt'
  if ($previewParserExitCode -ne 0) {
    Write-Host "[PreviewParser] PreviewParser tests failed" -ForegroundColor Red
    exit $previewParserExitCode
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

# ── Run E2E Pipeline Tests ──
# BLOCKING again as of 2026-10-12 (Q13 + Q12): the Q12 environment probe
# (E2EWorkflowTests.cpp initTestCase) skips — counted and reported — only when
# a static slicing prerequisite is missing; with prerequisites present a slice
# failure FAILs. Measured on this canonical host: 32 passed / 0 failed /
# 0 skipped, so a red run here is a real pipeline signal, not a missing-env
# false red.
Write-Stage 'E2E' "Running pipeline tests"
$e2eExe = './E2EWorkflowTests.exe'
if (Test-Path $e2eExe) {
  $e2eExitCode = Invoke-QtTestReport $e2eExe 'E2EWorkflowTests.verify.txt'
  if ($e2eExitCode -ne 0) {
    Write-Host "[E2E] Pipeline tests failed" -ForegroundColor Red
    exit $e2eExitCode
  }
  Write-Host "[E2E] All pipeline tests passed" -ForegroundColor Green
} else {
  Write-Host "[E2E] E2EWorkflowTests.exe not found" -ForegroundColor Red
  exit 1
}

# ── Slice regression gate (conditional: fires only when slicing code changed) ──
# Runs scripts/perf/run_slice_gate.ps1 when uncommitted changes or the commits
# since the last green gate run touch the slicing surface (list below).
# Correctness = normalized golden-gcode compare (hard fail, exit 1); efficiency
# = PerfBench slice min-of-N vs baseline thresholds (hard fail, exit 2).
# OWZX_SLICE_GATE=0 skips; OWZX_SLICE_GATE=force runs regardless of the diff.
$sliceGateSurface = @(
  'src/core/services/SliceService.*',
  'src/core/services/ProjectService.*',
  'src/core/services/PresetService.*',
  'src/core/viewmodels/PreviewViewModel.*',
  'src/core/viewmodels/EditorViewModel.cpp',
  'src/cli/*',
  'third_party/OrcaSlicer',
  'tests/PerfBench.cpp',
  'tests/CliTests.cpp',
  'tools/gcode_compare.py',
  'scripts/perf/*',
  'scripts/auto_verify_with_vcvars.ps1',
  'tests/data/slice_gate_baseline.json'
)
$sliceGateStatePath = Join-Path $buildDir 'perf_out\slice_gate_state.json'
$sliceGateScript = Join-Path $repoRoot 'scripts\perf\run_slice_gate.ps1'

if ($env:OWZX_SLICE_GATE -eq '0') {
  Write-Stage 'SliceGate' 'OWZX_SLICE_GATE=0 -- slice regression gate skipped by explicit override'
} elseif (-not (Test-Path $sliceGateScript)) {
  Write-Host '[SliceGate] run_slice_gate.ps1 not found, skipping' -ForegroundColor DarkGray
} else {
  $sliceGateReason = $null
  if ($env:OWZX_SLICE_GATE -eq 'force') {
    $sliceGateReason = 'OWZX_SLICE_GATE=force'
  } else {
    $dirtySliceFiles = @(git -C $repoRoot status --porcelain -- $sliceGateSurface 2>$null)
    if ($dirtySliceFiles.Count -gt 0) {
      $sliceGateReason = ('uncommitted slice-surface change: ' + ($dirtySliceFiles[0] -replace '^\S+\s+', ''))
    } else {
      $sliceStateCommit = $null
      if (Test-Path $sliceGateStatePath) {
        try { $sliceStateCommit = (Get-Content $sliceGateStatePath -Raw | ConvertFrom-Json).commit } catch { $sliceStateCommit = $null }
      }
      $sliceStateValid = $false
      if ($sliceStateCommit) {
        git -C $repoRoot cat-file -e "$sliceStateCommit^{commit}" 2>$null
        if ($LASTEXITCODE -eq 0) { $sliceStateValid = $true }
      }
      if (-not $sliceStateValid) {
        $sliceGateReason = 'first run (no valid gate state recorded yet)'
      } else {
        $committedSliceFiles = @(git -C $repoRoot diff --name-only "$sliceStateCommit..HEAD" -- $sliceGateSurface 2>$null)
        if ($committedSliceFiles.Count -gt 0) {
          $sliceGateReason = ('committed slice-surface change since ' + $sliceStateCommit + ': ' + $committedSliceFiles[0])
        }
      }
    }
  }

  if ($sliceGateReason) {
    Write-Stage 'SliceGate' ("triggered -- " + $sliceGateReason)
    & powershell -NoProfile -ExecutionPolicy Bypass -File $sliceGateScript
    $sliceGateExit = $LASTEXITCODE
    if ($sliceGateExit -ne 0) {
      Fail-Stage 'SliceGate' ("slice regression gate failed (exit " + $sliceGateExit + ": 1=correctness, 2=efficiency, 3=infra) -- see build\perf_out\slice_gate_report.json and tests\data\slice_gate_baseline.json")
    }
    $headCommitNow = (git -C $repoRoot rev-parse --short HEAD 2>$null)
    if ($headCommitNow) {
      $headCommitNow = $headCommitNow.Trim()
      $gateStateDir = Split-Path -Parent $sliceGateStatePath
      if (-not (Test-Path $gateStateDir)) { New-Item -ItemType Directory -Force -Path $gateStateDir | Out-Null }
      @{ commit = $headCommitNow; passed_at_utc = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ') } |
        ConvertTo-Json | Set-Content -Path $sliceGateStatePath
      Write-Host ("[SliceGate] green at commit " + $headCommitNow) -ForegroundColor Green
    }
  } else {
    Write-Host '[SliceGate] no slice-surface changes since last green run -- skipping slice regression gate' -ForegroundColor DarkGray
  }
}

# ── Reclaim disk: PDBs are regenerable build artifacts ──
# Each full-PDB target is ~1 GB (main exe via /DEBUG:FULL; tests used to emit
# PDBs via a propagated /DEBUG, now removed). E: historically dropped below
# ~5 GB free mid-run, killing ninja. Deleting PDBs after a successful verify
# keeps the tree at a stable size; the next build regenerates them.
$pdbs = Get-ChildItem -Path '.' -Filter '*.pdb' -File -ErrorAction SilentlyContinue
if ($pdbs) {
  $freedBytes = ($pdbs | Measure-Object -Property Length -Sum).Sum
  $pdbs | Remove-Item -Force -ErrorAction SilentlyContinue
  Write-Stage 'Cleanup' ("Removed " + $pdbs.Count + " regenerable PDB files (freed " + [math]::Round($freedBytes / 1GB, 2) + " GB)")
}

exit 0
