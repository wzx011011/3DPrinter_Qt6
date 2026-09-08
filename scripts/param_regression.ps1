param(
  [string]$Manifest = "tests/param_regression/manifest.json",
  [string]$OutDir = "tests/output/param_regression",
  [string]$OwzxCli = "",
  [string]$UpstreamCli = "",
  [ValidateSet("smoke", "full", "compare-only")]
  [string]$Mode = "smoke",
  [switch]$RunSlicing,
  [switch]$SkipUpstream,
  [switch]$RequireUpstream
)

$ErrorActionPreference = "Stop"

function Resolve-RepoPath([string]$Path) {
  if ([System.IO.Path]::IsPathRooted($Path)) {
    return $Path
  }
  return (Join-Path $repoRoot $Path)
}

function ConvertTo-JsonDepth([object]$Value) {
  return ($Value | ConvertTo-Json -Depth 20)
}

function Write-JsonFile([string]$Path, [object]$Value) {
  $parent = Split-Path -Parent $Path
  if ($parent) {
    New-Item -ItemType Directory -Force -Path $parent | Out-Null
  }
  ConvertTo-JsonDepth $Value | Set-Content -Path $Path -Encoding UTF8
}

function Find-Gcode([string]$Dir) {
  $files = Get-ChildItem -Path $Dir -Filter "*.gcode" -File -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTimeUtc -Descending
  if ($files.Count -eq 0) {
    return $null
  }
  return $files[0].FullName
}

function Quote-ProcessArgument([string]$Arg) {
  if ($null -eq $Arg) {
    return '""'
  }
  if ($Arg -notmatch '[\s"]') {
    return $Arg
  }
  return '"' + ($Arg -replace '(\\*)"', '$1$1\"' -replace '(\\+)$', '$1$1') + '"'
}

function New-UpstreamSettings([object]$Metadata, [object]$Overrides) {
  $settings = [ordered]@{}
  foreach ($prop in $Metadata.PSObject.Properties) {
    $settings[$prop.Name] = $prop.Value
  }
  foreach ($prop in $Overrides.PSObject.Properties) {
    $value = $prop.Value
    if ($value -is [bool]) {
      $settings[$prop.Name] = ($(if ($value) { "1" } else { "0" }))
    } elseif ($value -is [array]) {
      $settings[$prop.Name] = @($value | ForEach-Object { "$_" })
    } else {
      $settings[$prop.Name] = "$value"
    }
  }
  return $settings
}

