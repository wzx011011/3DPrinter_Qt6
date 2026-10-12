# Slice regression gate, three layers:
#   1. correctness (normalized golden gcode via owzx-cli + gcode_compare.py)
#   2. efficiency  (PerfBench slice stage, min-of-N vs baseline thresholds)
#   3. step profile (P4, docs/开发计划_2026-10-10.md §6d: 4 representative
#      models x per-step wall-clock profile from the SliceService step
#      timeline, vs baseline.step_profiles; a step regressing >+30% with a
#      >=20ms baseline in BOTH measurement rounds is a regression)
#
# Invoked automatically by scripts/auto_verify_with_vcvars.ps1 when the
# change surface touches the slicing pipeline; also runnable standalone:
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File scripts/perf/run_slice_gate.ps1
#   powershell ... -File scripts/perf/run_slice_gate.ps1 -UpdateBaseline   # re-gold after an INTENDED change
#
# Env overrides:
#   OWZX_SLICE_GATE_REPEATS   PerfBench timing repeats per round (default 3)
#
# Exit codes: 0 pass | 1 correctness regression | 2 timing/step-profile regression | 3 infra error
#
# Baseline lives at tests/data/slice_gate_baseline.json (committed). Correctness
# hard-compares normalized_sha256 / line count / semantic counts produced by
# tools/gcode_compare.py --summarize; gcode size is informational (raw bytes
# still carry path-dependent comment lengths). Timing hard-fails only when the
# min-of-N breaches the baseline threshold in BOTH measurement rounds (the
# docs/perf-baseline.md noise rules: slice_ms is a volatile wall-clock metric).
# The step-profile layer hard-fails the same two-round way; small steps
# (baseline < 20ms) are exempt from the percentage rule (noise floor), and a
# step KEY appearing/disappearing (engine set_status wording changed) fails
# immediately and is re-golded only via -UpdateBaseline after human review.

param([switch]$UpdateBaseline)

$ErrorActionPreference = 'Stop'

$repoRoot  = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$buildDir  = Join-Path $repoRoot 'build'
$cliExe    = Join-Path $buildDir 'owzx-cli.exe'
$perfExe   = Join-Path $buildDir 'PerfBench.exe'
$modelsDir = Join-Path $buildDir 'perf_models'
$outDir    = Join-Path $buildDir 'perf_out\slice_gate'
$baselinePath = Join-Path $repoRoot 'tests\data\slice_gate_baseline.json'
$reportPath   = Join-Path $buildDir 'perf_out\slice_gate_report.json'
$comparePy    = Join-Path $repoRoot 'tools\gcode_compare.py'

$repeats = 3
if ($env:OWZX_SLICE_GATE_REPEATS) { $repeats = [int]$env:OWZX_SLICE_GATE_REPEATS }
if ($repeats -lt 1) { $repeats = 1 }

# Pin layer_height to a value compatible with the preset-merge defaults
# (line_width 0.2 must stay STRICTLY greater than layer_height -- Print.cpp
# validate_extrusion_width errors on width <= layer height; the preset default
# is layer_height 0.08, so 0.15 pins the knob without tripping that check).
# Line widths stay unpinned: raw --line_width 0 ("auto") overrides land after
# normalize_fdm on the CLI path and fail validation; preset widths are already
# deterministic.
$script:pinArgs = @('--layer_height', '0.15')
$script:pins = [ordered]@{ layer_height = 0.15 }

# Bootstrap model manifest (used when establishing the baseline; afterwards the
# baseline itself is the manifest).
$script:defaultModels = @(
    @{ key = 'sphere_400k'; stl = 'build/perf_models/bench_sphere_400k.stl' },
    @{ key = 'block_20xy';  stl = 'tests/test_models/Block20XY.stl' }
)

# P4 step-profile manifest (§6d): 4 representative models, each sliced once
# per profile round through PerfBench's in-process SliceService path. `tier`
# is the PERF_BENCH_TIERS selector; the timing layer above stays pinned to
# the 400k tier only. The tower tier enables enable_support inside PerfBench
# (support generation IS its workload).
$script:profileModels = @(
    @{ key = 'sphere_400k';    tier = '400k';  stl = 'build/perf_models/bench_sphere_400k.stl' },
    @{ key = 'overhang_tower'; tier = 'tower'; stl = 'build/perf_models/bench_overhang_tower.stl' },
    @{ key = 'grid_20x100k';   tier = 'grid';  stl = 'build/perf_models/bench_grid_20x100k.stl' },
    @{ key = 'block_20xy';     tier = 'block'; stl = 'tests/test_models/Block20XY.stl' }
)

