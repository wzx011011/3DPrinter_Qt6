import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"
import "../components"

// FilamentPanel — per-extruder filament slot management
// Aligned with upstream Plater filament management section
Item {
    id: root
    required property var editorVm
    required property var configVm

    implicitHeight: filamentCol.implicitHeight + 16

    // Right-click context menu for filament slots (aligned with upstream MaterialContextMenu)
    CxMenu {
        id: filamentContextMenu
        property int slotIndex: -1

        CxMenuItem {
            text: qsTr("编辑")
            onTriggered: {
                if (!root.configVm || filamentContextMenu.slotIndex < 0) return
                var presets = root.configVm.filamentPresetNames
                var name = filamentContextMenu.slotIndex < presets.length
                    ? presets[filamentContextMenu.slotIndex] : ""
                root.configVm.setCurrentFilamentPreset(name)
            }
        }

        CxMenuItem {
            text: qsTr("删除")
            enabled: root.editorVm && root.editorVm.extruderCount > 1
            onTriggered: {
                if (root.editorVm && filamentContextMenu.slotIndex >= 0)
                    root.editorVm.deleteFilamentSlot(filamentContextMenu.slotIndex)
            }
        }

        CxMenu {
            title: qsTr("合并到...")
            visible: root.editorVm && root.editorVm.extruderCount > 1

            Repeater {
                model: root.editorVm ? root.editorVm.extruderCount : 0
                delegate: CxMenuItem {
                    required property int index
                    // Skip the current slot — can't merge with self
                    visible: index !== filamentContextMenu.slotIndex
                    text: {
                        if (!root.configVm) return qsTr("T%1").arg(index)
                        var presets = root.configVm.filamentPresetNames
                        var name = index < presets.length ? presets[index] : ""
                        return name.length > 0
                            ? qsTr("T%1 — %2").arg(index).arg(name)
                            : qsTr("T%1").arg(index)
                    }
                    onTriggered: {
                        if (root.editorVm && filamentContextMenu.slotIndex >= 0)
                            root.editorVm.mergeFilamentSlots(filamentContextMenu.slotIndex, index)
                    }
                }
            }
        }
    }

    ColumnLayout {
        id: filamentCol
        anchors.fill: parent
        spacing: 6

        CxSectionHeader {
            Layout.fillWidth: true
            title: qsTr("耗材")
            subtitle: root.editorVm
                ? qsTr("%1 个挤出机").arg(root.editorVm.extruderCount)
                : ""
        }

        // Per-extruder slots
        Repeater {
            model: root.editorVm ? root.editorVm.extruderCount : 0

            delegate: Rectangle {
                required property int index
                Layout.fillWidth: true
                height: 32
                radius: 6
                color: slotMA.containsMouse ? Theme.bgHover : Theme.bgElevated
                border.width: 1
                border.color: root.editorVm && root.editorVm.mmuSelectedExtruder === index
                    ? Theme.accent : Theme.borderSubtle

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 6

                    // Color circle
                    Rectangle {
                        width: 16
                        height: 16
                        radius: 8
                        color: {
                            if (!root.configVm) return "#888"
                            var presets = root.configVm.filamentPresetNames
                            if (index < presets.length) {
                                // Use hash of name for color
                                var h = 0
                                for (var i = 0; i < presets[index].length; i++)
                                    h = (h * 31 + presets[index].charCodeAt(i)) % 360
                                return Qt.hsla(h / 360, 0.7, 0.5, 1.0)
                            }
                            return "#888"
                        }
                        border.width: 1
                        border.color: Theme.borderSubtle
                    }

                    // Extruder label
                    Text {
                        text: qsTr("T%1").arg(index)
                        color: Theme.textSecondary
                        font.pixelSize: 10
                        font.bold: true
                    }

                    // Preset name
                    Text {
                        Layout.fillWidth: true
                        text: {
                            if (!root.configVm) return ""
                            var presets = root.configVm.filamentPresetNames
                            return index < presets.length ? presets[index] : ""
                        }
                        color: Theme.textPrimary
                        font.pixelSize: 10
                        elide: Text.ElideRight
                    }
                }

                MouseArea {
                    id: slotMA
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    cursorShape: Qt.PointingHandCursor
                    onClicked: function(mouse) {
                        if (mouse.button === Qt.RightButton) {
                            filamentContextMenu.slotIndex = index
                            filamentContextMenu.popup()
                        } else {
                            if (root.editorVm)
                                root.editorVm.setMmuSelectedExtruder(index)
                        }
                    }
                }
            }
        }

        // Add filament + bed type row
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            CxButton {
                text: qsTr("+ 耗材")
                cxStyle: CxButton.Style.Secondary
                implicitHeight: 24
                font.pixelSize: Theme.fontSizeXS
                onClicked: {
                    if (root.configVm)
                        root.configVm.setCurrentFilamentPreset("")
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: qsTr("自动匹配")
                color: autoMatchMA.containsMouse ? Theme.accent : Theme.textSecondary
                font.pixelSize: 9
                font.bold: true

                MouseArea {
                    id: autoMatchMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (root.configVm)
                            root.configVm.autoMatchFilament()
                    }
                }
            }
        }
    }
}
