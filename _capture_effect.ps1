# -*- coding: utf-8 -*-
param([int]$ProcId)
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public class RS9 {
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
}
'@
Add-Type -AssemblyName System.Drawing

$p = Get-Process -Id $ProcId
$h = $p.MainWindowHandle
[void][RS9]::SetWindowPos($h, [IntPtr]::Zero, 0, 0, 2558, 1390, 0x0004)
Start-Sleep -Milliseconds 500
[void][RS9]::SetWindowPos($h, [IntPtr]::Zero, 0, 0, 2560, 1392, 0x0004)
[void][RS9]::SetForegroundWindow($h)
Start-Sleep -Milliseconds 2500

# full window shot
$bmp = New-Object System.Drawing.Bitmap(2560, 1392)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen(0, 0, 0, 0, (New-Object System.Drawing.Size(2560, 1392)))
$g.Dispose()
$bmp.Save('E:\ai\3DPrinter_Qt6\_effect_full.png', [System.Drawing.Imaging.ImageFormat]::Png)

# crop bottom-left cube area (viewport starts near x=320; cube at its left-bottom),
# upscale 2x with high-quality interpolation
$crop = $bmp.Clone((New-Object System.Drawing.Rectangle(300, 980, 700, 400)), $bmp.PixelFormat)
$big = New-Object System.Drawing.Bitmap(1400, 800)
$g2 = [System.Drawing.Graphics]::FromImage($big)
$g2.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g2.DrawImage($crop, 0, 0, 1400, 800)
$g2.Dispose()
$big.Save('E:\ai\3DPrinter_Qt6\_effect_cube_2x.png', [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose(); $crop.Dispose(); $big.Dispose()
Write-Output 'SAVED _effect_full.png + _effect_cube_2x.png'
