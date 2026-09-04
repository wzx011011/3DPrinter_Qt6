param([int]$ProcId, [string]$OutDir = "_orbit2")
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WinZ {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint data, UIntPtr extra);
  public struct R { public int L, T, Rt, B; }
}
"@
$p = Get-Process -Id $ProcId
[WinZ]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 1200
$r = New-Object WinZ+R
[WinZ]::GetWindowRect($p.MainWindowHandle, [ref]$r) | Out-Null
# reuse detected viewport rect from prior sweep: x 320..2556, y 72..1364
$AX0 = $r.L + 320; $AX1 = $r.L + 2556; $AY0 = $r.T + 72; $AY1 = $r.T + 1364
$VW = $AX1 - $AX0; $VH = $AY1 - $AY0
function Snap([string]$name) {
  Start-Sleep -Milliseconds 300
  $bmp = New-Object System.Drawing.Bitmap($VW, $VH)
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen($AX0, $AY0, 0, 0, (New-Object System.Drawing.Size($VW, $VH)))
  $g.Dispose()
  $path = Join-Path $OutDir "$name.png"
  $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
  $colors = @{}; $sum=0.0; $sum2=0.0; $n=0
  $rnd = New-Object System.Random(7)
  for ($i=0; $i -lt 4000; $i++) {
    $c = $bmp.GetPixel($rnd.Next(40,$VW-40), $rnd.Next(40,$VH-40))
    $lum = 0.299*$c.R + 0.587*$c.G + 0.114*$c.B
    $sum+=$lum; $sum2+=$lum*$lum; $n++
    $key = "{0:X2}{1:X2}{2:X2}" -f $c.R,$c.G,$c.B
    if (-not $colors.ContainsKey($key)) { $colors[$key]=0 }
    $colors[$key]++
  }
  $bmp.Dispose()
  $mean=$sum/$n; $std=[Math]::Sqrt([Math]::Max(0,$sum2/$n-$mean*$mean))
  Write-Output ("SNAP {0} mean={1:N1} std={2:N1} distinct={3}" -f $name,$mean,$std,$colors.Count)
}
function WheelAt([int]$x,[int]$y,[int]$delta,[int]$times) {
  [WinZ]::SetCursorPos($x,$y) | Out-Null
  Start-Sleep -Milliseconds 60
  $u = [System.BitConverter]::ToUInt32([System.BitConverter]::GetBytes([int]$delta),0)
  for ($i=0; $i -lt $times; $i++) {
    [WinZ]::mouse_event(0x0800,0,0,$u,[UIntPtr]::Zero)
    Start-Sleep -Milliseconds 25
  }
}
# zoom in deep over viewport center, snap; zoom out far, snap; then at below-horizon angle
$cx = [int]($AX0 + $VW*0.5); $cy = [int]($AY0 + $VH*0.45)
WheelAt $cx $cy -120 25
Snap "t3_zoomin"
WheelAt $cx $cy 120 40
Snap "t3_zoomout"
WheelAt $cx $cy -120 15
Snap "t3_zoommid"
# orbit to below-horizon then zoom (bed hidden + zoom interplay)
[WinZ]::SetCursorPos([int]($AX0+$VW*0.5),[int]($AY0+$VH*0.30)) | Out-Null
[WinZ]::mouse_event(0x0002,0,0,0,[UIntPtr]::Zero); Start-Sleep -Milliseconds 40
$cur=[System.Windows.Forms.Cursor]::Position
for ($i=1;$i -le 10;$i++){ [WinZ]::SetCursorPos($cur.X,$cur.Y + 45*$i) | Out-Null; Start-Sleep -Milliseconds 18 }
[WinZ]::mouse_event(0x0004,0,0,0,[UIntPtr]::Zero); Start-Sleep -Milliseconds 150
WheelAt $cx $cy -120 15
Snap "t3_below_zoomin"
WheelAt $cx $cy 120 20
Snap "t3_below_zoomout"
if ($p.HasExited) { Write-Output "APP-EXITED"; exit 1 }
Write-Output "ZOOM-SWEEP-DONE"
