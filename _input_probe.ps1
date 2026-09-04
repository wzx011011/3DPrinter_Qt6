param([int]$ProcId, [string]$OutDir, [string]$Tag)
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WinProbe {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint data, UIntPtr extra);
  [DllImport("user32.dll")] public static extern uint GetDpiForWindow(IntPtr h);
  public struct R { public int L, T, Rt, B; }
}
"@
Add-Type -AssemblyName System.Drawing
$p = Get-Process -Id $ProcId -ErrorAction Stop
[WinProbe]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 900
$dpi = [WinProbe]::GetDpiForWindow($p.MainWindowHandle)
Write-Output ("DPI={0} (scale={1:F2})" -f $dpi, ($dpi/96.0))
$r = New-Object WinProbe+R
[WinProbe]::GetWindowRect($p.MainWindowHandle, [ref]$r) | Out-Null
$w = $r.Rt - $r.L; $h = $r.B - $r.T

function Snap([string]$name) {
  Start-Sleep -Milliseconds 400
  $bmp = New-Object System.Drawing.Bitmap($w, $h)
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen($r.L, $r.T, 0, 0, $bmp.Size)
  $g.Dispose()
  $x0 = [int]($w*0.30); $x1 = [int]($w*0.95); $y0 = [int]($h*0.15); $y1 = [int]($h*0.90)
  $sum = 0.0; $sum2 = 0.0; $n = 0
  for ($sx = 0; $sx -lt 30; $sx++) {
    for ($sy = 0; $sy -lt 20; $sy++) {
      $px = $bmp.GetPixel($x0 + [int](($x1-$x0)*$sx/29), $y0 + [int](($y1-$y0)*$sy/19))
      $v = ($px.R + $px.G + $px.B) / 3.0
      $sum += $v; $sum2 += $v*$v; $n++
    }
  }
  $bmp.Save("$OutDir\$Tag`_$name.png", [System.Drawing.Imaging.ImageFormat]::Png)
  $bmp.Dispose()
  $mean = $sum/$n; $std = [Math]::Sqrt([Math]::Max(0.0, $sum2/$n - $mean*$mean))
  Write-Output ("{0}`tmean={1:F1}`tstd={2:F1}" -f $name, $mean, $std)
}

function BtnDrag([string]$btn, [int]$sx0, [int]$sy0, [int]$dx, [int]$dy) {
  $down = 0x0002; $up = 0x0004
  if ($btn -eq 'M') { $down = 0x0020; $up = 0x0040 }
  if ($btn -eq 'R') { $down = 0x0008; $up = 0x0010 }
  [WinProbe]::SetCursorPos($r.L + $sx0, $r.T + $sy0) | Out-Null
  Start-Sleep -Milliseconds 250
  # hover wiggle to establish tracking
  [WinProbe]::SetCursorPos($r.L + $sx0 + 4, $r.T + $sy0 + 3) | Out-Null
  Start-Sleep -Milliseconds 120
  [WinProbe]::SetCursorPos($r.L + $sx0, $r.T + $sy0) | Out-Null
  Start-Sleep -Milliseconds 120
  [WinProbe]::mouse_event($down, 0, 0, 0, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds 90
  for ($i = 1; $i -le 16; $i++) {
    [WinProbe]::SetCursorPos($r.L + $sx0 + [int]($dx*$i/16), $r.T + $sy0 + [int]($dy*$i/16)) | Out-Null
    Start-Sleep -Milliseconds 35
  }
  [WinProbe]::mouse_event($up, 0, 0, 0, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds 250
}

Snap "00_base"
BtnDrag 'L' 750 450 0 -120; Snap "01_leftdrag_empty"
BtnDrag 'L' 850 330 0 -120; Snap "02_leftdrag_model"
BtnDrag 'M' 750 420 150 0;  Snap "03_middledrag_pan"
BtnDrag 'R' 750 420 160 0;  Snap "04_rightdrag_azim"
BtnDrag 'R' 750 420 0 120;  Snap "05_rightdrag_pitch"
[WinProbe]::SetCursorPos($r.L + 750, $r.T + 420) | Out-Null
[WinProbe]::mouse_event(0x0800, 0, 0, [uint32]120, [UIntPtr]::Zero)
Start-Sleep -Milliseconds 300
Snap "06_wheel"
if ($p.HasExited) { Write-Output "APP-EXITED"; exit 1 }
Write-Output "PROBE-DONE"
