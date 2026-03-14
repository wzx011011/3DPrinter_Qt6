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

# Build WITHOUT libslic3r to verify QML GUI (CRT isolation is a separate task)
& cmake -S .. -B . -G Ninja "-DCMAKE_BUILD_TYPE=Release" "-DBUILD_LIBSLIC3R=OFF" "-DCREALITY_QML_GUI=ON" "-DQT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT=3.21" 2>&1
if ($LASTEXITCODE -ne 0) {
  Write-Host "CMAKE CONFIGURE FAILED"
  exit 1
}

$env:CL = "/Zm300 /bigobj $env:CL"
ninja -j1 FramelessDialogDemo.exe
if ($LASTEXITCODE -ne 0) {
  Write-Host "BUILD FAILED"
  exit 1
}
Write-Host "BUILD OK"

# Deploy Qt DLLs
if (-not (Test-Path './Qt6Core.dll')) {
  & 'E:/Qt6.10/bin/windeployqt.exe' --release --qmldir '../src' --no-translations --no-system-d3d-compiler --no-opengl-sw './FramelessDialogDemo.exe' 2>$null
}

# Keep OCCT stub DLLs — they are needed if any libslic3r global constructor
# calls an OCCT function (delay-load will load the stub at that point).
# Remove only .exp/.lib artifacts from stub builds.
Remove-Item 'TK*.exp' -ErrorAction SilentlyContinue
Remove-Item 'TK*.lib' -ErrorAction SilentlyContinue

# Check what DLLs the exe imports now
Write-Host "=== Dependencies ==="
cmd /c ('"' + $vcvars + '" >nul & dumpbin /dependents .\FramelessDialogDemo.exe 2>&1') | Where-Object { $_ -match '\.dll' -or $_ -match 'Image has' }

# Run the app
$p = Start-Process -FilePath '.\FramelessDialogDemo.exe' -WorkingDirectory (Get-Location) -PassThru
for ($i = 1; $i -le 10; $i++) {
  Start-Sleep -Seconds 1
  $proc = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
  if (-not $proc) {
    Write-Host "After ${i}s: EXIT (code=$($p.ExitCode))"
    break
  }
  $ws = [math]::Round($proc.WorkingSet64 / 1MB)
  Write-Host "After ${i}s: WS=${ws}MB Threads=$($proc.Threads.Count) MainWindow='$($proc.MainWindowTitle)'"
  if ($proc.MainWindowTitle -ne '') {
    Write-Host "WINDOW FOUND!"
    break
  }
}

# Check if startup_diagnostics.log was created
if (Test-Path 'startup_diagnostics.log') {
  Write-Host ""
  Write-Host "=== startup_diagnostics.log ==="
  Get-Content 'startup_diagnostics.log'
}

Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
