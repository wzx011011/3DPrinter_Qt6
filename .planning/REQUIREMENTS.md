# Requirements: OWzx Slicer v4.0 Preview Page UI Restoration

**Defined:** 2026-07-06
**Core Value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## v4.0 Requirements

### Source-Truth Inventory

- [x] **PVAUDIT-01**: User-facing Preview page work has a single current inventory that maps screenshot-visible regions to OrcaSlicer source files, Qt targets, replacement decisions, and verification evidence.

### Preview Layout

- [x] **PVLAYOUT-01**: User can enter Preview and see a screenshot-aligned layout for top controls, viewport, side panels, layer slider, and bottom playback/status controls without overlap or layout jumps.
- [x] **PVLAYOUT-02**: User can read Preview statistics, G-code metadata, print-time/material estimates, and legend surfaces in upstream-like density and positions.
- [x] **PVLAYOUT-03**: User sees no visible Preview placeholders, raw internal labels, or dead controls in the restored layout.

### Layer, Move, And Playback Controls

- [x] **PVCTRL-01**: User can change visible layer range and current layer through restored slider/input controls with state reflected in the renderer.
- [x] **PVCTRL-02**: User can use playback controls for layer/move stepping and animation without desynchronizing PreviewViewModel state.
- [x] **PVCTRL-03**: User can rotate, pan, zoom, and fit the Preview camera without causing the model or toolpath to disappear.

### G-code Roles, Color Modes, And Rendering

- [x] **PVRENDER-01**: User can view G-code toolpaths with upstream-mapped role colors and role visibility controls.
- [x] **PVRENDER-02**: User can switch screenshot-visible Preview color modes and see honest availability for any blocked modes.
- [x] **PVRENDER-03**: User can slice from Prepare, enter Preview, adjust layer/range/role visibility, and return without losing loaded G-code payload state.

### Cleanup And Verification

- [ ] **PVCLEAN-01**: Deprecated Preview page components, imports, resource entries, tests, or disconnected UI paths are removed when replaced by the restored implementation.
- [ ] **PVVERIFY-01**: Automated source/QML audits cover the restored Preview bindings, absence of visible placeholders, and required upstream mapping evidence.
- [ ] **PVVERIFY-02**: Final milestone verification runs the canonical build command, launches `build/OWzxSlicer.exe`, records Preview page visual evidence against the target screenshot, and passes E2E import-slice-preview-export checks.

## Future Requirements

### Adjacent Restoration

- **SETTINGS-FUTURE-01**: Restore parameter settings dialogs beyond Preview dependencies.
- **DEVICE-FUTURE-01**: Complete device send/upload/cloud print and Monitor task lifecycle workflows.
- **ASSEMBLE-FUTURE-01**: Complete AssembleView as a dedicated source-truth milestone.
- **BACKEND-FUTURE-01**: Resolve the D3D12 QRhi crash and evaluate Vulkan only after an SDK/runtime path exists.

## Out of Scope

Explicitly excluded to keep v4.0 focused.

| Feature | Reason |
|---|---|
| Prepare page redesign beyond Preview regression fixes | v3.9 already shipped Prepare restoration; v4.0 should not reopen it unless Preview depends on it. |
| Parameter settings dialog restoration beyond Preview entry/display needs | Settings workflows need a dedicated source-truth pass. |
| Device, cloud print, Monitor, and hardware task lifecycle workflows | These require protocol/hardware verification and are unrelated to Preview page layout parity. |
| AssembleView | Separate user workflow and source-truth surface. |
| D3D12 or Vulkan backend promotion | Renderer backend work is blocked/future and not required for Preview UI parity on D3D11. |
| libslic3r slicing algorithm changes | GUI restoration must not change slicing engine behavior. |
| New product behavior not mapped to OrcaSlicer upstream | Violates the project core value. |

## Traceability

| Requirement | Phase | Status |
|---|---|---|
| PVAUDIT-01 | Phase 79 | Complete |
| PVLAYOUT-01 | Phase 80 | Complete |
| PVLAYOUT-02 | Phase 80 | Complete |
| PVLAYOUT-03 | Phase 80 | Complete |
| PVCTRL-01 | Phase 81 | Complete |
| PVCTRL-02 | Phase 81 | Complete |
| PVCTRL-03 | Phase 81 | Complete |
| PVRENDER-01 | Phase 82 | Complete |
| PVRENDER-02 | Phase 82 | Complete |
| PVRENDER-03 | Phase 82 | Complete |
| PVCLEAN-01 | Phase 83 | Pending |
| PVVERIFY-01 | Phase 83 | Pending |
| PVVERIFY-02 | Phase 83 | Pending |

**Coverage:**
- v4.0 requirements: 13 total
- Mapped to phases: 13
- Unmapped: 0

---
*Requirements defined: 2026-07-06*
*Last updated: 2026-07-07 after Phase 82 completion*
