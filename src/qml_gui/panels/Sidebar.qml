import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// C6 — 右侧 Sidebar（可折叠，340px 宽，Tab：对象 / 打印 / 切片）
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
            tabBar.currentIndex = 1
            if (root.configVm && root.editorVm && root.editorVm.canOpenSelectionSettings) {
                root.configVm.activateObjectScope(root.editorVm.settingsTargetType,
                                                  root.editorVm.settingsTargetName,
                                                  root.editorVm.settingsTargetObjectIndex,
                                                  root.editorVm.settingsTargetVolumeIndex)
            }
        }
    }

    Behavior on implicitWidth { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

    // ── 折叠切换按钮 ──────────────────────────────────────────────
    Rectangle {
        id: toggleBtn
        anchors.top: parent.top
        anchors.topMargin: 14
        anchors.left: parent.left
        anchors.leftMargin: -10
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

    // ── 展开内容区 ────────────────────────────────────────────────
    Item {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: toggleBtn.right
        anchors.right: parent.right
        visible: !root.collapsed
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 18
                color: "#1a202bd9"
                border.width: 1
                border.color: Theme.borderSubtle

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    // Tab 标签栏
                    Rectangle {
                        Layout.fillWidth: true
                        height: 38
                        radius: 12
                        color: Theme.bgPanel
                        border.width: 1
                        border.color: Theme.borderSubtle

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 4
                            spacing: 4

                            Repeater {
                                model: [qsTr("对象"), qsTr("打印"), qsTr("切片")]
                                delegate: Rectangle {
                                    Layout.fillWidth: true
                                    height: 30
                                    radius: 9
                                    color: tabBar.currentIndex === index ? Theme.bgElevated : "transparent"
                                    border.width: 1
                                    border.color: tabBar.currentIndex === index ? Theme.borderDefault : "transparent"

                                    Rectangle {
                                        anchors.bottom: parent.bottom
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        width: parent.width - 18
                                        height: 2
                                        radius: 1
                                        color: tabBar.currentIndex === index ? Theme.accent : "transparent"
                                    }

                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData
                                        color: tabBar.currentIndex === index ? Theme.textPrimary : Theme.textSecondary
                                        font.pixelSize: Theme.fontSizeMD
                                        font.bold: tabBar.currentIndex === index
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: tabBar.currentIndex = index
                                    }
                                }
                            }
                        }
                    }

                    TabBar {
                        id: tabBar
                        visible: false
                        currentIndex: 1
                    }

                    StackLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: tabBar.currentIndex

                        // Tab 0: 对象 — 变换面板 + 对象列表
                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 0

                                // ── ObjectManipulation 面板（对齐上游 ObjectManipulation）──
                                Rectangle {
                                    Layout.fillWidth: true
                                    implicitHeight: manipCol.implicitHeight + 16
                                    visible: root.editorVm && root.editorVm.hasObjectManipSelection
                                    color: Theme.bgPanel
                                    radius: Theme.radiusSM
                                    border.width: 1
                                    border.color: Theme.borderSubtle

                                    ColumnLayout {
                                        id: manipCol
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 8
                                        spacing: 8

                                        // 标题行
                                        RowLayout {
                                            Layout.fillWidth: true
                                            Text {
                                                text: qsTr("变换")
                                                color: Theme.textPrimary
                                                font.pixelSize: Theme.fontSizeSM
                                                font.bold: true
                                            }
                                            Item { Layout.fillWidth: true }
                                            // 统一缩放锁定
                                            Rectangle {
                                                width: uniformMA.containsMouse ? 56 : 50
                                                height: 20
                                                radius: 5
                                                color: root.editorVm && root.editorVm.uniformScale ? Theme.accentSubtle : Theme.bgElevated
                                                border.width: 1
                                                border.color: root.editorVm && root.editorVm.uniformScale ? Theme.accent : Theme.borderSubtle
                                                Text { anchors.centerIn: parent; text: qsTr("🔒 统一"); color: root.editorVm && root.editorVm.uniformScale ? Theme.accent : Theme.textDisabled; font.pixelSize: 9 }
                                                MouseArea {
                                                    id: uniformMA
                                                    anchors.fill: parent
                                                    hoverEnabled: true
                                                    cursorShape: Qt.PointingHandCursor
                                                    onClicked: { if (root.editorVm) root.editorVm.uniformScale = !root.editorVm.uniformScale }
                                                }
                                            }
                                            // 重置变换
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

                                        // 位置
                                        Repeater {
                                            model: [
                                                {label: "X", color: "#ef4444", px: "objectPosX", py: "objectPosY", pz: "objectPosZ", isScale: false},
                                                {label: "Y", color: "#22c55e", px: "objectPosY", py: "", pz: "", isScale: false},
                                                {label: "Z", color: "#3b82f6", px: "objectPosZ", py: "", pz: "", isScale: false}
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
                                                    color: "#0d1117"
                                                    border.width: 1
                                                    border.color: spinMA.containsMouse ? modelData.color : "#2e3848"
                                                    TextInput {
                                                        id: spinInput
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
                                                Text {
                                                    text: modelData.isScale ? "×" : "mm"
                                                    color: Theme.textDisabled
                                                    font.pixelSize: 9
                                                    Layout.preferredWidth: 18
                                                }
                                            }
                                        }

                                        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSubtle }

                                        // 旋转
                                        Text {
                                            text: qsTr("旋转 (°)")
                                            color: Theme.textTertiary
                                            font.pixelSize: 9
                                        }
                                        Repeater {
                                            model: [
                                                {label: "X", color: "#ef4444", px: "objectRotX", py: "objectRotY", pz: "objectRotZ", isScale: false},
                                                {label: "Y", color: "#22c55e", px: "objectRotY", py: "", pz: "", isScale: false},
                                                {label: "Z", color: "#3b82f6", px: "objectRotZ", py: "", pz: "", isScale: false}
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
                                                    color: "#0d1117"
                                                    border.width: 1
                                                    border.color: rotMA.containsMouse ? modelData.color : "#2e3848"
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
                                                Text {
                                                    text: "°"
                                                    color: Theme.textDisabled
                                                    font.pixelSize: 9
                                                    Layout.preferredWidth: 18
                                                }
                                            }
                                        }

                                        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSubtle }

                                        // 缩放
                                        Text {
                                            text: qsTr("缩放 (%)")
                                            color: Theme.textTertiary
                                            font.pixelSize: 9
                                        }
                                        Repeater {
                                            model: [
                                                {label: "X", color: "#ef4444", px: "objectScaleX", py: "objectScaleY", pz: "objectScaleZ", isScale: true},
                                                {label: "Y", color: "#22c55e", px: "objectScaleY", py: "", pz: "", isScale: true},
                                                {label: "Z", color: "#3b82f6", px: "objectScaleZ", py: "", pz: "", isScale: true}
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
                                                    color: "#0d1117"
                                                    border.width: 1
                                                    border.color: sclMA.containsMouse ? modelData.color : "#2e3848"
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
                                                Text {
                                                    text: "%"
                                                    color: Theme.textDisabled
                                                    font.pixelSize: 9
                                                    Layout.preferredWidth: 18
                                                }
                                            }
                                        }

                                        // 尺寸信息（只读）
                                        Rectangle {
                                            Layout.fillWidth: true
                                            height: dimRow.implicitHeight + 10
                                            radius: 4
                                            color: "#0d1117"
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

                                ObjectList {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    editorVm: root.editorVm
                                }
                            }
                        }

                        Loader {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            active: tabBar.currentIndex === 1
                            sourceComponent: Component {
                                PrintSettings {
                                    editorVm: root.editorVm
                                    configVm: root.configVm
                                    processCategory: root.processCategory
                                }
                            }
                        }

                        SliceProgress {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            editorVm: root.editorVm
                        }
                    }
                }
            }
        }
    }

    // 切换到打印标签页（供 ProcessBar 和底部操作调用）
    function switchToPrintTab() {
        tabBar.currentIndex = 1
    }

    // 折叠状态下的竖排文字提示
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
