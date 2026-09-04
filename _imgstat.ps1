param([string]$Path, [int]$GridW = 16, [int]$GridH = 9, [int]$X0 = -1, [int]$Y0 = -1, [int]$X1 = -1, [int]$Y1 = -1)
Add-Type -AssemblyName System.Drawing
$bmp = [System.Drawing.Bitmap]::FromFile((Resolve-Path $Path))
if ($X0 -lt 0) { $X0 = 0; $Y0 = 0; $X1 = $bmp.Width - 1; $Y1 = $bmp.Height - 1 }
$w = $X1 - $X0 + 1; $h = $Y1 - $Y0 + 1
Write-Output "IMG $($bmp.Width)x$($bmp.Height) region ${w}x${h} at ($X0,$Y0)"
# coarse color grid
for ($gy = 0; $gy -lt $GridH; $gy++) {
  $row = @()
  for ($gx = 0; $gx -lt $GridW; $gx++) {
    $sx = $X0 + [int](($gx + 0.5) * $w / $GridW); $sy = $Y0 + [int](($gy + 0.5) * $h / $GridH)
    $c = $bmp.GetPixel([Math]::Min($sx, $bmp.Width-1), [Math]::Min($sy, $bmp.Height-1))
    $row += ("{0:X2}{1:X2}{2:X2}" -f $c.R, $c.G, $c.B)
  }
  Write-Output ($row -join " ")
}
# region stats: distinct colors sampled, luminance stddev
$colors = @{}; $sum = 0.0; $sum2 = 0.0; $n = 0
$rnd = New-Object System.Random(42)
for ($i = 0; $i -lt 5000; $i++) {
  $sx = $rnd.Next($X0, $X1 + 1); $sy = $rnd.Next($Y0, $Y1 + 1)
  $c = $bmp.GetPixel($sx, $sy)
  $lum = 0.299 * $c.R + 0.587 * $c.G + 0.114 * $c.B
  $sum += $lum; $sum2 += $lum * $lum; $n++
  $key = "{0:X2}{1:X2}{2:X2}" -f $c.R, $c.G, $c.B
  if (-not $colors.ContainsKey($key)) { $colors[$key] = 0 }
  $colors[$key]++
}
$mean = $sum / $n; $var = $sum2 / $n - $mean * $mean
Write-Output ("STATS distinct={0} lumMean={1:N1} lumStd={2:N1}" -f $colors.Count, $mean, [Math]::Sqrt([Math]::Max(0, $var)))
$top = $colors.GetEnumerator() | Sort-Object Value -Descending | Select-Object -First 8
foreach ($t in $top) { Write-Output ("TOP {0} {1}" -f $t.Key, $t.Value) }
$bmp.Dispose()
