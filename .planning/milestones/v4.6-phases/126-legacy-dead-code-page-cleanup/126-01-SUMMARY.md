# Phase 126 Summary: Legacy Dead-Code Page Cleanup

**Phase:** 126 (WS4, CLEAN-01)
**Status:** Complete
**Date:** 2026-07-15

## What Shipped
Closed CLEAN-01: deleted 4 dead QML files (DeviceListPage/AuxiliaryPage/ModelMallPage/AuxiliaryListPanel) + AuxiliaryService + cleaned routes/qml.qrc/CMake/BackendContext. These were in the removed LAN/device/cloud scope — deletion, not repair (research corrected the REQUIREMENTS wording).

## Changes
- Deleted: DeviceListPage.qml, AuxiliaryPage.qml, ModelMallPage.qml, AuxiliaryListPanel.qml, AuxiliaryService.h/.cpp.
- main.qml: slot 7 AuxiliaryPage → structural placeholder.
- BBLTopbar.qml: removed "辅助" tab.
- qml.qrc: removed 4 entries.
- BackendContext.h/.cpp: removed AuxiliaryService member/Q_PROPERTY/accessor/include/forward-decl.
- CMakeLists.txt: removed AuxiliaryService compile entry.
- QmlUiAuditTests.cpp: legacyDeadCodePagesRemoved slot; removed ModelMallPage references from visiblePlaceholderSurfacesAreHonest.

## Verification
Canonical build (j6) exit 0; all 5 ctest groups PASS; APP_RUNNING_PID=26948; grep for deleted names = empty (except removal-doc comments).
