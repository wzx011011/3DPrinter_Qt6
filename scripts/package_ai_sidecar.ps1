# package_ai_sidecar.ps1 — assemble the OPTIONAL AI sidecar component
# (OWzx-only decision, docs/ai-control.md) into build/ai_sidecar/.
#
# Layout produced:
#   build/ai_sidecar/agent.py                  harness entry (also in repo: tools/ai_sidecar/agent.py)
#   build/ai_sidecar/python/python.exe         embedded CPython 3.12
#   build/ai_sidecar/python/Lib/site-packages/ claude-agent-sdk (+ bundled claude.exe, ~230 MB)
#
# Requires network on first run (python.org + PyPI); caches the embeddable zip
# under build/ai_sidecar_cache/ so subsequent runs are offline.
param(
    [string]$PythonVersion = '3.12.10',
    [string]$BuildDir = ''
)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $BuildDir) { $BuildDir = Join-Path $repoRoot 'build' }
$cacheDir = Join-Path $BuildDir 'ai_sidecar_cache'
$outDir = Join-Path $BuildDir 'ai_sidecar'
$pyDir = Join-Path $outDir 'python'

New-Item -ItemType Directory -Force -Path $cacheDir | Out-Null

# 1. Fetch (cached) embedded CPython
$zip = Join-Path $cacheDir "python-$PythonVersion-embed-amd64.zip"
if (-not (Test-Path $zip)) {
    Write-Host "[ai-sidecar] downloading python $PythonVersion embeddable..."
    Invoke-WebRequest -Uri "https://www.python.org/ftp/python/$PythonVersion/python-$PythonVersion-embed-amd64.zip" -OutFile $zip
}

# 2. Fresh extract + pip bootstrap
if (Test-Path $pyDir) { Remove-Item -Recurse -Force $pyDir }
Expand-Archive -Path $zip -DestinationPath $pyDir -Force
# Enable site-packages + `import site` in the embeddable ._pth
$pth = Join-Path $pyDir "python$($PythonVersion.Split('.')[0..1] -join '')._pth"
if (-not (Test-Path $pth)) { $pth = Get-ChildItem $pyDir -Filter '*._pth' | Select-Object -First 1 -ExpandProperty FullName }
Set-Content -Path $pth -Value "python$($PythonVersion.Split('.')[0..1] -join '').zip`n.`nLib\site-packages`nimport site"

$getPip = Join-Path $cacheDir 'get-pip.py'
if (-not (Test-Path $getPip)) {
    Invoke-WebRequest -Uri 'https://bootstrap.pypa.io/get-pip.py' -OutFile $getPip
}
$pythonExe = Join-Path $pyDir 'python.exe'
& $pythonExe $getPip --quiet --no-warn-script-location
if ($LASTEXITCODE -ne 0) { throw 'pip bootstrap failed' }

# 3. Install pinned SDK
& $pythonExe -m pip install --quiet --no-warn-script-location -r (Join-Path $repoRoot 'tools\ai_sidecar\requirements.txt')
if ($LASTEXITCODE -ne 0) { throw 'pip install failed' }

# 4. Copy the harness entry + selftest
Copy-Item -Force (Join-Path $repoRoot 'tools\ai_sidecar\agent.py') $outDir

& $pythonExe (Join-Path $outDir 'agent.py') --selftest
if ($LASTEXITCODE -ne 0) { throw 'sidecar selftest failed' }

$sizeMb = [math]::Round(((Get-ChildItem $outDir -Recurse -File | Measure-Object -Sum Length).Sum / 1MB), 0)
Write-Host "[ai-sidecar] packaged at $outDir ($sizeMb MB). Ship as an OPTIONAL component."
