param([int]$X = 700, [int]$Y = 400, [int]$W = 600, [int]$H = 280, [string]$Path = "E:\ai\3DPrinter_Qt6\build\hash_probe.png")
Add-Type -AssemblyName System.Drawing
$b = New-Object System.Drawing.Bitmap($W, $H)
$g = [System.Drawing.Graphics]::FromImage($b)
$g.CopyFromScreen($X, $Y, 0, 0, (New-Object System.Drawing.Size($W, $H)))
$g.Dispose()
$b.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
$b.Dispose()
Write-Host ((Get-FileHash $Path -Algorithm MD5).Hash.Substring(0, 8))
