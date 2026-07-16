# Requirements: OWzx Slicer — OrcaSlicer Qt6/QML Migration

**Defined:** 2026-07-17
**Core Value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## v5.0 Requirements — Advanced Feature Recovery & Tech-Debt Closure

**Milestone goal:** In one cycle, close the accumulated tech debt across v4.6/v4.7/v4.8 AND recover three long-blocked P1 feature clusters (Emboss / Preset Bundle / PartPlate), plus correct the mistaken "OpenVDB unavailable" premise by linking OpenVDB and unlocking the Hollow gizmo.

**Scope rule:** All work maps to OrcaSlicer v7.0.1 upstream. The "OpenVDB unavailable" constraint is **revised** — OpenVDB IS present in DEPS_PREFIX (`libopenvdb.lib` 55MB + `FindOpenVDB.cmake`); the v4.x "unavailable" premise was an incomplete CMake port. LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows remain removed.

**Phases start at 141** (continuing from v4.8 phase 140).

---

### WS1 — Tech-Debt Closure

Carry-forward from v4.8 audit. All four items are code-solvable and small.

- [ ] **DEBT-01**: Boolean intersection (op==2) on two overlapping volumes returns the true A∩B volume (not A−B). `ProjectServiceMock.cpp:3152-3156` swaps `MeshBoolean::minus` → `MeshBoolean::intersect` (upstream API already exists at `MeshBoolean.hpp:54,58,62`); tool object is NOT deleted after intersection (current code deletes tool for all ops — semantically wrong for intersection).
- [ ] **DEBT-02**: Orphaned "网格布尔运算" CxMenuItem (`PreparePage.qml:401-406`) that calls `EditorViewModel::meshBooleanSelected()` stub (`EditorViewModel.cpp:4538-4543`, returns false + qWarning "not yet implemented") is removed OR repointed to the working `booleanExecute` path. No dead UI remains.
- [ ] **DEBT-03**: `ProjectServiceMock::drillObject` returns `true` on the success path (fix MSVC C4715 — currently the `try{}` success block at `ProjectServiceMock.cpp:3242-3362` has no `return true;`, falling off the end). Canonical build compiles with zero C4715 warnings on `drillObject`.
- [ ] **DEBT-04**: Assembly-mode rotate/scale transform composes into the live canvas render. `RhiViewportRenderer::buildModelVertices` (`RhiViewportRenderer.cpp:1615-1743`, currently translate-only at lines 1718-1740) applies the full assemble transform matrix (translate + rotate + scale sourced from `ModelInstance::m_assemble_transformation`). Rotate/scale gizmo drags on AssembleView reflect in the on-canvas preview immediately, not only after reload. Prepare/Preview canvases byte-for-byte unaffected.
- [ ] **DEBT-05**: v5.0 regression lock added to `QmlUiAuditTests` (`v50TechDebtRegressionLocked` slot) asserts all DEBT-01..04 anchors AND re-asserts v4.8/v4.7/v4.6 anchors. Canonical build exit 0 + 5/5 ctest PASS.

---

### WS2 — OpenVDB Unlock & Hollow Gizmo

Corrects the v4.x mistaken "OpenVDB unavailable" premise. OpenVDB is built and present in DEPS_PREFIX; only the Qt6 CMake port is incomplete.

- [ ] **VDB-01**: Root `CMakeLists.txt` invokes `find_package(OpenVDB 5.0 COMPONENTS openvdb)` using the bundled `FindOpenVDB.cmake` (already present at `${DEPS_PREFIX}/lib/cmake/OpenVDB/` and `third_party/OrcaSlicer/cmake/modules/`). `OPENVDB_USE_STATIC_LIBS=ON`, `USE_BLOSC=TRUE`. An alias target `openvdb_libs` is created from `OpenVDB::openvdb` so existing `if(TARGET openvdb_libs)` branches in `BuildLibslic3rFromSource.cmake:366,718` activate. `IlmBase::Half` / `Blosc::blosc` remapped per upstream `OrcaSlicer/CMakeLists.txt:754-767`.
- [ ] **VDB-02**: `libslic3r_from_source` now compiles `OpenVDBUtils.cpp` (previously excluded by `BuildLibslic3rFromSource.cmake:374`); `target_link_libraries(... PUBLIC openvdb_libs)` executes. `mesh_to_grid` / `grid_to_mesh` / `redistance_grid` symbols are present in the static lib AND resolved at exe link (no LNK2019).
- [ ] **VDB-03**: `EditorViewModel` gizmo-availability switch (`EditorViewModel.cpp:5986-5990` case 8 Hollow) returns `true` when an SLA-style printer is selected (was unconditionally `false`). Tooltip blocker at `EditorViewModel.cpp:6028-6030` ("Blocked: OpenVDB unavailable") is removed. The Hollow bit in `availableGizmoMask` flips on for SLA printers.
- [ ] **VDB-04**: A Hollow tool button is added to `GLToolbars.qml` (currently zero matches for "hollow" — confirmed by exploration). Visible only when an SLA printer is selected AND exactly one object is selected. Clicking it enters Hollow gizmo mode.
- [ ] **VDB-05**: A minimal Hollow gizmo panel surfaces the existing Q_PROPERTY parameters (`hollowEnabled` / `hollowHoleRadius` / `hollowHoleHeight` / `hollowOffset` / `hollowQuality` / `hollowClosingDistance` already declared at `EditorViewModel.h:326-340,590-595` + setters at `EditorViewModel.cpp:1308-1341`). Editing any parameter updates the model and triggers re-slice on SLA printers. Parameters round-trip through 3MF save→reload.
- [ ] **VDB-06**: SLA hollowing actually produces an interior cavity + drain holes when sliced via an SLA printer profile (`SLA/Hollowing.cpp` is invoked through the Print path). The hollowed mesh is rendered in the Preview. (Acceptance: a solid cube sliced with `hollowEnabled=true` produces a non-solid G-code with visible cavity; with `hollowEnabled=false` produces solid G-code.)

