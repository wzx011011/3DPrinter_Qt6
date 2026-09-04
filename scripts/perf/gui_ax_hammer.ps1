# Invoke OWzx Slicer toolbar view buttons via UI Automation (no CUA broker
# needed). Drives camera changes -> viewport repaints -> perf buckets.
param([int]$Pid_ = 0, [int[]]$ButtonIdx = @(18, 19, 20, 21), [int]$Rounds = 3)
Add-Type -AssemblyName UIAutomationClient
$p = Get-Process -Id $Pid_ -ErrorAction Stop
$root = [System.Windows.Automation.AutomationElement]::FromHandle($p.MainWindowHandle)
$cond = New-Object System.Windows.Automation.PropertyCondition(
    [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
    [System.Windows.Automation.ControlType]::Button)
$buttons = $root.FindAll([System.Windows.Automation.TreeScope]::Descendants, $cond)
Write-Host ("buttons=" + $buttons.Count)
$invoked = 0
for ($r = 0; $r -lt $Rounds; $r++) {
  foreach ($i in $ButtonIdx) {
    if ($i -ge $buttons.Count) { continue }
    $b = $buttons.Item($i)
    $ip = $null
    if ($b.TryGetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern, [ref]$ip)) {
      $ip.Invoke()
      $invoked++
      Start-Sleep -Milliseconds 900
    }
  }
}
Write-Host ("invoked=" + $invoked)
