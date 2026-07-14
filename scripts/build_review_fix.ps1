$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
$rawPath = $env:PATH
$kept = New-Object System.Collections.Generic.List[string]
foreach ($entry in ($rawPath -split ';')) {
  if ($entry -and ($entry.Contains(' ') -and $entry.Contains('('))) { } else { [void]$kept.Add($entry) }
}
$env:PATH = ($kept -join ';')
$envDump = cmd /c ('"' + $vcvars + '" >nul & set')
foreach ($line in $envDump) { $idx = $line.IndexOf('='); if ($idx -gt 0) { Set-Item -Path ("env:" + $line.Substring(0, $idx)) -Value $line.Substring($idx + 1) } }
$winKitsRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'
if (-not (Test-Path $winKitsRoot)) { $winKitsRoot = 'C:\Program Files (x86)\Windows Kits\10' }
if ((-not $env:INCLUDE) -or (-not ($env:INCLUDE -match 'ucrt')) -and (Test-Path $winKitsRoot)) {
  $wkVer = (Get-ChildItem -Path (Join-Path $winKitsRoot 'Include') -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1).Name
  if ($wkVer) {
    $wkIncBase = Join-Path $winKitsRoot "Include\$wkVer"
    $wkLibBase = Join-Path $winKitsRoot "Lib\$wkVer"
    $missingInc = @('shared', 'ucrt', 'um') | Where-Object { $p = Join-Path $wkIncBase $_; (Test-Path $p) -and ($env:INCLUDE -notlike "*$p*") }
    $missingLib = @('ucrt', 'um') | Where-Object { $p = Join-Path $wkLibBase "$_\x64"; (Test-Path $p) -and ($env:LIB -notlike "*$p*") }
    if ($missingInc) { $env:INCLUDE = (($missingInc | ForEach-Object { Join-Path $wkIncBase $_ }) -join ';') + ';' + $env:INCLUDE }
    if ($missingLib) { $env:LIB = (($missingLib | ForEach-Object { Join-Path $wkLibBase "$_\x64" }) -join ';') + ';' + $env:LIB }
    $wkBin = Join-Path $winKitsRoot "Bin\$wkVer\x64"
    if ((Test-Path $wkBin) -and ($env:PATH -notlike "*$wkBin*")) { $env:PATH = "$wkBin;$env:PATH" }
  }
}
Set-Location 'e:/ai/3DPrinter_Qt6/build'
$env:CMAKE_PREFIX_PATH = "E:\Qt6.10"
$env:Qt6_DIR = "E:\Qt6.10"
$env:PATH = "E:\Qt6.10\bin;$env:PATH"
$env:CL = "/Zm300 /bigobj $env:CL"
Stop-Process -Name 'OWzxSlicer' -Force -ErrorAction SilentlyContinue
Write-Host "[REVIEW] Building owzx_app_core + test targets..." -ForegroundColor Cyan
& ninja -j16 owzx_app_core PartPlateTests QmlUiAuditTests ViewModelSmokeTests PrepareSceneDataTests 2>&1 | ForEach-Object { Write-Host $_ }
if ($LASTEXITCODE -ne 0) { Write-Host "[REVIEW] BUILD FAILED (exit $LASTEXITCODE)" -ForegroundColor Red; exit $LASTEXITCODE }
Write-Host "[REVIEW] Build OK" -ForegroundColor Green
