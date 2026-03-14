Write-Host "Testing BUILD_LIBSLIC3R=ON app stability..."

$env:PATH = "E:\ai\3D-Printer\deps\build\OrcaSlicer_dep\usr\local\bin\occt;" + $env:PATH

$passCount = 0
$failCount = 0

for ($i = 1; $i -le 5; $i++) {
    Stop-Process -Name "FramelessDialogDemo" -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 500

    $proc = Start-Process -FilePath "build\FramelessDialogDemo.exe" -WorkingDirectory "build" -PassThru -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 5

    $alive = Get-Process -Id $proc.Id -ErrorAction SilentlyContinue
    if ($alive) {
        Stop-Process -Id $proc.Id -Force
        $passCount++
        Write-Host "Run $i : PASS"
    } else {
        $failCount++
        Write-Host "Run $i : FAIL (exit $($proc.ExitCode))"
    }
    Start-Sleep -Milliseconds 500
}

Write-Host ""
Write-Host "Results: $passCount pass, $failCount fail"
