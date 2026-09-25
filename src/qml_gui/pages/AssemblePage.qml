import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OWzxGL 1.0
import ".."
import "../controls"
import "../panels"

// ─────────────────────────────────────────────────────────────────────────────
// AssemblePage.qml — Phase 90 AssembleView shell + CanvasAssembleView host.
//
// Mirrors upstream `class AssembleView : public wxPanel`
// (third_party/OrcaSlicer/src/slic3r/GUI/GUI_Preview.hpp:180), the third
// GLCanvas3D host alongside view3D (Prepare) and preview (Preview).
//
// Canvas-overlay chrome (upstream GUI_Preview.cpp:789-794 enables the return
// toolbar and disables the main toolbar; GLCanvas3D.cpp:8659-8679 renders the
// overlay chain: collapse toolbar, plate-selection toolbar, return toolbar).
// The page has NO permanent top/bottom bars — all controls float over the
// full-height canvas, mirroring shotScreen/装配页.png:
//   top-left  : <> sidebar-collapse toggle + "< Return" white pill
//   top-center: green "1 PLA" plate badge (_render_imgui_select_plate_toolbar)
//   top-right : gizmo icon strip (assemble selectable set is Move/Rotate/
//               Measure/Assembly/MmSegmentation, GLGizmosManager.cpp:64-71;
//               Assembly/MmSegmentation buttons land when those gizmos ship)
//   bottom    : floating "Assembly Control" bar centered at canvas_w/2,
//               canvas_h - 10 (GLCanvas3D.cpp:9897) + "Assembly Info" box in
//               the canvas bottom-right corner (GLCanvas3D.cpp:10024).
// ─────────────────────────────────────────────────────────────────────────────

