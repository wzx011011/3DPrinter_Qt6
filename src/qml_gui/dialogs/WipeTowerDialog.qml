import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P8.6b -- WipeTowerDialog (aligns with upstream WipeTowerDialog)
// Wipe tower configuration for multi-extruder: ramming, flushing volumes
// Usage: WipeTowerDialog { id: dlg }  ->  dlg.open()

CxDialog {
    id: root

    closePolicy: Popup.NoAutoClose

    dialogTitle: qsTr("擦料塔设置")

    anchors.centerIn: parent
    width: 420
    height: 380

    property bool advancedMode: false

    // Mock flush volume matrix (aligns with upstream extruder-to-extruder flush volumes)
    property var flushMatrix: [
        [0, 140, 160, 120],
        [140, 0, 150, 130],
        [160, 150, 0, 140],
        [120, 130, 140, 0]
    ]

    property var extruderNames: [qsTr("耗材1"), qsTr("耗材2"), qsTr("耗材3"), qsTr("耗材4")]
    property var extruderColors: [Theme.statusInfo, Theme.statusError, Theme.accent, Theme.statusWarning]

    // Ramming settings
    property real rammingVolume: 10.0
    property real rammingLineWidthMulti: 1.0
    property real rammingStepMulti: 1.0

    // Wiping settings
    property real flushMultiplier: 1.0
    property real minFlushVolume: 80.0

    contentItem: ColumnLayout {
        width: root.width
        spacing: Theme.spacingMD
        anchors.margins: Theme.spacingXL
        // Mode toggle
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            Text {
                text: qsTr("模式")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
            }

            Rectangle {
                width: simpleText.implicitWidth + 16
                height: 24
                radius: 4
                color: !root.advancedMode ? Theme.accent : Theme.bgElevated
                border.width: 1
                border.color: !root.advancedMode ? Theme.accent : Theme.borderSubtle

                Text {
                    id: simpleText
                    anchors.centerIn: parent
                    text: qsTr("简单")
                    color: !root.advancedMode ? Theme.textOnAccent : Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.advancedMode = false
                }
            }

            Rectangle {
                width: advText.implicitWidth + 16
                height: 24
                radius: 4
                color: root.advancedMode ? Theme.accent : Theme.bgElevated
                border.width: 1
                border.color: root.advancedMode ? Theme.accent : Theme.borderSubtle

                Text {
                    id: advText
                    anchors.centerIn: parent
                    text: qsTr("高级")
                    color: root.advancedMode ? Theme.textOnAccent : Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.advancedMode = true
                }
            }
        }

        // -- Ramming Section --
        Text {
            text: qsTr("撞击设置")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
            font.bold: true
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 4
            columnSpacing: 8
            rowSpacing: 6

            Text { text: qsTr("体积"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS }
            CxTextField { Layout.fillWidth: true; implicitHeight: 22; font.pixelSize: Theme.fontSizeXS; text: root.rammingVolume.toFixed(1) }

            Text { text: qsTr("线宽倍率"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS }
            CxTextField { Layout.fillWidth: true; implicitHeight: 22; font.pixelSize: Theme.fontSizeXS; text: root.rammingLineWidthMulti.toFixed(2) }

            Text { text: qsTr("步进倍率"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS }
            CxTextField { Layout.fillWidth: true; implicitHeight: 22; font.pixelSize: Theme.fontSizeXS; text: root.rammingStepMulti.toFixed(2) }
        }

        // -- Wiping Section --
        Text {
            text: qsTr("擦洗设置")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
            font.bold: true
            topPadding: 4
        }

        // Flush volume matrix (simplified)
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: flushGrid.implicitHeight + 12
            radius: 4
            color: Theme.bgPanel
            border.color: Theme.borderInput
            border.width: 1

            Grid {
                id: flushGrid
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: Theme.spacingSM
                columns: 5
                rowSpacing: 4
                columnSpacing: 4

                // Header row
                Text { width: 50; height: 20; text: ""; color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS }
                Repeater {
                    model: root.extruderNames.length
                    Rectangle {
                        width: 60
                        height: 20
                        radius: 3
                        color: root.extruderColors[index]
                        opacity: 0.3
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("耗材%1").arg(index + 1)
                            color: root.extruderColors[index]
                            font.pixelSize: 8
                            font.bold: true
                        }
                    }
                }

                // Data rows
                Repeater {
                    model: root.extruderNames.length

                    Row {
                        spacing: Theme.spacingXS
                        // Row header
                        Rectangle {
                            width: 50
                            height: 22
                            radius: 3
                            color: root.extruderColors[index]
                            opacity: 0.3
                            Text {
                                anchors.centerIn: parent
                                text: qsTr("耗材%1").arg(index + 1)
                                color: root.extruderColors[index]
                                font.pixelSize: 8
                                font.bold: true
                            }
                        }

                        // Flush volume cells
                        Repeater {
                            model: root.extruderNames.length
                            CxTextField {
                                width: 60
                                implicitHeight: 22
                                font.pixelSize: Theme.fontSizeXS
                                horizontalAlignment: Text.AlignHCenter
                                text: parent.Repeater ? (root.flushMatrix[index] !== undefined
                                    ? root.flushMatrix[index][modelData] : "0") : "0"
                                enabled: index !== parent.parent.parent.index
                            }
                        }
                    }
                }
            }
        }

        // Multiplier + min volume
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            Text {
                Layout.preferredWidth: 70
                text: qsTr("擦洗倍率")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeXS
            }
            CxTextField {
                Layout.preferredWidth: 60
                implicitHeight: 22
                font.pixelSize: Theme.fontSizeXS
                text: root.flushMultiplier.toFixed(2)
            }

            Text {
                Layout.preferredWidth: 70
                text: qsTr("最小体积")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeXS
            }
            CxTextField {
                Layout.preferredWidth: 60
                implicitHeight: 22
                font.pixelSize: Theme.fontSizeXS
                text: root.minFlushVolume.toFixed(1)
            }

            Item { Layout.fillWidth: true }

            CxButton {
                text: qsTr("计算")
                cxStyle: CxButton.Style.Secondary
                compact: true
                enabled: false
            }

            CxButton {
                text: qsTr("重置")
                cxStyle: CxButton.Style.Secondary
                compact: true
            }
        }

        Item { Layout.fillHeight: true }
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
                text: qsTr("确定")
                cxStyle: CxButton.Style.Primary
                onClicked: root.accept()
            }

            CxButton {
                text: qsTr("取消")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.reject()
            }
        }
    }
}
