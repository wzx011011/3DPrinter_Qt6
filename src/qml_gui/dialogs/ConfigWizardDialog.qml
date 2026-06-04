import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../controls"

// P8.1 -- ConfigWizardDialog: first-run configuration wizard
// Multi-page wizard: Welcome -> Printer -> Filament -> Done
// Usage: ConfigWizardDialog { id: wizard }  ->  wizard.open()
CxDialog {
    id: root

    closePolicy: Popup.CloseOnEscape

    dialogTitle: qsTr("首次配置向导")

    anchors.centerIn: parent
    width: 480
    height: 420

    // Public properties for reading selected values after completion
    property string selectedPrinter: ""
    property string selectedBedType: ""
    property string selectedFilament: ""
    property string selectedNozzle: "0.4"

    signal wizardFinished()

    contentItem: ColumnLayout {
        spacing: 0

        // Page indicator (replaces header page count display)
        RowLayout {
            Layout.fillWidth: true
            Layout.rightMargin: 16
            spacing: 10

            Item { Layout.fillWidth: true }

            Text {
                id: pageIndicatorText
                color: "#7a8fa3"; font.pixelSize: 11
                text: qsTr("第 %1/%2 步").arg(swipeView.currentIndex + 1).arg(swipeView.count)
            }
        }

        SwipeView {
            id: swipeView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 20
            clip: true
            currentIndex: 0
            interactive: false

            // -- Page 0: Welcome --
            Item {
                ColumnLayout {
                    anchors.fill: parent; spacing: 16

                    Item { Layout.fillHeight: true }

                    // Logo
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 72; height: 72; radius: 14
                        color: "#111e1a"
                        border.color: "#22c564"; border.width: 2
                        Text { anchors.centerIn: parent; text: "🖨"; font.pixelSize: 34 }
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("欢迎使用 3D 打印切片软件")
                        color: "#e2e8f5"; font.pixelSize: 18; font.bold: true
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 340
                        text: qsTr("我们将引导您完成基本配置，选择您的打印机和耗材，以获得最佳切片体验。")
                        color: "#7a8fa3"; font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        lineHeight: 1.6
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // -- Page 1: Printer Selection --
            Item {
                ColumnLayout {
                    anchors.fill: parent; spacing: 16

                    Text {
                        text: qsTr("选择打印机")
                        color: "#e2e8f5"; font.pixelSize: 16; font.bold: true
                    }

                    Text {
                        text: qsTr("请选择您使用的打印机型号：")
                        color: "#7a8fa3"; font.pixelSize: 11
                    }

                    // Printer preset combo
                    Rectangle {
                        Layout.fillWidth: true; height: 36; radius: 5
                        color: "#10141c"; border.color: "#2a3040"

                        RowLayout {
                            anchors.fill: parent; anchors.margins: 8; spacing: 8
                            Text { text: "🖨"; font.pixelSize: 14 }
                            CxComboBox {
                                id: printerCombo
                                Layout.fillWidth: true
                                model: [
                                    qsTr("Creality K1C 0.4 nozzle"),
                                    qsTr("K1 Max 0.4 nozzle"),
                                    qsTr("Ender-3 S1 0.4 nozzle"),
                                    qsTr("CR-10 Smart Pro 0.4 nozzle")
                                ]
                                currentIndex: 0
                            }
                        }
                    }

                    // Bed type
                    Text {
                        text: qsTr("热床类型：")
                        color: "#7a8fa3"; font.pixelSize: 11
                    }

                    Rectangle {
                        Layout.fillWidth: true; height: 36; radius: 5
                        color: "#10141c"; border.color: "#2a3040"

                        RowLayout {
                            anchors.fill: parent; anchors.margins: 8; spacing: 8
                            Text { text: "🟦"; font.pixelSize: 14 }
                            CxComboBox {
                                id: bedCombo
                                Layout.fillWidth: true
                                model: [
                                    qsTr("光滑 PEI 板"),
                                    qsTr("普通 PEI 板"),
                                    qsTr("PC 热床"),
                                    qsTr("EP 热床")
                                ]
                                currentIndex: 0
                            }
                        }
                    }

                    // Nozzle diameter display
                    Rectangle {
                        Layout.fillWidth: true; height: 32; radius: 4
                        color: "#111620"
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 10; spacing: 6
                            Text { text: qsTr("喷嘴直径:"); color: "#566070"; font.pixelSize: 11 }
                            Text { text: "0.4mm"; color: "#22c564"; font.pixelSize: 11; font.bold: true }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // -- Page 2: Filament Selection --
            Item {
                ColumnLayout {
                    anchors.fill: parent; spacing: 16

                    Text {
                        text: qsTr("选择耗材")
                        color: "#e2e8f5"; font.pixelSize: 16; font.bold: true
                    }

                    Text {
                        text: qsTr("请选择您常用的耗材类型：")
                        color: "#7a8fa3"; font.pixelSize: 11
                    }

                    // Filament preset combo
                    Rectangle {
                        Layout.fillWidth: true; height: 36; radius: 5
                        color: "#10141c"; border.color: "#2a3040"

                        RowLayout {
                            anchors.fill: parent; anchors.margins: 8; spacing: 8
                            Text { text: "🟡"; font.pixelSize: 14 }
                            CxComboBox {
                                id: filamentCombo
                                Layout.fillWidth: true
                                model: ["PLA", "ABS", "PETG", "TPU", "ASA"]
                                currentIndex: 0
                            }
                        }
                    }

                    // Temperature info card
                    Rectangle {
                        Layout.fillWidth: true; radius: 6; color: "#111620"; border.color: "#1e2535"
                        Layout.preferredHeight: tempInfo.implicitHeight + 24

                        ColumnLayout {
                            id: tempInfo
                            anchors.fill: parent; anchors.margins: 12; spacing: 8

                            RowLayout {
                                spacing: 6
                                Text { text: qsTr("喷嘴温度:"); color: "#566070"; font.pixelSize: 11 }
                                Text {
                                    id: nozzleTempText
                                    color: "#f5a623"; font.pixelSize: 12; font.bold: true
                                    text: {
                                        switch (filamentCombo.currentIndex) {
                                        case 0: return "210°C";  // PLA
                                        case 1: return "240°C";  // ABS
                                        case 2: return "230°C";  // PETG
                                        case 3: return "220°C";  // TPU
                                        case 4: return "250°C";  // ASA
                                        default: return "210°C";
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                spacing: 6
                                Text { text: qsTr("热床温度:"); color: "#566070"; font.pixelSize: 11 }
                                Text {
                                    id: bedTempText
                                    color: "#3b9eff"; font.pixelSize: 12; font.bold: true
                                    text: {
                                        switch (filamentCombo.currentIndex) {
                                        case 0: return "60°C";   // PLA
                                        case 1: return "100°C";  // ABS
                                        case 2: return "80°C";   // PETG
                                        case 3: return "50°C";   // TPU
                                        case 4: return "100°C";  // ASA
                                        default: return "60°C";
                                        }
                                    }
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                color: "#566070"; font.pixelSize: 10
                                wrapMode: Text.WordWrap
                                lineHeight: 1.5
                                text: {
                                    switch (filamentCombo.currentIndex) {
                                    case 0: return qsTr("PLA 是最常用的耗材，易于打印，适合初学者。建议打印时开启风扇冷却。");
                                    case 1: return qsTr("ABS 强度高，耐热性好，适合工程零件。打印时建议关闭风扇，防止翘边。");
                                    case 2: return qsTr("PETG 兼具强度和韧性，透明度可选，适合功能性零件。");
                                    case 3: return qsTr("TPU 是柔性耗材，适合打印弹性零件。建议降低打印速度以获得更好质量。");
                                    case 4: return qsTr("ASA 具有优异的户外耐候性，适合户外使用的零件。打印时建议关闭风扇。");
                                    default: return "";
                                    }
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // -- Page 3: Done --
            Item {
                ColumnLayout {
                    anchors.fill: parent; spacing: 16

                    Item { Layout.fillHeight: true }

                    // Success icon
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 64; height: 64; radius: 32
                        color: "#0e6636"
                        Text { anchors.centerIn: parent; text: "✓"; color: "#18c75e"; font.pixelSize: 32; font.bold: true }
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("设置完成!")
                        color: "#e2e8f5"; font.pixelSize: 18; font.bold: true
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("您的基本配置已保存，可以开始使用了。")
                        color: "#7a8fa3"; font.pixelSize: 12
                    }

                    // Summary card
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 320
                        radius: 6; color: "#111620"; border.color: "#1e2535"
                        Layout.preferredHeight: summaryCol.implicitHeight + 24

                        ColumnLayout {
                            id: summaryCol
                            anchors.fill: parent; anchors.margins: 12; spacing: 8

                            Row {
                                spacing: 8
                                Text { text: qsTr("打印机:"); color: "#566070"; font.pixelSize: 11; width: 70 }
                                Text { text: printerCombo.displayText; color: "#c8d4e0"; font.pixelSize: 11 }
                            }

                            Row {
                                spacing: 8
                                Text { text: qsTr("热床:"); color: "#566070"; font.pixelSize: 11; width: 70 }
                                Text { text: bedCombo.displayText; color: "#c8d4e0"; font.pixelSize: 11 }
                            }

                            Row {
                                spacing: 8
                                Text { text: qsTr("耗材:"); color: "#566070"; font.pixelSize: 11; width: 70 }
                                Text { text: filamentCombo.displayText; color: "#c8d4e0"; font.pixelSize: 11 }
                            }

                            Row {
                                spacing: 8
                                Text { text: qsTr("喷嘴温度:"); color: "#566070"; font.pixelSize: 11; width: 70 }
                                Text { text: nozzleTempText.text; color: "#f5a623"; font.pixelSize: 11 }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }

        // Page indicator dots
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 8
            Layout.bottomMargin: 4
            spacing: 8
            Repeater {
                model: swipeView.count
                Rectangle {
                    width: swipeView.currentIndex === index ? 20 : 8
                    height: 8
                    radius: 4
                    color: swipeView.currentIndex === index ? "#18c75e" : "#2e3848"
                    Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.InOutQuad } }
                    Behavior on color { ColorAnimation { duration: 200 } }
                }
            }
        }

        // Navigation buttons
        Rectangle { Layout.fillWidth: true; height: 1; color: "#1e2535" }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.topMargin: 12
            Layout.bottomMargin: 16
            spacing: 12

            // Back button
            Rectangle {
                visible: swipeView.currentIndex > 0
                Layout.preferredWidth: 80
                Layout.preferredHeight: 30
                radius: 4
                color: backHov.containsMouse ? "#2e3848" : "#242a33"
                Text { anchors.centerIn: parent; text: qsTr("上一步"); color: "#a8b5c8"; font.pixelSize: 12 }
                MouseArea { id: backHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: swipeView.decrementCurrentIndex(); }
            }

            Item { Layout.fillWidth: true }

            // Next / Finish button
            Rectangle {
                Layout.preferredWidth: 100
                Layout.preferredHeight: 30
                radius: 4
                color: nextHov.containsMouse ? "#19a84e" : "#157a39"
                Text {
                    anchors.centerIn: parent
                    text: {
                        if (swipeView.currentIndex === 0) return qsTr("开始设置");
                        if (swipeView.currentIndex === 3) return qsTr("完成");
                        return qsTr("下一步");
                    }
                    color: "white"; font.pixelSize: 12; font.bold: true
                }
                MouseArea {
                    id: nextHov
                    z: 999
                    anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (swipeView.currentIndex === 3) {
                            root.selectedPrinter = printerCombo.currentText;
                            root.selectedBedType = bedCombo.currentText;
                            root.selectedFilament = filamentCombo.currentText;
                            backend.configWizardCompleted = true;
                            root.wizardFinished();
                            root.close();
                        } else {
                            swipeView.incrementCurrentIndex();
                        }
                    }
                }
            }
        }
    }
}
