# Phase 187: v5.4 Cross-Workstream Regression Gate

**Status:** Planned
**Workstream:** Cross-WS
**Requirement:** REGRESS-08
**Dependencies:** Phases 180-186 complete (Wave C final gate)

## Goal

Add a consolidated `v54RegressionLocked` source-audit slot to `tests/QmlUiAuditTests.cpp` that locks every v5.4 anchor and re-asserts the v5.3/v5.2/v5.1/v5.0/v4.x anchors. This is the standard cross-workstream gate pattern established by v4.5 Phase 116 and continued through v5.3 Phase 179.

## Scope

### Add `v54RegressionLocked()` to `tests/QmlUiAuditTests.cpp`

**Declaration** (after line 628, where `v53RegressionLocked()` is declared):
```cpp
// Phase 187 (REGRESS-08): v5.4 cross-workstream regression gate. Spots
// every v5.4 anchor + re-asserts v5.3/v5.2/v5.1/v5.0/v4.x.
void v54RegressionLocked();
```

**Implementation** (after line 8079, where `v53RegressionLocked()` ends), three-section structure mirroring v53:

#### Section 1 — v5.4 anchor assertions

For each workstream, pick 1-2 canonical anchors:

- **CRASH-01 (Phase 180)**: verify a key string from the renderer/selection destructor guard landed in `src/qml_gui/Renderer/RhiViewport.cpp` (e.g. `m_isClosing` or equivalent guard identifier).
- **CRASH-02 (Phase 181)**: verify `valid_instance` bounds-check in `src/core/model/PartPlate.cpp`; verify `"processes"` (not `"presets"`) in `src/qml_gui/dialogs/ExportPresetBundleDialog.qml`.
- **CRASH-03 (Phase 182)**: verify `reload_from_disk` case-insensitive fallback in `src/core/viewmodels/EditorViewModel.cpp`; verify `PreferencesPage.qml` checkbox exists.
- **FEAT-04 (Phase 183)**: verify `perExtruderValues` role in `ConfigOptionModel.h` (or equivalent nullable-aware accessor).
- **I18N-07 (Phases 184-185)**: verify de/fr/ja/ko coverage by spot-checking a key string in each .ts (e.g. `"Drucken"` in `i18n/de.ts`, `"Imprimer"` in `i18n/fr.ts`, `"印刷"` in `i18n/ja.ts`, `"인쇄"` in `i18n/ko.ts`).
- **META-01 (Phase 186)**: verify `.planning/milestones/v5.4-ROADMAP.md` exists and contains `Upstream Sync Closure`.

#### Section 2 — Historical re-assertions

Carry forward the same anchors v53 carried (this is a snapshot of "what we don't want to regress"):

- **v5.3 re-assertion**: `Theme.borderActive` + `kSidebarMinWidth = 300` (CL-01/CL-02 carry).
- **v5.2 re-assertion**: same as v53's v5.2 segment.
- **v5.1 re-assertion**: `comparePresetsDetailed` in EditorViewModel.h / ProjectServiceMock.cpp / ConfigViewModel.h.
- **v5.0 re-assertion**: `Slic3r::Emboss::text2shapes` in ProjectServiceMock.cpp.
- **v4.6 re-assertion (canary)**: `calibMode = 7` + `calibMode = 9` in CalibrationServiceMock.cpp.

#### Section 3 — Naming convention

All failure messages use prefix `REGRESS-08/<requirement-id>:` for v5.4 anchors and `REGRESS-08/vX.Y:` for historical re-assertions, mirroring v53's style.

## Out of Scope

- Any new product behavior.
- Any non-QmlUiAuditTests changes (this gate is purely a source-audit slot).
- Re-asserting v4.5/v4.7/v4.8 specifically (those anchors are covered transitively via later milestones' slots; if a v4.5-era anchor is at risk, surface it for addition).

## Verification

- `tests/QmlUiAuditTests.cpp` compiles, `v54RegressionLocked` runs as a new test case.
- ctest 5/5 groups PASS: QmlUiAuditTests (count +1 → 137+), ViewModelSmokeTests, PartPlateTests, PrepareSceneDataTests, PreviewParserTests.
- Canonical build (`scripts/auto_verify_with_vcvars.ps1`) exits 0, 0 errors.
- owzx-cli slice end-to-end 4 tests pass (carry-over from bb3 sync verification baseline).

## Risk Notes

- If any Phase 180-185 deliverable's anchor string isn't yet decided at planning time, the slot can use a placeholder `// TODO: anchor TBD` and be finalized in execution. The structure must be in place; specific strings can be adjusted per phase outcome.
- The historical re-assertion set is curated, not exhaustive — v5.4 deliberately carries the same set as v5.3 to avoid unbounded growth. If a v5.4 change invalidates an older anchor (e.g. RhiViewport destructor refactor removes a checked symbol), the slot must be updated to track the renamed symbol rather than the old name.
