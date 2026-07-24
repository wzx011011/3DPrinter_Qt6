# Requirements: OWzx Slicer

## Latest Completed Milestone: v5.7 D3D12 Backend Investigation

For the full requirement text, see
`.planning/milestones/v5.7-REQUIREMENTS.md`.

**Status:** Complete (2026-07-24). D3D12 promotion **reverted** after
real-machine verification showed a crash on AMD Radeon APU. D3D11-first
restored. Phase 207-210 retained. Canonical build (D3D11 default) exited `0`,
all ctest + E2E passed.

## Completed Requirements (v5.7)

| Requirement | Phase | Summary |
|---|---:|---|
| DOC-01 | 206 | Top-level planning files identify v5.7 as active; v5.6 archived. |
| REPRO-01 | 207 | Diagnostics + reproduction; root cause located (AMD APU swapchain). |
| MIT-01 | 208 | Seam A: fold readback resourceUpdate into next beginPass (retained). |
| MIT-02 | 209 | Seam B: pack camera UBO into single 80-byte write (retained). |
| MIT-03 | 210 | Seam C: first-frame force upload + initialize buffer-flag reset (retained). |
| PROMO-01 | 211 | D3D12 promotion attempted → reverted (AMD APU crash); D3D11-first restored. |

| Requirement | Phase | Summary |
|---|---:|---|
| DOC-01 | 193 | Top-level planning files identify v5.6 as active; v5.5 archived as complete. |
| UI-01 | 194 | Promote inline OptionRow components; unify step-button idioms into Cx controls. |
| UI-02 | 195 | Extract KBShortcutsDialog into a 5-group dialog aligned with upstream. |
| FEAT-01 | 196 | Emboss async spinner; SliceProgress Cancelled/Error state coverage. |
| FEAT-02 | 197 | Calibration dedicated tower geometry loading; correct the `.drc` term. |
| FEAT-03 | 198 | ObjectList tree deepening toward upstream GUI_ObjectList.cpp. |
| DLG-01 | 199 | PresetServiceMock vendor/model enumeration layer for ConfigWizard. |
| DLG-02 | 200 | ConfigWizard single-vendor wizard rewrite; close the completion loop. |
| DLG-03 | 201 | AMS mock-to-ViewModel architecture cleanup; data source stays mock. |
| DLG-04 | 202 | Plugin manager real backend aligned with WebDownPluginDlg; no Python. |
| RHI-01 | 203 | D3D12 root-cause confirmation; default stays D3D11. |
| I18N-01 | 204 | de/fr/ja/ko translation coverage to >=85%. |
| GATE-01 | 205 | Cross-workstream regression gate and v5.6 milestone audit. |

## Global Constraints

- Canonical build command:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- Canonical build directory: `build/`
- Qt 6.10 is a prebuilt dependency, not built from source by this project.
- OrcaSlicer upstream behavior remains the source truth for product behavior.
- v5.6 must not reopen LAN/device/cloud/printer-hardware scope.
- v5.6 must not embed CPython.
- v5.6 must not promote D3D12 to the default backend.
- v5.6 must not patch upstream OrcaSlicer source.
- LAN/cloud/device/camera/printer-hardware workflows remain declined unless
  explicitly reopened by the user.

## Previously Completed (v5.5, archived 2026-07-23)

| Requirement | Phase | Summary |
|---|---:|---|
| DOC-01 | 188 | Complete: planning handoff and milestone state reconciled. |
| PROV-01 | 189 | Complete: Qt, Orca source, and dependency provenance documented. |
| VERIFY-01 | 190 | Complete: the canonical script reports inputs and failure stages. |
| RUN-01 | 191 | Complete: the canonical script records `OWzxSlicer.exe` liveness. |
| GATE-01 | 192 | Complete: the audit compares CI/local evidence and classifies differences. |

## Deferred after v5.6 (per user decisions 2026-07-24)

- H2C/A2L multi-nozzle UI (bb3 fork submodule + product decision pending).
- Per-extruder config editor UI (Cmp-03 sub-item; needs multi-extruder fixture).
- ConfigWizard multi-vendor selection + PresetUpdater + AppConfig.
- D3D12 default-backend promotion itself (pending root cause).
- AMS real device/cloud data sources (printer-hardware scope).
- Python script/macro framework / CPython embedding.
