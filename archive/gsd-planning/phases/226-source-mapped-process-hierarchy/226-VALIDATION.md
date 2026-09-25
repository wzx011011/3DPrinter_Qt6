# Phase 226 Validation

This matrix is required for `226-01-PLAN.md`. It covers HIER-01 and HIER-02
only. All automated checks use the canonical build script and stop immediately
when it or a focused test fails.

## Task Matrix

| Plan task | Requirement | Focused automated command | Required manual inspection | Required evidence |
|---|---|---|---|---|
| Task 1: ordered C++ manifest | HIER-01, HIER-02 | `ViewModelSmokeTests.exe sourceMappedProcessHierarchyMatchesTabPrint` after the canonical build | None before QML projection exists; Task 2 and the phase gate inspect the rendered contract. | Independent six-page, 36 page-qualified group, membership, ordering, and fail-closed mapping assertions pass. |
| Task 2: Process-only QML projection | HIER-01, HIER-02 | `QmlUiAuditTests.exe processSettingsConsumesSourceMappedHierarchy` after the canonical build | Complete the Process dialog checklist below at both required sizes. | Process branch consumes C++ hierarchy data, lacks old pages and fallback/disclosure tokens, and retains Printer/Material anchors. |
| Phase gate | HIER-01, HIER-02 | Both focused tests after one canonical build, then encoding guard on the Phase-only temporary index | Confirm the completed Task 2 checklist and preserved Printer/Material routes. | Full task evidence, UTF-8 without BOM, and preserved user hunks. |

Run the command for each completed task from the repository root.

```powershell
& powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& .\build\ViewModelSmokeTests.exe sourceMappedProcessHierarchyMatchesTabPrint
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
```

```powershell
& powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& .\build\QmlUiAuditTests.exe processSettingsConsumesSourceMappedHierarchy
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
```

For the phase gate, run one canonical build before both focused tests. Do not
run a test after a failed build.

```powershell
& powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& .\build\ViewModelSmokeTests.exe sourceMappedProcessHierarchyMatchesTabPrint
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& .\build\QmlUiAuditTests.exe processSettingsConsumesSourceMappedHierarchy
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
```

## Required Manual Process Inspection

After the phase gate, launch `build\OWzxSlicer.exe` and open the existing
Settings dialog with the Process tier selected. Inspect both the 736px by 593px
dialog and the established higher UI-scale fixture.

1. Quality is selected on initial open. The tab strip shows only Quality,
   Strength, Speed, Support, Multimaterial, and Others in that order.
2. Click each tab, then use Left, Right, Home, End, Enter, and Space. Exactly
   one tab is active, focus is visible, and the active page's static group
   titles and rows follow the source order.
3. Confirm long tab labels stay on one line, option rows and headings do not
   overlap or clip, no sidebar is visible, and no Process group title has a
   `+`, chevron, count, reset action, or disclosure behavior.
4. Open Printer and Material settings and confirm their existing tabs, headers,
   and row route remain unchanged. Record any visual issue as a Phase 226
   failure; do not work around it with a fallback page or group.

## Protected-Hunk Preflight

`tests/ViewModelSmokeTests.cpp` and `tests/QmlUiAuditTests.cpp` have unrelated
unstaged user edits at the start of Phase 226. Run this once before modifying
any Phase-owned file. It saves the original target-file bytes, user diffs, and
real index under `.git`; it does not alter source files or stage user work.

```powershell
$ErrorActionPreference = 'Stop'
$gitDir = & git rev-parse --git-dir
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$phaseRoot = Join-Path $gitDir "phase226-preflight-$stamp"
New-Item -ItemType Directory -Force -Path $phaseRoot | Out-Null

$phaseFiles = @(
  'src/qml_gui/Models/ConfigOptionModel.h',
  'src/qml_gui/Models/ConfigOptionModel.cpp',
  'src/qml_gui/dialogs/SettingsDialog.qml',
  'tests/ViewModelSmokeTests.cpp',
  'tests/QmlUiAuditTests.cpp'
)
$protectedTests = @(
  'tests/ViewModelSmokeTests.cpp',
  'tests/QmlUiAuditTests.cpp'
)

& git diff --no-ext-diff --check
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$head = & git rev-parse HEAD
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
[IO.File]::WriteAllText((Join-Path $phaseRoot 'head.txt'), "$head`n", [Text.Encoding]::ASCII)
Copy-Item -LiteralPath (Join-Path $gitDir 'index') -Destination (Join-Path $phaseRoot 'index-before')

