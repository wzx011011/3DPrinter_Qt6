$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildScript = Join-Path $repoRoot "scripts\build_with_vcvars.ps1"

if (-not (Test-Path $buildScript)) {
  Write-Error "Build script not found: $buildScript"
  exit 1
}

Set-Location $repoRoot
& $buildScript
exit $LASTEXITCODE
