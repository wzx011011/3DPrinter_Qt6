import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Legend — 图例组件（对齐上游 GCodeViewer legend）
// 支持三种渲染模式：
//   0 = discrete: 色块 + 标签 + 计数（FeatureType 等）
//   1 = gradient:  渐变条 + min/max 标签（Height/Feedrate/Temperature 等）
//   2 = extruder:  挤出机色块 + 标签 + 计数（Tool/ColorPrint/FilamentId）
Item {
    id: root
    required property var previewVm

    // 0=discrete, 1=gradient, 2=extruder
    readonly property int legendType: root.previewVm ? root.previewVm.legendType : 0

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingSM

        Label { text: qsTr("图例"); color: Theme.textPrimary; font.bold: true; font.pixelSize: Theme.fontSizeLG }

        Rectangle {
            Layout.fillWidth: true
            radius: 12
            color: Theme.bgElevated
            border.width: 1
            border.color: Theme.borderSubtle

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                // ── Gradient legend (对齐上游 Range_Colors bluish→reddish) ──
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: root.legendType === 1

                    // Gradient bar
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 16
                        radius: 4
                        // 10-stop gradient matching upstream GCodeViewer Range_Colors
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

                    // Min / Max labels
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: root.previewVm ? root.previewVm.legendGradientMinLabel : "--"
                            color: Theme.textPrimary
                            font.pixelSize: 10
                            font.family: "monospace"
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: root.previewVm ? root.previewVm.legendGradientMaxLabel : "--"
                            color: Theme.textPrimary
                            font.pixelSize: 10
                            font.family: "monospace"
                        }
                    }
                }

                // ── Discrete / Extruder legend ──
                Repeater {
                    model: root.previewVm ? root.previewVm.legendItems : []
                    delegate: RowLayout {
                        spacing: 8
                        visible: root.legendType !== 1

                        Rectangle {
                            width: 10
                            height: 10
                            radius: 3
                            color: modelData.color
                        }
                        Label {
                            text: modelData.label + (modelData.count > 0 ? (" (" + modelData.count + ")") : "")
                            color: Theme.textPrimary
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }
}
