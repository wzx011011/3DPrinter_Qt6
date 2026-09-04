param([int]$Pid_ = 0)
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class SW {
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
}
"@
$p = Get-Process -Id $Pid_ -ErrorAction Stop
$h = $p.MainWindowHandle
[SW]::ShowWindow($h, 6) | Out-Null   # SW_MINIMIZE
Start-Sleep -Milliseconds 700
[SW]::ShowWindow($h, 9) | Out-Null   # SW_RESTORE
Start-Sleep -Milliseconds 500
[SW]::SetForegroundWindow($h) | Out-Null
Write-Host "restored"
