$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location $repoRoot

& powershell.exe -ExecutionPolicy Bypass -File scripts\auto_verify_with_vcvars.ps1
$exitCode = $LASTEXITCODE

Write-Output "CANONICAL_EXIT_CODE=$exitCode"
Write-Output "CANONICAL_EXIT_TIME=$(Get-Date -Format o)"
exit $exitCode
