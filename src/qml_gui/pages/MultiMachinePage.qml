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
    // Aligns with upstream table: Project Name / Printer / Status / Send Time / Actions
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
                            text: qsTr("Stop All")
                            color: Theme.textOnAccent
                            font.pixelSize: Theme.fontSizeSM
                        }
                        TapHandler { onTapped: _vm.stopAllLocalTasks() }
                    }
                    Item { Layout.fillWidth: true }
                }

                // ── Table header ──
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
                        Text { text: qsTr("Project Name"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.fillWidth: true }
                        Text { text: qsTr("Printer"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 130 }
                        Text { text: qsTr("Status"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 130 }
                        Text { text: qsTr("Send Time"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 130 }
                        Text { text: qsTr("Remaining"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 80 }
                        Text { text: qsTr("Progress"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 90 }
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

                // ── Empty state ──
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: parent.height * 0.4
                    visible: !_vm.hasLocalTasks
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: Theme.spacingLG
                        Text {
                            text: qsTr("No sending tasks")
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeXL
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
            }
        }
    }

    // ══════════════════════════════════════════════════════
    // Delegate: Local task row
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

            width: localTaskList.width
            height: 44
            color: _hovered ? Theme.bgHover : (_selected ? Theme.accentSubtle : Theme.bgSurface)
            radius: Theme.radiusSM
            property bool _hovered: false

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

                // Checkbox + project name
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingMD
                    // Selection indicator
                    Rectangle {
                        width: 16
                        height: 16
                        radius: 3
                        color: _selected ? Theme.accent : "transparent"
                        border.color: _selected ? Theme.accent : Theme.borderDefault
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            visible: _selected
                            text: "\u2713"
                            color: Theme.textOnAccent
                            font.pixelSize: 10
                        }
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
                        if (_status === 7 || _status === 4) return Theme.statusError; // failed
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

                // Remaining time (对齐 upstream get_left_time)
                Text {
                    Layout.preferredWidth: 80
                    text: (_status === 1 || _status === 5) ? _remaining : "--"
                    color: (_status === 1 || _status === 5) ? Theme.textSecondary : Theme.textDisabled
                    font.pixelSize: Theme.fontSizeXS
                    font.family: "monospace"
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
                            radius: 3
                            color: Theme.bgElevated
                            Rectangle {
                                width: parent.width * (_progress / 100.0)
                                height: parent.height
                                radius: 3
                                color: Theme.accent
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
            }
        }
    }

    // ══════════════════════════════════════════════════════
    // Component: Task Sent Tab (CloudTaskManagerPage)
    // ══════════════════════════════════════════════════════
    Component {
        id: cloudTaskTabComponent

        Item {
            property var _vm: root.multiMachineVm

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingLG
                spacing: 0

                // ── Toolbar ──
                RowLayout {
                    Layout.fillWidth: true
                    Layout.bottomMargin: Theme.spacingMD
                    Item { Layout.fillWidth: true }
                    // Stop All button (aligns with upstream btn_stop_all)
                    Rectangle {
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

                // ── Table header ──
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
                        Text { text: qsTr("Project Name"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.fillWidth: true }
                        Text { text: qsTr("Printer"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 130 }
                        Text { text: qsTr("Status"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 130 }
                        Text { text: qsTr("Send Time"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 130 }
                        Text { text: qsTr("Remaining"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 80 }
                        Text { text: qsTr("Progress"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS; font.bold: true; Layout.preferredWidth: 90 }
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
            }
        }
    }

    // ══════════════════════════════════════════════════════
    // Delegate: Cloud task row
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

            width: cloudTaskList.width
            height: 44
            color: _hovered ? Theme.bgHover : Theme.bgSurface
            radius: Theme.radiusSM
            property bool _hovered: false

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: cloudRow._hovered = true
                onExited: cloudRow._hovered = false
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingXL
                anchors.rightMargin: Theme.spacingXL
                spacing: 0

                Text {
                    Layout.fillWidth: true
                    text: _projectName
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    elide: Text.ElideRight
                }

                Text {
                    Layout.preferredWidth: 130
                    text: _devName
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    elide: Text.ElideRight
                }

                RowLayout {
                    Layout.preferredWidth: 130
                    spacing: Theme.spacingMD
                    Text {
                        text: _statusText
                        color: {
                            if (_status === 1) return Theme.statusSuccess;  // finish
                            if (_status === 2) return Theme.statusError;    // failed
                            if (_status === 0) return Theme.statusInfo;    // printing
                            return Theme.textSecondary;
                        }
                        font.pixelSize: Theme.fontSizeSM
                    }
                    // Pause/Resume button for printing tasks
                    Rectangle {
                        visible: _status === 0
                        width: 50
                        height: 22
                        radius: Theme.radiusSM
                        color: Theme.bgElevated
                        border.color: Theme.borderDefault
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Pause")
                            color: Theme.textSecondary
                            font.pixelSize: 9
                        }
                        TapHandler { onTapped: _vm.pauseCloudTask(index) }
                    }
                }

                Text {
                    Layout.preferredWidth: 130
                    text: _sendTime
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeXS
                }

                // Remaining time (对齐 upstream get_left_time)
                Text {
                    Layout.preferredWidth: 80
                    text: _status === 0 ? _remaining : "--"
                    color: _status === 0 ? Theme.textSecondary : Theme.textDisabled
                    font.pixelSize: Theme.fontSizeXS
                    font.family: "monospace"
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
                            radius: 3
                            color: Theme.bgElevated
                            Rectangle {
                                width: parent.width * (_progress / 100.0)
                                height: parent.height
                                radius: 3
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
            }
        }
    }

    // ══════════════════════════════════════════════════════
    // Dialog: Send Task to Device (对齐上游 MultiMachinePickPage)
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
            Text { text: qsTr("Task:"); color: Theme.textSecondary; font.pixelSize: 12 }
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
                        font.pixelSize: 11
                    }
                    highlighted: taskSelector.highlightedIndex === index
                }
                contentItem: Text {
                    text: taskSelector.currentIndex >= 0
                          ? root.multiMachineVm.localTaskProjectName(taskSelector.currentIndex)
                          : qsTr("Select task...")
                    color: Theme.textPrimary
                    font.pixelSize: 11
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 8
                }
            }

            // Online device list
            Text { text: qsTr("Online Devices:"); color: Theme.textSecondary; font.pixelSize: 12 }
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
                            radius: 4
                            color: sendToDeviceDialog.selectedDeviceIndex === index ? "#1c2a3e" : "#1a1e28"
                            border.color: sendToDeviceDialog.selectedDeviceIndex === index ? "#18c75e" : "#2e3444"
                            border.width: 1

                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 12
                                spacing: 8
                                // Online indicator
                                Rectangle { width: 8; height: 8; radius: 4; color: "#18c75e" }
                                Text {
                                    text: root.multiMachineVm.onlineMachineName(index)
                                    color: sendToDeviceDialog.selectedDeviceIndex === index ? "#18c75e" : "#c8d4e0"
                                    font.pixelSize: 12
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
                Rectangle {
                    width: 80; height: 28; radius: 4
                    color: "#252b38"
                    border.color: "#363d4e"; border.width: 1
                    Text { anchors.centerIn: parent; text: qsTr("Cancel"); color: "#c8d4e0"; font.pixelSize: 11 }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: sendToDeviceDialog.close() }
                }
                Rectangle {
                    width: 80; height: 28; radius: 4
                    color: "#18c75e"
                    Text { anchors.centerIn: parent; text: qsTr("Send"); color: "white"; font.pixelSize: 11 }
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