foreach ($path in $phaseFiles) {
  $baseline = Join-Path $phaseRoot (Join-Path 'baseline' $path.Replace('/', '\\'))
  New-Item -ItemType Directory -Force -Path (Split-Path -Parent $baseline) | Out-Null
  Copy-Item -LiteralPath $path -Destination $baseline
}
foreach ($path in $protectedTests) {
  $diffPath = Join-Path $phaseRoot ("user-" + [IO.Path]::GetFileName($path) + '.patch')
  & git diff --no-ext-diff --binary --unified=0 --output=$diffPath -- $path
  if ($LASTEXITCODE -gt 1) { exit $LASTEXITCODE }
  if ((Get-Item -LiteralPath $diffPath).Length -eq 0) { throw "Expected protected user diff is empty: $path" }
}
$stagedBeforePatch = Join-Path $phaseRoot 'staged-before.patch'
& git diff --cached --no-ext-diff --binary --output=$stagedBeforePatch
if ($LASTEXITCODE -gt 1) { exit $LASTEXITCODE }
Write-Host "Phase 226 preflight saved at $phaseRoot"
```

The two Phase test additions must use new test-slot declarations beside the
existing settings/config test declarations and append their implementations in
new named Phase 226 test blocks at file end. Do not edit the current user-hunk
regions: ViewModel enum and auto-drop tests, or QML audit geometry and
auto-drop tests. If another edit has made those anchors overlap, stop before
implementing the test additions.

## Non-Interactive Phase-Only Commit

After all task verification and manual inspection pass, run the following
procedure. It produces three-line-context patches from each current Phase file
relative to its preflight snapshot. The patches therefore contain Phase 226
additions only, including only the two new test slots, not the user hunks. The
only zero-context patches are the captured user hunks, which are used solely to
assert that those hunks never enter the temporary index and remain in the
working tree after the commit.

The procedure commits through a temporary index that starts at the preflight
`HEAD`. It never runs `git add`, `git add -p`, `git commit --only`, or a
whole-file staging command on the dirty test files. Before applying a Phase
patch, it requires every Phase hunk's three-line context and every named Phase
test anchor to be disjoint from the captured protected-user hunk ranges. It
then applies the context-bearing patches to the temporary `HEAD` index without
`--unidiff-zero` and reads that index's blobs to prove each declaration and
definition landed at its declared location before `commit-tree` runs.

```powershell
$ErrorActionPreference = 'Stop'
if (-not $phaseRoot) { throw 'Reuse the phaseRoot printed by Protected-Hunk Preflight.' }
$head = Get-Content -LiteralPath (Join-Path $phaseRoot 'head.txt') -Raw
$head = $head.Trim()
$phaseFiles = @(
  'src/qml_gui/Models/ConfigOptionModel.h',
  'src/qml_gui/Models/ConfigOptionModel.cpp',
  'src/qml_gui/dialogs/SettingsDialog.qml',
  'tests/ViewModelSmokeTests.cpp',
  'tests/QmlUiAuditTests.cpp'
)
$protectedTests = @(
  'tests/ViewModelSmokeTests.cpp',
  'tests/QmlUiAuditTests.cpp'
)
$phaseTestAnchors = @{
  'tests/ViewModelSmokeTests.cpp' = @{
    Slot = 'void sourceMappedProcessHierarchyMatchesTabPrint();'
    SlotPredecessor = 'void presetReadOnlyActionBlockerReasons();'
    SlotSuccessor = '// v3.0 Phase 16-01: PartPlate/PartPlateList domain model (pure-data, no libslic3r dep)'
    Definition = 'void ViewModelSmokeTests::sourceMappedProcessHierarchyMatchesTabPrint()'
    DefinitionPrefix = 'void ViewModelSmokeTests::'
    TestMain = 'QTEST_MAIN(ViewModelSmokeTests)'
  }
  'tests/QmlUiAuditTests.cpp' = @{
    Slot = 'void processSettingsConsumesSourceMappedHierarchy();'
    SlotPredecessor = 'void deletedSettingsPathsStayAbsent();'
    SlotSuccessor = '// Phase 57-02 (CLEAN-01 regression): the 3 named routes plus the dead'
    Definition = 'void QmlUiAuditTests::processSettingsConsumesSourceMappedHierarchy()'
    DefinitionPrefix = 'void QmlUiAuditTests::'
    TestMain = 'QTEST_MAIN(QmlUiAuditTests)'
  }
}

function Get-PatchHunks {
  param([Parameter(Mandatory = $true)][string]$PatchPath)

  $hunks = @()
  foreach ($line in [IO.File]::ReadLines($PatchPath)) {
    if ($line -match '^@@ -(?<oldStart>\d+)(?:,(?<oldCount>\d+))? \+(?<newStart>\d+)(?:,(?<newCount>\d+))? @@') {
      $hunks += [pscustomobject]@{
        OldStart = [int]$Matches.oldStart
        OldCount = if ($Matches.oldCount) { [int]$Matches.oldCount } else { 1 }
        NewStart = [int]$Matches.newStart
        NewCount = if ($Matches.newCount) { [int]$Matches.newCount } else { 1 }
        Header = $line
      }
    }
  }
  if ($hunks.Count -eq 0) { throw "Patch has no hunks: $PatchPath" }
  return $hunks
}