function Invoke-LoggedProcess([string]$Exe, [string[]]$ArgumentList, [string]$WorkingDirectory, [string]$StdoutPath, [string]$StderrPath) {
  $psi = New-Object System.Diagnostics.ProcessStartInfo
  $psi.FileName = $Exe
  $psi.WorkingDirectory = $WorkingDirectory
  $psi.UseShellExecute = $false
  $psi.RedirectStandardOutput = $true
  $psi.RedirectStandardError = $true
  $psi.Arguments = (($ArgumentList | ForEach-Object { Quote-ProcessArgument $_ }) -join " ")

  $process = New-Object System.Diagnostics.Process
  $process.StartInfo = $psi
  [void]$process.Start()
  $stdout = $process.StandardOutput.ReadToEnd()
  $stderr = $process.StandardError.ReadToEnd()
  $process.WaitForExit()

  Set-Content -Path $StdoutPath -Value $stdout -Encoding UTF8
  Set-Content -Path $StderrPath -Value $stderr -Encoding UTF8

  return [ordered]@{
    exit_code = $process.ExitCode
    stdout = $StdoutPath
    stderr = $StderrPath
    command = @($Exe) + $ArgumentList
  }
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$manifestPath = Resolve-RepoPath $Manifest
if (-not (Test-Path $manifestPath)) {
  throw "Manifest not found: $manifestPath"
}

$manifestObj = Get-Content -Raw $manifestPath | ConvertFrom-Json
$outRoot = Resolve-RepoPath $OutDir
New-Item -ItemType Directory -Force -Path $outRoot | Out-Null

if ([string]::IsNullOrWhiteSpace($OwzxCli)) {
  $OwzxCli = Resolve-RepoPath $manifestObj.owzx.cli_default
}
if ([string]::IsNullOrWhiteSpace($UpstreamCli) -and $env:ORCASLICER_CLI) {
  $UpstreamCli = $env:ORCASLICER_CLI
}

$canRunOwzx = Test-Path $OwzxCli
$canRunUpstream = (-not $SkipUpstream) -and -not [string]::IsNullOrWhiteSpace($UpstreamCli) -and (Test-Path $UpstreamCli)

if ($RequireUpstream -and -not $canRunUpstream) {
  throw "Upstream OrcaSlicer CLI not found. Pass -UpstreamCli or set ORCASLICER_CLI."
}

$models = @($manifestObj.baseline_models)
$cases = @($manifestObj.parameter_cases)
if ($Mode -eq "smoke") {
  $modelIds = @($manifestObj.smoke.baseline_ids)
  $caseIds = @($manifestObj.smoke.case_ids)
  $models = @($models | Where-Object { $modelIds -contains $_.id })
  $cases = @($cases | Where-Object { $caseIds -contains $_.id })
}

$summary = [ordered]@{
  schema_version = 1
  mode = $Mode
  objective = "parameter_chain_regression"
  run_slicing = [bool]$RunSlicing
  manifest = $manifestPath
  output_dir = $outRoot
  owzx_cli = $OwzxCli
  upstream_cli = $UpstreamCli
  can_run_owzx = $canRunOwzx
  can_run_upstream = $canRunUpstream
  started_at = (Get-Date).ToString("o")
  results = @()
}

if ($RunSlicing -and $Mode -ne "compare-only" -and -not $canRunOwzx) {
  $summary.results += [ordered]@{
    status = "skipped"
    reason = "OWzx CLI not found. Build with scripts/auto_verify_with_vcvars.ps1 first."
    missing = $OwzxCli
  }
  Write-JsonFile (Join-Path $outRoot "summary.json") $summary
  Write-Host "SKIP: OWzx CLI not found: $OwzxCli"
  exit 0
}

foreach ($model in $models) {
  $modelPath = Resolve-RepoPath $model.path
  foreach ($case in $cases) {
    $caseId = "$($model.id)__$($case.id)"
    $caseDir = Join-Path $outRoot $caseId
    $settingsDir = Join-Path $caseDir "settings"
    $owzxDir = Join-Path $caseDir "owzx"
    $upstreamDir = Join-Path $caseDir "upstream"
    $reportsDir = Join-Path $caseDir "reports"
    New-Item -ItemType Directory -Force -Path $settingsDir, $owzxDir, $upstreamDir, $reportsDir | Out-Null

    $owzxSettings = Join-Path $settingsDir "owzx-overrides.json"
    $upstreamSettings = Join-Path $settingsDir "upstream-process.json"
    Write-JsonFile $owzxSettings $case.overrides
    Write-JsonFile $upstreamSettings (New-UpstreamSettings $manifestObj.upstream.settings_metadata $case.overrides)

    $result = [ordered]@{
      id = $caseId
      model_id = $model.id
      model_path = $modelPath
      case_id = $case.id
      description = $case.description
      status = "pending"
      owzx_settings = $owzxSettings
      upstream_settings = $upstreamSettings
      owzx_gcode = $null
      upstream_gcode = $null
      compare_report = $null
      owzx = $null
      upstream = $null
    }

    if (-not (Test-Path $modelPath)) {
      $result.status = "skipped"
      $result.reason = "Baseline model not found"
      $summary.results += $result
      continue
    }

    if ($Mode -ne "compare-only" -and -not $RunSlicing) {
      $result.status = "parameter_artifacts_generated"
      $result.reason = "Default mode validates parameter-chain artifact generation only. Pass -RunSlicing for representative G-code regression."
      $summary.results += $result
      continue
    }

    if ($Mode -ne "compare-only") {
      Write-Host "[$caseId] slicing OWzx..."
      $owzxArgs = @("--load", $modelPath, "--load-settings", $owzxSettings, "--slice", "--output-dir", $owzxDir, "--quiet")
      $result.owzx = Invoke-LoggedProcess -Exe $OwzxCli -ArgumentList $owzxArgs -WorkingDirectory $repoRoot -StdoutPath (Join-Path $caseDir "owzx.stdout.log") -StderrPath (Join-Path $caseDir "owzx.stderr.log")
      if ($result.owzx.exit_code -ne 0) {
        $result.status = "owzx_failed"
        $summary.results += $result
        continue
      }
      $result.owzx_gcode = Find-Gcode $owzxDir
      if (-not $result.owzx_gcode) {
        $result.status = "owzx_no_gcode"
        $summary.results += $result
        continue
      }

      if ($canRunUpstream) {
        Write-Host "[$caseId] slicing upstream..."
        $upstreamArgs = @(
          $modelPath,
          $manifestObj.upstream.settings_argument,
          $upstreamSettings
        ) + @($manifestObj.upstream.slice_arguments) + @(
          $manifestObj.upstream.output_argument,
          $upstreamDir
        )
        $result.upstream = Invoke-LoggedProcess -Exe $UpstreamCli -ArgumentList $upstreamArgs -WorkingDirectory $repoRoot -StdoutPath (Join-Path $caseDir "upstream.stdout.log") -StderrPath (Join-Path $caseDir "upstream.stderr.log")
        if ($result.upstream.exit_code -ne 0) {
          $result.status = "upstream_failed"
          $summary.results += $result
          continue
        }
        $result.upstream_gcode = Find-Gcode $upstreamDir
        if (-not $result.upstream_gcode) {
          $result.status = "upstream_no_gcode"
          $summary.results += $result
          continue
        }
      } else {
        $result.status = "skipped_upstream"
        $result.reason = "Upstream OrcaSlicer CLI not found. Pass -UpstreamCli or set ORCASLICER_CLI."
        $summary.results += $result
        continue
      }
    } else {
      $result.owzx_gcode = Find-Gcode $owzxDir
      $result.upstream_gcode = Find-Gcode $upstreamDir
      if (-not $result.owzx_gcode -or -not $result.upstream_gcode) {
        $result.status = "compare_skipped"
        $result.reason = "Existing OWzx/upstream G-code pair not found"
        $summary.results += $result
        continue
      }
    }

    $compareReport = Join-Path $reportsDir "gcode-compare.json"
    $compareArgs = @(
      "tools/gcode_compare.py",
      "--expected", $result.upstream_gcode,
      "--actual", $result.owzx_gcode,
      "--report", $compareReport
    )
    $compare = Invoke-LoggedProcess -Exe "python" -ArgumentList $compareArgs -WorkingDirectory $repoRoot -StdoutPath (Join-Path $caseDir "compare.stdout.log") -StderrPath (Join-Path $caseDir "compare.stderr.log")
    $result.compare_report = $compareReport
    $result.compare = $compare

    if ($compare.exit_code -eq 0) {
      $result.status = "passed"
    } else {
      $result.status = "normalized_diff"
    }
    $summary.results += $result
  }
}

$summary.finished_at = (Get-Date).ToString("o")
$summaryPath = Join-Path $outRoot "summary.json"
Write-JsonFile $summaryPath $summary

$failed = @($summary.results | Where-Object { $_.status -in @("owzx_failed", "owzx_no_gcode", "upstream_failed", "upstream_no_gcode", "normalized_diff") })
$skipped = @($summary.results | Where-Object { $_.status -like "skipped*" -or $_.status -eq "compare_skipped" })
$passed = @($summary.results | Where-Object { $_.status -eq "passed" })

Write-Host "Parameter regression summary: $summaryPath"
Write-Host "  passed:  $($passed.Count)"
Write-Host "  failed:  $($failed.Count)"
Write-Host "  skipped: $($skipped.Count)"

if ($failed.Count -gt 0) {
  exit 1
}
exit 0
