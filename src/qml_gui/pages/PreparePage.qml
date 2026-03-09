import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../controls"
import "../panels"
import "../dialogs"
import CrealityGL 1.0

Item {
    id: root
    required property var editorVm
    required property var configVm
    property alias viewport3dRef: viewport3d

    component QuickToolButton: Rectangle {
        id: tool
        property string label: ""
        property url iconSource: ""
        property string toolTipText: ""
        property bool active: false
        signal clicked()

        width: 32
        height: 32
        radius: 9
        color: active ? Theme.accentSubtle : (toolMouse.containsMouse ? Theme.bgHover : Theme.bgPanel)
        border.width: 1
        border.color: active ? Theme.accent : Theme.borderSubtle

        Item {
            anchors.fill: parent

            Image {
                anchors.centerIn: parent
                visible: tool.iconSource !== ""
                width: 16
                height: 16
                source: tool.iconSource
                fillMode: Image.PreserveAspectFit
                smooth: true
                opacity: tool.active ? 1.0 : 0.9
            }

            Text {
                anchors.centerIn: parent
                visible: tool.iconSource === ""
                text: tool.label
                color: tool.active ? Theme.accentLight : Theme.textSecondary
                font.pixelSize: Theme.fontSizeLG
                font.bold: tool.active
            }
        }

        MouseArea {
            id: toolMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: tool.clicked()
        }

        ToolTip.visible: toolMouse.containsMouse && tool.toolTipText.length > 0
        ToolTip.text: tool.toolTipText
        ToolTip.delay: 400
    }

    component PillAction: Rectangle {
        id: chip
        property string label: ""
        property url iconSource: ""
        property bool primary: false
        signal clicked()

        radius: 16
        height: 34
        width: Math.max(96, chipRow.implicitWidth + 28)
        color: primary ? (chipMouse.containsMouse ? Theme.accentLight : Theme.accent) : (chipMouse.containsMouse ? Theme.bgHover : Theme.bgPanel)
        border.width: 1
        border.color: primary ? Theme.accentDark : Theme.borderSubtle

        Row {
            id: chipRow
            anchors.centerIn: parent
            spacing: 6

            Image {
                visible: chip.iconSource !== ""
                width: 14
                height: 14
                source: chip.iconSource
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            Text {
                id: chipLabel
                text: chip.label
                color: primary ? Theme.textOnAccent : Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                font.bold: primary
            }
        }

        MouseArea {
            id: chipMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: chip.clicked()
        }
    }

    function applyFitHintIfReady() {
        if (!root.editorVm)
            return
        var h = root.editorVm.fitHint
        if (h && h.w > 0)
            viewport3d.requestFitView(h.x, h.y, h.z, h.w)
    }

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
        color: Theme.bgBase

        GLViewport {
            id: viewport3d
            anchors.fill: parent
            canvasType: GLViewport.CanvasView3D
            meshData: root.editorVm ? root.editorVm.meshData : null

            Connections {
                target: root.editorVm
                function onStateChanged() {
                    root.applyFitHintIfReady()
                }
            }

            onVisibleChanged: {
                if (visible)
                    Qt.callLater(root.applyFitHintIfReady)
            }

            Component.onCompleted: Qt.callLater(root.applyFitHintIfReady)

            DropArea {
                anchors.fill: parent
                keys: ["text/uri-list"]
                onDropped: (drop) => {
                    if (drop.hasUrls && drop.urls.length > 0 && root.editorVm)
                        root.editorVm.loadFile(drop.urls[0].toString())
                }

                Rectangle {
                    anchors.fill: parent
                    color: "#4a0b1018"
                    visible: parent.containsDrag

                    Rectangle {
                        anchors.centerIn: parent
                        width: 260
                        height: 64
                        radius: Theme.radiusXL
                        color: Theme.bgPanel
                        border.width: 1
                        border.color: Theme.accent

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("松开以导入模型")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeXL
                            font.bold: true
                        }
                    }
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#1a213018" }
                GradientStop { position: 0.4; color: "transparent" }
                GradientStop { position: 1.0; color: "#09101840" }
            }
        }

        Rectangle {
            id: topTools
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 14
            width: toolsRow.implicitWidth + 14
            height: 42
            radius: 14
            color: "#191f2acc"
            border.width: 1
            border.color: Theme.borderSubtle

            RowLayout {
                id: toolsRow
                anchors.centerIn: parent
                spacing: Theme.spacingSM

                QuickToolButton {
                    iconSource: "qrc:/qml/assets/icons/folder-plus.svg"
                    toolTipText: qsTr("导入模型")
                    active: true
                    onClicked: openFileDlg.open()
                }
                QuickToolButton {
                    iconSource: "qrc:/qml/assets/icons/box.svg"
                    toolTipText: qsTr("对象视图")
                }
                QuickToolButton {
                    iconSource: "qrc:/qml/assets/icons/rotate-2.svg"
                    toolTipText: qsTr("重置视角")
                    onClicked: root.applyFitHintIfReady()
                }
                QuickToolButton {
                    iconSource: "qrc:/qml/assets/icons/layout-sidebar-right.svg"
                    toolTipText: sidebar.collapsed ? qsTr("展开侧栏") : qsTr("收起侧栏")
                    active: !sidebar.collapsed
                    onClicked: sidebar.collapsed = !sidebar.collapsed
                }
                QuickToolButton {
                    iconSource: "qrc:/qml/assets/icons/list-details.svg"
                    toolTipText: qsTr("对象列表")
                }
                QuickToolButton {
                    iconSource: "qrc:/qml/assets/icons/settings.svg"
                    toolTipText: qsTr("准备页设置")
                }
            }
        }

        Rectangle {
            id: leftPanel
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: 14
            anchors.topMargin: 66
            anchors.bottomMargin: 16
            width: 224
            radius: 18
            color: "#1a202bd9"
            border.width: 1
            border.color: Theme.borderSubtle

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingLG
                spacing: Theme.spacingMD

                Text {
                    text: qsTr("平板")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeLG
                    font.bold: true
                }

                Repeater {
                    model: root.editorVm ? root.editorVm.plateCount : 0
                    delegate: Rectangle {
                        required property int index
                        Layout.fillWidth: true
                        Layout.preferredHeight: 54
                        radius: 12
                        color: root.editorVm && root.editorVm.currentPlateIndex === index ? Theme.accentSubtle : Theme.bgElevated
                        border.width: 1
                        border.color: root.editorVm && root.editorVm.currentPlateIndex === index ? Theme.accent : Theme.borderDefault

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 10

                            Rectangle {
                                width: 26
                                height: 26
                                radius: 8
                                color: "#101720"
                                border.width: 1
                                border.color: Theme.borderSubtle

                                Text {
                                    anchors.centerIn: parent
                                    text: (index + 1).toString()
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeMD
                                    font.bold: true
                                }
                            }

                            Column {
                                spacing: 2

                                Text {
                                    text: root.editorVm ? root.editorVm.plateName(index) : ""
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeMD
                                    font.bold: true
                                }

                                Text {
                                    text: root.editorVm && root.editorVm.currentPlateIndex === index ? qsTr("当前工作平板") : qsTr("点击切换查看")
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSizeSM
                                }
                            }
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

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.borderSubtle
                }

                Text {
                    text: qsTr("打印机")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeLG
                    font.bold: true
                }

                CxComboBox {
                    Layout.fillWidth: true
                    model: ["Creality K1C 0.4 nozzle", "K1 Max 0.4 nozzle"]
                }

                CxComboBox {
                    Layout.fillWidth: true
                    model: [qsTr("光面PEI板/涂层板"), qsTr("普通PEI板")]
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSM

                    Rectangle {
                        Layout.fillWidth: true
                        height: 30
                        radius: 10
                        color: root.editorVm && root.editorVm.showAllObjects ? Theme.accent : Theme.bgElevated
                        border.width: 1
                        border.color: root.editorVm && root.editorVm.showAllObjects ? Theme.accentDark : Theme.borderSubtle

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("全部")
                            color: root.editorVm && root.editorVm.showAllObjects ? Theme.textOnAccent : Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.editorVm) root.editorVm.setShowAllObjects(true)
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 30
                        radius: 10
                        color: root.editorVm && !root.editorVm.showAllObjects ? Theme.accent : Theme.bgElevated
                        border.width: 1
                        border.color: root.editorVm && !root.editorVm.showAllObjects ? Theme.accentDark : Theme.borderSubtle

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("对象")
                            color: root.editorVm && !root.editorVm.showAllObjects ? Theme.textOnAccent : Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.editorVm) root.editorVm.setShowAllObjects(false)
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        Sidebar {
            id: sidebar
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.topMargin: 14
            anchors.rightMargin: 14
            anchors.bottomMargin: 14
            width: implicitWidth
            editorVm: root.editorVm
            configVm: root.configVm
        }

        Column {
            anchors.right: sidebar.left
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            Repeater {
                model: [
                    { icon: "qrc:/qml/assets/icons/folder-plus.svg", tip: qsTr("导入模型") },
                    { icon: "qrc:/qml/assets/icons/list-details.svg", tip: qsTr("对象列表") },
                    { icon: "qrc:/qml/assets/icons/rotate-2.svg", tip: qsTr("视图重置") },
                    { icon: "qrc:/qml/assets/icons/lock.svg", tip: qsTr("锁定视图") },
                    { icon: "qrc:/qml/assets/icons/settings.svg", tip: qsTr("准备页设置") }
                ]
                delegate: Rectangle {
                    width: 38
                    height: 38
                    radius: 19
                    color: "#1a202bd9"
                    border.width: 1
                    border.color: Theme.borderSubtle

                    Image {
                        anchors.centerIn: parent
                        width: 16
                        height: 16
                        source: modelData.icon
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                    }

                    MouseArea {
                        id: sideToolMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (index === 0)
                                openFileDlg.open()
                            else if (index === 2)
                                root.applyFitHintIfReady()
                            else if (index === 4)
                                backend.openSettings()
                        }
                    }

                    ToolTip.visible: sideToolMouse.containsMouse
                    ToolTip.text: modelData.tip
                    ToolTip.delay: 400
                }
            }
        }

        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: 68
            anchors.right: sidebar.left
            anchors.rightMargin: 14
            width: 46
            height: 62
            radius: 14
            color: "#1a202bd9"
            border.width: 1
            border.color: Theme.borderSubtle

            Text {
                anchors.centerIn: parent
                text: qsTr("上\n前")
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontSizeMD
                font.bold: true
            }
        }

        Row {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 18
            anchors.right: sidebar.left
            anchors.rightMargin: 18
            spacing: 8

            PillAction {
                iconSource: "qrc:/qml/assets/icons/settings.svg"
                label: qsTr("打印配置")
            }

            PillAction {
                iconSource: "qrc:/qml/assets/icons/send-2.svg"
                label: qsTr("发送打印")
                primary: true
                onClicked: printDlg.open()
            }
        }
    }
}
