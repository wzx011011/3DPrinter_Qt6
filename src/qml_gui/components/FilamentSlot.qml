import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// Compact per-extruder filament preset selector aligned with the Prepare sidebar.
Item {
    id: root
    required property int slotIndex
    required property var configVm

    readonly property string presetName: configVm ? configVm.materialPresetName(slotIndex) : ""
    readonly property bool compatible: configVm ? configVm.isFilamentCompatible(presetName) : true
    readonly property color slotColor: {
        var colors = ["#b97914", "#b9b9b9", "#b9b9b9", "#214bc2", "#d63a21"]
        return slotIndex < colors.length ? colors[slotIndex] : "#b9b9b9"
    }

    implicitHeight: 34
    opacity: compatible ? 1.0 : 0.58

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusSM
        color: Theme.bgInset
        border.width: 1
        border.color: root.compatible ? Theme.borderSubtle : "#f05545"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 4
            spacing: 6

            Rectangle {
                width: 14
                height: 14
                radius: 7
                color: root.slotColor
                border.width: 1
                border.color: Qt.lighter(root.slotColor, 1.25)
                Layout.alignment: Qt.AlignVCenter

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    // TODO(Phase 56): Color metadata is not writable yet; keep the swatch non-mutating.
                    onClicked: {}
                }

                Loader {
                    id: colorPickerLoader
                    active: false
                    sourceComponent: Component {
                        Popup {
                            parent: root
                            y: parent.height + 4
                            x: 0
                            width: 120
                            height: 80
                            padding: 4
                            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                            onClosed: colorPickerLoader.active = false

                            GridLayout {
                                anchors.fill: parent
                                columns: 5
                                rowSpacing: 2
                                columnSpacing: 2

                                Repeater {
                                    model: ["#FFFFFF", "#F5F5DC", "#B9B9B9", "#808080", "#000000",
                                            "#B97914", "#D63A21", "#214BC2", "#22C55E", "#EAB308"]
                                    delegate: Rectangle {
                                        required property string modelData
                                        width: 18
                                        height: 18
                                        radius: Theme.radiusSM
                                        color: modelData
                                        border.width: 1
                                        border.color: Qt.darker(modelData, 1.2)
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: colorPickerLoader.active = false
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Text {
                text: (root.slotIndex + 1).toString()
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
                font.bold: true
                Layout.alignment: Qt.AlignVCenter
            }

            CxComboBox {
                id: filamentCombo
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                font.pixelSize: 9
                model: root.configVm ? root.configVm.filamentPresetNames : []
                currentIndex: {
                    if (!root.configVm) return -1
                    var names = root.configVm.filamentPresetNames
                    for (var i = 0; i < names.length; i++) {
                        if (names[i] === root.presetName) return i
                    }
                    return -1
                }
                onActivated: {
                    if (!root.configVm) return
                    var names = root.configVm.filamentPresetNames
                    if (index >= 0 && index < names.length)
                        root.configVm.requestCurrentFilamentPreset(names[index])
                }
            }

            Text {
                visible: !root.compatible
                text: "!"
                color: "#f05545"
                font.pixelSize: Theme.fontSizeSM
                font.bold: true
                Layout.alignment: Qt.AlignVCenter
                ToolTip.text: qsTr("Current filament may be incompatible with printer")
                ToolTip.visible: incompatibleMA.containsMouse
                MouseArea {
                    id: incompatibleMA
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.NoButton
                }
            }
        }
    }
}
