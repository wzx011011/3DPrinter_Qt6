import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// C3 — 对象列表面板
// 绑定到 EditorViewModel Q_INVOKABLE 访问器，避免 QVariantList 暴露到 QML
Item {
    id: root
    required property var editorVm

    // ── 顶部工具栏 ──────────────────────────────────────────────
    Rectangle {
        id: toolbar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 36
        radius: 12
        color: Theme.bgPanel
        border.width: 1
        border.color: Theme.borderSubtle

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 8
            spacing: 4

            Text {
                text: qsTr("对象列表")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
                font.bold: true
            }
            Rectangle {
                width: 54; height: 22; radius: 7
                color: root.editorVm && !root.editorVm.showAllObjects ? Theme.accent : Theme.bgElevated
                Text { anchors.centerIn: parent; text: qsTr("当前盘"); color: root.editorVm && !root.editorVm.showAllObjects ? Theme.textOnAccent : Theme.textPrimary; font.pixelSize: 9 }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { if (root.editorVm) root.editorVm.setShowAllObjects(false) }
                }
            }
            Rectangle {
                width: 42; height: 22; radius: 7
                color: root.editorVm && root.editorVm.showAllObjects ? Theme.accent : Theme.bgElevated
                Text { anchors.centerIn: parent; text: qsTr("全部"); color: root.editorVm && root.editorVm.showAllObjects ? Theme.textOnAccent : Theme.textPrimary; font.pixelSize: 9 }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { if (root.editorVm) root.editorVm.setShowAllObjects(true) }
                }
            }
            Item { Layout.fillWidth: true }
            Text {
                text: root.editorVm ? root.editorVm.objectCount + qsTr(" 个") : "0" + qsTr(" 个")
                color: Theme.textDisabled
                font.pixelSize: 11
            }
        }
    }

    // ── 对象列表 ─────────────────────────────────────────────────
    ListView {
        id: listView
        anchors.top: toolbar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: importBtn.top
        anchors.bottomMargin: 6
        clip: true
        model: root.editorVm ? root.editorVm.objectCount : 0

        delegate: Item {
            id: row
            width: listView.width
            height: 40
            required property int index

            readonly property bool isSelected: root.editorVm
                                               && row.index === root.editorVm.selectedObjectIndex
            readonly property bool objVisible: root.editorVm
                                               && root.editorVm.objectVisible(row.index)

            // 背景
            Rectangle {
                anchors.fill: parent
                radius: 10
                color: row.isSelected ? Theme.accentSubtle
                     : selMA.containsMouse ? Theme.bgHover
                     : "transparent"
                Rectangle {
                    // 左侧选中指示条
                    width: 3; height: parent.height - 10
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 2
                    radius: 2
                    color: row.isSelected ? Theme.accent : "transparent"
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 6
                spacing: 8

                // 可见性切换小圆点
                Rectangle {
                    width: 9; height: 9; radius: 5
                    color: row.objVisible ? Theme.accent : Theme.bgPressed
                    MouseArea {
                        anchors.fill: parent
                        z: 2
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (root.editorVm)
                                root.editorVm.setObjectVisible(row.index, !row.objVisible)
                        }
                    }
                }

                // 文件名
                Text {
                    Layout.fillWidth: true
                    text: root.editorVm ? root.editorVm.objectName(row.index) : ""
                    color: row.isSelected ? Theme.textPrimary : "#bbc7d4"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }

                // 删除按钮
                Rectangle {
                    width: 22; height: 22; radius: 7
                    color: delMA.containsMouse ? "#7d2020" : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        color: delMA.containsMouse ? "#ffaaaa" : Theme.textDisabled
                        font.pixelSize: 10
                    }
                    MouseArea {
                        id: delMA
                        anchors.fill: parent
                        z: 2
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { if (root.editorVm) root.editorVm.deleteObject(row.index) }
                    }
                }
            }

            // 选中点击层（最低 z，被按钮覆盖）
            MouseArea {
                id: selMA
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: { if (root.editorVm) root.editorVm.selectObject(row.index) }
            }
        }
    }

    // ── 空列表占位 ────────────────────────────────────────────────
    Text {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -20
        visible: !root.editorVm || root.editorVm.objectCount === 0
        text: qsTr("场景中无对象\n请从顶部菜单导入模型")
        color: Theme.textDisabled
        font.pixelSize: 12
        horizontalAlignment: Text.AlignHCenter
        lineHeight: 1.5
    }

    // ── 导入按钮 ─────────────────────────────────────────────────
    Rectangle {
        id: importBtn
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8
        height: 34
        radius: 12
        color: importMA.containsMouse ? Theme.accentLight : Theme.accent

        Text {
            anchors.centerIn: parent
            text: qsTr("+ 导入模型")
            color: Theme.textOnAccent
            font.pixelSize: 12
            font.bold: true
        }

        MouseArea {
            id: importMA
            anchors.fill: parent
            hoverEnabled: true
            enabled: false
            cursorShape: Qt.ArrowCursor
        }
    }
}
