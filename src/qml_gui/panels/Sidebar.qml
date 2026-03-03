import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// C6 — 右侧 Sidebar（可折叠，340px 宽，Tab：对象 / 打印 / 切片）
Item {
    id: root
    required property var editorVm
    required property var configVm

    property bool collapsed: false

    readonly property int expandedWidth:  340
    readonly property int collapsedWidth: 28

    implicitWidth: collapsed ? collapsedWidth : expandedWidth

    Behavior on implicitWidth { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

    // ── 折叠切换按钮 ──────────────────────────────────────────────
    Rectangle {
        id: toggleBtn
        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.left: parent.left
        width: 24
        height: 24
        radius: 4
        z: 10
        color: toggleMA.containsMouse ? "#2e3540" : "transparent"

        Text {
            anchors.centerIn: parent
            text: root.collapsed ? "›" : "‹"
            color: "#7a8fa3"
            font.pixelSize: 14
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
            spacing: 0

            // G5 — 预设快速选择栏 ─────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                height: 36
                color: "#191f2a"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 6
                    Text { text: qsTr("预设"); color: "#7a8fa3"; font.pixelSize: 11 }
                    ComboBox {
                        id: sidebarPresetCombo
                        Layout.fillWidth: true
                        font.pixelSize: 11
                        // 从 PresetListModel 读取所有预设（3 个分类合并展平）
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
                height: 34
                color: "#1e2330"

                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    Repeater {
                        model: [qsTr("对象"), qsTr("打印"), qsTr("切片")]
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            height: 34
                            color: tabBar.currentIndex === index ? "#2a313c" : "transparent"

                            Rectangle {
                                // 底部激活指示线
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                height: 2
                                color: tabBar.currentIndex === index ? "#22c564" : "transparent"
                            }

                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: tabBar.currentIndex === index ? "#e2e8f1" : "#7a8fa3"
                                font.pixelSize: 12
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

                // 隐藏的 TabBar 仅用于状态管理
                TabBar {
                    id: tabBar
                    visible: false
                    currentIndex: 0
                }
            }

            // 内容区域（StackLayout 切换面板）
            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: tabBar.currentIndex

                // 页 0 — 对象列表
                ObjectList {
                    editorVm: root.editorVm
                }

                // 页 1 — 打印设置（Loader 懒加载：避免 QAbstractListModel 在
                //         比较模式下提前实例化引发 debug heap 崩溃）
                Loader {
                    active: tabBar.currentIndex === 1
                    sourceComponent: Component {
                        PrintSettings {
                            configVm: root.configVm
                        }
                    }
                }

                // 页 2 — 切片进度
                SliceProgress {
                    editorVm: root.editorVm
                }
            }

            // G5 — 高级参数设置入口 ────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                height: 30
                color: "#0f1218"

                Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    anchors.topMargin: 4
                    anchors.bottomMargin: 4
                    radius: 4
                    color: advHov.containsMouse ? "#1e3040" : "#161d28"
                    border.color: "#2a3848"

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("⚙  高级设置")
                        color: "#9daaba"
                        font.pixelSize: 11
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

    // 折叠状态下的竖排文字提示
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: toggleBtn.bottom
        anchors.topMargin: 12
        visible: root.collapsed
        text: qsTr("侧\n栏")
        color: "#566070"
        font.pixelSize: 11
        horizontalAlignment: Text.AlignHCenter
        lineHeight: 1.6
    }
}
