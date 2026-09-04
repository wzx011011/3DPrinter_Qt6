param([string]$Path, [int]$X0, [int]$Y0, [int]$W, [int]$H)
Add-Type -AssemblyName System.Drawing
$bmp = [System.Drawing.Bitmap]::FromFile((Resolve-Path $Path))
$colors = @{}
for ($y = $Y0; $y -lt $Y0 + $H; $y++) {
  for ($x = $X0; $x -lt $X0 + $W; $x++) {
    $c = $bmp.GetPixel($x, $y)
    $key = "{0:X2}{1:X2}{2:X2}" -f $c.R, $c.G, $c.B
    if (-not $colors.ContainsKey($key)) { $colors[$key] = 0 }
    $colors[$key]++
  }
}
$bmp.Dispose()
$top = $colors.GetEnumerator() | Sort-Object Value -Descending | Select-Object -First 12
foreach ($t in $top) { Write-Output ("{0} {1}" -f $t.Key, $t.Value) }
Write-Output ("distinct={0}" -f $colors.Count)
