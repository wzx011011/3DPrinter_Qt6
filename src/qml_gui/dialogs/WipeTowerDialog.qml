import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P8.6b -- WipeTowerDialog (aligns with upstream WipeTowerDialog)
// Wipe tower configuration for multi-extruder: ramming, flushing volumes
// Usage: WipeTowerDialog { id: dlg }  ->  dlg.open()

Dialog {
    id: root

    modal: true
    closePolicy: Popup.NoAutoClose
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
    property var extruderColors: ["#3B82F6", "#EF4444", "#22C55E", "#F59E0B"]

    // Ramming settings
    property real rammingVolume: 10.0
    property real rammingLineWidthMulti: 1.0
    property real rammingStepMulti: 1.0

    // Wiping settings
    property real flushMultiplier: 1.0
    property real minFlushVolume: 80.0

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
            text: qsTr("擦料塔设置")
            color: "#e2e8f5"
            font.pixelSize: 14
            font.bold: true
        }
    }

    contentItem: ColumnLayout {
        width: parent.width
        spacing: 10
        anchors.margins: 16

        // Mode toggle
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: qsTr("模式")
                color: Theme.textSecondary
                font.pixelSize: 11
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
                    font.pixelSize: 11
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
                    font.pixelSize: 11
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.advancedMode = true
                }
            }
        }

        // ── Ramming Section ──
        Text {
            text: qsTr("撞击设置")
            color: Theme.textSecondary
            font.pixelSize: 11
            font.bold: true
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 4
            columnSpacing: 8
            rowSpacing: 6

            Text { text: qsTr("体积"); color: Theme.textTertiary; font.pixelSize: 10 }
            CxTextField { Layout.fillWidth: true; implicitHeight: 22; font.pixelSize: 10; text: root.rammingVolume.toFixed(1) }

            Text { text: qsTr("线宽倍率"); color: Theme.textTertiary; font.pixelSize: 10 }
            CxTextField { Layout.fillWidth: true; implicitHeight: 22; font.pixelSize: 10; text: root.rammingLineWidthMulti.toFixed(2) }

            Text { text: qsTr("步进倍率"); color: Theme.textTertiary; font.pixelSize: 10 }
            CxTextField { Layout.fillWidth: true; implicitHeight: 22; font.pixelSize: 10; text: root.rammingStepMulti.toFixed(2) }
        }

        // ── Wiping Section ──
        Text {
            text: qsTr("擦洗设置")
            color: Theme.textSecondary
            font.pixelSize: 11
            font.bold: true
            topPadding: 4
        }

        // Flush volume matrix (simplified)
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: flushGrid.implicitHeight + 12
            radius: 4
            color: "#161b24"
            border.color: "#2e3848"
            border.width: 1

            Grid {
                id: flushGrid
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 6
                columns: 5
                rowSpacing: 4
                columnSpacing: 4

                // Header row
                Text { width: 50; height: 20; text: ""; color: Theme.textTertiary; font.pixelSize: 9 }
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
                        spacing: 4

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
                                font.pixelSize: 9
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
            spacing: 8

            Text {
                Layout.preferredWidth: 70
                text: qsTr("擦洗倍率")
                color: Theme.textTertiary
                font.pixelSize: 10
            }
            CxTextField {
                Layout.preferredWidth: 60
                implicitHeight: 22
                font.pixelSize: 10
                text: root.flushMultiplier.toFixed(2)
            }

            Text {
                Layout.preferredWidth: 70
                text: qsTr("最小体积")
                color: Theme.textTertiary
                font.pixelSize: 10
            }
            CxTextField {
                Layout.preferredWidth: 60
                implicitHeight: 22
                font.pixelSize: 10
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
