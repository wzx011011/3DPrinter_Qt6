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
        spacing: 6

        Label {
            text: qsTr("图例")
            color: Theme.textPrimary
            font.bold: true
            font.pixelSize: Theme.fontSizeMD
        }

        Rectangle {
            Layout.fillWidth: true
            radius: 5
            color: Theme.bgCard
            border.width: 1
            border.color: Theme.borderSubtle
            implicitHeight: legendContent.implicitHeight + 16

            ColumnLayout {
                id: legendContent
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 8
                spacing: 7

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: root.legendType === 1

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 16
                        radius: 4
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: Theme.chromeBorder }
                            GradientStop { position: 0.12; color: Theme.scrollBarHoverColor }
                            GradientStop { position: 0.24; color: Theme.scrollBarHoverColor }
                            GradientStop { position: 0.36; color: Theme.accentDark }
                            GradientStop { position: 0.48; color: Theme.accentDark }
                            GradientStop { position: 0.6; color: "#7db828" }
                            GradientStop { position: 0.72; color: Theme.statusWarning }
                            GradientStop { position: 0.82; color: Theme.statusWarning }
                            GradientStop { position: 0.91; color: Theme.statusError }
                            GradientStop { position: 1.0; color: Theme.chromeDangerPressed }
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
                            font.family: Theme.fontMono
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: root.previewVm ? root.previewVm.legendGradientMaxLabel : "--"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeXS
                            font.family: Theme.fontMono
                        }
                    }
                }

                Repeater {
                    model: root.previewVm ? root.previewVm.legendItems : []
                    delegate: RowLayout {
                        id: legendRow
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 8
                        visible: root.legendType !== 1

                        Rectangle {
                            Layout.preferredWidth: 10
                            Layout.preferredHeight: 10
                            radius: 2
                            color: legendRow.modelData.color
                        }
                        Label {
                            Layout.fillWidth: true
                            text: legendRow.modelData.label + (legendRow.modelData.count > 0 ? " (" + legendRow.modelData.count + ")" : "")
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