function Test-HunkSpansOverlap {
  param(
    [Parameter(Mandatory = $true)]$Left,
    [Parameter(Mandatory = $true)][ValidateSet('Old', 'New')][string]$LeftSide,
    [Parameter(Mandatory = $true)]$Right,
    [Parameter(Mandatory = $true)][ValidateSet('Old', 'New')][string]$RightSide
  )

  $leftStart = [int]$Left."${LeftSide}Start"
  $leftCount = [int]$Left."${LeftSide}Count"
  $rightStart = [int]$Right."${RightSide}Start"
  $rightCount = [int]$Right."${RightSide}Count"
  # Treat a zero-length insertion/deletion as its adjacent source line. This is
  # intentionally conservative: Phase context may not touch that insertion point.
  $leftEnd = if ($leftCount -gt 0) { $leftStart + $leftCount - 1 } else { $leftStart }
  $rightEnd = if ($rightCount -gt 0) { $rightStart + $rightCount - 1 } else { $rightStart }
  return $leftStart -le $rightEnd -and $rightStart -le $leftEnd
}

function Assert-PhasePatchIsDisjointFromProtectedHunks {
  param(
    [Parameter(Mandatory = $true)][string]$Path,
    [Parameter(Mandatory = $true)][string]$PhasePatch,
    [Parameter(Mandatory = $true)][string]$UserPatch,
    [Parameter(Mandatory = $true)][hashtable]$Anchor
  )

  $phaseText = [IO.File]::ReadAllText($PhasePatch, [Text.Encoding]::UTF8)
  foreach ($needle in @($Anchor.Slot, $Anchor.Definition)) {
    $matches = [regex]::Matches($phaseText, [regex]::Escape("+$needle"))
    if ($matches.Count -ne 1) { throw "Phase patch must add exactly one named anchor '$needle': $Path" }
  }

  $phaseHunks = @(Get-PatchHunks $PhasePatch)
  $userHunks = @(Get-PatchHunks $UserPatch)
  foreach ($phaseHunk in $phaseHunks) {
    foreach ($userHunk in $userHunks) {
      # Phase old-side spans are the preflight baseline/context. User new-side
      # spans are that same preflight file, so overlap means the Phase hunk's
      # anchor or surrounding three-line context touches protected user work.
      if (Test-HunkSpansOverlap $phaseHunk Old $userHunk New) {
        throw "Phase hunk context overlaps a protected user hunk in $Path. Move the named test additions before continuing. Phase: $($phaseHunk.Header); user: $($userHunk.Header)"
      }
    }
  }
}

function Assert-TemporaryIndexTestAnchors {
  param([Parameter(Mandatory = $true)][hashtable]$Anchors)

  foreach ($path in $Anchors.Keys) {
    $anchor = $Anchors[$path]
    $blobLines = @(& git show ":$path")
    if ($LASTEXITCODE -ne 0) { throw "Cannot read temporary-index blob: $path" }
    $blob = [string]::Join("`n", $blobLines)
    foreach ($needle in @($anchor.Slot, $anchor.SlotPredecessor, $anchor.SlotSuccessor, $anchor.Definition, $anchor.TestMain)) {
      $count = [regex]::Matches($blob, [regex]::Escape($needle)).Count
      if ($count -ne 1) { throw "Temporary-index anchor '$needle' must occur exactly once: $path" }
    }

    $slotOffset = $blob.IndexOf($anchor.Slot)
    $predecessorOffset = $blob.IndexOf($anchor.SlotPredecessor)
    $successorOffset = $blob.IndexOf($anchor.SlotSuccessor)
    $definitionOffset = $blob.IndexOf($anchor.Definition)
    $testMainOffset = $blob.IndexOf($anchor.TestMain)
    if (-not ($predecessorOffset -lt $slotOffset -and $slotOffset -lt $successorOffset)) {
      throw "Temporary-index test slot is not at its declared settings/config anchor: $path"
    }
    $slotRegion = $blob.Substring($predecessorOffset, $successorOffset - $predecessorOffset)
    $slotDeclarations = @([regex]::Matches($slotRegion, '(?m)^  void .+;$'))
    if ($slotDeclarations.Count -ne 2 -or $slotDeclarations[-1].Value -ne "  $($anchor.Slot)") {
      throw "Temporary-index test slot is not adjacent to its declared settings/config slot: $path"
    }
    if (-not ($definitionOffset -gt $successorOffset -and $definitionOffset -lt $testMainOffset)) {
      throw "Temporary-index test definition is not appended before QTEST_MAIN: $path"
    }
    if ($blob.LastIndexOf($anchor.DefinitionPrefix) -ne $definitionOffset) {
      throw "Temporary-index test definition is not the final named test block: $path"
    }

    $lineIndex = 0
    foreach ($line in $blobLines) {
      $lineIndex++
      if ($line -like "*$($anchor.Slot)*" -or $line -like "*$($anchor.Definition)*") {
        Write-Host "Temporary-index Phase 226 anchor at ${path}:${lineIndex}: $line"
      }
    }
  }
}
$patchRoot = Join-Path $phaseRoot 'phase-patches'
New-Item -ItemType Directory -Force -Path $patchRoot | Out-Null

