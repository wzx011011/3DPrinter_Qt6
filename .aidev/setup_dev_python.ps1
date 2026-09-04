$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Set-Location $PSScriptRoot

if (-not (Test-Path 'py312\python.exe')) {
  Expand-Archive -Path 'py312.zip' -DestinationPath 'py312' -Force
  # Embeddable Python: enable import site + site-packages
  Set-Content -Path 'py312\python312._pth' -Value "python312.zip`n.`nLib\site-packages`nimport site"
}
if (-not (Test-Path 'get-pip.py')) {
  Invoke-WebRequest -Uri 'https://bootstrap.pypa.io/get-pip.py' -OutFile 'get-pip.py'
}
& "$PSScriptRoot\py312\python.exe" "$PSScriptRoot\get-pip.py" --quiet --no-warn-script-location
Write-Host "pip bootstrap done"
& "$PSScriptRoot\py312\python.exe" -m pip install --quiet --no-warn-script-location claude-agent-sdk
Write-Host "sdk install done"
& "$PSScriptRoot\py312\python.exe" -c "import claude_agent_sdk as s; print('SDK version:', getattr(s, '__version__', 'unknown'))"
