# Roadmap: OWzx Slicer

## Milestones

- ✅ **v2.9** Implementation Realignment and Stabilization — Phases 10-15 (shipped 2026-06-25)
- ✅ **v3.0** PartPlate Core — Phases 16-22 (shipped 2026-06-26)
- ✅ **v3.1** QRhi Rendering — Phases 23-28 (shipped 2026-06-28)
- ✅ **v3.2** Multi-Plate Data Polish — Phases 29-32 (audited 2026-06-28)
- ✅ **v3.3** Slice Preview Main Flow MVP — Phases 33-36 (superseded by v3.4)
- ✅ **v3.4** Import to G-code Complete Workflow — Phases 37-43 (closed by automated E2E)
- ✅ **v3.5** Preset Authoring Complete Workflow — Phases 44-49 (superseded after Phase 46)
- ✅ **v3.6** Screenshot-Driven OrcaSlicer UI Restoration — Phases 50-58 (shipped 2026-07-03)
- ✅ **v3.7** Screenshot-Level UI Parity Closure — Phases 59-64 (2026-07-04)
- ✅ **v3.8** RHI Gizmo Parity — Phases 65-73 (shipped 2026-07-04)
- ✅ **v3.9** Prepare Page UI Restoration — Phases 74-78 (shipped 2026-07-06)
- ✅ **v4.0** Preview Page UI Restoration — Phases 79-83 (shipped 2026-07-07)
- ✅ **v4.1** Parameter Settings Dialogs Source-Truth Restoration — Phases 84-88 (shipped 2026-07-09)
- ✅ **v4.2** AssembleView Source-Truth Restoration — Phases 89-93 (shipped 2026-07-09)
- ✅ **v4.3** Real Thumbnail Capture And 3MF Round-Trip — Phases 94-98 (shipped 2026-07-10)
- ✅ **v4.4** Wipe-Tower Geometry Readback And Real Rendering — Phases 99-102 (shipped 2026-07-12)
- ✅ **v4.5** Backlog Closure — Phases 103-116 (shipped 2026-07-13)
- ✅ **v4.6** Core Feature Completion Sweep — Phases 117-128 (shipped 2026-07-15)
- ✅ **v4.7** Polish, i18n & Advanced Feature Recovery — Phases 129-135 (shipped 2026-07-15, tech_debt)
- ✅ **v4.8** Dependency Unlock, Assembly Transform & i18n Completion — Phases 136-140 (shipped 2026-07-16, tech_debt)
- 🚧 **v5.0** Advanced Feature Recovery & Tech-Debt Closure — Phases 141-153 (in planning)

## Current Milestone: v5.0 Advanced Feature Recovery & Tech-Debt Closure

**Goal:** In one cycle, close the accumulated tech debt across v4.6/v4.7/v4.8 AND recover three long-blocked P1 feature clusters (Emboss / Preset Bundle / PartPlate), plus correct the mistaken "OpenVDB unavailable" premise by linking OpenVDB and unlocking the Hollow gizmo.

**Scope rule:** All work is offline/local and maps to OrcaSlicer v7.0.1 upstream. The "OpenVDB unavailable" constraint is **revised**: OpenVDB IS present in DEPS_PREFIX (`libopenvdb.lib` 55MB + `FindOpenVDB.cmake`); the v4.x "unavailable" premise was an incomplete CMake port. LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows remain removed.

