import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OWzxGL 1.0
import ".."
import "../controls"

Item {
    id: root

    required property var editorVm
    required property var viewport3d

    readonly property int viewportToolbarHeight: 34
    readonly property int toolbarButtonSize: 30
    readonly property int gizmoToolbarWidth: 36
    readonly property int toolbarGap: 4
    readonly property string iconBase: "qrc:/qml/assets/icons/"

    signal addModelRequested()
    signal fitViewRequested()
    signal sliceRequested()

    function canUseGizmo(mode) {
        return !!root.editorVm && ((root.editorVm.availableGizmoMask & (1 << mode)) !== 0)
    }

    function gizmoStatus(mode) {
        return root.editorVm ? root.editorVm.gizmoStatusText(mode) : qsTr("Backend unavailable")
    }

    function gizmoTip(text, mode) {
        var status = root.gizmoStatus(mode)
        return status === "Ready" ? text : text + " - " + status
    }

    function activateGizmo(mode) {
        if (root.viewport3d && root.canUseGizmo(mode))
            root.viewport3d.gizmoMode = mode
    }

    function iconForTool(toolId) {
        switch (toolId) {
        case GLViewport.GizmoMove:
            return root.iconBase + "maximize.svg"
        case GLViewport.GizmoRotate:
            return root.iconBase + "rotate-2.svg"
        case GLViewport.GizmoScale:
            return root.iconBase + "restore.svg"
        case GLViewport.GizmoFlatten:
            return root.iconBase + "mirror.svg"
        case GLViewport.GizmoCut:
        case GLViewport.GizmoAdvancedCut:
            return root.iconBase + "scissors.svg"
        case GLViewport.GizmoSupportPaint:
        case GLViewport.GizmoSeamPaint:
            return root.iconBase + "settings.svg"
        case GLViewport.GizmoSimplify:
            return root.iconBase + "layers-subtract.svg"
        case GLViewport.GizmoMeasure:
            return root.iconBase + "maximize.svg"
        case GLViewport.GizmoMeshBoolean:
            return root.iconBase + "box.svg"
        case GLViewport.GizmoEmboss:
        case GLViewport.GizmoSVG:
            return root.iconBase + "layout-grid.svg"
        default:
            return root.iconBase + "box.svg"
        }
    }

    Rectangle {
        id: viewportActionToolbar
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 8
        width: actionRow.implicitWidth + 10
        height: root.viewportToolbarHeight
        radius: 4
        color: "#2a2f36cc"
        border.width: 1
        border.color: "#69707880"

        RowLayout {
            id: actionRow
            anchors.centerIn: parent
            spacing: root.toolbarGap

            ActionToolButton {
                iconName: "plus.svg"
                toolTipText: qsTr("Add model")
                onClicked: root.addModelRequested()
            }

            ActionToolButton {
                iconName: "layout-grid-plus.svg"
                toolTipText: root.editorVm && root.editorVm.canAddPlate
                             ? qsTr("Add plate")
                             : qsTr("Maximum plate count reached")
                enabled: root.editorVm && root.editorVm.canAddPlate
                onClicked: root.editorVm.addPlate()
            }

            ActionToolButton {
                iconName: "rotate-2.svg"
                toolTipText: root.gizmoTip(qsTr("Auto orient"), GLViewport.GizmoRotate)
                enabled: root.canUseGizmo(GLViewport.GizmoRotate)
                onClicked: root.editorVm.autoOrientSelected()
            }

            ActionToolButton {
                iconName: "layout-grid.svg"
                toolTipText: root.editorVm && root.editorVm.canArrangeObjects
                             ? qsTr("Arrange all objects")
                             : qsTr("Load a model before arranging")
                enabled: root.editorVm && root.editorVm.canArrangeObjects
                onClicked: root.editorVm.arrangeAllObjects()
            }

            ToolbarSeparator { vertical: true }

            ActionToolButton {
                iconName: "clone.svg"
                toolTipText: root.editorVm && root.editorVm.canDuplicateSelectedObjects
                             ? qsTr("Duplicate selected objects")
                             : qsTr("Select one or more objects")
                enabled: root.editorVm && root.editorVm.canDuplicateSelectedObjects
                onClicked: root.editorVm.duplicateSelectedObjects()
            }

            ActionToolButton {
                iconName: "scissors.svg"
                toolTipText: root.gizmoTip(qsTr("Split object"), GLViewport.GizmoCut)
                enabled: root.canUseGizmo(GLViewport.GizmoCut)
                onClicked: root.editorVm.splitSelectedObject()
            }

            ActionToolButton {
                iconName: "layers.svg"
                toolTipText: qsTr("Layer editing is not backed by a Qt workflow yet")
                enabled: false
            }
        }
    }

    Rectangle {
        id: viewportGizmoToolbar
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 18
        width: root.gizmoToolbarWidth
        height: gizmoColumn.implicitHeight + 10
        radius: 4
        color: "#6f747c66"
        border.width: 1
        border.color: "#c3c7cd66"

        ColumnLayout {
            id: gizmoColumn
            anchors.centerIn: parent
            spacing: root.toolbarGap

            GizmoToolButton { toolId: GLViewport.GizmoMove; textTip: qsTr("Move"); iconSource: iconForTool(toolId) }
            GizmoToolButton { toolId: GLViewport.GizmoRotate; textTip: qsTr("Rotate"); iconSource: iconForTool(toolId) }
            GizmoToolButton { toolId: GLViewport.GizmoScale; textTip: qsTr("Scale"); iconSource: iconForTool(toolId) }

            ToolbarSeparator { }

            GizmoToolButton { toolId: GLViewport.GizmoFlatten; textTip: qsTr("Place on face"); iconSource: iconForTool(toolId) }
            GizmoToolButton { toolId: GLViewport.GizmoCut; textTip: qsTr("Cut"); iconSource: iconForTool(toolId) }
            GizmoToolButton { toolId: GLViewport.GizmoAdvancedCut; textTip: qsTr("Advanced cut"); iconSource: iconForTool(toolId) }

            ToolbarSeparator { }

            GizmoToolButton { toolId: GLViewport.GizmoSupportPaint; textTip: qsTr("Support painting"); iconSource: iconForTool(toolId) }
            GizmoToolButton { toolId: GLViewport.GizmoSeamPaint; textTip: qsTr("Seam painting"); iconSource: iconForTool(toolId) }
            GizmoToolButton { toolId: GLViewport.GizmoSimplify; textTip: qsTr("Simplify mesh"); iconSource: iconForTool(toolId) }
            GizmoToolButton { toolId: GLViewport.GizmoMeasure; textTip: qsTr("Measure"); iconSource: iconForTool(toolId) }
            GizmoToolButton { toolId: GLViewport.GizmoMeshBoolean; textTip: qsTr("Mesh boolean"); iconSource: iconForTool(toolId) }
            GizmoToolButton { toolId: GLViewport.GizmoEmboss; textTip: qsTr("Emboss"); iconSource: iconForTool(toolId) }
            GizmoToolButton { toolId: GLViewport.GizmoSVG; textTip: qsTr("SVG emboss"); iconSource: iconForTool(toolId) }
        }
    }

    Rectangle {
        id: viewportViewControls
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 18
        anchors.bottomMargin: 58
        width: viewRow.implicitWidth + 10
        height: root.viewportToolbarHeight
        radius: 4
        color: "#2a2f36cc"
        border.width: 1
        border.color: "#69707880"

        RowLayout {
            id: viewRow
            anchors.centerIn: parent
            spacing: root.toolbarGap

            ViewToolButton {
                iconName: "box.svg"
                toolTipText: qsTr("Top view")
                onClicked: if (root.viewport3d) root.viewport3d.requestViewPreset(0)
            }

            ViewToolButton {
                iconName: "layout-grid.svg"
                toolTipText: qsTr("Front view")
                onClicked: if (root.viewport3d) root.viewport3d.requestViewPreset(1)
            }

            ViewToolButton {
                iconName: "layout-sidebar-right.svg"
                toolTipText: qsTr("Right view")
                onClicked: if (root.viewport3d) root.viewport3d.requestViewPreset(2)
            }

            ViewToolButton {
                iconName: "box.svg"
                toolTipText: qsTr("Isometric view")
                onClicked: if (root.viewport3d) root.viewport3d.requestViewPreset(3)
            }

            ViewToolButton {
                iconName: "maximize.svg"
                toolTipText: qsTr("Fit view")
                onClicked: root.fitViewRequested()
            }
        }
    }

    component ToolbarSeparator: Rectangle {
        property bool vertical: false
        Layout.preferredWidth: vertical ? 1 : 24
        Layout.preferredHeight: vertical ? 22 : 1
        color: "#9097a066"
    }

    component ActionToolButton: CxIconButton {
        property string iconName: ""

        cxStyle: CxIconButton.Style.Ghost
        buttonSize: root.toolbarButtonSize
        iconSize: 16
        iconSource: root.iconBase + iconName
    }

    component ViewToolButton: CxIconButton {
        property string iconName: ""

        cxStyle: CxIconButton.Style.Ghost
        buttonSize: root.toolbarButtonSize
        iconSize: 16
        iconSource: root.iconBase + iconName
    }

    component GizmoToolButton: CxIconButton {
        property int toolId: -1
        property string textTip: ""

        cxStyle: CxIconButton.Style.Ghost
        buttonSize: root.toolbarButtonSize
        iconSize: 16
        iconSource: ""
        selected: root.viewport3d && root.viewport3d.gizmoMode === toolId
        enabled: root.canUseGizmo(toolId)
        toolTipText: root.gizmoTip(textTip, toolId)
        onClicked: root.activateGizmo(toolId)
    }
}