---

### WS3 — Emboss Text Gizmo (Full Port)

Port upstream `GLGizmoEmboss.cpp` (3811 lines) end-to-end. Largest workstream.

- [ ] **EMB-01**: Real font loading replaces the synchronous mock in `EditorViewModel::addTextObject` (`EditorViewModel.cpp:1726-1751`) → `ProjectServiceMock::addTextVolume` (`ProjectServiceMock.h:244`, `ProjectServiceMock.cpp:2494,2513`). Upstream `EmbossStyleManager` (font enumeration + caching) is ported to Qt6 as a C++ service; system + bundled fonts enumerate and select.
- [ ] **EMB-02**: Text-to-shape conversion via upstream `Emboss::text2shapes` (`libslic3r/Emboss.cpp`) is invoked; shapes are extruded to 3D with user-controlled height/depth (`embossHeight`/`embossDepth` Q_PROPERTYs at `EditorViewModel.h:609-612`). Result is a real `TextEmboss`-typed volume (`MockVolumeType::TextEmboss=5`) with geometry, not a placeholder.
- [ ] **EMB-03**: Async `EmbossJob` pipeline (`GUI/Jobs/EmbossJob.cpp`) is ported to Qt6 via Qt Concurrent — text edits re-extrude on a worker thread without blocking the UI; result is delivered via a signal and merged into the model on the GUI thread. Cancellation works (typing fast does not pile up stale jobs).
- [ ] **EMB-04**: Emboss gizmo panel surfaces text input + font selector + size/height/depth/style controls + depth modifier + use surface option + curve projection option. Editable on a selected TextEmboss volume (round-trips via the model). UI mapped to upstream `GLGizmoEmboss` panel layout.
- [ ] **EMB-05**: The existing `GLToolbars.qml:281` Emboss button enters the gizmo on click; if no object is selected, clicking creates a new TextEmboss volume at the canvas center; if a TextEmboss volume is selected, the panel edits it. Context-menu "Add Text" / "Add SVG" paths (`EditorViewModel::addTextObject`, `addSvgObject`) use the same pipeline.
- [ ] **EMB-06**: Emboss volumes persist + round-trip via 3MF (TextEmboss type, text content, font, size, depth). Save→reload restores the editable TextEmboss volume with identical geometry. Ctest: `testEmbossRoundTrip` asserts geometry + metadata match.
- [ ] **EMB-07**: SVG emboss path (`GizmoSVG` button, `addSvgObject`) loads an SVG file and extrudes its paths via the same Emboss pipeline (mapped to upstream `GLGizmoEmboss` SVG branch).

---

### WS4 — Preset Bundle Full Chain

Replace simplified JSON with upstream-compatible bundle behavior + port the two missing dialogs.

