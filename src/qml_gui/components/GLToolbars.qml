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
    readonly property int targetToolbarButtonSize: 30
    readonly property int gizmoToolbarWidth: 36
    readonly property int toolbarGap: 3
    readonly property int targetActionToolbarTop: 22
    // v5.14: viewport-relative anchors replace the Phase 77 absolute pixel
    // offsets (left 598 / top 392 / center+300) which only landed correctly
    // at one window size. Screenshot truth: action bar hugs the viewport
    // top-left; the gizmo rail hugs the viewport right edge starting ~22%
    // down, with overflow collapsed behind a "more" affordance.
    readonly property int targetActionToolbarLeft: 10
    readonly property real gizmoRailTopRatio: 0.22
    readonly property int gizmoRailRightMargin: 14
    property bool gizmoRailExpanded: false
    readonly property color targetToolbarSurface: Theme.bgFloating
    readonly property color targetToolbarBorder: Theme.borderSubtle
    readonly property string iconBase: "qrc:/qml/assets/icons/"

    signal addModelRequested()
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
        case GLViewport.GizmoHollow:
            // Phase 143 (VDB-04): Hollow gizmo. No dedicated icon shipped yet;
            // reuse the layers-subtract metaphor (a hollowed model subtracts
            // interior volume). Dedicated hollow.svg can be added later.
            return root.iconBase + "layers-subtract.svg"
        default:
            return root.iconBase + "box.svg"
        }
    }

    Item {
        id: prepareTopActionToolbar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: root.targetActionToolbarTop
        anchors.leftMargin: root.targetActionToolbarLeft
        width: actionRow.implicitWidth + 12
        height: root.viewportToolbarHeight

        Rectangle {
            id: viewportActionToolbar
            anchors.fill: parent
            radius: 3
            color: root.targetToolbarSurface
            border.width: 1
            border.color: root.targetToolbarBorder

            RowLayout {
                id: actionRow
                anchors.centerIn: parent
                spacing: root.toolbarGap

                ActionToolButton {
                    iconName: "box.svg"
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
                    iconName: "mirror.svg"
                    toolTipText: root.gizmoTip(qsTr("Auto orient"), GLViewport.GizmoRotate)
                    enabled: root.canUseGizmo(GLViewport.GizmoRotate)
                    onClicked: root.editorVm.autoOrientSelected()
                }

                ActionToolButton {
                    iconName: "list-details.svg"
                    toolTipText: root.editorVm && root.editorVm.canArrangeObjects
                                 ? qsTr("Arrange all objects")
                                 : qsTr("Load a model before arranging")
                    enabled: root.editorVm && root.editorVm.canArrangeObjects
                    onClicked: root.editorVm.arrangeAllObjects()
                }

                ToolbarSeparator { vertical: true }

                ActionToolButton {
                    iconName: "plus.svg"
                    toolTipText: root.editorVm && root.editorVm.canDuplicateSelectedObjects
                                 ? qsTr("Duplicate selected objects")
                                 : qsTr("Select one or more objects")
                    enabled: root.editorVm && root.editorVm.canDuplicateSelectedObjects
                    onClicked: root.editorVm.duplicateSelectedObjects()
                }

                ActionToolButton {
                    iconName: "minus.svg"
                    toolTipText: root.gizmoTip(qsTr("Split object"), GLViewport.GizmoCut)
                    enabled: root.canUseGizmo(GLViewport.GizmoCut)
                    onClicked: root.editorVm.splitSelectedObject()
                }

                ToolbarSeparator { vertical: true }

                ActionToolButton {
                    iconName: "trash.svg"
                    toolTipText: root.editorVm && root.editorVm.canDeleteSelection
                                 ? qsTr("Delete selected objects")
                                 : qsTr("Select one or more objects")
                    enabled: root.editorVm && root.editorVm.canDeleteSelection
                    onClicked: root.editorVm.deleteSelection()
                }

                ActionToolButton {
                    iconName: "copy.svg"
                    toolTipText: root.editorVm && root.editorVm.hasSelection
                                 ? qsTr("Copy selected objects")
                                 : qsTr("Select one or more objects")
                    enabled: root.editorVm && root.editorVm.hasSelection
                    onClicked: root.editorVm.copySelectedObjects()
                }

                ActionToolButton {
                    iconName: "clipboard.svg"
                    toolTipText: root.editorVm && root.editorVm.hasClipboardContent
                                 ? qsTr("Paste objects")
                                 : qsTr("Clipboard is empty")
                    enabled: root.editorVm && root.editorVm.hasClipboardContent
                    onClicked: root.editorVm.pasteObjects()
                }

                ToolbarSeparator { vertical: true }

                ActionToolButton {
                    iconName: "mirror.svg"
                    toolTipText: root.editorVm && root.editorVm.canTransformSelection
                                 ? qsTr("Mirror selected objects on X")
                                 : qsTr("Select one or more objects")
                    enabled: root.editorVm && root.editorVm.canTransformSelection
                    onClicked: root.editorVm.mirrorSelectedObjects(0)
                }

                ActionToolButton {
                    iconName: "layout-sidebar-right.svg"
                    toolTipText: root.editorVm && root.editorVm.canTransformSelection
                                 ? qsTr("Mirror selected objects on Y")
                                 : qsTr("Select one or more objects")
                    enabled: root.editorVm && root.editorVm.canTransformSelection
                    onClicked: root.editorVm.mirrorSelectedObjects(1)
                }

                ActionToolButton {
                    iconName: "layers.svg"
                    toolTipText: root.editorVm && root.editorVm.canTransformSelection
                                 ? qsTr("Mirror selected objects on Z")
                                 : qsTr("Select one or more objects")
                    enabled: root.editorVm && root.editorVm.canTransformSelection
                    onClicked: root.editorVm.mirrorSelectedObjects(2)
                }

                ToolbarSeparator { vertical: true }

                ActionToolButton {
                    iconName: "maximize.svg"
                    toolTipText: root.editorVm && root.editorVm.canTransformSelection
                                 ? qsTr("Center selected objects")
                                 : qsTr("Select one or more objects")
                    enabled: root.editorVm && root.editorVm.canTransformSelection
                    onClicked: root.editorVm.centerSelectedObjects()
                }

                ActionToolButton {
                    iconName: "layers-subtract.svg"
                    toolTipText: root.editorVm && root.editorVm.canTransformSelection
                                 ? qsTr("Repair selected mesh")
                                 : qsTr("Select one or more objects")
                    enabled: root.editorVm && root.editorVm.canTransformSelection
                    onClicked: root.editorVm.fixMeshSelected()
                }

                ActionToolButton {
                    iconName: "settings.svg"
                    toolTipText: root.editorVm && root.editorVm.canOpenSelectionSettings
                                 ? qsTr("Object settings")
                                 : qsTr("Select an object or volume")
                    enabled: root.editorVm && root.editorVm.canOpenSelectionSettings
                    onClicked: root.editorVm.requestSelectionSettings()
                }
            }
        }
    }

    Item {
        id: prepareRightGizmoToolbar
        anchors.top: parent.top
        anchors.right: parent.right
        // Screenshot truth: rail floats just inside the viewport right edge,
        // starting ~22% of the viewport height down (not vertically centered,
        // not top-docked). Ratio-based so it scales with the window.
        anchors.topMargin: Math.round(parent.height * root.gizmoRailTopRatio)
        anchors.rightMargin: root.gizmoRailRightMargin
        width: root.gizmoToolbarWidth
        height: gizmoColumn.implicitHeight + 12

        Rectangle {
            id: viewportGizmoToolbar
            anchors.fill: parent
            radius: 4
            color: root.targetToolbarSurface
            border.width: 1
            border.color: root.targetToolbarBorder

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

                // ── Overflow gizmos: collapsed behind the expander (screenshot
                // truth shows ~6 primary icons + a "more" affordance). ──
                ToolbarSeparator { visible: root.gizmoRailExpanded }

                Repeater {
                    model: [
                        { tool: GLViewport.GizmoSupportPaint, tip: qsTr("Support painting") },
                        { tool: GLViewport.GizmoSeamPaint, tip: qsTr("Seam painting") },
                        { tool: GLViewport.GizmoSimplify, tip: qsTr("Simplify mesh") },
                        { tool: GLViewport.GizmoMeasure, tip: qsTr("Measure") },
                        { tool: GLViewport.GizmoMeshBoolean, tip: qsTr("Mesh boolean") },
                        { tool: GLViewport.GizmoEmboss, tip: qsTr("Emboss") },
                        { tool: GLViewport.GizmoSVG, tip: qsTr("SVG emboss") },
                        { tool: GLViewport.GizmoHollow, tip: qsTr("Hollow") },
                        { tool: GLViewport.GizmoMmuSegmentation, tip: qsTr("Multicolor painting") },
                        { tool: GLViewport.GizmoFaceDetector, tip: qsTr("Face detect") },
                        { tool: GLViewport.GizmoDrill, tip: qsTr("Drill holes") },
                        { tool: GLViewport.GizmoText, tip: qsTr("Text emboss") }
                    ]
                    delegate: GizmoToolButton {
                        required property var modelData
                        toolId: modelData.tool
                        textTip: modelData.tip
                        iconSource: iconForTool(toolId)
                        visible: root.gizmoRailExpanded
                    }
                }

                ToolbarSeparator { }

                GizmoToolButton {
                    toolId: -1
                    iconSource: root.iconBase + (root.gizmoRailExpanded ? "x.svg" : "dots.svg")
                    textTip: root.gizmoRailExpanded ? qsTr("Hide more tools") : qsTr("More tools")
                    toolTipText: root.gizmoRailExpanded ? qsTr("Hide more tools") : qsTr("More tools")
                    selected: root.gizmoRailExpanded
                    enabled: true
                    onClicked: root.gizmoRailExpanded = !root.gizmoRailExpanded
                }
            }
        }
    }

    component ToolbarSeparator: Rectangle {
        property bool vertical: false
        Layout.preferredWidth: vertical ? 1 : 24
        Layout.preferredHeight: vertical ? 22 : 1
        color: Theme.borderStrong
    }

    component ActionToolButton: CxIconButton {
        property string iconName: ""

        cxStyle: CxIconButton.Style.Ghost
        buttonSize: root.targetToolbarButtonSize
        iconSize: 16
        iconSource: root.iconBase + iconName
    }


    component GizmoToolButton: CxIconButton {
        property int toolId: -1
        property string textTip: ""

        cxStyle: CxIconButton.Style.Ghost
        buttonSize: root.targetToolbarButtonSize
        iconSize: 16
        iconSource: ""
        selected: root.viewport3d && root.viewport3d.gizmoMode === toolId
        enabled: root.canUseGizmo(toolId)
        toolTipText: root.gizmoTip(textTip, toolId)
        onClicked: root.activateGizmo(toolId)
    }
}
