$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
$libs = @(
    'E:\ai\3D-Printer\deps\build\OrcaSlicer_dep\usr\local\lib\tbb12.lib',
    'E:\ai\3D-Printer\deps\build\OrcaSlicer_dep\usr\local\lib\tbbmalloc.lib',
    'E:\ai\3D-Printer\deps\build\OrcaSlicer_dep\usr\local\lib\qhullstatic_r.lib',
    'E:\ai\3D-Printer\deps\build\OrcaSlicer_dep\usr\local\lib\qhullcpp.lib',
    'E:\ai\3D-Printer\deps\build\OrcaSlicer_dep\usr\local\lib\assimp-vc142-mt.lib',
    'E:\ai\3D-Printer\deps\build\OrcaSlicer_dep\usr\local\lib\cr_tpms_library.lib',
    'E:\ai\3D-Printer\out\vs2026-x64-release\build\src\libslic3r\Release\libslic3r.lib'
)
foreach ($lib in $libs) {
    if (Test-Path $lib) {
        $name = [System.IO.Path]::GetFileName($lib)
        Write-Host "=== $name ==="
        $out = cmd /c ("`"" + $vcvars + "`" >nul & dumpbin /DIRECTIVES `"" + $lib + "`"") 2>&1 | Out-String
        $defaultLibs = ($out -split "`r?`n" | Where-Object { $_ -match 'DEFAULTLIB' }) | ForEach-Object { ($_.Trim() -replace '^\s+', '') } | Sort-Object -Unique
        foreach ($dl in $defaultLibs) { Write-Host "  $dl" }
    }
}