# P4 step-profile thresholds: a step is a regression only when it regresses
# by more than max_regression_pct AND the baseline step is at least
# min_step_ms (small steps are exempt -- noise floor).
$script:stepThresholds = [ordered]@{ max_regression_pct = 30; min_step_ms = 20 }

function Write-Gate([string]$Message, [string]$Color = 'Gray') {
    Write-Host ("[slice-gate] " + $Message) -ForegroundColor $Color
}

function Fail-Infra([string]$Message) {
    Write-Gate $Message 'Red'
    exit 3
}

function Get-HeadCommit {
    try { return (git -C $repoRoot rev-parse --short HEAD 2>$null).Trim() }
    catch { return 'unknown' }
}

# ── Preflight ─────────────────────────────────────────────────────────────
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

if (-not (Test-Path $cliExe))  { Fail-Infra "owzx-cli.exe not found at $cliExe -- run scripts/auto_verify_with_vcvars.ps1 first" }
if (-not (Test-Path $perfExe)) { Fail-Infra "PerfBench.exe not found at $perfExe -- run scripts/auto_verify_with_vcvars.ps1 first" }

$benchSphere = Join-Path $modelsDir 'bench_sphere_400k.stl'
$benchTower  = Join-Path $modelsDir 'bench_overhang_tower.stl'
$benchGrid   = Join-Path $modelsDir 'bench_grid_20x100k.stl'
if ((-not (Test-Path $benchSphere)) -or (-not (Test-Path $benchTower)) -or (-not (Test-Path $benchGrid))) {
    Write-Gate 'generating benchmark models...'
    python (Join-Path $repoRoot 'scripts\perf\gen_benchmark_models.py') $modelsDir
    if ($LASTEXITCODE -ne 0) { Fail-Infra 'gen_benchmark_models.py failed' }
}

# PerfBench links Qt6Test (not part of the deployed runtime); resolve the Qt
# bin dir from the build's CMake cache -- same trick as run_stage_bench.ps1.
$qtBin = (Get-Content (Join-Path $buildDir 'CMakeCache.txt') |
    Select-String -Pattern '^Qt6_DIR:[^=]*=(.*)$').Matches[0].Groups[1].Value |
    Split-Path -Parent | Split-Path -Parent | Split-Path -Parent
$env:PATH = "$(Join-Path $qtBin 'bin');$env:PATH"

$baseline = $null
$baselineExists = Test-Path $baselinePath
if ($baselineExists) {
    $baseline = Get-Content $baselinePath -Raw | ConvertFrom-Json
}
$establishing = $UpdateBaseline -or (-not $baselineExists)
if ($establishing) {
    Write-Gate ("baseline " + $(if ($baselineExists) {'refresh requested (-UpdateBaseline)'} else {'missing -- establishing from this run'}) + " (yellow: re-gold blesses the CURRENT build)") 'Yellow'
}

