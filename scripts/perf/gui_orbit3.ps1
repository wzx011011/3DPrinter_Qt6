# Wave 2 GUI perf input driver: long orbit sweeps + long hover sweep over
# the 3D viewport, with explicit foreground activation (docs/perf-baseline.md).
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
# Long orbit: 5 circular sweeps, each ~2.4 s of left-drag.
for ($s = 0; $s -lt 5; $s++) {
  $x0 = 1250 + ($s % 2) * 200
  [M]::SetCursorPos($x0, 700) | Out-Null
  Start-Sleep -Milliseconds 150
  [M]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
  for ($i = 0; $i -lt 160; $i++) {
    $x = $x0 + [int](560 * [Math]::Sin(2 * [Math]::PI * $i / 160))
    $y = 620 + [int](200 * [Math]::Sin(2 * [Math]::PI * ($s + 1) * $i / 160))
    [M]::SetCursorPos($x, $y) | Out-Null
    Start-Sleep -Milliseconds 15
  }
  [M]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds 250
}
# Long hover sweep (no buttons) across the model.
for ($i = 0; $i -lt 320; $i++) {
  $x = 1250 + [int](620 * (0.5 + 0.5 * [Math]::Sin($i / 23.0)))
  $y = 480 + [int](320 * (0.5 + 0.5 * [Math]::Sin($i / 17.0 + 1.0)))
  [M]::SetCursorPos($x, $y) | Out-Null
  Start-Sleep -Milliseconds 10
}
Write-Host "orbit3 done"
