param([int]$ProcId, [string]$OutPath)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WinRect {
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
  public struct R { public int L, T, Rt, B; }
}
"@
$p = Get-Process -Id $ProcId -ErrorAction Stop
$r = New-Object WinRect+R
[WinRect]::GetWindowRect($p.MainWindowHandle, [ref]$r) | Out-Null
$w = $r.Rt - $r.L; $h = $r.B - $r.T
if ($w -le 0 -or $h -le 0) { Write-Output "BADRECT ${w}x${h}"; exit 1 }
$bmp = New-Object System.Drawing.Bitmap($w, $h)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($r.L, $r.T, 0, 0, (New-Object System.Drawing.Size($w, $h)))
$g.Dispose()
$bmp.Save($OutPath, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Output "SAVED $OutPath ${w}x${h} rect=($($r.L),$($r.T),$($r.Rt),$($r.B))"
