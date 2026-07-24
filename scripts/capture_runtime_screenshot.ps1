# capture_runtime_screenshot.ps1
# Phase v5.1: runtime screenshot capture for visual verification.
# Mirrors the v3.7 / v4.x capture pattern: launch OWzxSlicer with deep-link args
# (frameSwapped one-shot gate ensures at least one frame rendered), then capture
# the window via System.Drawing.CopyFromScreen and save to .planning/milestones/.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File scripts/capture_runtime_screenshot.ps1 -Page prepare -Out v5.1-runtime-prepare.png
#   powershell -ExecutionPolicy Bypass -File scripts/capture_runtime_screenshot.ps1 -Page preview -Out v5.1-runtime-preview.png
#   powershell -ExecutionPolicy Bypass -File scripts/capture_runtime_screenshot.ps1 -OpenDialog settings -Out v5.1-runtime-settings.png
#
# Required: build/OWzxSlicer.exe must exist (built via auto_verify_with_vcvars.ps1).

param(
  [Parameter(Mandatory=$true)]
  [string]$Out,
  [string]$Page = "",
  [string]$OpenDialog = "",
  [string]$Model = "",
  [int]$WaitSeconds = 6,
  [string]$Exe = "build/OWzxSlicer.exe"
)

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

if (-not (Test-Path $Exe)) {
  Write-Error "OWzxSlicer.exe not found at: $Exe. Run scripts/auto_verify_with_vcvars.ps1 first."
  exit 1
}

# Build deep-link args (mirrors v3.7 / v4.x frameSwapped-gated launch).
$args = @("--skip-first-run")
if ($Page)    { $args += @("--open-page", $Page) }
if ($OpenDialog) { $args += @("--open-dialog", $OpenDialog) }
if ($Model)   { $args += @("--load-model", $Model) }

Write-Host "[capture] launching: $Exe $($args -join ' ')"
$proc = Start-Process -FilePath $Exe -ArgumentList $args -PassThru

# Wait for the first frameSwapped gate + page navigation + model load.
Write-Host "[capture] waiting $WaitSeconds s for app to render..."
Start-Sleep -Seconds $WaitSeconds

# Capture the foreground window.
# Find the OWzxSlicer top-level window handle.
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Drawing;
public class Win {
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@

# Bring the just-launched app to the foreground (in case another window stole focus).
# Refresh process so MainWindowHandle is populated (Qt window is created async
# after QApplication event loop starts; calling MainWindowHandle without Refresh
# returns empty and breaks SetForegroundWindow).
$proc.Refresh()
$hwnd = $proc.MainWindowHandle
if ($hwnd -eq [IntPtr]::Zero) {
  # Fallback: small retry loop if Qt hasn't created its window yet.
  for ($i = 0; $i -lt 8 -and $hwnd -eq [IntPtr]::Zero; $i++) {
    Start-Sleep -Milliseconds 500
    $proc.Refresh()
    $hwnd = $proc.MainWindowHandle
  }
}
if ($hwnd -eq [IntPtr]::Zero) {
  Write-Error "[capture] MainWindowHandle still zero after refresh+retries; aborting."
  Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
  exit 1
}
[Win]::SetForegroundWindow($hwnd) | Out-Null
Start-Sleep -Milliseconds 500
# Verify the foreground window is actually ours; if not, the foreground was
# stolen (e.g. by a remote-control helper) and we still capture our window by
# its own rect rather than trusting GetForegroundWindow.
$rect = New-Object Win+RECT
[Win]::GetWindowRect($hwnd, [ref]$rect) | Out-Null

$width  = $rect.Right - $rect.Left
$height = $rect.Bottom - $rect.Top
if ($width -le 0 -or $height -le 0) {
  Write-Error "[capture] invalid window rect (${width}x${height}); capture aborted."
  Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
  exit 1
}

$bmp = New-Object System.Drawing.Bitmap($width, $height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$graphics = [System.Drawing.Graphics]::FromImage($bmp)
$graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, (New-Object System.Drawing.Size($width, $height)), [System.Drawing.CopyPixelOperation]::SourceCopy)

# Resolve output path relative to repo root.
$outPath = if ([System.IO.Path]::IsPathRooted($Out)) { $Out } else { Join-Path (Get-Location) $Out }
$bmp.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
$graphics.Dispose()
$bmp.Dispose()

Write-Host "[capture] saved: $outPath ($width x $height)"
Write-Host "[capture] stopping OWzxSlicer (PID $($proc.Id))..."
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
exit 0
