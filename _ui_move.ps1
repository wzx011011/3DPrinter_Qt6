param([int]$ProcId, [int]$X, [int]$Y)
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WinMove {
  [DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr h, int x, int y, int w, int hh, bool repaint);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
}
"@
$p = Get-Process -Id $ProcId -ErrorAction Stop
[WinMove]::MoveWindow($p.MainWindowHandle, $X, $Y, 1280, 800, $true) | Out-Null
[WinMove]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Write-Output "moved to ${X},${Y}"
