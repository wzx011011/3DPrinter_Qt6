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
