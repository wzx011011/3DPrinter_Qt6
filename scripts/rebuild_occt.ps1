# rebuild_occt.ps1 — Rebuild OCCT 7.6.0 from source with current MSVC toolchain
# Fixes ABI incompatibility between pre-built OCCT DLLs (MSVC 14.30) and our toolchain (MSVC 14.50)

$ErrorActionPreference = "Stop"

$OCCT_SOURCE = "E:/ai/3D-Printer/deps/build/dep_OCCT-prefix/src/dep_OCCT"
$OCCT_BUILD  = "E:/ai/3D-Printer/deps/build/dep_OCCT-rebuild"
$INSTALL_PREFIX = "E:/ai/3D-Printer/deps/build/OrcaSlicer_dep/usr/local"
$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'

# Verify source exists
if (-not (Test-Path "$OCCT_SOURCE/CMakeLists.txt")) {
    Write-Host "[ERROR] OCCT source not found at $OCCT_SOURCE" -ForegroundColor Red
    exit 1
}

# Create clean build directory
if (Test-Path $OCCT_BUILD) { Remove-Item -Recurse -Force $OCCT_BUILD }
New-Item -ItemType Directory -Path $OCCT_BUILD | Out-Null

# Find FreeType
$FREETYPE_INCLUDE = "$INSTALL_PREFIX/include/freetype2"
$FREETYPE_LIB = Get-ChildItem "$INSTALL_PREFIX/lib/*freetype*" -ErrorAction SilentlyContinue | Select-Object -First 1

$ft_args = ""
if ($FREETYPE_LIB) {
    $ft_args = "-DUSE_FREETYPE=ON -DFREETYPE_INCLUDE_DIR_freetype2=$FREETYPE_INCLUDE -DFREETYPE_LIBRARY=$($FREETYPE_LIB.FullName)"
    Write-Host "  FreeType found: $($FREETYPE_LIB.FullName)" -ForegroundColor Green
} else {
    $ft_args = "-DUSE_FREETYPE=OFF"
    Write-Host "  FreeType not found, building without font support" -ForegroundColor Yellow
}

# Write batch script — all build steps in single cmd session with vcvars
$batchFile = "$OCCT_BUILD\_build.bat"
@"
@echo off
call "$vcvars" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] vcvars64.bat failed
    exit /b 1
)

echo [1/4] Configuring OCCT 7.6.0...
cmake -S "$OCCT_SOURCE" -B "$OCCT_BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX -DBUILD_LIBRARY_TYPE=Shared -DUSE_TK=OFF -DUSE_TBB=OFF -DUSE_FFMPEG=OFF -DUSE_VTK=OFF -DBUILD_DOC_Overview=OFF -DBUILD_MODULE_ApplicationFramework=OFF -DBUILD_MODULE_Draw=OFF -DBUILD_MODULE_FoundationClasses=OFF -DBUILD_MODULE_ModelingAlgorithms=OFF -DBUILD_MODULE_ModelingData=OFF -DBUILD_MODULE_Visualization=OFF -DINSTALL_DIR_BIN=bin/occt -DINSTALL_DIR_LIB=lib/occt -DINSTALL_DIR_INCLUDE=include/occt -DINSTALL_DIR_CMAKE=lib/cmake/occt -DCMAKE_CXX_STANDARD=17 $ft_args
if errorlevel 1 (
    echo [ERROR] CMake configure failed
    exit /b 1
)

echo [2/4] Building OCCT...
cmake --build "$OCCT_BUILD" --config Release -j 16
if errorlevel 1 (
    echo [ERROR] Build failed
    exit /b 1
)

echo [3/4] Installing OCCT...
cmake --install "$OCCT_BUILD" --config Release
if errorlevel 1 (
    echo [ERROR] Install failed
    exit /b 1
)

echo [4/4] Done.
"@ | Set-Content -Path $batchFile -Encoding ASCII

Write-Host "Running OCCT rebuild..." -ForegroundColor Cyan
& cmd /c $batchFile 2>&1 | Tee-Object -Variable buildOutput

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] OCCT rebuild failed" -ForegroundColor Red
    exit $LASTEXITCODE
}

# Copy DLLs to project build dir
Write-Host "`n[Post-build] Copying DLLs to project..." -ForegroundColor Cyan
$project_build = "E:/ai/3DPrinter_Qt6/build"
$new_dlls = Get-ChildItem "$INSTALL_PREFIX/bin/occt/TK*.dll" -ErrorAction SilentlyContinue
$new_libs = Get-ChildItem "$INSTALL_PREFIX/lib/occt/TK*.lib" -ErrorAction SilentlyContinue
Write-Host "  Installed $($new_dlls.Count) DLLs, $($new_libs.Count) libs" -ForegroundColor Green

foreach ($dll in $new_dlls) {
    Copy-Item $dll.FullName "$project_build/" -Force
}
Write-Host "  Copied DLLs to $project_build" -ForegroundColor Green

Write-Host "`nOCCT rebuild complete!" -ForegroundColor Green
Write-Host "Now rebuild the project: powershell -File scripts/auto_verify_with_vcvars.ps1" -ForegroundColor Yellow
