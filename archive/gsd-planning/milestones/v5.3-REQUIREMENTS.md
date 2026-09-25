# Requirements: OWzx Slicer — OrcaSlicer Qt6/QML Migration

**Defined:** 2026-07-19 (v5.3)
**Core Value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## v5.3 Requirements — Feature Completion & v5.2 Closure

**Milestone goal:** Three parallel tracks in one cycle — (1) close the v5.2 UI Excellence BLOCKERs that were honestly deferred (destructive confirms + dialog spacing + pseudo-button sweep); (2) ship the 3 functional feature gaps identified by the v5.3 code-level feature-parity audit (per-object settings dialog, object-layer range editor, simplify mesh gizmo); (3) attack the i18n long tail (de/fr/ja/ko).

**Scope rule:** All work is offline/local. No new product behavior without upstream mapping. SLA print path remains DECLINED. LAN/cloud/camera/printer-hardware stay out.

**Phases start at 171** (continuing from v5.2 phase 170).

---

### CL — v5.2 BLOCKER Closure

- [ ] **CL-01**: Every destructive action confirms before firing. v5.2 Phase 169 shipped the ConfirmDialog component but only routed deleteSelection through it. The remaining 11 triggers must be wired: CaliHistoryDialog 清空; HomePage cloudUnbindDevice; MultiMachinePage removeDevice/stopAllLocalTasks/stopAllCloudTasks; MonitorPage disconnectDevice; ObjectList bulk delete; plus any other destructive trigger surfaced by the v5.2 audits.
- [ ] **CL-02**: Dialog spacing sweep — 23 of 24 dialogs use zero `Theme.spacing*` tokens (hand-rolled 0/1/2/3/4/6/8/10/12/14/16/20/24 values). Migrate to the spacing scale via a script-based mechanical sweep (mirroring the Phase 162/163 color/typography pattern).
- [ ] **CL-03**: Pseudo-button sweep — the ~13 true Rectangle+Text+MouseArea pseudo-button candidates identified by v5.2 Phase 168 hand-convert to CxButton/CxIconButton. Each requires site-specific context (signal args, disabled bindings) — hand-converted, not script-converted.

### FEAT — Functional Feature Gaps (from v5.3 parity audit)

- [ ] **FEAT-01**: Per-object settings override dialog (upstream `GUI_ObjectSettings`). Backend APIs exist (`setObjectPrintable`, `setVolumeExtruderId`, scoped overrides, `selectionSettingsRequested` signal). Ship the QML inspector that opens on right-click → shows the object/volume's print/filament params, allows per-object override of layer_height/infill/support/etc. Mirrors upstream `GUI_ObjectSettings::SettingsFactory`.
- [ ] **FEAT-02**: Object layer-range editor (upstream `GUI_ObjectLayers`). Backend is fully ready (`MockLayerRange`, `objectLayerRanges`, `setLayerRangeValue`). Ship the QML dialog to add/edit/delete per-layer-height ranges (e.g. bottom 0.1mm, top 0.2mm) — reachable from the ObjectList context menu. UI-only addition; the data layer is done.
- [ ] **FEAT-03**: Simplify mesh gizmo (upstream `GLGizmoSimplify`). Currently explicit stub at `EditorViewModel.cpp:1355` ("not yet implemented"). Ship the real implementation: call libslic3r's `QuadricEdgeCollapse::quadric_edge_collapse(...)` on the selected volume's mesh, expose the wanted-face-count + max-error params via the existing Q_PROPERTYs, add a SimplifyDialog for preview/apply. Mirrors upstream `GLGizmoSimplify.cpp`.

### I18N — Translation Long Tail

- [ ] **I18N-06**: de/fr/ja/ko translation long tail completion. Baseline ~8641 messages/lang × 4 langs. Many source strings are new since v4.8 (Emboss/PresetBundle/PartPlate/v5.0/v5.1/v5.2 surfaces). Approach: run `lupdate` to refresh the .ts files, then translate via batched LLM-assisted passes (per-language). Target: ≥85% translation coverage per language (vs current ~44%).

### Cross-Workstream

- [ ] **REGRESS-07**: v5.3 regression gate. A `v53RegressionLocked` source-audit slot re-asserts every v5.3 anchor (CL/FEAT/I18N) AND re-asserts the v5.2/v5.1/v5.0/v4.x anchors. Canonical build exits 0 + 5/5 ctest groups PASS.

---

## Deferred / Declined

### SLA print path — DECLINED (user decision 2026-07-19)

**VDB-06 / SLA-01..06 / Hollow 3MF persistence / SlaSupports gizmo / FaceDetector gizmo** — all reclassified as declined-with-reason. See v5.2 REQUIREMENTS for full rationale.

### Future (post-v5.3 candidate backlog)

- Calibration `.drc` tower geometry (v4.6 CALIB tech debt)
- D3D12 default-backend promotion (deferred from v4.5)
- Cmp-03 OptionRow + MoveSlider/PreviewLayerRail unification (deeper refactor)
- XD-02 async Emboss spinner + SliceProgress state coverage
- ConfigWizard depth (multi-stage with vendor profile download)
- AMS material settings real backend (depends on printer-hardware scope decision)
- KBShortcutsDialog / Auxiliary file-tree panel (low-impact)

## Out of Scope

Explicitly excluded from v5.3. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| SLA print path / SlaSupports / FaceDetector | DECLINED — see above |
| LAN device discovery, device send/upload, MQTT/SSDP | Removed from forward scope by user direction 2026-07-07. |
| Cloud print, cloud account/sync, OAuth | Removed from forward scope by user direction 2026-07-07. |
| Monitor task lifecycle, ModelMall/Home WebView | Removed from forward scope by user direction 2026-07-07. |
| Live camera / RTSP / WebRTC streams | Removed from forward scope; FFmpeg/WebRTC deps unavailable. |
| Printer-connected hardware workflows | Hardware-dependent; stays out under printer-hardware removal rule. |
| libslic3r slicing algorithm changes | Out of scope — GUI migration only. |
| New build directories or non-canonical build scripts | Build rules: only `build/` + `scripts/auto_verify_with_vcvars.ps1`. |

## Traceability

Populated during ROADMAP.md creation.
