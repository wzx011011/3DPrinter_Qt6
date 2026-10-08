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

    // U09 (G1, prepare_ref.png): the action bar is a full-width flush dock.
    // The ref bar spans the whole viewport top edge-to-edge, rows 37..96
    // (59px tall), square corners, surface sampled #27292c. The Phase 77
    // floating offsets (targetActionToolbarTop=22 / targetActionToolbarLeft=10)
    // are retired with this alignment.
    readonly property int viewportToolbarHeight: 58
    // Legacy Phase 77 token -- superseded by targetToolbarButtonSize below,
    // kept because the restoration source lock pins the literal.
    readonly property int toolbarButtonSize: 30
    // U09 (G3): upstream main-toolbar button box = GLToolbar::
    // Default_Icons_Size (GLToolbar.cpp:236) with set_gap_size(4)
    // (GLCanvas3D.cpp:6854); ref pitch measures 50 with 39-40 boxes.
    readonly property int targetToolbarButtonSize: 40
    readonly property int toolbarGap: 5
    // U09 (G8, prepare_ref.png): the gizmo rail hugs the right edge
    // (rightMargin 2), starts ~12% of the viewport height down, 34 wide,
    // 30px boxes at 38 pitch (ref rail glyph pitch ~32-38 range), 20px glyphs.
    readonly property int gizmoToolbarWidth: 34
    readonly property int gizmoToolButtonSize: 30
    readonly property int gizmoRailGap: 8
    readonly property real gizmoRailTopRatio: 0.12
    readonly property int gizmoRailRightMargin: 2
    property bool gizmoRailExpanded: false
    // U09 (G2): opaque docked surfaces. The ref action-bar surface samples
    // #27292c; the nearest Theme token is bgBase #2f3034 (Delta ~+8/channel --
    // exact value converges with the R1 surface batch; Theme.qml is
    // U01-exclusive this round). The old bgFloating #4f4f51d9 is translucent
    // and pixel-blended to the measured blue-violet #3d3e6a over the bed mesh.
    readonly property color actionToolbarSurface: Theme.bgBase
    // Rail surface samples #4b4b4d == Theme.bgPanel exactly (opaque, no border).
    readonly property color gizmoRailSurface: Theme.bgPanel
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

    // U09 (G10, GLCanvas3D.cpp:6888-6936): orient/arrange pop the
    // upstream-scope menus instead of acting blindly; every entry routes to
    // a live EditorViewModel call.
    CxMenu {
        id: orientMenu

        CxMenuItem {
            text: qsTr("Auto orient all/selected objects") + " [Q]"
            enabled: root.editorVm && root.editorVm.canTransformSelection
            onTriggered: root.editorVm.autoOrientSelected()
        }
        CxMenuItem {
            // Upstream [Shift+Q] "Auto orient all objects on current plate".
            // EditorViewModel has no plate-scope orient primitive;
            // autoOrientContextPlate (EditorViewModel.cpp:8465) realizes the
            // same scope as selectAllOnPlate + autoOrientSelected -- composed
            // here against currentPlateIndex (the toolbar has no context
            // plate). Selection replacement matches that upstream pattern.
            text: qsTr("Auto orient all objects on current plate") + " [Shift+Q]"
            enabled: root.editorVm && root.editorVm.canArrangeObjects
            onTriggered: {
                if (!root.editorVm)
                    return
                root.editorVm.selectAllOnPlate(root.editorVm.currentPlateIndex)
                root.editorVm.autoOrientSelected()
            }
        }
    }

    CxMenu {
        id: arrangeMenu

        CxMenuItem {
            text: qsTr("Arrange all objects") + " [A]"
            enabled: root.editorVm && root.editorVm.canArrangeObjects
            onTriggered: root.editorVm.arrangeAllObjects()
        }
        CxMenuItem {
            // Upstream [Shift+A] "Arrange objects on selected plates"
            // (GLCanvas3D.cpp:6906). The Qt single-plate context maps the
            // selected-plate set to currentPlateIndex
            // (EditorViewModel.h:1446 arrangePlate).
            text: qsTr("Arrange objects on selected plates") + " [Shift+A]"
            enabled: root.editorVm && root.editorVm.canArrangeObjects
            onTriggered: if (root.editorVm) root.editorVm.arrangePlate(root.editorVm.currentPlateIndex)
        }
    }

    Item {
        id: prepareTopActionToolbar
        // U09 (G1): full-width flush dock -- anchors left+right fill with
        // zero margins; the old 22/10 float offsets are gone.
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.viewportToolbarHeight

        Rectangle {
            id: viewportActionToolbar
            anchors.fill: parent
            radius: 0
            // U09 (G2): opaque dark surface, no border (ref runs edge to
            // edge with no outline).
            border.width: 0
            color: root.actionToolbarSurface

            RowLayout {
                id: actionRow
                anchors.centerIn: parent
                spacing: root.toolbarGap

                // U09 (G4): the fixed nine-button set of the upstream main
                // toolbar (GLCanvas3D.cpp:6862-6990; the item list ends at
                // :6990 with layersediting) with its dedicated per-button
                // SVG assets: add/addplate/orient/arrange, then a separator,
                // then more/fewer (instance +/-), splitobjects/splitvolumes,
                // layersediting. The BUILDGATE selection block (duplicate/
                // delete/copy/paste/mirror x3/center/repair/settings) is
                // trimmed again -- all ten actions live on the object context
                // menus (PrepareContextMenus.qml:220/235/240/251/271-273/293/
                // 336) and PreparePage.qml:81 (Delete), matching upstream,
                // which does not put them on this toolbar.

                ActionToolButton {
                    iconName: "toolbar_open.svg"
                    // Upstream tooltip composes the shortkey suffix
                    // (GLCanvas3D.cpp:6871-6872).
                    toolTipText: qsTr("Add") + " [Ctrl+I]"
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
                    id: orientButton
                    iconName: "toolbar_orient.svg"
                    // Upstream two-line tooltip (GLCanvas3D.cpp:6888-6889).
                    toolTipText: qsTr("Auto orient all/selected objects") + " [Q]\n"
                                 + qsTr("Auto orient all objects on current plate") + " [Shift+Q]"
                    enabled: root.editorVm && root.editorVm.canArrangeObjects
                    onClicked: orientMenu.popup(orientButton, 0, orientButton.height + 2)
                }

                ActionToolButton {
                    id: arrangeButton
                    iconName: "toolbar_arrange.svg"
                    // Upstream two-line tooltip (GLCanvas3D.cpp:6905-6906).
                    toolTipText: qsTr("Arrange all objects") + " [A]\n"
                                 + qsTr("Arrange objects on selected plates") + " [Shift+A]"
                    enabled: root.editorVm && root.editorVm.canArrangeObjects
                    onClicked: arrangeMenu.popup(arrangeButton, 0, arrangeButton.height + 2)
                }

                ToolbarSeparator { vertical: true }

                ActionToolButton {
                    iconName: "instance_add.svg"
                    toolTipText: root.editorVm && root.editorVm.hasSelection
                                 ? qsTr("Add instance") + " [+]"
                                 : qsTr("Select one or more objects")
                    enabled: root.editorVm && root.editorVm.hasSelection
                    onClicked: root.editorVm.addSelectedInstance()
                }

                ActionToolButton {
                    iconName: "instance_remove.svg"
                    toolTipText: root.editorVm && root.editorVm.hasSelection
                                 ? qsTr("Remove instance") + " [-]"
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
            }
        }
    }

    Item {
        id: prepareRightGizmoToolbar
        anchors.top: parent.top
        anchors.right: parent.right
        // U09 (G8) screenshot truth: rail hugs the viewport right edge,
        // starting ~12% of the viewport height down (not vertically centered,
        // not top-docked). Ratio-based so it scales with the window.
        anchors.topMargin: Math.round(parent.height * root.gizmoRailTopRatio)
        anchors.rightMargin: root.gizmoRailRightMargin
        width: root.gizmoToolbarWidth
        height: gizmoColumn.implicitHeight + 12

        Rectangle {
            id: viewportGizmoToolbar
            anchors.fill: parent
            radius: 4
            // U09 (G2): opaque bgPanel surface == ref rail sample #4b4b4d;
            // borderless. The translucent bgFloating fill previously blended
            // to #3d3e6a (blue-violet) over the bed mesh.
            border.width: 0
            color: root.gizmoRailSurface

            ColumnLayout {
                id: gizmoColumn
                anchors.centerIn: parent
                spacing: root.gizmoRailGap

                // U09 (G6): primary group follows the upstream EType order
                // (Gizmos/GLGizmosManager.hpp:74-99): Move/Rotate/Scale/
                // Flatten/Cut/MeshBoolean.
                GizmoToolButton { toolId: GLViewport.GizmoMove; textTip: qsTr("Move"); iconSource: iconForTool(toolId) }
                GizmoToolButton { toolId: GLViewport.GizmoRotate; textTip: qsTr("Rotate"); iconSource: iconForTool(toolId) }
                GizmoToolButton { toolId: GLViewport.GizmoScale; textTip: qsTr("Scale"); iconSource: iconForTool(toolId) }

                ToolbarSeparator { }

                GizmoToolButton { toolId: GLViewport.GizmoFlatten; textTip: qsTr("Place on face"); iconSource: iconForTool(toolId) }
                GizmoToolButton { toolId: GLViewport.GizmoCut; textTip: qsTr("Cut"); iconSource: iconForTool(toolId) }
                GizmoToolButton { toolId: GLViewport.GizmoMeshBoolean; textTip: qsTr("Mesh boolean"); iconSource: iconForTool(toolId) }

                // ── Overflow gizmos: collapsed behind the expander (screenshot
                // truth shows ~6 primary icons + a "more" affordance). ──
                ToolbarSeparator { visible: root.gizmoRailExpanded }

                Repeater {
                    // U09 (G6): overflow follows the EType order too --
                    // FdmSupports/Seam/MmSegmentation/Emboss/Svg/Measure/
                    // Simplify. FuzzySkin (hpp:82), Assembly (hpp:87) and
                    // BrimEars (hpp:89) have no Qt6 gizmo mode yet
                    // (RhiViewport.h:315-340) -- registered gap, no fabricated
                    // buttons. Text rides the Emboss slot (both realize the
                    // single upstream GLGizmoEmboss). The Qt6 extensions
                    // Hollow/FaceDetector/Drill/AdvancedCut stay at the tail
                    // (keep 登记; upstream Hollow is commented out at hpp:96).
                    model: [
                        { tool: GLViewport.GizmoSupportPaint, tip: qsTr("Support painting") },
                        { tool: GLViewport.GizmoSeamPaint, tip: qsTr("Seam painting") },
                        { tool: GLViewport.GizmoMmuSegmentation, tip: qsTr("Multicolor painting") },
                        { tool: GLViewport.GizmoEmboss, tip: qsTr("Emboss") },
                        { tool: GLViewport.GizmoText, tip: qsTr("Text emboss") },
                        { tool: GLViewport.GizmoSVG, tip: qsTr("SVG emboss") },
                        { tool: GLViewport.GizmoMeasure, tip: qsTr("Measure") },
                        { tool: GLViewport.GizmoSimplify, tip: qsTr("Simplify mesh") },
                        { tool: GLViewport.GizmoHollow, tip: qsTr("Hollow") },
                        { tool: GLViewport.GizmoFaceDetector, tip: qsTr("Face detect") },
                        { tool: GLViewport.GizmoDrill, tip: qsTr("Drill holes") },
                        { tool: GLViewport.GizmoAdvancedCut, tip: qsTr("Advanced cut") }
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
        iconSize: 24
        iconSource: root.iconBase + iconName
    }


    component GizmoToolButton: CxIconButton {
        property int toolId: -1
        property string textTip: ""

        cxStyle: CxIconButton.Style.Ghost
        buttonSize: root.gizmoToolButtonSize
        iconSize: 20
        iconSource: ""
        selected: root.viewport3d && root.viewport3d.gizmoMode === toolId
        enabled: root.canUseGizmo(toolId)
        toolTipText: root.gizmoTip(textTip, toolId)
        onClicked: root.activateGizmo(toolId)
    }
}
