---
phase: 14
phase_name: Visible Placeholder Triage
plan_id: 14-01
title: Make visible UI placeholders honest
status: complete
wave: 1
type: implementation
autonomous: true
requirements_addressed:
  - UI-01
  - UI-02
  - UI-03
  - UI-04
  - UI-05
files_modified:
  - src/qml_gui/BBLTopbar.qml
  - src/qml_gui/main.qml
  - src/qml_gui/panels/LeftSidebar.qml
  - src/qml_gui/pages/ModelMallPage.qml
  - src/qml_gui/pages/PreferencesPage.qml
  - tests/QmlUiAuditTests.cpp
  - .planning/phases/14-visible-placeholder-triage/14-SUMMARY.md
  - .planning/phases/14-visible-placeholder-triage/14-VERIFICATION.md
  - .planning/phases/14-visible-placeholder-triage/14-REVIEW.md
  - .planning/REQUIREMENTS.md
  - .planning/ROADMAP.md
  - .planning/STATE.md
---

# Plan 14-01: Make Visible UI Placeholders Honest

<objective>
Remove or classify visible placeholder/no-op UI paths so users see wired workflows where behavior exists and clear unavailable states where source-truth behavior remains future or blocked.
</objective>

<tasks>

1. Add failing static QML audit checks for Phase 14.
   - Files: `tests/QmlUiAuditTests.cpp`
   - Action: Assert no top-level empty handlers remain in touched QML.
   - Action: Assert ModelMall does not expose publish/search marketplace claims as active runtime copy.
   - Action: Assert Preferences update check no longer has an empty click handler.
   - Action: Assert LeftSidebar placeholder controls are hidden/disabled with unavailable copy.
   - Acceptance criteria: Phase 14 UI audit fails before QML edits.

2. Make unavailable topbar/menu/model mall surfaces honest.
   - Files: `src/qml_gui/BBLTopbar.qml`, `src/qml_gui/pages/ModelMallPage.qml`
   - Action: Keep already wired export/preferences/calibration actions.
   - Action: Keep account/model-store/publish/filament topbar placeholders hidden.
   - Action: Disable or hide ModelMall publish and fake marketplace controls; replace user-facing English claims with neutral unavailable Chinese copy.
   - Acceptance criteria: no visible fake publish/store workflow remains.

3. Clean sidebar and preferences no-op controls.
   - Files: `src/qml_gui/panels/LeftSidebar.qml`, `src/qml_gui/pages/PreferencesPage.qml`
   - Action: Hide or disable advanced/compare/object-table controls that only contain TODO handlers.
   - Action: Remove empty update-check click handler and show unavailable copy instead.
   - Action: Keep partial parameter/layer sections honest with Chinese unavailable copy.
   - Acceptance criteria: touched QML contains no empty action handlers or runtime implementation labels for these surfaces.

4. Verify and close Phase 14.
   - Files: `.planning/phases/14-visible-placeholder-triage/14-SUMMARY.md`, `.planning/phases/14-visible-placeholder-triage/14-VERIFICATION.md`, `.planning/phases/14-visible-placeholder-triage/14-REVIEW.md`, `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`, `.planning/STATE.md`
   - Action: Run target build/tests, full QML audit, canonical verification command, and update traceability.
   - Acceptance criteria: Phase 14 complete, Phase 15 is next.

</tasks>

<verification>

- Build affected tests:
  `cmd.exe /d /s /c "call ""C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"" && cmake --build build --target QmlUiAuditTests --config Release"`
- Static UI audit:
  `build\QmlUiAuditTests.exe -o build\QmlUiAuditTests.phase14.txt,txt`
- Full canonical verification:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`

</verification>

<success_criteria>

- UI-01 through UI-05 have source/test/planning evidence.
- No touched top-level visible action is a silent no-op.
- Placeholder surfaces remain hidden, disabled, or clearly unavailable in product-facing copy.
- Canonical verification passes.

</success_criteria>

## PLANNING COMPLETE