**Five workstreams (32 requirements across 13 phases, 141-153):**
- **WS1 — Tech-Debt Closure (DEBT-01..05, Phase 141):** CGAL-02 true intersection (swap `minus`→`intersect` on op==2 + don't delete tool), remove orphaned `meshBooleanSelected` menu stub, fix `drillObject` C4715, ASM rotate/scale live-visual compose (full transform matrix in `buildModelVertices`), v5.0 regression lock seed.
- **WS2 — OpenVDB Unlock & Hollow Gizmo (VDB-01..06, Phases 142-143):** `find_package(OpenVDB)` + `openvdb_libs` alias in root CMake + activate `OpenVDBUtils.cpp`; unlock Hollow gizmo availability + button + panel + SLA slice cavity.
- **WS3 — Emboss Text Gizmo Full Port (EMB-01..07, Phases 144-146):** real font loading + `text2shapes` extrude; async `EmbossJob` via Qt Concurrent + gizmo panel; button/SVG/3MF round-trip wiring.
- **WS4 — Preset Bundle Full Chain (PSET-01..07, Phases 147-149):** upstream `.ini`-compatible bundle format + `CreatePresetsDialog`; `UnsavedChangesDialog` 3-way diff + Simple/Advanced C++ filter; Compare/Diff + dirty propagation + round-trip.
- **WS5 — PartPlate UI Completion (PLATE-01..06, Phases 150-152):** read-only UI gap analysis → implementation (reorder / per-plate print-sequence / config-override / thumbnails) → save/reload regression.
- **Cross-WS — REGRESS-04 (Phase 153):** consolidated v5.0 regression gate.

**Coverage note (off-by-one correction):** the REQUIREMENTS.md header previously stated "33 requirements" but the literal ID list contains 32 unique IDs (DEBT-01..05=5, VDB-01..06=6, EMB-01..07=7, PSET-01..07=7, PLATE-01..06=6, REGRESS-04=1 → 32). The roadmapper has updated REQUIREMENTS.md to reflect the accurate count of 32; coverage is 32/32 mapped, 0 unmapped.

## Phases

- [x] Phase 141: v4.x Tech-Debt Closure (WS1) ✓
- [x] Phase 142: OpenVDB CMake Unlock And libslic3r Link (WS2) ✓
- [x] Phase 143: Hollow Gizmo Availability + Button + Panel + SLA Slice (WS2) ✓ partial (VDB-06 → v5.1+)
- [x] Phase 144: Emboss Font Loading And Text2Shapes Extrude (WS3) ✓
- [x] Phase 145: Async EmbossJob And Gizmo Panel (WS3) ✓
- [x] Phase 146: Emboss Wiring, 3MF Round-Trip, And SVG Path (WS3) ✓
- [ ] Phase 147: Preset Bundle INI Format And CreatePresetsDialog (WS4)
- [ ] Phase 148: UnsavedChangesDialog 3-Way Diff And Simple/Advanced Filter (WS4)
- [ ] Phase 149: Compare/Diff, Dirty Propagation, And Bundle Round-Trip (WS4)
- [ ] Phase 150: PartPlate UI Gap Analysis (WS5, read-only)
- [ ] Phase 151: PartPlate UI Implementation (WS5)
- [ ] Phase 152: PartPlate Multi-Plate Save/Reload Regression (WS5)
- [ ] Phase 153: v5.0 Cross-Workstream Regression Gate (REGRESS-04)

| Phase | Name | Goal | Requirements |
|---|---|---|---|
| 141 | v4.x Tech-Debt Closure | Close the four small code-only v4.6/v4.7/v4.8 tech-debt items + seed the v5.0 regression lock | DEBT-01, DEBT-02, DEBT-03, DEBT-04, DEBT-05 |
| 142 | OpenVDB CMake Unlock And libslic3r Link | Correct the v4.x "OpenVDB unavailable" premise at the CMake layer; activate OpenVDBUtils symbols | VDB-01, VDB-02 |
| 143 | Hollow Gizmo Availability + Button + Panel + SLA Slice | Make the Hollow gizmo reachable, visible, and functional on SLA printers | VDB-03, VDB-04, VDB-05, VDB-06 |
| 144 | Emboss Font Loading And Text2Shapes Extrude | Replace mock addTextObject with real font enumeration + extruded text geometry | EMB-01, EMB-02 |
| 145 | Async EmbossJob And Gizmo Panel | Non-blocking text re-extrude on a worker thread + editable emboss gizmo panel | EMB-03, EMB-04 |
| 146 | Emboss Wiring, 3MF Round-Trip, And SVG Path | Wire the existing Emboss/SVG buttons into the real gizmo + persist + SVG path | EMB-05, EMB-06, EMB-07 |
| 147 | Preset Bundle INI Format And CreatePresetsDialog | Upstream-compatible bundle IO + minimal source-truth CreatePresetsDialog | PSET-01, PSET-02 |
| 148 | UnsavedChangesDialog 3-Way Diff And Simple/Advanced Filter | 3-way unsaved-changes dialog + C++-level Simple/Advanced option filtering | PSET-03, PSET-04 |
| 149 | Compare/Diff, Dirty Propagation, And Bundle Round-Trip | Side-by-side preset diff + consistent dirty propagation + bundle round-trip ctest | PSET-05, PSET-06, PSET-07 |
| 150 | PartPlate UI Gap Analysis | Read-only enumeration of PlateBar UI gaps vs upstream; output drives 151 | PLATE-01 |
| 151 | PartPlate UI Implementation | Implement the PLATE-02..05 affordances identified by the gap analysis | PLATE-02, PLATE-03, PLATE-04, PLATE-05 |
| 152 | PartPlate Multi-Plate Save/Reload Regression | Lock full plate state round-trip through 3MF | PLATE-06 |
| 153 | v5.0 Cross-Workstream Regression Gate | Consolidate all v5.0 anchors + re-assert v4.8/v4.7/v4.6 | REGRESS-04 |

### Build Order (parallelism guidance for the executor)

Phase numbers are sequential, but several phases are parallel-safe and may be executed concurrently. The dependency chain below encodes the rationale from the roadmapper constraints (WS1 early, WS2 before WS3, WS5 gap before WS5 impl, REGRESS-04 last).

- **Wave A (parallel, no deps):** Phase 141 (WS1 tech-debt — small, unblocks confidence, seeds the regression slot that later WS will append to). Phase 142 (WS2 OpenVDB CMake unlock — independent build-chain work). Phase 150 (WS5 PLATE-01 read-only gap analysis — read-only, runs early so its output informs Phase 151).
- **Wave B (after 142):** Phase 143 (WS2 Hollow UI — needs OpenVDB symbols + libslic3r SLA path from 142).
- **Wave C (after 142 — proving the libslic3r build-chain pattern):** Phase 144 (WS3 Emboss foundation — font loading + text2shapes). Runs after 142 because Emboss's EmbossJob async pipeline leans on the same libslic3r surface-area expansion pattern.
- **Wave D (after 144):** Phase 145 (WS3 async job + panel — needs the extrude pipeline from 144).
- **Wave E (after 145):** Phase 146 (WS3 wiring + round-trip + SVG — needs the panel from 145).
- **Wave F (parallel, no deps on WS2/WS3):** Phase 147 (WS4 bundle format + CreatePresetsDialog). May start as early as Wave A if bandwidth allows, but listed here because it is the largest non-build-chain UI work.
- **Wave G (after 147):** Phase 148 (WS4 UnsavedChangesDialog + filter — needs the bundle format from 147).
- **Wave H (after 148):** Phase 149 (WS4 Compare/Diff + dirty propagation + round-trip — needs the dialog from 148).
- **Wave I (after 150):** Phase 151 (WS5 PartPlate UI impl — needs the gap-analysis output from 150). May run in parallel with WS3/WS4 waves.
- **Wave J (after 151):** Phase 152 (WS5 save/reload regression — needs the impl from 151).
- **Wave K (last):** Phase 153 (REGRESS-04) — needs all feature phases (141-152).

**Recommended critical-path serial summary:** 141 → 142 → (143 ‖ 144) → 145 → 146 → 147 → 148 → 149 → 153, with 150 → 151 → 152 slotted into the WS3/WS4 windows. The 5-workstream ordering WS1 → WS2 → WS5(gap) → WS3 → WS4 → WS5(impl) → REGRESS-04 is honored by the wave structure.

---

### Phase 141: v4.x Tech-Debt Closure

**Status:** Not started
**Workstream:** WS1 (Tech-Debt)
**Goal:** Close the four small code-only items carried out of v4.8 (CGAL-02 true intersection, orphaned `meshBooleanSelected` menu stub, `drillObject` C4715, ASM rotate/scale live-visual compose) and seed the v5.0 regression lock that later workstreams will append to.
**Depends on:** —
**UI hint:** no (pure C++ + one QML menu-item deletion; no new QML surfaces)
**Requirements:** DEBT-01, DEBT-02, DEBT-03, DEBT-04, DEBT-05

Success criteria:
1. Selecting two overlapping volumes and choosing the intersection boolean op produces the true A∩B volume (not A−B), AND the tool object is NOT deleted after intersection (the current code deletes the tool for all ops — semantically wrong for intersection). The user-visible boolean menu exposes intersection with correct results.
2. The orphaned "网格布尔运算" context menu item in `PreparePage.qml` that calls the `meshBooleanSelected()` stub is removed (or repointed to the working `booleanExecute` path) with no dead UI remaining — `grep` for `meshBooleanSelected` returns zero QML callers.
3. `ProjectServiceMock::drillObject` returns `true` on its success path; the canonical build compiles with ZERO C4715 warnings on `drillObject` (build log is grep-clean for `C4715.*drillObject`).
4. Rotate/scale gizmo drags on AssembleView compose into the live canvas preview immediately (full translate+rotate+scale matrix sourced from `ModelInstance::m_assemble_transformation` in `RhiViewportRenderer::buildModelVertices`); Prepare and Preview canvases are byte-for-byte unaffected (existing PrepareScene/Preview regression tests still pass).
5. A new `v50TechDebtRegressionLocked` source-audit slot in `QmlUiAuditTests` asserts all DEBT-01..04 anchors AND re-asserts the v4.8/v4.7/v4.6 anchors; canonical build (`scripts/auto_verify_with_vcvars.ps1`) exit 0 and 5/5 ctest groups PASS.

---

### Phase 142: OpenVDB CMake Unlock And libslic3r Link

**Status:** Not started
**Workstream:** WS2 (OpenVDB)
**Goal:** Correct the v4.x "OpenVDB unavailable" premise at the build layer: invoke `find_package(OpenVDB)` in the root CMakeLists, create the `openvdb_libs` alias target that no source file ever created (the root cause of the "unavailable" premise), and activate `OpenVDBUtils.cpp` in the libslic3r-from-source build so `mesh_to_grid`/`grid_to_mesh`/`redistance_grid` symbols resolve at exe link.
**Depends on:** —
**UI hint:** no (CMake-only)
**Requirements:** VDB-01, VDB-02

Success criteria:
1. Root `CMakeLists.txt` invokes `find_package(OpenVDB 5.0 COMPONENTS openvdb)` using the bundled `FindOpenVDB.cmake` (already present at `${DEPS_PREFIX}/lib/cmake/OpenVDB/` and `third_party/OrcaSlicer/cmake/modules/`); `OPENVDB_USE_STATIC_LIBS=ON` and `USE_BLOSC=TRUE` are set; an alias target `openvdb_libs` is created from `OpenVDB::openvdb` so the existing `if(TARGET openvdb_libs)` branches in `BuildLibslic3rFromSource.cmake:366,718` activate; `IlmBase::Half` / `Blosc::blosc` are remapped per upstream `OrcaSlicer/CMakeLists.txt:754-767`.
2. `libslic3r_from_source` compiles `OpenVDBUtils.cpp` (previously excluded by `BuildLibslic3rFromSource.cmake:374`); `target_link_libraries(... PUBLIC openvdb_libs)` executes; the `mesh_to_grid` / `grid_to_mesh` / `redistance_grid` symbols are present in the static lib AND resolved at exe link (the build log is grep-clean for `LNK2019.*mesh_to_grid` / `LNK2019.*grid_to_mesh` / `LNK2019.*redistance_grid`).
3. The canonical build (`scripts/auto_verify_with_vcvars.ps1`) exits 0 with OpenVDB linked; the existing 5/5 ctest groups still PASS (no regression from the new link line); the executable launches and reaches the Prepare page without a missing-DLL or unresolved-symbol dialog.

---

### Phase 143: Hollow Gizmo Availability + Button + Panel + SLA Slice

**Status:** Not started
**Workstream:** WS2 (OpenVDB/Hollow)
**Goal:** Make the Hollow gizmo reachable (availability switch true on SLA printers), visible (tool button + panel), and functional (produces an interior cavity + drain holes when sliced via an SLA profile). The Q_PROPERTY parameters already exist at `EditorViewModel.h:326-340,590-595`.
**Depends on:** Phase 142 (needs the linked OpenVDB symbols + activated `OpenVDBUtils.cpp` for the SLA slice path)
**UI hint:** yes (new Hollow tool button in `GLToolbars.qml` + new Hollow gizmo panel — `ui_safety_gate` applies)
**Requirements:** VDB-03, VDB-04, VDB-05, VDB-06

Success criteria:
1. When an SLA-style printer profile is selected, the Hollow gizmo reports as available (`EditorViewModel.cpp:5986-5990` case 8 returns `true`), the tooltip blocker at `EditorViewModel.cpp:6028-6030` ("Blocked: OpenVDB unavailable") is removed, and the Hollow bit in `availableGizmoMask` flips on for SLA printers; non-SLA printers keep Hollow unavailable (the existing behavior for non-SLA is preserved).
2. A Hollow tool button is added to `GLToolbars.qml` (currently zero matches for "hollow"); it is visible ONLY when an SLA printer is selected AND exactly one object is selected; clicking it enters Hollow gizmo mode (verified by `activeGizmo` transitioning to the Hollow mode id).
3. A minimal Hollow gizmo panel surfaces the existing Q_PROPERTY parameters (`hollowEnabled` / `hollowHoleRadius` / `hollowHoleHeight` / `hollowOffset` / `hollowQuality` / `hollowClosingDistance`); editing any parameter updates the model, marks the preset dirty, and triggers re-slice on SLA printers; the parameters round-trip through 3MF save→reload (an automated test asserts save→reload preserves all six values).
4. Slicing a solid cube with `hollowEnabled=true` via an SLA printer profile produces a non-solid G-code with a visible interior cavity + drain holes (libslic3r `SLA/Hollowing.cpp` is invoked through the Print path, backed by the `OpenVDBUtils` `mesh_to_grid`/`grid_to_mesh`/`redistance_grid` symbols); with `hollowEnabled=false` the same cube produces solid G-code. Verified by a ctest that asserts hollowed output differs from solid output.
5. Canonical build exit 0 + 5/5 ctest groups PASS; non-SLA Prepare/Preview behavior is byte-for-byte unaffected (existing regression slots pass).

---

### Phase 144: Emboss Font Loading And Text2Shapes Extrude

**Status:** Not started
**Workstream:** WS3 (Emboss)
**Goal:** Replace the synchronous mock in `addTextObject` with real font loading + upstream `Emboss::text2shapes` extrusion, producing a real `TextEmboss`-typed volume with geometry (not a placeholder). This is the foundation phase that the async pipeline (145) and panel (145) build on.
**Depends on:** Phase 142 (the Emboss pipeline leans on the same libslic3r surface-area expansion pattern that the OpenVDB link proves; `Emboss.cpp` is part of libslic3r and benefits from the same link hygiene established in 142)
**UI hint:** no (foundation is C++ service + libslic3r wiring; panel comes in 145)
**Requirements:** EMB-01, EMB-02

Success criteria:
1. A Qt6 `EmbossStyleManager` C++ service ports upstream font enumeration + caching; system fonts AND bundled fonts enumerate and select; the synchronous mock in `EditorViewModel::addTextObject` is replaced by a real call through `ProjectServiceMock::addTextVolume` that uses the style manager.
2. Text-to-shape conversion invokes upstream `Emboss::text2shapes` (`libslic3r/Emboss.cpp`); shapes are extruded to 3D with user-controlled `embossHeight`/`embossDepth` (`EditorViewModel.h:609-612`); the result is a real `TextEmboss`-typed volume (`MockVolumeType::TextEmboss=5`) with non-empty geometry (a ctest asserts the produced volume has triangle count > 0 and the expected text bounding-box height).
3. Font selection persists for the volume (font family + size are stored on the volume); changing the font after creation re-extrudes to the new glyph set.
4. Canonical build exit 0 + 5/5 ctest groups PASS; no LNK errors on Emboss symbols (proves the libslic3r surface-area expansion pattern from Phase 142).

---

### Phase 145: Async EmbossJob And Gizmo Panel

**Status:** Not started
**Workstream:** WS3 (Emboss)
**Goal:** Port the upstream async `EmbossJob` pipeline to Qt6 via Qt Concurrent so text edits re-extrude on a worker thread without blocking the UI, and surface an editable Emboss gizmo panel.
**Depends on:** Phase 144 (needs the extrude pipeline + style manager)
**UI hint:** yes (new Emboss gizmo panel — `ui_safety_gate` applies)
**Requirements:** EMB-03, EMB-04

Success criteria:
1. Text edits re-extrude on a Qt Concurrent worker thread (the UI thread does not stall while the glyph tessellation runs); the result is delivered via a signal and merged into the model on the GUI thread; cancellation works — typing fast does not pile up stale jobs (a ctest asserts the latest edit's geometry matches the final text and no stale merge races).
2. An Emboss gizmo panel surfaces text input + font selector + size/height/depth/style controls + depth modifier + use-surface option + curve-projection option, mapped to upstream `GLGizmoEmboss` panel layout; editing the panel on a selected TextEmboss volume round-trips through the model (changes are reflected in the next render and survive a panel close/reopen in the same session).
3. The UI stays responsive during a long extrude (e.g., a 200-character text volume) — the panel does not freeze and other gizmos remain operable while the worker runs.
4. Canonical build exit 0 + 5/5 ctest groups PASS.

---

### Phase 146: Emboss Wiring, 3MF Round-Trip, And SVG Path

**Status:** Not started
**Workstream:** WS3 (Emboss)
**Goal:** Wire the existing `GLToolbars.qml:281` Emboss button and the context-menu Add Text / Add SVG paths into the real gizmo, persist TextEmboss volumes through 3MF, and add the SVG emboss path.
**Depends on:** Phase 145 (needs the panel + async job)
**UI hint:** yes (button behavior + SVG file picker — `ui_safety_gate` applies)
**Requirements:** EMB-05, EMB-06, EMB-07

Success criteria:
1. Clicking the existing `GLToolbars.qml:281` Emboss button enters the gizmo; if no object is selected, clicking creates a new TextEmboss volume at the canvas center; if a TextEmboss volume is selected, the panel edits it (verified by an automated click-through test or a deterministic argv fixture). Context-menu "Add Text" uses the same pipeline (`EditorViewModel::addTextObject`).
2. Emboss volumes persist + round-trip via 3MF — TextEmboss type, text content, font, size, and depth survive save→reload; the reloaded volume is editable with identical geometry. A `testEmbossRoundTrip` ctest asserts geometry + metadata match.
3. The SVG emboss path (`GizmoSVG` button, `EditorViewModel::addSvgObject`) loads an SVG file and extrudes its paths via the same Emboss pipeline (mapped to upstream `GLGizmoEmboss` SVG branch); the resulting volume is a `TextEmboss`-typed (or SVG-typed) volume with non-empty geometry.
4. Canonical build exit 0 + 5/5 ctest groups PASS.

---

### Phase 147: Preset Bundle INI Format And CreatePresetsDialog

**Status:** Not started
**Workstream:** WS4 (Preset Bundle)
**Goal:** Replace the simplified JSON approximation in `PresetServiceMock::exportBundle`/`importBundle` with upstream-compatible `.ini`-based PresetBundle metadata, and port a minimal source-truth `CreatePresetsDialog`.
**Depends on:** —
**UI hint:** yes (new CreatePresetsDialog — `ui_safety_gate` applies)
**Requirements:** PSET-01, PSET-02

Success criteria:
1. `exportBundle`/`importBundle` write/read an upstream-compatible `.ini`-based PresetBundle format (in addition to or replacing the JSON approximation); bundle metadata (`vendor`, `version`, `compatible_printers`, inheritance chain) matches upstream `PresetBundle` semantics; a round-trip ctest asserts export → reimport yields an identical preset set (names, values, inheritance).
2. A minimal `CreatePresetsDialog` ported from upstream `CreatePresetsDialog.*` lets the user create a new user preset by: selecting a base (inherits), choosing scope (printer/process/material), naming, and saving; the created preset appears in the corresponding PresetComboBox and persists across restarts (a ctest restarts the service and asserts the preset is still present).
3. The JSON approximation is removed OR explicitly classified as fallback per the PROJECT.md Preset Rule (no dead dual-path code remains).
4. Canonical build exit 0 + 5/5 ctest groups PASS.

---

### Phase 148: UnsavedChangesDialog 3-Way Diff And Simple/Advanced Filter

**Status:** Not started
**Workstream:** WS4 (Preset Bundle)
**Goal:** Port the upstream `UnsavedChangesDialog` 3-way diff UI and move the Simple/Advanced option filter from QML into C++ as a typed behavior.
**Depends on:** Phase 147 (needs the bundle format to express the dirty state and inheritance for the 3-way diff)
**UI hint:** yes (UnsavedChangesDialog + filter toggle — `ui_safety_gate` applies)
**Requirements:** PSET-03, PSET-04

Success criteria:
1. A 3-way `UnsavedChangesDialog` ported from upstream `UnsavedChangesDialog.*` offers Keep / Discard / Save-As / Cancel when the user switches preset/page/scope with `isPresetDirty==true` (`ConfigViewModel.h:34`); the existing `pendingUnsavedAction`/`pendingUnsavedTarget` infrastructure (`ConfigViewModel.h:51-53`) backs the dialog (no parallel dirty-state machine is created).
2. Simple/Advanced filtering is implemented in C++ (`ConfigViewModel` or `PresetServiceMock`) — upstream's filter rule (which options show in Simple mode) is a typed behavior; toggling the filter re-renders the option list without changing values (a ctest asserts values are unchanged across a Simple↔Advanced toggle and that the Simple set is a subset of the Advanced set).
3. The dialog correctly distinguishes a close-window flow from a preset-switch flow (no false prompts on clean transitions; always prompts on dirty transitions); verified by an automated test that drives both flows.
4. Canonical build exit 0 + 5/5 ctest groups PASS.

---

### Phase 149: Compare/Diff, Dirty Propagation, And Bundle Round-Trip

**Status:** Not started
**Workstream:** WS4 (Preset Bundle)
**Goal:** Port the side-by-side Compare/Diff preset flow, make dirty-state propagation consistent across all page/preset/scope transitions, and lock the bundle round-trip with a source-audit slot.
**Depends on:** Phase 148 (needs the diff infrastructure from the 3-way dialog)
**UI hint:** yes (Compare/Diff view reachable from settings sidebar — `ui_safety_gate` applies)
**Requirements:** PSET-05, PSET-06, PSET-07

Success criteria:
1. A Compare/Diff preset flow lets the user select two presets and see a side-by-side diff of differing options (mapped to upstream `UnsavedChangesDialog` diff view mode); reachable from the settings sidebar; a ctest asserts the diff correctly identifies changed/added/removed keys between two fixtures.
2. Dirty-state propagation is consistent across page switches (Prepare → Preview → Settings), preset switches, and scope switches (Global/Object/Plate); no silent loss of unsaved changes — every transition through dirty state either prompts or auto-saves per upstream behavior (a ctest drives each transition with a dirty model and asserts the prompt/auto-save fires).
3. A preset bundle round-trip ctest: create user preset → edit → save → export bundle → restart (re-init service) → import bundle → assert preset set + values + inheritance match; locked by a source-audit slot in `QmlUiAuditTests` so the round-trip cannot silently regress.
4. Canonical build exit 0 + 5/5 ctest groups PASS.

---

### Phase 150: PartPlate UI Gap Analysis

**Status:** Not started
**Workstream:** WS5 (PartPlate)
**Goal:** Read-only enumeration of the concrete missing UI affordances in the PlateBar UI vs upstream `PartPlate.cpp` user-facing behavior. The output (`.planning/research/partplate-ui-gap.md`) refines PLATE-02..06 implementation scope in Phase 151.
**Depends on:** —
**UI hint:** no (read-only research artifact)
**Requirements:** PLATE-01

Success criteria:
1. `.planning/research/partplate-ui-gap.md` exists and lists, for each upstream `PartPlate.cpp` user-facing behavior, whether the Qt6 PlateBar UI (`PreparePage.qml:3356+`) implements it; missing items are concrete (specific UI element + upstream source line + Qt6 target).
2. The gap analysis explicitly maps each of PLATE-02..06 (reorder, per-plate print-sequence dialog, per-plate config override editor, non-current-plate thumbnails, save/reload regression) to either "implement as specified" or "refined scope: <details>", so Phase 151 has no ambiguity.
3. The analysis is read-only — no source files outside `.planning/research/` are modified; canonical build is unaffected (still exit 0, 5/5 ctest).

---

### Phase 151: PartPlate UI Implementation

**Status:** Not started
**Workstream:** WS5 (PartPlate)
**Goal:** Implement the PLATE-02..05 affordances as refined by the Phase 150 gap analysis: plate reorder UI, per-plate print-sequence dialog, per-plate config override editing at Plate scope, and real thumbnails for non-current plates.
**Depends on:** Phase 150 (needs the refined scope from the gap analysis)
**UI hint:** yes (PlateBar affordances + per-plate dialog surfaces — `ui_safety_gate` applies)
**Requirements:** PLATE-02, PLATE-03, PLATE-04, PLATE-05

Success criteria:
1. Plate reorder UI is present in PlateBar (drag-to-reorder and/or explicit move-left/move-right); reorder persists through 3MF round-trip (a ctest asserts plate order survives save→reload).
2. A per-plate print-sequence dialog is surfaced from a plate context action; the dialog binds to the existing `platePrintSequence` / `PlatePrintSequence` enum (`PartPlate.h:47-119`); the selection round-trips through 3MF.
3. Per-plate config override is editable through the settings dialog when scope=Plate (existing `activatePlateScope`/`requestPlateScope` at `ConfigViewModel.h:129-134`); editing a plate-scoped option writes to `PartPlate::m_config` (`PartPlate.h:316`) via the QVariant adapter, not just the global preset (a ctest asserts the override is stored on the plate and survives a global preset change).
4. Non-current plates show a real thumbnail in PlateBar (either cached from 3MF via `plateThumbnailBase64` at `ProjectServiceMock.h:331-375` or a mini-render); a ctest or runtime check asserts PlateBar cards for non-current plates render a non-empty thumbnail image.
5. Canonical build exit 0 + 5/5 ctest groups PASS; global-scope settings editing is unaffected (no regression).

---

### Phase 152: PartPlate Multi-Plate Save/Reload Regression

**Status:** Not started
**Workstream:** WS5 (PartPlate)
**Goal:** Lock full plate state round-trip through 3MF with a source-audit slot so the PLATE-02..05 work cannot silently regress.
**Depends on:** Phase 151 (needs the implemented UI + data paths to assert against)
**UI hint:** no (test-only)
**Requirements:** PLATE-06

Success criteria:
1. A multi-plate save/reload regression ctest asserts the full plate state — count, names, per-plate config overrides, print sequence, bed type, locked/printable flags, and thumbnails — round-trips through 3MF; the test uses a multi-plate fixture (≥3 plates with at least one override and one non-default print sequence).
2. The regression is locked by a source-audit slot in `QmlUiAuditTests` (or `ViewModelSmokeTests`) so a future change that breaks any of the round-tripped fields fails the build.
3. Canonical build (`scripts/auto_verify_with_vcvars.ps1`) exit 0 + 5/5 ctest groups PASS.

---

### Phase 153: v5.0 Cross-Workstream Regression Gate

**Status:** Not started
**Workstream:** Cross-cutting (REGRESS-04)
**Goal:** Consolidated v5.0 regression gate that re-asserts all v5.0 anchors (DEBT-01..05, VDB-01..06, EMB-01..07, PSET-01..07, PLATE-01..06) AND re-asserts the v4.8/v4.7/v4.6 anchors.
**Depends on:** Phases 141-152 (all feature phases)
**UI hint:** no (test-only)
**Requirements:** REGRESS-04

Success criteria:
1. A consolidated `v50RegressionLocked` source-audit slot (in `QmlUiAuditTests` and/or `ViewModelSmokeTests`) re-asserts every v5.0 anchor from Phases 141-152; a single anchor regression fails the build.
2. The slot ALSO re-asserts the v4.8 (`v48CrossWorkstreamRegressionLocked`), v4.7, and v4.6 anchors — no regression introduced into earlier milestones by the v5.0 work.
3. Canonical build (`scripts/auto_verify_with_vcvars.ps1`) exits 0; 5/5 ctest groups PASS (PrepareScene / PartPlate / ViewModel / UI / PreviewParser); app launch liveness confirmed (the executable reaches the Prepare page without error).
4. The Phase 141 `v50TechDebtRegressionLocked` slot is either merged into the consolidated `v50RegressionLocked` slot or kept and explicitly referenced by it (no orphaned duplicate gate logic).

---

## Next Milestone

Not started. After v5.0 ships, run `/gsd:new-milestone` to define the next cycle. Candidate backlog: de/fr/ja/ko translation long tail; calibration `.drc` tower geometry; FaceDetector/SlaSupports gizmos (downstream of WS2 OpenVDB link if it proves stable); Emboss SVG advanced font features (variable fonts, system font enumeration) if WS3 baseline ships clean.

**Archives:** `.planning/milestones/v{version}-ROADMAP.md`, `v{version}-REQUIREMENTS.md`, `v{version}-MILESTONE-AUDIT.md`, `v{version}-phases/`.

---

*Last updated: 2026-07-17 via `/gsd:roadmapper` — v5.0 roadmap created (13 phases, 141-153, 32 requirements mapped, 0 unmapped).*
