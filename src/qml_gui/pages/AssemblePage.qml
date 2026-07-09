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
// GLCanvas3D host alongside view3D (Prepare) and preview (Preview). The 4-region
// chrome (top bar, left settings sidebar, central 3D canvas, bottom controls
// with assembly-info panel) follows shotScreen/装配页.png.
//
// Phase 90 scope: shell + canvas host + basic mesh render (proves the host
// works end-to-end). Explosion-ratio slider, Assembly measurement gizmo, and
// the data pool are deferred to Phase 91/92/93. The bottom controls row shows
// the assembly-info panel only.
// ─────────────────────────────────────────────────────────────────────────────

Item {
    id: root
    required property var editorVm
    property var configVm
    property string processCategory: ""

    // Per-page top bar height (the app-level top bar is BBLTopbar in main.qml).
    readonly property int topBarHeight: 32
    readonly property int bottomBarHeight: 44
    readonly property int sidebarWidth: 392

    // ── Region 5: per-page top bar ──
    // Minimal for Phase 90; navigation toggle lives in BBLTopbar (task 90-01-05).
    Rectangle {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.topBarHeight
        color: Theme.bgBase
        border.width: 0

        Label {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: Theme.spacingMD
            text: qsTr("装配")
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeLG
            font.bold: true
        }
    }

    // ── Region 1: left settings sidebar (reused from Prepare) ──
    // Phase 90 reuses LeftSidebar as-is (90-CONTEXT.md decision 6); the shared
    // model data renders the same settings. AssembleView-specific visibility
    // filtering can be added in Phase 91+.
    LeftSidebar {
        id: leftSidebar
        anchors.top: topBar.bottom
        anchors.bottom: bottomBar.top
        anchors.left: parent.left
        width: root.sidebarWidth
        editorVm: root.editorVm
        configVm: root.configVm
        processCategory: root.processCategory
    }

    // ── Region 3: central CanvasAssembleView host ──
    // Mirrors PreparePage's GLViewport bed/plate/object bindings so the shared
    // model renders. The mesh-render proof comes from the CanvasAssembleView
    // branch in RhiViewportRenderer (task 90-01-02). ASMROUTE-01 selection
    // routing surface: onObjectPickedSource forwards to editorVm.
    GLViewport {
        id: assembleViewport
        anchors.top: topBar.bottom
        anchors.bottom: bottomBar.top
        anchors.left: leftSidebar.right
        anchors.right: parent.right
        canvasType: GLViewport.CanvasAssembleView
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
        selectedSourceObjectIndex: root.editorVm ? root.editorVm.selectedSourceObjectIndex : -1
        onObjectPickedSource: function(sourceIndex) {
            if (root.editorVm)
                root.editorVm.selectSourceObject(sourceIndex)
        }
    }

    // ── Region 4: bottom controls row with assembly-info panel ──
    // Phase 90 shows the assembly-info panel only. The "爆炸比例" explosion
    // slider and "选择模式" dropdown are deferred to Phase 91/92.
    Rectangle {
        id: bottomBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.bottomBarHeight
        color: Theme.bgBase
        border.width: 0

        // 装配体信息 (Assembly Info) panel on the right, mirroring 装配页.png.
        // Phase 90 derives basic info from editorVm (object count + the existing
        // selected-object dimensions text); the full explosion-aware info is
        // Phase 91.
        Item {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: Theme.spacingMD
            width: assemblyInfoRow.implicitWidth + 24
            height: parent.height

            RowLayout {
                id: assemblyInfoRow
                anchors.centerIn: parent
                spacing: Theme.spacingMD

                Label {
                    text: qsTr("装配体信息")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                }
                Label {
                    text: qsTr("对象: %1").arg(root.editorVm ? root.editorVm.objectCount : 0)
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                }
                Label {
                    visible: root.editorVm && root.editorVm.selectedObjectCount > 0
                    text: root.editorVm ? root.editorVm.selectedObjectInfoText : ""
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    elide: Text.ElideRight
                    Layout.maximumWidth: 260
                }
            }
        }
    }
}
