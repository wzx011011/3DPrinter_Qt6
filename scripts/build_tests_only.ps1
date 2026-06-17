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

Set-Location 'E:/ai/3DPrinter_Qt6/build'
$env:CMAKE_PREFIX_PATH = "E:\Qt6.10"
$env:Qt6_DIR = "E:\Qt6.10"
$env:CL = "/Zm300 /bigobj $env:CL"

ninja -j16 E2EPipelineTests.exe
exit $LASTEXITCODE
