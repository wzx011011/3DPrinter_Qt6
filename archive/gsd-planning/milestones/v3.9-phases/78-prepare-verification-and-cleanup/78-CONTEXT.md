# Phase 78 Context: Prepare Verification And Cleanup

## Milestone

v3.9 Prepare Page UI Restoration

## Scope

Phase 78 closes the Prepare page restoration milestone after Phases 74-77:

- Phase 74 froze the source-truth and screenshot gap matrix.
- Phase 75 restored the Prepare sidebar and preset density.
- Phase 76 restored object, plate, and slice workflow panels.
- Phase 77 restored viewport controls and gizmo-facing UI.

This phase does not introduce new product behavior. It verifies that the restored Prepare implementation is the only active path, that stale QML resources stay deleted, and that the canonical verifier and runtime launch remain clean.

## Source Truth

- Visual truth: `shotScreen/` Prepare page screenshot.
- Behavior truth: OrcaSlicer `Plater.*`, `GLCanvas3D.*`, `GUI_ObjectList.*`, `GUI_ObjectSettings.*`, and `Gizmos/*`.
- Qt targets: `PreparePage.qml`, `LeftSidebar.qml`, `ObjectList.qml`, `SliceProgress.qml`, `DockableSidebar.qml`, `GLToolbars.qml`, and the existing backend viewmodels/services they bind.

## Observations Before Work

- The workspace is clean except untracked `.zcode/`.
- `qml.qrc` references the restored Prepare files and no longer references the legacy `Sidebar.qml`, `FilamentPanel.qml`, or `PrintSettings.qml`.
- The canonical verifier passed after Phase 77.
- Runtime diagnostics still showed a pre-existing QML warning from `CxTextArea.qml` because `ScrollBar.vertical` was attached directly to `TextArea`, which Qt rejects for non-Flickable targets.
- `tests/data/test_model.stl` exists, but `main_qml.cpp` does not consume file-path argv, so Phase 78 visual evidence uses the default Prepare startup screen unless the user loads a model interactively.

## Requirements

- CLEAN-01: deprecated Prepare page components, imports, resource entries, tests, or disconnected UI paths are removed when replaced by the restored implementation.
- VERIFY-01: automated source/QML audits cover restored Prepare bindings, absence of visible placeholders, and required upstream mapping evidence.
- VERIFY-02: canonical verifier passes, `build/OWzxSlicer.exe` launches, and Prepare visual evidence is recorded against the target screenshot.

## Risks

- Screenshot capture on this Windows session may fail or capture a blank surface; prior Windows Graphics Capture attempts returned `0x80004002`.
- Default runtime visual evidence cannot prove model-loaded parity without a supported startup file-load path or manual user interaction.
- Full canonical verification is slow and must remain the only build path.
