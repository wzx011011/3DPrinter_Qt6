---
title: Prepare Page Visual Parity Design
date: 2026-07-06
status: approved
---

# Prepare Page Visual Parity Design

## Goal

Restore the remaining Prepare page visual differences against `shotScreen/准备页.png` without changing product behavior or hard-coding normal runtime data. The target is screenshot-level layout parity for the default Prepare page at 2560x1400.

## Scope

The work covers the remaining visible regions after the left sidebar width restoration:

- central Prepare viewport background, bed framing, grid tone, and default fit state;
- top viewport toolbar position, density, disabled-state treatment, and icon button sizing;
- right floating viewport tool strip position, button sizing, spacing, and opacity;
- bottom status/action strip placement, color, density, and visibility;
- visual-compare fixture/state support only where needed to separate layout issues from runtime data differences.

The already-restored left sidebar width remains fixed at the screenshot boundary. It can receive small color or spacing adjustments, but not a width redesign.

## Non-Goals

- Do not replace source-truth behavior with screenshot-only behavior.
- Do not hard-code normal product printer, filament, process, or model data merely to match one screenshot.
- Do not create alternate build directories or alternate verification scripts.
- Do not modify unrelated Preview, settings dialog, or Monitor pages in this task.

## Design

Add a visual parity contract around the Prepare page default layout. The production UI should continue to bind to real viewmodels, while an existing or small visual-compare path may provide deterministic screenshot state for audits.

Implementation should stay mostly in QML layout and theme-level tokens:

- `PreparePage.qml` owns viewport overlays, toolbar geometry, floating tools, and bottom status surfaces.
- `LeftSidebar.qml` remains the restored sidebar and may only receive scoped polish.
- `Theme.qml` may receive narrowly named color tokens only when reused by multiple Prepare surfaces.
- C++ should only change if deterministic visual-compare state or renderer defaults are impossible in QML.

## Data And State

Visual comparison should distinguish two classes of difference:

- Layout differences: fixed by QML/C++ changes and locked by tests.
- Runtime data differences: printer names, material names, profile names, loaded model, and camera/object state. These should be controlled only by visual-compare fixture/state support, not by changing normal app defaults.

## Testing

Use test-first coverage before production changes:

- Add QML audit tests for structural contracts around viewport toolbar, right floating strip, bottom status strip, and visual-compare state gates.
- Add targeted viewmodel or backend tests only if new deterministic visual-compare state is needed.
- Use pixel evidence from 2560x1400 screenshots saved under `build/pixel-audit/`.
- Final verification must run `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.

## Acceptance

- At 2560x1400, the current Prepare screenshot has the same major region boundaries as `shotScreen/准备页.png`.
- Left sidebar boundary remains within 6px of the target and is not regressed.
- Top toolbar, right tool strip, and bottom status surfaces are within a small pixel tolerance of the target region positions.
- Remaining differences are documented as fixture/data differences or explicit future renderer limitations.
- Encoding guard and canonical verification pass.
