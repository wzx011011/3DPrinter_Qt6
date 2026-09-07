# UI region comparison: OWzx vs OrcaSlicer prepare pages
param(
    [string]$OwzxPath = "D:\ai\3DPrinter_Qt6-perf\build\ui_compare\owzx_prepare_page.png",
    [string]$OrcaPath = "D:\ai\3DPrinter_Qt6-perf\build\ui_compare\orca_2m_prepare.png",
    [string]$OutDir = "D:\ai\3DPrinter_Qt6-perf\build\ui_compare"
)
Add-Type -AssemblyName System.Drawing

function Get-RegionStats {
    param([System.Drawing.Bitmap]$img, [int]$x, [int]$y, [int]$w, [int]$h, [string]$label)
    $rSum = 0; $gSum = 0; $bSum = 0; $n = 0
    $colors = @{}
    for ($py = $y; $py -lt ($y + $h); $py += 4) {
        for ($px = $x; $px -lt ($x + $w); $px += 4) {
            if ($px -ge $img.Width -or $py -ge $img.Height) { continue }
            $c = $img.GetPixel($px, $py)
            $rSum += $c.R; $gSum += $c.G; $bSum += $c.B; $n++
            $key = "{0:X2}{1:X2}{2:X2}" -f $c.R, $c.G, $c.B
            if ($colors.ContainsKey($key)) { $colors[$key]++ } else { $colors[$key] = 1 }
        }
    }
    if ($n -eq 0) { return }
    $avgR = [math]::Round($rSum / $n); $avgG = [math]::Round($gSum / $n); $avgB = [math]::Round($bSum / $n)
    $top = $colors.GetEnumerator() | Sort-Object Value -Descending | Select-Object -First 3
    $topStr = ($top | ForEach-Object { "#$($_.Key)($($_.Value))" }) -join " "
    Write-Host ("{0,-18} avgRGB=({1},{2},{3}) dominant: {4}" -f $label, $avgR, $avgG, $avgB, $topStr)
}

$owzx = [System.Drawing.Bitmap]::FromFile($OwzxPath)
$orca = [System.Drawing.Bitmap]::FromFile($OrcaPath)
Write-Host ("OWzx: {0}x{1}  Orca: {2}x{3}" -f $owzx.Width, $owzx.Height, $orca.Width, $orca.Height)
Write-Host ""

# Region comparison using proportional coordinates (normalize to fraction of width/height)
$regions = @(
    @{ Label = "Top Navigation";   XF = 0.00; YF = 0.00; WF = 1.00; HF = 0.08 },
    @{ Label = "Left Sidebar";     XF = 0.00; YF = 0.08; WF = 0.22; HF = 0.85 },
    @{ Label = "3D Viewport";      XF = 0.23; YF = 0.08; WF = 0.77; HF = 0.85 },
    @{ Label = "Status Bar";       XF = 0.00; YF = 0.93; WF = 1.00; HF = 0.07 }
)

foreach ($region in $regions) {
    Write-Host ("--- " + $region.Label + " ---")
    $ox = [int]($owzx.Width * $region.XF); $oy = [int]($owzx.Height * $region.YF)
    $ow = [int]($owzx.Width * $region.WF); $oh = [int]($owzx.Height * $region.HF)
    Write-Host "OWzx:"
    Get-RegionStats -img $owzx -x $ox -y $oy -w $ow -h $oh -label $region.Label
    $rx = [int]($orca.Width * $region.XF); $ry = [int]($orca.Height * $region.YF)
    $rw = [int]($orca.Width * $region.WF); $rh = [int]($orca.Height * $region.HF)
    Write-Host "Orca:"
    Get-RegionStats -img $orca -x $rx -y $ry -w $rw -h $rh -label $region.Label
    Write-Host ""
}

$owzx.Dispose()
$orca.Dispose()