# ── Stage 1: correctness (headless CLI slice + normalized golden compare) ──
function Invoke-CliSlice([string]$StlRel, [string]$SubDir) {
    $stlAbs = Join-Path $repoRoot ($StlRel -replace '/', '\')
    if (-not (Test-Path $stlAbs)) { Fail-Infra "model not found: $stlAbs" }
    $dest = Join-Path $outDir $SubDir
    New-Item -ItemType Directory -Force -Path $dest | Out-Null
    $args_ = @('--load', $stlAbs) + $script:pinArgs + @('--slice', '--output-dir', $dest)
    Write-Gate ("slicing " + $StlRel)
    $log = Join-Path $dest 'cli_slice.log'
    # Relax EAP around the redirected native call: PS 5.1 wraps redirected
    # stderr lines in ErrorRecords, which under 'Stop' would throw on the
    # CLI's harmless progress chatter instead of recording its exit code.
    $prevEap = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & $cliExe @args_ *> $log
    $ErrorActionPreference = $prevEap
    if ($LASTEXITCODE -ne 0) {
        Get-Content $log -Tail 20 | ForEach-Object { Write-Gate ("  cli: " + $_) 'Red' }
        Fail-Infra "owzx-cli slice failed for $StlRel (exit $LASTEXITCODE, log: $log)"
    }
    # single-plate export name = <model base name>.gcode (SliceService::defaultExportGCodeFileName)
    $expected = Join-Path $dest ([System.IO.Path]::GetFileNameWithoutExtension($stlAbs) + '.gcode')
    if (-not (Test-Path $expected)) { Fail-Infra "expected gcode not produced: $expected" }
    return $expected
}

function Get-GcodeSummary([string]$GcodePath, [string]$SummaryPath) {
    python $comparePy --summarize $GcodePath --report $SummaryPath
    if ($LASTEXITCODE -ne 0) { Fail-Infra "gcode_compare.py --summarize failed for $GcodePath" }
    return Get-Content $SummaryPath -Raw | ConvertFrom-Json
}

$correctnessResults = @()
$correctnessFailed = $false

$modelList = @()
if ($establishing) {
    $modelList = $script:defaultModels
} else {
    foreach ($p in $baseline.models.PSObject.Properties) {
        $modelList += @{ key = $p.Name; stl = $p.Value.stl }
    }
}

$newModelsJson = [ordered]@{}
foreach ($m in $modelList) {
    $gcode = Invoke-CliSlice $m.stl $m.key
    $summary = Get-GcodeSummary $gcode (Join-Path $outDir ($m.key + '.summary.json'))

    $newModelsJson[$m.key] = [ordered]@{
        stl                   = $m.stl
        normalized_sha256     = $summary.normalized_sha256
        normalized_line_count = $summary.normalized_line_count
        layer_changes         = $summary.layer_changes
        commands              = $summary.commands
        total_e               = $summary.total_e
        size_bytes            = $summary.size_bytes
    }

    if ($establishing) { continue }

    $gold = $baseline.models.($m.key)
    $verdict = 'PASS'
    $diffs = @()
    if ($summary.normalized_sha256 -ne $gold.normalized_sha256) { $diffs += 'normalized_sha256' }
    if ($summary.normalized_line_count -ne $gold.normalized_line_count) { $diffs += 'normalized_line_count' }
    if ($summary.layer_changes -ne $gold.layer_changes) { $diffs += 'layer_changes' }
    if ($summary.total_e -ne $gold.total_e) { $diffs += ('total_e (baseline {0} vs now {1})' -f $gold.total_e, $summary.total_e) }
    $cmdNames = @($summary.commands.PSObject.Properties.Name)
    $goldCmdNames = @($gold.commands.PSObject.Properties.Name)
    if ($cmdNames.Count -ne $goldCmdNames.Count) {
        $diffs += ('command set (baseline {0} kinds vs now {1})' -f $goldCmdNames.Count, $cmdNames.Count)
    } else {
        foreach ($c in $goldCmdNames) {
            if (-not ($summary.commands.$c) -or [long]$summary.commands.$c -ne [long]$gold.commands.$c) {
                $diffs += ("command {0} count (baseline {1} vs now {2})" -f $c, $gold.commands.$c, $summary.commands.$c)
            }
        }
    }
    if ($diffs.Count -gt 0) {
        $verdict = 'FAIL'
        $correctnessFailed = $true
        Write-Gate ("CORRECTNESS FAIL [{0}]: {1}" -f $m.key, ($diffs -join '; ')) 'Red'
        Write-Gate ("  summary: {0}  baseline: {1}" -f (Join-Path $outDir ($m.key + '.summary.json')), $baselinePath) 'Red'
    } else {
        $sizeDelta = 0
        if ($gold.size_bytes -gt 0) { $sizeDelta = [math]::Round(($summary.size_bytes - $gold.size_bytes) / $gold.size_bytes * 100, 2) }
        Write-Gate ("correctness PASS [{0}] (gcode {1} bytes, size delta {2}% vs golden)" -f $m.key, $summary.size_bytes, $sizeDelta)
    }
    $correctnessResults += @{ key = $m.key; verdict = $verdict; diffs = $diffs; size_bytes = $summary.size_bytes }
}

if ($correctnessFailed) {
    Write-Gate 'GATE FAILED: slice output regressed vs golden baseline (exit 1)' 'Red'
    exit 1
}

# ── Stage 2: efficiency (PerfBench slice tier, min-of-N, two-round rule) ───
function Invoke-TimingRound([int]$Round) {
    $mins = @{ slice_ms = [double]::MaxValue; preview_parse_ms = [double]::MaxValue; gcode_bytes = 0 }
    $runs = @()
    for ($i = 1; $i -le $repeats; $i++) {
        $env:OWZX_PERF_LOG = '1'
        $env:PERF_BENCH_MODELS_DIR = $modelsDir
        $env:PERF_BENCH_TIERS = '400k'
        $env:PERF_BENCH_SLICE = '1'
        $roundReport = Join-Path $outDir ("stage_report_r{0}_n{1}.json" -f $Round, $i)
        $env:PERF_BENCH_REPORT = $roundReport
        $prevEap = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        & $perfExe *> $null
        $ErrorActionPreference = $prevEap
        if ($LASTEXITCODE -ne 0) { Fail-Infra "PerfBench exited with code $LASTEXITCODE (round $Round run $i)" }
        $r = Get-Content $roundReport -Raw | ConvertFrom-Json
        $sm = [double]$r.models.'400k'.slice_ms
        $pp = [double]$r.models.'400k'.preview_parse_ms
        $gb = 0
        if ($r.models.'400k'.PSObject.Properties.Name -contains 'slice_gcode_bytes') { $gb = [long]$r.models.'400k'.slice_gcode_bytes }
        $runs += @{ n = $i; slice_ms = $sm; preview_parse_ms = $pp; slice_gcode_bytes = $gb }
        if ($sm -lt $mins.slice_ms) { $mins.slice_ms = $sm }
        if ($pp -lt $mins.preview_parse_ms) { $mins.preview_parse_ms = $pp }
        if ($gb -gt $mins.gcode_bytes) { $mins.gcode_bytes = $gb }
    }
    return @{ mins = $mins; runs = $runs }
}

$round1 = Invoke-TimingRound 1
$curSlice = $round1.mins.slice_ms
$curPreview = $round1.mins.preview_parse_ms

$timingFailed = $false
$timingRows = @()

if (-not $establishing) {
    $baseSlice = [double]$baseline.timing.slice_ms_min
    $basePreview = [double]$baseline.timing.preview_parse_ms_min
    $slicePct = [math]::Round(($curSlice - $baseSlice) / $baseSlice * 100, 1)
    $previewPct = [math]::Round(($curPreview - $basePreview) / $basePreview * 100, 1)
    $sliceLimit = [double]$baseline.timing.max_regression_pct.slice_ms
    $previewLimit = [double]$baseline.timing.max_regression_pct.preview_parse_ms

    # Volatile-metric rule (docs/perf-baseline.md): one breaching round gets an
    # automatic re-measure; only a breach in BOTH rounds is a regression.
    if ($slicePct -gt $sliceLimit -or $previewPct -gt $previewLimit) {
        Write-Gate ("round-1 min breached threshold (slice {0}% / preview {1}% vs limits {2}%/{3}%) -- re-measuring once" -f $slicePct, $previewPct, $sliceLimit, $previewLimit) 'Yellow'
        $round2 = Invoke-TimingRound 2
        if ($round2.mins.slice_ms -lt $curSlice) { $curSlice = $round2.mins.slice_ms }
        if ($round2.mins.preview_parse_ms -lt $curPreview) { $curPreview = $round2.mins.preview_parse_ms }
        $slicePct = [math]::Round(($curSlice - $baseSlice) / $baseSlice * 100, 1)
        $previewPct = [math]::Round(($curPreview - $basePreview) / $basePreview * 100, 1)
    }

    foreach ($row in @(
        @{ metric = 'slice_ms';          base = $baseSlice;  cur = $curSlice;  limit = $sliceLimit },
        @{ metric = 'preview_parse_ms';  base = $basePreview; cur = $curPreview; limit = $previewLimit })) {
        $pct = [math]::Round(($row.cur - $row.base) / $row.base * 100, 1)
        if ($pct -gt $row.limit) {
            $timingFailed = $true
            $row['verdict'] = 'REGRESSION'
            Write-Gate ("EFFICIENCY FAIL: {0} {1}ms vs baseline {2}ms (+{3}% > {4}% limit)" -f $row.metric, $row.cur, $row.base, $pct, $row.limit) 'Red'
        } elseif ($pct -lt -1) {
            $row['verdict'] = ('IMPROVED ' + $pct + '%')
            Write-Gate ("{0}: {1}ms vs baseline {2}ms ({3}% -- improved)" -f $row.metric, $row.cur, $row.base, $pct) 'Green'
        } else {
            $row['verdict'] = 'ok'
            Write-Gate ("{0}: {1}ms vs baseline {2}ms ({3}% -- within noise)" -f $row.metric, $row.cur, $row.base, $pct)
        }
        $timingRows += $row
    }

    if ($baseline.timing.slice_gcode_bytes -and $round1.mins.gcode_bytes) {
        $gB = [long]$baseline.timing.slice_gcode_bytes
        $gN = [long]$round1.mins.gcode_bytes
        $gPct = [math]::Round(($gN - $gB) / $gB * 100, 2)
        if ([math]::Abs($gPct) -gt 1) {
            Write-Gate ("NOTE: PerfBench slice gcode size {0}B vs baseline {1}B ({2}%) -- correctness stage above is the authority" -f $gN, $gB, $gPct) 'Yellow'
        }
    }

    if ($timingFailed) {
        Write-Gate 'GATE FAILED: slice efficiency regressed beyond threshold in both rounds (exit 2)' 'Red'
        exit 2
    }
} else {
    Write-Gate ("timing round: slice_ms min {0}ms, preview_parse_ms min {1}ms (recorded as new baseline)" -f $curSlice, $curPreview)
}

# ── Stage 3: step profiles (P4, §6d; 4 models x one PerfBench round each) ──
# Each profile model runs PerfBench once with its tier; the report's
# slice_steps_ms / slice_step_counts come from the SliceService step
# timeline (first-occurrence stamps; the trailing "done" sentinel is
# dropped and only anchors gcode_export's duration).
function Invoke-ProfileRound([int]$Round) {
    $profiles = [ordered]@{}
    foreach ($m in $script:profileModels) {
        $env:OWZX_PERF_LOG = '1'
        $env:PERF_BENCH_MODELS_DIR = $modelsDir
        $env:PERF_BENCH_TIERS = $m.tier
        $env:PERF_BENCH_SLICE = '1'
        $profReport = Join-Path $outDir ("step_profile_r{0}_{1}.json" -f $Round, $m.key)
        $env:PERF_BENCH_REPORT = $profReport
        Write-Gate ("profiling {0} (tier {1}, round {2})" -f $m.key, $m.tier, $Round)
        $prevEap = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        & $perfExe *> $null
        $ErrorActionPreference = $prevEap
        if ($LASTEXITCODE -ne 0) { Fail-Infra "PerfBench exited with code $LASTEXITCODE (profile $($m.key) round $Round)" }
        $r = Get-Content $profReport -Raw | ConvertFrom-Json
        $node = $r.models.($m.tier)
        $propNames = @($node.PSObject.Properties.Name)
        if (-not $node -or ($propNames -notcontains 'slice_steps_ms')) {
            Fail-Infra "step profile missing for $($m.key): PerfBench wrote no slice_steps_ms (mock build without libslic3r?)"
        }
        $profiles[$m.key] = [ordered]@{
            stl           = $m.stl
            slice_ms      = [double]$node.slice_ms
            steps_ms      = $node.slice_steps_ms
            steps_counts  = $node.slice_step_counts
        }
    }
    return $profiles
}

# Compares one model's current profile against the baseline profile. Returns
# rows (for report + console) and the failure list. Failure rules:
#   - step KEY set changed (engine wording changed) -> always a failure;
#   - step regressed > +30% AND baseline step >= min_step_ms -> a failure.
function Compare-StepProfile([string]$Key, $BaseProfile, $CurProfile) {
    $baseSteps = $BaseProfile.steps_ms
    $curSteps = $CurProfile.steps_ms
    $baseNames = @($baseSteps.PSObject.Properties.Name)
    $curNames = @($curSteps.PSObject.Properties.Name)
    $maxPct = [double]$script:stepThresholds.max_regression_pct
    $minMs = [double]$script:stepThresholds.min_step_ms
    $rows = @()
    $failures = @()

    foreach ($name in $baseNames) {
        if ($curNames -notcontains $name) {
            $failures += "step key '$name' disappeared (engine set_status wording changed? re-gold via -UpdateBaseline after review)"
            $rows += @{ model = $Key; step = $name; base_ms = [math]::Round([double]$baseSteps.$name, 1); cur_ms = $null; delta_pct = $null; verdict = 'MISSING-KEY' }
        }
    }
    foreach ($name in $curNames) {
        if ($baseNames -notcontains $name) {
            $failures += "new step key '$name' appeared (engine set_status wording changed? re-gold via -UpdateBaseline after review)"
            $rows += @{ model = $Key; step = $name; base_ms = $null; cur_ms = [math]::Round([double]$curSteps.$name, 1); delta_pct = $null; verdict = 'NEW-KEY' }
        }
    }

    foreach ($name in $baseNames) {
        if ($curNames -notcontains $name) { continue }
        $b = [double]$baseSteps.$name
        $c = [double]$curSteps.$name
        $pct = if ($b -gt 0) { [math]::Round(($c - $b) / $b * 100, 1) } else { 0.0 }
        if ($b -lt $minMs) {
            $verdict = 'ok (below {0}ms floor)' -f $minMs
            Write-Gate ("  {0,-22} {1,9:F1} -> {2,9:F1} ms  {3,7}%  {4}" -f $name, $b, $c, $pct, $verdict) 'DarkGray'
        } elseif ($pct -gt $maxPct) {
            $verdict = 'REGRESSION'
            $failures += ("step '{0}' {1}ms vs baseline {2}ms (+{3}% > {4}% limit)" -f $name, $c, $b, $pct, $maxPct)
            Write-Gate ("  {0,-22} {1,9:F1} -> {2,9:F1} ms  {3,7}%  {4}" -f $name, $b, $c, $pct, $verdict) 'Red'
        } elseif ($pct -lt -10) {
            $verdict = 'improved'
            Write-Gate ("  {0,-22} {1,9:F1} -> {2,9:F1} ms  {3,7}%  {4}" -f $name, $b, $c, $pct, $verdict) 'Green'
        } else {
            $verdict = 'ok'
            Write-Gate ("  {0,-22} {1,9:F1} -> {2,9:F1} ms  {3,7}%  {4}" -f $name, $b, $c, $pct, $verdict)
        }
        $rows += @{ model = $Key; step = $name; base_ms = [math]::Round($b, 1); cur_ms = [math]::Round($c, 1); delta_pct = $pct; verdict = $verdict }
    }
    return @{ rows = $rows; failures = $failures }
}

$stepProfileRows = @()
$stepProfileFailed = $false
$stepProfileRoundsRun = 0
if ($establishing) {
    Write-Gate 'step profiles: establishing baseline (single round per model)' 'Yellow'
    $script:baselineProfiles = Invoke-ProfileRound 1
    $stepProfileRoundsRun = 1
    foreach ($m in $script:profileModels) {
        $p = $script:baselineProfiles.($m.key)
        Write-Gate ("  {0}: slice {1:F0}ms, {2} step keys (recorded as new baseline)" -f $m.key, $p.slice_ms, @($p.steps_ms.PSObject.Properties.Name).Count)
    }
} else {
    if (-not $baseline.step_profiles) {
        Fail-Infra 'baseline lacks step_profiles (pre-P4 schema) -- run with -UpdateBaseline to re-gold'
    }
    $profiles1 = Invoke-ProfileRound 1
    $stepProfileRoundsRun = 1
    $allRows1 = @()
    $fail1 = @()
    foreach ($m in $script:profileModels) {
        Write-Gate ("step profile [{0}] (slice {1:F0}ms vs baseline {2:F0}ms):" -f $m.key, $profiles1.($m.key).slice_ms, $baseline.step_profiles.($m.key).slice_ms)
        $cmp = Compare-StepProfile $m.key $baseline.step_profiles.($m.key) $profiles1.($m.key)
        $allRows1 += $cmp.rows
        $fail1 += $cmp.failures
    }
    $stepProfileRows = $allRows1

    # Volatile-metric rule (same as the timing layer): one breaching round
    # gets an automatic re-measure; only a breach in BOTH rounds fails.
    if ($fail1.Count -gt 0) {
        Write-Gate ("round-1 step profile breached ({0} finding(s)) -- re-measuring profiles once" -f $fail1.Count) 'Yellow'
        $profiles2 = Invoke-ProfileRound 2
        $stepProfileRoundsRun = 2
        $allRows2 = @()
        $fail2 = @()
        foreach ($m in $script:profileModels) {
            Write-Gate ("step profile [{0}] round 2 (slice {1:F0}ms):" -f $m.key, $profiles2.($m.key).slice_ms)
            $cmp2 = Compare-StepProfile $m.key $baseline.step_profiles.($m.key) $profiles2.($m.key)
            $allRows2 += $cmp2.rows
            $fail2 += $cmp2.failures
        }
        $stepProfileRows = $allRows2
        if ($fail2.Count -gt 0) {
            $stepProfileFailed = $true
            foreach ($f in $fail2) { Write-Gate ("STEP PROFILE FAIL: {0}" -f $f) 'Red' }
        } else {
            Write-Gate 'round-2 step profiles within thresholds -- treated as noise (docs/perf-baseline.md two-round rule)' 'Yellow'
        }
    }
}

if ($stepProfileFailed) {
    Write-Gate 'GATE FAILED: slice step profile regressed beyond threshold in both rounds (exit 2)' 'Red'
    exit 2
}

# ── Persist baseline / report ──────────────────────────────────────────────
$commit = Get-HeadCommit
$timestamp = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')

if ($establishing) {
    $newBaseline = [ordered]@{
        version = 2
        established_at_utc = $timestamp
        established_from_commit = $commit
        pins = $script:pins
        models = $newModelsJson
        timing = [ordered]@{
            repeats = $repeats
            slice_ms_min = [math]::Round($curSlice, 1)
            preview_parse_ms_min = [math]::Round($curPreview, 1)
            slice_gcode_bytes = $round1.mins.gcode_bytes
            max_regression_pct = [ordered]@{ slice_ms = 25; preview_parse_ms = 10 }
        }
        # P4 (§6d): per-step wall-clock profiles of the 4 representative
        # models, one PerfBench round each. steps_ms keys are the §6d step
        # keys (engine set_status text -> normalized key); a key appearing or
        # disappearing means the engine wording changed -> gate red ->
        # re-gold after human review.
        step_profiles = $script:baselineProfiles
        step_thresholds = $script:stepThresholds
    }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $baselinePath) | Out-Null
    # Write BOM-less UTF-8: PS 5.1's Set-Content -Encoding UTF8 adds a BOM,
    # which breaks plain json.load() consumers (python, jq) on this committed file.
    [System.IO.File]::WriteAllText($baselinePath, ($newBaseline | ConvertTo-Json -Depth 8), (New-Object System.Text.UTF8Encoding($false)))
    Write-Gate ("baseline written: {0} (commit {1}) -- commit it to make the golden durable" -f $baselinePath, $commit) 'Yellow'
}

