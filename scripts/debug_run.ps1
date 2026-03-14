Set-Location 'E:\ai\3DPrinter_Qt6\build'
$env:QT_LOGGING_RULES = 'qt.qml*=true'
$env:QML_IMPORT_TRACE = '1'
$env:QT_DEBUG_PLUGINS = '1'
$proc = Start-Process -FilePath '.\FramelessDialogDemo.exe' -PassThru -NoNewWindow -RedirectStandardError 'stderr.txt' -RedirectStandardOutput 'stdout.txt'
Start-Sleep -Seconds 5
if ($proc.HasExited) {
  Write-Host "App exited with code: $($proc.ExitCode)"
}
$proc | Stop-Process -Force -ErrorAction SilentlyContinue
Write-Host "=== stdout (last 50) ==="
if (Test-Path 'stdout.txt') { Get-Content 'stdout.txt' -Tail 50 }
Write-Host "=== stderr (last 50) ==="
if (Test-Path 'stderr.txt') { Get-Content 'stderr.txt' -Tail 50 }
