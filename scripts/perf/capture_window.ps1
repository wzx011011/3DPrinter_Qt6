# Capture a window by PID using PrintWindow (works for D3D/composition surfaces)
param([int]$TargetPid, [string]$OutPath)
Add-Type -AssemblyName System.Drawing
Add-Type '
using System;
using System.Runtime.InteropServices;
public static class PW {
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
    public struct RECT { public int Left, Top, Right, Bottom; }
}'
$p = Get-Process -Id $TargetPid -ErrorAction Stop
$h = $p.MainWindowHandle
if ($h -eq [IntPtr]::Zero) { Write-Host "no window"; exit 1 }
[PW]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Milliseconds 500
$r = New-Object PW+RECT
[PW]::GetWindowRect($h, [ref]$r) | Out-Null
$w = $r.Right - $r.Left
$ht = $r.Bottom - $r.Top
Write-Host ("rect={0},{1} {2}x{3}" -f $r.Left, $r.Top, $w, $ht)
$b = New-Object System.Drawing.Bitmap($w, $ht)
$g = [System.Drawing.Graphics]::FromImage($b)
$dc = $g.GetHdc()
[PW]::PrintWindow($h, $dc, 2) | Out-Null  # PW_RENDERFULLCONTENT
$g.ReleaseHdc($dc)
$g.Dispose()
$b.Save($OutPath, [System.Drawing.Imaging.ImageFormat]::Png)
$b.Dispose()
Write-Host ("saved " + $OutPath)
