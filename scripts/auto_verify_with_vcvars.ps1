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

# Reduce MSVC memory pressure in large TUs/autogen files
$env:CL = "/Zm300 /bigobj $env:CL"

ninja -j1 FramelessDialogDemo.exe ViewModelSmokeTests.exe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

ctest --output-on-failure -R ViewModelSmokeTests
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$p = Start-Process -FilePath './FramelessDialogDemo.exe' -WorkingDirectory (Get-Location) -PassThru
Start-Sleep -Seconds 5
if ($p.HasExited) {
  Write-Host ("APP_EXIT_CODE=" + $p.ExitCode)
  exit 1
}
Write-Host ("APP_RUNNING_PID=" + $p.Id)
Stop-Process -Id $p.Id -Force
exit 0
