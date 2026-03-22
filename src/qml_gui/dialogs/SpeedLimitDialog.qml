import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P8.6a -- SpeedLimitDialog (aligns with upstream AccelerationAndSpeedLimitDialog)
// Speed and acceleration limit configuration by weight/height ranges
// Usage: SpeedLimitDialog { id: dlg }  ->  dlg.open()

Dialog {
    id: root

    modal: true
    closePolicy: Popup.NoAutoClose
    anchors.centerIn: parent
    width: 440
    height: 360

    // Mock limit data (aligns with upstream acceleration/speed limit items)
    property var limitItems: [
        { enabled: true, rangeMin: "0", rangeMax: "50", speedLimit: "200", accelLimit: "3000", type: qsTr("重量") },
        { enabled: true, rangeMin: "50", rangeMax: "100", speedLimit: "150", accelLimit: "2500", type: qsTr("重量") },
        { enabled: false, rangeMin: "0", rangeMax: "10", speedLimit: "100", accelLimit: "2000", type: qsTr("高度") }
    ]

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
            text: qsTr("速度与加速度限制")
            color: "#e2e8f5"
            font.pixelSize: 14
            font.bold: true
        }
    }

    contentItem: ColumnLayout {
        width: parent.width
        spacing: 8
        anchors.margins: 16

        // Header row
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text { Layout.preferredWidth: 20; text: ""; color: Theme.textTertiary; font.pixelSize: 10 }
            Text { Layout.preferredWidth: 50; text: qsTr("范围最小"); color: Theme.textTertiary; font.pixelSize: 10 }
            Text { Layout.preferredWidth: 50; text: qsTr("范围最大"); color: Theme.textTertiary; font.pixelSize: 10 }
            Text { Layout.preferredWidth: 70; text: qsTr("速度限制"); color: Theme.textTertiary; font.pixelSize: 10 }
            Text { Layout.preferredWidth: 70; text: qsTr("加速度限制"); color: Theme.textTertiary; font.pixelSize: 10 }
            Text { Layout.preferredWidth: 40; text: qsTr("类型"); color: Theme.textTertiary; font.pixelSize: 10 }
            Item { Layout.fillWidth: true }
        }

        // Separator
        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#2e3848" }

        // Limit items list
        Repeater {
            model: root.limitItems

            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Layout.topMargin: 4

                CxCheckBox {
                    Layout.preferredWidth: 20
                    checked: modelData.enabled
                    onCheckedChanged: root.limitItems[index].enabled = checked
                }

                CxTextField {
                    Layout.preferredWidth: 50
                    implicitHeight: 24
                    font.pixelSize: 10
                    text: modelData.rangeMin
                    enabled: modelData.enabled
                }

                CxTextField {
                    Layout.preferredWidth: 50
                    implicitHeight: 24
                    font.pixelSize: 10
                    text: modelData.rangeMax
                    enabled: modelData.enabled
                }

                CxTextField {
                    Layout.preferredWidth: 70
                    implicitHeight: 24
                    font.pixelSize: 10
                    text: modelData.speedLimit
                    enabled: modelData.enabled
                }

                CxTextField {
                    Layout.preferredWidth: 70
                    implicitHeight: 24
                    font.pixelSize: 10
                    text: modelData.accelLimit
                    enabled: modelData.enabled
                }

                Text {
                    Layout.preferredWidth: 40
                    text: modelData.type
                    color: modelData.enabled ? Theme.textSecondary : Theme.textTertiary
                    font.pixelSize: 10
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    width: 20
                    height: 20
                    radius: 3
                    color: "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "×"
                        color: "#EF4444"
                        font.pixelSize: 14
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { /* Mock: remove item */ }
                    }
                }
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
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 10

            CxButton {
                text: qsTr("添加")
                cxStyle: CxButton.Style.Secondary
                compact: true
                enabled: false
            }

            CxButton {
                text: qsTr("重置默认")
                cxStyle: CxButton.Style.Secondary
                compact: true
            }

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
