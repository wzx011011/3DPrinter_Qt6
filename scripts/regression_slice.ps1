# 切片自回归测试 — 验证切片链路不崩溃
# 退出码: 0=成功, 1=崩溃, 2=未完成, 3=前置缺失
$ErrorActionPreference = 'Continue'
$buildDir = 'E:\ai\3DPrinter_Qt6\build'
$cliExe = "$buildDir\owzx-cli.exe"
$model = 'E:\ai\3DPrinter_Qt6\third_party\OrcaSlicer\tests\data\test_3mf\Prusa.stl'
$env:Path = 'E:\Qt6.10\bin;' + $env:Path

Write-Host '=== Slice Regression Test ==='

if (-not (Test-Path $cliExe)) { Write-Host "FAIL: CLI not found"; exit 3 }
if (-not (Test-Path $model))  { Write-Host "FAIL: Model not found"; exit 3 }

Set-Location $buildDir
$out = & $cliExe '--load' $model '--slice' '--output-dir' '.' 2>&1 | Out-String
$exitCode = $LASTEXITCODE

if ($exitCode -ne 0) {
    Write-Host "FAIL: Slice crashed (exit=$exitCode)"
    ($out -split "`n") | Select-Object -Last 10 | ForEach-Object { Write-Host "  $_" }
    exit 1
}

if (($out -notmatch 'Slice complete') -or ($out -notmatch 'G-code')) {
    Write-Host 'FAIL: Exit 0 but incomplete'
    exit 2
}

Write-Host 'PASS: Slice completed'
($out -split "`n") | Where-Object { $_ -match 'Slice complete|Filament|Layers|G-code written' } | ForEach-Object { Write-Host "  $_" }
exit 0