foreach ($path in $phaseFiles) {
  $baseline = Join-Path $phaseRoot (Join-Path 'baseline' $path.Replace('/', '\\'))
  $patch = Join-Path $patchRoot ([IO.Path]::GetFileName($path) + '.patch')
  & git diff --no-index --no-ext-diff --binary --unified=3 --label "a/$path" --label "b/$path" --output=$patch $baseline $path
  if ($LASTEXITCODE -gt 1) { exit $LASTEXITCODE }
  if ((Get-Item -LiteralPath $patch).Length -eq 0) { throw "Phase patch is empty: $path" }
}

foreach ($path in $protectedTests) {
  $phasePatch = Join-Path $patchRoot ([IO.Path]::GetFileName($path) + '.patch')
  $userPatch = Join-Path $phaseRoot ("user-" + [IO.Path]::GetFileName($path) + '.patch')
  Assert-PhasePatchIsDisjointFromProtectedHunks $path $phasePatch $userPatch $phaseTestAnchors[$path]
}

$oldIndex = $env:GIT_INDEX_FILE
$tempIndex = Join-Path $phaseRoot 'phase226.index'
$restoreIndex = Join-Path $phaseRoot 'restore.index'
try {
  $env:GIT_INDEX_FILE = $tempIndex
  & git read-tree $head
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  foreach ($path in $phaseFiles) {
    $patch = Join-Path $patchRoot ([IO.Path]::GetFileName($path) + '.patch')
    & git apply --cached --check --whitespace=nowarn $patch
    if ($LASTEXITCODE -ne 0) { throw "Phase-only patch overlaps baseline or index: $path" }
    & git apply --cached --whitespace=nowarn $patch
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  }

  Assert-TemporaryIndexTestAnchors $phaseTestAnchors

  foreach ($path in $protectedTests) {
    $userPatch = Join-Path $phaseRoot ("user-" + [IO.Path]::GetFileName($path) + '.patch')
    & git apply --cached --reverse --check --whitespace=nowarn --unidiff-zero $userPatch
    if ($LASTEXITCODE -eq 0) { throw "Captured user hunk entered the Phase index: $path" }
  }

  $stagedNames = @(& git diff --cached --name-only --diff-filter=ACMR | Sort-Object)
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  $expectedNames = @($phaseFiles | Sort-Object)
  if (Compare-Object $expectedNames $stagedNames) { throw 'Temporary index contains a non-Phase file or misses a Phase file.' }
  & git diff --cached --check
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  & python "$env:USERPROFILE\.coding-encoding-guard\encoding_guard.py"
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  $tree = & git write-tree
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  & git read-tree $tree
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  & git apply --cached --check --whitespace=nowarn (Join-Path $phaseRoot 'staged-before.patch')
  if ($LASTEXITCODE -ne 0) { throw 'Existing staged user work overlaps the Phase tree; no commit was created.' }

  $tree = & git write-tree
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  $commit = 'feat(226-01): restore source-mapped Process hierarchy' | & git commit-tree $tree -p $head
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  & git update-ref HEAD $commit $head
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  $env:GIT_INDEX_FILE = $null
  & git read-tree $commit
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  & git apply --cached --whitespace=nowarn (Join-Path $phaseRoot 'staged-before.patch')
  if ($LASTEXITCODE -ne 0) { throw 'Commit exists, but the real index could not restore prior staged user work. Restore index-before before continuing.' }

  foreach ($path in $protectedTests) {
    $userPatch = Join-Path $phaseRoot ("user-" + [IO.Path]::GetFileName($path) + '.patch')
    & git apply --reverse --check --whitespace=nowarn --unidiff-zero $userPatch
    if ($LASTEXITCODE -ne 0) { throw "Captured user hunk no longer exists in the worktree: $path" }
  }
}
finally {
  if ($null -eq $oldIndex -or $oldIndex -eq '') { Remove-Item Env:GIT_INDEX_FILE -ErrorAction SilentlyContinue } else { $env:GIT_INDEX_FILE = $oldIndex }
}
```

If any Phase-hunk context/anchor assertion, `--check`, user-hunk assertion, or
index restoration fails, do not stage or commit a whole test file. Preserve the patch directory, restore
`$phaseRoot\index-before` only if the final index restoration failed, and report
the overlap with the generated patch path.
