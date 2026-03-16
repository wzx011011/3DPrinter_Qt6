import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CrealityGL 1.0
import ".."
import "../controls"
import "../components" as Components

Item {
    id: root
    required property var previewVm
    focus: true

    // Keyboard shortcuts for Preview (matching upstream GCodeViewer)
    Keys.onPressed: (event) => {
        if (!root.previewVm)
            return
        switch (event.key) {
        case Qt.Key_Space:
            root.previewVm.togglePlayPause()
            event.accepted = true
            break
        case Qt.Key_Left:
            root.previewVm.setCurrentMove(Math.max(0, root.previewVm.currentMove - 100))
            event.accepted = true
            break
        case Qt.Key_Right:
            root.previewVm.setCurrentMove(Math.min(root.previewVm.moveCount, root.previewVm.currentMove + 100))
            event.accepted = true
            break
        case Qt.Key_Home:
            root.previewVm.setCurrentMove(0)
            event.accepted = true
            break
        case Qt.Key_End:
            root.previewVm.setCurrentMove(root.previewVm.moveCount)
            event.accepted = true
            break
        // 层范围导航（对齐上游 IMSlider 鼠标滚轮行为）
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
        color: Theme.bgBase
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingMD
        anchors.margins: Theme.spacingLG

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                radius: 14
                color: Theme.bgPanel
                border.width: 1
                border.color: Theme.borderSubtle

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: Theme.spacingMD

                    Label {
                        text: qsTr("预览模式")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeLG
                        font.bold: true
                    }

                    CxComboBox {
                        Layout.preferredWidth: 190
                        model: root.previewVm.viewModes
                        currentIndex: root.previewVm.viewModeIndex
                        onActivated: root.previewVm.setViewModeIndex(currentIndex)
                    }

                    Item { Layout.fillWidth: true }

                    // Camera preset view buttons (matching upstream GCodeViewer)
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
                                width: 26
                                height: 26
                                radius: 6
                                color: viewMA.containsMouse ? "#2a3545" : "#1e2229"
                                border.color: viewMA.containsMouse ? "#3e5060" : "#2e3540"

                                Text {
                                    anchors.centerIn: parent
                                    text: parent.modelData.label
                                    color: "#9daaba"
                                    font.pixelSize: 10
                                }

                                MouseArea {
                                    id: viewMA
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: previewViewport.requestViewPreset(parent.modelData.preset)
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.preferredHeight: 28
                        Layout.preferredWidth: 118
                        radius: 10
                        color: Theme.accentSubtle
                        border.width: 1
                        border.color: Theme.accentDark

                        Label {
                            anchors.centerIn: parent
                            text: root.previewVm.slicing ? (root.previewVm.progress + "%") : root.previewVm.estimatedTime
                            color: Theme.accentLight
                            font.bold: true
                        }
                    }

                    // Layer/move summary (对齐上游 GCodeViewer header info)
                    Rectangle {
                        visible: root.previewVm && root.previewVm.layerCount > 0
                        Layout.preferredHeight: 28
                        Layout.preferredWidth: 130
                        radius: 10
                        color: "#1e2229"
                        border.width: 1
                        border.color: "#2e3540"

                        Label {
                            anchors.centerIn: parent
                            text: (root.previewVm ? root.previewVm.layerCount : 0) + " 层 · "
                                  + (root.previewVm ? root.previewVm.moveCount : 0) + " 步"
                            color: "#7a8a9a"
                            font.pixelSize: 11
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingMD

            Rectangle {
                Layout.preferredWidth: 240
                Layout.fillHeight: true
                color: Theme.bgPanel
                radius: 16
                border.width: 1
                border.color: Theme.borderSubtle
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingLG
                    Components.LayerSlider {
                        Layout.fillWidth: true
                        previewVm: root.previewVm
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.bgSurface
                radius: 18
                border.width: 1
                border.color: Theme.borderSubtle

                GLViewport {
                    id: previewViewport
                    anchors.fill: parent
                    anchors.margins: 8
                    canvasType: GLViewport.CanvasPreview
                    previewData: root.previewVm.gcodePreviewData
                    layerMin: root.previewVm.currentLayerMin
                    layerMax: root.previewVm.currentLayerMax
                    moveEnd: root.previewVm.currentMove
                    showTravelMoves: root.previewVm.showTravelMoves
                    showBed: root.previewVm.showBed
                }

                // 工具位置提示框（对齐上游 GCodeViewer::Marker::render）
                Components.ToolPositionTooltip {
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    anchors.margins: 16
                    previewVm: root.previewVm
                    visible: root.previewVm ? root.previewVm.showMarker : true
                }
            }

            Rectangle {
                Layout.preferredWidth: 280
                Layout.fillHeight: true
                color: Theme.bgPanel
                radius: 16
                border.width: 1
                border.color: Theme.borderSubtle
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingLG
                    spacing: Theme.spacingLG
                    Components.StatsPanel {
                        Layout.fillWidth: true
                        previewVm: root.previewVm
                    }
                    Components.Legend {
                        Layout.fillWidth: true
                        previewVm: root.previewVm
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.bgPanel
            radius: 16
            border.width: 1
            border.color: Theme.borderSubtle
            Components.MoveSlider {
                anchors.fill: parent
                anchors.margins: Theme.spacingLG
                previewVm: root.previewVm
            }
        }
    }
}
