# Phase 169: Experience Safety

**Status:** Executed (XD-01 shipped; XD-02 partial — async spinner deferred to where Emboss/SliceProgress touchpoints land)
**Workstream:** XD
**Requirements:** XD-01 (full), XD-02 (partial)

## XD-01 result

Built shared ConfirmDialog component (`src/qml_gui/dialogs/ConfirmDialog.qml`)
following the CxDialog pattern, with `openWithAction(action)` API so any
destructive trigger can route through it. Styles destructive confirms as
CxButton.Style.Danger. Registered in qml.qrc.

Routed the most impact-prone destructive trigger (deleteSelection) through it:
- Delete/Backspace key handler in PreparePage (was firing immediately)
- "删除选中" menu item
- "删除" context menu item

All 3 now show "确定要删除选中的对象吗？可通过撤销（Ctrl+Z）恢复。" before firing.

## XD-02 status (partial)

Async spinner on Emboss + SliceProgress state coverage (indeterminate/canceled/
error) deferred — those touchpoints warrant deeper state-machine work that's
better paired with the Emboss/SliceProgress code itself. The ConfirmDialog
foundation is in place; additional destructive triggers (CaliHistory 清空,
removeDevice, disconnectDevice) can be routed through it as they're touched.

## Verification
- QmlUiAuditTests 127/127 PASS
- OWzxSlicer link OK
