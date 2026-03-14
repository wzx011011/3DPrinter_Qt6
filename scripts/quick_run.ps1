Set-Location 'E:\ai\3DPrinter_Qt6\build'
$proc = Start-Process -FilePath '.\FramelessDialogDemo.exe' -PassThru -NoNewWindow -RedirectStandardError 'stderr.txt' -RedirectStandardOutput 'stdout.txt'
Start-Sleep -Seconds 5
$proc | Stop-Process -Force -ErrorAction SilentlyContinue
Write-Host "=== stdout ==="
Get-Content 'stdout.txt' -Tail 30
Write-Host "=== stderr ==="
Get-Content 'stderr.txt' -Tail 30