Item {
    id: root
    required property var editorVm
    property var configVm
    property string processCategory: ""

    // Phase 164 (SW-01): assemble sidebar now sources its width from the
    // backend sidebar constants (was hardcoded 392 — part of the 7-layer lock).
    readonly property int sidebarWidth: backend ? backend.sidebarWidth : 392
    // Upstream collapse toolbar toggles the settings sidebar
    // (GLCanvas3D.cpp:8671 _render_collapse_toolbar overlay). Collapsing frees
    // the canvas width; the viewport re-anchors via the sidebar width binding.
    property bool sidebarCollapsed: false

    // Light canvas-overlay surface tokens. Theme carries only dark tokens;
    // values below are sampled from shotScreen/装配页.png:
    //   overlaySurface  #FAFAFA  (top icon strip + measure panel fill, 250,250,250)
    //   pillSurface     #FAFAFC  (back-pill fill, 250,250,252)
    //   floatSurface    #E7E7E7  (bottom bar + info box, adjudicated ref sample)
    //   lightBorder     #C8C8C8  (strip border 200,200,200 / info box 204,208,209)
    //   darkText        #545859  (overlay glyphs, 84,88,89)
    //   badgeGreen      #57DF3D  (plate badge fill, 87,223,61)
    //   badgeText       #004E00  (plate badge glyphs, 0,78,0)
    //   selectFirst     #009688  (selection row 1 / teal accents, 0,150,136 family)
    //   selectSecond    #A437A4  (selection row 2, adjudicated ref sample)
    //   revertOrange    #FF6F00  (revert buttons; upstream resources/images/
    //                             revert_btn.svg stroke color)
    readonly property color overlaySurface: "#FAFAFA"
    readonly property color pillSurface: "#FAFAFC"
    readonly property color floatSurface: "#E7E7E7"
    readonly property color lightBorder: "#C8C8C8"
    readonly property color darkText: "#545859"
    readonly property color badgeGreen: "#57DF3D"
    readonly property color badgeText: "#004E00"
    // Phase 92 (ASMMEASURE-02): teal accent for the measure panel value text.
    // Points at the ref-sampled measure teal (装配页_测量.png, 0,150,136 family)
    // instead of a gray Theme token so values match the upstream measure-panel
    // teal (#4ec9b0 family) and the on-canvas overlay.
    readonly property color measureAccent: "#009688"
    readonly property color selectFirst: "#009688"
    readonly property color selectSecond: "#A437A4"
    readonly property color revertOrange: "#FF6F00"

    // Phase 138 (ASM-01): assembly-canvas transform-mode state. Tracks which
    // Move/Rotate gizmo is active when the Assembly Measure gizmo is off.
    // Mirrors PreparePage's activeGizmoDragMode (PreparePage.qml:19). The
    // gizmoMode binding below drives the viewport; the gizmo strip writes this
    // property gated on editorVm.availableGizmoMask.
    // (Scale is NOT part of the assemble selectable gizmo set
    // (GLGizmosManager.cpp:64-71), so no Scale toggle exists here.)
    property int assembleTransformMode: GLViewport.GizmoMove
    property int activeGizmoDragMode: GLViewport.GizmoMove

    // Phase 138 (ASM-01): switch the active transform gizmo if the mask permits
    // it (mirrors PreparePage.setGizmoIfAvailable at PreparePage.qml:112). No
    // business logic — pure mask-gated routing.
    function setAssembleGizmoIfAvailable(mode) {
        if (!root.editorVm)
            return false
        if ((root.editorVm.availableGizmoMask & (1 << mode)) === 0)
            return false
        root.assembleTransformMode = mode
        return true
    }

    // Filament family token for the top-center plate badge. Upstream shows the
    // active plate chip with its filament material
    // (_render_imgui_select_plate_toolbar, GLCanvas3D.cpp:8909). Qt6 reads the
    // selected filament preset (PresetServiceMock::FilamentCat = 1) and
    // extracts the material family; empty when no preset service/family is
    // available (the badge then shows the plate number only).
    function plateFilamentAbbrev() {
        const service = (typeof backend !== "undefined") ? backend.presetServiceMock : null
        if (!service)
            return ""
        const name = String(service.selectedPresetForCategory(1))
        const tokens = name.toUpperCase().split(/[^A-Z]+/)
        const families = ["PLA", "PETG", "PET", "ABS", "ASA", "TPU", "TPC",
                          "PVA", "HIPS", "PA", "PC", "PP", "PEEK", "PEI",
                          "PCTG", "PS", "PVB"]
        for (let i = 0; i < tokens.length; ++i) {
            for (let j = 0; j < families.length; ++j) {
                if (tokens[i].indexOf(families[j]) === 0)
                    return families[j]
            }
        }
        return ""
    }

    // Bottom-bar "?" tooltip content: the assemble-view shortcut list
    // (GLCanvas3D.cpp:1218-1222 m_shortcuts_assembly_view).
    function assemblyShortcutsTooltip() {
        return qsTr("鼠标左键: 选择对象") + "\n"
             + qsTr("Alt+鼠标左键: 选择零件") + "\n"
             + qsTr("1~16 数字键: 快速更改对象颜色")
    }

    // Measure-panel "?" tooltip content: the measure-gizmo shortcut list
    // (GLGizmoMeasure.cpp:455-460 m_shortcuts).
    function measureShortcutsTooltip() {
        return qsTr("鼠标左键: 选择") + "\n"
             + qsTr("Shift+鼠标左键: 选择点") + "\n"
             + qsTr("Delete: 重新开始选择") + "\n"
             + qsTr("Esc: 退出前取消上一个选择")
    }

    // Clipboard helper (same idiom as dialogs/SysInfoDialog.qml:138-145):
    // QML has no direct clipboard API; a hidden TextInput's selection copy is
    // the standard Quick idiom. Mirrors the upstream per-row copy button
    // (GLGizmoMeasure.cpp:1938-1942 ClipboardBtn -> wxTheClipboard).
    function copyToClipboard(text) {
        const temp = Qt.createQmlObject(
            'import QtQuick; TextInput { visible: false }', root, "clipboardHelper")
        temp.text = text
        temp.selectAll()
        temp.copy()
        temp.destroy()
    }

    // Phase 92 (ASMMEASURE-01): Ctrl+Y toggles the Assembly measurement gizmo,
    // mirroring upstream GLGizmoAssembly (WXK_CONTROL_Y,
    // GLGizmoAssembly.cpp:45-51). Activability is enforced in the viewmodel
    // (AssembleView + explosion ratio ~= 1.0 + >=2 volumes selected), so this
    // shortcut is a pure routing call — no business logic in QML
    // (AGENTS.md qml-boundaries rule). The same key is redo on Prepare
    // (PreparePage.qml), but AssemblePage owns it within its focus scope.
    Shortcut {
        sequences: ["Ctrl+Y"]
        enabled: root.editorVm !== null && root.editorVm !== undefined
        onActivated: {
            if (!root.editorVm)
                return
            if (root.editorVm.assemblyMeasureGizmoActive)
                root.editorVm.deactivateAssemblyMeasureGizmo()
            else
                root.editorVm.activateAssemblyMeasureGizmo()
        }
    }

    // ── Region 1: left settings sidebar (reused from Prepare) ──
    // Phase 90 reuses LeftSidebar as-is (90-CONTEXT.md decision 6); the shared
    // model data renders the same settings. The canvas now spans the full page
    // height (upstream AssembleView canvas fills its wxSplitter pane; the
    // collapse toolbar hides the sidebar entirely), so the sidebar stretches
    // top-to-bottom and collapses to zero width via the <> toggle.
    LeftSidebar {
        id: leftSidebar
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: root.sidebarCollapsed ? 0 : root.sidebarWidth
        visible: !root.sidebarCollapsed
        clip: true
        editorVm: root.editorVm
        configVm: root.configVm
        processCategory: root.processCategory
    }

    // ── Region 3: central CanvasAssembleView host ──
    // Mirrors PreparePage's GLViewport plate/object bindings so the shared
    // model renders. The canvas occupies the full page height: upstream
    // AssembleView draws no page-level top bar and the Assembly Control bar
    // floats OVER the canvas (GLCanvas3D.cpp:9897). ASMROUTE-01 selection
    // routing surface: onObjectPickedSource forwards to editorVm.
    GLViewport {
        id: assembleViewport
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: leftSidebar.right
        anchors.right: parent.right
        canvasType: GLViewport.CanvasAssembleView
        // Upstream CanvasAssembleView does not render the bed or the world
        // axes (GLCanvas3D.cpp:2100-2108: both _render_bed and the axes
        // render are commented out on the assemble branch; the ref canvas is
        // a plain background). showBed gates every bed surface AND renderAxes
        // in the renderer (RhiViewportRenderer.cpp:922-977, :2248), so a
        // static false here is the whole upstream alignment.
        showBed: false
        // Phase 91 (ASMEXPLODE-01): bind the explosion ratio so the renderer
        // re-applies the per-volume offset on every change. Re-render loop:
        // slider writes editorVm.explosionRatio -> stateChanged -> this binding
        // re-evaluates -> RhiViewport::setExplosionRatio -> update() ->
        // synchronize()+render() re-upload with the new offset.
        explosionRatio: root.editorVm ? root.editorVm.explosionRatio : 1.0
        // Phase 92 (ASMMEASURE-01/02): gizmoMode + the two selected source
        // indices close the overlay re-render loop. When the Assembly measure
        // gizmo is active, drive the viewport's gizmoMode to
        // GizmoAssemblyMeasure (19) so the renderer draws the overlay; the two
        // selection bindings forward the first-two selected source indices.
        // Re-render loop: Ctrl+Y -> viewmodel activates -> stateChanged ->
        // these bindings re-evaluate -> RhiViewport setters -> update() ->
        // synchronize()+render() re-upload + draw the overlay.
        gizmoMode: root.editorVm && root.editorVm.assemblyMeasureGizmoActive
                   ? GLViewport.GizmoAssemblyMeasure : root.assembleTransformMode
        assemblyMeasureSelectedA: {
            if (!root.editorVm || !root.editorVm.assemblyMeasureGizmoActive)
                return -1
            const idx = root.editorVm.assemblyMeasureSelectedSourceIndices()
            return (idx && idx.length > 0) ? idx[0] : -1
        }
        assemblyMeasureSelectedB: {
            if (!root.editorVm || !root.editorVm.assemblyMeasureGizmoActive)
                return -1
            const idx = root.editorVm.assemblyMeasureSelectedSourceIndices()
            return (idx && idx.length > 1) ? idx[1] : -1
        }
        meshData: root.editorVm ? root.editorVm.meshData : null
        bedWidth: root.editorVm ? root.editorVm.bedWidth : 220
        bedDepth: root.editorVm ? root.editorVm.bedDepth : 220
        bedOriginX: root.editorVm ? root.editorVm.bedOriginX : 0
        bedOriginY: root.editorVm ? root.editorVm.bedOriginY : 0
        bedShapeType: root.editorVm ? root.editorVm.bedShapeType : 0
        bedDiameter: root.editorVm ? root.editorVm.bedDiameter : 220
        currentPlateIndex: root.editorVm ? root.editorVm.currentPlateIndex : 0
        plateCount: root.editorVm ? root.editorVm.plateCount : 0
        activePlateObjectIndices: root.editorVm ? root.editorVm.activePlateObjectIndices : []
        meshBatchSourceObjectIndices: root.editorVm ? root.editorVm.meshBatchSourceObjectIndices : []
        // Phase 138 (ASM-01): bind the per-source-object assemble offset list so
        // the renderer applies the per-object translation on the CanvasAssembleView
        // path (Plan 138-03 task 2). Re-evaluates on stateChanged after every
        // gizmo drag write (Plan 138-02 routing).
        assembleOffsets: root.editorVm ? root.editorVm.assembleOffsets : []
        // Phase 141 (DEBT-04): bind parallel rotation/scale lists so the renderer
        // composes the full transform (translate * rotate * scale) — Rotate/Scale
        // gizmo drags now reflect in the live render (v4.8 tech debt closure).
        assembleRotations: root.editorVm ? root.editorVm.assembleRotations : []
        assembleScales: root.editorVm ? root.editorVm.assembleScales : []
        selectedSourceObjectIndex: root.editorVm ? root.editorVm.selectedSourceObjectIndex : -1
        onObjectPickedSource: function(sourceIndex) {
            if (root.editorVm)
                root.editorVm.selectSourceObject(sourceIndex)
        }
        // Phase 138 (ASM-01): gizmo drag-signal wiring mirrored from
        // PreparePage.qml (the onGizmoDragBegin/onGizmo*Requested/onGizmoDragEnd
        // block). The ViewModel slots (Plan 138-02) route to the assemble
        // transform when this canvas is active. activeGizmoDragMode records which
        // gizmo began the drag so end*Drag dispatches correctly.
        onGizmoDragBegin: {
            if (root.editorVm) {
                root.activeGizmoDragMode = assembleViewport.gizmoMode
                if (root.activeGizmoDragMode === GLViewport.GizmoRotate)
                    root.editorVm.beginGizmoRotateDrag()
                else if (root.activeGizmoDragMode === GLViewport.GizmoScale)
                    root.editorVm.beginGizmoScaleDrag()
                else
                    root.editorVm.beginGizmoMoveDrag()
            }
        }
        onGizmoMoveRequested: function(worldDelta) {
            if (root.editorVm)
                root.editorVm.applyGizmoMoveDelta(worldDelta.x, worldDelta.y, worldDelta.z)
        }
        onGizmoRotateRequested: function(axis, radians) {
            if (root.editorVm)
                root.editorVm.applyGizmoRotateDelta(axis, radians)
        }
        onGizmoScaleRequested: function(axis, factor) {
            if (root.editorVm)
                root.editorVm.applyGizmoScaleFactor(axis, factor)
        }
        onGizmoDragEnd: {
            if (root.editorVm) {
                if (root.activeGizmoDragMode === GLViewport.GizmoRotate)
                    root.editorVm.endGizmoRotateDrag()
                else if (root.activeGizmoDragMode === GLViewport.GizmoScale)
                    root.editorVm.endGizmoScaleDrag()
                else
                    root.editorVm.endGizmoMoveDrag()
            }
        }
    }

    // ── Canvas overlay: sidebar-collapse toggle (canvas top-left) ──
    // Upstream _render_collapse_toolbar (GLCanvas3D.cpp:8671) draws the <>
    // chevron pair at the canvas top-left; clicking it collapses/expands the
    // settings sidebar.
    AbstractButton {
        id: collapseToggle
        anchors.top: assembleViewport.top
        anchors.topMargin: 6
        anchors.left: assembleViewport.left
        anchors.leftMargin: 8
        width: 28
        height: 22
        hoverEnabled: true
        opacity: hovered ? 1.0 : 0.75
        ToolTip.visible: hovered
        ToolTip.text: sidebarCollapsed ? qsTr("展开侧栏") : qsTr("收起侧栏")
        ToolTip.delay: 400
        contentItem: Text {
            text: root.sidebarCollapsed ? "\u276F\u276E" : "\u276E\u276F"
            color: root.darkText
            font.pixelSize: Theme.fontSizeMD
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 4
            color: collapseToggle.hovered ? root.lightBorder : "transparent"
        }
        onClicked: root.sidebarCollapsed = !root.sidebarCollapsed
    }

    // ── Canvas overlay: "< 返回" white pill (upstream return toolbar) ──
    // GUI_Preview.cpp:789-794 enable_return_toolbar(true): the assemble canvas
    // hosts a return affordance instead of a main toolbar; activating it goes
    // back to the Prepare (View3D) canvas.
    AbstractButton {
        id: backPill
        anchors.top: assembleViewport.top
        anchors.topMargin: 18
        anchors.left: collapseToggle.right
        anchors.leftMargin: 16
        implicitWidth: pillLabel.implicitWidth + 28
        implicitHeight: 28
        hoverEnabled: true
        opacity: hovered ? 1.0 : 0.92
        contentItem: Text {
            id: pillLabel
            text: "\u2039 " + qsTr("返回")
            color: root.darkText
            font.pixelSize: Theme.fontSizeMD
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: height / 2
            color: root.pillSurface
            border.width: 1
            border.color: backPill.hovered ? root.lightBorder : "transparent"
        }
        onClicked: if (typeof backend !== "undefined" && backend)
                       backend.requestChangeViewMode(backend.vmView3D)
    }

    // ── Canvas overlay: top-center green plate badge ──
    // _render_imgui_select_plate_toolbar (GLCanvas3D.cpp:8909): the selected
    // plate chip shows the plate number over the filament material. Data comes
    // from the existing editorVm plate index (display is 1-based,
    // PreparePage.qml:1691) + the selected filament preset family.
    Rectangle {
        id: plateBadge
        anchors.top: assembleViewport.top
        anchors.topMargin: 8
        anchors.horizontalCenter: assembleViewport.horizontalCenter
        width: 56
        height: 48
        radius: 4
        color: root.badgeGreen
        // Re-evaluates on stateChanged (objectCount carries the NOTIFY), so a
        // preset switch that changes the family refreshes the badge text.
        readonly property string filamentText: {
            const _oc = root.editorVm ? root.editorVm.objectCount : -1
            return root.plateFilamentAbbrev()
        }
        Column {
            anchors.centerIn: parent
            spacing: 0
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: String(Math.max(0, root.editorVm ? root.editorVm.currentPlateIndex : 0) + 1)
                color: root.badgeText
                font.pixelSize: Theme.fontSizeLG
                font.bold: true
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: plateBadge.filamentText
                visible: plateBadge.filamentText.length > 0
                color: root.badgeText
                font.pixelSize: Theme.fontSizeSM
            }
        }
    }

    // ── Canvas overlay: top gizmo icon strip ──
    // Upstream draws the selectable gizmos top-centered on the assemble canvas
    // (GLGizmosManager.cpp:1227-1244); the assemble selectable set is exactly
    // Move/Rotate/Measure/Assembly/MmSegmentation (GLGizmosManager.cpp:64-71).
    // Move/Rotate/Measure are wired below; the Assembly + MmSegmentation
    // buttons land when those gizmos ship.
    Rectangle {
        id: gizmoStrip
        anchors.top: assembleViewport.top
        anchors.topMargin: 8
        anchors.left: plateBadge.right
        anchors.leftMargin: 24
        width: stripRow.implicitWidth + 20
        height: 48
        radius: 4
        color: root.overlaySurface
        border.width: 1
        border.color: root.lightBorder

        Row {
            id: stripRow
            anchors.centerIn: parent
            spacing: 4

            CxIconButton {
                cxStyle: CxIconButton.Style.Ghost
                buttonSize: 36
                iconSize: 24
                iconSource: "qrc:/qml/assets/icons/toolbar_move.svg"
                toolTipText: qsTr("移动")
                selected: !(root.editorVm && root.editorVm.assemblyMeasureGizmoActive)
                          && root.assembleTransformMode === GLViewport.GizmoMove
                enabled: root.editorVm
                         && ((root.editorVm.availableGizmoMask & (1 << GLViewport.GizmoMove)) !== 0)
                onClicked: root.setAssembleGizmoIfAvailable(GLViewport.GizmoMove)
            }
            CxIconButton {
                cxStyle: CxIconButton.Style.Ghost
                buttonSize: 36
                iconSize: 24
                iconSource: "qrc:/qml/assets/icons/toolbar_rotate.svg"
                toolTipText: qsTr("旋转")
                selected: !(root.editorVm && root.editorVm.assemblyMeasureGizmoActive)
                          && root.assembleTransformMode === GLViewport.GizmoRotate
                enabled: root.editorVm
                         && ((root.editorVm.availableGizmoMask & (1 << GLViewport.GizmoRotate)) !== 0)
                onClicked: root.setAssembleGizmoIfAvailable(GLViewport.GizmoRotate)
            }
            CxIconButton {
                cxStyle: CxIconButton.Style.Ghost
                buttonSize: 36
                iconSize: 24
                iconSource: "qrc:/qml/assets/icons/toolbar_measure.svg"
                toolTipText: qsTr("测量")
                selected: root.editorVm && root.editorVm.assemblyMeasureGizmoActive
                enabled: root.editorVm !== null && root.editorVm !== undefined
                onClicked: {
                    if (!root.editorVm)
                        return
                    if (root.editorVm.assemblyMeasureGizmoActive)
                        root.editorVm.deactivateAssemblyMeasureGizmo()
                    else
                        root.editorVm.activateAssemblyMeasureGizmo()
                }
            }
        }
    }

    // ── Phase 92 (ASMMEASURE-02): right-side 测量 (Measurement) panel ──
    // Mirrors upstream GLGizmoAssembly::on_render_input_window and the
    // shotScreen/装配页_测量.png right-side panel: header, mode label, two
    // colored per-face selection rows with circular revert buttons
    // (GLGizmoMeasure.cpp:1856-1911), the FACE_FACE selection tip, distance +
    // angle value rows each with a copy-to-clipboard button
    // (GLGizmoMeasure.cpp:1931-1943), and a bottom row with the "?" shortcut
    // help button + right-aligned Done primary button (GLGizmoAssembly.cpp:
    // 116-122). Presentation-only: reads editorVm properties and calls the
    // Q_INVOKABLE activator; all measurement math lives in C++
    // (AssemblyMeasureGeometry + EditorViewModel).
    Rectangle {
        id: measurePanel
        anchors.top: assembleViewport.top
        anchors.bottom: assembleViewport.bottom
        anchors.right: assembleViewport.right
        anchors.rightMargin: Theme.spacingMD
        anchors.topMargin: 64
        anchors.bottomMargin: 64
        width: 220
        visible: root.editorVm && root.editorVm.assemblyMeasureGizmoActive
        color: root.overlaySurface
        border.color: root.lightBorder
        border.width: 1
        radius: Theme.radiusMD
        z: 10

        // Selection count from the shared source-object selection (NOTIFY
        // stateChanged). The per-triangle plane model is future; each selected
        // volume stands in for one selected plane (EditorViewModel.cpp:3987).
        readonly property int planeSelectionCount: {
            if (!root.editorVm || !root.editorVm.assemblyMeasureGizmoActive)
                return 0
            return root.editorVm.selectedSourceObjectIndices.length
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingMD
            spacing: Theme.spacingSM

            Label {
                text: qsTr("测量")
                color: root.darkText
                font.pixelSize: Theme.fontSizeLG
                font.bold: true
            }
            Label {
                // ONLY_ASSEMBLY measure mode (GLGizmoAssembly.cpp:25-29).
                text: qsTr("装配测量")
                color: root.darkText
                font.pixelSize: Theme.fontSizeMD
            }
            Rectangle { height: 1; Layout.fillWidth: true; color: root.lightBorder }

            // FACE_FACE selection tip (GLGizmoMeasure.cpp:1858-1862; the
            // upstream tip breaks over two lines).
            Label {
                text: qsTr("在物体上选择 2 个平面，\n使物体相互装配。")
                color: root.darkText
                font.pixelSize: Theme.fontSizeSM
                visible: measurePanel.planeSelectionCount < 2
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            // Selection row 1 (GLGizmoMeasure.cpp:1864-1885): colored caption +
            // circular revert button once the feature exists (:1878).
            RowLayout {
                spacing: Theme.spacingSM
                visible: measurePanel.planeSelectionCount >= 1
                Layout.fillWidth: true
                Label {
                    text: qsTr("选中 1 平面")
                    color: root.selectFirst
                    font.pixelSize: Theme.fontSizeMD
                    Layout.fillWidth: true
                }
                AbstractButton {
                    id: revertFirst
                    implicitWidth: 24
                    implicitHeight: 24
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("重选")
                    ToolTip.delay: 400
                    contentItem: Text {
                        text: "\u21BA"
                        color: root.revertOrange
                        font.pixelSize: Theme.fontSizeLG
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: width / 2
                        color: revertFirst.hovered ? root.lightBorder : "transparent"
                        border.width: 1
                        border.color: revertFirst.hovered ? root.revertOrange : root.lightBorder
                    }
                    onClicked: {
                        // Upstream reset_feature1 (GLGizmoMeasure.cpp:1882).
                        // Qt6 keeps one shared source selection, so the reset
                        // clears it (selectSourceObject(-1) clears + emits).
                        if (root.editorVm)
                            root.editorVm.selectSourceObject(-1)
                    }
                }
            }

            // Selection row 2 (GLGizmoMeasure.cpp:1890-1910): revert button
            // only when BOTH features exist (:1903).
            RowLayout {
                spacing: Theme.spacingSM
                visible: measurePanel.planeSelectionCount >= 2
                Layout.fillWidth: true
                Label {
                    text: qsTr("选中 2 平面")
                    color: root.selectSecond
                    font.pixelSize: Theme.fontSizeMD
                    Layout.fillWidth: true
                }
                AbstractButton {
                    id: revertSecond
                    implicitWidth: 24
                    implicitHeight: 24
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("重选")
                    ToolTip.delay: 400
                    contentItem: Text {
                        text: "\u21BA"
                        color: root.revertOrange
                        font.pixelSize: Theme.fontSizeLG
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: width / 2
                        color: revertSecond.hovered ? root.lightBorder : "transparent"
                        border.width: 1
                        border.color: revertSecond.hovered ? root.revertOrange : root.lightBorder
                    }
                    onClicked: {
                        // Upstream reset_feature2 (GLGizmoMeasure.cpp:1908);
                        // see the row-1 note on the shared-selection reset.
                        if (root.editorVm)
                            root.editorVm.selectSourceObject(-1)
                    }
                }
            }

            Rectangle { height: 1; Layout.fillWidth: true; color: root.lightBorder }

            // Distance row (mm, formatted to upstream 3-decimal precision) with
            // the trailing copy button (GLGizmoMeasure.cpp:1931-1943: the row
            // copies "<label>: <value>" to the clipboard).
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSM
                Label {
                    text: qsTr("距离")
                    color: root.darkText
                    font.pixelSize: Theme.fontSizeMD
                    Layout.preferredWidth: 40
                }
                Label {
                    text: root.editorVm ? root.editorVm.assemblyMeasureDistanceText : ""
                    color: root.measureAccent
                    font.pixelSize: Theme.fontSizeMD
                    font.family: "Consolas, monospace"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                AbstractButton {
                    id: copyDistance
                    implicitWidth: 22
                    implicitHeight: 22
                    hoverEnabled: true
                    enabled: root.editorVm
                             && root.editorVm.assemblyMeasureDistanceText.length > 0
                    ToolTip.visible: hovered && enabled
                    ToolTip.text: qsTr("复制到剪贴板")
                    ToolTip.delay: 400
                    contentItem: Image {
                        source: "qrc:/qml/assets/icons/copy.svg"
                        sourceSize: Qt.size(14, 14)
                        fillMode: Image.PreserveAspectFit
                        opacity: copyDistance.enabled ? 1.0 : 0.35
                    }
                    onClicked: {
                        if (root.editorVm)
                            root.copyToClipboard(qsTr("距离") + ": "
                                                 + root.editorVm.assemblyMeasureDistanceText)
                    }
                }
            }

            // Angle row (degrees + glyph, formatted to upstream 3-decimal
            // precision) with the trailing copy button.
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSM
                Label {
                    text: qsTr("角度")
                    color: root.darkText
                    font.pixelSize: Theme.fontSizeMD
                    Layout.preferredWidth: 40
                }
                Label {
                    text: root.editorVm ? root.editorVm.assemblyMeasureAngleText : ""
                    color: root.measureAccent
                    font.pixelSize: Theme.fontSizeMD
                    font.family: "Consolas, monospace"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                AbstractButton {
                    id: copyAngle
                    implicitWidth: 22
                    implicitHeight: 22
                    hoverEnabled: true
                    enabled: root.editorVm
                             && root.editorVm.assemblyMeasureAngleText.length > 0
                    ToolTip.visible: hovered && enabled
                    ToolTip.text: qsTr("复制到剪贴板")
                    ToolTip.delay: 400
                    contentItem: Image {
                        source: "qrc:/qml/assets/icons/copy.svg"
                        sourceSize: Qt.size(14, 14)
                        fillMode: Image.PreserveAspectFit
                        opacity: copyAngle.enabled ? 1.0 : 0.35
                    }
                    onClicked: {
                        if (root.editorVm)
                            root.copyToClipboard(qsTr("角度") + ": "
                                                 + root.editorVm.assemblyMeasureAngleText)
                    }
                }
            }

            Item { Layout.fillHeight: true }  // spacer

            // Bottom row: "?" shortcut-help button left + right-aligned Done
            // primary button (GLGizmoAssembly.cpp:116-122 render_tooltip_button
            // + begin_right_aligned_buttons(Done); Done resets all gizmos).
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSM
                AbstractButton {
                    id: panelHelpButton
                    implicitWidth: 24
                    implicitHeight: 24
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: root.measureShortcutsTooltip()
                    ToolTip.delay: 400
                    contentItem: Text {
                        text: "?"
                        color: "#FFFFFF"
                        font.pixelSize: Theme.fontSizeMD
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: width / 2
                        color: root.selectFirst
                        opacity: panelHelpButton.hovered ? 1.0 : 0.9
                    }
                }
                Item { Layout.fillWidth: true }
                CxButton {
                    cxStyle: CxButton.Style.Primary
                    compact: true
                    text: qsTr("完成")
                    onClicked: if (root.editorVm) root.editorVm.deactivateAssemblyMeasureGizmo()
                }
            }
        }
    }

    // Phase 92 (ASMMEASURE-01): activability hint when the gizmo is NOT active
    // but the user is on AssembleView. Mirrors upstream GLGizmoAssembly::
    // on_get_name hint ("Please confirm explosion ratio = 1 and select at
    // least two volumes."). Shown floating over the canvas above the
    // Assembly Control bar.
    Label {
        anchors.bottom: bottomBar.top
        anchors.horizontalCenter: assembleViewport.horizontalCenter
        anchors.bottomMargin: Theme.spacingMD
        padding: Theme.spacingSM
        visible: root.editorVm && !root.editorVm.assemblyMeasureGizmoActive
                 && root.editorVm.explosionRatio > 1.01
        text: qsTr("请将爆炸比例重置为 1.00 后再使用测量")
        color: root.darkText
        font.pixelSize: Theme.fontSizeMD
        background: Rectangle { color: root.floatSurface; radius: Theme.radiusSM; border.color: root.lightBorder; border.width: 1 }
        z: 10
    }

    // ── Region 4: floating Assembly Control bar (canvas bottom-center) ──
    // _render_assemble_control positions the AlwaysAutoResize title-less
    // window centered at (canvas_w/2, canvas_h - 10*scale)
    // (GLCanvas3D.cpp:9897-9898) — the bar floats OVER the full-height canvas
    // instead of reserving page height. The bar opens with the 25px "?" help
    // button + accent "|" separator (_render_assembly_tooltip_button,
    // GLCanvas3D.cpp:9839-9865, called first at :9900-9904), then the 爆炸比例
    // group. Upstream slider + DragFloat range is 1.0-3.0 (:9943/:9948); the
    // reset affordance calls editorVm.resetExplosionRatio() (upstream
    // reset_explosion_ratio, GLCanvas3D.hpp:770-771). The slider only writes
    // the property; all rendering/separation logic lives in the viewmodel +
    // renderer (QML-boundaries rule).
    Rectangle {
        id: bottomBar
        anchors.bottom: assembleViewport.bottom
        anchors.bottomMargin: 10
        anchors.horizontalCenter: assembleViewport.horizontalCenter
        width: controlRow.implicitWidth + 24
        height: 38
        radius: 8
        color: root.floatSurface
        border.width: 1
        border.color: root.lightBorder
        z: 10

        RowLayout {
            id: controlRow
            anchors.centerIn: parent
            spacing: Theme.spacingMD

            // "?" shortcut-help button (25px, GLCanvas3D.cpp:9856
            // button_size = 25*scale) listing the assemble-view shortcuts.
            AbstractButton {
                id: barHelpButton
                implicitWidth: 25
                implicitHeight: 25
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: root.assemblyShortcutsTooltip()
                ToolTip.delay: 400
                contentItem: Text {
                    text: "?"
                    color: "#FFFFFF"
                    font.pixelSize: Theme.fontSizeMD
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: width / 2
                    color: root.selectFirst
                    opacity: barHelpButton.hovered ? 1.0 : 0.9
                }
            }

            // COL_ACTIVE "|" separator after the help button
            // (GLCanvas3D.cpp:9861).
            Label {
                text: "|"
                color: Theme.accent
                font.pixelSize: Theme.fontSizeLG
            }

            Label {
                text: qsTr("爆炸比例")
                color: root.darkText
                font.pixelSize: Theme.fontSizeMD
            }
            CxSlider {
                // Upstream range 1.0-3.0 (bbl_slider_float_style,
                // GLCanvas3D.cpp:9943; DragFloat min 1.0 at :9948) — the
                // explosion-ratio semantic floor is 1.
                from: 1.0
                to: 3.0
                stepSize: 0.01
                value: root.editorVm ? root.editorVm.explosionRatio : 1.0
                implicitWidth: 160
                onMoved: if (root.editorVm) root.editorVm.explosionRatio = value
            }
            Label {
                text: root.editorVm ? root.editorVm.explosionRatio.toFixed(2) : "1.00"
                color: root.darkText
                font.pixelSize: Theme.fontSizeMD
                font.family: "Consolas, monospace"
                Layout.preferredWidth: 36
            }
            CxButton {
                cxStyle: CxButton.Style.Ghost
                compact: true
                text: qsTr("重置")
                onClicked: if (root.editorVm) root.editorVm.resetExplosionRatio()
            }
        }
    }

    // ── Region 5: Assembly Info box (canvas bottom-right) ──
    // _render_assemble_info (GLCanvas3D.cpp:10001-10042): an independent
    // window pinned to the canvas bottom-right corner (margin 10*scale,
    // :10023-10024) that is HIDDEN when the selection is empty (:10007-10009)
    // and shows two aligned rows: "Volume: %.2f" + "Size: %.2f x %.2f x %.2f"
    // (:10035-10038). Data comes from editorVm.measureDimensions
    // (dx, dy, dz, volume — EditorViewModel.h:159-160).
    Rectangle {
        id: assemblyInfoBox
        anchors.right: assembleViewport.right
        anchors.bottom: assembleViewport.bottom
        anchors.rightMargin: 10
        anchors.bottomMargin: 10
        width: infoColumn.implicitWidth + 24
        height: infoColumn.implicitHeight + 16
        radius: 8
        color: root.floatSurface
        border.width: 1
        border.color: root.lightBorder
        visible: root.editorVm && root.editorVm.selectedObjectCount > 0
                 && (root.editorVm.measureDimensions.x > 0
                     || root.editorVm.measureDimensions.y > 0
                     || root.editorVm.measureDimensions.z > 0)
        z: 10

        ColumnLayout {
            id: infoColumn
            anchors.centerIn: parent
            spacing: 4

            Label {
                text: qsTr("装配体信息")
                color: root.darkText
                font.pixelSize: Theme.fontSizeMD
                font.bold: true
            }
            RowLayout {
                spacing: Theme.spacingSM
                Label {
                    text: qsTr("体积:")
                    color: root.darkText
                    font.pixelSize: Theme.fontSizeMD
                    Layout.preferredWidth: 44
                }
                Label {
                    text: root.editorVm
                          ? root.editorVm.measureDimensions.w.toFixed(2) : ""
                    color: root.darkText
                    font.pixelSize: Theme.fontSizeMD
                    font.family: "Consolas, monospace"
                }
            }
            RowLayout {
                spacing: Theme.spacingSM
                Label {
                    text: qsTr("尺寸:")
                    color: root.darkText
                    font.pixelSize: Theme.fontSizeMD
                    Layout.preferredWidth: 44
                }
                Label {
                    text: root.editorVm
                          ? root.editorVm.measureDimensions.x.toFixed(2) + " x "
                            + root.editorVm.measureDimensions.y.toFixed(2) + " x "
                            + root.editorVm.measureDimensions.z.toFixed(2) : ""
                    color: root.darkText
                    font.pixelSize: Theme.fontSizeMD
                    font.family: "Consolas, monospace"
                }
            }
        }
    }
}
