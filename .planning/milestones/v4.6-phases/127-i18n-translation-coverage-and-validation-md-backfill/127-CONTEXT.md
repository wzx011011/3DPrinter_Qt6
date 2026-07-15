# Phase 127: i18n Translation Coverage And VALIDATION.md Backfill - Context

**Gathered:** 2026-07-15
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
## Phase Boundary

Close I18N-01 + PROC-01: establish the translation pipeline (lupdate/lrelease) and translate the v4.6-touched core strings into at least zh_CN (proof-of-pipeline), with other languages (ja/ko/de/fr) filled to a documented baseline. Backfill missing Nyquist VALIDATION.md for v4.6 phases + previously-shipped phases.

</domain>

<decisions>
## Implementation Decisions

### i18n honest scoping
All non-en .ts files are currently 0% finished / 100% unfinished (1645 strings each; zh_CN has 2073). Translating all 8225 strings in one phase is unrealistic. **Strategy: proof-of-pipeline + core-string translation.**
- Establish/document the lupdate→translate→lrelease workflow so future phases keep .ts updated (prevents rot-back-to-0%).
- Translate the v4.6-touched UI strings (TickCode dialog/menu text from Phases 117-119, paint gizmo panel text from 120-123, calibration range labels from 124-125) into zh_CN as the proof.
- For ja/ko/de/fr: run lupdate to refresh the .ts (capture new strings), mark a documented baseline, leave detailed translation as future work with a clear remaining-work estimate. Honest scoping, not false 100% claim.

### PROC-01 VALIDATION.md
Produce VALIDATION.md for v4.6 phases (117-128) and backfill for previously-shipped phases missing them (v4.4/v4.5 carry-forward process debt). Use the Nyquist template if one exists; otherwise a consistent per-phase verification map.

</decisions>

<specifics>
## Code Access Points
- i18n/*.ts (6 files: de/en/fr/ja/ko/zh_CN — all 0% finished).
- CMakeLists.txt (Qt Linguist tools — confirm lupdate/lrelease wiring).
- src/qml_gui/qtquickcontrols2.conf (locale config).
- .planning/phases/*/ (VALIDATION.md missing in several).

## Source-Truth Anchors
- qsTr() coverage ~95% (strings are marked; only content is untranslated).
- Qt Linguist: lupdate (extract) → manual translate in .ts → lrelease (compile .qm).

</specifics>

<deferred>
## Deferred Ideas
- Full 100% translation of ja/ko/de/fr (8225 strings) — future, documented baseline in this phase.
- Additional languages (zh_TW/es/pt/ru/it/tr/uk) — future.

</deferred>
