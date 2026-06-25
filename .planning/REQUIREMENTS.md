# Requirements: OWzx Slicer

**Defined:** 2026-06-24
**Core Value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## Requirement Status Terms

- **Real:** source-truth behavior is implemented and verified with deterministic evidence.
- **Hybrid:** a real path exists, but fallback/mock behavior remains or verification is incomplete.
- **Mock:** local simulation only.
- **Blocked:** requires an unavailable dependency, protocol, credential, or product decision.
- **Placeholder:** visible UI or enum exists but has no meaningful backend behavior.

## Current Baseline Classification

| Area | Status | Evidence / Notes |
|---|---|---|
| Model/project load | Hybrid/Real | Real libslic3r paths exist; workflow edges still need source-truth parity checks when touched. |
| Basic slicing and G-code export | Real | Canonical verify passed on 2026-06-24, including E2E pipeline tests. |
| Preview rendering | Hybrid/Real | Rendering path exists; detailed upstream preview behavior remains a separate parity topic. |
| Calibration | Hybrid | PA, Flow Rate, and Temp Tower dispatch through `SliceService`, are reachable from visible topbar paths, and have deterministic ViewModel coverage. Hardware calibration and Max Volumetric Speed remain blocked/pending. |
| SSDP discovery | Hybrid | Real UDP discovery exists; Phase 13 added deterministic Bambu/Creality parser and timeout fixture coverage. Device state remains mock-heavy. |
| MQTT/FTP/send print | Hybrid | Real transport wrappers exist; Phase 13 added deterministic telemetry, command-envelope, topic, FTP URL, remote-path, and error-path coverage. Live printer publish/upload still requires manual hardware verification. |
| Camera stream | Hybrid | FFmpeg/RTSP code path exists; Phase 13 added deterministic default RTSP URL and no-stream/state-machine coverage. Real RTSP decode still requires hardware or a controlled fixture. |
| Preset bundle | Partial | Current bundle IO is simplified JSON, not upstream-compatible preset bundle behavior. |
| PartPlate / plate-scoped config | Hybrid/Placeholder | Multi-plate APIs exist, but upstream PartPlate config behavior and AssembleView are incomplete. |
| ModelMall / WebView | Blocked/Placeholder | WebView availability is hardcoded false. |
| Visible top-level UI actions | Hybrid/Placeholder | Implemented calibration, export, and preferences routes are wired where supported; unavailable account/store/publish/layer/AssembleView/ModelMall surfaces are hidden, disabled, or documented as blocked/future scope. |
| Planning docs | Realigned | v2.9 entry files, requirements, roadmap, and evidence agree after Phase 15 verification. |

## v2.9 Requirements - Implementation Realignment and Stabilization

### Planning Realignment

- [x] **PLAN-01**: Maintainer can read `.planning/INDEX.md`, `PROJECT.md`, `STATE.md`, `REQUIREMENTS.md`, and `ROADMAP.md` and see the same active milestone, baseline status, and next phase. *(Phase 10)*
- [x] **PLAN-02**: Current git history through v2.8 is reflected in planning as shipped history, not as future v2.7/v2.8 scope. *(Phase 10)*
- [x] **PLAN-03**: Every active service/workflow surface is classified as Real, Hybrid, Mock, Blocked, or Placeholder with evidence. *(Phase 10)*
- [x] **PLAN-04**: Missing rule-file references from AGENTS are either restored or redirected to existing canonical rules. *(Phase 10)*
- [x] **PLAN-05**: Old CrealityPrint-era references outside historical evidence are classified as intentional history, vendor data, or cleanup debt. *(Phase 10)*

### Source Hygiene

- [x] **HYGIENE-01**: Literal escape artifacts such as `\r\n` inside source comments are removed or converted to real line breaks where they affect behavior.
- [x] **HYGIENE-02**: Encoding-damaged source comments and user-visible strings in active files are repaired or explicitly scoped for follow-up.
- [x] **HYGIENE-03**: Residual backup/source artifacts under `src/`, including `SliceService.cpp.backup`, are removed or moved to an explicit archive after ownership is confirmed.
- [x] **HYGIENE-04**: Untracked baseline files introduced by recent implementation work are either committed as intentional work, ignored intentionally, or documented as external artifacts.

### Calibration Stabilization

- [x] **CAL-01**: User can launch implemented calibration modes from visible UI paths rather than only through backend/internal wiring.
- [x] **CAL-02**: PA, Flow Rate, and Temp Tower calibration paths have deterministic regression coverage for job creation or generated slice requests.
- [x] **CAL-03**: Calibration mock fallback behavior still works when `SliceService` is unavailable.
- [x] **CAL-04**: Calibration modes not implemented in Qt6 are explicitly marked Pending or Blocked with upstream references.
- [x] **CAL-05**: Calibration progress and completion reporting are driven by the real slice path when slicing is active and by the fallback timer only in mock mode.

### Hybrid Integration Verification

- [x] **INT-01**: SSDP discovery parsing and timeout/error paths are covered by deterministic tests or fixtures. *(Phase 13)*
- [x] **INT-02**: MQTT status parsing and pause/resume/stop publish payloads are covered without requiring a live printer. *(Phase 13)*
- [x] **INT-03**: FTP send-print path has deterministic success/error-path coverage without requiring a live printer. *(Phase 13)*
- [x] **INT-04**: Camera stream behavior has deterministic no-stream/error-path coverage and an explicit note for real RTSP verification requirements. *(Phase 13)*
- [x] **INT-05**: Software viewport / OpenGL fallback behavior is documented and covered by startup smoke evidence. *(Phase 13)*
- [x] **INT-06**: App settings and bed-shape persistence introduced by recent work are verified or explicitly deferred. *(Phase 13)*

