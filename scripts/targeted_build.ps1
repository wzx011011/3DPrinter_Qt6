# targeted_build.ps1 - build specific ninja targets using the same vcvars
# environment import as auto_verify_with_vcvars.ps1 (direct .bat invocation
# from some shells drops INCLUDE via the polluted-PATH vcvars failure).
# Usage: powershell -ExecutionPolicy Bypass -File scripts/targeted_build.ps1 -Targets QmlUiAuditTests
param(
  [Parameter(Mandatory=$true)]
  [string[]]$Targets
)

$vcvarsCandidates = @(
  'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat',
  'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat'
)
$vcvars = $vcvarsCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $vcvars) { Write-Error "vcvars64.bat not found"; exit 1 }

$envDump = cmd /c ('"' + $vcvars + '" >nul & set')
$vcvarsPath = $null
foreach ($line in $envDump) {
  $idx = $line.IndexOf('=')
  if ($idx -gt 0) {
    $name = $line.Substring(0, $idx)
    $value = $line.Substring($idx + 1)
    if ($name -ieq 'PATH') {
      if (($name -ceq 'PATH') -or (-not $vcvarsPath)) { $vcvarsPath = $value }
      continue
    }
    Set-Item -Path ("env:" + $name) -Value $value
  }
}
if ($vcvarsPath) {
  Set-Item -Path 'env:PATH' -Value $vcvarsPath
  Set-Item -Path 'env:Path' -Value $vcvarsPath
}

$env:CL = "/Zm300 /bigobj $env:CL"
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot
Write-Host "[targeted] cwd: $(Get-Location)"
# Accept comma-separated targets (powershell.exe -File passes them as one arg).
if ($Targets.Count -eq 1 -and $Targets[0].Contains(',')) {
  $Targets = $Targets[0].Split(',') | ForEach-Object { $_.Trim() } | Where-Object { $_ }
}
foreach ($t in $Targets) {
  Write-Host "[targeted] building $t"
  ninja -C build -j6 $t
  if ($LASTEXITCODE -ne 0) { Write-Error "[targeted] target failed: $t"; exit $LASTEXITCODE }
}
Write-Host "[targeted] done"
exit 0
