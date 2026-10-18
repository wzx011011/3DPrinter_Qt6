# qml_hot_reload.ps1 — Qt 6.12 QML hot reload launcher for OWzx (dev only).
#
# Launches OWzxSlicer.exe under qmlpreview with a full qrc->source mapping so
# trivial QML edits (colors / texts / bindings / literals — including module
# pages under `import OWzx`) hot-patch the RUNNING app in place: no restart,
# C++ singletons and the RHI viewport stay alive. Structural changes (add /
# remove objects or properties) are NOT applied live — restart for those.
#
# Requires a build with OWZX_QML_HOT_RELOAD=ON (the dev default; the option
# compiles the QQmlDebuggingEnabler into OWzxSlicer and qmlpreview attaches
# through it. CI release builds pass =OFF — the debug hook must not ship).
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File scripts/qml_hot_reload.ps1
#   powershell -ExecutionPolicy Bypass -File scripts/qml_hot_reload.ps1 -AppArgs "--skip-first-run","--open-page","preview"
#
# Then just edit any .qml under src/qml_gui/ and save. Ctrl+C here stops the app.
param(
  [string[]]$AppArgs = @("--skip-first-run"),
  [switch]$VerbosePreview
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir '..')
$buildDir = Join-Path $repoRoot 'build'

$exe = Join-Path $buildDir 'OWzxSlicer.exe'
if (-not (Test-Path $exe)) {
  Write-Error "OWzxSlicer.exe not found at: $exe. Run scripts/auto_verify_with_vcvars.ps1 first."
  exit 1
}

# Resolve qmlpreview.exe from the Qt the build actually used (CMakeCache
# Qt6_DIR), falling back to the pinned .deps tree.
$qmlpreview = $null
$cacheFile = Join-Path $buildDir 'CMakeCache.txt'
if (Test-Path $cacheFile) {
  $qt6Dir = (Select-String -Path $cacheFile -Pattern '^Qt6_DIR:INTERNAL=(.*)$').Matches |
    ForEach-Object { $_.Groups[1].Value } | Select-Object -First 1
  if (-not $qt6Dir) {
    $qt6Dir = (Select-String -Path $cacheFile -Pattern '^Qt6_DIR:.*=(.*)$').Matches |
      ForEach-Object { $_.Groups[1].Value } | Select-Object -First 1
  }
  if ($qt6Dir) { $qmlpreview = Join-Path (Split-Path (Split-Path (Split-Path $qt6Dir))) 'bin\qmlpreview.exe' }
}
if (-not $qmlpreview -or -not (Test-Path $qmlpreview)) {
  $qmlpreview = Join-Path $repoRoot '.deps\Qt6.12\6.12.0\msvc2022_64\bin\qmlpreview.exe'
}
if (-not (Test-Path $qmlpreview)) {
  Write-Error "qmlpreview.exe not found (looked at: $qmlpreview)"
  exit 1
}

# Build the merged --resource map. Three ingredients, all required:
#   1. owzx_qml_raw_qml_0.qrc        — every .qml qrc alias -> src/qml_gui source
#   2. extra_qmldirs.qrc             — subdirectory qmldirs -> build/OWzx/<dir>
#   3. root module qmldir mapping    — WITHOUT it the module resolves purely
#                                      from qrc and module pages never patch
#                                      (verified 2026-10-18: only main.qml did).
$rawQrcPath = Join-Path $buildDir 'src\qml_gui\.qt\rcc\owzx_qml_raw_qml_0.qrc'
$extraQrcPath = Join-Path $buildDir 'src\qml_gui\.qt\rcc\owzx_qml_raw_qml_0_extra_qmldirs.qrc'
$moduleQmldir = Join-Path $buildDir 'OWzx\qmldir'
foreach ($p in @($rawQrcPath, $extraQrcPath, $moduleQmldir)) {
  if (-not (Test-Path $p)) {
    Write-Error "Mapping input missing: $p (build with auto_verify_with_vcvars.ps1 first)."
    exit 1
  }
}
function Get-QrcBody([string]$path) {
  return [regex]::Match((Get-Content $path -Raw), '<qresource.*?>(.*?)</qresource>', 'Singleline').Groups[1].Value
}
$mapFile = Join-Path $buildDir 'hot_reload_map.qrc'
$rootMap = '    <file alias="qmldir">' + ($moduleQmldir -replace '\\', '/') + '</file>' + "`n"
$merged = "<RCC>`n  <qresource prefix=`"/qml/OWzx/`">`n" + $rootMap +
  (Get-QrcBody $rawQrcPath) + (Get-QrcBody $extraQrcPath) + "  </qresource>`n</RCC>`n"
Set-Content -Path $mapFile -Value $merged -Encoding UTF8
Write-Host "[hot-reload] resource map written: $mapFile"

# qmlpreview owns the console; app args pass through after the exe path.
$previewArgs = @('--resource', $mapFile)
if ($VerbosePreview) { $previewArgs += '--verbose' }
Write-Host "[hot-reload] launching: $qmlpreview $($previewArgs -join ' ') $exe $($AppArgs -join ' ')"
& $qmlpreview @previewArgs $exe @AppArgs
exit $LASTEXITCODE
