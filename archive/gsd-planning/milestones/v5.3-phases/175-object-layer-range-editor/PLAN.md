# Phase 175: Object Layer-Range Editor

**Status:** Executed
**Workstream:** FEAT
**Requirement:** FEAT-02

## Result

Ship the QML dialog for per-layer-height ranges. Mirrors upstream GUI_ObjectLayers.
Backend was fully ready (objectLayerRanges/addObjectLayerRange/removeObjectLayerRange/
setLayerRangeValue/layerRangeValue on ProjectServiceMock).

- EditorViewModel: 7 new Q_INVOKABLE proxies (objectLayerRangeCount/layerRangeMinZ/
  layerRangeMaxZ/layerRangeValue/addObjectLayerRange/removeObjectLayerRange/
  setLayerRangeValue).
- ObjectLayersDialog.qml (new CxDialog): ListView of existing ranges (min/max Z +
  layer_height override per row + remove button). Add-range row at the bottom
  (minZ + maxZ + layer_height inputs + Add button).
- PreparePage: instantiates the dialog + new "层高范围..." menu item in object
  context menu (对齐上游 GUI_ObjectLayers entry).
- qml.qrc registration.

## Verification
- QmlUiAuditTests 133/133 PASS
- OWzxSlicer link OK
