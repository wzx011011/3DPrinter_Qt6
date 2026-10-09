# ui_diff_loop.ps1
# R-loop tool (docs/ui-reference/restoration-map.md §5): launch OWzxSlicer at
# the upstream-truth client size (1366x721), capture the window, and pixel-diff
# it against docs/visual-compare/upstream_<page>.png.
#
# Dev fast loop: set -DevQmlDir to point at src/qml_gui so the app loads QML
# from disk via the OWZX_DEV_QML_DIR interceptor -- QML edits then need only an
# app restart, not the rcc+link rebuild cycle (QML_DISABLE_DISK_CACHE=1 is set
# automatically for the child process).
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File scripts/ui_diff_loop.ps1 -Page prepare
#   powershell -ExecutionPolicy Bypass -File scripts/ui_diff_loop.ps1 -Page preview -KeepOpen
# Requires: build/OWzxSlicer.exe (canonical build) + python with Pillow.

param(
  [Parameter(Mandatory=$true)][ValidateSet("prepare","preview")]
  [string]$Page,
  [string]$Exe = "build/OWzxSlicer.exe",
  [string]$OutDir = "build/ui_diff",
  [int]$ClientWidth = 1366,
  [int]$ClientHeight = 721,
  [int]$WaitSeconds = 6,
  [string]$Upstream = "",
  [switch]$KeepOpen,          # leave the app running (manual inspection); skips capture
  [string]$DevQmlDir = "src/qml_gui"
)

$ErrorActionPreference = "Stop"
# Resolve paths against the repo root (script's parent dir), not the caller's
# cwd, so the loop works from any shell location.
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not [System.IO.Path]::IsPathRooted($Exe)) { $Exe = Join-Path $repoRoot $Exe }
if (-not [System.IO.Path]::IsPathRooted($OutDir)) { $OutDir = Join-Path $repoRoot $OutDir }

if (-not (Test-Path $Exe)) {
  Write-Error "OWzxSlicer.exe not found at: $Exe. Run scripts/auto_verify_with_vcvars.ps1 first."
  exit 1
}
if ($Upstream -eq "") { $Upstream = Join-Path $repoRoot "docs/visual-compare/upstream_$Page.png" }
if (-not (Test-Path $Upstream)) { Write-Error "upstream truth screenshot missing: $Upstream"; exit 1 }

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win {
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hWnd, IntPtr after,
      int x, int y, int cx, int cy, uint flags);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@
# Physical-pixel coordinates: Qt is per-monitor DPI aware; without this the
# PS process gets virtualized coords and the client size lands wrong.
[Win]::SetProcessDPIAware() | Out-Null

# Dev QML source override (interceptor added in main_qml.cpp).
$env:QML_DISABLE_DISK_CACHE = "1"
if ($DevQmlDir -ne "") {
  $env:OWZX_DEV_QML_DIR = Join-Path $repoRoot $DevQmlDir
} else {
  Remove-Item Env:OWZX_DEV_QML_DIR -ErrorAction SilentlyContinue
}

# Startup page: the --open-page deep link currently trips the known
# AI-SIDECAR-RACE startup crash (same ...27a2 AV signature since 2026-09-15,
# ~seconds after the delayed page switch; see crash_dumps/crash_stack.log and
# the AI-SIDECAR-RACE ledger rounds). The persisted startup-page preference
# lands on the same page via the first-frame StackLayout path and is
# crash-free, so drive THAT instead -- saved and restored around the run.
$pageIndex = @{ prepare = 1; preview = 2 }[$Page]
$regPath = 'HKCU:\Software\OWzx\OWzxSlicer'
$savedHome  = (Get-ItemProperty $regPath -Name showHomePage -ErrorAction SilentlyContinue).showHomePage
$savedDpage = (Get-ItemProperty $regPath -Name defaultPage  -ErrorAction SilentlyContinue).defaultPage
New-Item -Path $regPath -Force | Out-Null
Set-ItemProperty $regPath -Name showHomePage -Value 'false' -Type String
Set-ItemProperty $regPath -Name defaultPage  -Value "$pageIndex" -Type String

$launchArgs = @("--skip-first-run")
Write-Host "[loop] launching: $Exe $($launchArgs -join ' ')"
$proc = Start-Process -FilePath $Exe -ArgumentList $launchArgs -PassThru

