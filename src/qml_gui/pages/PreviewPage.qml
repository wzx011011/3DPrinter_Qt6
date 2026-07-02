import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OWzxGL 1.0
import ".."
import "../controls"
import "../components" as Components

Item {
    id: root
    required property var previewVm
    focus: true

    property bool leftPanelExpanded: true
    property bool rightPanelExpanded: true
    readonly property int leftPanelWidth: leftPanelExpanded ? 286 : 36
    readonly property int rightPanelWidth: rightPanelExpanded ? 322 : 42
    readonly property bool hasPreviewData: root.previewVm && root.previewVm.previewReady

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
            root.previewVm.setCurrentMove(Math.max(0, root.previewVm.currentMove - step))
            event.accepted = true
            break
        }
        case Qt.Key_Right: {
            const step = event.modifiers & Qt.ControlModifier ? 100
                       : event.modifiers & Qt.ShiftModifier ? 10 : 1
            root.previewVm.setCurrentMove(Math.min(root.previewVm.moveCount, root.previewVm.currentMove + step))
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
        color: "#55575f"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: previewHeader
            Layout.fillWidth: true
            Layout.preferredHeight: 42
            color: "#2f3036"
            border.width: 0

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 10

                Label {
                    text: qsTr("预览模式")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeLG
                    font.bold: true
                }

                CxComboBox {
                    Layout.preferredWidth: 190
                    model: root.previewVm ? root.previewVm.viewModes : []
                    currentIndex: root.previewVm ? root.previewVm.viewModeIndex : 0
                    onActivated: if (root.previewVm) root.previewVm.setViewModeIndex(currentIndex)
                }

                Row {
                    spacing: 4
                    Repeater {
                        model: [
                            { label: qsTr("顶"), preset: 0 },
                            { label: qsTr("前"), preset: 1 },
                            { label: qsTr("右"), preset: 2 },
                            { label: qsTr("等轴"), preset: 3 }
                        ]
                        delegate: Rectangle {
                            required property var modelData
                            width: modelData.preset === 3 ? 44 : 28
                            height: 28
                            radius: 4
                            color: cameraButtonMouse.containsMouse ? Theme.bgHover : Theme.bgElevated
                            border.width: 1
                            border.color: cameraButtonMouse.containsMouse ? Theme.accentDark : Theme.borderSubtle

                            Text {
                                anchors.centerIn: parent
                                text: parent.modelData.label
                                color: Theme.textPrimary
                                font.pixelSize: 11
                            }

                            MouseArea {
                                id: cameraButtonMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: previewViewport.requestViewPreset(parent.modelData.preset)
                            }
                        }
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
                color: "#27292f"
                border.width: 1
                border.color: "#3a3d45"
                clip: true

                Behavior on Layout.preferredWidth { NumberAnimation { duration: 120 } }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: root.leftPanelExpanded ? 10 : 4
                    spacing: 10

                    SidePanelHeader {
                        title: qsTr("盘与层")
                        expanded: root.leftPanelExpanded
                        onToggleRequested: root.leftPanelExpanded = !root.leftPanelExpanded
                    }

                    ColumnLayout {
                        visible: root.leftPanelExpanded
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 10

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 112
                            radius: 6
                            color: "#383b43"
                            border.width: 1
                            border.color: root.hasPreviewData ? Theme.accentDark : Theme.borderSubtle

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 10

                                Rectangle {
                                    Layout.preferredWidth: 74
                                    Layout.preferredHeight: 74
                                    radius: 6
                                    color: "#4b4d56"
                                    border.width: 1
                                    border.color: Theme.borderSubtle

                                    Canvas {
                                        anchors.fill: parent
                                        anchors.margins: 12
                                        onPaint: {
                                            const ctx = getContext("2d")
                                            ctx.reset()
                                            ctx.strokeStyle = root.hasPreviewData ? Theme.accent : Theme.textTertiary
                                            ctx.lineWidth = 3
                                            ctx.beginPath()
                                            ctx.moveTo(width * 0.18, height * 0.70)
                                            ctx.lineTo(width * 0.78, height * 0.38)
                                            ctx.lineTo(width * 0.62, height * 0.20)
                                            ctx.moveTo(width * 0.36, height * 0.60)
                                            ctx.lineTo(width * 0.68, height * 0.74)
                                            ctx.stroke()
                                        }
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    Label {
                                        Layout.fillWidth: true
                                        text: root.previewVm ? root.previewVm.plateSummary : qsTr("当前盘")
                                        color: Theme.textPrimary
                                        font.bold: true
                                        font.pixelSize: Theme.fontSizeMD
                                        elide: Text.ElideRight
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        text: root.previewVm ? root.previewVm.previewStatusText : qsTr("请先切片或载入 G-code")
                                        color: root.hasPreviewData ? Theme.accentLight : Theme.textSecondary
                                        font.pixelSize: Theme.fontSizeSM
                                        elide: Text.ElideRight
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        text: root.previewVm ? root.previewVm.warningSummary : ""
                                        color: Theme.textTertiary
                                        font.pixelSize: Theme.fontSizeSM
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }

                        InfoRow { label: qsTr("层范围"); value: root.previewVm ? root.previewVm.currentLayerLabel : "--" }
                        InfoRow { label: qsTr("当前移动"); value: root.previewVm ? root.previewVm.currentMoveLabel : "--" }
                        InfoRow { label: qsTr("当前时间"); value: root.previewVm ? root.previewVm.currentTime : "0s" }
                        InfoRow { label: qsTr("总时间"); value: root.previewVm ? root.previewVm.totalTime : "--:--:--" }
                        InfoRow { label: qsTr("耗材"); value: root.previewVm ? root.previewVm.filamentUsed : "--" }
                        InfoRow { label: qsTr("重量"); value: root.previewVm ? root.previewVm.filamentWeight : "--" }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: Theme.borderSubtle
                        }

                        Components.LayerSlider {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            previewVm: root.previewVm
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
                    layerMin: root.previewVm.currentLayerMin
                    layerMax: root.previewVm.currentLayerMax
                    moveEnd: root.previewVm.currentMove
                    showTravelMoves: root.previewVm.showTravelMoves
                    roleVisibility: root.previewVm ? root.previewVm.roleVisibilities : []
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
                    width: emptyStateText.implicitWidth + 28
                    height: 40
                    radius: 6
                    color: "#20242bcc"
                    border.width: 1
                    border.color: Theme.borderSubtle

                    Label {
                        id: emptyStateText
                        anchors.centerIn: parent
                        text: root.previewVm ? root.previewVm.previewStatusText : qsTr("请先切片或载入 G-code")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeMD
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
                color: "#202126"
                border.width: 1
                border.color: "#3a3d45"
                clip: true

                Behavior on Layout.preferredWidth { NumberAnimation { duration: 120 } }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: root.rightPanelExpanded ? 10 : 4
                    spacing: 8

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
                            Layout.fillWidth: true
                            Layout.preferredHeight: Math.min(360, parent.height * 0.42)
                            clip: true
                            contentWidth: availableWidth

                            ColumnLayout {
                                width: parent.width
                                spacing: 8

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
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 6
                            color: "#191b20"
                            border.width: 1
                            border.color: Theme.borderSubtle
                            clip: true

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 0

                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 34
                                    Layout.leftMargin: 10
                                    Layout.rightMargin: 10

                                    Label {
                                        text: qsTr("G-code")
                                        color: Theme.textPrimary
                                        font.bold: true
                                        font.pixelSize: Theme.fontSizeMD
                                    }
                                    Item { Layout.fillWidth: true }
                                    Label {
                                        text: root.previewVm ? qsTr("行 %1 / %2").arg(root.previewVm.currentGcodeLine).arg(root.previewVm.gcodeLineCount) : qsTr("行 -- / --")
                                        color: Theme.textTertiary
                                        font.pixelSize: Theme.fontSizeSM
                                    }
                                }

                                ListView {
                                    id: gcodeList
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true
                                    model: root.previewVm ? root.previewVm.gcodeLines : []

                                    delegate: Rectangle {
                                        required property var modelData
                                        width: gcodeList.width
                                        height: 22
                                        color: modelData.current ? "#3a2515" : "transparent"

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 8
                                            anchors.rightMargin: 8
                                            spacing: 8

                                            Text {
                                                Layout.preferredWidth: 52
                                                text: modelData.line
                                                color: modelData.current ? "#ff9f40" : Theme.textTertiary
                                                horizontalAlignment: Text.AlignRight
                                                font.pixelSize: 10
                                                font.family: "Consolas"
                                            }
                                            Text {
                                                Layout.fillWidth: true
                                                text: modelData.text
                                                color: modelData.current ? "#ffb866" : Theme.textSecondary
                                                elide: Text.ElideRight
                                                font.pixelSize: 10
                                                font.family: "Consolas"
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
                Layout.preferredWidth: 44
                Layout.fillHeight: true
                color: "#2f323a"
                border.width: 1
                border.color: "#3a3d45"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 8

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: root.previewVm ? root.previewVm.layerCount : 0
                        color: Theme.accentLight
                        font.pixelSize: Theme.fontSizeSM
                        font.bold: true
                    }

                    Slider {
                        id: verticalLayerSlider
                        Layout.fillHeight: true
                        Layout.alignment: Qt.AlignHCenter
                        orientation: Qt.Vertical
                        from: 0
                        to: root.previewVm ? Math.max(0, root.previewVm.layerCount - 1) : 0
                        stepSize: 1
                        value: root.previewVm ? root.previewVm.currentLayerMax : 0
                        enabled: root.previewVm && root.previewVm.layerCount > 0
                        onMoved: if (root.previewVm) root.previewVm.setLayerRange(Math.round(value), Math.round(value))
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: root.previewVm ? root.previewVm.currentLayerMax + 1 : 0
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                    }
                }
            }
        }

        Rectangle {
            id: moveSliderBar
            Layout.fillWidth: true
            Layout.preferredHeight: 58
            color: "#2f3036"
            border.width: 1
            border.color: "#3a3d45"

            Components.MoveSlider {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                anchors.topMargin: 8
                anchors.bottomMargin: 8
                previewVm: root.previewVm
            }
        }
    }

    component HeaderMetric: Rectangle {
        id: headerMetricRoot
        property string label: ""
        property string value: ""

        Layout.preferredHeight: 28
        Layout.preferredWidth: 128
        radius: 4
        color: "#24272e"
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
                font.pixelSize: 16
            }

            MouseArea {
                id: headerMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: toggleRequested()
            }
        }
    }

    component InfoRow: RowLayout {
        id: infoRowRoot
        property string label: ""
        property string value: ""

        Layout.fillWidth: true
        spacing: 8

        Label {
            Layout.preferredWidth: 72
            text: infoRowRoot.label
            color: Theme.textTertiary
            font.pixelSize: Theme.fontSizeSM
            elide: Text.ElideRight
        }
        Label {
            Layout.fillWidth: true
            text: infoRowRoot.value
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeSM
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideRight
        }
    }
}
