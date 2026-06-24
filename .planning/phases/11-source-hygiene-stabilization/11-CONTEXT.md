# Phase 11: Source Hygiene Stabilization - Context

**Gathered:** 2026-06-25
**Status:** Ready for planning
**Mode:** Smart discuss infrastructure skip

<domain>
## Phase Boundary

Phase 11 removes source-level ambiguity introduced by dirty local implementation work. The phase is limited to behavior-affecting literal escape artifacts, encoding-damaged active source comments or user-visible strings, residual backup files under `src/`, and classification of untracked baseline implementation files. It must not revert unrelated local changes and must preserve the OrcaSlicer source-truth migration rules.

</domain>

<decisions>
## Implementation Decisions

### the agent's Discretion
- Treat this as a pure source hygiene phase, not a product behavior phase.
- Prefer narrow fixes that make existing code readable, buildable, and classifiable.
- Preserve current implementation intent when repairing comments or user-visible strings.
- Do not delete or revert untracked implementation files until ownership is classified from local references and build wiring.
- Use the canonical full verification command only after source edits are complete.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `.planning/REQUIREMENTS.md` defines HYGIENE-01 through HYGIENE-04 and the Real/Hybrid/Mock/Blocked/Placeholder terms.
- `.planning/STATE.md` already lists the known hygiene targets: `CalibrationServiceMock.cpp`, encoding-damaged active files, `SliceService.cpp.backup`, disabled calibration UI, and placeholder exports.
- `.Codex/rules/build-rules.md` fixes the only full build command and build directory.

### Established Patterns
- C++ services and viewmodels use two-space indentation, Qt `QObject::tr()` for user-visible strings, and explicit signals after state changes.
- Business logic belongs in C++ services/viewmodels; QML should remain wiring and presentation.
- Current dirty worktree includes tracked edits and untracked implementation files; unrelated changes must not be reverted.

### Integration Points
- `src/core/services/CalibrationServiceMock.cpp` contains a literal `\r\n` sequence inside a `//` comment at the fallback timer branch, likely commenting out intended mock fallback code.
- `src/core/services/SliceService.cpp` contains many encoding-damaged comments and user-visible `QObject::tr()` / `QStringLiteral()` strings.
- `src/core/services/SliceService.cpp.backup` is an untracked backup artifact under `src/`.
- Untracked implementation files include `AppSettingsService.*`, `SoftwareViewport.*`, and `tests/QmlUiAuditTests.cpp`; they need classification as intentional implementation, external artifact, or deferred cleanup.

</code_context>

<specifics>
## Specific Ideas

- Fix the literal `\r\n` artifact in `CalibrationServiceMock.cpp` first because it may affect runtime fallback behavior.
- Repair or scope encoding-damaged user-visible strings in `SliceService.cpp`; comments may be repaired where intent is clear or documented as follow-up if not.
- Move or remove `SliceService.cpp.backup` only after confirming it is not referenced by build files.
- Classify untracked baseline files in Phase 11 artifacts even if they are not all committed in this phase.

</specifics>

<deferred>
## Deferred Ideas

- Do not complete calibration UI wiring in Phase 11; that belongs to Phase 12 and Phase 14.
- Do not resolve MQTT/FTP/camera verification in Phase 11; that belongs to Phase 13.
- Do not implement placeholder UI workflows in Phase 11; that belongs to Phase 14.

</deferred>
