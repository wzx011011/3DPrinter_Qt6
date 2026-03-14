$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
$outFile = 'E:\ai\3DPrinter_Qt6\build\imports_dump.txt'

$envDump = cmd /c ("`"" + $vcvars + "`" >nul & dumpbin /IMPORTS E:\ai\3DPrinter_Qt6\build\FramelessDialogDemo.exe")
$envDump | Out-File -FilePath $outFile -Encoding utf8

Write-Host "=== Imported DLL Names ==="
Get-Content $outFile | Where-Object { $_ -match 'DLL Name' } | ForEach-Object { $_.Trim() }

Write-Host ""
Write-Host "=== All sections ==="
Get-Content $outFile | Where-Object { $_ -match '^\s+(DLL Name|Section|Summary)' } | ForEach-Object { $_.Trim() }
