import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Item {
    id: root
    required property var previewVm

    readonly property int legendType: root.previewVm ? root.previewVm.legendType : 0
    implicitHeight: legendLayout.implicitHeight

    ColumnLayout {
        id: legendLayout
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 8

        Label {
            text: qsTr("图例")
            color: Theme.textPrimary
            font.bold: true
            font.pixelSize: Theme.fontSizeLG
        }

        Rectangle {
            Layout.fillWidth: true
            radius: 6
            color: "#24272e"
            border.width: 1
            border.color: Theme.borderSubtle
            implicitHeight: legendContent.implicitHeight + 18

            ColumnLayout {
                id: legendContent
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 9
                spacing: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: root.legendType === 1

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 16
                        radius: 4
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#0b2c7a" }
                            GradientStop { position: 0.12; color: "#154d8a" }
                            GradientStop { position: 0.24; color: "#1a6b8a" }
                            GradientStop { position: 0.36; color: "#1a8a6b" }
                            GradientStop { position: 0.48; color: "#2d9e3e" }
                            GradientStop { position: 0.6; color: "#7db828" }
                            GradientStop { position: 0.72; color: "#c9b818" }
                            GradientStop { position: 0.82; color: "#d98a14" }
                            GradientStop { position: 0.91; color: "#d85a14" }
                            GradientStop { position: 1.0; color: "#c22525" }
                        }
                        border.width: 1
                        border.color: Theme.borderSubtle
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: root.previewVm ? root.previewVm.legendGradientMinLabel : "--"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeXS
                            font.family: "Consolas"
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: root.previewVm ? root.previewVm.legendGradientMaxLabel : "--"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeXS
                            font.family: "Consolas"
                        }
                    }
                }

                Repeater {
                    model: root.previewVm ? root.previewVm.legendItems : []
                    delegate: RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        visible: root.legendType !== 1

                        Rectangle {
                            width: 10
                            height: 10
                            radius: 2
                            color: modelData.color
                        }
                        Label {
                            Layout.fillWidth: true
                            text: modelData.label + (modelData.count > 0 ? " (" + modelData.count + ")" : "")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeSM
                            elide: Text.ElideRight
                        }
                    }
                }

                Label {
                    visible: !root.previewVm || root.previewVm.legendItems.length === 0
                    text: qsTr("暂无图例数据")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeSM
                }
            }
        }
    }
}
