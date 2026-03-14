Set-Location 'E:\ai\3DPrinter_Qt6\build'
Remove-Item 'startup_diagnostics.log' -ErrorAction SilentlyContinue
$env:QT_LOGGING_RULES = '*.debug=true;qt.qml*=true'
$env:QML_DEBUG_LOG = '1'
$proc = Start-Process -FilePath '.\FramelessDialogDemo.exe' -PassThru
Start-Sleep -Seconds 5
if ($proc.HasExited) {
  Write-Host "APP_EXIT_CODE=$($proc.ExitCode)"
  exit 1
} else {
  Write-Host "APP_RUNNING_PID=$($proc.Id)"
  Stop-Process -Id $proc.Id -Force
  exit 0
}
