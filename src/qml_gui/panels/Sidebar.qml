import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"
import "../components"

// Sidebar — single-scroll collapsible sections (aligned with upstream Plater scrolled_sizer)
Item {
    id: root
    required property var editorVm
    required property var configVm

    property bool collapsed: false
    property string processCategory: ""

    readonly property int expandedWidth:  328
    readonly property int collapsedWidth: 32

    implicitWidth: collapsed ? collapsedWidth : expandedWidth

    Connections {
        target: root.editorVm

        function onSelectionSettingsRequested() {
            printSection.expanded = true
            if (root.configVm && root.editorVm && root.editorVm.canOpenSelectionSettings) {
                root.configVm.activateObjectScope(root.editorVm.settingsTargetType,
                                                  root.editorVm.settingsTargetName,
                                                  root.editorVm.settingsTargetObjectIndex,
                                                  root.editorVm.settingsTargetVolumeIndex)
            }
        }
    }

    Behavior on implicitWidth { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

    // ── Collapse toggle button ──
    Rectangle {
        id: toggleBtn
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: -14
        width: 28
        height: 28
        radius: 10
        z: 10
        color: toggleMA.containsMouse ? Theme.bgHover : Theme.bgPanel
        border.width: 1
        border.color: Theme.borderSubtle

        Text {
            anchors.centerIn: parent
            text: root.collapsed ? "›" : "‹"
            color: Theme.textSecondary
            font.pixelSize: 14
            font.bold: true
        }
        MouseArea {
            id: toggleMA
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.collapsed = !root.collapsed
        }
    }

    // ── Expanded content area ──
    Item {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: toggleBtn.right
        anchors.right: parent.right
        visible: !root.collapsed
        clip: true

        Rectangle {
            anchors.fill: parent
            radius: 18
            color: Theme.bgFloating
            border.width: 1
            border.color: Theme.borderSubtle

            CxScrollView {
                anchors.fill: parent
                anchors.margins: 8

                ColumnLayout {
                    width: parent.width
                    spacing: 4

                    // ── Section 1: Objects (collapsible) ──
                    CollapsibleSection {
                        id: objectSection
                        Layout.fillWidth: true
                        title: qsTr("对象")
                        iconText: "◇"
                        expanded: true

                        ObjectList {
                            width: parent.width
                            height: 200
                            editorVm: root.editorVm
                        }
                    }

                    // ── Section 2: Transform (collapsible, visible on selection) ──
                    CollapsibleSection {
                        Layout.fillWidth: true
                        title: qsTr("变换")
                        iconText: "✥"
                        expanded: true
                        visible: root.editorVm && root.editorVm.hasObjectManipSelection

                        ColumnLayout {
                            width: parent.width
                            spacing: 6

                                // Title row
                                RowLayout {
                                    Layout.fillWidth: true
                                    Item { Layout.fillWidth: true }
                                    Rectangle {
                                        width: uniformMA.containsMouse ? 56 : 50
                                        height: 20
                                        radius: 5
                                        color: root.editorVm && root.editorVm.uniformScale ? Theme.accentSubtle : Theme.bgElevated
                                        border.width: 1
                                        border.color: root.editorVm && root.editorVm.uniformScale ? Theme.accent : Theme.borderSubtle
                                        Text { anchors.centerIn: parent; text: qsTr("统一"); color: root.editorVm && root.editorVm.uniformScale ? Theme.accent : Theme.textDisabled; font.pixelSize: 9 }
                                        MouseArea {
                                            id: uniformMA
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: { if (root.editorVm) root.editorVm.uniformScale = !root.editorVm.uniformScale }
                                        }
                                    }
                                    Rectangle {
                                        width: resetMA.containsMouse ? 48 : 42
                                        height: 20
                                        radius: 5
                                        color: Theme.bgElevated
                                        border.width: 1
                                        border.color: Theme.borderSubtle
                                        Text { anchors.centerIn: parent; text: qsTr("重置"); color: Theme.textSecondary; font.pixelSize: 9 }
                                        MouseArea {
                                            id: resetMA
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: { if (root.editorVm) root.editorVm.resetObjectTransform() }
                                        }
                                    }
                                }

                                // Position
                                Repeater {
                                    model: [
                                        {label: "X", color: "#ef4444", px: "objectPosX"},
                                        {label: "Y", color: "#22c55e", px: "objectPosY"},
                                        {label: "Z", color: "#3b82f6", px: "objectPosZ"}
                                    ]
                                    delegate: RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 4
                                        required property var modelData

                                        Text {
                                            text: modelData.label
                                            color: modelData.color
                                            font.pixelSize: 10
                                            font.bold: true
                                            Layout.preferredWidth: 14
                                        }
                                        Rectangle {
                                            Layout.fillWidth: true
                                            height: 22
                                            radius: 4
                                            color: Theme.bgInset
                                            border.width: 1
                                            border.color: spinMA.containsMouse ? modelData.color : Theme.borderInput
                                            TextInput {
                                                anchors.fill: parent
                                                anchors.leftMargin: 4
                                                anchors.rightMargin: 4
                                                verticalAlignment: Text.AlignVCenter
                                                horizontalAlignment: Text.AlignRight
                                                color: Theme.textPrimary
                                                font.pixelSize: 10
                                                font.family: "monospace"
                                                selectByMouse: true
                                                validator: DoubleValidator { decimals: 2; notation: DoubleValidator.StandardNotation }
                                                text: {
                                                    if (!root.editorVm) return "0"
                                                    return Number(root.editorVm[modelData.px]).toFixed(2)
                                                }
                                                onEditingFinished: {
                                                    if (!root.editorVm) return
                                                    const val = parseFloat(text)
                                                    if (isNaN(val)) return
                                                    root.editorVm[modelData.px] = val
                                                }
                                            }
                                            MouseArea {
                                                id: spinMA
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                acceptedButtons: Qt.NoButton
                                            }
                                        }
                                        Text { text: "mm"; color: Theme.textDisabled; font.pixelSize: 9; Layout.preferredWidth: 18 }
                                    }
                                }

                                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSubtle }

                                // Rotation
                                Text { text: qsTr("旋转 (°)"); color: Theme.textTertiary; font.pixelSize: 9 }
                                Repeater {
                                    model: [
                                        {label: "X", color: "#ef4444", px: "objectRotX"},
                                        {label: "Y", color: "#22c55e", px: "objectRotY"},
                                        {label: "Z", color: "#3b82f6", px: "objectRotZ"}
                                    ]
                                    delegate: RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 4
                                        required property var modelData

                                        Text {
                                            text: modelData.label
                                            color: modelData.color
                                            font.pixelSize: 10
                                            font.bold: true
                                            Layout.preferredWidth: 14
                                        }
                                        Rectangle {
                                            Layout.fillWidth: true
                                            height: 22
                                            radius: 4
                                            color: Theme.bgInset
                                            border.width: 1
                                            border.color: rotMA.containsMouse ? modelData.color : Theme.borderInput
                                            TextInput {
                                                anchors.fill: parent
                                                anchors.leftMargin: 4
                                                anchors.rightMargin: 4
                                                verticalAlignment: Text.AlignVCenter
                                                horizontalAlignment: Text.AlignRight
                                                color: Theme.textPrimary
                                                font.pixelSize: 10
                                                font.family: "monospace"
                                                selectByMouse: true
                                                validator: DoubleValidator { decimals: 1; bottom: -360; top: 360 }
                                                text: {
                                                    if (!root.editorVm) return "0"
                                                    return Number(root.editorVm[modelData.px]).toFixed(1)
                                                }
                                                onEditingFinished: {
                                                    if (!root.editorVm) return
                                                    const val = parseFloat(text)
                                                    if (isNaN(val)) return
                                                    root.editorVm[modelData.px] = val
                                                }
                                            }
                                            MouseArea {
                                                id: rotMA
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                acceptedButtons: Qt.NoButton
                                            }
                                        }
                                        Text { text: "°"; color: Theme.textDisabled; font.pixelSize: 9; Layout.preferredWidth: 18 }
                                    }
                                }

                                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSubtle }

                                // Scale
                                Text { text: qsTr("缩放 (%)"); color: Theme.textTertiary; font.pixelSize: 9 }
                                Repeater {
                                    model: [
                                        {label: "X", color: "#ef4444", px: "objectScaleX"},
                                        {label: "Y", color: "#22c55e", px: "objectScaleY"},
                                        {label: "Z", color: "#3b82f6", px: "objectScaleZ"}
                                    ]
                                    delegate: RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 4
                                        required property var modelData

                                        Text {
                                            text: modelData.label
                                            color: modelData.color
                                            font.pixelSize: 10
                                            font.bold: true
                                            Layout.preferredWidth: 14
                                        }
                                        Rectangle {
                                            Layout.fillWidth: true
                                            height: 22
                                            radius: 4
                                            color: Theme.bgInset
                                            border.width: 1
                                            border.color: sclMA.containsMouse ? modelData.color : Theme.borderInput
                                            TextInput {
                                                anchors.fill: parent
                                                anchors.leftMargin: 4
                                                anchors.rightMargin: 4
                                                verticalAlignment: Text.AlignVCenter
                                                horizontalAlignment: Text.AlignRight
                                                color: Theme.textPrimary
                                                font.pixelSize: 10
                                                font.family: "monospace"
                                                selectByMouse: true
                                                validator: DoubleValidator { decimals: 1; bottom: 0.01 }
                                                text: {
                                                    if (!root.editorVm) return "100"
                                                    return (Number(root.editorVm[modelData.px]) * 100).toFixed(1)
                                                }
                                                onEditingFinished: {
                                                    if (!root.editorVm) return
                                                    const val = parseFloat(text)
                                                    if (isNaN(val) || val <= 0) return
                                                    root.editorVm[modelData.px] = val / 100.0
                                                }
                                            }
                                            MouseArea {
                                                id: sclMA
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                acceptedButtons: Qt.NoButton
                                            }
                                        }
                                        Text { text: "%"; color: Theme.textDisabled; font.pixelSize: 9; Layout.preferredWidth: 18 }
                                    }
                                }

                                // Dimensions info (read-only)
                                Rectangle {
                                    Layout.fillWidth: true
                                    height: dimRow.implicitHeight + 10
                                    radius: 4
                                    color: Theme.bgInset
                                    visible: root.editorVm && root.editorVm.measureDimensions.x > 0

                                    RowLayout {
                                        id: dimRow
                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 8
                                        spacing: 6
                                        Text { text: "W:"; color: "#ef4444"; font.pixelSize: 9; font.bold: true }
                                        Text { text: root.editorVm ? root.editorVm.measureDimensions.x.toFixed(1) : "--"; color: Theme.textSecondary; font.pixelSize: 9; font.family: "monospace" }
                                        Text { text: "D:"; color: "#22c55e"; font.pixelSize: 9; font.bold: true }
                                        Text { text: root.editorVm ? root.editorVm.measureDimensions.y.toFixed(1) : "--"; color: Theme.textSecondary; font.pixelSize: 9; font.family: "monospace" }
                                        Text { text: "H:"; color: "#3b82f6"; font.pixelSize: 9; font.bold: true }
                                        Text { text: root.editorVm ? root.editorVm.measureDimensions.z.toFixed(1) : "--"; color: Theme.textSecondary; font.pixelSize: 9; font.family: "monospace" }
                                    }
                                }
                        }
                    }

                    // ── Section 3: Filament (collapsible) ──
                    CollapsibleSection {
                        id: filamentSection
                        Layout.fillWidth: true
                        title: qsTr("耗材")
                        iconText: "◎"
                        expanded: false

                        FilamentPanel {
                            width: parent.width
                            editorVm: root.editorVm
                            configVm: root.configVm
                        }
                    }

                    // ── Section 4: Process/Print Settings (collapsible) ──
                    CollapsibleSection {
                        id: printSection
                        Layout.fillWidth: true
                        title: qsTr("打印设置")
                        iconText: "✦"
                        expanded: true

                        PrintSettings {
                            width: parent.width
                            height: 400
                            editorVm: root.editorVm
                            configVm: root.configVm
                            processCategory: root.processCategory
                        }
                    }

                    // ── Section 5: Slice Progress (collapsible, visible after slice) ──
                    CollapsibleSection {
                        id: sliceSection
                        Layout.fillWidth: true
                        title: qsTr("切片")
                        iconText: "☰"
                        expanded: true
                        visible: root.editorVm ? root.editorVm.isSlicing() : false

                        SliceProgress {
                            width: parent.width
                            height: 120
                            editorVm: root.editorVm
                        }
                    }

                    // ── Section 6: Auxiliary Files (对齐 upstream GUI_AuxiliaryList) ──
                    CollapsibleSection {
                        id: auxSection
                        Layout.fillWidth: true
                        title: qsTr("辅助文件")
                        iconText: "📁"
                        expanded: false

                        AuxiliaryListPanel {
                            width: parent.width
                            height: 260
                        }
                    }

                    Item { Layout.preferredHeight: 20 }
                }
            }
        }
    }

    function switchToPrintTab() {
        printSection.expanded = true
    }

    // Collapsed vertical text hint
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: toggleBtn.bottom
        anchors.topMargin: 12
        visible: root.collapsed
        text: qsTr("侧\n栏")
        color: Theme.textDisabled
        font.pixelSize: 11
        horizontalAlignment: Text.AlignHCenter
        lineHeight: 1.6
    }
}
