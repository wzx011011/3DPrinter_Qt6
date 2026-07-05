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
    readonly property int leftPanelWidth: 390
    readonly property int rightPanelWidth: rightPanelExpanded ? 268 : 36
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
                    // Bind the dense 20-bool mask (roleVisibilityMask), NOT the
                    // 18-row QVariantMap list (roleVisibilities). The renderer's
                    // synchronize expects a flat 20-bool list indexed by canonical
                    // libvgcode role; roleVisibilities is for the UI Repeater only.
                    roleVisibility: root.previewVm ? root.previewVm.roleVisibilityMask : []
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
                            Layout.fillWidth: true
                            Layout.preferredHeight: 300
                            Layout.maximumHeight: 300
                            clip: true
                            contentWidth: availableWidth

                            ColumnLayout {
                                width: parent.width
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
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 4
                            color: "#191b20"
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
                                        required property var modelData
                                        width: gcodeList.width
                                        height: 19
                                        color: modelData.current ? "#3a2515" : "transparent"

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 8
                                            anchors.rightMargin: 8
                                            spacing: 8

                                            Text {
                                                Layout.preferredWidth: 44
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
                Layout.preferredWidth: 36
                Layout.fillHeight: true
                color: "#2f323a"
                border.width: 1
                border.color: "#3a3d45"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 6

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
            Layout.preferredHeight: 48
            color: "#2f3036"
            border.width: 1
            border.color: "#3a3d45"

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
}
