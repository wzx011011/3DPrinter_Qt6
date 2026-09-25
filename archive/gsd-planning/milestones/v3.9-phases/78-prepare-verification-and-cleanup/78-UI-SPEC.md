# Phase 78 UI Spec: Prepare Final Verification

## Contract

The Prepare page must keep the restored UI structure from Phases 75-77:

- Dense left sidebar using `LeftSidebar`, `ObjectList`, and `SliceProgress`.
- Restored central Prepare page using `GLViewport`, `GLToolbars`, compact plate strip, and transform mini panel.
- Icon-first viewport toolbars with compact top, right, and lower-left controls.
- No large legacy floating `>> Slice` button.
- No deleted Prepare sidebar panel paths in `qml.qrc` or on disk.

## Cleanup Contract

- `panels/Sidebar.qml`, `panels/FilamentPanel.qml`, and `panels/PrintSettings.qml` must stay absent from disk and resources.
- Restored Prepare paths must remain explicitly present in `qml.qrc`.
- Phase 75-77 audit tests must remain present so future changes cannot bypass this milestone's source-truth coverage.
- QML startup diagnostics must not include the known `CxTextArea` invalid attached ScrollBar warning.

## Visual Evidence Contract

Record a runtime Prepare screenshot after canonical verification and manual app launch. If no command-line model load is available, record the default Prepare startup state and document that limitation.

## Non-Goals

- No new product behavior.
- No Preview, Settings, Device, cloud, or AssembleView restoration beyond direct Prepare dependencies.
- No slicing engine changes.
- No renderer backend promotion.
