$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
$dlls = @(
    'E:\ai\3DPrinter_Qt6\build\libgmp-10.dll',
    'E:\ai\3DPrinter_Qt6\build\libmpfr-4.dll',
    'E:\ai\3DPrinter_Qt6\build\Qt6Core.dll',
    'E:\ai\3DPrinter_Qt6\build\Qt6Quick.dll'
)
foreach ($dll in $dlls) {
    if (Test-Path $dll) {
        $name = [System.IO.Path]::GetFileName($dll)
        Write-Host "=== $name ==="
        $out = cmd /c ("`"" + $vcvars + "`" >nul & dumpbin /DEPENDENTS `"" + $dll + "`"") 2>&1 | Out-String
        $out -split "`r?`n" | Where-Object { $_ -match 'DLL Name' } | ForEach-Object { Write-Host ("  " + $_.Trim()) }
        $out -split "`r?`n" | Where-Object { $_ -match 'MSVCR|msvcp|msvcrt|LIBCMT' } | ForEach-Object { Write-Host ("  CRT: " + $_.Trim()) }
    }
}
