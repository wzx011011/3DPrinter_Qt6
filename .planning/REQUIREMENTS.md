# Requirements: OWzx Slicer — OrcaSlicer Qt6/QML Migration

**Defined:** 2026-07-17
**Core Value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## v5.1 Requirements — v5.0 Deferred Items Closure

**Milestone goal:** Close the 4 documented v5.0 partials (PSET-05 / EMB-06 / PLATE-05 / PLATE-06) plus Emboss style/SVG follow-ups. Each item is a 1-phase completion of a primitive already shipped in v5.0.

**Scope rule:** All work is offline/local and maps to OrcaSlicer v7.0.1 upstream. SLA print path is explicitly deferred to v5.2. LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows remain removed.

**Phases start at 154** (continuing from v5.0 phase 153).

---

### CLOS — v5.0 Partial Closures (must-have)

- [ ] **CLOS-01 (PSET-05)**: A QML side-by-side diff dialog consumes the existing `PresetServiceMock::comparePresets(A, B)` primitive (shipped Phase 149). User can select two presets and see the `{key, valueA, valueB, status}` entries rendered as a 3-column visual diff (added / removed / changed classification). Reachable from the settings sidebar.
- [ ] **CLOS-02 (EMB-06)**: TextEmboss volumes persist editable-text metadata via the upstream 3MF `<text>` block (`TextConfigurationSerialization`). Save→reload restores the volume as a re-editable TextEmboss with identical text + font + size + depth (not just geometry). Ctest `testEmbossTextMetadataRoundTrip` asserts text + font + size + depth match.
- [ ] **CLOS-03 (PLATE-05)**: Runtime thumbnail capture scheduler for session-created/modified plates. A new `Q_INVOKABLE setPlateThumbnailFromBase64(plateIndex, b64)` write path on ProjectServiceMock → `PartPlate::setThumbnail(QImage)`. On plate-content-change (or before save), the scheduler iterates plates and captures each via the existing `requestThumbnailCapture` path. PlateBar cards for non-current plates now show real thumbnails within the session, not blank.
- [ ] **CLOS-04 (PLATE-06)**: A live multi-plate save/reload ctest runs against a `ProjectServiceMock` test fixture (the unit-test harness gap that forced Phase 152 to source-audit-lock only). Asserts the full plate state (count, names, per-plate config overrides, print sequence, bed type, locked/printable flags, thumbnails) round-trips through 3MF.

---

### EMBO-F — Emboss Follow-ups (nice-to-have, single phase if context allows)

- [ ] **EMBO-F01**: Emboss style controls wired to upstream `FontProp` axes — boldness slider (already exists as `font_prop.boldness` field; UI exposes it), italic flag, and the use-surface + curve-projection options. Surfaces in the existing Emboss panel.
- [ ] **EMBO-F02**: SVG emboss advanced features — curve projection option + depth modifier. Extends the existing `addSvgVolume` path (Phase 146 verified).

---

### Cross-Workstream

- [ ] **REGRESS-05**: v5.1 regression gate. A `v51RegressionLocked` source-audit slot re-asserts all v5.1 anchors AND re-asserts the v5.0/v4.x anchors. Canonical build (`scripts/auto_verify_with_vcvars.ps1`) exit 0 + 5/5 ctest groups PASS.

---

## v2 Requirements (Future / Deferred)

### SLA print path (v5.2 milestone — research at .planning/research/sla-scope.md)

- **SLA-01**: Wire `SLAPrint` into SliceService via `PrinterTechnology` dispatch (FFF vs SLA). The libslic3r SLA surface is already compiled and linked.
- **SLA-02**: SLA preset schema (printer + material + print categories for SLA) + `printer_technology` config option read.
- **SLA-03**: `.sl1` export dialog + `exportArchiveToPath` API on SliceService.
- **SLA-04 (VDB-06 closure)**: Slicing a solid cube with `hollowEnabled=true` produces a `.sl1` archive whose layer PNGs show a visible cavity; with `hollowEnabled=false` produces solid layers. (Original VDB-06 said "G-code" — corrected to `.sl1` per SLA output format.)
- **SLA-05**: SlaSupports gizmo port (`GLGizmoSlaSupports.cpp` 1237 lines + RHI point rendering + manual/auto support generation).
- **SLA-06**: FaceDetector gizmo.

### Translation Content

- **I18N-06**: de/fr/ja/ko long tail (~906 messages/lang remaining after v4.8 I18N-05).

### Other

- D3D12 default-backend promotion (deferred from v4.5).
- Calibration `.drc` tower geometry (v4.6 CALIB tech debt).

---

## Out of Scope

Explicitly excluded from v5.1. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| SLA print path (SLAPrint wiring) | Deferred to v5.2 (dedicated SLA milestone). Research confirms ~4 phases for VDB-06 close; libslic3r SLA already compiled. |
| LAN device discovery, device send/upload, MQTT/SSDP | Removed from forward scope by user direction 2026-07-07. |
| Cloud print, cloud account/sync, OAuth | Removed from forward scope by user direction 2026-07-07. |
| Monitor task lifecycle, ModelMall/Home WebView | Removed from forward scope by user direction 2026-07-07. |
| Live camera / RTSP / WebRTC streams | Removed from forward scope; FFmpeg/WebRTC deps unavailable. |
| Printer-connected hardware workflows | Hardware-dependent; stays out under printer-hardware removal rule. |
| libslic3r slicing algorithm changes | Out of scope — GUI migration only. |
| New build directories or non-canonical build scripts | Build rules: only `build/` + `scripts/auto_verify_with_vcvars.ps1`. |
| D3D12 or Vulkan as default backend | Blocked; QRhi/D3D11 stays default. |

---

## Traceability

Populated by `gsd-roadmapper` during ROADMAP.md creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| CLOS-01 (PSET-05 diff-view) | 154 | Pending |
| CLOS-02 (EMB-06 3MF text metadata) | 155 | Pending |
| CLOS-03 (PLATE-05 runtime thumbnails) | 156 | Pending |
| CLOS-04 (PLATE-06 live ctest) | 157 | Pending |
| EMBO-F01 (Emboss style controls) | 158 | Pending |
| EMBO-F02 (SVG advanced) | 158 | Pending |
| REGRESS-05 (regression gate) | 159 | Pending |

**Coverage:**
- v5.1 requirements: 7 total (CLOS-01..04, EMBO-F01..02, REGRESS-05)
- Mapped to phases: 7/7 (100%)
- Unmapped: 0

---

*Requirements defined: 2026-07-17*
*Last updated: 2026-07-17 after v5.1 milestone definition (5 closure workstreams, 7 requirements)*
