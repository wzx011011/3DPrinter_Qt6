import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P8.4 -- AMS Settings Dialog (aligns with upstream AMSMaterialsSetting / AMSSetting / AmsMappingPopup)
// Filament slot management (4 slots), mapping rules, filament remaining progress
// Usage: AMSSettingsDialog { id: dlg }  ->  dlg.open()

Dialog {
    id: root

    modal: true
    closePolicy: Popup.NoAutoClose
    anchors.centerIn: parent
    width: 500
    height: 440

    // Mock filament data (aligns with upstream AMS slot model)
    readonly property var slotColors: ["#3B82F6", "#EF4444", "#22C55E", "#F59E0B"]
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

    background: Rectangle {
        color: "#1a1f28"
        radius: 8
        border.color: "#2e3848"
        border.width: 1
    }

    ColumnLayout {
        width: parent.width
        height: 40
        spacing: 0

        Item { Layout.fillWidth: true; Layout.fillHeight: true }

        Text {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.alignment: Qt.AlignVCenter
            text: qsTr("AMS 设置")
            color: "#e2e8f5"
            font.pixelSize: 14
            font.bold: true
        }
    }

    contentItem: ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 12
        clip: true
        contentWidth: availableWidth
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 10

            // ── Section 1: Filament Slot Management ──
            Text {
                text: qsTr("耗材槽位管理")
                color: Theme.textSecondary
                font.pixelSize: 11
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
                        color: "#1e2330"
                        border.color: "#2e3848"
                        border.width: 1

                        ColumnLayout {
                            id: slotCol
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 10
                            spacing: 6

                            // Slot header: color indicator + name
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                Rectangle {
                                    width: 16
                                    height: 16
                                    radius: 8
                                    color: root.slotColors[index]
                                    border.color: "#3a4050"
                                    border.width: 1
                                }

                                Text {
                                    text: qsTr("槽位 %1").arg(index + 1)
                                    color: Theme.textPrimary
                                    font.pixelSize: 11
                                    font.bold: true
                                }

                                Item { Layout.fillWidth: true }

                                CxCheckBox {
                                    text: qsTr("自动换色")
                                    font.pixelSize: 10
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
                                font.pixelSize: 11
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
                                spacing: 6

                                CxComboBox {
                                    Layout.fillWidth: true
                                    implicitHeight: 24
                                    font.pixelSize: 11
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
                                    spacing: 3
                                    Repeater {
                                        model: root.slotColors
                                        Rectangle {
                                            required property int index
                                            required property string modelData
                                            width: 16
                                            height: 16
                                            radius: 3
                                            color: modelData
                                            border.color: "#566070"
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

            // ── Section 2: Mapping Configuration ──
            Text {
                text: qsTr("映射配置")
                color: Theme.textSecondary
                font.pixelSize: 11
                font.bold: true
                topPadding: 4
            }

            // Mapping ListView
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: mappingList.contentHeight + 8
                radius: 4
                color: "#161b24"
                border.color: "#2e3848"
                border.width: 1

                ListView {
                    id: mappingList
                    anchors.fill: parent
                    anchors.margins: 4
                    model: root.mappingRules
                    spacing: 2
                    interactive: false

                    delegate: Rectangle {
                        required property int index
                        required property var modelData
                        width: ListView.view.width
                        height: 26
                        radius: 3
                        color: index % 2 === 0 ? "#1a2030" : "#161b24"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 8

                            Text {
                                text: qsTr("Slot %1").arg(modelData.slot)
                                color: Theme.textPrimary
                                font.pixelSize: 11
                                font.bold: true
                            }

                            Text {
                                text: "->"
                                color: Theme.textTertiary
                                font.pixelSize: 11
                            }

                            Text {
                                text: qsTr("Extruder %1").arg(modelData.extruder)
                                color: Theme.textSecondary
                                font.pixelSize: 11
                            }

                            Text {
                                text: qsTr("(温度覆盖 %1°C)").arg(modelData.temp)
                                color: Theme.accent
                                font.pixelSize: 11
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

            // ── Section 3: Filament Remaining ──
            Text {
                text: qsTr("槽位耗材余量")
                color: Theme.textSecondary
                font.pixelSize: 11
                font.bold: true
                topPadding: 4
            }

            Repeater {
                model: 4

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        color: root.slotColors[index]
                        border.color: "#3a4050"
                        border.width: 1
                    }

                    Text {
                        text: qsTr("Slot %1:").arg(index + 1)
                        color: Theme.textSecondary
                        font.pixelSize: 11
                        Layout.preferredWidth: 50
                    }

                    // Progress bar
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 10
                        radius: 5
                        color: "#1e2330"
                        border.color: "#2e3848"
                        border.width: 1

                        Rectangle {
                            width: parent.width * (root.remainingPct[index] / 100)
                            height: parent.height
                            radius: 5
                            color: {
                                var pct = root.remainingPct[index]
                                if (pct <= 20) return "#EF4444"
                                if (pct <= 50) return "#F59E0B"
                                return "#18c75e"
                            }
                        }
                    }

                    Text {
                        text: root.remainingPct[index] + "%"
                        color: {
                            var pct = root.remainingPct[index]
                            if (pct <= 20) return "#EF4444"
                            if (pct <= 50) return "#F59E0B"
                            return Theme.textPrimary
                        }
                        font.pixelSize: 11
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
        color: "#141920"
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
            anchors.rightMargin: 16
            spacing: 10

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
