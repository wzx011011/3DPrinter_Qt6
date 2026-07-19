import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P8.4 -- AMS Settings Dialog (aligns with upstream AMSMaterialsSetting / AMSSetting / AmsMappingPopup)
// Filament slot management (4 slots), mapping rules, filament remaining progress
// Usage: AMSSettingsDialog { id: dlg }  ->  dlg.open()

CxDialog {
    id: root

    closePolicy: Popup.NoAutoClose

    dialogTitle: qsTr("AMS 设置")

    anchors.centerIn: parent
    width: 500
    height: 440

    // Mock filament data (aligns with upstream AMS slot model)
    readonly property var slotColors: [Theme.statusInfo, Theme.statusError, Theme.accent, Theme.statusWarning]
    readonly property var slotNames: [qsTr("蓝色 PLA"), qsTr("红色 ABS"), qsTr("绿色 PETG"), qsTr("黄色 TPU")]
    readonly property var slotMaterials: ["PLA", "ABS", "PETG", "TPU"]
    readonly property var slotAutoSwap: [true, false, true, false]
    readonly property var materialTypes: ["PLA", "ABS", "PETG", "TPU", "ASA", "PC", "PA-CF", "PVA"]

    // Mock mapping data (aligns with upstream AmsMappingPopup)
    readonly property var mappingRules: [
        { slot: 1, extruder: 1, temp: 215 },
        { slot: 2, extruder: 2, temp: 230 },
        { slot: 3, extruder: 3, temp: 240 }
    ]

    // Mock remaining percentages (aligns with upstream AMS filament remaining)
    readonly property var remainingPct: [65, 42, 88, 15]

    // Editable state
    property var editNames: []
    property var editMaterials: []
    property var editAutoSwap: []

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
                    model: 4

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
                                    color: root.slotColors[index]
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
                                    Component.onCompleted: checked = (root.editAutoSwap[index] !== undefined)
                                        ? root.editAutoSwap[index]
                                        : root.slotAutoSwap[index]
                                    onCheckedChanged: {
                                        var arr = root.editAutoSwap.slice()
                                        arr[index] = checked
                                        root.editAutoSwap = arr
                                    }
                                }
                            }

                            // Filament name
                            CxTextField {
                                Layout.fillWidth: true
                                implicitHeight: 24
                                font.pixelSize: Theme.fontSizeSM
                                text: root.editNames[index] !== undefined
                                    ? root.editNames[index]
                                    : root.slotNames[index]
                                onEditingFinished: {
                                    var arr = root.editNames.slice()
                                    arr[index] = text
                                    root.editNames = arr
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
                                    model: root.materialTypes
                                    currentIndex: {
                                        var m = root.editMaterials[index] !== undefined
                                            ? root.editMaterials[index]
                                            : root.slotMaterials[index]
                                        return root.materialTypes.indexOf(m)
                                    }
                                    onActivated: function(idx) {
                                        var arr = root.editMaterials.slice()
                                        arr[index] = root.materialTypes[idx]
                                        root.editMaterials = arr
                                    }
                                }

                                // Preset color buttons
                                Row {
                                    spacing: Theme.spacingXS
                                    Repeater {
                                        model: root.slotColors
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
                    model: root.mappingRules
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
                        }
                    }
                }
            }

            CxButton {
                Layout.alignment: Qt.AlignRight
                text: qsTr("添加映射")
                cxStyle: CxButton.Style.Secondary
                compact: true
                enabled: false
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
                model: 4

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingMD
                    Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        color: root.slotColors[index]
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
                            width: parent.width * (root.remainingPct[index] / 100)
                            height: parent.height
                            radius: 5
                            color: {
                                var pct = root.remainingPct[index]
                                if (pct <= 20) return Theme.statusError
                                if (pct <= 50) return Theme.statusWarning
                                return Theme.accent
                            }
                        }
                    }

                    Text {
                        text: root.remainingPct[index] + "%"
                        color: {
                            var pct = root.remainingPct[index]
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

            CxButton {
                text: qsTr("关闭")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.close()
            }
        }
    }

    onOpened: {
        editNames = slotNames.slice()
        editMaterials = slotMaterials.slice()
        editAutoSwap = slotAutoSwap.slice()
    }
}
