import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OWzxGL 1.0
import ".."
import "../controls"
import "../components" as Components
import "../panels"

Item {
    id: root
    required property var previewVm
    required property var editorVm
    required property var configVm
    property string processCategory: ""
    focus: true
    // CANVAS-FOCUS: same contract as PreparePage -- the page root is the
    // focus proxy of the preview canvas; claim active focus whenever the
    // page becomes visible so the playback key table works without a prior
    // click (upstream routes these through the focused canvas).
    onVisibleChanged: if (visible) forceActiveFocus()
    Timer {
        running: root.visible
        interval: 0
        onTriggered: root.forceActiveFocus()
    }

    // Phase 237 (VIEW-01): expose the preview RhiViewport so the shell View
    // menu / Ctrl+0..6 shortcuts can route camera presets to the ACTIVE
    // canvas (upstream MainFrame::select_view targets the current canvas3D,
    // MainFrame.cpp:3455).
    property alias previewViewportRef: previewViewport

    // The analysis pane and upstream G-code window are separate surfaces.
    property bool analysisExpanded: true
    // Phase 164 (SW-01): preview left panel now sources its width from the
    // backend sidebar constants (was hardcoded 392 — part of the 7-layer lock).
    readonly property int targetPreviewLeftWidth: backend ? backend.sidebarWidth : 392
    readonly property int targetPreviewRightWidth: Theme.rightPanelWidth
    readonly property int targetPreviewLayerRailWidth: 38
    readonly property int targetPreviewMoveBarHeight: 50
    readonly property int leftPanelWidth: root.targetPreviewLeftWidth
    readonly property int rightPanelWidth: root.targetPreviewRightWidth
    readonly property bool hasPreviewData: root.previewVm && root.previewVm.previewReady

    function cameraButtonLabel(index) {
        switch (index) {
        case 0: return qsTr("顶")
        case 1: return qsTr("前")
        case 2: return qsTr("右")
        default: return qsTr("等轴")
        }
    }

    // preview-4: localized display names for the legend-header view-mode
    // combo. The upstream English display strings stay verbatim in
    // PreviewViewModel::viewModes() (locked by ViewModelSmokeTests
    // ::viewModesExposeUpstreamTenModes); only this presentation layer
    // localizes them, index-aligned with viewModes().
    function viewModeDisplayName(index) {
        switch (index) {
        case 0: return qsTr("走线类型")     // Line Type
        case 1: return qsTr("耗材")         // Filament
        case 2: return qsTr("速度")         // Speed
        case 3: return qsTr("层高")         // Layer Height
        case 4: return qsTr("线宽")         // Line Width
        case 5: return qsTr("流量")         // Flow
        case 6: return qsTr("层耗时")       // Layer Time
        case 7: return qsTr("层耗时(对数)") // Layer Time (log)
        case 8: return qsTr("风扇转速")     // Fan Speed
        case 9: return qsTr("温度")         // Temperature
        default: return ""
        }
    }
    readonly property var localizedViewModeNames: {
        const modes = root.previewVm ? root.previewVm.viewModes : []
        const names = []
        for (let i = 0; i < modes.length; ++i)
            names.push(root.viewModeDisplayName(i))
        return names
    }

    // CANVAS-FOCUS: mirror of PreparePage.handleCanvasKey -- the preview
    // playback key table (upstream preview canvas keys) lives in this
    // function, invoked from Keys.onPressed below while the page root holds
    // active focus.
    function handlePreviewKey(event) {
        if (!root.previewVm)
            return
        switch (event.key) {
        case Qt.Key_Space:
            root.previewVm.togglePlayPause()
            event.accepted = true
            break
        case Qt.Key_Left: {
            const step = event.modifiers & Qt.ControlModifier ? 100
                       : event.modifiers & Qt.ShiftModifier ? 10 : 1
            root.previewVm.stepCurrentMove(-step)
            event.accepted = true
            break
        }
        case Qt.Key_Right: {
            const step = event.modifiers & Qt.ControlModifier ? 100
                       : event.modifiers & Qt.ShiftModifier ? 10 : 1
            root.previewVm.stepCurrentMove(step)
            event.accepted = true
            break
        }
        case Qt.Key_Home:
            root.previewVm.setCurrentMove(0)
            event.accepted = true
            break
        case Qt.Key_End:
            root.previewVm.setCurrentMove(root.previewVm.moveCount)
            event.accepted = true
            break
        case Qt.Key_PageUp:
            root.previewVm.moveLayerRange(event.modifiers & Qt.ShiftModifier ? 10 : 1)
            event.accepted = true
            break
        case Qt.Key_PageDown:
            root.previewVm.moveLayerRange(event.modifiers & Qt.ShiftModifier ? -10 : -1)
            event.accepted = true
            break
        case Qt.Key_L:
            root.previewVm.setSingleLayer(!root.previewVm.singleLayer)
            event.accepted = true
            break
        case Qt.Key_C:
            // Upstream preview table (KBShortcutsDialog.cpp): "C" = on/off
            // the G-code window (show_gcode_window).
            root.previewVm.setShowGcodeWindow(!root.previewVm.showGcodeWindow)
            event.accepted = true
            break
        }
    }

    Keys.onPressed: (event) => root.handlePreviewKey(event)

    Rectangle {
        anchors.fill: parent
        color: Theme.borderStrong
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: previewHeader
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: Theme.switchTrackOff

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8

                Label {
                    text: qsTr("预览")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeLG
                    font.bold: true
                }

                CxComboBox {
                    Layout.preferredWidth: 204
                    model: root.previewVm ? root.previewVm.viewModes : []
                    currentIndex: root.previewVm ? root.previewVm.viewModeIndex : 0
                    onActivated: if (root.previewVm) root.previewVm.setViewModeIndex(currentIndex)
                }

                Rectangle {
                    id: viewModeStatusPill
                    visible: root.previewVm && !root.previewVm.currentViewModeAvailable
                    Layout.preferredWidth: 72
                    Layout.preferredHeight: 24
                    radius: 4
                    color: Theme.bgWarningSubtle
                    border.width: 1
                    border.color: Theme.statusErrorPressed

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("No data")
                        color: Theme.statusWarning
                        font.pixelSize: Theme.fontSizeSM
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    MouseArea {
                        id: viewModeStatusMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                    }

                    ToolTip.visible: viewModeStatusMouse.containsMouse
                    ToolTip.text: root.previewVm ? root.previewVm.currentViewModeStatus : ""
                    ToolTip.delay: 450
                }

                Row {
                    spacing: 4
                    Repeater {
                        model: 4
                        delegate: Rectangle {
                            id: cameraPresetButton
                            required property int index
                            readonly property int preset: index
                            width: index === 3 ? 46 : 28
                            height: 28
                            radius: 4
                            color: cameraButtonMouse.containsMouse ? Theme.bgHover : Theme.bgElevated
                            border.width: 1
                            border.color: cameraButtonMouse.containsMouse ? Theme.accentDark : Theme.borderSubtle

                            Text {
                                anchors.centerIn: parent
                                text: root.cameraButtonLabel(cameraPresetButton.index)
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeSM
                                elide: Text.ElideRight
                            }

                            MouseArea {
                                id: cameraButtonMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: previewViewport.requestViewPreset(cameraPresetButton.preset)
                            }
                        }
                    }

                    Rectangle {
                        id: previewFitButton
                        width: 34
                        height: 28
                        radius: 4
                        color: previewFitMouse.containsMouse ? Theme.bgHover : Theme.bgElevated
                        border.width: 1
                        border.color: previewFitMouse.containsMouse ? Theme.accentDark : Theme.borderSubtle

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Fit")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeSM
                            font.bold: true
                        }

                        MouseArea {
                            id: previewFitMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: previewViewport.requestPreviewFit()
                        }

                        ToolTip.visible: previewFitMouse.containsMouse
                        ToolTip.text: qsTr("Fit preview")
                        ToolTip.delay: 450
                    }
                }

                Item { Layout.fillWidth: true }

                HeaderMetric {
                    label: qsTr("时间")
                    value: root.previewVm ? (root.previewVm.slicing ? root.previewVm.progress + "%" : root.previewVm.totalTime) : "--"
                }

                HeaderMetric {
                    label: qsTr("层")
                    value: root.previewVm ? root.previewVm.currentLayerLabel : "-- / --"
                }

                HeaderMetric {
                    label: qsTr("移动")
                    value: root.previewVm ? root.previewVm.currentMoveLabel : "-- / --"
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                id: leftPanel
                Layout.preferredWidth: root.leftPanelWidth
                Layout.fillHeight: true
                color: Theme.bgCard
                border.width: 1
                border.color: Theme.borderDefault
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        LeftSidebar {
                            anchors.fill: parent
                            editorVm: root.editorVm
                            configVm: root.configVm
                            processCategory: root.processCategory
                        }
                    }
                }
            }

            Item {
                id: centerArea
                Layout.fillWidth: true
                Layout.fillHeight: true

                GLViewport {
                    id: previewViewport
                    anchors.fill: parent
                    canvasType: GLViewport.CanvasPreview
                    previewData: root.previewVm.gcodePreviewData
                    // v5.16 (NAVIGATOR): show_3d_navigator (default on).
                    navigatorEnabled: typeof backend !== "undefined" && backend.settingsViewModel ? backend.settingsViewModel.show3DNavigator : true
                    // Phase 238 (PREV-01): ghost shells -- the preview canvas
                    // receives the SAME object mesh stream the Prepare canvas
                    // uses (upstream always loads shells at preview,
                    // GCodeViewer.cpp:3076 load_shells + :983-988). The
                    // renderer draws them semi-transparent behind the
                    // toolpaths (render_shells :4023).
                    meshData: root.editorVm ? root.editorVm.meshData : null
                    meshBatchSourceObjectIndices: root.editorVm ? root.editorVm.meshBatchSourceObjectIndices : []
                    meshBatchVolumeIndices: root.editorVm ? root.editorVm.meshBatchVolumeIndices : []
                    meshBatchInstanceIndices: root.editorVm ? root.editorVm.meshBatchInstanceIndices : []
                    // P15.1/15.2 (COLOR): per-batch render channels for the upstream
                    // model coloring (filament color / translucent modifiers / printable).
                    meshBatchVolumeTypes: root.editorVm ? root.editorVm.meshBatchVolumeTypes : []
                    meshBatchExtruderIds: root.editorVm ? root.editorVm.meshBatchExtruderIds : []
                    meshBatchPrintableFlags: root.editorVm ? root.editorVm.meshBatchPrintableFlags : []
                    activePlateObjectIndices: root.editorVm ? root.editorVm.activePlateObjectIndices : []
                    currentPlateIndex: root.editorVm ? root.editorVm.currentPlateIndex : 0
                    plateCount: root.editorVm ? root.editorVm.plateCount : 1
                    // v5.15 (BEDTEX): preview renders the printer bed
                    // (geometry + texture) behind the toolpath, matching
                    // upstream GCodeViewer -> _render_bed.
                    bedWidth: root.editorVm ? root.editorVm.bedWidth : 220
                    bedDepth: root.editorVm ? root.editorVm.bedDepth : 220
                    bedOriginX: root.editorVm ? root.editorVm.bedOriginX : 0
                    bedOriginY: root.editorVm ? root.editorVm.bedOriginY : 0
                    bedShapeType: root.editorVm ? root.editorVm.bedShapeType : 0
                    bedDiameter: root.editorVm ? root.editorVm.bedDiameter : 220
                    bedTextureUrl: root.editorVm ? root.editorVm.bedTextureUrl : ""
                    // v5.16 (BEDMODEL/BEDTYPE-TEX)
                    bedModelMeshData: root.editorVm ? root.editorVm.bedModelMeshData : ""
                    bedTypeTexturesActive: root.editorVm ? root.editorVm.bedTypeTexturesActive : false
                    bedCaliLinesActive: root.editorVm ? root.editorVm.bedCaliLinesActive : false
                    bedTypeImagesDir: root.editorVm ? root.editorVm.bedTypeImagesDir : ""
                    currentPlateBedType: root.editorVm ? root.editorVm.currentPlateBedType : 0
                    layerMin: root.previewVm.currentLayerMin
                    layerMax: root.previewVm.currentLayerMax
                    moveEnd: root.previewVm.currentMove
                    showTravelMoves: root.previewVm.showTravelMoves
                    roleVisibility: root.previewVm.roleVisibilityMask
                    showBed: root.previewVm.showBed
                    // Phase 238 (PREV-02): the marker renders only when a real
                    // tool position exists (upstream Marker visibility is tied
                    // to a current sequential move).
                    showMarker: root.previewVm
                                ? (root.previewVm.showMarker && root.previewVm.hasToolPosition && root.hasPreviewData)
                                : false
                    gcodeViewMode: root.previewVm.viewModeIndex
                    markerX: root.previewVm.toolX
                    markerY: root.previewVm.toolY
                    markerZ: root.previewVm.toolZ
                }

                // v5.16 (NAVIGATOR): navigator cube label overlay
                // (upstream ImGuizmo axis/face labels).
                Components.NavigatorLabels {
                    anchors.fill: parent
                    viewport: previewViewport
                }

                // preview-7 (ref shotScreen/预览页.png): the current-plate
                // summary floats at the canvas TOP-LEFT as a ~125x125 rounded
                // square card (teal border, plate badge embedded top-left,
                // thumbnail centered) with a 26x26 "</>" G-code window toggle
                // button above it -- upstream Preview only has the canvas with
                // the plate thumbnail floating on it (GUI_Preview.cpp:285-289),
                // not a sidebar strip. The thumbnail re-reads on every
                // EditorViewModel stateChanged (invokable accessors are not
                // auto-reactive); with no thumbnail cached the plate color
                // stands in (THUMBVERIFY-01: never fabricate a mock image).
                Column {
                    id: plateCardCluster
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 8

                    property string plateThumbBase64: ""
                    property int plateIndex: root.editorVm ? root.editorVm.currentPlateIndex : 0

                    function refreshPlateThumb() {
                        if (!root.editorVm)
                            return
                        plateCardCluster.plateThumbBase64 =
                            root.editorVm.plateThumbnailBase64(plateCardCluster.plateIndex)
                    }
                    Connections {
                        target: root.editorVm
                        function onStateChanged() { plateCardCluster.refreshPlateThumb() }
                    }
                    // Qt.callLater: the initial read must not run synchronously
                    // inside component finalization (engine->load window).
                    Component.onCompleted: Qt.callLater(plateCardCluster.refreshPlateThumb)

                    // 26x26 "</>" square button above the card (ref x~398-423,
                    // y~69-92); the "</>" glyph toggles the G-code window
                    // upstream (gCodeButtonIcon -> toggle_show_gcode_window,
                    // GCodeViewer.cpp:3517-3521).
                    Rectangle {
                        id: canvasGcodeWindowButton
                        width: 26
                        height: 26
                        radius: 4
                        color: canvasGcodeWindowMouse.containsMouse ? Theme.bgHover : "#20242bd0"
                        border.width: 1
                        border.color: canvasGcodeWindowMouse.containsMouse ? Theme.accentDark : Theme.borderSubtle

                        Text {
                            anchors.centerIn: parent
                            text: "</>"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeXS
                            font.bold: true
                        }

                        MouseArea {
                            id: canvasGcodeWindowMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.previewVm) root.previewVm.setShowGcodeWindow(!root.previewVm.showGcodeWindow)
                        }
                    }

                    Rectangle {
                        id: plateSummaryCard
                        width: 125
                        height: 125
                        radius: 8
                        // Floating-card floor: dark translucent, sampled from
                        // the same float-overlay family as the empty-state
                        // pill (ref card interior over the canvas).
                        color: "#20242bd0"
                        // Teal card border, ref measured RGB(0,150,136).
                        border.width: 1.5
                        border.color: "#009688"
                        clip: true

                        Image {
                            id: plateThumbImage
                            anchors.fill: parent
                            anchors.margins: 4
                            fillMode: Image.PreserveAspectFit
                            source: plateCardCluster.plateThumbBase64.length > 0
                                ? (plateCardCluster.plateThumbBase64.indexOf("data:image/") === 0
                                   ? plateCardCluster.plateThumbBase64
                                   : "data:image/png;base64," + plateCardCluster.plateThumbBase64)
                                : ""
                            visible: plateCardCluster.plateThumbBase64.length > 0
                            asynchronous: true
                        }

                        // Thumbnail fallback: the plate color block (same
                        // accessor the Prepare plate cards use as fallback).
                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 4
                            visible: plateCardCluster.plateThumbBase64.length === 0
                            color: root.editorVm
                                ? root.editorVm.plateThumbnailColor(plateCardCluster.plateIndex)
                                : Theme.bgPanel
                            radius: 3
                        }

                        // Plate number badge embedded INSIDE the card's
                        // top-left corner (ref "1" badge).
                        Rectangle {
                            id: plateBadge
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.margins: 4
                            width: plateBadgeLabel.implicitWidth + 10
                            height: plateBadgeLabel.implicitHeight + 4
                            radius: 3
                            color: "#20242bd0"
                            Label {
                                id: plateBadgeLabel
                                anchors.centerIn: parent
                                text: root.editorVm
                                    ? qsTr("盘 %1").arg(plateCardCluster.plateIndex + 1)
                                    : qsTr("盘")
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeXS
                            }
                        }
                    }
                }

                Rectangle {
                    visible: !root.hasPreviewData
                    anchors.centerIn: parent
                    width: Math.min(parent.width - 32, emptyStateText.implicitWidth + 28)
                    height: 40
                    radius: 6
                    color: "#20242bcc"
                    border.width: 1
                    border.color: Theme.borderSubtle

                    Label {
                        id: emptyStateText
                        anchors.centerIn: parent
                        width: parent.width - 18
                        text: root.previewVm ? root.previewVm.previewStatusText : qsTr("请先切片或载入 G-code")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeMD
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                // preview-10: the ToolPosition window anchors to the canvas
                // bottom-CENTER (upstream set_next_window_pos(0.5*canvas_width,
                // canvas_height, pivot 0.5, 1.0) -- GCodeViewer.cpp:335/:721),
                // not the bottom-left corner.
                Components.ToolPositionTooltip {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 14
                    previewVm: root.previewVm
                    visible: root.previewVm ? (root.previewVm.showMarker && root.previewVm.hasToolPosition && root.hasPreviewData) : false
                }
            }

            Rectangle {
                id: rightPanel
                Layout.preferredWidth: root.rightPanelWidth
                Layout.fillHeight: true
                color: Theme.bgFloating
                border.width: 1
                border.color: Theme.borderDefault
                clip: true

                Behavior on Layout.preferredWidth { NumberAnimation { duration: 120 } }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    // preview-4: legend window header row matches the upstream
                    // first row (GCodeViewer.cpp:3511-3521 fold + "</>"
                    // toggle_show_gcode_window buttons, :3532-3557 view-type
                    // combo) -- [fold][</>][view mode], no title text.
                    SidePanelHeader {
                        expanded: root.analysisExpanded
                        onToggleRequested: root.analysisExpanded = !root.analysisExpanded
                        onGcodeWindowToggled: if (root.previewVm) root.previewVm.setShowGcodeWindow(!root.previewVm.showGcodeWindow)
                        currentViewMode: root.previewVm ? root.previewVm.viewModeIndex : 0
                        viewModeNames: root.localizedViewModeNames
                        onViewModeSelected: function(index) {
                            if (root.previewVm)
                                root.previewVm.setViewModeIndex(index)
                        }
                    }

                    ColumnLayout {
                        visible: root.analysisExpanded
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 8

                        ScrollView {
                            id: rightAnalysisStack
                            Layout.fillWidth: true
                            Layout.preferredHeight: 392
                            Layout.minimumHeight: 260
                            Layout.maximumHeight: 430
                            clip: true
                            contentWidth: availableWidth
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                            ColumnLayout {
                                width: rightAnalysisStack.availableWidth
                                spacing: 6

                                Components.StatsPanel {
                                    Layout.fillWidth: true
                                    previewVm: root.previewVm
                                }

                                Components.VisibilityFilter {
                                    Layout.fillWidth: true
                                    previewVm: root.previewVm
                                }

                                Components.Legend {
                                    Layout.fillWidth: true
                                    previewVm: root.previewVm
                                }
                            }
                        }

                        Rectangle {
                            id: gcodeSourcePanel
                            visible: root.previewVm && root.previewVm.showGcodeWindow && root.hasPreviewData
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: 150
                            // preview-6: upstream G-code window is a borderless-
                            // titled floating window with rounding 8 and 80%
                            // background alpha (GCodeViewer.cpp:923-925
                            // WindowRounding 8 + SetNextWindowBgAlpha 0.8);
                            // ref-measured window floor #262627.
                            radius: 8
                            color: "#262627CC"
                            border.width: 1
                            border.color: Theme.borderSubtle
                            clip: true

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 0

                                // preview-4: upstream G-code window is
                                // NoDecoration — no title/header row
                                // (GCodeViewer.cpp:923-925); the list starts
                                // directly under the window edge.
                                ListView {
                                    id: gcodeList
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true
                                    model: root.previewVm ? root.previewVm.gcodeLines : []

                                    delegate: Rectangle {
                                        id: gcodeRow
                                        required property var modelData
                                        width: gcodeList.width
                                        height: 19
                                        // preview-6: upstream highlights the
                                        // current line with an orange RECT
                                        // BORDER, not a row fill (GCodeViewer.cpp
                                        // :945-947 AddRect over the selected
                                        // line); the row background stays
                                        // transparent for every line.
                                        color: "transparent"
                                        border.width: gcodeRow.modelData.current ? 1 : 0
                                        border.color: "#C16737"

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 8
                                            anchors.rightMargin: 8
                                            spacing: 8

                                            Text {
                                                Layout.preferredWidth: 44
                                                text: gcodeRow.modelData.line
                                                // preview-6: every line number is
                                                // orange (LINE_NUMBER_COLOR =
                                                // COL_ORANGE_LIGHT =
                                                // ColorRGBA::ORANGE 0.923,0.504,
                                                // 0.264 -- GCodeViewer.cpp:846 +
                                                // :949-955, ImGuiWrapper.cpp:165).
                                                color: "#EB8043"
                                                horizontalAlignment: Text.AlignRight
                                                font.pixelSize: Theme.fontSizeXS
                                                font.family: Theme.fontMono
                                            }
                                            // PREVIEW-GCODE-SOURCE-TOKENS (upstream
                                            // GCodeViewer.cpp:537-540): command yellow
                                            // (0.8,0.8,0), parameters white, comment
                                            // gray (0.7). The ViewModel pre-tokenizes
                                            // each line; QML only colors the parts.
                                            Row {
                                                Layout.fillWidth: true
                                                spacing: 0
                                                clip: true

                                                Text {
                                                    id: gcodeCommandText
                                                    text: gcodeRow.modelData.command
                                                    visible: text.length > 0
                                                    color: "#CCCC00"
                                                    font.pixelSize: Theme.fontSizeXS
                                                    font.family: Theme.fontMono
                                                }
                                                Text {
                                                    text: gcodeRow.modelData.parameters
                                                          && gcodeCommandText.visible
                                                          ? " " + gcodeRow.modelData.parameters : ""
                                                    visible: text.length > 0
                                                    color: Theme.textPrimary
                                                    font.pixelSize: Theme.fontSizeXS
                                                    font.family: Theme.fontMono
                                                }
                                                Text {
                                                    text: gcodeRow.modelData.comment
                                                    visible: text.length > 0
                                                    color: Theme.textTertiary
                                                    font.pixelSize: Theme.fontSizeXS
                                                    font.family: Theme.fontMono
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                id: verticalLayerRail
                Layout.preferredWidth: root.targetPreviewLayerRailWidth
                Layout.fillHeight: true
                color: Theme.bgElevated
                border.width: 1
                border.color: Theme.borderDefault

                Components.PreviewLayerRail {
                    anchors.fill: parent
                    anchors.margins: 4
                    previewVm: root.previewVm
                }
            }
        }

        Rectangle {
            id: moveSliderBar
            Layout.fillWidth: true
            Layout.preferredHeight: root.targetPreviewMoveBarHeight
            color: Theme.switchTrackOff
            border.width: 1
            border.color: Theme.borderDefault

            Components.MoveSlider {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                anchors.topMargin: 6
                anchors.bottomMargin: 6
                previewVm: root.previewVm
            }
        }
    }

    component HeaderMetric: Rectangle {
        id: headerMetricRoot
        property string label: ""
        property string value: ""

        Layout.preferredHeight: 28
        Layout.preferredWidth: 118
        radius: 4
        color: Theme.bgCard
        border.width: 1
        border.color: Theme.borderSubtle

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 6
            Label {
                text: headerMetricRoot.label
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeXS
            }
            Label {
                Layout.fillWidth: true
                text: headerMetricRoot.value
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeSM
                font.bold: true
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignRight
            }
        }
    }

    component SidePanelHeader: RowLayout {
        id: sidePanelHeaderRoot
        property bool expanded: true
        signal toggleRequested()
        signal gcodeWindowToggled()
        property int currentViewMode: 0
        property var viewModeNames: []
        signal viewModeSelected(int index)

        Layout.fillWidth: true
        spacing: 6

        // [fold] glyph button (upstream GCodeViewer.cpp:3511-3514).
        Rectangle {
            Layout.preferredWidth: 26
            Layout.preferredHeight: 26
            radius: 4
            color: headerMouse.containsMouse ? Theme.bgHover : Theme.bgElevated
            border.width: 1
            border.color: Theme.borderSubtle

            Text {
                anchors.centerIn: parent
                text: sidePanelHeaderRoot.expanded ? "<" : ">"
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeXL
            }

            MouseArea {
                id: headerMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: sidePanelHeaderRoot.toggleRequested()
            }
        }

        // [</>] G-code window toggle (upstream GCodeViewer.cpp:3517-3521
        // gCodeButtonIcon glyph button -> toggle_show_gcode_window).
        Rectangle {
            Layout.preferredWidth: 26
            Layout.preferredHeight: 26
            radius: 4
            color: gcodeToggleMouse.containsMouse ? Theme.bgHover : Theme.bgElevated
            border.width: 1
            border.color: Theme.borderSubtle

            Text {
                anchors.centerIn: parent
                text: "</>"
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeXS
                font.bold: true
            }

            MouseArea {
                id: gcodeToggleMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: sidePanelHeaderRoot.gcodeWindowToggled()
            }
        }

        // [view mode] combo (upstream GCodeViewer.cpp:3532-3557 BBLBeginCombo
        // over the view_type_items table); row index still equals the
        // upstream view type, display names localized at the QML layer.
        CxComboBox {
            Layout.fillWidth: true
            model: sidePanelHeaderRoot.viewModeNames
            currentIndex: sidePanelHeaderRoot.currentViewMode
            onActivated: sidePanelHeaderRoot.viewModeSelected(currentIndex)
        }
    }
}
