param([int]$Steps = 40)
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class M {
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint d, UIntPtr e);
}
"@
[M]::SetCursorPos(1050, 620) | Out-Null
Start-Sleep -Milliseconds 300
[M]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)   # LEFTDOWN
for ($i = 0; $i -lt $Steps; $i++) {
  $x = 1050 + [int](520 * $i / $Steps)
  $y = 620 + [int](180 * $i / $Steps)
  [M]::SetCursorPos($x, $y) | Out-Null
  Start-Sleep -Milliseconds 16
}
[M]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)   # LEFTUP
Start-Sleep -Milliseconds 200
# hover sweep over the model (no buttons)
for ($i = 0; $i -lt 60; $i++) {
  $x = 1000 + [int](560 * $i / 60)
  $y = 520 + [int](160 * [Math]::Sin($i / 6.0))
  [M]::SetCursorPos($x, $y) | Out-Null
  Start-Sleep -Milliseconds 16
}
Write-Host "input done"
