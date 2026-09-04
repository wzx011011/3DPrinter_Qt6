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
[M]::SetCursorPos(1500, 750) | Out-Null
Start-Sleep -Milliseconds 250
# wheel zoom over the viewport (forces renders)
for ($i = 0; $i -lt 12; $i++) { [M]::mouse_event(0x0800, 0, 0, (4294967196), [UIntPtr]::Zero); Start-Sleep -Milliseconds 40 }  # -100 detents
for ($i = 0; $i -lt 12; $i++) { [M]::mouse_event(0x0800, 0, 0, 100, [UIntPtr]::Zero); Start-Sleep -Milliseconds 40 }
Start-Sleep -Milliseconds 200
# orbit drag inside the viewport
[M]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
for ($i = 0; $i -lt 140; $i++) {
  $x = 1200 + [int](700 * $i / 140)
  $y = 680 + [int](240 * $i / 140)
  [M]::SetCursorPos($x, $y) | Out-Null
  Start-Sleep -Milliseconds 12
}
[M]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
Start-Sleep -Milliseconds 300
# hover sweep
for ($i = 0; $i -lt 150; $i++) {
  $x = 1150 + [int](700 * $i / 150)
  $y = 560 + [int](280 * (0.5 + 0.5 * [Math]::Sin($i / 8.0)))
  [M]::SetCursorPos($x, $y) | Out-Null
  Start-Sleep -Milliseconds 10
}
Write-Host "orbit2 done"
