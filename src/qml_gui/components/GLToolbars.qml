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
    // vp-3: variable layer height entry (upstream "layersediting" toolbar
    // item, GLCanvas3D.cpp:6970-6990). The page opens its layer-height
    // editor on this signal.
    signal layersEditingRequested()

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

    // vp-3 (TOOLICONS): one dedicated icon per tool, killing the former
    // one-image-many-meanings mapping (maximize shared by Move/Measure,
    // settings shared by both paint gizmos, mirror triple-booked). Icon
    // filenames are the upstream gizmo assets
    // (GLGizmosManager.cpp:206-224: toolbar_move/rotate/scale/flatten/cut/
    // support/seam/meshboolean/measure/text, reduce_triangles.svg for
    // Simplify, mmu_segmentation.svg for multicolor). Tools without an
    // upstream counterpart (SVG emboss, face detect, drill, advanced cut)
    // keep a feather fallback; Cut and AdvancedCut intentionally share the
    // cut icon (same meaning family, matching upstream's single Cut gizmo).
    function iconForTool(toolId) {
        switch (toolId) {
        case GLViewport.GizmoMove:
            return root.iconBase + "toolbar_move.svg"
        case GLViewport.GizmoRotate:
            return root.iconBase + "toolbar_rotate.svg"
        case GLViewport.GizmoScale:
            return root.iconBase + "toolbar_scale.svg"
        case GLViewport.GizmoFlatten:
            return root.iconBase + "toolbar_flatten.svg"
        case GLViewport.GizmoCut:
        case GLViewport.GizmoAdvancedCut:
            return root.iconBase + "toolbar_cut.svg"
        case GLViewport.GizmoSupportPaint:
            return root.iconBase + "toolbar_support.svg"
        case GLViewport.GizmoSeamPaint:
            return root.iconBase + "toolbar_seam.svg"
        case GLViewport.GizmoSimplify:
            return root.iconBase + "reduce_triangles.svg"
        case GLViewport.GizmoMeasure:
            return root.iconBase + "toolbar_measure.svg"
        case GLViewport.GizmoMeshBoolean:
            return root.iconBase + "toolbar_meshboolean.svg"
        case GLViewport.GizmoEmboss:
        case GLViewport.GizmoText:
            // Both Qt6 entries realize the single upstream Emboss gizmo
            // (GLGizmoEmboss, toolbar_text.svg, GLGizmosManager.cpp:216).
            return root.iconBase + "toolbar_text.svg"
        case GLViewport.GizmoSVG:
            // Upstream GLGizmoSVG ships no icon (GLGizmosManager.cpp:217);
            // keep the grid metaphor as its Qt6 fallback.
            return root.iconBase + "layout-grid.svg"
        case GLViewport.GizmoHollow:
            return root.iconBase + "param_hollow.svg"
        case GLViewport.GizmoMmuSegmentation:
            return root.iconBase + "mmu_segmentation.svg"
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

                // vp-3 (TOOLSET): the fixed nine-button set of the upstream
                // main toolbar (GLCanvas3D.cpp:6862-6990) with its dedicated
                // per-button SVG assets: add/addplate/orient/arrange, then a
                // separator, then more/fewer (instance +/-), splitobjects/
                // splitvolumes, layersediting. The former generic 15-button
                // feather set (delete/copy/paste/mirror x3/center/repair/
                // settings) is gone -- those actions live on the object list
                // and context menus, matching upstream, which does not put
                // them on this toolbar.

                ActionToolButton {
                    iconName: "toolbar_open.svg"
                    toolTipText: qsTr("Add")
                    onClicked: root.addModelRequested()
                }

                ActionToolButton {
                    iconName: "toolbar_add_plate.svg"
                    toolTipText: root.editorVm && root.editorVm.canAddPlate
                                 ? qsTr("Add plate")
                                 : qsTr("Maximum plate count reached")
                    enabled: root.editorVm && root.editorVm.canAddPlate
                    onClicked: root.editorVm.addPlate()
                }

                ActionToolButton {
                    iconName: "toolbar_orient.svg"
                    toolTipText: root.editorVm && root.editorVm.canArrangeObjects
                                 ? qsTr("Auto orient all/selected objects")
                                 : qsTr("Load a model before orienting")
                    enabled: root.editorVm && root.editorVm.canArrangeObjects
                    onClicked: root.editorVm.autoOrientSelected()
                }

                ActionToolButton {
                    iconName: "toolbar_arrange.svg"
                    toolTipText: root.editorVm && root.editorVm.canArrangeObjects
                                 ? qsTr("Arrange all objects")
                                 : qsTr("Load a model before arranging")
                    enabled: root.editorVm && root.editorVm.canArrangeObjects
                    onClicked: root.editorVm.arrangeAllObjects()
                }

                ToolbarSeparator { vertical: true }

                ActionToolButton {
                    iconName: "instance_add.svg"
                    toolTipText: root.editorVm && root.editorVm.hasSelection
                                 ? qsTr("Add instance")
                                 : qsTr("Select one or more objects")
                    enabled: root.editorVm && root.editorVm.hasSelection
                    onClicked: root.editorVm.addSelectedInstance()
                }

                ActionToolButton {
                    iconName: "instance_remove.svg"
                    toolTipText: root.editorVm && root.editorVm.hasSelection
                                 ? qsTr("Remove instance")
                                 : qsTr("Select one or more objects")
                    enabled: root.editorVm && root.editorVm.hasSelection
                    onClicked: root.editorVm.removeSelectedInstance()
                }

                ActionToolButton {
                    iconName: "split_objects.svg"
                    toolTipText: root.editorVm && root.editorVm.hasSelection
                                 ? qsTr("Split to objects")
                                 : qsTr("Select one or more objects")
                    enabled: root.editorVm && root.editorVm.hasSelection
                    onClicked: if (root.editorVm) root.editorVm.splitSelectedToObjects()
                }

                ActionToolButton {
                    iconName: "split_parts.svg"
                    toolTipText: root.editorVm && root.editorVm.hasSelection
                                 ? qsTr("Split to parts")
                                 : qsTr("Select one or more objects")
                    enabled: root.editorVm && root.editorVm.hasSelection
                    onClicked: if (root.editorVm) root.editorVm.splitSelectedToParts()
                }

                ActionToolButton {
                    iconName: "toolbar_variable_layer_height.svg"
                    toolTipText: root.editorVm && root.editorVm.hasSelection
                                 ? qsTr("Variable layer height")
                                 : qsTr("Select one or more objects")
                    enabled: root.editorVm && root.editorVm.hasSelection
                    onClicked: root.layersEditingRequested()
                }

                // BUILDGATE restore (2026-09-24): vp-3 trimmed the toolbar to
                // the upstream main-toolbar set, but the standing restoration
                // contracts (prepareRestoredControlsAreActionable,
                // prepareWorkflowActionsBindCppGates) still lock the selection
                // actions onto this toolbar. Re-anchored verbatim from the
                // last committed revision, minus the split-object button the
                // upstream split pair above already covers.
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
