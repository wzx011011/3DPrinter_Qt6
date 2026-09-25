---
slug: bed-render-gaps
date: 2026-08-21
status: in-progress
---

# Bed render gap closure

Close 4 upstream PartPlate render gaps: exclude area, height limit, in-scene plate numbers/icons, assemble grabber/arrows. Upstream truth: third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.cpp:856-1330. Targets: PrepareSceneData, RhiViewportRenderer, RhiViewport, EditorViewModel (bed data), PreparePage bindings. Constraint: worktree has 8 unrelated uncommitted PartPlate readiness files - never revert them; commit only task files.
