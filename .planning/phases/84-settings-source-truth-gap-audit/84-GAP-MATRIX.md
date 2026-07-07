# Phase 84 Settings Gap Matrix

**Target evidence:**

- `shotScreen/打印机参数设置页.png` (printer settings, 736x593)
- `shotScreen/材料参数设置页.png` (material settings, 736x593)

**Process settings evidence:** no process screenshot is available. Process
settings must use OrcaSlicer source-truth parity and the same restored shell,
not an invented OWzx-only visual design.

**Scope:** printer, material, and process parameter settings dialogs only.
No LAN/device/cloud/network, AssembleView, D3D12/Vulkan, or full preset-bundle
import/export work is in Phase 84 scope.

## Summary

Phase 84 is the v4.1 source-truth audit. Its job is to freeze the settings
restoration map before implementation. Current Phase 56 settings code already
contains important semantic wiring, but the visible settings dialogs still need
screenshot-level restoration and explicit reconciliation of deferred visual
evidence.

This matrix is the canonical routing artifact for Phase 85-88.

## Canonical Region Matrix

| Region | Target Observation | Current Evidence | Qt Targets | Upstream Source | Decision | Gap | Severity | Owner | Requirement | Verification |
|---|---|---|---|---|---|---|---|---|---|---|
| SET-SHELL | Printer/material screenshots show a 736x593 independent dark settings window with native title text, close button, no embedded page chrome, and a vertical scrollbar. Process uses same shell by source parity. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | To be classified. | To be classified. | High | Phase 85 | SETLAYOUT-01, SETLAYOUT-02 | Source/QML audit plus Phase 88 runtime screenshots. |
| SET-PRESET-ACTIONS | Top row shows a preset combo, compact action icons, and compact mode/search affordances rather than a wide text-heavy action bar. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | To be classified. | To be classified. | High | Phase 85, Phase 87 | SETLAYOUT-01, SETLAYOUT-03, SETSEM-01, SETSEM-02 | QML audit for real actions and Phase 88 interaction evidence. |
| SET-TABS | Printer tabs: 基础信息, 打印机G-code, 材料, 挤出机, 移动能力, 注释. Material tabs: 耗材丝, 冷却, 参数覆盖, 高级, 材料, 依赖, 注释. Process uses upstream print-process tab parity. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | To be classified. | To be classified. | High | Phase 85 | SETLAYOUT-02, SETLAYOUT-03 | Source/QML audit for tab names/order and screenshot comparison. |
| SET-SECTIONS | Screenshots show compact section headers with small cyan icons/dividers and dense vertical flow, not nested cards. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | To be classified. | To be classified. | High | Phase 86 | SETCTRL-01 | QML audit plus Phase 88 visual evidence. |
| SET-TYPED-ROWS | Rows include checkboxes, numeric fields with unit suffixes, enum combos, text/color controls, and paired min/max numeric rows. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | To be classified. | To be classified. | Critical | Phase 86 | SETCTRL-02 | Source/QML audit for typed controls and existing ViewModel smoke tests. |
| SET-STATE-INDICATORS | Dirty, read-only, value-source, nullable/inherit, vector/per-extruder, validation, and disabled states must stay visible without row jumps or ambiguous affordances. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | To be classified. | To be classified. | High | Phase 86, Phase 87 | SETCTRL-03, SETSEM-01 | ViewModel/QML audits and Phase 88 runtime interaction evidence. |
| SET-ENTRYPOINTS | Printer/material/process settings open from Prepare/sidebar/preset entry points as independent non-modal windows. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | To be classified. | To be classified. | High | Phase 85, Phase 87 | SETLAYOUT-01, SETSEM-03 | Existing smoke tests plus Phase 88 UIAutomation launch evidence. |
| SET-SEARCH-MODE | Search and simple/advanced filtering are per dialog and must not hide current dirty/error states. Screenshot action row implies compact affordances, not the current wide text field by default. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | To be classified. | To be classified. | Medium | Phase 85, Phase 87 | SETLAYOUT-03, SETSEM-02 | Source/QML audit and interaction evidence. |
| SET-PRESET-SEMANTICS | Preset selection, save, save-as, reset option/group/all, discard, cancel, and unsaved-close guard preserve upstream-mapped behavior. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | Preserve/harden. | To be classified. | Critical | Phase 87 | SETSEM-01 | Existing Phase 56 tests plus new audit checks. |
| SET-PERSISTENCE | Settings edits invalidate slice state, preserve dirty overrides through project save/load, and do not clear Prepare/Preview payload state. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | Preserve/harden. | To be classified. | Critical | Phase 87 | SETSEM-03 | Existing E2E tests plus Phase 88 regression pass. |
| SET-CLEANUP | Deprecated settings surfaces, mojibake, placeholders, disconnected controls, unused left group sidebar, stale qrc/imports/tests, and off-design code paths are removed or explicitly classified. | To be filled in Task 84-01-02. | To be filled in Task 84-01-02. | Source-truth cleanup rule; OrcaSlicer setting surfaces above. | To be classified. | To be classified. | High | Phase 88 | SETCLEAN-01, SETVERIFY-01, SETVERIFY-02 | Source/QML audit, canonical verifier, runtime visual evidence. |

## Requirement Coverage

| Requirement | Covered By |
|---|---|
| SETAUDIT-01 | This matrix maps settings regions to target screenshots, upstream source files, Qt targets, decisions, owner phases, and verification. |
| SETAUDIT-02 | The residual reconciliation section must map Phase 56 deferred visual-UAT items into v4.1 ownership. |

## Phase Routing

| Phase | Work To Start From This Audit |
|---|---|
| 85 | Restore screenshot-visible shell, top row, tab strip, sizing, density, clean text, and visible entry behavior. |
| 86 | Restore section flow and typed option controls while preserving C++ option models. |
| 87 | Preserve and harden preset save/reset/search/dirty/edit semantics, slice invalidation, and project persistence. |
| 88 | Remove stale paths, add source/QML audits, run canonical verifier, launch app, and capture final settings visual evidence. |
