import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../panels"
import "../dialogs"
import CrealityGL 1.0

Item {
    id: root
    required property var editorVm
    required property var configVm
    property alias viewport3dRef: viewport3d

    function undoFromTopbar() {
        viewport3d.undo()
    }
    function redoFromTopbar() {
        viewport3d.redo()
    }

    PrintDialog {
        id: printDlg
        editorVm: root.editorVm
    }

    FileDialog {
        id: openFileDlg
        title: qsTr("打开模型文件")
        nameFilters: [
            qsTr("3MF 文件 (*.3mf)"),
            qsTr("STL 文件 (*.stl)"),
            qsTr("OBJ 文件 (*.obj)"),
            qsTr("AMF 文件 (*.amf)"),
            qsTr("所有文件 (*)")
        ]
        onAccepted: {
            if (root.editorVm)
                root.editorVm.loadFile(selectedFile.toString())
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#30343b"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: "#2a2f37"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 6
                Repeater {
                    model: ["⛶","◻","◇","⌗","⛭","⊞","⊟","A"]
                    delegate: Rectangle {
                        width: 22
                        height: 22
                        radius: 2
                        color: toolHov.containsMouse ? "#3d8858" : "#2f343d"
                        border.color: "#434a57"
                        Text { anchors.centerIn: parent; text: modelData; color: "#8e98a8"; font.pixelSize: 11 }
                        // First button (⛶) opens the file dialog
                        MouseArea {
                            id: toolHov
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            visible: index === 0
                            onClicked: openFileDlg.open()
                            ToolTip.visible: containsMouse
                            ToolTip.text: qsTr("导入模型")
                            ToolTip.delay: 500
                        }
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            anchors.margins: 0
            spacing: 0

            Rectangle {
                Layout.preferredWidth: 232
                Layout.fillHeight: true
                color: "#2a2f36"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    Label { text: qsTr("平板"); color: "#dfe6ef"; font.pixelSize: 12 }
                    Repeater {
                        model: root.editorVm ? root.editorVm.plateCount : 0
                        delegate: Rectangle {
                            required property int index
                            Layout.preferredWidth: 72
                            Layout.preferredHeight: 56
                            radius: 4
                            color: root.editorVm && root.editorVm.currentPlateIndex === index ? "#3f2b2b" : "#313743"
                            border.color: root.editorVm && root.editorVm.currentPlateIndex === index ? "#00d36c" : "#4b5261"
                            Text {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.margins: 4
                                text: (index + 1).toString()
                                color: "#e8edf4"
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: 4
                                text: root.editorVm ? root.editorVm.plateName(index) : ""
                                color: "#c8d0dc"
                                font.pixelSize: 9
                                elide: Text.ElideRight
                                width: parent.width - 8
                                horizontalAlignment: Text.AlignHCenter
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (root.editorVm) {
                                        root.editorVm.setCurrentPlateIndex(index)
                                        root.editorVm.setShowAllObjects(false)
                                    }
                                }
                            }
                        }
                    }

                    Label { text: qsTr("打印机"); color: "#dfe6ef"; font.pixelSize: 12 }
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["Creality K1C 0.4 nozzle", "K1 Max 0.4 nozzle"]
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: [qsTr("光面PEI板/涂层板"), qsTr("普通PEI板")]
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Rectangle {
                            width: 28; height: 16; radius: 2
                            color: root.editorVm && root.editorVm.showAllObjects ? "#28be63" : "#3d434f"
                            Text { anchors.centerIn: parent; text: qsTr("全部"); color: "white"; font.pixelSize: 10 }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: { if (root.editorVm) root.editorVm.setShowAllObjects(true) }
                            }
                        }
                        Rectangle {
                            width: 28; height: 16; radius: 2
                            color: root.editorVm && !root.editorVm.showAllObjects ? "#28be63" : "#3d434f"
                            Text { anchors.centerIn: parent; text: qsTr("对象"); color: "#c8d0dc"; font.pixelSize: 10 }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: { if (root.editorVm) root.editorVm.setShowAllObjects(false) }
                            }
                        }
                        Item { Layout.fillWidth: true }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // E6 — real OpenGL viewport replaces the placeholder Rectangle
            GLViewport {
                id: viewport3d
                Layout.fillWidth: true
                Layout.fillHeight: true
                canvasType: GLViewport.CanvasView3D
                // 模型加载后把网格数据推给渲染线程
                meshData: root.editorVm ? root.editorVm.meshData : null

                // 监听 fitHint 变化: 模型加载完毕后自动适配相机
                Connections {
                    target: root.editorVm
                    function onStateChanged() {
                        if (!root.editorVm) return
                        var h = root.editorVm.fitHint
                        // fitHint.w = radius; 非零才有效
                        if (h && h.w > 0)
                            viewport3d.requestFitView(h.x, h.y, h.z, h.w)
                    }
                }

                // Drag-and-drop model files directly onto the viewport
                DropArea {
                    anchors.fill: parent
                    keys: ["text/uri-list"]
                    onDropped: (drop) => {
                        if (drop.hasUrls && drop.urls.length > 0 && root.editorVm)
                            root.editorVm.loadFile(drop.urls[0].toString())
                    }
                    // Dim-overlay while hovering
                    Rectangle {
                        anchors.fill: parent
                        color: "#40000000"
                        visible: parent.containsDrag
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("松开以导入模型")
                            color: "white"
                            font.pixelSize: 18
                            font.bold: true
                        }
                    }
                }
                Column {
                    spacing: 8
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    Repeater {
                        model: ["＋","☰","AUTO","🔒","⚙"]
                        delegate: Rectangle {
                            width: 36
                            height: 28
                            radius: 14
                            color: "#5b616b"
                            Text { anchors.centerIn: parent; text: modelData; color: "#dde4ed"; font.pixelSize: 11 }
                        }
                    }
                }

                Rectangle {
                    width: 34
                    height: 64
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.top: parent.top
                    anchors.topMargin: 16
                    color: "#8b8f96"
                    radius: 2
                    Text { anchors.centerIn: parent; text: qsTr("上\n前"); color: "#eef3fa"; horizontalAlignment: Text.AlignHCenter }
                }

                Row {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: 6
                    anchors.bottomMargin: 6
                    spacing: 4
                    // 切片角色
                    Rectangle {
                        width: 76; height: 22; color: "#575c64"; radius: 2
                        Text { anchors.centerIn: parent; text: qsTr("切片角色"); color: "#eef3fa"; font.pixelSize: 10 }
                    }
                    // 发送打印 — 打开 PrintDialog
                    Rectangle {
                        width: 76; height: 22
                        color: sendHov.containsMouse ? "#19a84e" : "#157a39"
                        radius: 2
                        Text { anchors.centerIn: parent; text: qsTr("发送打印"); color: "white"; font.pixelSize: 10 }
                        MouseArea {
                            id: sendHov
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: printDlg.open()
                        }
                    }
                }
            }

            Sidebar {
                id: sidebar
                Layout.fillHeight: true
                editorVm: root.editorVm
                configVm: root.configVm
            }
        }
    }
}
