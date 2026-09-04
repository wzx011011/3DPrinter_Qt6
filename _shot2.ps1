param([int]$ProcId, [string]$Out)
Add-Type -AssemblyName System.Drawing
$p = Get-Process -Id $ProcId -ErrorAction Stop
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win3 {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
  public struct R { public int L, T, Rt, B; }
}
"@
[Win3]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 900
$r = New-Object Win3+R
[Win3]::GetWindowRect($p.MainWindowHandle, [ref]$r) | Out-Null
$w = $r.Rt - $r.L; $h = $r.B - $r.T
$bmp = New-Object System.Drawing.Bitmap($w, $h)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($r.L, $r.T, 0, 0, $bmp.Size)
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Output "saved ${w}x${h}"
