import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../dialogs"

Item {
    id: root
    required property var settingsVm

    AboutDialog {
        id: aboutDlg
    }

    // 切换到《关于》分类时自动弹出
    Connections {
        target: root.settingsVm
        function onPrefCategoryChanged() {
            if (root.settingsVm.prefCategory === 7)
                aboutDlg.open()
        }
    }

    Rectangle { anchors.fill: parent; color: "#0d0f12" }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Sidebar categories
        Rectangle {
            Layout.preferredWidth: 200; Layout.fillHeight: true; color: "#0f1218"

            Column {
                anchors.fill: parent; anchors.topMargin: 12; spacing: 2

                Repeater {
                    model: [
                        { icon: "🎨", name: qsTr("外观") },
                        { icon: "🌍", name: qsTr("语言") },
                        { icon: "⌨",  name: qsTr("快捷键") },
                        { icon: "🖨",  name: qsTr("打印机") },
                        { icon: "🔒",  name: qsTr("账号与隐私") },
                        { icon: "📦",  name: qsTr("更新") },
                        { icon: "🛠",  name: qsTr("高级") },
                        { icon: "❓",  name: qsTr("关于") }
                    ]
                    delegate: Rectangle {
                        required property var modelData
                        required property int index
                        width: parent.width - 12; x: 6; height: 36; radius: 4
                        color: root.settingsVm.prefCategory === index ? "#1c2a3e"
                             : (catHov.containsMouse ? "#151c28" : "transparent")
                        border.color: root.settingsVm.prefCategory === index ? "#18c75e" : "transparent"
                        border.width: 1

                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left; anchors.leftMargin: 12; spacing: 10
                            Text { text: modelData.icon; font.pixelSize: 14 }
                            Text { text: modelData.name; color: "#c8d4e0"; font.pixelSize: 12 }
                        }
                        HoverHandler { id: catHov }
                        TapHandler { onTapped: root.settingsVm.setPrefCategory(index) }
                    }
                }
            }
        }

        // Content area
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true; color: "#0d0f12"

            ColumnLayout {
                anchors.fill: parent; anchors.margins: 24; spacing: 16

                Text {
                    text: root.settingsVm.prefCategoryTitle; color: "#e8edf6"
                    font.pixelSize: 16; font.bold: true
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#1e2430" }

                // Appearance settings (shown when index=0)
                ColumnLayout {
                    visible: root.settingsVm.prefCategory === 0
                    Layout.fillWidth: true; spacing: 16

                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("界面主题"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 140 }
                        Repeater {
                            model: [qsTr("深色 (默认)"), qsTr("深蓝"), qsTr("极暗")]
                            delegate: Rectangle {
                                required property var modelData
                                required property int index
                                width: 100; height: 32; radius: 4
                                color: root.settingsVm.themeIndex === index ? "#1c2a3e" : "#1a1e28"
                                border.color: root.settingsVm.themeIndex === index ? "#18c75e" : "#2e3444"; border.width: 1
                                Text { anchors.centerIn: parent; text: modelData; color: root.settingsVm.themeIndex === index ? "#18c75e" : "#8a96a8"; font.pixelSize: 11 }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.settingsVm.setThemeIndex(index)
                                }
                            }
                        }
                    }

                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("字体大小"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 140 }
                        Slider {
                            from: 10; to: 16; stepSize: 1; value: root.settingsVm.fontSize
                            Layout.preferredWidth: 200
                            onMoved: root.settingsVm.setFontSize(Math.round(value))
                        }
                        Text { text: root.settingsVm.fontSize + "px"; color: "#c8d4e0"; font.pixelSize: 11 }
                    }

                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("界面缩放"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 140 }
                        ComboBox {
                            model: ["100%","125%","150%","175%","200%"]
                            currentIndex: root.settingsVm.uiScaleIndex
                            implicitWidth: 120
                            implicitHeight: 28
                            onActivated: root.settingsVm.setUiScaleIndex(currentIndex)
                        }
                    }
                }

                // Language settings (index=1)
                ColumnLayout {
                    visible: root.settingsVm.prefCategory === 1
                    Layout.fillWidth: true; spacing: 8
                    Repeater {
                        model: [qsTr("简体中文"),"English","日本語","한국어","Deutsch","Français"]
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true; height: 40; radius: 4
                            color: root.settingsVm.languageIndex === index ? "#1c2a3e" : "#13181f"
                            border.color: root.settingsVm.languageIndex === index ? "#18c75e" : "#252c38"
                            border.width: 1

                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left; anchors.leftMargin: 16; spacing: 12
                                Text {
                                    text: root.settingsVm.languageIndex === index ? "✓" : " "
                                    color: "#18c75e"; font.pixelSize: 13; font.bold: true
                                }
                                Text {
                                    text: modelData
                                    color: root.settingsVm.languageIndex === index ? "#18c75e" : "#c8d4e0"
                                    font.pixelSize: 13
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.settingsVm.setLanguageIndex(index)
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                // Save/Restore buttons
                RowLayout {
                    spacing: 12
                    Item { Layout.fillWidth: true }

                    // 实时生效提示
                    Text {
                        id: appliedHint
                        text: qsTr("✓ 已实时生效")
                        color: "#18c75e"
                        font.pixelSize: 11
                        opacity: 0
                        Behavior on opacity { NumberAnimation { duration: 300 } }
                    }

                    Button {
                        text: qsTr("恢复默认")
                        implicitHeight: 30; implicitWidth: 90
                        background: Rectangle { radius: 4; color: "#252b38"; border.color: "#363d4e"; border.width: 1 }
                        contentItem: Text { text: parent.text; color: "#c8d4e0"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: root.settingsVm.resetPreferences()
                    }
                    Button {
                        text: qsTr("应用")
                        implicitHeight: 30; implicitWidth: 80
                        background: Rectangle { radius: 4; color: "#18c75e" }
                        contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: {
                            appliedHint.opacity = 1
                            appliedHintTimer.restart()
                        }
                    }

                    Timer {
                        id: appliedHintTimer
                        interval: 2000
                        onTriggered: appliedHint.opacity = 0
                    }
                }
            }
        }
    }
}
