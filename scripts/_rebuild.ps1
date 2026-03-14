Set-Location 'E:\ai\3DPrinter_Qt6\build'

$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) { Write-Host "ERROR: vcvars64.bat not found"; exit 1 }

$envDump = cmd /c ('"' + $vcvars + '" >nul & set')
foreach ($line in $envDump) {
  $idx = $line.IndexOf('=')
  if ($idx -gt 0) {
    $name = $line.Substring(0, $idx)
    $value = $line.Substring($idx + 1)
    Set-Item -Path ("env:" + $name) -Value $value
  }
}

Write-Host "VS CC=$env:CC CXX=$env:CXX"

$cmakeArgs = @(
  '-S', 'E:\ai\3DPrinter_Qt6',
  '-B', '.',
  '-G', 'Ninja',
  '-DCMAKE_BUILD_TYPE=Release',
  '-DBUILD_LIBSLIC3R=OFF',
  '-DCREALITY_QML_GUI=ON',
  '-DQT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT=3.21'
)

& cmake @cmakeArgs 2>&1 | Write-Host
if ($LASTEXITCODE -ne 0) { Write-Host "cmake FAILED exit=$LASTEXITCODE"; exit 1 }

$env:CL = "/Zm300 /bigobj $env:CL"
ninja -j1 FramelessDialogDemo.exe
if ($LASTEXITCODE -ne 0) { Write-Host "ninja FAILED exit=$LASTEXITCODE"; exit 1 }

# Remove OCCT DLLs
Remove-Item 'TK*.dll' -ErrorAction SilentlyContinue
Remove-Item 'cr_tpms_library.dll' -ErrorAction SilentlyContinue

$p = Start-Process -FilePath './FramelessDialogDemo.exe' -WorkingDirectory (Get-Location) -PassThru
Start-Sleep -Seconds 5
if ($p.HasExited) {
  Write-Host "APP_EXIT_CODE=$($p.ExitCode)"
  exit 1
}
Write-Host "APP_RUNNING_PID=$($p.Id)"
Stop-Process -Id $p.Id -Force
Write-Host "ALL OK"
exit 0
