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

    readonly property int expandedWidth:  328
    readonly property int collapsedWidth: 32

    implicitWidth: collapsed ? collapsedWidth : expandedWidth

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

                    // G5 — 预设快速选择栏 ─────────────────────────────────
                    Rectangle {
                        Layout.fillWidth: true
                        height: 40
                        radius: 12
                        color: Theme.bgPanel
                        border.width: 1
                        border.color: Theme.borderSubtle

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 8
                            Text { text: qsTr("预设"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM }
                            ComboBox {
                                id: sidebarPresetCombo
                                Layout.fillWidth: true
                                font.pixelSize: 11
                                model: {
                                    if (!root.configVm || !root.configVm.presetList) return []
                                    var pl = root.configVm.presetList
                                    var cats = [qsTr("打印质量"),qsTr("耗材"),qsTr("打印机")]
                                    var arr  = []
                                    for (var ci = 0; ci < cats.length; ++ci) {
                                        var n = pl.countByCategory(cats[ci])
                                        for (var pi = 0; pi < n; ++pi)
                                            arr.push(pl.presetName(pl.globalIndex(cats[ci], pi)))
                                    }
                                    return arr
                                }
                            }
                        }
                    }

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
                        currentIndex: 0
                    }

                    StackLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: tabBar.currentIndex

                        ObjectList {
                            editorVm: root.editorVm
                        }

                        Loader {
                            active: tabBar.currentIndex === 1
                            sourceComponent: Component {
                                PrintSettings {
                                    configVm: root.configVm
                                }
                            }
                        }

                        SliceProgress {
                            editorVm: root.editorVm
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 34
                        radius: 12
                        color: Theme.bgPanel
                        border.width: 1
                        border.color: Theme.borderSubtle

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 4
                            radius: 9
                            color: advHov.containsMouse ? Theme.bgHover : "transparent"
                            border.width: 1
                            border.color: advHov.containsMouse ? Theme.borderDefault : "transparent"

                            Text {
                                anchors.centerIn: parent
                                text: qsTr("⚙  高级设置")
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeMD
                            }

                            MouseArea {
                                id: advHov
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: backend.openSettings()
                            }
                        }
                    }
                }
            }
        }
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
