param([int]$ProcId, [int]$X, [int]$Y, [int]$Clicks = 1)
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WinClick {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint data, UIntPtr extra);
}
"@
$p = Get-Process -Id $ProcId -ErrorAction Stop
[WinClick]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 800
for ($i = 0; $i -lt $Clicks; $i++) {
  [WinClick]::SetCursorPos($X, $Y) | Out-Null
  Start-Sleep -Milliseconds 150
  [WinClick]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds 70
  [WinClick]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds 350
}
if ($p.HasExited) { Write-Output "APP-EXITED"; exit 1 }
Write-Output "clicked ${X},${Y} x${Clicks}"
