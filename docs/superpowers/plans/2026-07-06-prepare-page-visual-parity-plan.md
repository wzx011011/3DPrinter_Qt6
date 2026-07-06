# Prepare Page Visual Parity Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Restore the remaining Prepare page visual differences against `shotScreen/准备页.png` while preserving source-truth behavior.

**Architecture:** Keep product behavior bound to existing viewmodels. Fix visual layout in QML surfaces and use visual-compare state only to make screenshot audits deterministic. Lock the geometry contracts through existing `QmlUiAuditTests`.

**Tech Stack:** C++17, Qt 6.10, QML, Qt Test, CMake/Ninja/MSVC, Windows 10/11.

## Global Constraints

- Build command: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
- Build directory: `build/` only.
- Text encoding: UTF-8 without BOM.
- QML stays presentation-only; business logic remains in C++ viewmodels/services.
- Visual truth: `shotScreen/准备页.png`.
- Do not hard-code normal product printer, filament, process, or model data solely to match one screenshot.

---

### Task 1: Capture Baseline And Add Visual Contract Tests

**Files:**
- Modify: `tests/QmlUiAuditTests.cpp`
- Read: `src/qml_gui/pages/PreparePage.qml`
- Read: `src/qml_gui/panels/LeftSidebar.qml`
- Evidence: `build/pixel-audit/prepare-full-visual-baseline.json`

**Interfaces:**
- Consumes: `readSource(QStringLiteral(...))` helper in `QmlUiAuditTests.cpp`.
- Produces: `prepareFullVisualParityContract()` test slot.

- [ ] **Step 1: Capture baseline pixel metrics**

Run a Python screenshot script that:
- captures the current maximized `OWzx Slicer` window;
- scales `shotScreen/准备页.png` to the runtime size;
- writes major region boundary metrics to `build/pixel-audit/prepare-full-visual-baseline.json`.

- [ ] **Step 2: Write failing structural test**

Add `void prepareFullVisualParityContract();` to `QmlUiAuditTests` and assert the current desired contracts:
- top viewport toolbar has a named root id and screenshot-style geometry constants;
- right floating viewport strip has a named root id and screenshot-style geometry constants;
- bottom status strip has a named root id and screenshot-style geometry constants;
- visual compare mode has a deterministic Prepare branch if runtime data needs fixture separation.

- [ ] **Step 3: Run test to verify failure**

Run the focused UI audit target via the canonical build environment if needed. Expected: `prepareFullVisualParityContract` fails because the new ids/constants are not implemented yet.

### Task 2: Restore Viewport Overlay Geometry

**Files:**
- Modify: `src/qml_gui/pages/PreparePage.qml`
- Modify: `tests/QmlUiAuditTests.cpp`

**Interfaces:**
- Consumes: `backend.visualCompareMode`, existing viewport overlay ids, existing action bindings.
- Produces: named overlay roots and geometry constants used by the audit test.

- [ ] **Step 1: Implement top toolbar geometry**

Give the top toolbar a stable id and explicit screenshot-oriented constants for x/y/height, icon size, spacing, and disabled opacity. Preserve all existing action enablement and callbacks.

- [ ] **Step 2: Implement right floating strip geometry**

Give the right floating tool strip a stable id and explicit screenshot-oriented constants for right inset, top anchor, button size, spacing, and opacity. Preserve existing viewport/action behavior.

- [ ] **Step 3: Implement bottom status strip geometry**

Give the bottom status/action strip a stable id and explicit screenshot-oriented constants for bottom inset, width, height, color, and text/button density. Preserve existing status text and action behavior.

- [ ] **Step 4: Run focused audit**

Run `QmlUiAuditTests` and ensure `prepareFullVisualParityContract` passes.

### Task 3: Add Deterministic Visual Compare State Where Needed

**Files:**
- Modify: `src/qml_gui/pages/PreparePage.qml`
- Modify: `src/qml_gui/panels/LeftSidebar.qml` only if fixture labels need visual isolation.
- Modify: `tests/QmlUiAuditTests.cpp`

**Interfaces:**
- Consumes: `backend.visualCompareMode`.
- Produces: visually deterministic labels or placeholder states only under visual compare mode.

- [ ] **Step 1: Add visual-compare guards**

If runtime data remains a dominant visual diff, add QML-only display fallbacks guarded by `backend.visualCompareMode`. Do not change normal product data.

- [ ] **Step 2: Lock guards in tests**

Extend `prepareFullVisualParityContract` to assert the guards are present and normal bindings remain present.

- [ ] **Step 3: Run focused audit**

Run `QmlUiAuditTests`; expected pass.

### Task 4: Pixel Evidence, Canonical Verification, And GSD Summary

**Files:**
- Modify: `.planning/quick/260706-r8m-prepare-page-full-visual-parity/260706-r8m-PLAN.md`
- Create: `.planning/quick/260706-r8m-prepare-page-full-visual-parity/260706-r8m-SUMMARY.md`
- Modify: `.planning/STATE.md`

**Interfaces:**
- Consumes: screenshot evidence from `build/pixel-audit/`.
- Produces: complete GSD quick record.

- [ ] **Step 1: Capture final pixel evidence**

Capture current and target screenshots at 2560x1400 and save:
- `build/pixel-audit/prepare-full-visual-runtime-final.png`
- `build/pixel-audit/prepare-full-visual-side-by-side-final.png`
- `build/pixel-audit/prepare-full-visual-final.json`

- [ ] **Step 2: Run checks**

Run:
- `python %USERPROFILE%\.coding-encoding-guard\encoding_guard.py <changed files>`
- `git diff --check`
- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`

- [ ] **Step 3: Update GSD records**

Mark quick task complete, add final metrics, and record commit ids.

- [ ] **Step 4: Commit**

Commit code/test changes, then commit GSD documentation updates separately.
