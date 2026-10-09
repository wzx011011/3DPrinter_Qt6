import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"
import "../dialogs"

// Aligns with upstream MultiMachinePage (Tabbook with 3 tabs):
//   Tab 1: "Device"   -> MultiMachineManagerPage
//   Tab 2: "Task Sending" -> LocalTaskManagerPage
//   Tab 3: "Task Sent"    -> CloudTaskManagerPage

Item {
    id: root
    required property var multiMachineVm

    // Start 2s refresh timer when page becomes visible (aligns with upstream MultiMachinePage::Show)
    Component.onCompleted: multiMachineVm.startRefreshTimer()
    Component.onDestruction: multiMachineVm.stopRefreshTimer()

    Rectangle {
        anchors.fill: parent
        color: Theme.bgBase
    }

    // Phase 171 (CL-01): destructive-action confirms for removeDevice +
    // stopAllLocalTasks + stopAllCloudTasks (were firing immediately on tap).
    property int _pendingRemoveIndex: -1
    ConfirmDialog {
        id: removeDeviceConfirm
        dialogTitle: qsTr("移除设备")
        message: qsTr("确定要从列表中移除该设备吗？此操作不会影响设备本身。")
        confirmText: qsTr("移除")
        cancelText: qsTr("取消")
        destructive: true
        onAccepted: {
            if (root._pendingRemoveIndex >= 0)
                multiMachineVm.removeDevice(root._pendingRemoveIndex)
            root._pendingRemoveIndex = -1
        }
        onRejected: root._pendingRemoveIndex = -1
    }
    ConfirmDialog {
        id: stopLocalTasksConfirm
        dialogTitle: qsTr("停止本地任务")
        message: qsTr("确定要停止所有本地打印任务吗？进行中的任务将被中止。")
        confirmText: qsTr("停止")
        cancelText: qsTr("取消")
        destructive: true
        onAccepted: multiMachineVm.stopAllLocalTasks()
    }
    ConfirmDialog {
        id: stopCloudTasksConfirm
        dialogTitle: qsTr("停止云任务")
        message: qsTr("确定要停止所有云端打印任务吗？进行中的任务将被中止。")
        confirmText: qsTr("停止")
        cancelText: qsTr("取消")
        destructive: true
        onAccepted: multiMachineVm.stopAllCloudTasks()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // R-P1.E (fake-success disclosure): the device fleet / task send on
        // this page runs on the local mock services -- no real machine is
        // reached. Same disclosure pattern as MonitorPage / HomePage.
        Text {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingXL
            Layout.rightMargin: Theme.spacingXL
            Layout.topMargin: Theme.spacingSM
            wrapMode: Text.WordWrap
            color: Theme.statusWarning
            font.pixelSize: Theme.fontSizeSM
            text: qsTr("演示模式：多设备列表与任务发送为本地模拟数据，不会连接真实设备（真实设备推送依赖 MQTT，当前为外部阻塞项）。")
        }

        // No in-page header/title bar: upstream MultiMachinePage only packs the
        // Tabbook into the main sizer (MultiMachinePage.cpp:11-25), the page
        // opens directly with the side tab strip + content area.

        // ── Side tabs + content area ──
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ── Left sidebar tabs (vertical, aligns with upstream Tabbook wxNB_LEFT) ──
            // Tabbook.cpp:17-25: strip/buttons bg #FEFFFF (dark-mapped to
            // bgSurface), selected fill #BFE1DE (dark-mapped to subtle accent
            // fill), Body_14 / Head_14 bold, 220x46 flat buttons with a 14px
            // monitor_arrow bitmap (Tabbook.cpp:41/:127-136, radius 0).
            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 220
                color: Theme.bgSurface
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Repeater {
                        model: [
                            qsTr("Device"),
                            qsTr("Task Sending"),
                            qsTr("Task Sent")
                        ]
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            Layout.preferredWidth: 220
                            Layout.preferredHeight: 46
                            radius: 0
                            color: tabBar.currentIndex === index ? Theme.accentSubtle : "transparent"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.spacingLG
                                anchors.rightMargin: Theme.spacingMD
                                spacing: Theme.spacingSM
                                Text {
                                    text: modelData
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeLG
                                    font.bold: tabBar.currentIndex === index
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                // 14px monitor_arrow bitmap on every tab button
                                // (Tabbook.cpp:41/:95-97)
                                Image {
                                    width: 14
                                    height: 14
                                    source: "qrc:/qml/assets/icons/monitor_arrow.svg"
                                    sourceSize: Qt.size(14, 14)
                                    fillMode: Image.PreserveAspectFit
                                }
                            }

                            TapHandler {
                                onTapped: tabBar.currentIndex = index
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
                // right border
                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 1
                    color: Theme.borderSubtle
                }
            }

            // ── Tab content area ──
            StackLayout {
                id: tabBar
                currentIndex: 0
                Layout.fillWidth: true
                Layout.fillHeight: true

                // ════════════════════════════════════════
                // Tab 0: Device Manager (MultiMachineManagerPage)
                // ════════════════════════════════════════
                Loader { sourceComponent: deviceTabComponent }

                // ════════════════════════════════════════
                // Tab 1: Task Sending (LocalTaskManagerPage)
                // ════════════════════════════════════════
                Loader { sourceComponent: localTaskTabComponent }

                // ════════════════════════════════════════
                // Tab 2: Task Sent (CloudTaskManagerPage)
                // ════════════════════════════════════════
                Loader { sourceComponent: cloudTaskTabComponent }
            }
        }
    }

    // ══════════════════════════════════════════════════════
    // Component: Device Manager Tab
    // Aligns with upstream MultiMachineManagerPage:
    //   - "Edit Printers" button (top-right)
    //   - Table header (Device Name / Task Name / Status / Actions)
    //   - Device list rows
    //   - "Add" button + tip when no devices
    //   - Pagination controls
    // ══════════════════════════════════════════════════════
    Component {
        id: deviceTabComponent

        Item {
            property var _vm: root.multiMachineVm

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingLG
                spacing: 0

                // ── Toolbar: right-aligned "Edit Printers" only ──
                // Upstream device page has no search box, no sort chips and no
                // refresh button (MultiMachineManagerPage.cpp:285-298, sort
                // lives on the table head buttons :306-359). Block is fixed to
                // DEVICE_ITEM_MAX_WIDTH 900, centered (MultiMachine.hpp:12).
                RowLayout {
                    Layout.preferredWidth: 900
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: Theme.spacingMD
                    Item { Layout.fillWidth: true }
                    // Edit Printers (aligns with upstream m_button_edit:
                    // Confirm/Window style, 90x36 fully rounded,
                    // MultiMachineManagerPage.cpp:287-288/:715-717;
                    // btn_confirm #009688 hover #26A69A -> accent/accentLight)
                    Rectangle {
                        width: 90
                        height: 36
                        radius: Theme.radiusXXL
                        color: editPrintersArea.hovered ? Theme.accentLight : Theme.accent
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Edit Printers")
                            color: Theme.textOnAccent
                            font.pixelSize: Theme.fontSizeMD
                        }
                        HoverHandler { id: editPrintersArea }
                        TapHandler { onTapped: _vm.editPrinters() }
                    }
                }

                // ── Table header (aligns with upstream m_table_head_panel) ──
                // Height = row height 50, columns Device Name 180 / Task Name
                // 180 / Device Status 320 / Actions 180 (MultiMachine.hpp:14,
                // MultiMachineManagerPage.hpp:13-16), Body_13 font, radius 0.
                // Sorting lives on the head buttons with a double-directional
                // arrow icon (MultiMachineManagerPage.cpp:306-359).
                Rectangle {
                    Layout.preferredWidth: 900
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredHeight: 50
                    color: Theme.bgElevated
                    radius: 0
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 15  // DEVICE_LEFT_PADDING_LEFT
                        spacing: 0
                        // Device Name head button toggles name sort asc/desc
                        RowLayout {
                            Layout.preferredWidth: 180
                            Layout.fillHeight: true
                            spacing: 4
                            Text {
                                text: qsTr("Device Name")
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSize13
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Image {
                                width: 14
                                height: 14
                                source: "qrc:/qml/assets/icons/toolbar_double_directional_arrow.svg"
                                sourceSize: Qt.size(14, 14)
                                fillMode: Image.PreserveAspectFit
                                TapHandler { onTapped: _vm.sortDevicesByName() }
                            }
                        }
                        Text { text: qsTr("Task Name"); color: Theme.textTertiary; font.pixelSize: Theme.fontSize13; elide: Text.ElideRight; Layout.preferredWidth: 180 }
                        // Device Status head button toggles state sort asc/desc
                        RowLayout {
                            Layout.preferredWidth: 320
                            Layout.fillHeight: true
                            spacing: 4
                            Text {
                                text: qsTr("Device Status")
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSize13
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Image {
                                width: 14
                                height: 14
                                source: "qrc:/qml/assets/icons/toolbar_double_directional_arrow.svg"
                                sourceSize: Qt.size(14, 14)
                                fillMode: Image.PreserveAspectFit
                                TapHandler { onTapped: _vm.sortDevicesByStatus() }
                            }
                        }
                        Text { text: qsTr("Actions"); color: Theme.textTertiary; font.pixelSize: Theme.fontSize13; elide: Text.ElideRight; Layout.preferredWidth: 180 }
                        Item { Layout.fillWidth: true }
                    }
                }

                // ── Device list (table rows, aligns with upstream MultiMachineItem) ──
                // Fixed DEVICE_ITEM_MAX_WIDTH 900 block, centered
                // (MultiMachine.hpp:12, MultiMachineManagerPage.cpp:492).
                ScrollView {
                    Layout.preferredWidth: 900
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: availableWidth

                    Column {
                        id: deviceList
                        width: parent.width
                        spacing: Theme.spacingXXS
                        Repeater {
                            model: _vm.machineCount
                            delegate: deviceRowDelegate
                        }
                    }
                }

                // ── Empty state (aligns with upstream m_tip_text + m_button_add) ──
                // Upstream tip uses Head_20 with colour (50,58,61)
                // (MultiMachineManagerPage.cpp:380-386) -> textSecondary.
                Item {
                    Layout.preferredWidth: 900
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillHeight: parent.height * 0.4
                    visible: !_vm.hasDevices
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: Theme.spacingLG
                        Text {
                            text: qsTr("Please select the devices you would like to manage here (up to 6 devices)")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeXXL  // upstream Head_20
                            horizontalAlignment: Text.AlignHCenter
                            Layout.preferredWidth: 500
                            wrapMode: Text.Wrap
                        }
                        // Add button (aligns with upstream m_button_add:
                        // Confirm/Window, 90x36 fully rounded,
                        // MultiMachineManagerPage.cpp:388-389/:724-726)
                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: 90
                            height: 36
                            radius: Theme.radiusXXL
                            color: addBtnArea.hovered ? Theme.accentLight : Theme.accent
                            Text {
                                anchors.centerIn: parent
                                text: qsTr("Add")
                                color: Theme.textOnAccent
                                font.pixelSize: Theme.fontSizeMD
                            }
                            HoverHandler { id: addBtnArea }
                            TapHandler { onTapped: _vm.addDevice() }
                        }
                    }
                }

                // ── Pagination controls (aligns with upstream m_flipping_panel) ──
                // 20x20 go_last_plate/go_next_plate icon buttons
                // (MultiMachineManagerPage.cpp:421-442), 50-wide page input +
                // 25x25 "Go" (:457-471). Hidden when total pages <= 1
                // (MultiTaskManagerPage.cpp:1317 same rule).
                RowLayout {
                    Layout.preferredWidth: 900
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: Theme.spacingSM
                    visible: _vm.totalPages > 1
                    Item { Layout.fillWidth: true }
                    // Previous page
                    Item {
                        width: 20
                        height: 20
                        Image {
                            anchors.fill: parent
                            source: "qrc:/qml/assets/icons/go_last_plate.svg"
                            sourceSize: Qt.size(20, 20)
                            fillMode: Image.PreserveAspectFit
                            opacity: _vm.currentPage > 0 ? 1.0 : 0.4
                        }
                        TapHandler {
                            enabled: _vm.currentPage > 0
                            onTapped: _vm.currentPage = _vm.currentPage - 1
                        }
                    }
                    Text {
                        text: (_vm.currentPage + 1) + " / " + _vm.totalPages
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                        Layout.leftMargin: Theme.spacingSM
                        Layout.rightMargin: Theme.spacingSM
                    }
                    // Next page
                    Item {
                        width: 20
                        height: 20
                        Image {
                            anchors.fill: parent
                            source: "qrc:/qml/assets/icons/go_next_plate.svg"
                            sourceSize: Qt.size(20, 20)
                            fillMode: Image.PreserveAspectFit
                            opacity: _vm.currentPage < _vm.totalPages - 1 ? 1.0 : 0.4
                        }
                        TapHandler {
                            enabled: _vm.currentPage < _vm.totalPages - 1
                            onTapped: _vm.currentPage = _vm.currentPage + 1
                        }
                    }
                    // Upstream m_flipping_panel also accepts a page number and
                    // applies it as a one-based page selection.
                    CxTextField {
                        id: devicePageInput
                        Layout.preferredWidth: 46
                        Layout.preferredHeight: 24
                        Layout.leftMargin: Theme.spacingLG
                        text: (_vm.currentPage + 1).toString()
                        horizontalAlignment: TextInput.AlignHCenter
                        validator: IntValidator { bottom: 1; top: Math.max(1, _vm.totalPages) }
                        function applyPage() {
                            var requested = parseInt(text, 10)
                            if (!isNaN(requested))
                                _vm.currentPage = requested - 1
                            text = (_vm.currentPage + 1).toString()
                        }
                        onAccepted: applyPage()
                    }
                    CxButton {
                        compact: true
                        text: qsTr("Go")
                        cxStyle: CxButton.Style.Secondary
                        onClicked: devicePageInput.applyPage()
                    }
                    Item { Layout.fillWidth: true }
                }
            }
        }
    }

    // ══════════════════════════════════════════════════════
    // Delegate: Device row (aligns with upstream MultiMachineItem::doRender)
    // Shows: device name (with offline indicator), task name, status+progress, View button
    // ══════════════════════════════════════════════════════
    Component {
        id: deviceRowDelegate

        Rectangle {
            id: deviceRow
            required property int index
            property var _vm: root.multiMachineVm
            property string _name: _vm.machineName(index)
            property string _taskName: _vm.machineTaskName(index)
            property int _statusInt: _vm.machineStatusInt(index)
            property string _statusText: _vm.machineStatus(index)
            property bool _online: _vm.machineOnline(index)
            property int _progress: _vm.machineProgress(index)
            property string _remaining: _vm.machineRemaining(index)

            width: deviceList.width
            height: 50
            // Hover draws a 1px accent outline only (upstream SetPen(0,150,136)
            // + transparent brush, DrawRoundedRectangle r3,
            // MultiMachineManagerPage.cpp:230-234); background stays row colour.
            color: Theme.bgSurface
            radius: Theme.radiusSM
            border.width: _hovered ? 1 : 0
            border.color: Theme.accent
            property bool _hovered: false
            property bool _editingName: false

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: deviceRow._hovered = true
                onExited: deviceRow._hovered = false
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 15  // DEVICE_LEFT_PADDING_LEFT
                spacing: 0

                // Column 1: Device name (inline editing on double-click, 对齐上游 MultiMachineManagerPage rename)
                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 180
                    spacing: Theme.spacingSM
                    // Online indicator (aligns with upstream state_online)
                    Rectangle {
                        width: 8
                        height: 8
                        radius: Theme.radiusSM
                        color: _online ? Theme.statusSuccess : Theme.textDisabled
                    }
                    // Device name text (visible when not editing)
                    Text {
                        visible: !deviceRow._editingName
                        text: _online ? _name : (_name + " (" + qsTr("Offline") + ")")
                        color: _online ? Theme.textPrimary : Theme.textTertiary
                        font.pixelSize: Theme.fontSizeMD
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    // Inline name editor (visible when editing, 对齐上游 MultiMachineManagerPage rename)
                    CxTextField {
                        id: nameEditor
                        visible: deviceRow._editingName
                        Layout.fillWidth: true
                        text: deviceRow._name
                        font.pixelSize: Theme.fontSizeMD
                        selectByMouse: true
                        onAccepted: {
                            _vm.setMachineName(index, text)
                            deviceRow._editingName = false
                        }
                        onActiveFocusChanged: {
                            if (!activeFocus && deviceRow._editingName) {
                                _vm.setMachineName(index, text)
                                deviceRow._editingName = false
                            }
                        }
                        Component.onCompleted: if (visible) forceActiveFocus()
                    }
                }
                // Double-click handler to enter name editing mode
                MouseArea {
                    anchors.fill: parent
                    z: -1
                    acceptedButtons: Qt.LeftButton
                    onDoubleClicked: deviceRow._editingName = true
                }

                // Column 2: Task name (aligns with upstream subtask_name / "No task")
                Text {
                    Layout.preferredWidth: 180
                    text: _taskName
                    color: _taskName === "No task" || _taskName === qsTr("No task") ? Theme.textDisabled : Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    elide: Text.ElideRight
                }

                // Column 3: Status cell, 320 wide (aligns with upstream
                // MultiMachineManagerPage.cpp:189-217): active states draw a
                // Body_12 "progress%  |  -remaining" line offset 10 from top
                // with a 320x10 r2 progress bar underneath (track #E9E9E9
                // dark-mapped, fill #009688 -> fixed accent); other states
                // draw only the status text. Remaining time is folded into
                // the progress line (upstream get_left_time :250-265).
                Item {
                    Layout.preferredWidth: 320
                    Layout.fillHeight: true
                    property bool _active: _statusInt >= 3 && _statusInt <= 6
                    // Active progress line
                    Text {
                        visible: parent._active
                        x: 0
                        y: 10
                        text: _progress + "%  |  -" + _remaining
                        color: Theme.accent  // upstream wxColour(0,150,136)
                        font.pixelSize: Theme.fontSizeMD
                    }
                    // Progress bar 320x10 r2
                    Rectangle {
                        visible: parent._active
                        x: 0
                        y: 30
                        width: 320
                        height: 10
                        radius: Theme.radiusXS
                        color: Theme.bgElevated  // dark-mapped track #E9E9E9
                        Rectangle {
                            width: parent.width * (_progress / 100.0)
                            height: parent.height
                            radius: Theme.radiusXS
                            color: Theme.accent  // fixed fill, no per-state tint
                        }
                    }
                    // Non-active states: status text only
                    Text {
                        visible: !parent._active
                        anchors.verticalCenter: parent.verticalCenter
                        text: _statusText
                        color: {
                            if (_statusInt === 1) return Theme.statusSuccess;     // finish (green)
                            if (_statusInt === 2) return Theme.statusError;       // failed (red)
                            return Theme.textPrimary;
                        }
                        font.pixelSize: Theme.fontSize13
                        elide: Text.ElideRight
                    }
                }

                // Column 4: View button (aligns with upstream "View" 90x38 r6
                // white + dark border Body_14, MultiMachineManagerPage.cpp:221-226
                // -> EVT_MULTI_DEVICE_VIEW)
                Rectangle {
                    Layout.preferredWidth: 90
                    Layout.preferredHeight: 38
                    radius: Theme.radiusMD
                    color: Theme.bgElevated
                    border.color: Theme.borderDefault
                    border.width: 1
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        anchors.centerIn: parent
                        text: qsTr("View")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeLG
                    }
                    TapHandler {
                        onTapped: _vm.viewMachine(index)
                    }
                }
            }

            // Remove button (small X on hover, top-right)
            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 4
                width: 16
                height: 16
                radius: Theme.radiusLG
                color: _hovered ? Theme.bgPressed : "transparent"
                visible: _hovered
                Text {
                    anchors.centerIn: parent
                    text: "x"
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeXS
                }
                TapHandler {
                    // Phase 171 (CL-01): confirm before removing device.
                    onTapped: {
                        root._pendingRemoveIndex = index
                        removeDeviceConfirm.open()
                    }
                }
            }
        }
    }

    // ══════════════════════════════════════════════════════
    // Component: Task Sending Tab (LocalTaskManagerPage)
    // Aligns with upstream table: Project Name / Printer / Status / Send Time / Actions
    // Per-task Cancel button for pending/sending tasks (aligns with MultiTaskItem::onCancel)
    // Stop All for batch cancel of selected tasks (aligns with btn_stop_all)
    // ══════════════════════════════════════════════════════
    Component {
        id: localTaskTabComponent

        Item {
            property var _vm: root.multiMachineVm

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingLG
                spacing: 0

                // ── Toolbar ──
                // Task blocks use CLOUD_TASK_ITEM_MAX_WIDTH 1100, centered
                // (MultiTaskManagerPage.hpp:20).
                RowLayout {
                    Layout.preferredWidth: 1100
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: Theme.spacingMD
                    Item { Layout.fillWidth: true }
                    Text {
                        text: _vm.localSelectedCount > 0
                              ? qsTr("%1 selected").arg(_vm.localSelectedCount)
                              : ""
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                    }
                    // Send All button (对齐上游 SendMultiMachinePage send all)
                    Rectangle {
                        visible: _vm.hasLocalTasks && _vm.onlineMachineCount > 0
                        width: 80
                        height: Theme.controlHeightSM
                        radius: Theme.radiusSM
                        color: sendAllBtnArea.containsMouse ? Theme.accentSubtle : Theme.accent
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Send All")
                            color: Theme.textOnAccent
                            font.pixelSize: Theme.fontSizeSM
                            font.bold: true
                        }
                        HoverHandler { id: sendAllBtnArea }
                        TapHandler { onTapped: _vm.sendAllTasksToDevice() }
                    }
                    // Send to Device button (aligns with upstream SendMultiMachinePage)
                    Rectangle {
                        visible: _vm.hasLocalTasks && _vm.onlineMachineCount > 0
                        width: 110
                        height: Theme.controlHeightSM
                        radius: Theme.radiusSM
                        color: Theme.accent
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Send to Device")
                            color: Theme.textOnAccent
                            font.pixelSize: Theme.fontSizeSM
                        }
                        TapHandler { onTapped: sendToDeviceDialog.open() }
                    }
                    // Stop All button (aligns with upstream btn_stop_all)
                    Rectangle {
                        visible: _vm.localSelectedCount > 0
                        width: 80
                        height: Theme.controlHeightSM
                        radius: Theme.radiusSM
                        color: Theme.statusError
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Stop All")
                            color: Theme.textOnAccent
                            font.pixelSize: Theme.fontSizeSM
                        }
                        TapHandler {
                            // Phase 171 (CL-01): confirm before stopping all local tasks.
                            onTapped: stopLocalTasksConfirm.open()
                        }
                    }
                    Item { Layout.fillWidth: true }
                }

                // ── Table header ──
                Rectangle {
                    Layout.preferredWidth: 1100
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredHeight: 34
                    color: Theme.bgElevated
                    radius: Theme.radiusSM
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingXL
                        anchors.rightMargin: Theme.spacingXL
                        spacing: 0
                        Text { text: qsTr("Project Name"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.fillWidth: true }
                        Text { text: qsTr("Printer"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 130 }
                        Text { text: qsTr("Status"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 130 }
                        Text { text: qsTr("Send Time"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 130 }
                        Text { text: qsTr("Remaining"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 80 }
                        Text { text: qsTr("Progress"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 90 }
                        Text { text: qsTr("Actions"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 70 }
                    }
                }

                // ── Task list ──
                ScrollView {
                    Layout.preferredWidth: 1100
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: availableWidth

                    Column {
                        id: localTaskList
                        width: parent.width
                        spacing: Theme.spacingXXS
                        Repeater {
                            model: _vm.localTaskCount
                            delegate: localTaskRowDelegate
                        }
                    }
                }

                // ── Empty state ──
                // Upstream "There are no tasks to be sent!" Head_24 with
                // colour (50,58,61) (MultiTaskManagerPage.cpp:668-674)
                // -> qsTr Chinese + 24px + textSecondary.
                Item {
                    Layout.preferredWidth: 1100
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillHeight: parent.height * 0.4
                    visible: !_vm.hasLocalTasks
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: Theme.spacingLG
                        Text {
                            text: qsTr("没有要发送的任务！")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeDisplay  // upstream Head_24
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
            }
        }
    }

    // ══════════════════════════════════════════════════════
    // Delegate: Local task row
    // Selection only for pending/sending (aligns with upstream state_local_task <= 1)
    // Cancel button only for pending/sending (aligns with MultiTaskItem::onCancel)
    // ══════════════════════════════════════════════════════
    Component {
        id: localTaskRowDelegate

        Rectangle {
            id: localRow
            required property int index
            property var _vm: root.multiMachineVm
            property string _projectName: _vm.localTaskProjectName(index)
            property string _devName: _vm.localTaskDevName(index)
            property int _status: _vm.localTaskStatus(index)
            property string _statusText: _vm.localTaskStatusText(index)
            property int _progress: _vm.localTaskProgress(index)
            property string _sendTime: _vm.localTaskSendTime(index)
            property string _remaining: _vm.localTaskRemaining(index)
            property bool _selected: _vm.localTaskSelected(index)
            property bool _canSelect: _status === 0 || _status === 1  // pending or sending
            property bool _canCancel: _status === 0 || _status === 1  // pending or sending

            width: localTaskList.width
            height: 50
            // Hover draws a 1px accent outline only (upstream
            // MultiTaskManagerPage.cpp:453-457); selected state shows on the
            // checkbox only, no whole-row tint (MultiTaskManagerPage.cpp:314-458).
            color: Theme.bgSurface
            radius: Theme.radiusSM
            border.width: _hovered ? 1 : 0
            border.color: Theme.accent
            property bool _hovered: false

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: localRow._hovered = true
                onExited: localRow._hovered = false
                onClicked: if (_canSelect) _vm.selectLocalTask(index)
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingXL
                anchors.rightMargin: Theme.spacingXL
                spacing: 0

                // Checkbox + project name
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingMD
                    // 18px check bitmaps (aligns with upstream check_on /
                    // check_off_focused / check_off_disabled,
                    // MultiTaskManagerPage.cpp:30-32)
                    Image {
                        width: 18
                        height: 18
                        source: _selected ? "qrc:/qml/assets/icons/check_on.svg"
                                : (_canSelect ? "qrc:/qml/assets/icons/check_off_focused.svg"
                                              : "qrc:/qml/assets/icons/check_off_disabled.svg")
                        sourceSize: Qt.size(18, 18)
                        fillMode: Image.PreserveAspectFit
                    }
                    Text {
                        text: _projectName
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMD
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                Text {
                    Layout.preferredWidth: 130
                    text: _devName
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    elide: Text.ElideRight
                }

                Text {
                    Layout.preferredWidth: 130
                    text: _statusText
                    color: {
                        if (_status === 6) return Theme.statusSuccess;  // success
                        if (_status === 7 || _status === 3 || _status === 4) return Theme.statusError; // failed/cancel
                        if (_status === 1 || _status === 5) return Theme.statusInfo; // sending/printing
                        return Theme.textSecondary;
                    }
                    font.pixelSize: Theme.fontSizeSM
                }

                Text {
                    Layout.preferredWidth: 130
                    text: _sendTime
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeXS
                }

                // Remaining time (aligns with upstream get_left_time)
                Text {
                    Layout.preferredWidth: 80
                    text: (_status === 1 || _status === 5) ? _remaining : "--"
                    color: (_status === 1 || _status === 5) ? Theme.textSecondary : Theme.textDisabled
                    font.pixelSize: Theme.fontSizeXS
                    font.family: Theme.fontMono
                }

                // Progress bar
                Item {
                    Layout.preferredWidth: 90
                    height: parent.height
                    RowLayout {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Theme.spacingSM
                        Rectangle {
                            width: 60
                            height: 6
                            radius: Theme.radiusSM
                            color: Theme.bgElevated
                            Rectangle {
                                width: parent.width * (_progress / 100.0)
                                height: parent.height
                                radius: Theme.radiusSM
                                color: Theme.accent  // fixed fill, no per-state tint
                            }
                        }
                        Text {
                            text: _progress + "%"
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeXS
                            Layout.preferredWidth: 30
                        }
                    }
                }

                // Cancel button (aligns with upstream MultiTaskItem
                // m_button_cancel: 70x35 r6, white bg + dark border
                // (38,46,48) + dark text, MultiTaskManagerPage.cpp:59-65)
                Rectangle {
                    Layout.preferredWidth: 70
                    Layout.preferredHeight: 35
                    radius: Theme.radiusMD
                    color: _hovered && _canCancel ? Theme.bgHover : Theme.bgElevated
                    border.color: Theme.borderDefault
                    border.width: 1
                    anchors.verticalCenter: parent.verticalCenter
                    visible: _canCancel
                    opacity: _canCancel ? 1.0 : 0.4
                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Cancel")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMD
                    }
                    TapHandler {
                        enabled: _canCancel
                        onTapped: _vm.cancelLocalTask(index)
                    }
                }
            }
        }
    }

    // ══════════════════════════════════════════════════════
    // Component: Task Sent Tab (CloudTaskManagerPage)
    // Aligns with upstream CloudTaskManagerPage:
    //   - Per-task checkbox (only for printing tasks, aligns with EVT_MULTI_DEVICE_SELECTED)
    //   - Pause/Resume button per task (aligns with MultiTaskItem onPause/onResume)
    //   - Stop button per task (aligns with MultiTaskItem onStop)
    //   - Neutral Pause/Resume/Stop control strip below the list
    //     (aligns with upstream m_ctrl_btn_panel)
    //   - Pagination (aligns with upstream m_flipping_panel, m_count_page_item=10)
    // ══════════════════════════════════════════════════════
    Component {
        id: cloudTaskTabComponent

        Item {
            property var _vm: root.multiMachineVm

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingLG
                spacing: 0

                // No top toolbar: upstream puts the Pause/Resume/Stop control
                // strip BELOW the list as a neutral button row
                // (MultiTaskManagerPage.cpp:1154-1182).

                // ── Table header ──
                // Task blocks use CLOUD_TASK_ITEM_MAX_WIDTH 1100, centered
                // (MultiTaskManagerPage.hpp:20).
                Rectangle {
                    Layout.preferredWidth: 1100
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredHeight: 34
                    color: Theme.bgElevated
                    radius: Theme.radiusSM
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingXL
                        anchors.rightMargin: Theme.spacingXL
                        spacing: 0
                        Text { text: qsTr("Project Name"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.fillWidth: true }
                        Text { text: qsTr("Printer"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 130 }
                        Text { text: qsTr("Status"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 130 }
                        Text { text: qsTr("Send Time"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 130 }
                        Text { text: qsTr("Remaining"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 80 }
                        Text { text: qsTr("Progress"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 80 }
                        Text { text: qsTr("Actions"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 150 }
                    }
                }

                // ── Cloud task list (page-aware, aligns with upstream m_count_page_item=10) ──
                ScrollView {
                    Layout.preferredWidth: 1100
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: availableWidth

                    Column {
                        id: cloudTaskList
                        width: parent.width
                        spacing: Theme.spacingXXS
                        Repeater {
                            model: _vm.pagedCloudTaskCount
                            delegate: cloudTaskRowDelegate
                        }
                    }
                }

                // ── Empty state ──
                // Upstream "No historical tasks!" Head_24 with colour
                // (50,58,61) (MultiTaskManagerPage.cpp:1040-1046)
                // -> qsTr Chinese + 24px + textSecondary.
                Item {
                    Layout.preferredWidth: 1100
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillHeight: parent.height * 0.4
                    visible: !_vm.hasCloudTasks
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: Theme.spacingLG
                        Text {
                            text: qsTr("没有历史任务！")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeDisplay  // upstream Head_24
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }

                // ── Pagination controls (aligns with upstream CloudTaskManagerPage m_flipping_panel) ──
                // 20x20 go_last_plate/go_next_plate icon buttons + 50-wide
                // page input + 25x25 r5 "Go" (MultiTaskManagerPage.cpp:
                // 1083-1147). Hidden when total pages <= 1 (:1317).
                RowLayout {
                    Layout.preferredWidth: 1100
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: Theme.spacingSM
                    visible: _vm.cloudTotalPages > 1
                    Item { Layout.fillWidth: true }
                    // Previous page
                    Item {
                        width: 20
                        height: 20
                        Image {
                            anchors.fill: parent
                            source: "qrc:/qml/assets/icons/go_last_plate.svg"
                            sourceSize: Qt.size(20, 20)
                            fillMode: Image.PreserveAspectFit
                            opacity: _vm.cloudCurrentPage > 0 ? 1.0 : 0.4
                        }
                        TapHandler {
                            enabled: _vm.cloudCurrentPage > 0
                            onTapped: _vm.cloudCurrentPage = _vm.cloudCurrentPage - 1
                        }
                    }
                    Text {
                        text: (_vm.cloudCurrentPage + 1) + " / " + _vm.cloudTotalPages
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                        Layout.leftMargin: Theme.spacingSM
                        Layout.rightMargin: Theme.spacingSM
                    }
                    // Next page
                    Item {
                        width: 20
                        height: 20
                        Image {
                            anchors.fill: parent
                            source: "qrc:/qml/assets/icons/go_next_plate.svg"
                            sourceSize: Qt.size(20, 20)
                            fillMode: Image.PreserveAspectFit
                            opacity: _vm.cloudCurrentPage < _vm.cloudTotalPages - 1 ? 1.0 : 0.4
                        }
                        TapHandler {
                            enabled: _vm.cloudCurrentPage < _vm.cloudTotalPages - 1
                            onTapped: _vm.cloudCurrentPage = _vm.cloudCurrentPage + 1
                        }
                    }
                    // Page number input + Go (one-based selection, upstream
                    // MultiTaskManagerPage.cpp:1123-1140)
                    CxTextField {
                        id: cloudPageInput
                        Layout.preferredWidth: 50
                        Layout.preferredHeight: 24
                        Layout.leftMargin: Theme.spacingLG
                        text: (_vm.cloudCurrentPage + 1).toString()
                        horizontalAlignment: TextInput.AlignHCenter
                        validator: IntValidator { bottom: 1; top: Math.max(1, _vm.cloudTotalPages) }
                        function applyPage() {
                            var requested = parseInt(text, 10)
                            if (!isNaN(requested))
                                _vm.cloudCurrentPage = requested - 1
                            text = (_vm.cloudCurrentPage + 1).toString()
                        }
                        onAccepted: applyPage()
                    }
                    // 25x25 r5 neutral Go (upstream m_page_num_enter
                    // ctrl_bg white/pressed #969696, :1133-1137)
                    Rectangle {
                        Layout.leftMargin: Theme.spacingSM
                        width: 25
                        height: 25
                        radius: Theme.radiusMD
                        color: cloudGoArea.hovered ? Theme.bgHover : Theme.bgElevated
                        border.color: Theme.borderDefault
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Go")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                        }
                        HoverHandler { id: cloudGoArea }
                        TapHandler { onTapped: cloudPageInput.applyPage() }
                    }
                    Item { Layout.fillWidth: true }
                }

                // ── Control strip below the list (aligns with upstream
                // m_ctrl_btn_panel: "n selected" + neutral Pause/Resume/Stop,
                // MultiTaskManagerPage.cpp:1154-1182) ──
                RowLayout {
                    Layout.preferredWidth: 1100
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: Theme.spacingMD
                    visible: _vm.hasCloudTasks
                    Text {
                        text: _vm.cloudSelectedCount > 0
                              ? qsTr("%1 selected").arg(_vm.cloudSelectedCount)
                              : ""
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                        Layout.leftMargin: 15
                    }
                    // Neutral Pause (aligns with upstream btn_pause_all, no "All" suffix)
                    Rectangle {
                        visible: _vm.cloudSelectedCount > 0
                        width: 58
                        height: 24
                        radius: Theme.radiusMD
                        color: Theme.bgElevated
                        border.color: Theme.borderDefault
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Pause")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                        }
                        TapHandler { onTapped: _vm.pauseAllCloudTasks() }
                    }
                    // Neutral Resume (aligns with upstream btn_continue_all)
                    Rectangle {
                        visible: _vm.cloudSelectedCount > 0
                        width: 58
                        height: 24
                        radius: Theme.radiusMD
                        color: Theme.bgElevated
                        border.color: Theme.borderDefault
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Resume")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                        }
                        TapHandler { onTapped: _vm.resumeAllCloudTasks() }
                    }
                    // Neutral Stop (aligns with upstream btn_stop_all)
                    Rectangle {
                        visible: _vm.cloudSelectedCount > 0
                        width: 58
                        height: 24
                        radius: Theme.radiusMD
                        color: Theme.bgElevated
                        border.color: Theme.borderDefault
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Stop")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                        }
                        TapHandler {
                            // Phase 171 (CL-01): confirm before stopping all cloud tasks.
                            onTapped: stopCloudTasksConfirm.open()
                        }
                    }
                    Item { Layout.fillWidth: true }
                }
            }
        }
    }

    // ══════════════════════════════════════════════════════
    // Delegate: Cloud task row
    // Checkbox only for printing tasks (aligns with upstream EVT_MULTI_DEVICE_SELECTED state_cloud_task == 0)
    // Pause/Resume toggle (aligns with MultiTaskItem onPause/onResume + can_pause/can_resume)
    // Stop button (aligns with MultiTaskItem onStop)
    // ══════════════════════════════════════════════════════
    Component {
        id: cloudTaskRowDelegate

        Rectangle {
            id: cloudRow
            required property int index
            property var _vm: root.multiMachineVm
            property string _projectName: _vm.pagedCloudTaskProjectName(index)
            property string _devName: _vm.pagedCloudTaskDevName(index)
            property int _status: _vm.pagedCloudTaskStatus(index)
            property string _statusText: _vm.pagedCloudTaskStatusText(index)
            property int _progress: _vm.pagedCloudTaskProgress(index)
            property string _sendTime: _vm.pagedCloudTaskSendTime(index)
            property string _remaining: _vm.pagedCloudTaskRemaining(index)
            property bool _selected: _vm.pagedCloudTaskSelected(index)
            property bool _isPrinting: _status === 0
            property bool _isPaused: _status === 4
            property bool _canSelect: _isPrinting || _isPaused  // aligns with upstream state_cloud_task == 0 selection
            property bool _canPause: _isPrinting
            property bool _canResume: _isPaused
            property bool _canStop: _isPrinting || _isPaused

            width: cloudTaskList.width
            height: 50
            // Hover draws a 1px accent outline only (upstream
            // MultiTaskManagerPage.cpp:453-457); selected state shows on the
            // checkbox only, no whole-row tint (MultiTaskManagerPage.cpp:314-458).
            color: Theme.bgSurface
            radius: Theme.radiusSM
            border.width: _hovered ? 1 : 0
            border.color: Theme.accent
            property bool _hovered: false

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: cloudRow._hovered = true
                onExited: cloudRow._hovered = false
                onClicked: if (_canSelect) _vm.selectCloudTask(index)
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingXL
                anchors.rightMargin: Theme.spacingXL
                spacing: 0

                // Checkbox + project name
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingMD
                    // 18px check bitmaps (aligns with upstream check_on /
                    // check_off_focused / check_off_disabled,
                    // MultiTaskManagerPage.cpp:30-32)
                    Image {
                        width: 18
                        height: 18
                        source: _selected ? "qrc:/qml/assets/icons/check_on.svg"
                                : (_canSelect ? "qrc:/qml/assets/icons/check_off_focused.svg"
                                              : "qrc:/qml/assets/icons/check_off_disabled.svg")
                        sourceSize: Qt.size(18, 18)
                        fillMode: Image.PreserveAspectFit
                    }
                    Text {
                        text: _projectName
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMD
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                Text {
                    Layout.preferredWidth: 130
                    text: _devName
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    elide: Text.ElideRight
                }

                // Status text + inline pause/resume toggle (aligns with upstream m_button_pause/m_button_resume)
                RowLayout {
                    Layout.preferredWidth: 130
                    spacing: Theme.spacingSM
                    Text {
                        text: {
                            if (_isPaused) return qsTr("Paused")
                            return _statusText
                        }
                        color: {
                            if (_status === 1) return Theme.statusSuccess;  // finish
                            if (_status === 2) return Theme.statusError;    // failed
                            if (_isPrinting) return Theme.statusInfo;       // printing
                            if (_isPaused) return Theme.statusWarning;      // paused
                            return Theme.textSecondary;
                        }
                        font.pixelSize: Theme.fontSizeSM
                    }
                }

                Text {
                    Layout.preferredWidth: 130
                    text: _sendTime
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeXS
                }

                // Remaining time (aligns with upstream get_left_time)
                Text {
                    Layout.preferredWidth: 80
                    text: _isPrinting ? _remaining : "--"
                    color: _isPrinting ? Theme.textSecondary : Theme.textDisabled
                    font.pixelSize: Theme.fontSizeXS
                    font.family: Theme.fontMono
                }

                // Progress bar
                Item {
                    Layout.preferredWidth: 80
                    height: parent.height
                    RowLayout {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Theme.spacingSM
                        Rectangle {
                            width: 50
                            height: 6
                            radius: Theme.radiusSM
                            color: Theme.bgElevated
                            Rectangle {
                                width: parent.width * (_progress / 100.0)
                                height: parent.height
                                radius: Theme.radiusSM
                                color: {
                                    if (_status === 1) return Theme.statusSuccess;
                                    if (_status === 2) return Theme.statusError;
                                    return Theme.statusInfo;
                                }
                            }
                        }
                        Text {
                            text: _progress + "%"
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeXS
                            Layout.preferredWidth: 30
                        }
                    }
                }

                // Actions column (aligns with upstream MultiTaskItem
                // m_button_pause/m_button_resume/m_button_stop: 70x35 r6;
                // Pause/Stop white bg + dark border + dark text, Resume
                // #009688 bg + white text, MultiTaskManagerPage.cpp:38-93)
                Row {
                    Layout.preferredWidth: 150
                    spacing: 4
                    anchors.verticalCenter: parent.verticalCenter
                    // Pause button (aligns with upstream MultiTaskItem::onPause)
                    Rectangle {
                        visible: _canPause
                        width: 70
                        height: 35
                        radius: Theme.radiusMD
                        color: _hovered ? Theme.bgHover : Theme.bgElevated
                        border.color: Theme.borderDefault
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Pause")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                        }
                        TapHandler { onTapped: _vm.pauseCloudTask(index) }
                    }
                    // Resume button (aligns with upstream MultiTaskItem::onResume:
                    // teal bg + white text; #009688/#26A69A -> accent/accentLight)
                    Rectangle {
                        visible: _canResume
                        width: 70
                        height: 35
                        radius: Theme.radiusMD
                        color: resumeBtnArea.hovered ? Theme.accentLight : Theme.accent
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Resume")
                            color: Theme.textOnAccent
                            font.pixelSize: Theme.fontSizeMD
                        }
                        HoverHandler { id: resumeBtnArea }
                        TapHandler { onTapped: _vm.resumeCloudTask(index) }
                    }
                    // Stop button (aligns with upstream MultiTaskItem::onStop)
                    Rectangle {
                        visible: _canStop
                        width: 70
                        height: 35
                        radius: Theme.radiusMD
                        color: _hovered ? Theme.bgHover : Theme.bgElevated
                        border.color: Theme.borderDefault
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Stop")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                        }
                        TapHandler { onTapped: _vm.stopCloudTask(index) }
                    }
                }
            }
        }
    }

    // ══════════════════════════════════════════════════════
    // Dialog: Send Task to Device (aligns with upstream MultiMachinePickPage)
    // ══════════════════════════════════════════════════════
    Dialog {
        id: sendToDeviceDialog
        title: qsTr("Select Device to Send")
        modal: true
        anchors.centerIn: parent
        width: 360
        height: Math.min(400, root.height * 0.6)

        property int selectedDeviceIndex: -1

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            // Task selector
            Text { text: qsTr("Task:"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD }
            CxComboBox {
                id: taskSelector
                Layout.fillWidth: true
                model: root.multiMachineVm.localTaskCount
                textRole: ""
                delegate: ItemDelegate {
                    width: taskSelector.width
                    contentItem: Text {
                        text: root.multiMachineVm.localTaskProjectName(index)
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                    }
                    highlighted: taskSelector.highlightedIndex === index
                }
                displayText: taskSelector.currentIndex >= 0
                              ? root.multiMachineVm.localTaskProjectName(taskSelector.currentIndex)
                              : qsTr("Select task...")
            }

            // Online device list
            Text { text: qsTr("Online Devices:"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD }
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                Column {
                    width: parent.width
                    spacing: 4
                    Repeater {
                        model: root.multiMachineVm.onlineMachineCount
                        delegate: Rectangle {
                            required property int index
                            width: parent.width
                            height: 36
                            radius: Theme.radiusSM
                            color: sendToDeviceDialog.selectedDeviceIndex === index ? Theme.accentSubtle : Theme.bgElevated
                            border.color: sendToDeviceDialog.selectedDeviceIndex === index ? Theme.accent : Theme.borderDefault
                            border.width: 1

                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 12
                                spacing: 8
                                // Online indicator
                                Rectangle { width: 8; height: 8; radius: Theme.radiusSM; color: Theme.statusSuccess }
                                Text {
                                    text: root.multiMachineVm.onlineMachineName(index)
                                    color: sendToDeviceDialog.selectedDeviceIndex === index ? Theme.accent : Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeMD
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: sendToDeviceDialog.selectedDeviceIndex = index
                            }
                        }
                    }
                }
            }

            // Send button
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Item { Layout.fillWidth: true }
                // Phase 173 (CL-03): Cancel button migrated from Rectangle+
                // Text+MouseArea pseudo-button to CxButton (Secondary style,
                // compact). Gains press-scale, focus border, ToolTip.
                CxButton {
                    text: qsTr("Cancel")
                    compact: true
                    cxStyle: CxButton.Style.Secondary
                    onClicked: sendToDeviceDialog.close()
                }
                // Phase 173 (CL-03): Send button migrated from Rectangle+
                // Text+MouseArea pseudo-button to CxButton (Primary style,
                // compact). Gains press-scale, focus border, ToolTip.
                CxButton {
                    text: qsTr("Send")
                    compact: true
                    cxStyle: CxButton.Style.Primary
                    onClicked: {
                        if (taskSelector.currentIndex >= 0 && sendToDeviceDialog.selectedDeviceIndex >= 0) {
                            root.multiMachineVm.sendTaskToDevice(taskSelector.currentIndex, sendToDeviceDialog.selectedDeviceIndex)
                            sendToDeviceDialog.close()
                        }
                    }
                }
            }
        }
        onOpened: { selectedDeviceIndex = -1; taskSelector.currentIndex = 0 }
    }
}