- [ ] **PSET-01**: `PresetServiceMock::exportBundle` / `importBundle` (`PresetServiceMock.cpp:835-887, 889+`) write/read upstream-compatible `.ini`-based PresetBundle format in addition to (or replacing) the current JSON approximation. Bundle metadata (`vendor`, `version`, `compatible_printers`, inheritance chain) matches upstream `PresetBundle` semantics. Round-trip test: export → reimport yields identical preset set.
- [ ] **PSET-02**: Minimal source-truth `CreatePresetsDialog` is ported from upstream `CreatePresetsDialog.*`. User can create a new user preset by: selecting a base (inherits), choosing scope (printer/process/material), naming, and saving. Created preset appears in the corresponding PresetComboBox and persists across restarts.
- [ ] **PSET-03**: `UnsavedChangesDialog` 3-way diff UI is ported from upstream `UnsavedChangesDialog.*`. When the user switches preset/page/scope with `isPresetDirty==true` (`ConfigViewModel.h:34`), the dialog offers Keep / Discard / Save-As / Cancel. The existing `pendingUnsavedAction` / `pendingUnsavedTarget` infrastructure (`ConfigViewModel.h:51-53`) backs the dialog.
- [ ] **PSET-04**: Simple/Advanced filtering is implemented in C++ (`ConfigViewModel` or `PresetServiceMock`), not QML — upstream's filter rule (which options show in Simple mode) is a typed behavior. Toggling the filter re-renders the option list without changing values.
- [ ] **PSET-05**: Compare/Diff preset flow is ported — user can select two presets and see a side-by-side diff of differing options (mapped to upstream `UnsavedChangesDialog` diff view mode). Reachable from the settings sidebar.
- [ ] **PSET-06**: Preset bundle round-trip ctest: create user preset → edit → save → export bundle → restart (re-init service) → import bundle → assert preset set + values + inheritance match. Locked by a source-audit slot.
- [ ] **PSET-07**: Dirty-state propagation is consistent across page switches (Prepare → Preview → Settings), preset switches, and scope switches (Global/Object/Plate). No silent loss of unsaved changes; every transition through dirty state either prompts or auto-saves per upstream behavior.

---

### WS5 — PartPlate Multi-Plate UI Completion

Data model is already mature (`PartPlate` class with `DynamicPrintConfig m_config`, 30+ Q_INVOKABLEs, 3MF staging). This WS completes the UI + remaining UX gaps. Specific REQ-IDs may consolidate after phase-1 gap analysis.

- [ ] **PLATE-01**: A PartPlate UI gap analysis is performed (read-only) before implementation: enumerate exactly what's missing in the PlateBar UI (`PreparePage.qml:3356+`) vs upstream `PartPlate.cpp` user-facing behavior. Output: `.planning/research/partplate-ui-gap.md` listing concrete missing UI items. Drives PLATE-02..06 refinement.
- [ ] **PLATE-02**: Plate reorder UI (drag-to-reorder in PlateBar, or explicit move-left/move-right action) — currently data has order but no UI affordance. Reorder persists through 3MF round-trip.
- [ ] **PLATE-03**: Per-plate print sequence dialog (data exists via `platePrintSequence` / `PlatePrintSequence` enum at `PartPlate.h:47-119`; UI may be partial at `PreparePage.qml:1387` `addPlateOtherLayersSeqEntry`). Dialog surfaced from plate context action; selection round-trips.
- [ ] **PLATE-04**: Per-plate config override is editable through the settings dialog when scope=Plate (existing `activatePlateScope` / `requestPlateScope` at `ConfigViewModel.h:129-134`). Editing a plate-scoped option writes to `PartPlate::m_config` (`PartPlate.h:316`) via the QVariant adapter (`ProjectServiceMock.h:544-548 m_mockPlateOverrides`), not just the global preset.
- [ ] **PLATE-05**: Plate thumbnail is rendered for non-current plates (currently only current plate renders live). Either cached thumbnail from 3MF (`plateThumbnailBase64` at `ProjectServiceMock.h:331-375`) or a mini-render. PlateBar cards show real thumbnails.
- [ ] **PLATE-06**: Multi-plate save/reload regression ctest asserts full plate state (count, names, per-plate config overrides, print sequence, bed type, locked/printable flags, thumbnails) round-trips through 3MF. Locked by a source-audit slot.

---

### Cross-Workstream

- [ ] **REGRESS-04**: v5.0 cross-workstream regression gate. A consolidated `v50RegressionLocked` source-audit slot re-asserts all v5.0 anchors (DEBT-01..05, VDB-01..06, EMB-01..07, PSET-01..07, PLATE-01..06) AND re-asserts v4.8/v4.7/v4.6 anchors. Canonical build (`scripts/auto_verify_with_vcvars.ps1`) exit 0 + 5/5 ctest groups PASS + app launch liveness confirmed.

---

## v2 Requirements (Future / Deferred)

Not in v5.0 roadmap; tracked for future promotion.

### Translation Content

- **I18N-06**: de/fr/ja/ko long tail (~906 messages/lang remaining after v4.8 I18N-05). Non-code translation work; separate workflow.
- **I18N-07**: Additional languages (zh_TW / es / pt / ru / it / tr / uk per upstream).

### Advanced Gizmos (downstream of OpenVDB)

- **SLA-01**: FaceDetector gizmo — unlock if WS2 OpenVDB link proves stable.
- **SLA-02**: SlaSupports gizmo — full SLA support tree generation; depends on OpenVDB + SLA support algorithm port.

### Emboss Extensions

