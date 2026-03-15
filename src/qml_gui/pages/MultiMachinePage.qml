import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Aligns with upstream MultiMachinePage (Tabbook with 3 tabs):
//   Tab 1: "Device"   -> MultiMachineManagerPage
//   Tab 2: "Task Sending" -> LocalTaskManagerPage
//   Tab 3: "Task Sent"    -> CloudTaskManagerPage

Item {
    id: root
    required property var multiMachineVm

    Rectangle {
        anchors.fill: parent
        color: Theme.bgBase
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Page header (top bar) ──
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.titleBarHeight
            color: Theme.chromeSurface
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingXL
                anchors.rightMargin: Theme.spacingXL
                spacing: Theme.spacingLG
                Text {
                    text: qsTr("Multi-Device Print")
                    color: Theme.chromeText
                    font.pixelSize: Theme.fontSizeLG
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: multiMachineVm.selectedCountText
                    color: Theme.chromeTextMuted
                    font.pixelSize: Theme.fontSizeSM
                }
            }
            // bottom border
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: Theme.chromeBorder
            }
        }

        // ── Side tabs + content area ──
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ── Left sidebar tabs (vertical, aligns with upstream Tabbook wxNB_LEFT) ──
            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 180
                color: Theme.bgInset
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
                            Layout.fillWidth: true
                            Layout.preferredHeight: 42
                            color: tabBar.currentIndex === index ? Theme.bgSurface : "transparent"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.spacingXL
                                spacing: Theme.spacingMD
                                // Active indicator bar
                                Rectangle {
                                    width: 3
                                    height: 18
                                    radius: 1.5
                                    color: tabBar.currentIndex === index ? Theme.accent : "transparent"
                                }
                                Text {
                                    text: modelData
                                    color: tabBar.currentIndex === index ? Theme.textPrimary : Theme.textTertiary
                                    font.pixelSize: Theme.fontSizeMD
                                    font.bold: tabBar.currentIndex === index
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

                // ── Toolbar: Search + Edit Printers button ──
                RowLayout {
                    Layout.fillWidth: true
                    Layout.bottomMargin: Theme.spacingMD
                    spacing: Theme.spacingMD

                    // Search bar (对齐上游 search/filter)
                    Rectangle {
                        Layout.preferredWidth: 200
                        Layout.preferredHeight: 30
                        radius: Theme.radiusMD
                        color: Theme.bgElevated
                        border.width: 1
                        border.color: searchField.activeFocus ? Theme.borderFocus : Theme.borderSubtle

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 6

                            Text {
                                text: "\u2315"
                                color: Theme.textTertiary
                                font.pixelSize: 13
                            }

                            TextField {
                                id: searchField
                                Layout.fillWidth: true
                                background: null
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeMD
                                placeholderText: qsTr("搜索设备...")
                                placeholderTextColor: Theme.textTertiary
                                selectByMouse: true
                                onTextChanged: _vm.searchText = text
                            }

                            Text {
                                visible: searchField.text.length > 0
                                text: "\u2715"
                                color: Theme.textTertiary
                                font.pixelSize: 10
                                TapHandler {
                                    onTapped: {
                                        searchField.text = ""
                                        searchField.forceActiveFocus()
                                    }
                                }
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }
                    // Sort by name
                    Rectangle {
                        width: 90
                        height: Theme.controlHeightSM
                        radius: Theme.radiusSM
                        color: Theme.bgElevated
                        border.color: Theme.borderDefault
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Device Name")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeXS
                        }
                        TapHandler { onTapped: _vm.sortDevicesByName() }
                    }
                    // Sort by status
                    Rectangle {
                        width: 80
                        height: Theme.controlHeightSM
                        radius: Theme.radiusSM
                        color: Theme.bgElevated
                        border.color: Theme.borderDefault
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Status")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeXS
                        }
                        TapHandler { onTapped: _vm.sortDevicesByStatus() }
                    }
                    Item { Layout.fillWidth: true }
                    // Edit Printers button (aligns with upstream m_button_edit)
                    Rectangle {
                        width: 100
                        height: Theme.controlHeightMD
                        radius: Theme.radiusMD
                        color: Theme.bgElevated
                        border.color: Theme.borderDefault
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Edit Printers")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeSM
                        }
                        TapHandler { onTapped: _vm.editPrinters() }
                    }
                }

                // ── Table header (aligns with upstream m_table_head_panel) ──
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    color: Theme.bgElevated
                    radius: Theme.radiusSM
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingXL
                        anchors.rightMargin: Theme.spacingXL
                        spacing: 0
                        Text { text: qsTr("Device Name"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.fillWidth: true; Layout.preferredWidth: 180 }
                        Text { text: qsTr("Task Name"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.fillWidth: true; Layout.preferredWidth: 160 }
                        Text { text: qsTr("Status"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.fillWidth: true; Layout.preferredWidth: 180 }
                        Text { text: qsTr("Remaining"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 90 }
                        Text { text: qsTr("Actions"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 80 }
                    }
                }

                // ── Device list (table rows, aligns with upstream MultiMachineItem) ──
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: availableWidth

                    Column {
                        id: deviceList
                        width: parent.width
                        spacing: 1
                        Repeater {
                            model: _vm.machineCount
                            delegate: deviceRowDelegate
                        }
                    }
                }

                // ── Empty state (aligns with upstream m_tip_text + m_button_add) ──
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: parent.height * 0.4
                    visible: !_vm.hasDevices
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: Theme.spacingLG
                        Text {
                            text: qsTr("Please select the devices you would like to manage here (up to 6 devices)")
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeXL
                            horizontalAlignment: Text.AlignHCenter
                            Layout.preferredWidth: 500
                            wrapMode: Text.Wrap
                        }
                        // Add button (aligns with upstream m_button_add)
                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: 90
                            height: Theme.controlHeightMD
                            radius: Theme.radiusMD
                            color: Theme.accent
                            Text {
                                anchors.centerIn: parent
                                text: qsTr("Add")
                                color: Theme.textOnAccent
                                font.pixelSize: Theme.fontSizeSM
                                font.bold: true
                            }
                            TapHandler { onTapped: _vm.addDevice() }
                        }
                    }
                }

                // ── Pagination controls (aligns with upstream m_flipping_panel) ──
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.spacingSM
                    visible: _vm.totalPages > 1
                    Item { Layout.fillWidth: true }
                    // Previous page
                    Rectangle {
                        width: 24
                        height: 24
                        radius: Theme.radiusSM
                        color: _vm.currentPage > 0 ? Theme.bgElevated : Theme.bgPanel
                        Text {
                            anchors.centerIn: parent
                            text: "<"
                            color: _vm.currentPage > 0 ? Theme.textPrimary : Theme.textDisabled
                            font.pixelSize: Theme.fontSizeSM
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
                    Rectangle {
                        width: 24
                        height: 24
                        radius: Theme.radiusSM
                        color: _vm.currentPage < _vm.totalPages - 1 ? Theme.bgElevated : Theme.bgPanel
                        Text {
                            anchors.centerIn: parent
                            text: ">"
                            color: _vm.currentPage < _vm.totalPages - 1 ? Theme.textPrimary : Theme.textDisabled
                            font.pixelSize: Theme.fontSizeSM
                        }
                        TapHandler {
                            enabled: _vm.currentPage < _vm.totalPages - 1
                            onTapped: _vm.currentPage = _vm.currentPage + 1
                        }
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
            color: _hovered ? Theme.bgHover : Theme.bgSurface
            radius: Theme.radiusSM
            property bool _hovered: false

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: deviceRow._hovered = true
                onExited: deviceRow._hovered = false
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingXL
                anchors.rightMargin: Theme.spacingXL
                spacing: 0

                // Column 1: Device name
                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 180
                    spacing: Theme.spacingSM
                    // Online indicator
                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: _online ? Theme.statusSuccess : Theme.textDisabled
                    }
                    Text {
                        text: _online ? _name : (_name + " (" + qsTr("Offline") + ")")
                        color: _online ? Theme.textPrimary : Theme.textTertiary
                        font.pixelSize: Theme.fontSizeMD
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                // Column 2: Task name
                Text {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 160
                    text: _taskName
                    color: _taskName === "No task" || _taskName === qsTr("No task") ? Theme.textDisabled : Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    elide: Text.ElideRight
                }

                // Column 3: Status + progress
                Item {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 180
                    Layout.fillHeight: true
                    // Status text and progress bar for active states (3=printing, 4=pause, 5=prepare, 6=slicing)
                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2
                        RowLayout {
                            spacing: Theme.spacingSM
                            Text {
                                text: _statusText
                                color: {
                                    if (_statusInt === 1) return Theme.statusSuccess;     // finish
                                    if (_statusInt === 2) return Theme.statusError;       // failed
                                    if (_statusInt >= 3 && _statusInt <= 6) return Theme.statusInfo; // active
                                    return Theme.textPrimary;
                                }
                                font.pixelSize: Theme.fontSizeMD
                                font.bold: _statusInt >= 3 && _statusInt <= 6
                            }
                            // Progress percentage for active states
                            Text {
                                visible: _statusInt >= 3 && _statusInt <= 6
                                text: _progress + "%"
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSizeXS
                            }
                        }
                        // Progress bar (aligns with upstream DrawRoundedRectangle progress)
                        Rectangle {
                            visible: _statusInt >= 3 && _statusInt <= 6
                            Layout.preferredWidth: 200
                            Layout.preferredHeight: 6
                            radius: 3
                            color: Theme.bgElevated
                            Rectangle {
                                width: parent.width * (_progress / 100.0)
                                height: parent.height
                                radius: 3
                                color: {
                                    if (_statusInt === 3) return Theme.statusInfo;   // printing: blue
                                    if (_statusInt === 4) return Theme.statusWarning; // pause: orange
                                    return Theme.accent;
                                }
                            }
                        }
                    }
                }

                // Column 4: Remaining time (对齐上游 get_left_time dd:hh:mm)
                Text {
                    Layout.preferredWidth: 90
                    text: (_statusInt >= 3 && _statusInt <= 6) ? _remaining : "--"
                    color: (_statusInt >= 3 && _statusInt <= 6) ? Theme.textSecondary : Theme.textDisabled
                    font.pixelSize: Theme.fontSizeSM
                    font.family: "monospace"
                    anchors.verticalCenter: parent.verticalCenter
                }

                // Column 5: View button (aligns with upstream "View" rounded button)
                Rectangle {
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 34
                    radius: Theme.radiusMD
                    color: _hovered ? Theme.bgElevated : Theme.bgSurface
                    border.color: Theme.borderDefault
                    border.width: 1
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        anchors.centerIn: parent
                        text: qsTr("View")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
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
                radius: 8
                color: _hovered ? Theme.bgPressed : "transparent"
                visible: _hovered
                Text {
                    anchors.centerIn: parent
                    text: "x"
                    color: Theme.textTertiary
                    font.pixelSize: 9
                }
                TapHandler { onTapped: _vm.removeDevice(index) }
            }
        }
    }

    // ══════════════════════════════════════════════════════
    // Component: Task Sending Tab (LocalTaskManagerPage)
    // Aligns with upstream table: [Checkbox] Project Name / Device Name / Status / Info / Sent Time / Actions
    // ══════════════════════════════════════════════════════
    Component {
        id: localTaskTabComponent

        Item {
            property var _vm: root.multiMachineVm
            // All-selected only considers selectable tasks (status <= 1)
            property bool _allLocalSelected: _vm.localSelectedCount > 0 && _vm.localSelectedCount === _vm.localTaskCount

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingLG
                spacing: 0

                // ── Bottom control bar (aligns with upstream m_ctrl_btn_panel) ──
                // Placed before list so it visually sits below in upstream, but
                // we keep the toolbar at top for QML convention.
                // ── Toolbar ──
                RowLayout {
                    Layout.fillWidth: true
                    Layout.bottomMargin: Theme.spacingMD
                    Item { Layout.fillWidth: true }
                    Text {
                        text: _vm.localSelectedCount > 0
                              ? qsTr("%1 selected").arg(_vm.localSelectedCount)
                              : ""
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                    }
                    // Send to Device button (对齐上游 SendMultiMachinePage)
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
                            text: qsTr("Cancel All")
                            color: Theme.textOnAccent
                            font.pixelSize: Theme.fontSizeSM
                        }
                        TapHandler { onTapped: _vm.stopAllLocalTasks() }
                    }
                    Item { Layout.fillWidth: true }
                }

                // ── Table header (aligns with upstream m_table_head_panel) ──
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    color: Theme.bgElevated
                    radius: Theme.radiusSM
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingXL
                        anchors.rightMargin: Theme.spacingXL
                        spacing: 0
                        // Select-all checkbox (aligns with upstream m_select_checkbox)
                        Rectangle {
                            width: 16
                            height: 16
                            radius: 3
                            color: _allLocalSelected ? Theme.accent : "transparent"
                            border.color: _allLocalSelected ? Theme.accent : Theme.borderDefault
                            border.width: 1
                            anchors.verticalCenter: parent.verticalCenter
                            Text {
                                anchors.centerIn: parent
                                visible: _allLocalSelected
                                text: "\u2713"
                                color: Theme.textOnAccent
                                font.pixelSize: 10
                            }
                            TapHandler {
                                onTapped: {
                                    if (_allLocalSelected)
                                        _vm.unselectAllLocalTasks()
                                    else
                                        _vm.selectAllLocalTasks()
                                }
                            }
                        }
                        Item { width: Theme.spacingMD }
                        Text { text: qsTr("Project Name"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.fillWidth: true }
                        Text { text: qsTr("Device Name"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 130 }
                        Text { text: qsTr("Task Status"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 130 }
                        Text { text: qsTr("Info"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 130 }
                        Text { text: qsTr("Sent Time"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 130 }
                        Text { text: qsTr("Actions"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 80 }
                    }
                }

                // ── Task list ──
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: availableWidth

                    Column {
                        id: localTaskList
                        width: parent.width
                        spacing: 1
                        Repeater {
                            model: _vm.localTaskCount
                            delegate: localTaskRowDelegate
                        }
                    }
                }

                // ── Empty state (aligns with upstream m_tip_text) ──
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: parent.height * 0.4
                    visible: !_vm.hasLocalTasks
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: Theme.spacingLG
                        Text {
                            text: qsTr("There are no tasks to be sent!")
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeXL
                            horizontalAlignment: Text.AlignHCenter
                            Layout.preferredWidth: 400
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
        }
    }

    // ══════════════════════════════════════════════════════
    // Delegate: Local task row
    // Aligns with upstream MultiTaskItem::doRender (m_task_type=0)
    // Columns: [Checkbox] Project Name / Device Name / Task Status / Info / Sent Time / Actions
    // Checkbox enabled only for status <= 1 (pending/sending), disabled otherwise (aligns with upstream)
    // Cancel button shown only for status <= 1 (aligns with upstream update_info)
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
            property int _sendingPercent: _vm.localTaskSendingPercent(index)
            readonly property bool _selectable: _status <= 1  // pending or sending
            readonly property bool _cancelable: _status <= 1  // show cancel button

            width: localTaskList.width
            height: 50
            color: _hovered ? Theme.bgHover : (_selected ? Theme.accentSubtle : Theme.bgSurface)
            radius: Theme.radiusSM
            property bool _hovered: false

            // Hover border (aligns with upstream doRender m_hover rounded rect)
            Rectangle {
                anchors.fill: parent
                radius: Theme.radiusSM
                color: "transparent"
                border.color: _hovered ? Theme.accent : "transparent"
                border.width: 1
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: localRow._hovered = true
                onExited: localRow._hovered = false
                onClicked: _vm.selectLocalTask(index)
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingXL
                anchors.rightMargin: Theme.spacingXL
                spacing: 0

                // Checkbox (aligns with upstream EVT_MULTI_DEVICE_SELECTED / state_selected)
                Rectangle {
                    width: 16
                    height: 16
                    radius: 3
                    color: _selected ? Theme.accent : "transparent"
                    border.color: _selectable ? (_selected ? Theme.accent : Theme.borderDefault) : Theme.borderSubtle
                    border.width: 1
                    opacity: _selectable ? 1.0 : 0.4
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        anchors.centerIn: parent
                        visible: _selected
                        text: "\u2713"
                        color: Theme.textOnAccent
                        font.pixelSize: 10
                    }
                }

                Item { width: Theme.spacingMD }

                // Project name
                Text {
                    Layout.fillWidth: true
                    text: _projectName
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    elide: Text.ElideRight
                }

                // Device name
                Text {
                    Layout.preferredWidth: 130
                    text: _devName
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    elide: Text.ElideRight
                    anchors.verticalCenter: parent.verticalCenter
                }

                // Task status
                Text {
                    Layout.preferredWidth: 130
                    text: _statusText
                    color: {
                        if (_status === 6) return Theme.statusSuccess;  // success
                        if (_status === 3 || _status === 4) return Theme.statusError; // cancel/failed
                        if (_status === 1 || _status === 5) return Theme.statusInfo; // sending/printing
                        return Theme.textSecondary;
                    }
                    font.pixelSize: Theme.fontSizeSM
                    anchors.verticalCenter: parent.verticalCenter
                }

                // Info column (aligns with upstream TASK_LEFT_PRO_INFO)
                // Shows sending progress bar for status=1 (sending)
                Item {
                    Layout.preferredWidth: 130
                    Layout.fillHeight: true
                    visible: _status === 1  // sending state shows progress
                    RowLayout {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Theme.spacingSM
                        // Sending progress bar (aligns with upstream doRender sending percent)
                        Rectangle {
                            width: 80
                            height: 6
                            radius: 3
                            color: Theme.bgElevated
                            Rectangle {
                                width: parent.width * (_sendingPercent / 100.0)
                                height: parent.height
                                radius: 3
                                color: Theme.statusInfo
                            }
                        }
                        Text {
                            text: _sendingPercent + "%"
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeXS
                            Layout.preferredWidth: 35
                        }
                    }
                }

                // Send time (aligns with upstream TASK_LEFT_SEND_TIME)
                Text {
                    Layout.preferredWidth: 130
                    text: _sendTime
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeXS
                    anchors.verticalCenter: parent.verticalCenter
                }

                // Actions column (aligns with upstream m_button_cancel)
                // Cancel button shown for pending/sending tasks
                Rectangle {
                    Layout.preferredWidth: 70
                    Layout.preferredHeight: 28
                    radius: 6
                    color: _cancelable ? (_cancelBtnHovered ? Theme.bgPressed : Theme.bgElevated) : Theme.bgPanel
                    border.color: _cancelable ? Theme.borderDefault : Theme.borderSubtle
                    border.width: 1
                    visible: _cancelable
                    anchors.verticalCenter: parent.verticalCenter
                    property bool _cancelBtnHovered: false

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Cancel")
                        color: _cancelable ? Theme.textPrimary : Theme.textDisabled
                        font.pixelSize: Theme.fontSizeXS
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: parent._cancelBtnHovered = true
                        onExited: parent._cancelBtnHovered = false
                        cursorShape: Qt.PointingHandCursor
                        onClicked: _vm.cancelLocalTask(index)
                    }
                }
                // Non-cancelable placeholder
                Item {
                    Layout.preferredWidth: 70
                    visible: !_cancelable
                }
            }
        }
    }

    // ══════════════════════════════════════════════════════
    // Component: Task Sent Tab (CloudTaskManagerPage)
    // Aligns with upstream: [Checkbox] Task Name / Device Name / Task Status / Info / Sent Time / Actions
    // Bulk actions: Pause All / Resume All / Stop All (aligns with upstream btn_pause_all/btn_continue_all/btn_stop_all)
    // Pagination: aligns with upstream m_flipping_panel
    // ══════════════════════════════════════════════════════
    Component {
        id: cloudTaskTabComponent

        Item {
            property var _vm: root.multiMachineVm
            property bool _allCloudSelected: _vm.cloudSelectedCount > 0 && _vm.cloudSelectedCount === _vm.cloudTaskCount

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingLG
                spacing: 0

                // ── Toolbar ──
                RowLayout {
                    Layout.fillWidth: true
                    Layout.bottomMargin: Theme.spacingMD
                    Item { Layout.fillWidth: true }
                    Text {
                        text: _vm.cloudSelectedCount > 0
                              ? qsTr("%1 selected").arg(_vm.cloudSelectedCount)
                              : ""
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                    }
                    // Pause All button (aligns with upstream btn_pause_all)
                    Rectangle {
                        visible: _vm.cloudSelectedCount > 0
                        width: 80
                        height: Theme.controlHeightSM
                        radius: Theme.radiusSM
                        color: Theme.accent
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Pause All")
                            color: Theme.textOnAccent
                            font.pixelSize: Theme.fontSizeSM
                        }
                        TapHandler { onTapped: _vm.pauseAllCloudTasks() }
                    }
                    // Resume All button (aligns with upstream btn_continue_all)
                    Rectangle {
                        visible: _vm.cloudSelectedCount > 0
                        width: 90
                        height: Theme.controlHeightSM
                        radius: Theme.radiusSM
                        color: Theme.accent
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Resume All")
                            color: Theme.textOnAccent
                            font.pixelSize: Theme.fontSizeSM
                        }
                        TapHandler { onTapped: _vm.resumeAllCloudTasks() }
                    }
                    // Stop All button (aligns with upstream btn_stop_all)
                    Rectangle {
                        visible: _vm.cloudSelectedCount > 0
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
                        TapHandler { onTapped: _vm.stopAllCloudTasks() }
                    }
                    Item { Layout.fillWidth: true }
                }

                // ── Table header (aligns with upstream m_table_head_panel) ──
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    color: Theme.bgElevated
                    radius: Theme.radiusSM
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingXL
                        anchors.rightMargin: Theme.spacingXL
                        spacing: 0
                        // Select-all checkbox (aligns with upstream m_select_checkbox)
                        Rectangle {
                            width: 16
                            height: 16
                            radius: 3
                            color: _allCloudSelected ? Theme.accent : "transparent"
                            border.color: _allCloudSelected ? Theme.accent : Theme.borderDefault
                            border.width: 1
                            anchors.verticalCenter: parent.verticalCenter
                            Text {
                                anchors.centerIn: parent
                                visible: _allCloudSelected
                                text: "\u2713"
                                color: Theme.textOnAccent
                                font.pixelSize: 10
                            }
                            TapHandler {
                                onTapped: {
                                    if (_allCloudSelected)
                                        _vm.unselectAllCloudTasks()
                                    else
                                        _vm.selectAllCloudTasks()
                                }
                            }
                        }
                        Item { width: Theme.spacingMD }
                        Text { text: qsTr("Task Name"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.fillWidth: true }
                        Text { text: qsTr("Device Name"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 120 }
                        Text { text: qsTr("Task Status"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 110 }
                        Text { text: qsTr("Info"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 130 }
                        Text { text: qsTr("Sent Time"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 120 }
                        Text { text: qsTr("Actions"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.fillWidth: true }
                    }
                }

                // ── Cloud task list ──
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: availableWidth

                    Column {
                        id: cloudTaskList
                        width: parent.width
                        spacing: 1
                        Repeater {
                            model: _vm.cloudTaskCount
                            delegate: cloudTaskRowDelegate
                        }
                    }
                }

                // ── Empty state ──
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: parent.height * 0.4
                    visible: !_vm.hasCloudTasks
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: Theme.spacingLG
                        Text {
                            text: qsTr("No sent tasks")
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeXL
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }

                // ── Pagination controls (aligns with upstream CloudTaskManagerPage m_flipping_panel) ──
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.spacingSM
                    visible: _vm.cloudTotalPages > 1
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        width: 24
                        height: 24
                        radius: Theme.radiusSM
                        color: _vm.cloudCurrentPage > 0 ? Theme.bgElevated : Theme.bgPanel
                        Text {
                            anchors.centerIn: parent
                            text: "<"
                            color: _vm.cloudCurrentPage > 0 ? Theme.textPrimary : Theme.textDisabled
                            font.pixelSize: Theme.fontSizeSM
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
                    Rectangle {
                        width: 24
                        height: 24
                        radius: Theme.radiusSM
                        color: _vm.cloudCurrentPage < _vm.cloudTotalPages - 1 ? Theme.bgElevated : Theme.bgPanel
                        Text {
                            anchors.centerIn: parent
                            text: ">"
                            color: _vm.cloudCurrentPage < _vm.cloudTotalPages - 1 ? Theme.textPrimary : Theme.textDisabled
                            font.pixelSize: Theme.fontSizeSM
                        }
                        TapHandler {
                            enabled: _vm.cloudCurrentPage < _vm.cloudTotalPages - 1
                            onTapped: _vm.cloudCurrentPage = _vm.cloudCurrentPage + 1
                        }
                    }
                    Item { Layout.fillWidth: true }
                }
            }
        }
    }

    // ══════════════════════════════════════════════════════
    // Delegate: Cloud task row
    // Aligns with upstream MultiTaskItem::doRender (m_task_type=1)
    // Columns: [Checkbox] Task Name / Device Name / Task Status / Info / Sent Time / Actions
    // Checkbox enabled only for status == 0 (printing), disabled otherwise (aligns with upstream)
    // Actions: Pause / Resume / Stop buttons for printing tasks (aligns with upstream m_button_pause/resume/stop)
    // Info column: progress bar + remaining time for active printing tasks (aligns with upstream TASK_LEFT_PRO_INFO)
    // ══════════════════════════════════════════════════════
    Component {
        id: cloudTaskRowDelegate

        Rectangle {
            id: cloudRow
            required property int index
            property var _vm: root.multiMachineVm
            property string _projectName: _vm.cloudTaskProjectName(index)
            property string _devName: _vm.cloudTaskDevName(index)
            property int _status: _vm.cloudTaskStatus(index)
            property string _statusText: _vm.cloudTaskStatusText(index)
            property int _progress: _vm.cloudTaskProgress(index)
            property string _sendTime: _vm.cloudTaskSendTime(index)
            property string _remaining: _vm.cloudTaskRemaining(index)
            property bool _selected: _vm.cloudTaskSelected(index)
            readonly property bool _selectable: _status === 0  // only printing tasks selectable
            readonly property bool _isPrinting: _status === 0

            width: cloudTaskList.width
            height: 50
            color: _hovered ? Theme.bgHover : (_selected ? Theme.accentSubtle : Theme.bgSurface)
            radius: Theme.radiusSM
            property bool _hovered: false
            // Track pause/resume toggle state for printing tasks
            property bool _paused: false

            // Hover border (aligns with upstream doRender m_hover)
            Rectangle {
                anchors.fill: parent
                radius: Theme.radiusSM
                color: "transparent"
                border.color: _hovered ? Theme.accent : "transparent"
                border.width: 1
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: cloudRow._hovered = true
                onExited: cloudRow._hovered = false
                onClicked: _vm.selectCloudTask(index)
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingXL
                anchors.rightMargin: Theme.spacingXL
                spacing: 0

                // Checkbox (aligns with upstream EVT_MULTI_CLOUD_TASK_SELECTED)
                Rectangle {
                    width: 16
                    height: 16
                    radius: 3
                    color: _selected ? Theme.accent : "transparent"
                    border.color: _selectable ? (_selected ? Theme.accent : Theme.borderDefault) : Theme.borderSubtle
                    border.width: 1
                    opacity: _selectable ? 1.0 : 0.4
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        anchors.centerIn: parent
                        visible: _selected
                        text: "\u2713"
                        color: Theme.textOnAccent
                        font.pixelSize: 10
                    }
                }

                Item { width: Theme.spacingMD }

                // Task name
                Text {
                    Layout.fillWidth: true
                    text: _projectName
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    elide: Text.ElideRight
                }

                // Device name
                Text {
                    Layout.preferredWidth: 120
                    text: _devName
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    elide: Text.ElideRight
                    anchors.verticalCenter: parent.verticalCenter
                }

                // Task status
                Text {
                    Layout.preferredWidth: 110
                    text: _statusText
                    color: {
                        if (_status === 1) return Theme.statusSuccess;  // finish
                        if (_status === 2) return Theme.statusError;    // failed
                        if (_status === 0) return Theme.statusInfo;    // printing
                        return Theme.textSecondary;
                    }
                    font.pixelSize: Theme.fontSizeSM
                    anchors.verticalCenter: parent.verticalCenter
                }

                // Info column (aligns with upstream TASK_LEFT_PRO_INFO for cloud tasks)
                // Shows progress bar + remaining time for printing tasks
                Item {
                    Layout.preferredWidth: 130
                    Layout.fillHeight: true
                    visible: _isPrinting
                    ColumnLayout {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2
                        // Progress bar + percentage + remaining time (aligns with upstream doRender cloud info)
                        RowLayout {
                            spacing: Theme.spacingSM
                            Rectangle {
                                width: 70
                                height: 6
                                radius: 3
                                color: Theme.bgElevated
                                Rectangle {
                                    width: parent.width * (_progress / 100.0)
                                    height: parent.height
                                    radius: 3
                                    color: Theme.statusInfo
                                }
                            }
                            Text {
                                text: _progress + "%  |  " + _remaining
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSizeXS
                            }
                        }
                    }
                }

                // Sent time
                Text {
                    Layout.preferredWidth: 120
                    text: _sendTime
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeXS
                    anchors.verticalCenter: parent.verticalCenter
                }

                // Actions column (aligns with upstream m_button_pause / m_button_resume / m_button_stop)
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSM
                    visible: _isPrinting

                    // Pause button (aligns with upstream m_button_pause)
                    Rectangle {
                        width: 55
                        height: 28
                        radius: 6
                        color: _pauseBtnHover ? Theme.bgPressed : Theme.bgElevated
                        border.color: Theme.borderDefault
                        border.width: 1
                        visible: !cloudRow._paused
                        anchors.verticalCenter: parent.verticalCenter
                        property bool _pauseBtnHover: false

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Pause")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeXS
                        }
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: parent._pauseBtnHover = true
                            onExited: parent._pauseBtnHover = false
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                _vm.pauseCloudTask(index)
                                cloudRow._paused = true
                            }
                        }
                    }

                    // Resume button (aligns with upstream m_button_resume)
                    Rectangle {
                        width: 60
                        height: 28
                        radius: 6
                        color: Theme.accent
                        border.color: Theme.accent
                        border.width: 1
                        visible: cloudRow._paused
                        anchors.verticalCenter: parent.verticalCenter
                        property bool _resumeBtnHover: false

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Resume")
                            color: Theme.textOnAccent
                            font.pixelSize: Theme.fontSizeXS
                        }
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: parent._resumeBtnHover = true
                            onExited: parent._resumeBtnHover = false
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                _vm.resumeCloudTask(index)
                                cloudRow._paused = false
                            }
                        }
                    }

                    // Stop button (aligns with upstream m_button_stop)
                    Rectangle {
                        width: 50
                        height: 28
                        radius: 6
                        color: _stopBtnHover ? Theme.bgPressed : Theme.bgElevated
                        border.color: Theme.borderDefault
                        border.width: 1
                        anchors.verticalCenter: parent.verticalCenter
                        property bool _stopBtnHover: false

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Stop")
                            color: Theme.statusError
                            font.pixelSize: Theme.fontSizeXS
                        }
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: parent._stopBtnHover = true
                            onExited: parent._stopBtnHover = false
                            cursorShape: Qt.PointingHandCursor
                            onClicked: _vm.stopCloudTask(index)
                        }
                    }
                }
            }
        }
    }

    // ══════════════════════════════════════════════════════
    // Dialog: Send Task to Device (对齐上游 MultiMachinePickPage)
    // Uses Theme tokens instead of hardcoded colors
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
            Text { text: qsTr("Task:"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeXS }
            ComboBox {
                id: taskSelector
                Layout.fillWidth: true
                implicitHeight: 28
                model: root.multiMachineVm.localTaskCount
                textRole: ""
                delegate: ItemDelegate {
                    width: taskSelector.width
                    contentItem: Text {
                        text: root.multiMachineVm.localTaskProjectName(index)
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                    }
                    highlighted: taskSelector.highlightedIndex === index
                }
                contentItem: Text {
                    text: taskSelector.currentIndex >= 0
                          ? root.multiMachineVm.localTaskProjectName(taskSelector.currentIndex)
                          : qsTr("Select task...")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeXS
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 8
                }
            }

            // Online device list
            Text { text: qsTr("Online Devices:"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeXS }
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
                            color: sendToDeviceDialog.selectedDeviceIndex === index ? Theme.accentSubtle : Theme.bgSurface
                            border.color: sendToDeviceDialog.selectedDeviceIndex === index ? Theme.accent : Theme.borderDefault
                            border.width: 1

                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 12
                                spacing: 8
                                // Online indicator
                                Rectangle { width: 8; height: 8; radius: 4; color: Theme.statusSuccess }
                                Text {
                                    text: root.multiMachineVm.onlineMachineName(index)
                                    color: sendToDeviceDialog.selectedDeviceIndex === index ? Theme.accent : Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeSM
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

            // Send button row
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Item { Layout.fillWidth: true }
                Rectangle {
                    width: 80; height: 28; radius: Theme.radiusSM
                    color: Theme.bgElevated
                    border.color: Theme.borderDefault; border.width: 1
                    Text { anchors.centerIn: parent; text: qsTr("Cancel"); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeXS }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: sendToDeviceDialog.close() }
                }
                Rectangle {
                    width: 80; height: 28; radius: Theme.radiusSM
                    color: Theme.accent
                    Text { anchors.centerIn: parent; text: qsTr("Send"); color: Theme.textOnAccent; font.pixelSize: Theme.fontSizeXS }
                    MouseArea {
                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (taskSelector.currentIndex >= 0 && sendToDeviceDialog.selectedDeviceIndex >= 0) {
                                root.multiMachineVm.sendTaskToDevice(taskSelector.currentIndex, sendToDeviceDialog.selectedDeviceIndex)
                                sendToDeviceDialog.close()
                            }
                        }
                    }
                }
            }
        }
        onOpened: { selectedDeviceIndex = -1; taskSelector.currentIndex = 0 }
    }
}