$report = [ordered]@{
    timestamp_utc = $timestamp
    commit = $commit
    mode = $(if ($establishing) { 'establish-baseline' } else { 'enforce' })
    repeats = $repeats
    correctness = $correctnessResults
    timing = [ordered]@{
        mins = @{ slice_ms = [math]::Round($curSlice, 1); preview_parse_ms = [math]::Round($curPreview, 1) }
        runs_round1 = $round1.runs
        rows = $timingRows
    }
    # P4 (§6d): step-profile verdicts, per step: baseline -> current -> delta%.
    step_profiles = [ordered]@{
        thresholds = $script:stepThresholds
        rounds_run = $stepProfileRoundsRun
        failed = $stepProfileFailed
        rows = $stepProfileRows
    }
}
($report | ConvertTo-Json -Depth 8) | Set-Content -Path $reportPath -Encoding UTF8
Write-Gate ("report: " + $reportPath)

# Hygiene: PerfBench slices drop timestamped gcode files into build/ -- clean
# stale bench leftovers (this gate's own outputs live under perf_out/).
# P4: the profile tiers add the tower/grid/block base names.
Get-ChildItem -Path $buildDir -Filter '*.gcode' -File -ErrorAction SilentlyContinue |
    Where-Object { ($_.Name -like 'bench_*' -or $_.Name -like 'Block20XY_*') -and $_.LastWriteTime -lt (Get-Date).AddMinutes(-15) } |
    Remove-Item -Force -ErrorAction SilentlyContinue

Write-Gate 'PASS: slice correctness + efficiency + step-profile gate green' 'Green'
exit 0
