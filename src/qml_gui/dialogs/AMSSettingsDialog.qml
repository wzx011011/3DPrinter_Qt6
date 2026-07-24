import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P8.4 -- AMS Settings Dialog (aligns with upstream AMSMaterialsSetting / AMSSetting / AmsMappingPopup)
// Filament slot management (4 slots), mapping rules, filament remaining progress.
// Usage: AMSSettingsDialog { id: dlg }  ->  dlg.open()
//
// Phase 201 (v5.6 AMS Architecture Cleanup): all mock data previously hardcoded
// as readonly QML properties is now sourced from AmsMaterialsViewModel
// (backend.amsMaterialsViewModel). Edits go through the viewmodel's set* /
// addMappingRule / removeMappingRule / resetToDefaults APIs and are persisted
// to local QSettings (ams/materials/*). No network / device / cloud access.

CxDialog {
    id: root

    closePolicy: Popup.NoAutoClose

    dialogTitle: qsTr("AMS 设置")

    anchors.centerIn: parent
    width: 500
    height: 440

    // ViewModel binding (Phase 201). Set by main.qml via required property or
    // directly. When null the dialog renders with empty state safely.
    property var amsVm: null

    // Convenience read-only views over the viewmodel. Empty fallbacks keep the
    // dialog robust before the viewmodel is wired.
    readonly property var _slotColors: amsVm ? amsVm.slotColors : []
    readonly property var _slotNames: amsVm ? amsVm.slotNames : []
    readonly property var _slotMaterials: amsVm ? amsVm.slotMaterials : []
    readonly property var _slotAutoSwap: amsVm ? amsVm.slotAutoSwap : []
    readonly property var _remainingPct: amsVm ? amsVm.remainingPct : []
    readonly property var _materialTypes: amsVm ? amsVm.materialTypes : []
    readonly property var _mappingRules: amsVm ? amsVm.mappingRules : []
    readonly property int _slotCount: amsVm ? amsVm.slotCount : 4

    contentItem: ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: Theme.spacingLG
        clip: true
        contentWidth: availableWidth
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: Theme.spacingMD
            // -- Section 1: Filament Slot Management --
            Text {
                text: qsTr("耗材槽位管理")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
                font.bold: true
            }

            // 4 slot cards in a 2x2 grid
            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 8
                rowSpacing: 8

                Repeater {
                    model: root._slotCount

                    Rectangle {
                        required property int index
                        Layout.fillWidth: true
                        implicitHeight: slotCol.implicitHeight + 16
                        radius: 6
                        color: Theme.scrollBarTrackColor
                        border.color: Theme.borderInput
                        border.width: 1

                        ColumnLayout {
                            id: slotCol
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: Theme.spacingMD
                            spacing: Theme.spacingSM
                            // Slot header: color indicator + name
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.spacingSM
                                Rectangle {
                                    width: 16
                                    height: 16
                                    radius: 8
                                    color: root._slotColors[index] || Theme.borderDefault
                                    border.color: Theme.borderDefault
                                    border.width: 1
                                }

                                Text {
                                    text: qsTr("槽位 %1").arg(index + 1)
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeSM
                                    font.bold: true
                                }

                                Item { Layout.fillWidth: true }

                                CxCheckBox {
                                    text: qsTr("自动换色")
                                    font.pixelSize: Theme.fontSizeXS
                                    // Bind directly to the viewmodel so edits persist immediately.
                                    checked: root._slotAutoSwap[index] === true
                                    onCheckedChanged: {
                                        if (root.amsVm)
                                            root.amsVm.setSlotAutoSwap(index, checked)
                                    }
                                }
                            }

                            // Filament name
                            CxTextField {
                                Layout.fillWidth: true
                                implicitHeight: 24
                                font.pixelSize: Theme.fontSizeSM
                                text: root._slotNames[index] || ""
                                onEditingFinished: {
                                    if (root.amsVm)
                                        root.amsVm.setSlotName(index, text)
                                }
                            }

                            // Material type + color picker row
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.spacingSM
                                CxComboBox {
                                    Layout.fillWidth: true
                                    implicitHeight: 24
                                    font.pixelSize: Theme.fontSizeSM
                                    model: root._materialTypes
                                    currentIndex: {
                                        var m = root._slotMaterials[index] || ""
                                        return root._materialTypes.indexOf(m)
                                    }
                                    onActivated: function(idx) {
                                        if (root.amsVm)
                                            root.amsVm.setSlotMaterial(index, root._materialTypes[idx])
                                    }
                                }

                                // Preset color buttons
                                Row {
                                    spacing: Theme.spacingXS
                                    Repeater {
                                        model: root._slotColors
                                        Rectangle {
                                            required property int index
                                            required property string modelData
                                            width: 16
                                            height: 16
                                            radius: 3
                                            color: modelData
                                            border.color: Theme.textDisabled
                                            border.width: 1

                                            MouseArea {
                                                anchors.fill: parent
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    // Visual feedback placeholder
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // -- Section 2: Mapping Configuration --
            Text {
                text: qsTr("映射配置")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
                font.bold: true
                topPadding: 4
            }

            // Mapping ListView
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: mappingList.contentHeight + 8
                radius: 4
                color: Theme.bgPanel
                border.color: Theme.borderInput
                border.width: 1

                ListView {
                    id: mappingList
                    anchors.fill: parent
                    anchors.margins: Theme.spacingXS
                    model: root._mappingRules
                    spacing: Theme.spacingXS
                    interactive: false

                    delegate: Rectangle {
                        required property int index
                        required property var modelData
                        width: ListView.view.width
                        height: 26
                        radius: 3
                        color: index % 2 === 0 ? Theme.chromeHover : Theme.bgPanel

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spacingMD
                            anchors.rightMargin: Theme.spacingMD
                            spacing: Theme.spacingMD
                            Text {
                                text: qsTr("Slot %1").arg(modelData.slot)
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeSM
                                font.bold: true
                            }

                            Text {
                                text: "->"
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSizeSM
                            }

                            Text {
                                text: qsTr("Extruder %1").arg(modelData.extruder)
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeSM
                            }

                            Text {
                                text: qsTr("(温度覆盖 %1°C)").arg(modelData.temp)
                                color: Theme.accent
                                font.pixelSize: Theme.fontSizeSM
                            }

                            Item { Layout.fillWidth: true }

                            // Remove a single mapping rule (Phase 201: previously no delete UI).
                            CxButton {
                                text: "x"
                                cxStyle: CxButton.Style.Secondary
                                compact: true
                                onClicked: {
                                    if (root.amsVm)
                                        root.amsVm.removeMappingRule(index)
                                }
                            }
                        }
                    }
                }
            }

            // Phase 201: "Add mapping" is now functional (was enabled:false).
            // Appends a default rule for the next slot/extruder pair; the user
            // tunes values via the same viewmodel APIs in a follow-up editor.
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                CxButton {
                    Layout.alignment: Qt.AlignRight
                    text: qsTr("添加映射")
                    cxStyle: CxButton.Style.Secondary
                    compact: true
                    enabled: root.amsVm !== null
                    onClicked: {
                        var next = root.amsVm.mappingRuleCount + 1
                        root.amsVm.addMappingRule(next, next, 210)
                    }
                }
            }

            // -- Section 3: Filament Remaining --
            Text {
                text: qsTr("槽位耗材余量")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
                font.bold: true
                topPadding: 4
            }

            Repeater {
                model: root._slotCount

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingMD
                    Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        color: root._slotColors[index] || Theme.borderDefault
                        border.color: Theme.borderDefault
                        border.width: 1
                    }

                    Text {
                        text: qsTr("Slot %1:").arg(index + 1)
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                        Layout.preferredWidth: 50
                    }

                    // Progress bar
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 10
                        radius: 5
                        color: Theme.scrollBarTrackColor
                        border.color: Theme.borderInput
                        border.width: 1

                        Rectangle {
                            width: parent.width * ((root._remainingPct[index] || 0) / 100)
                            height: parent.height
                            radius: 5
                            color: {
                                var pct = root._remainingPct[index] || 0
                                if (pct <= 20) return Theme.statusError
                                if (pct <= 50) return Theme.statusWarning
                                return Theme.accent
                            }
                        }
                    }

                    Text {
                        text: (root._remainingPct[index] || 0) + "%"
                        color: {
                            var pct = root._remainingPct[index] || 0
                            if (pct <= 20) return Theme.statusError
                            if (pct <= 50) return Theme.statusWarning
                            return Theme.textPrimary
                        }
                        font.pixelSize: Theme.fontSizeSM
                        font.bold: true
                        Layout.preferredWidth: 36
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }
        }
    }

    footer: Rectangle {
        width: parent.width
        height: 48
        color: Theme.bgSurface
        radius: 8
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 12
            color: parent.color
        }

        RowLayout {
            anchors.fill: parent
            anchors.rightMargin: Theme.spacingXL
            spacing: Theme.spacingMD
            Item { Layout.fillWidth: true }

            // Phase 201: reset mock data + clear persisted overrides.
            CxButton {
                text: qsTr("重置默认")
                cxStyle: CxButton.Style.Secondary
                enabled: root.amsVm !== null
                onClicked: {
                    if (root.amsVm)
                        root.amsVm.resetToDefaults()
                }
            }

            CxButton {
                text: qsTr("关闭")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.close()
            }
        }
    }
}
