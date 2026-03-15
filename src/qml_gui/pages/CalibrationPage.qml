import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../dialogs"
import "../controls"
import "../components"

// Upstream reference:
//   CalibrationPanel (CalibrationPanel.hpp/cpp) - left tab panel with tabbook
//   CalibrationWizard (CalibrationWizard.hpp/cpp) - wizard page chain: Start -> Preset -> Cali -> Save
//   CalibrationDialog (Calibration.hpp/cpp) - hardware calibration dialog with step checkboxes
//   CalibMode enum: Calib_PA_Line, Calib_Flow_Rate, Calib_Vol_speed_Tower, Calib_Temp_Tower, etc.
//
// This page merges the CalibrationPanel tab-based layout with the CalibrationDialog
// hardware calibration step selection, driven by CalibrationServiceMock/CalibrationViewModel.

Item {
    id: root
    required property var calibrationVm
    property var _calibItems: []
    property var _totalSteps: 0

    Component.onCompleted: {
        reloadCalibItems()
        reloadSteps()
    }

    function reloadCalibItems() {
        var arr = []
        var n = calibrationVm.calibItemCount()
        for (var i = 0; i < n; ++i)
            arr.push({
                         icon: calibrationVm.calibItemIcon(i),
                         name: calibrationVm.calibItemName(i),
                         desc: calibrationVm.calibItemDesc(i),
                         status: calibrationVm.calibItemStatus(i)
                     })
        _calibItems = arr
    }

    function reloadSteps() {
        var arr = []
        var n = calibrationVm.stepCount()
        for (var i = 0; i < n; ++i)
            arr.push({
                         title: calibrationVm.stepTitle(i),
                         desc: calibrationVm.stepDesc(i),
                         state: calibrationVm.stepState(i)
                     })
        _stepsArr = arr
        _totalSteps = n
    }

    property var _stepsArr: []

    Connections {
        target: root.calibrationVm
        function onSelectionChanged() { reloadSteps() }
        function onStatusChanged() { reloadCalibItems(); reloadSteps() }
        function onStepChanged() { reloadSteps() }
    }

    CalibrationDialog {
        id: calibDlg
        calibrationVm: root.calibrationVm
    }

    CaliHistoryDialog {
        id: historyDlg
        calibrationVm: root.calibrationVm
    }

    // Background
    Rectangle { anchors.fill: parent; color: Theme.bgBase }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header bar (aligned with upstream CalibrationPanel's SideTools + Tabbook layout)
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: Theme.bgSurface

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingXL
                anchors.rightMargin: Theme.spacingXL
                spacing: Theme.spacingLG

                Text {
                    text: qsTr("Calibration Center")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeXL
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                // History button (对齐上游 FlowCalibHeaderView)
                Rectangle {
                    Layout.preferredHeight: 28
                    Layout.preferredWidth: historyBtnRow.implicitWidth + 20
                    radius: 4
                    color: historyBtnHov.containsMouse ? Theme.bgHover : "transparent"
                    border.color: Theme.borderSubtle
                    border.width: 1

                    Row {
                        id: historyBtnRow
                        anchors.centerIn: parent
                        spacing: 6
                        Text {
                            text: "\u231B" // Clock/history icon
                            color: Theme.textSecondary
                            font.pixelSize: 12
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: qsTr("历史记录")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSM
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    HoverHandler { id: historyBtnHov }
                    TapHandler {
                        onTapped: historyDlg.open()
                    }
                }

                // Category filter pills
                Repeater {
                    model: [
                        { label: qsTr("All"), cat: "all" },
                        { label: qsTr("Slice Calibration"), cat: "slice" },
                        { label: qsTr("Hardware Calibration"), cat: "hardware" }
                    ]
                    Rectangle {
                        Layout.preferredHeight: 26
                        Layout.preferredWidth: catLbl.implicitWidth + 20
                        radius: 13
                        color: _activeFilter === modelData.cat ? Theme.accent : "transparent"
                        border.color: _activeFilter === modelData.cat ? Theme.accent : Theme.borderSubtle
                        border.width: 1

                        property string _activeFilter: "all"

                        Text {
                            id: catLbl
                            anchors.centerIn: parent
                            text: modelData.label
                            color: parent._activeFilter === modelData.cat ? Theme.textOnAccent : Theme.textTertiary
                            font.pixelSize: Theme.fontSizeSM
                        }

                        TapHandler {
                            onTapped: {
                                // Update filter across all pills
                                var pills = catFilterRow.children
                                for (var i = 0; i < pills.length; ++i) {
                                    pills[i]._activeFilter = modelData.cat
                                }
                            }
                        }
                    }
                }
            }
        }

        // Filter bar row id
        Item {
            id: catFilterRow
            visible: false
        }

        // Separator
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.borderSubtle
        }

        // Main content: left sidebar + right detail
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // === Left sidebar: Calibration type list (aligned with upstream Tabbook tabs) ===
            Rectangle {
                Layout.preferredWidth: 260
                Layout.fillHeight: true
                color: Theme.bgSurface

                Column {
                    anchors.fill: parent
                    spacing: 0

                    // Slice calibration section header
                    Rectangle {
                        width: parent.width
                        height: 28
                        color: Theme.bgPanel

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: Theme.spacingLG
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("SLICE CALIBRATION")
                            color: Theme.textDisabled
                            font.pixelSize: Theme.fontSizeXS
                            font.bold: true
                            font.letterSpacing: 1
                        }
                    }

                    ScrollView {
                        width: parent.width
                        Layout.fillHeight: true
                        clip: true
                        ScrollBar.vertical.policy: _sliceRepeater.count > 3 ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff

                        Column {
                            width: parent.width
                            spacing: 1

                            Repeater {
                                id: _sliceRepeater
                                model: root._calibItems

                                delegate: Rectangle {
                                    width: parent.width - 8
                                    x: 4
                                    height: 52
                                    radius: Theme.radiusMD
                                    color: {
                                        if (root.calibrationVm.selectedIndex === index)
                                            return Theme.bgElevated
                                        if (itemHov.containsMouse)
                                            return Theme.bgHover
                                        return "transparent"
                                    }
                                    border.color: root.calibrationVm.selectedIndex === index ? Theme.accent : "transparent"
                                    border.width: 1

                                    // Status indicator dot
                                    property string statusColor: {
                                        if (modelData.status === 1) return Theme.statusInfo    // InProgress
                                        if (modelData.status === 2) return Theme.statusSuccess  // Completed
                                        if (modelData.status === 3) return Theme.statusError    // Failed
                                        return Theme.textDisabled                            // NotStarted
                                    }

                                    Row {
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        anchors.leftMargin: Theme.spacingLG
                                        spacing: Theme.spacingMD

                                        // Status dot
                                        Rectangle {
                                            width: 6
                                            height: 6
                                            radius: 3
                                            anchors.verticalCenter: parent.verticalCenter
                                            color: parent.parent.parent.statusColor
                                            visible: modelData.status !== 0
                                        }

                                        Text {
                                            text: modelData.icon || "\u2699"
                                            font.pixelSize: 18
                                            color: Theme.textSecondary
                                            anchors.verticalCenter: parent.verticalCenter
                                        }

                                        Column {
                                            spacing: 1
                                            anchors.verticalCenter: parent.verticalCenter
                                            Text {
                                                text: modelData.name
                                                color: Theme.textPrimary
                                                font.pixelSize: Theme.fontSizeMD
                                            }
                                            Text {
                                                text: modelData.desc || ""
                                                color: Theme.textDisabled
                                                font.pixelSize: Theme.fontSizeXS
                                            }
                                        }
                                    }

                                    HoverHandler { id: itemHov }
                                    TapHandler {
                                        onTapped: {
                                            root.calibrationVm.selectItem(index)
                                            root.reloadSteps()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Vertical separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: Theme.borderSubtle
            }

            // === Right detail area ===
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.bgBase

                // Empty state
                Column {
                    anchors.centerIn: parent
                    spacing: Theme.spacingXL
                    visible: root.calibrationVm.selectedIndex < 0

                    Text {
                        text: "\u2699"
                        font.pixelSize: 56
                        color: Theme.bgHover
                        horizontalAlignment: Text.AlignHCenter
                        width: 300
                    }
                    Text {
                        text: qsTr("Select a calibration type from the left panel")
                        color: Theme.textDisabled
                        font.pixelSize: Theme.fontSizeLG
                        horizontalAlignment: Text.AlignHCenter
                        width: 300
                    }
                }

                // Selected calibration detail
                ColumnLayout {
                    visible: root.calibrationVm.selectedIndex >= 0
                    anchors.fill: parent
                    anchors.margins: Theme.spacingXXL
                    spacing: Theme.spacingLG

                    // Title row with status badge
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingMD

                        Text {
                            text: root.calibrationVm.selectedTitle
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeXXL
                            font.bold: true
                        }

                        // Status badge
                        Rectangle {
                            visible: root.calibrationVm.selectedStatus > 0
                            Layout.preferredHeight: 22
                            Layout.preferredWidth: statusLbl.implicitWidth + 16
                            radius: 11
                            color: {
                                if (root.calibrationVm.selectedStatus === 1) return Qt.rgba(0x3b, 0x9e, 0xff, 0.15)
                                if (root.calibrationVm.selectedStatus === 2) return Qt.rgba(0x18, 0xc7, 0x5e, 0.15)
                                if (root.calibrationVm.selectedStatus === 3) return Qt.rgba(0xe0, 0x40, 0x40, 0.15)
                                return "transparent"
                            }

                            Text {
                                id: statusLbl
                                anchors.centerIn: parent
                                text: {
                                    var s = root.calibrationVm.selectedStatus
                                    if (s === 1) return qsTr("In Progress")
                                    if (s === 2) return qsTr("Completed")
                                    if (s === 3) return qsTr("Failed")
                                    return ""
                                }
                                color: {
                                    var s = root.calibrationVm.selectedStatus
                                    if (s === 1) return Theme.statusInfo
                                    if (s === 2) return Theme.statusSuccess
                                    if (s === 3) return Theme.statusError
                                    return Theme.textSecondary
                                }
                                font.pixelSize: Theme.fontSizeXS
                                font.bold: true
                            }
                        }

                        Item { Layout.fillWidth: true }
                    }

                    // Description
                    Text {
                        text: root.calibrationVm.selectedDescription
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeMD
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                    }

                    // === Wizard step indicator (aligned with upstream CalibrationWizard step chain) ===
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: wizardContent.implicitHeight + 32
                        color: Theme.bgSurface
                        radius: Theme.radiusLG
                        border.color: Theme.borderSubtle
                        border.width: 1

                        ColumnLayout {
                            id: wizardContent
                            anchors.fill: parent
                            anchors.margins: Theme.spacingXL
                            spacing: Theme.spacingMD

                            // Step indicator title
                            Text {
                                text: qsTr("Calibration Steps")
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeLG
                                font.bold: true
                            }

                            // Step indicator bar
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                Repeater {
                                    model: root._stepsArr

                                    RowLayout {
                                        spacing: 0
                                        Layout.fillWidth: index < root._stepsArr.length - 1

                                        // Step circle (对齐上游 StepCtrl: pending/active/completed)
                                        Rectangle {
                                            width: 28
                                            height: 28
                                            radius: 14
                                            color: {
                                                // state: 0=pending, 1=active, 2=completed
                                                if (modelData.state === 2) return Theme.accent
                                                if (modelData.state === 1) return Theme.accent
                                                return Theme.borderDefault
                                            }

                                            Text {
                                                anchors.centerIn: parent
                                                text: modelData.state === 2 ? "\u2713" : (index + 1).toString()
                                                color: {
                                                    if (modelData.state === 2) return Theme.textOnAccent
                                                    if (modelData.state === 1) return Theme.textOnAccent
                                                    return Theme.textDisabled
                                                }
                                                font.pixelSize: modelData.state === 2 ? 14 : Theme.fontSizeSM
                                                font.bold: true
                                            }
                                        }

                                        // Step name
                                        Text {
                                            text: modelData.title
                                            color: {
                                                if (modelData.state === 2) return Theme.textPrimary
                                                if (modelData.state === 1) return Theme.textPrimary
                                                return Theme.textDisabled
                                            }
                                            font.pixelSize: Theme.fontSizeSM
                                            Layout.leftMargin: Theme.spacingSM
                                            Layout.fillWidth: true
                                        }

                                        // Connector line
                                        Rectangle {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 2
                                            color: modelData.state === 2 ? Theme.accent : Theme.borderDefault
                                            visible: index < root._stepsArr.length - 1
                                        }
                                    }
                                }
                            }

                            // Current step description
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: stepDescCol.implicitHeight + 24
                                color: Theme.bgInset
                                radius: Theme.radiusMD

                                ColumnLayout {
                                    id: stepDescCol
                                    anchors.fill: parent
                                    anchors.margins: Theme.spacingLG
                                    spacing: Theme.spacingSM

                                    Text {
                                        text: {
                                            if (root.calibrationVm.isRunning)
                                                return qsTr("Step %1: %2").arg(
                                                    root.calibrationVm.currentStepIndex + 1).arg(
                                                    root.calibrationVm.currentStepTitle)
                                            if (root.calibrationVm.selectedStatus === 2)
                                                return qsTr("Calibration Complete")
                                            if (root.calibrationVm.totalStepCount > 0)
                                                return qsTr("Step 1: %1").arg(
                                                    root.calibrationVm.stepCount() > 0 ? root.calibrationVm.stepTitle(0) : "")
                                            return ""
                                        }
                                        color: Theme.textPrimary
                                        font.pixelSize: Theme.fontSizeMD
                                        font.bold: true
                                    }

                                    Text {
                                        text: {
                                            if (root.calibrationVm.isRunning)
                                                return root.calibrationVm.currentStepDesc
                                            if (root.calibrationVm.selectedStatus === 2)
                                                return qsTr("Calibration finished successfully. Results have been saved.")
                                            if (root.calibrationVm.stepCount() > 0)
                                                return root.calibrationVm.stepDesc(0)
                                            return ""
                                        }
                                        color: Theme.textSecondary
                                        font.pixelSize: Theme.fontSizeSM
                                        wrapMode: Text.Wrap
                                        Layout.fillWidth: true
                                    }

                                    // Filament preset selector (对齐上游 CalibrationPresetPage)
                                    Rectangle {
                                        visible: root.calibrationVm.showPresetSelector
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 72
                                        radius: Theme.radiusSM
                                        color: Theme.bgElevated
                                        border.width: 1
                                        border.color: Theme.borderSubtle

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 10
                                            anchors.rightMargin: 10
                                            anchors.topMargin: 8
                                            spacing: 6

                                            Text {
                                                text: qsTr("耗材预设")
                                                color: Theme.textPrimary
                                                font.pixelSize: Theme.fontSizeSM
                                                font.bold: true
                                            }

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 6

                                                Rectangle {
                                                    Layout.fillWidth: true
                                                    height: 30
                                                    radius: 6
                                                    color: Theme.bgPanel
                                                    border.width: 1
                                                    border.color: Theme.borderDefault

                                                    Text {
                                                        anchors.left: parent.left
                                                        anchors.leftMargin: 10
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        text: root.calibrationVm.selectedFilamentPreset
                                                        color: Theme.textPrimary
                                                        font.pixelSize: Theme.fontSizeSM
                                                        elide: Text.ElideRight
                                                    }
                                                }

                                                // Dropdown button
                                                Rectangle {
                                                    width: 24
                                                    height: 30
                                                    radius: 6
                                                    color: Theme.bgHover

                                                    Text {
                                                        anchors.centerIn: parent
                                                        text: "▾"
                                                        color: Theme.textSecondary
                                                        font.pixelSize: 12
                                                    }

                                                    TapHandler {
                                                        onTapped: filamentMenu.open()
                                                    }
                                                }
                                            }

                                            // Simple inline preset selector (click to cycle)
                                            Text {
                                                color: Theme.textTertiary
                                                font.pixelSize: 9
                                                Layout.fillWidth: true
                                                wrapMode: Text.Wrap
                                                text: qsTr("点击下方按钮切换预设")
                                            }

                                            RowLayout {
                                                spacing: 4

                                                Repeater {
                                                    model: root.calibrationVm.filamentPresetNames
                                                    delegate: Rectangle {
                                                        required property string modelData
                                                        required property int index
                                                        width: filLabel.implicitWidth + 16
                                                        height: 26
                                                        radius: 6
                                                        color: root.calibrationVm.selectedFilamentPreset === modelData
                                                               ? Theme.accent : (filHov.containsMouse ? Theme.bgHover : Theme.bgPanel)
                                                        border.width: 1
                                                        border.color: root.calibrationVm.selectedFilamentPreset === modelData
                                                                    ? Theme.accent : Theme.borderSubtle

                                                        Text {
                                                            id: filLabel
                                                            anchors.centerIn: parent
                                                            text: modelData
                                                            color: root.calibrationVm.selectedFilamentPreset === modelData
                                                                   ? Theme.textOnAccent : Theme.textSecondary
                                                            font.pixelSize: 10
                                                            elide: Text.ElideRight
                                                        }

                                                        HoverHandler { id: filHov }
                                                        TapHandler {
                                                            onTapped: root.calibrationVm.selectedFilamentPreset = modelData
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // Progress bar (visible during calibration)
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: Theme.spacingSM
                                visible: root.calibrationVm.isRunning || root.calibrationVm.progress > 0

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: qsTr("Progress")
                                        color: Theme.textTertiary
                                        font.pixelSize: Theme.fontSizeSM
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: root.calibrationVm.progress + "%"
                                        color: Theme.textPrimary
                                        font.pixelSize: Theme.fontSizeSM
                                        font.bold: true
                                    }
                                }

                                ProgressBar {
                                    id: progressBar
                                    Layout.fillWidth: true
                                    from: 0
                                    to: 100
                                    value: root.calibrationVm.progress

                                    background: Rectangle {
                                        implicitHeight: 6
                                        radius: 3
                                        color: Theme.bgHover
                                    }

                                    contentItem: Item {
                                        Rectangle {
                                            width: progressBar.visualPosition * parent.width
                                            height: parent.height
                                            radius: 3
                                            color: Theme.accent
                                            Behavior on width { NumberAnimation { duration: 200 } }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Preview area (aligned with upstream bitmap area)
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 140
                        color: Theme.bgSurface
                        radius: Theme.radiusLG
                        border.color: Theme.borderSubtle
                        border.width: 1

                        Column {
                            anchors.centerIn: parent
                            spacing: Theme.spacingSM

                            Text {
                                text: root.calibrationVm.selectedPreviewLabel || ""
                                color: Theme.textDisabled
                                font.pixelSize: Theme.fontSizeMD
                                horizontalAlignment: Text.AlignHCenter
                                width: parent.width
                            }

                            Text {
                                text: qsTr("Preview area - connect printer to see real data")
                                color: Theme.bgHover
                                font.pixelSize: Theme.fontSizeXS
                                horizontalAlignment: Text.AlignHCenter
                                width: parent.width
                            }
                        }
                    }

                    // Action buttons (aligned with upstream CaliPageActionPanel)
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingMD

                        CxButton {
                            text: {
                                if (root.calibrationVm.isRunning)
                                    return qsTr("Cancel Calibration")
                                if (root.calibrationVm.selectedStatus === 2)
                                    return qsTr("Recalibrate")
                                return qsTr("Start Calibration")
                            }
                            cxStyle: root.calibrationVm.isRunning ? CxButton.Style.Danger : CxButton.Style.Primary
                            onClicked: {
                                if (root.calibrationVm.isRunning)
                                    root.calibrationVm.cancelCalibration()
                                else
                                    calibDlg.open()
                            }
                        }

                        CxButton {
                            text: qsTr("Reset Parameters")
                            cxStyle: CxButton.Style.Secondary
                            visible: root.calibrationVm.selectedStatus !== 0
                            onClicked: root.calibrationVm.resetParameters()
                        }

                        Item { Layout.fillWidth: true }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }
    }
}
