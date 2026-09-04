param(
  [int]$ProcId,
  [string]$OutDir = "_orbit",
  [switch]$SkipDetection
)
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WinInput2 {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint data, UIntPtr extra);
  public struct R { public int L, T, Rt, B; }
}
"@
function Screenshot([int]$px, [int]$py, [int]$pw, [int]$ph, [string]$path) {
  $bmp = New-Object System.Drawing.Bitmap($pw, $ph)
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen($px, $py, 0, 0, (New-Object System.Drawing.Size($pw, $ph)))
  $g.Dispose()
  $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
  $bmp.Dispose()
}
function AnalyzeRegion([string]$path, [int]$x0, [int]$y0, [int]$x1, [int]$y1) {
  $bmp = [System.Drawing.Bitmap]::FromFile((Resolve-Path $path))
  $colors = @{}; $sum = 0.0; $sum2 = 0.0; $n = 0; $bg = 0
  $rnd = New-Object System.Random(7)
  for ($i = 0; $i -lt 4000; $i++) {
    $sx = $rnd.Next($x0, $x1 + 1); $sy = $rnd.Next($y0, $y1 + 1)
    $c = $bmp.GetPixel($sx, $sy)
    $lum = 0.299 * $c.R + 0.587 * $c.G + 0.114 * $c.B
    $sum += $lum; $sum2 += $lum * $lum; $n++
    if ([Math]::Abs($c.R - 0x44) -le 6 -and [Math]::Abs($c.G - 0x47) -le 6 -and [Math]::Abs($c.B - 0x47) -le 6) { $bg++ }
    $key = "{0:X2}{1:X2}{2:X2}" -f $c.R, $c.G, $c.B
    if (-not $colors.ContainsKey($key)) { $colors[$key] = 0 }
    $colors[$key]++
  }
  $bmp.Dispose()
  $mean = $sum / $n; $std = [Math]::Sqrt([Math]::Max(0, $sum2 / $n - $mean * $mean))
  $bgFrac = $bg / $n
  "{0:N1} {1:N1} {2} {3:N4}" -f $mean, $std, $colors.Count, $bgFrac
}
function ImgDiff([string]$a, [string]$b) {
  $ba = [System.Drawing.Bitmap]::FromFile((Resolve-Path $a))
  $bb = [System.Drawing.Bitmap]::FromFile((Resolve-Path $b))
  $diff = 0; $n = 0; $step = 7
  for ($y = 0; $y -lt $ba.Height; $y += $step) {
    for ($x = 0; $x -lt $ba.Width; $x += $step) {
      $ca = $ba.GetPixel($x, $y); $cb = $bb.GetPixel($x, $y)
      if ([Math]::Abs($ca.R - $cb.R) -gt 12 -or [Math]::Abs($ca.G - $cb.G) -gt 12 -or [Math]::Abs($ca.B - $cb.B) -gt 12) { $diff++ }
      $n++
    }
  }
  $ba.Dispose(); $bb.Dispose()
  $diff / [Math]::Max(1, $n)
}

$p = Get-Process -Id $ProcId
[WinInput2]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 1500
$r = New-Object WinInput2+R
[WinInput2]::GetWindowRect($p.MainWindowHandle, [ref]$r) | Out-Null
$W = $r.Rt - $r.L; $H = $r.B - $r.T
Write-Output "WINDOW $W x $H at ($($r.L),$($r.T))"

# --- detect viewport rect via pixel scan (viewport bg ~444747) ---
$probe = "$env:TEMP\_probe_vp.png"
Screenshot $r.L $r.T $W $H $probe
$bmp = [System.Drawing.Bitmap]::FromFile($probe)
function IsVP($c) { [Math]::Abs($c.R - 0x44) -le 10 -and [Math]::Abs($c.G - 0x47) -le 10 -and [Math]::Abs($c.B - 0x47) -le 10 }
$midY = [int]($H * 0.55)
$vx0 = -1; $vx1 = -1
for ($x = 0; $x -lt $W; $x += 4) { if (IsVP $bmp.GetPixel($x, $midY)) { if ($vx0 -lt 0) { $vx0 = $x }; $vx1 = $x } }
$midX = [int](($vx0 + $vx1) / 2)
$vy0 = -1; $vy1 = -1
for ($y = 0; $y -lt $H; $y += 4) { if (IsVP $bmp.GetPixel($midX, $y)) { if ($vy0 -lt 0) { $vy0 = $y }; $vy1 = $y } }
$bmp.Dispose()
Remove-Item $probe
Write-Output "VIEWPORT x:[$vx0,$vx1] y:[$vy0,$vy1] (window-relative)"

if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Path $OutDir | Out-Null }
# absolute screen coords
$AX0 = $r.L + $vx0; $AX1 = $r.L + $vx1; $AY0 = $r.T + $vy0; $AY1 = $r.T + $vy1
$VW = $vx1 - $vx0; $VH = $vy1 - $vy0
$M = 60  # margin inside viewport for drag start points

