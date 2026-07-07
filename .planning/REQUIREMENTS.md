# Requirements: OWzx Slicer v4.1 Parameter Settings Dialogs Source-Truth Restoration

**Defined:** 2026-07-07
**Core Value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## v4.1 Requirements

### Source-Truth Inventory

- [x] **SETAUDIT-01**: User-facing printer, material, and process settings work has a current inventory that maps screenshot-visible regions to OrcaSlicer source files, Qt targets, replacement decisions, and verification evidence.
- [x] **SETAUDIT-02**: Phase 56 residual settings visual-UAT items are reconciled into v4.1 requirements or explicitly closed/deferred with evidence.

### Settings Window Layout

- [ ] **SETLAYOUT-01**: User can open printer, material, and process settings from Prepare/sidebar/preset entry points as independent non-modal windows with screenshot-aligned chrome, preset selector, action icons, top tabs, and stable size.
- [ ] **SETLAYOUT-02**: Printer and material settings dialogs match target screenshot density, spacing, tab order, and section flow; the process dialog reuses the same source-truth shell without a separate invented design.
- [ ] **SETLAYOUT-03**: User sees no mojibake, raw internal strings, placeholder controls, disconnected buttons, or off-design left group sidebar in restored settings windows.

### Option Sections And Controls

- [ ] **SETCTRL-01**: User can read settings as upstream/screenshot-like option sections with compact section headers, dividers, icons where applicable, and stable scroll behavior.
- [ ] **SETCTRL-02**: User can edit typed options with screenshot-aligned controls for checkboxes, numeric fields with units, enum combos, text/color fields, and paired min/max numeric rows.
- [ ] **SETCTRL-03**: Dirty, value-source, read-only, nullable/inherit, vector/per-extruder, and validation states are visible without row resize, overlap, or ambiguous disabled affordances.

### Preset And Edit Semantics

- [ ] **SETSEM-01**: Preset selection, save, save-as, reset option/group/all, discard, cancel, and unsaved-close guard remain mapped to upstream settings semantics.
- [ ] **SETSEM-02**: Search and simple/advanced filtering work per dialog without breaking tab/section navigation or hiding current dirty/error states.
- [ ] **SETSEM-03**: Settings edits invalidate slice state, preserve dirty overrides through project save/load, and keep Prepare/Preview payloads stable across settings dialog interaction.

### Cleanup And Verification

- [ ] **SETCLEAN-01**: Deprecated settings pages, components, routes, imports, resources, tests, and disconnected code paths left by replaced UI are removed or explicitly classified if still used.
- [ ] **SETVERIFY-01**: Automated source/QML audits cover the settings region map, clean text, required bindings, option-control structure, and upstream mapping anchors.
- [ ] **SETVERIFY-02**: The canonical verifier passes, `build/OWzxSlicer.exe` launches, and printer/material/process settings visual evidence is recorded; printer/material screenshots are compared against target images.

## Future Requirements

### Adjacent Local/Offline Work

- **ASSEMBLE-FUTURE-01**: Complete AssembleView as a dedicated source-truth milestone.
- **THUMB-FUTURE-01**: Complete real thumbnail capture and 3MF pixel round-trip.
- **FIXTURE-FUTURE-01**: Add missing CLI fixtures (`hotend.stl`, `Block20XY.stl`) and deterministic GUI fixture loading for visual screenshots.
- **BACKEND-FUTURE-01**: Resolve the D3D12 QRhi crash and evaluate Vulkan only after an SDK/runtime path exists.

### Removed Product Scope

- **NETWORK-REMOVED-01**: LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, and printer-connected hardware workflows are not future requirements unless the user explicitly reopens them.

## Out of Scope

Explicitly excluded to keep v4.1 focused.

| Feature | Reason |
|---|---|
| Full upstream PresetBundle import/export compatibility beyond settings dialog save/reset/preset selection | Larger preset-system milestone; v4.1 focuses on visible settings dialogs and already-wired edit semantics. |
| AssembleView | Separate user workflow and source-truth surface. |
| Device, cloud print, Monitor, ModelMall/Home WebView/cloud, live camera/network, and printer-connected hardware workflows | Removed from forward product scope by user direction on 2026-07-07. |
| D3D12 or Vulkan backend promotion | Renderer backend work is blocked/future and not required for settings UI restoration on D3D11. |
| libslic3r slicing algorithm changes | GUI restoration must not change slicing engine behavior. |
| New product behavior not mapped to OrcaSlicer upstream | Violates the project core value. |

## Traceability

| Requirement | Phase | Status |
|---|---|---|
| SETAUDIT-01 | Phase 84 | Complete |
| SETAUDIT-02 | Phase 84 | Complete |
| SETLAYOUT-01 | Phase 85 | Pending |
| SETLAYOUT-02 | Phase 85 | Pending |
| SETLAYOUT-03 | Phase 85 | Pending |
| SETCTRL-01 | Phase 86 | Pending |
| SETCTRL-02 | Phase 86 | Pending |
| SETCTRL-03 | Phase 86 | Pending |
| SETSEM-01 | Phase 87 | Pending |
| SETSEM-02 | Phase 87 | Pending |
| SETSEM-03 | Phase 87 | Pending |
| SETCLEAN-01 | Phase 88 | Pending |
| SETVERIFY-01 | Phase 88 | Pending |
| SETVERIFY-02 | Phase 88 | Pending |

**Coverage:**
- v4.1 requirements: 14 total
- Mapped to phases: 14
- Unmapped: 0

---
*Requirements defined: 2026-07-07*
*Last updated: 2026-07-07 after v4.1 milestone planning*
