---
plan: 29-06
phase: 29
status: complete
requirements: [ARRANGE-02]
---

# Plan 29-06 Summary: Load-path defensive origin refresh + QmlUiAudit regression

## What was built

- Added a defensive `refreshPlateOrigins()` call at the end of the 3MF load-path rebuild lambda (ProjectServiceMock.cpp ~line 5358) per RESEARCH §6. Guarantees plate origins are consistent even if the auto-arrange-on-load is later conditioned out. Belt-and-suspenders (the auto-arrange at ~5375 already triggers `rebuildPlatesAfterArrangement` → `updatePlateOrigins` via Plan 04).
- Added a public `refreshPlateOrigins()` accessor on `PartPlateList` (wraps the private `updatePlateOrigins`) so the load path and other out-of-band reconstructors can refresh origins.
- Confirmed `QmlUiAuditTests` stays green (no new QML in Phase 29 — only read-only Q_PROPERTY exposure).

## Key decisions / deviations

- The load-path auto-arrange already produces correct origins post-load (via Plan 04's rewired `arrangeObjects`), so this plan is belt-and-suspenders. It was kept because it's cheap (one loop) and protects against the auto-arrange being made conditional in the future.

## Verification

- Full canonical verify green (all targets including the auto-arrange load path).
- `QmlUiAuditTests` exits 0; no new QML files in Phase 29.

## Files changed

- `src/core/services/ProjectServiceMock.cpp` — `refreshPlateOrigins()` call in load-path rebuild lambda.
- `src/core/model/PartPlateList.h` — public `refreshPlateOrigins()` wrapper.
