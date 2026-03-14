$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
$envDump = cmd /c ('"' + $vcvars + '" >nul & set')
foreach ($line in $envDump) {
  $idx = $line.IndexOf('=')
  if ($idx -gt 0) {
    $name = $line.Substring(0, $idx)
    $value = $line.Substring($idx + 1)
    Set-Item -Path ("env:" + $name) -Value $value
  }
}
Set-Location 'E:\ai\3DPrinter_Qt6\build'
$cmakeArgs = '-S', 'E:\ai\3DPrinter_Qt6', '-B', '.', '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', '-DBUILD_LIBSLIC3R=OFF', '-DCREALITY_QML_GUI=ON', '-DQT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT=3.21'
& cmake @cmakeArgs 2>&1 | Write-Host
& ninja -j1 2>&1 | Select-Object -Last 40
