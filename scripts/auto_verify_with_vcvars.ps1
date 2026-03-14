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
Stop-Process -Name 'cmake', 'ninja', 'link' -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

$cmakeArgs = @(
  '-S', '..',
  '-B', '.',
  '-G', 'Ninja',
  '-DCMAKE_BUILD_TYPE=Release',
  '-DBUILD_LIBSLIC3R=OFF',
  '-DCREALITY_QML_GUI=ON',
  '-DQT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT=3.21'
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
  Stop-Process -Name 'FramelessDialogDemo' -Force -ErrorAction SilentlyContinue
  Stop-Process -Name 'cmake', 'ninja', 'link' -Force -ErrorAction SilentlyContinue
  Start-Sleep -Seconds 2
}

if (-not $configureSucceeded) { exit $lastConfigureExitCode }

# Reduce MSVC memory pressure in large TUs/autogen files
$env:CL = "/Zm300 /bigobj $env:CL"

ninja -j1 FramelessDialogDemo.exe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Deploy Qt runtime DLLs if not already present
if (-not (Test-Path './Qt6Core.dll')) {
  & 'E:/Qt6.10/bin/windeployqt.exe' --release --qmldir '../src' --no-translations --no-system-d3d-compiler --no-opengl-sw './FramelessDialogDemo.exe' 2>$null
}

# Remove OCCT DLLs — they cause 0xC0000005 at startup due to /MT vs /MD
# CRT mismatch between pre-built libslic3r and OCCT DLLs.
# OCCT is not used by the QML GUI (no STEP/OBJ import yet).
Remove-Item 'TK*.dll' -ErrorAction SilentlyContinue
Remove-Item 'cr_tpms_library.dll' -ErrorAction SilentlyContinue

$p = Start-Process -FilePath './FramelessDialogDemo.exe' -WorkingDirectory (Get-Location) -PassThru
Start-Sleep -Seconds 5
if ($p.HasExited) {
  Write-Host ("APP_EXIT_CODE=" + $p.ExitCode)
  exit 1
}
Write-Host ("APP_RUNNING_PID=" + $p.Id)
Stop-Process -Id $p.Id -Force
exit 0
