# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-05-31)

**Core value:** Upstream CrealityPrint source is functional truth -- Qt6 code must fully inherit upstream behavior, never freely design new product behavior.
**Current focus:** Phase 1: Prepare Workspace Alignment

## Current Position

Phase: 1 of 8 (Prepare Workspace Alignment)
Plan: 0 of TBD in current phase
Status: Ready to plan
Last activity: 2026-05-31 -- Roadmap created

Progress: [░░░░░░░░░░] 0%

## Performance Metrics

**Velocity:**
- Total plans completed: 0
- Average duration: -
- Total execution time: 0 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| - | - | - | - |

**Recent Trend:**
- Last 5 plans: (none)
- Trend: -

*Updated after each plan completion*

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table.
Recent decisions affecting current work:

- Roadmap creation: 15 Active requirements grouped into 8 phases by natural delivery boundaries
- Gizmo phases grouped by rendering capability (GL handles, painting, hollowing) rather than by upstream gizmo class
- Prepare workspace (Phase 1) prioritized as first phase because right panel and slicing state machine are prerequisites for Preview (Phase 3) and Settings (Phase 2)

### Pending Todos

None yet.

### Blockers/Concerns

- Phase 5 (Gizmo GL): GIZM-02/03/04 require non-TriangleSelector and non-OpenVDB alternative implementations -- research needed during planning
- Phase 6 (Device/Calibration): DEVC-01 requires protocol layer implementation; bambu_networking is closed-source (out of scope), so MQTT/SSDP must be built from scratch
- Phase 7 (Mall/Multi): MALL-01 requires QtWebEngine dependency availability in build environment
- God objects: ProjectServiceMock (4,563 lines) and EditorViewModel (3,500 lines) make any modification high-risk -- watch for parallel array sync bugs

## Deferred Items

| Category | Item | Status | Deferred At |
|----------|------|--------|-------------|
| Dependency | PartPlateList real integration (wxWidgets dep) | Out of scope | 2026-05-31 |
| Dependency | TriangleSelector real integration (wxWidgets dep) | Out of scope | 2026-05-31 |
| Dependency | OpenVDB integration (link failure) | Out of scope | 2026-05-31 |
| Dependency | FFmpeg/RTSP video streaming (not found) | Out of scope | 2026-05-31 |
| Dependency | bambu_networking real connection (closed source) | Out of scope | 2026-05-31 |
| Feature | Shell rendering (libslic3r GCodeViewer dep) | Out of scope | 2026-05-31 |
| Feature | SLA module full migration | Out of scope | 2026-05-31 |
| Feature | FaceDetector real implementation (upstream commented out) | Out of scope | 2026-05-31 |

## Session Continuity

Last session: 2026-05-31
Stopped at: Roadmap created, STATE.md initialized, ready for Phase 1 planning
Resume file: None
