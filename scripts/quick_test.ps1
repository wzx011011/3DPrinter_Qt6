Stop-Process -Name OWzxSlicer -Force -ErrorAction SilentlyContinue
Start-Sleep 1
Start-Process -FilePath 'E:\ai\3DPrinter_Qt6\build\OWzxSlicer.exe' -WorkingDirectory 'E:\ai\3DPrinter_Qt6\build'
Start-Sleep 3
$p = Get-Process -Name OWzxSlicer -ErrorAction SilentlyContinue
if ($p) {
    Write-Host "RUNNING PID=$($p.Id) WS=$([math]::Round($p.WorkingSet64/1MB))MB Threads=$($p.Threads.Count) MainWindow='$($p.MainWindowTitle)'"
} else {
    Write-Host "PROCESS NOT FOUND"
}
