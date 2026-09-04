param([int]$Pid_ = 0)
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class M {
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint d, UIntPtr e);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
}
"@
$p = Get-Process -Id $Pid_ -ErrorAction Stop
[M]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 600
# orbit: left drag across the 3D viewport
[M]::SetCursorPos(1050, 620) | Out-Null
Start-Sleep -Milliseconds 250
[M]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
for ($i = 0; $i -lt 90; $i++) {
  $x = 1050 + [int](520 * $i / 90)
  $y = 620 + [int](160 * $i / 90)
  [M]::SetCursorPos($x, $y) | Out-Null
  Start-Sleep -Milliseconds 14
}
[M]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
Start-Sleep -Milliseconds 300
# hover sweep (no buttons) across the model
for ($i = 0; $i -lt 120; $i++) {
  $x = 980 + [int](600 * $i / 120)
  $y = 500 + [int](220 * (0.5 + 0.5 * [Math]::Sin($i / 7.0)))
  [M]::SetCursorPos($x, $y) | Out-Null
  Start-Sleep -Milliseconds 12
}
Write-Host "fg input done"
