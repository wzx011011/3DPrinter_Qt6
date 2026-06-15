# Phase 1 Plan: Prepare Workspace Alignment

**Phase**: 1 / Prepare Workspace Alignment
**Mode**: YOLO (auto-execute)
**Date**: 2026-05-31

## Task Breakdown

### Group A: Toolbar Quick Wins (LOW risk)

| ID | Task | Files | Est. |
|----|------|-------|------|
| A1 | Add undo/redo buttons to toolbar with canUndo/canRedo guard | `PreparePage.qml` | 10min |
| A2 | Add `enabled: hasSelection` guard to all gizmo buttons (Move/Rotate/Scale/Cut/Flatten/SupportPaint/SeamPaint/Hollow/SLA) | `PreparePage.qml` | 5min |
| A3 | Add `enabled: hasSelection` guard to auto-orient and mirror buttons | `PreparePage.qml` | 5min |

### Group B: Slice Result Data Flow (PREP-02, MEDIUM risk)

| ID | Task | Files | Est. |
|----|------|-------|------|
| B1 | Populate filament length, layer count, cost in HAS_LIBSLIC3R slice path | `SliceService.cpp` | 20min |
| B2 | Add per-plate slice result storage (QMap<int,SliceResult>) | `SliceService.h/.cpp`, `EditorViewModel.cpp` | 30min |
| B3 | Fix extruderCount/extruderUsedLength/extruderUsedWeight to use real data | `EditorViewModel.cpp`, `SliceService.cpp` | 20min |

### Group C: Plate Management Alignment (PREP-01, MEDIUM-HIGH risk)

| ID | Task | Files | Est. |
|----|------|-------|------|
| C1 | Skip locked plates in requestSliceAll() | `EditorViewModel.cpp` | 5min |
| C2 | Invalidate slice results on model/config changes | `EditorViewModel.cpp` | 15min |
| C3 | Apply per-plate config (bed type, print sequence, spiral) to slice engine | `SliceService.cpp`, `ProjectServiceMock.h` | 30min |

### Group D: Object Tree Polish (PREP-01, LOW-MEDIUM risk)

| ID | Task | Files | Est. |
|----|------|-------|------|
| D1 | Add volume type icons (distinct per type) | `ObjectList.qml` | 10min |
| D2 | Add missing context menu items (Fix Mesh, Export STL, Split) | `ObjectList.qml`, `EditorViewModel.h/.cpp` | 25min |

### Group E: Background Slicing State Machine (PREP-03, MEDIUM risk)

| ID | Task | Files | Est. |
|----|------|-------|------|
| E1 | Add explicit state enum to SliceService (Idle/Slicing/Exporting/Completed/Cancelled/Error) | `SliceService.h/.cpp` | 15min |
| E2 | Add pre-slice validation (print.validate()) before process() | `SliceService.cpp` | 15min |
| E3 | Add BBL/CX printer flag setting before slicing | `SliceService.cpp` | 5min |
| E4 | Improve cancel handling — wait for completion before allowing re-slice | `SliceService.h/.cpp` | 15min |

## Execution Order

1. Group A (toolbar) — immediate, no dependencies
2. Group D (object tree) — immediate, no dependencies
3. Group C1-C2 (plate quick wins) — immediate, no dependencies
4. Group E (state machine) — builds on current SliceService
5. Group B (slice data flow) — depends on E1 for state enum
6. Group C3 (per-plate config to engine) — depends on B2 for per-plate storage

## Out of Scope for Phase 1

- Plate GL thumbnails (requires GL FBO capture pipeline — separate effort)
- Cross-plate drag-drop (requires PartPlateList awareness — deferred)
- SEH exception handling (low priority, defensive only)
- Post-processing scripts (no upstream equivalent in current config)
- Per-plate config snapshot mechanism (deferred to Phase 2 preset work)

## Success Criteria Verification

After all tasks complete:
1. Build passes (269/269 compile)
2. Smoke tests pass (66/66 or more)
3. Toolbar buttons correctly enable/disable based on selection
4. Slice results show real filament/layer/cost data in HAS_LIBSLIC3R mode
5. Multi-plate slice results preserved per plate
6. Locked plates skipped in Slice All
7. Slice results invalidated when model changes
8. SliceService has explicit state tracking