$script:shots = @()
function Snap([string]$name) {
  Start-Sleep -Milliseconds 300
  $path = Join-Path $OutDir "$name.png"
  Screenshot $AX0 $AY0 $VW $VH $path
  $script:shots += $path
  if ($SkipDetection) { Write-Output "SNAP $name"; return }
  $stats = AnalyzeRegion $path 40 40 ($VW - 40) ($VH - 40)
  Write-Output ("SNAP {0} mean/std/distinct/bgFrac: {1}" -f $name, $stats)
}
function MoveTo($x, $y) { [WinInput2]::SetCursorPos([int]$x, [int]$y) | Out-Null; Start-Sleep -Milliseconds 30 }
function Stroke([single]$dx, [single]$dy, [int]$steps = 8) {
  # press at current pos, drag by (dx,dy) in steps, release
  [WinInput2]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds 40
  $cur = [System.Windows.Forms.Cursor]::Position
  for ($i = 1; $i -le $steps; $i++) {
    MoveTo ($cur.X + $dx * $i / $steps) ($cur.Y + $dy * $i / $steps)
    Start-Sleep -Milliseconds 18
  }
  [WinInput2]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds 60
}
function Wheel([int]$delta, [int]$times) {
  # mouse_event wheel data is a signed value reinterpreted as uint32.
  $u = [System.BitConverter]::ToUInt32([System.BitConverter]::GetBytes([int]$delta), 0)
  for ($i = 0; $i -lt $times; $i++) {
    [WinInput2]::mouse_event(0x0800, 0, 0, $u, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 30
  }
}

Add-Type -AssemblyName System.Windows.Forms

# baseline
MoveTo ($AX0 + $VW * 0.5) ($AY0 + $VH * 0.30)
Snap "t000_baseline"

# ============ PHASE A: elevation ladder with azimuth sweep per rung ============
# ladder from top (+89) down to bottom (-89). Each rung: full 360-deg azimuth sweep
# 0.5 deg per px => 720px per revolution, in strokes of 120px
$rungs = @(
  @{ name = "e89top";   vdy = -260 },   # drag up 260px from baseline el35 -> clamped +89
  @{ name = "e60";      vdy = 58 },     # down 58px -> ~+60
  @{ name = "e30";      vdy = 60 },     # -> ~+30
  @{ name = "e0";       vdy = 60 },     # -> ~0 horizon
  @{ name = "em40";     vdy = 80 },     # -> ~-40
  @{ name = "em89bot";  vdy = 98 }      # -> clamped -89
)
$prevShot = $script:shots[-1]
foreach ($run in $rungs) {
  MoveTo ($AX0 + $VW * 0.50) ($AY0 + $VH * 0.30)
  Stroke 0 $run.vdy 10
  Snap ("t1_{0}_after_vdrag" -f $run.name)
  $last = $script:shots[-1]
  $d = ImgDiff $prevShot $last
  Write-Output ("  VDRAG {0}: diff-vs-prev {1:N3}" -f $run.name, $d)
  $prevShot = $last
  # azimuth sweep: 3 strokes of 240px right (=120deg each), snap after each
  for ($seg = 1; $seg -le 3; $seg++) {
    MoveTo ($AX0 + $VW * 0.15) ($AY0 + $VH * 0.30)
    Stroke 240 0 10
    Snap ("t1_{0}_az{1}" -f $run.name, ($seg * 120))
    $last = $script:shots[-1]
    $d = ImgDiff $prevShot $last
    Write-Output ("  AZSEG {0} #{1}: diff-vs-prev {2:N3}" -f $run.name, $seg, $d)
    $prevShot = $last
  }
}

# ============ PHASE B: pan extremes at mid elevation ============
MoveTo ($AX0 + $VW * 0.5) ($AY0 + $VH * 0.5)
# return elevation to ~+35 by dragging up from current -89: +248px
Stroke 0 -250 10
Snap "t2_mid_after_reset"
$prevShot = $script:shots[-1]
# middle-button pan far left then right then up then down
foreach ($dir in @(@(-220, 0), @(220, 0), @(0, -180), @(0, 180))) {
  MoveTo ($AX0 + $VW * 0.5) ($AY0 + $VH * 0.5)
  # middle button drag
  [WinInput2]::mouse_event(0x0020, 0, 0, 0, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds 40
  $cur = [System.Windows.Forms.Cursor]::Position
  $ddx = $dir[0]; $ddy = $dir[1]
  for ($i = 1; $i -le 8; $i++) { MoveTo ($cur.X + $ddx * $i / 8) ($cur.Y + $ddy * $i / 8); Start-Sleep -Milliseconds 18 }
  [WinInput2]::mouse_event(0x0040, 0, 0, 0, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds 150
  $nm = "t2_pan_{0}_{1}" -f $ddx, $ddy
  Snap $nm
  $last = $script:shots[-1]
  $d = ImgDiff $prevShot $last
  Write-Output ("  PAN {0},{1}: diff-vs-prev {2:N3}" -f $ddx, $ddy, $d)
  $prevShot = $last
}

# ============ PHASE C: zoom extremes ============
MoveTo ($AX0 + $VW * 0.5) ($AY0 + $VH * 0.5)
Wheel -120 20   # zoom in
Snap "t3_zoomin"
Wheel 120 30    # zoom out far
Snap "t3_zoomout"
Wheel -120 12   # back closer
Snap "t3_zoommid"

if ($p.HasExited) { Write-Output "APP-EXITED-DURING-SWEEP"; exit 1 }
Write-Output "SWEEP-DONE total-shots=$($script:shots.Count)"
