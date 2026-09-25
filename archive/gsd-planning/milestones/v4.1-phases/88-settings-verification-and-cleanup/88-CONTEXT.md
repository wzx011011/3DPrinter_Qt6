# Phase 88 Context: Settings Verification And Cleanup

## Scope

Final verification and cleanup for v4.1 Parameter Settings Dialogs Source-Truth Restoration.
LAN/device/cloud/network workflows are excluded.

## Requirements

- SETCLEAN-01: Deprecated settings pages, components, routes, imports, resources, tests, and disconnected code paths left by replaced UI are removed or explicitly classified if still used.
- SETVERIFY-01: Automated source/QML audits cover the settings region map, clean text, required bindings, option-control structure, and upstream mapping anchors.
- SETVERIFY-02: The canonical verifier passes, `build/OWzxSlicer.exe` launches, and printer/material/process settings visual evidence is recorded; printer/material screenshots are compared against target images.

## Existing Coverage

- `deletedSettingsPathsStayAbsent` locks removed SettingsPage/ConfigPage/ParamsPage/SearchDialog and old sidebar panels.
- `deletedRoutesStayAbsent` locks removed settings-page router/deferred-exit code.
- Phase 85 audit covers settings shell/dialog structure.
- Phase 86 audit covers typed option rows and metadata indicators.
- Phase 87 audit covers dirty pending preset workflow wiring.

## Cleanup Gap

`src/qml_gui/qml.qrc` contains active settings resources but its new entries are not consistently indented. This is harmless at runtime, but Phase 88 should normalize the final resource list and add a final audit that all restored settings resources and audit anchors remain present.

## Verification Direction

- Add a final QML audit that ties together settings resource entries, main dispatch, phase audit anchors, and deleted path coverage.
- Normalize the qrc settings entries.
- Record visual evidence from the launched app.
- Run the canonical verifier.
