param([int]$ProcId, [int]$Rounds = 5)
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WinInput {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint data, UIntPtr extra);
  public struct R { public int L, T, Rt, B; }
}
"@
$p = Get-Process -Id $ProcId -ErrorAction Stop
[WinInput]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 1200
$r = New-Object WinInput+R
[WinInput]::GetWindowRect($p.MainWindowHandle, [ref]$r) | Out-Null
$w = $r.Rt - $r.L; $h = $r.B - $r.T
# Viewport center region (middle of the window, slightly below the toolbar)
$cx = $r.L + [int]($w * 0.55)
$cy = $r.T + [int]($h * 0.52)
for ($round = 0; $round -lt $Rounds; $round++) {
  # orbit sweep: press left, drag across viewport, release
  [WinInput]::SetCursorPos($cx, $cy) | Out-Null
  Start-Sleep -Milliseconds 120
  [WinInput]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)  # LEFTDOWN
  Start-Sleep -Milliseconds 60
  for ($i = 1; $i -le 12; $i++) {
    [WinInput]::SetCursorPos($cx + $i * 18, $cy + [int]([Math]::Sin($i / 2.0) * 40)) | Out-Null
    Start-Sleep -Milliseconds 25
  }
  [WinInput]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)  # LEFTUP
  Start-Sleep -Milliseconds 150
  # pan sweep: middle button drag back
  [WinInput]::mouse_event(0x0020, 0, 0, 0, [UIntPtr]::Zero)  # MIDDLEDOWN
  Start-Sleep -Milliseconds 60
  for ($i = 1; $i -le 10; $i++) {
    [WinInput]::SetCursorPos($cx + 216 - $i * 20, $cy - $i * 8) | Out-Null
    Start-Sleep -Milliseconds 25
  }
  [WinInput]::mouse_event(0x0040, 0, 0, 0, [UIntPtr]::Zero)  # MIDDLEUP
  Start-Sleep -Milliseconds 200
}
if ($p.HasExited) { Write-Output "APP-EXITED"; exit 1 }
Write-Output "app survived $($Rounds) rounds"