### Visible Placeholder Triage

- [x] **UI-01**: Export project and export model menu actions either call real backend behavior or are visibly classified as deferred/blocked in planning. *(Phase 14)*
- [x] **UI-02**: Preferences menu action opens a real Preferences workflow or is explicitly deferred without silent no-op behavior. *(Phase 14)*
- [x] **UI-03**: BBLTopbar calibration entries for implemented modes are enabled and routed to calibration viewmodel actions. *(Phase 14)*
- [x] **UI-04**: Placeholder account, model store, publish, layer editing, AssembleView, and ModelMall/WebView surfaces are reclassified so broad UI presence is not counted as feature completion. *(Phase 14)*
- [x] **UI-05**: QML-side logic that affects durable behavior is identified and moved to C++ viewmodels/services or documented as presentation-only. *(Phase 14)*

### Verification Gate

- [x] **VERIFY-01**: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` passes after v2.9 planning and implementation stabilization. *(Phase 15)*
- [x] **VERIFY-02**: `ViewModelSmokeTests.exe` is either run explicitly or documented as built-only with a reason. *(Phase 15)*
- [x] **VERIFY-03**: The final v2.9 evidence links each completed requirement to build/test output, source references, or an explicit manual verification note. *(Phase 15)*

## Future Requirements

### v3.0 Candidate - PartPlate and AssembleView

- **PLATE-01**: User can manage upstream-equivalent multi-plate ownership, object assignment, reorder, duplicate, delete, rename, and slicing.
- **PLATE-02**: User can save and reload per-plate configuration overrides.
- **PLATE-03**: User can use a non-placeholder AssembleView for multi-plate assembly workflows.

### v3.1 Candidate - Preset System Completion

- **PRESET-01**: User can import/export upstream-compatible preset bundles, not only simplified JSON.
- **PRESET-02**: User can create presets through a source-truth CreatePresetsDialog workflow.
- **PRESET-03**: User receives correct dirty-state and unsaved-change prompts across preset/page switches.

### v3.2 Candidate - Web, Cloud, Multi-Machine

- **WEB-01**: User can access ModelMall/Home WebView when QtWebEngine and product policy allow it.
- **CLOUD-01**: Cloud and multi-machine workflows are moved from mock/local state to verified integration or explicit blocked state.

### Later Candidates

- Full i18n translation coverage beyond infrastructure.
- Full calibration mode coverage beyond PA, Flow Rate, and Temp Tower.
- OpenVDB-dependent hollow/support paint workflows when dependency status changes.
- WebRTC/MetaRTC camera workflows when dependencies and protocols are available.

## Out of Scope for v2.9

| Feature | Reason |
|---|---|
| Full PartPlate/AssembleView implementation | Large source-truth module; v2.9 first makes the baseline trustworthy. |
| Full upstream preset bundle compatibility | Requires dedicated preset-system milestone. |
| ModelMall/Home WebView | QtWebEngine/product integration remains a separate blocked or future scope. |
| Full i18n content migration | Not required to stabilize current implementation/planning drift. |
| New product behavior unrelated to OrcaSlicer upstream | Violates the source-truth migration constraint. |
| Alternate build scripts or build directories | Project rules allow only the canonical script and `build/`. |

## Traceability

| Requirement | Phase | Status |
|---|---|---|
| PLAN-01 | Phase 10 | Complete |
| PLAN-02 | Phase 10 | Complete |
| PLAN-03 | Phase 10 | Complete |
| PLAN-04 | Phase 10 | Complete |
| PLAN-05 | Phase 10 | Complete |
| HYGIENE-01 | Phase 11 | Complete |
| HYGIENE-02 | Phase 11 | Complete |
| HYGIENE-03 | Phase 11 | Complete |
| HYGIENE-04 | Phase 11 | Complete |
| CAL-01 | Phase 12 | Complete |
| CAL-02 | Phase 12 | Complete |
| CAL-03 | Phase 12 | Complete |
| CAL-04 | Phase 12 | Complete |
| CAL-05 | Phase 12 | Complete |
| INT-01 | Phase 13 | Complete |
| INT-02 | Phase 13 | Complete |
| INT-03 | Phase 13 | Complete |
| INT-04 | Phase 13 | Complete |
| INT-05 | Phase 13 | Complete |
| INT-06 | Phase 13 | Complete |
| UI-01 | Phase 14 | Complete |
| UI-02 | Phase 14 | Complete |
| UI-03 | Phase 14 | Complete |
| UI-04 | Phase 14 | Complete |
| UI-05 | Phase 14 | Complete |
| VERIFY-01 | Phase 15 | Complete |
| VERIFY-02 | Phase 15 | Complete |
| VERIFY-03 | Phase 15 | Complete |

**Coverage:**
- v2.9 requirements: 28 total
- Mapped to phases: 28
- Unmapped: 0

---

*Requirements defined: 2026-06-24*
*Last updated: 2026-06-25 after Phase 15 final verification and handoff.*
