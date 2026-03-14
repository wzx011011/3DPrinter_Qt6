$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) {
  Write-Error "vcvars64.bat not found: $vcvars"
  exit 1
}

$envDump = cmd /c ('"' + $vcvars + '" >nul 2>&1 & set')
foreach ($line in $envDump) {
  $idx = $line.IndexOf('=')
  if ($idx -gt 0) {
    $name = $line.Substring(0, $idx)
    $value = $line.Substring($idx + 1)
    Set-Item -Path ("env:" + $name) -Value $value
  }
}

$buildDir = 'E:/ai/3DPrinter_Qt6/build_src'
if (Test-Path $buildDir) {
  Remove-Item -Recurse -Force "$buildDir/*"
}

$cmakeArgs = @(
  '-S', 'E:/ai/3DPrinter_Qt6',
  '-B', $buildDir,
  '-G', 'Ninja',
  '-DCMAKE_BUILD_TYPE=Release',
  '-DBUILD_LIBSLIC3R=ON',
  '-DLIBSLIC3R_FROM_SOURCE=ON',
  '-DCREALITY_QML_GUI=ON',
  '-DQt6_DIR=E:/Qt6.10/lib/cmake/Qt6',
  '-DQT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT=3.21'
)

Write-Host "=== Running CMake configure (libslic3r from source) ==="
& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) {
  Write-Host "CMAKE_CONFIGURE_FAILED exit=$LASTEXITCODE"
  exit $LASTEXITCODE
}

Write-Host "=== CMake configure succeeded ==="
Write-Host "=== Running ninja build ==="

$env:CL = "/Zm300 /bigobj $env:CL"
& ninja -C $buildDir -j1 FramelessDialogDemo.exe
if ($LASTEXITCODE -ne 0) {
  Write-Host "BUILD_FAILED exit=$LASTEXITCODE"
  exit $LASTEXITCODE
}

Write-Host "=== Build succeeded ==="
