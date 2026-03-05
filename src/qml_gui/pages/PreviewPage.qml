import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CrealityGL 1.0
import "../components" as Components

Item {
    id: root
    required property var previewVm

    Rectangle {
        anchors.fill: parent
        color: "#2f333a"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8
        anchors.margins: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: qsTr("预览模式")
                color: "#dfe6ef"
            }

            ComboBox {
                Layout.preferredWidth: 180
                model: root.previewVm.viewModes
                currentIndex: root.previewVm.viewModeIndex
                onActivated: root.previewVm.setViewModeIndex(currentIndex)
            }

            Item { Layout.fillWidth: true }

            Label {
                text: root.previewVm.slicing ? (root.previewVm.progress + "%") : root.previewVm.estimatedTime
                color: "#7DFA98"
                font.bold: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            Rectangle {
                Layout.preferredWidth: 240
                Layout.fillHeight: true
                color: "#3d424b"
                radius: 6
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
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
                color: "#272b31"
                radius: 6

                GLViewport {
                    id: previewViewport
                    anchors.fill: parent
                    anchors.margins: 6
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
                color: "#3d424b"
                radius: 6
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 12
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
            Layout.preferredHeight: 48
            color: "#3d424b"
            radius: 6
            Components.MoveSlider {
                anchors.fill: parent
                anchors.margins: 8
                previewVm: root.previewVm
            }
        }
    }
}