try {
  # Wait for the Qt window. engine->load(main.qml) alone historically takes
  # ~26s (see build/startup_diagnostics.log), plus ~5s of static init on cold
  # start -- budget ~2 minutes and abort early if the process dies.
  $proc.Refresh()
  $hwnd = $proc.MainWindowHandle
  for ($i = 0; $i -lt 36 -and $hwnd -eq [IntPtr]::Zero; $i++) {
    Start-Sleep -Milliseconds 3000
    if ($proc.HasExited) {
      Write-Error "[loop] app exited during startup (exit code $($proc.ExitCode))."
      exit 1
    }
    $proc.Refresh()
    $hwnd = $proc.MainWindowHandle
  }
  if ($hwnd -eq [IntPtr]::Zero) {
    Write-Error "[loop] MainWindowHandle never appeared; aborting."
    exit 1
  }

  # Windowed at the upstream-truth client size. The frameless shell has no
  # native non-client area, so window rect == client rect.
  [Win]::ShowWindow($hwnd, 9) | Out-Null        # SW_RESTORE (default startup is maximized)
  Start-Sleep -Milliseconds 400
  [Win]::SetWindowPos($hwnd, [IntPtr]::Zero, 8, 8, $ClientWidth, $ClientHeight, 0x0004) | Out-Null  # SWP_NOZORDER
  [Win]::SetForegroundWindow($hwnd) | Out-Null
  Write-Host "[loop] waiting $WaitSeconds s for render + page settle..."
  Start-Sleep -Seconds $WaitSeconds

  if ($KeepOpen) {
    Write-Host "[loop] -KeepOpen: app left running (PID $($proc.Id)); capture skipped."
    return
  }

  # The handle can go stale across the restore/resize transition (Qt reapplies
  # visibility state async); refetch and retry the rect read before capturing.
  $rect = New-Object Win+RECT
  for ($attempt = 0; $attempt -lt 4; $attempt++) {
    $proc.Refresh()
    if ($proc.HasExited) { Write-Error "[loop] app exited before capture."; exit 1 }
    if ($proc.MainWindowHandle -ne [IntPtr]::Zero) { $hwnd = $proc.MainWindowHandle }
    [Win]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
    $width  = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    if ($width -gt 0 -and $height -gt 0) { break }
    Write-Warning "[loop] attempt $attempt : window rect ${width}x${height}; re-fetching handle..."
    Start-Sleep -Seconds 2
  }
  if ($width -ne $ClientWidth -or $height -ne $ClientHeight) {
    Write-Warning "[loop] window rect ${width}x${height} != client target ${ClientWidth}x${ClientHeight} (DPI or WM decorations?); capturing anyway."
  }

  New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
  $shotPath = Join-Path $OutDir "owzx_$Page.png"
  $bmp = New-Object System.Drawing.Bitmap($width, $height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
  $graphics = [System.Drawing.Graphics]::FromImage($bmp)
  $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, (New-Object System.Drawing.Size($width, $height)), [System.Drawing.CopyPixelOperation]::SourceCopy)
  $bmp.Save($shotPath, [System.Drawing.Imaging.ImageFormat]::Png)
  $graphics.Dispose(); $bmp.Dispose()
  Write-Host "[loop] captured: $shotPath"
}
finally {
  if (-not $KeepOpen) {
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
  }
  # Restore the user's startup-page preference.
  if ($null -eq $savedHome) {
    Remove-ItemProperty $regPath -Name showHomePage -ErrorAction SilentlyContinue
  } else {
    Set-ItemProperty $regPath -Name showHomePage -Value $savedHome -Type String
  }
  if ($null -eq $savedDpage) {
    Remove-ItemProperty $regPath -Name defaultPage -ErrorAction SilentlyContinue
  } else {
    Set-ItemProperty $regPath -Name defaultPage -Value $savedDpage -Type String
  }
}

if (-not $KeepOpen) {
  Write-Host "[loop] diffing against $Upstream ..."
  python (Join-Path $repoRoot "scripts/ui_diff_compare.py") --ours $shotPath --upstream $Upstream --out $OutDir --page $Page
}
