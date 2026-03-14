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

Stop-Process -Name 'FramelessDialogDemo' -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

# Build without extra flags (QML debug log uses env var now)
$env:CL = "/Zm300 /bigobj $env:CL"
& cmake -S .. -B . -G Ninja "-DCMAKE_BUILD_TYPE=Release" "-DBUILD_LIBSLIC3R=OFF" "-DCREALITY_QML_GUI=ON" "-DQT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT=3.21" 2>&1 | Write-Host
if ($LASTEXITCODE -ne 0) {
  Write-Host "CMAKE CONFIGURE FAILED"
  exit 1
}

ninja -j1 FramelessDialogDemo.exe 2>&1
if ($LASTEXITCODE -ne 0) {
  Write-Host "BUILD FAILED"
  exit 1
}
Write-Host "BUILD OK"

# Deploy Qt DLLs if needed
if (-not (Test-Path './Qt6Core.dll')) {
  & 'E:/Qt6.10/bin/windeployqt.exe' --release --qmldir '../src' --no-translations --no-system-d3d-compiler --no-opengl-sw './FramelessDialogDemo.exe' 2>$null
}

# Remove OCCT DLLs
Remove-Item 'TK*.dll' -ErrorAction SilentlyContinue
Remove-Item 'cr_tpms_library.dll' -ErrorAction SilentlyContinue

Remove-Item './qml_debug.log' -ErrorAction SilentlyContinue

# Enable QML debug logging via environment variable
$env:QML_DEBUG_LOG = "1"

# Start app
$p = Start-Process -FilePath '.\FramelessDialogDemo.exe' -WorkingDirectory (Get-Location) -PassThru

# Wait for app to run
for ($i = 1; $i -le 10; $i++) {
  Start-Sleep -Seconds 1
  $proc = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
  if (-not $proc) {
    Write-Host "Process exited after ${i}s"
    break
  }
  $ws = [math]::Round($proc.WorkingSet64 / 1MB)
  Write-Host "After ${i}s: WS=${ws}MB Threads=$($proc.Threads.Count)"
}

Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

if (Test-Path './qml_debug.log') {
  $lines = Get-Content './qml_debug.log'
  Write-Host "=== QML Debug Log ($($lines.Count) lines) ==="
  Get-Content './qml_debug.log'
  Write-Host "=== End Log ==="
} else {
  Write-Host "No qml_debug.log created"
}
