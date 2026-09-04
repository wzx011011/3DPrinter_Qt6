param([int]$ProcId, [string]$OutDir, [string]$Tag)
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WinSweep {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint data, UIntPtr extra);
  public struct R { public int L, T, Rt, B; }
}
"@
Add-Type -AssemblyName System.Drawing
$p = Get-Process -Id $ProcId -ErrorAction Stop
[WinSweep]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 900
$r = New-Object WinSweep+R
[WinSweep]::GetWindowRect($p.MainWindowHandle, [ref]$r) | Out-Null
$w = $r.Rt - $r.L; $h = $r.B - $r.T
$cx = $r.L + [int]($w * 0.55); $cy = $r.T + [int]($h * 0.52)

function Snap([string]$name) {
  Start-Sleep -Milliseconds 350
  $bmp = New-Object System.Drawing.Bitmap($w, $h)
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen($r.L, $r.T, 0, 0, $bmp.Size)
  $g.Dispose()
  # sample central viewport grid
  $x0 = [int]($w*0.15); $x1 = [int]($w*0.85); $y0 = [int]($h*0.20); $y1 = [int]($h*0.82)
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
  $flag = ""; if ($std -lt 3.0) { $flag = "BLANK?" }
  Write-Output ("{0}`tmean={1:F1}`tstd={2:F1}`t{3}" -f $name, $mean, $std, $flag)
}

function Drag([int]$dx, [int]$dy) {
  [WinSweep]::SetCursorPos($cx, $cy) | Out-Null
  Start-Sleep -Milliseconds 100
  [WinSweep]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds 60
  $steps = 10
  for ($i = 1; $i -le $steps; $i++) {
    [WinSweep]::SetCursorPos($cx + [int]($dx*$i/$steps), $cy + [int]($dy*$i/$steps)) | Out-Null
    Start-Sleep -Milliseconds 22
  }
  [WinSweep]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds 120
}

Snap "00_start"
# pitch up (drag down = look from above? depends on mapping) — sweep vertical both ways
for ($i = 1; $i -le 6; $i++) { Drag 0 110; Snap ("pitchA_{0:00}" -f $i) }
for ($i = 1; $i -le 12; $i++) { Drag 0 -110; Snap ("pitchB_{0:00}" -f $i) }
for ($i = 1; $i -le 6; $i++) { Drag 0 110; Snap ("pitchC_{0:00}" -f $i) }
# azimuth full circle
for ($i = 1; $i -le 12; $i++) { Drag 130 0; Snap ("azim_{0:00}" -f $i) }
# zoom extremes
for ($i = 1; $i -le 12; $i++) { [WinSweep]::SetCursorPos($cx, $cy) | Out-Null; [WinSweep]::mouse_event(0x0800, 0, 0, [uint32]4288100000, [UIntPtr]::Zero); Start-Sleep -Milliseconds 80 }; Snap "zoomin_far"
for ($i = 1; $i -le 20; $i++) { [WinSweep]::SetCursorPos($cx, $cy) | Out-Null; [WinSweep]::mouse_event(0x0800, 0, 0, [uint32]120, [UIntPtr]::Zero); Start-Sleep -Milliseconds 60 }; Snap "zoomout_near"
if ($p.HasExited) { Write-Output "APP-EXITED"; exit 1 }
Write-Output "SWEEP-DONE"
