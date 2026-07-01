import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// FilamentSlot — interactive per-extruder filament preset selector (aligned with upstream Sidebar filament slots)
// Shows: color dot, preset dropdown, compatibility state
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

    implicitHeight: 58
    opacity: compatible ? 1.0 : 0.5

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: root.slotColor
        border.width: root.compatible ? 0 : 2
        border.color: "#f05545"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 4
            spacing: 2

            // Row: color dot + slot number
            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                Rectangle {
                    width: 14
                    height: 14
                    radius: 7
                    color: root.slotColor
                    border.width: 1
                    border.color: Qt.lighter(root.slotColor, 1.3)

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        // Phase 52 PREPSB-01: color picker popup hidden --
                        // selecting a color did nothing (visual only).
                        // Full filament color metadata (PresetBundle
                        // filament_colour) is Phase 56 settings work.
                        // TODO(Phase 56): wire colorPickerLoader to real
                        // filament color metadata + configVm setter.
                        // onClicked intentionally a no-op until Phase 56.
                        onClicked: {}
                    }

                    Loader {
                        id: colorPickerLoader
                        // Phase 52 PREPSB-01: kept inactive -- popup is non-
                        // functional until Phase 56 wires real filament color
                        // metadata. Do NOT re-enable without a working setter.
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
                                            radius: 4
                                            color: modelData
                                            border.width: 1
                                            border.color: Qt.darker(modelData, 1.2)
                                            MouseArea {
                                                anchors.fill: parent
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    // Color selection — for now visual only
                                                    colorPickerLoader.active = false
                                                }
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
                    color: "#101114"
                    font.pixelSize: 10
                    font.bold: true
                }

                Item { Layout.fillWidth: true }
            }

            // Preset dropdown
            CxComboBox {
                id: filamentCombo
                Layout.fillWidth: true
                Layout.preferredHeight: 22
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
        }
    }
}
