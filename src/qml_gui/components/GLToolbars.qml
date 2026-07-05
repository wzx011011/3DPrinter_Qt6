import QtQuick
import QtQuick.Controls
import OWzxGL 1.0
import ".."

Item {
    id: root

    required property var editorVm
    required property var viewport3d

    signal addModelRequested()
    signal fitViewRequested()
    signal sliceRequested()

    function canUseGizmo(mode) {
        return !!root.editorVm && ((root.editorVm.availableGizmoMask & (1 << mode)) !== 0)
    }

    function gizmoStatus(mode) {
        return root.editorVm ? root.editorVm.gizmoStatusText(mode) : qsTr("Backend unavailable")
    }

    function gizmoTip(label, mode) {
        var status = root.gizmoStatus(mode)
        return status === "Ready" ? label : label + " - " + status
    }

    function activateGizmo(mode) {
        if (root.viewport3d && root.canUseGizmo(mode))
            root.viewport3d.gizmoMode = mode
    }

    Rectangle {
        id: mainToolbar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 14
        anchors.leftMargin: 14
        width: mainToolbarRow.implicitWidth + 18
        height: 42
        radius: 8
        color: Theme.bgFloating
        border.width: 1
        border.color: Theme.borderSubtle

        Row {
            id: mainToolbarRow
            anchors.centerIn: parent
            spacing: 4

            ToolButtonItem {
                label: "+"
                toolTipText: qsTr("Add model")
                enabled: true
                onClicked: root.addModelRequested()
            }

            ToolButtonItem {
                label: "P+"
                toolTipText: root.editorVm && root.editorVm.canAddPlate
                             ? qsTr("Add plate")
                             : qsTr("Maximum plate count reached")
                enabled: root.editorVm && root.editorVm.canAddPlate
                onClicked: root.editorVm.addPlate()
            }

            ToolButtonItem {
                label: "O"
                toolTipText: root.gizmoTip(qsTr("Auto orient"), GLViewport.GizmoRotate)
                enabled: root.canUseGizmo(GLViewport.GizmoRotate)
                onClicked: root.editorVm.autoOrientSelected()
            }

            ToolButtonItem {
                label: "A"
                toolTipText: root.editorVm && root.editorVm.canArrangeObjects
                             ? qsTr("Arrange all objects")
                             : qsTr("Load a model before arranging")
                enabled: root.editorVm && root.editorVm.canArrangeObjects
                onClicked: root.editorVm.arrangeAllObjects()
            }

            ToolbarSeparator { vertical: true }

            ToolButtonItem {
                label: "D"
                toolTipText: root.editorVm && root.editorVm.canDuplicateSelectedObjects
                             ? qsTr("Duplicate selected objects")
                             : qsTr("Select one or more objects")
                enabled: root.editorVm && root.editorVm.canDuplicateSelectedObjects
                onClicked: root.editorVm.duplicateSelectedObjects()
            }

            ToolButtonItem {
                label: "S"
                toolTipText: root.gizmoTip(qsTr("Split object"), GLViewport.GizmoCut)
                enabled: root.canUseGizmo(GLViewport.GizmoCut)
                onClicked: root.editorVm.splitSelectedObject()
            }

            ToolButtonItem {
                label: "L"
                toolTipText: qsTr("Layer editing is not backed by a Qt workflow yet")
                enabled: false
            }
        }
    }

    Rectangle {
        id: gizmosBar
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 14
        width: 42
        height: gizmosCol.implicitHeight + 16
        radius: 8
        color: Theme.bgFloating
        border.width: 1
        border.color: Theme.borderSubtle

        Column {
            id: gizmosCol
            anchors.centerIn: parent
            spacing: 2

            ToolButtonItem {
                label: "M"
                mode: GLViewport.GizmoMove
                toolTipText: root.gizmoTip(qsTr("Move"), mode)
                enabled: root.canUseGizmo(mode)
                active: root.viewport3d && root.viewport3d.gizmoMode === mode
                onClicked: root.activateGizmo(mode)
            }

            ToolButtonItem {
                label: "R"
                mode: GLViewport.GizmoRotate
                toolTipText: root.gizmoTip(qsTr("Rotate"), mode)
                enabled: root.canUseGizmo(mode)
                active: root.viewport3d && root.viewport3d.gizmoMode === mode
                onClicked: root.activateGizmo(mode)
            }

            ToolButtonItem {
                label: "S"
                mode: GLViewport.GizmoScale
                toolTipText: root.gizmoTip(qsTr("Scale"), mode)
                enabled: root.canUseGizmo(mode)
                active: root.viewport3d && root.viewport3d.gizmoMode === mode
                onClicked: root.activateGizmo(mode)
            }

            ToolbarSeparator { }

            ToolButtonItem {
                label: "F"
                mode: GLViewport.GizmoFlatten
                toolTipText: root.gizmoTip(qsTr("Place on face"), mode)
                enabled: root.canUseGizmo(mode)
                active: root.viewport3d && root.viewport3d.gizmoMode === mode
                onClicked: root.activateGizmo(mode)
            }

            ToolButtonItem {
                label: "C"
                mode: GLViewport.GizmoCut
                toolTipText: root.gizmoTip(qsTr("Cut"), mode)
                enabled: root.canUseGizmo(mode)
                active: root.viewport3d && root.viewport3d.gizmoMode === mode
                onClicked: root.activateGizmo(mode)
            }

            ToolButtonItem {
                label: "AC"
                mode: GLViewport.GizmoAdvancedCut
                toolTipText: root.gizmoTip(qsTr("Advanced cut"), mode)
                enabled: root.canUseGizmo(mode)
                active: root.viewport3d && root.viewport3d.gizmoMode === mode
                onClicked: root.activateGizmo(mode)
            }

            ToolbarSeparator { }

            ToolButtonItem {
                label: "SP"
                mode: GLViewport.GizmoSupportPaint
                toolTipText: root.gizmoTip(qsTr("Support painting"), mode)
                enabled: root.canUseGizmo(mode)
                active: root.viewport3d && root.viewport3d.gizmoMode === mode
                onClicked: root.activateGizmo(mode)
            }

            ToolButtonItem {
                label: "SE"
                mode: GLViewport.GizmoSeamPaint
                toolTipText: root.gizmoTip(qsTr("Seam painting"), mode)
                enabled: root.canUseGizmo(mode)
                active: root.viewport3d && root.viewport3d.gizmoMode === mode
                onClicked: root.activateGizmo(mode)
            }

            ToolButtonItem {
                label: "SI"
                mode: GLViewport.GizmoSimplify
                toolTipText: root.gizmoTip(qsTr("Simplify mesh"), mode)
                enabled: root.canUseGizmo(mode)
                active: root.viewport3d && root.viewport3d.gizmoMode === mode
                onClicked: root.activateGizmo(mode)
            }

            ToolButtonItem {
                label: "ME"
                mode: GLViewport.GizmoMeasure
                toolTipText: root.gizmoTip(qsTr("Measure"), mode)
                enabled: root.canUseGizmo(mode)
                active: root.viewport3d && root.viewport3d.gizmoMode === mode
                onClicked: root.activateGizmo(mode)
            }

            ToolButtonItem {
                label: "BO"
                mode: GLViewport.GizmoMeshBoolean
                toolTipText: root.gizmoTip(qsTr("Mesh boolean"), mode)
                enabled: root.canUseGizmo(mode)
                active: root.viewport3d && root.viewport3d.gizmoMode === mode
                onClicked: root.activateGizmo(mode)
            }

            ToolButtonItem {
                label: "E"
                mode: GLViewport.GizmoEmboss
                toolTipText: root.gizmoTip(qsTr("Emboss"), mode)
                enabled: root.canUseGizmo(mode)
                active: root.viewport3d && root.viewport3d.gizmoMode === mode
                onClicked: root.activateGizmo(mode)
            }

            ToolButtonItem {
                label: "SVG"
                mode: GLViewport.GizmoSVG
                toolTipText: root.gizmoTip(qsTr("SVG emboss"), mode)
                enabled: root.canUseGizmo(mode)
                active: root.viewport3d && root.viewport3d.gizmoMode === mode
                onClicked: root.activateGizmo(mode)
            }
        }
    }

    Rectangle {
        id: viewToolbar
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 14
        anchors.rightMargin: 14
        width: 42
        height: viewCol.implicitHeight + 16
        radius: 8
        color: Theme.bgFloating
        border.width: 1
        border.color: Theme.borderSubtle

        Column {
            id: viewCol
            anchors.centerIn: parent
            spacing: 2

            ToolButtonItem {
                label: "T"
                toolTipText: qsTr("Top view")
                onClicked: if (root.viewport3d) root.viewport3d.requestViewPreset(0)
            }

            ToolButtonItem {
                label: "F"
                toolTipText: qsTr("Front view")
                onClicked: if (root.viewport3d) root.viewport3d.requestViewPreset(1)
            }

            ToolButtonItem {
                label: "R"
                toolTipText: qsTr("Right view")
                onClicked: if (root.viewport3d) root.viewport3d.requestViewPreset(2)
            }

            ToolButtonItem {
                label: "I"
                toolTipText: qsTr("Isometric view")
                onClicked: if (root.viewport3d) root.viewport3d.requestViewPreset(3)
            }

            ToolButtonItem {
                label: "Z"
                toolTipText: qsTr("Fit view")
                onClicked: root.fitViewRequested()
            }
        }
    }

    component ToolbarSeparator: Rectangle {
        property bool vertical: false
        width: vertical ? 1 : 26
        height: vertical ? 22 : 1
        color: Theme.borderSubtle
    }

    component ToolButtonItem: Item {
        id: button
        property string label: ""
        property string toolTipText: ""
        property bool active: false
        property int mode: -1
        signal clicked()

        width: 34
        height: mode >= 0 ? 32 : 30
        opacity: enabled ? 1.0 : 0.35

        Rectangle {
            anchors.fill: parent
            radius: 6
            color: button.active ? Theme.accent
                : button.enabled && buttonMouse.containsMouse ? Theme.bgHover
                : "transparent"
            border.width: button.active ? 1 : 0
            border.color: button.active ? Theme.accent : "transparent"
        }

        Text {
            anchors.centerIn: parent
            text: button.label
            color: button.active ? Theme.textOnAccent
                : button.enabled ? Theme.textPrimary
                : Theme.textDisabled
            font.pixelSize: button.label.length > 2 ? 10 : 13
            font.bold: true
        }

        ToolTip.visible: buttonMouse.containsMouse && button.toolTipText.length > 0
        ToolTip.text: button.toolTipText
        ToolTip.delay: 400

        MouseArea {
            id: buttonMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: button.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: if (button.enabled) button.clicked()
        }
    }

}
