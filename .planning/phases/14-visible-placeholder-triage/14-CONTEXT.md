# Phase 14: Visible Placeholder Triage - Context

**Gathered:** 2026-06-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 14 makes visible UI surfaces honest: actions with existing backend behavior must be wired, unavailable workflows must be hidden or disabled with product-facing copy, and runtime QML must not expose TODO, placeholder, reserved, phase/version, or fake-cloud behavior as complete product workflows.

</domain>

<decisions>
## Implementation Decisions

### Runtime Honesty
- Do not implement large modules such as full ModelMall WebView, cloud account, AssembleView, or full layer editing in Phase 14.
- Hide or disable unavailable entry points instead of leaving empty click handlers.
- Prefer neutral Chinese runtime copy for unavailable surfaces.
- Keep implementation details, phase labels, and migration notes in planning artifacts rather than runtime UI.

### Existing Behavior First
- Export project, export model, preferences, and implemented calibration entries should remain wired because existing backend/viewmodel behavior already supports them.
- Placeholder topbar icons for account, model store, publish, and filament group remain hidden.
- ModelMall may remain as a preview/blocked surface, but must not claim a functioning marketplace or publish flow.
- Layer editing and parameter subtabs may remain visible only if they clearly state unavailable/partial state and do not expose empty clicks.

### Verification Scope
- Extend `QmlUiAuditTests` to guard the Phase 14 contract, especially empty handlers, fake publish/search copy, and unavailable runtime labels.
- Run targeted QML audit, full ViewModel smoke if touched dependencies require it, and the canonical PowerShell verification command.

### the agent's Discretion
- Choose the smallest QML edits that satisfy UI honesty and keep the current layout stable.
- Use existing `Theme` tokens and `Cx*` controls; avoid visual redesign.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `BBLTopbar.qml` already hides topbar account/model-store/publish/filament-group placeholders and wires export/preferences/calibration signals.
- `main.qml` already handles export project, export model, preferences, about, and shortcut overview.
- `QmlUiAuditTests.cpp` already contains static checks for top-level no-op actions and placeholder text.
- `14-UI-SPEC.md` already defines the visual/copy contract for Phase 14.

### Established Patterns
- Runtime UI is QML with `Theme.qml` color/type tokens and project `Cx*` controls.
- Durable behavior belongs in C++ viewmodels/services; QML should only wire existing actions or show honest unavailable states.
- Planning classifies unavailable surfaces rather than pretending they are complete.

### Integration Points
- `src/qml_gui/BBLTopbar.qml`
- `src/qml_gui/main.qml`
- `src/qml_gui/panels/LeftSidebar.qml`
- `src/qml_gui/pages/ModelMallPage.qml`
- `src/qml_gui/pages/PreferencesPage.qml`
- `tests/QmlUiAuditTests.cpp`

</code_context>

<specifics>
## Specific Ideas

Use the already approved `14-UI-SPEC.md` as the UI design contract. Keep Phase 14 focused on visible placeholder triage, not module expansion.

</specifics>

<deferred>
## Deferred Ideas

- Real ModelMall WebView and publish workflow.
- Cloud account login.
- Full AssembleView.
- Full variable layer height editor.
- Full preset parameter page grouping and diff dialogs.
- Real update-check service.

</deferred>
