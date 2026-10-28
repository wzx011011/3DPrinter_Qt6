import QtQuick
pragma ComponentBehavior: Bound
import QtQuick.Controls
import QtQuick.Layouts
import OWzx

Item {
    id: root

    required property var editorVm
    required property var viewport3d

    // 2026-10-09 upstream re-alignment: the viewport carries ONE floating,
    // content-width strip pinned top-center, holding the upstream main
    // toolbar and the gizmo row that continues right after it. Upstream
    // anchors the main toolbar with Layout::Horizontal + VO_Top + border 4
    // (GLCanvas3D.cpp:6845-6847), and the gizmo overlay renders on the same
    // top row starting at main_toolbar_offset + main_toolbar_width +
    // separator_width (GLGizmosManager.cpp do_render_overlay); the reference
    // strip spans ~985..2035 of the 2560px prepare shot, i.e. centered over
    // the viewport at content width. The former full-width flush dock (U09
    // G1) and the right-edge gizmo rail with its "More tools" expander (U09
    // G8) are retired under the 2026-10-07 "layout follows upstream"
    // ruling -- upstream has no rail and no expander: every selectable
    // gizmo is always visible.
    readonly property int viewportToolbarHeight: 58
    // Upstream main-toolbar button box = GLToolbar::Default_Icons_Size
    // (GLToolbar.cpp:236) with set_gap_size(4) (GLCanvas3D.cpp:6854).
    readonly property int targetToolbarButtonSize: 40
    readonly property int toolbarGap: 5
    // Opaque dark surface; the nearest Theme token is bgBase #2f3034.
    // (Colors stay brand-owned per the 2026-10-07 truth ruling; only layout
    // follows upstream.)
    readonly property color actionToolbarSurface: Theme.bgBase
    // TBLC (2026-10-20): the upstream icon assets are two-tone for a LIGHT
    // toolbar surface (glyph #2b3436 + teal #009688) -- on the dark strip
    // the glyph body matched the surface and the icons vanished. The dark/
    // variants remap the body to textSecondary and the accent to the brand
    // green; the base files remain the light-theme set (Theme.isDark flips
    // with the theme and re-resolves every iconSource binding).
    readonly property string iconBase: "qrc:/qml/assets/icons/"
                                       + (Theme.isDark ? "dark/" : "")

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

    // vp-3 (TOOLICONS): one dedicated icon per tool, using the upstream
    // gizmo assets VERBATIM from third_party/OrcaSlicer/resources/images
    // (2026-10-27 user ruling: "直接拿orca的来用" — the hand-drawn outline
    // family was rejected). base/ = upstream X.svg (light), dark/ = upstream
    // X_dark.svg, accent/ = X_dark recolored monochrome brand-green (active
    // gizmo state per the TBLC ruling). MmuSegmentation/FuzzySkin map to
    // their dedicated upstream art (mmu_segmentation/fuzzy_skin_paint).
    function iconForTool(toolId, accentVariant) {
        const base = accentVariant
                ? "qrc:/qml/assets/icons/accent/"
                : root.iconBase
        switch (toolId) {
        case GLViewport.GizmoMove:
            return base + "toolbar_move.svg"
        case GLViewport.GizmoRotate:
            return base + "toolbar_rotate.svg"
        case GLViewport.GizmoScale:
            return base + "toolbar_scale.svg"
        case GLViewport.GizmoFlatten:
            return base + "toolbar_flatten.svg"
        case GLViewport.GizmoCut:
            return base + "toolbar_cut.svg"
        case GLViewport.GizmoSupportPaint:
            return base + "toolbar_support.svg"
        case GLViewport.GizmoSeamPaint:
            return base + "toolbar_seam.svg"
        case GLViewport.GizmoMeshBoolean:
            return base + "toolbar_meshboolean.svg"
        case GLViewport.GizmoEmboss:
            return base + "toolbar_text.svg"
        case GLViewport.GizmoMeasure:
            return base + "toolbar_measure.svg"
        case GLViewport.GizmoMmuSegmentation:
            // 2026-10-27: upstream's own assets (resources/images) replace
            // the generic box fallback — MmuSegmentation and FuzzySkin both
            // have dedicated upstream art.
            return base + "toolbar_mmu_segmentation.svg"
        case GLViewport.GizmoFuzzySkin:
            return base + "toolbar_fuzzy_skin_paint.svg"
        default:
            return base + "toolbar_box.svg"
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

    // The single floating strip: upstream nine-button main toolbar, then the
    // separator toolbar (GLCanvas3D.cpp:6938 _init_separator_toolbar), then
    // the gizmo row.
    Rectangle {
        id: viewportActionToolbar
        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.horizontalCenter: parent.horizontalCenter
        width: stripRow.implicitWidth + 16
        height: root.viewportToolbarHeight
        radius: Theme.radiusXL
        border.width: 0
        color: root.actionToolbarSurface

        RowLayout {
            id: stripRow
            anchors.centerIn: parent
            spacing: root.toolbarGap

            // U09 (G4): the fixed nine-button set of the upstream main
            // toolbar (GLCanvas3D.cpp:6862-6990; the item list ends at
            // :6990 with layersediting) with its dedicated per-button
            // SVG assets: add/addplate/orient/arrange, then a separator,
            // then more/fewer (instance +/-), splitobjects/splitvolumes,
            // layersediting. The BUILDGATE selection block (duplicate/
            // delete/copy/paste/mirror x3/center/repair/settings) is
            // trimmed -- all ten actions live on the object context
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
                iconName: "toolbar_instance_add.svg"
                toolTipText: root.editorVm && root.editorVm.hasSelection
                             ? qsTr("Add instance") + " [+]"
                             : qsTr("Select one or more objects")
                enabled: root.editorVm && root.editorVm.hasSelection
                onClicked: root.editorVm.addSelectedInstance()
            }

            ActionToolButton {
                iconName: "toolbar_instance_remove.svg"
                toolTipText: root.editorVm && root.editorVm.hasSelection
                             ? qsTr("Remove instance") + " [-]"
                             : qsTr("Select one or more objects")
                enabled: root.editorVm && root.editorVm.hasSelection
                onClicked: root.editorVm.removeSelectedInstance()
            }

            ActionToolButton {
                iconName: "toolbar_split_objects.svg"
                toolTipText: root.editorVm && root.editorVm.hasSelection
                             ? qsTr("Split to objects")
                             : qsTr("Select one or more objects")
                enabled: root.editorVm && root.editorVm.hasSelection
                onClicked: if (root.editorVm) root.editorVm.splitSelectedToObjects()
            }

            ActionToolButton {
                iconName: "toolbar_split_parts.svg"
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

            // Upstream separator toolbar between the main toolbar and the
            // gizmo row (GLCanvas3D.cpp:6938-6984; the gizmo overlay x
            // starts at main_toolbar_width + separator_width).
            ToolbarSeparator { vertical: true }

            // Gizmo row: exactly the upstream selectable set, in EType order
            // (GLGizmosManager.hpp:74-99 + get_selectable_idxs). Svg and
            // Simplify are NOT selectable upstream (GLGizmoSVG.hpp:89 /
            // GLGizmoSimplify.hpp:34): Svg opens from the Add > SVG menu and
            // emboss-volume double-click (GLCanvas3D.cpp:4845-4880) and
            // Simplify from the object context menu (GUI_Factories.cpp:2105
            // -> GUI_ObjectList.cpp:6255) -- both carriers already exist in
            // PrepareContextMenus.qml. P3 (2026-10-15) registered FuzzySkin
            // (hpp:82): mode 20 routes to the per-object fuzzy_skin config
            // channel (EditorViewModel fuzzySkin* API, upstream
            // GLGizmoFuzzySkin.cpp:326-359); the facet-painting half is not
            // ported, so the mode renders no overlay yet. Assembly (hpp:87)
            // and BrimEars (hpp:89) still have no Qt6 gizmo mode
            // (RhiViewport.h) -- registered gap, no fabricated buttons. The
            // former separate Text button duplicated the single upstream
            // GLGizmoEmboss; the former Qt6-only Hollow/FaceDetector/Drill/
            // AdvancedCut buttons are non-upstream (Hollow is commented out
            // at hpp:96) and were removed on 2026-10-09 -- their gizmo
            // modes stay registered in RhiViewport pending an OWzx-only
            // product decision.
            GizmoToolButton { toolId: GLViewport.GizmoMove; textTip: qsTr("Move") }
            GizmoToolButton { toolId: GLViewport.GizmoRotate; textTip: qsTr("Rotate") }
            GizmoToolButton { toolId: GLViewport.GizmoScale; textTip: qsTr("Scale") }
            GizmoToolButton { toolId: GLViewport.GizmoFlatten; textTip: qsTr("Place on face") }
            GizmoToolButton { toolId: GLViewport.GizmoCut; textTip: qsTr("Cut") }
            GizmoToolButton { toolId: GLViewport.GizmoMeshBoolean; textTip: qsTr("Mesh boolean") }
            GizmoToolButton { toolId: GLViewport.GizmoSupportPaint; textTip: qsTr("Support painting") }
            GizmoToolButton { toolId: GLViewport.GizmoSeamPaint; textTip: qsTr("Seam painting") }
            // Upstream EType order: Seam(7) -> FuzzySkin(8) -> MmSegmentation(9).
            // Upstream name GLGizmoFuzzySkin::on_get_name
            // (GLGizmoFuzzySkin.cpp:28) "Paint-on fuzzy skin".
            GizmoToolButton { toolId: GLViewport.GizmoFuzzySkin; textTip: qsTr("Paint-on fuzzy skin") }
            GizmoToolButton { toolId: GLViewport.GizmoMmuSegmentation; textTip: qsTr("Multicolor painting") }
            GizmoToolButton { toolId: GLViewport.GizmoEmboss; textTip: qsTr("Emboss") }
            GizmoToolButton { toolId: GLViewport.GizmoMeasure; textTip: qsTr("Measure") }
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
        bareBorder: true
        buttonSize: root.targetToolbarButtonSize
        // 2026-10-26 pixel audit: 28px on the 40px box (upstream strip
        // proportions); the outline family reads at this size.
        iconSize: 28
        iconSource: root.iconBase + iconName
    }

    component GizmoToolButton: CxIconButton {
        property int toolId: -1
        property string textTip: ""

        // TBLC-selected (2026-10-20): upstream marks the active gizmo by
        // swapping the icon to its accent state tile (GLGizmosManager.cpp
        // :1274 current -> tile 2, the native-accent sprite; the toolbar
        // background is shared and NO button capsule is drawn). The former
        // solid-accent CxIconButton capsule drowned the light-gray glyph
        // and read as an empty green block; the active tool now shows the
        // monochrome brand-green icon on the same transparent Ghost button.
        readonly property bool toolActive: root.viewport3d
                                           && root.viewport3d.gizmoMode === toolId
        cxStyle: CxIconButton.Style.Ghost
        bareBorder: true
        buttonSize: root.targetToolbarButtonSize
        iconSize: 28
        iconSource: root.iconForTool(toolId, toolActive)
        enabled: root.canUseGizmo(toolId)
        toolTipText: root.gizmoTip(textTip, toolId)
        onClicked: root.activateGizmo(toolId)
    }
}
