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

    property bool rightPanelExpanded: true
    // Phase 164 (SW-01): preview left panel now sources its width from the
    // backend sidebar constants (was hardcoded 392 — part of the 7-layer lock).
    readonly property int targetPreviewLeftWidth: backend ? backend.sidebarWidth : 392
    readonly property int targetPreviewRightWidth: Theme.rightPanelWidth
    readonly property int targetPreviewLayerRailWidth: 38
    readonly property int targetPreviewMoveBarHeight: 50
    readonly property int leftPanelWidth: root.targetPreviewLeftWidth
    readonly property int rightPanelWidth: root.rightPanelExpanded ? root.targetPreviewRightWidth : 38
    readonly property bool hasPreviewData: root.previewVm && root.previewVm.previewReady

    function cameraButtonLabel(index) {
        switch (index) {
        case 0: return qsTr("顶")
        case 1: return qsTr("前")
        case 2: return qsTr("右")
        default: return qsTr("等轴")
        }
    }

    Keys.onPressed: (event) => {
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
        }
    }

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

                LeftSidebar {
                    anchors.fill: parent
                    editorVm: root.editorVm
                    configVm: root.configVm
                    processCategory: root.processCategory
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
                    layerMin: root.previewVm.currentLayerMin
                    layerMax: root.previewVm.currentLayerMax
                    moveEnd: root.previewVm.currentMove
                    showTravelMoves: root.previewVm.showTravelMoves
                    roleVisibility: root.previewVm.roleVisibilityMask
                    showBed: root.previewVm.showBed
                    showMarker: root.previewVm.showMarker
                    gcodeViewMode: root.previewVm.viewModeIndex
                    markerX: root.previewVm.toolX
                    markerY: root.previewVm.toolY
                    markerZ: root.previewVm.toolZ
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

                Components.ToolPositionTooltip {
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    anchors.margins: 14
                    previewVm: root.previewVm
                    visible: root.previewVm ? root.previewVm.showMarker : false
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
                    anchors.margins: root.rightPanelExpanded ? 8 : 4
                    spacing: 6

                    SidePanelHeader {
                        title: qsTr("分析")
                        expanded: root.rightPanelExpanded
                        onToggleRequested: root.rightPanelExpanded = !root.rightPanelExpanded
                    }

                    ColumnLayout {
                        visible: root.rightPanelExpanded
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
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: 150
                            radius: 4
                            color: Theme.bgPanel
                            border.width: 1
                            border.color: Theme.borderSubtle
                            clip: true

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 0

                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 30
                                    Layout.leftMargin: 8
                                    Layout.rightMargin: 8

                                    Label {
                                        text: qsTr("G-code")
                                        color: Theme.textPrimary
                                        font.bold: true
                                        font.pixelSize: Theme.fontSizeSM
                                    }
                                    Item { Layout.fillWidth: true }
                                    Label {
                                        text: root.previewVm ? qsTr("行 %1 / %2").arg(root.previewVm.currentGcodeLine).arg(root.previewVm.gcodeLineCount) : qsTr("行 -- / --")
                                        color: Theme.textTertiary
                                        font.pixelSize: Theme.fontSizeXS
                                    }
                                }

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
                                        color: gcodeRow.modelData.current ? Theme.bgWarningSubtle : "transparent"

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 8
                                            anchors.rightMargin: 8
                                            spacing: 8

                                            Text {
                                                Layout.preferredWidth: 44
                                                text: gcodeRow.modelData.line
                                                color: gcodeRow.modelData.current ? Theme.statusWarning : Theme.textTertiary
                                                horizontalAlignment: Text.AlignRight
                                                font.pixelSize: Theme.fontSizeXS
                                                font.family: Theme.fontMono
                                            }
                                            Text {
                                                Layout.fillWidth: true
                                                text: gcodeRow.modelData.text
                                                color: gcodeRow.modelData.current ? Theme.statusWarning : Theme.textSecondary
                                                elide: Text.ElideRight
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
        property string title: ""
        property bool expanded: true
        signal toggleRequested()

        Layout.fillWidth: true
        spacing: 6

        Label {
            visible: sidePanelHeaderRoot.expanded
            Layout.fillWidth: true
            text: sidePanelHeaderRoot.title
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeMD
            font.bold: true
            elide: Text.ElideRight
        }

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
    }
}