- **EMB-F01**: Variable font axes (weight/width/slant).
- **EMB-F02**: System font enumeration + font installation UI.
- **EMB-F03**: Per-line/per-word styling within a single text volume.

### Calibration Geometry

- **CALIB-F01**: Dedicated `.drc` calibration tower model loading (resources/calib packaging + CalibUtils port). Currently uses current-plate model (v4.6 tech debt).

### Rendering

- **RENDER-F01**: D3D12 default-backend promotion (deferred from v4.5; needs confirmed root cause + clean repro on the original machine).
- **RENDER-F02**: Vulkan default-backend evaluation (blocked: current Qt SDK lists vulkan under `QT_DISABLED_PUBLIC_FEATURES`).

### Preset Extensions

- **PSET-F01**: Preset bundle marketplace / cloud sync (out of scope under cloud-removal rule unless user reopens).

---

## Out of Scope

Explicitly excluded from v5.0. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| LAN device discovery, device send/upload, MQTT/SSDP | Removed from forward scope by user direction 2026-07-07. |
| Cloud print, cloud account/sync, OAuth | Removed from forward scope by user direction 2026-07-07. |
| Monitor task lifecycle, ModelMall/Home WebView | Removed from forward scope by user direction 2026-07-07. |
| Live camera / RTSP / WebRTC streams (FFmpeg/MetaRTC) | Removed from forward scope; FFmpeg/WebRTC deps unavailable. |
| Printer-connected hardware workflows (ManualLeveling/BedLeveling/Vibration calibration) | Hardware-dependent; stays out under printer-hardware removal rule. |
| Paho MQTT device communication migration | Removed scope (no device network). |
| libslic3r slicing algorithm changes | Out of scope — GUI migration only, libslic3r preserved. |
| CGAL 5.4→5.6+ dependency-bundle upgrade | v4.8 confirmed MeshBoolean works on 5.4; not needed for v5.0. The 2-line CGAL submodule compat patch from v4.8 stays (droppable later if convenient). |
| New build directories or non-canonical build scripts | Build rules: only `build/` + `scripts/auto_verify_with_vcvars.ps1`. |
| Product behavior without upstream mapping | Core value: source truth. |
| D3D12 or Vulkan as default backend | Blocked per RENDER-F01/F02; QRhi/D3D11 stays default. |

---

## Traceability

Populated by `gsd-roadmapper` during ROADMAP.md creation. Each v5.0 requirement maps to exactly one phase (phases 141-153).

| Requirement | Phase | Status |
|-------------|-------|--------|
| DEBT-01 | 141 | Complete |
| DEBT-02 | 141 | Complete |
| DEBT-03 | 141 | Complete |
| DEBT-04 | 141 | Complete |
| DEBT-05 | 141 | Complete |
| VDB-01 | 142 | Pending |
| VDB-02 | 142 | Pending |
| VDB-03 | 143 | Pending |
| VDB-04 | 143 | Pending |
| VDB-05 | 143 | Pending |
| VDB-06 | 143 | Pending |
| EMB-01 | 144 | Pending |
| EMB-02 | 144 | Pending |
| EMB-03 | 145 | Pending |
| EMB-04 | 145 | Pending |
| EMB-05 | 146 | Pending |
| EMB-06 | 146 | Pending |
| EMB-07 | 146 | Pending |
| PSET-01 | 147 | Pending |
| PSET-02 | 147 | Pending |
| PSET-03 | 148 | Pending |
| PSET-04 | 148 | Pending |
| PSET-05 | 149 | Pending |
| PSET-06 | 149 | Pending |
| PSET-07 | 149 | Pending |
| PLATE-01 | 150 | Pending |
| PLATE-02 | 151 | Pending |
| PLATE-03 | 151 | Pending |
| PLATE-04 | 151 | Pending |
| PLATE-05 | 151 | Pending |
| PLATE-06 | 152 | Pending |
| REGRESS-04 | 153 | Pending |

**Coverage:**
- v5.0 requirements: 32 total (DEBT-01..05, VDB-01..06, EMB-01..07, PSET-01..07, PLATE-01..06, REGRESS-04). NOTE: the previous header text in this file said "33 requirements" — that was an off-by-one miscount; the literal ID list contains 32 unique IDs (5+6+7+7+6+1=32). Corrected by the roadmapper on 2026-07-17. Both PROJECT.md and STATE.md reflect 32.
- Mapped to phases: 32/32 (100%)
- Unmapped: 0
- Orphans (phase with no requirement): none (every phase 141-153 has ≥1 mapped requirement)

---

*Requirements defined: 2026-07-17*
*Last updated: 2026-07-17 after v5.0 roadmap creation (5 workstreams, 32 requirements mapped to 13 phases 141-153, 0 unmapped)*
