# -*- coding: utf-8 -*-
param([int]$ProcId)
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public class PW {
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
  public struct R { public int L, T, Rt, B; }
}
'@
Add-Type -AssemblyName System.Drawing

$p = Get-Process -Id $ProcId
$h = $p.MainWindowHandle
$r = New-Object PW+R
[void][PW]::GetWindowRect($h, [ref]$r)
$w = $r.Rt - $r.L; $ht = $r.B - $r.T
if ($w -le 0 -or $ht -le 0) { Write-Output "BADRECT"; exit 1 }

$bmp = New-Object System.Drawing.Bitmap($w, $ht)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$dc = $g.GetHdc()
# PW_RENDERFULLCONTENT = 2: captures DirectComposition/D3D content
[void][PW]::PrintWindow($h, $dc, 2)
$g.ReleaseHdc($dc)
$g.Dispose()
$bmp.Save('E:\ai\3DPrinter_Qt6\_effect_full.png', [System.Drawing.Imaging.ImageFormat]::Png)

$crop = $bmp.Clone((New-Object System.Drawing.Rectangle(300, 980, 700, 400)), $bmp.PixelFormat)
$big = New-Object System.Drawing.Bitmap(1400, 800)
$g2 = [System.Drawing.Graphics]::FromImage($big)
$g2.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g2.DrawImage($crop, 0, 0, 1400, 800)
$g2.Dispose()
$big.Save('E:\ai\3DPrinter_Qt6\_effect_cube_2x.png', [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose(); $crop.Dispose(); $big.Dispose()

# quick sanity: report mean luminance
$colors = @{}; $sum = 0.0; $n = 0
$bmp2 = [System.Drawing.Bitmap]::FromFile('E:\ai\3DPrinter_Qt6\_effect_full.png')
$rnd = New-Object System.Random(7)
for ($i = 0; $i -lt 3000; $i++) {
  $c = $bmp2.GetPixel($rnd.Next(0, $w), $rnd.Next(0, $ht))
  $sum += 0.299 * $c.R + 0.587 * $c.G + 0.114 * $c.B; $n++
}
$bmp2.Dispose()
Write-Output ("PRINTWINDOW-SAVED ${w}x${ht} lumMean={0:N1}" -f ($sum / $n))
