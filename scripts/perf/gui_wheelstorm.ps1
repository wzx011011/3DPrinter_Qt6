# Wheel-zoom storm: every detent triggers a viewport repaint (camera zoom ->
# update() -> paint), which the earlier orbit drags could not guarantee (their
# drag rectangle partly overlapped the left sidebar). 600 detents at ~30 ms
# pacing gives the swrender.frame bucket (120 paints) multiple fills.
param([int]$Pid_ = 0)
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class M3 {
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint d, UIntPtr e);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
}
"@
$p = Get-Process -Id $Pid_ -ErrorAction Stop
[M3]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 700
# Park the cursor in the middle of the 3D viewport (verified live area).
[M3]::SetCursorPos(1550, 750) | Out-Null
Start-Sleep -Milliseconds 400
for ($i = 0; $i -lt 300; $i++) {
  # -120 detent (zoom out) / +120 (zoom in), alternating every 25 ticks so the
  # model stays roughly in place.
  $dir = if (([int]($i / 25)) % 2 -eq 0) { 4294967176 } else { 120 }
  [M3]::mouse_event(0x0800, 0, 0, $dir, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds 30
}
Write-Host "wheelstorm done"
