Set-Location 'E:\ai\3DPrinter_Qt6\build'

# RDP does not support OpenGL hardware acceleration
$sessionName = [System.Environment]::GetEnvironmentVariable('SESSIONNAME')
if ($sessionName -match 'RDP') {
    Write-Host "[RDP detected] Using software rendering"
    $env:QT_QUICK_BACKEND = 'software'
} else {
    Write-Host "[Local session] Using hardware rendering"
}

& ".\OWzxSlicer.exe"
