param([int]$ProcId, [int]$X, [int]$Y, [int]$Clicks = 1)
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WinClick2 {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint data, UIntPtr extra);
  public struct R { public int L, T, Rt, B; }
}
"@
$p = Get-Process -Id $ProcId -ErrorAction Stop
[WinClick2]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 800
$r = New-Object WinClick2+R
[WinClick2]::GetWindowRect($p.MainWindowHandle, [ref]$r) | Out-Null
$ax = $r.L + $X; $ay = $r.T + $Y
for ($i = 0; $i -lt $Clicks; $i++) {
  [WinClick2]::SetCursorPos($ax, $ay) | Out-Null
  Start-Sleep -Milliseconds 150
  [WinClick2]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds 70
  [WinClick2]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds 350
}
if ($p.HasExited) { Write-Output "APP-EXITED"; exit 1 }
Write-Output "clicked rel ${X},${Y} (abs ${ax},${ay}) x${Clicks}"
