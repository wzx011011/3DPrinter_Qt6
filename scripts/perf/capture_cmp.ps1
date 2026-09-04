param([int]$Pid_ = 0)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class M {
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint d, UIntPtr e);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
}
"@
function Shot([string]$path) {
  $b = New-Object System.Drawing.Bitmap(500, 500)
  $g = [System.Drawing.Graphics]::FromImage($b)
  $g.CopyFromScreen(1300, 500, 0, 0, (New-Object System.Drawing.Size(500, 500)))
  $g.Dispose(); $b.Save($path, [System.Drawing.Imaging.ImageFormat]::Png); $b.Dispose()
  (Get-FileHash $path -Algorithm MD5).Hash
}
$p = Get-Process -Id $Pid_ -ErrorAction Stop
[M]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 500
[M]::SetCursorPos(1550, 750) | Out-Null
Start-Sleep -Milliseconds 300
$h1 = Shot "E:\ai\3DPrinter_Qt6\build\shot_a.png"
for ($i = 0; $i -lt 10; $i++) { [M]::mouse_event(0x0800, 0, 0, 4294967196, [UIntPtr]::Zero); Start-Sleep -Milliseconds 60 }
Start-Sleep -Milliseconds 500
$h2 = Shot "E:\ai\3DPrinter_Qt6\build\shot_b.png"
for ($i = 0; $i -lt 14; $i++) { [M]::mouse_event(0x0800, 0, 0, 100, [UIntPtr]::Zero); Start-Sleep -Milliseconds 60 }
Start-Sleep -Milliseconds 500
$h3 = Shot "E:\ai\3DPrinter_Qt6\build\shot_c.png"
Write-Host ("a=" + $h1.Substring(0,8) + " b=" + $h2.Substring(0,8) + " c=" + $h3.Substring(0,8))
