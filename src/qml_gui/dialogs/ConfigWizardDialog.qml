import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// P8.1 — ConfigWizardDialog: first-run configuration wizard
// Multi-page wizard: Welcome -> Printer -> Filament -> Done
// Usage: ConfigWizardDialog { id: wizard }  ->  wizard.open()
Dialog {
    id: root

    modal: true
    closePolicy: Popup.CloseOnEscape
    anchors.centerIn: parent
    width: 480
    height: 420

    // Public properties for reading selected values after completion
    property string selectedPrinter: ""
    property string selectedBedType: ""
    property string selectedFilament: ""
    property string selectedNozzle: "0.4"

    signal wizardFinished()

    background: Rectangle {
        color: "#1a1f28"; radius: 8
        border.color: "#2e3848"; border.width: 1
    }

    header: Rectangle {
        width: parent.width; height: 44; color: "#141920"; radius: 8
        Rectangle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; height: 12; color: parent.color }

        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 12; spacing: 10
            Text { text: qsTr("首次配置向导"); color: "#e2e8f5"; font.pixelSize: 14; font.bold: true }
            Item { Layout.fillWidth: true }
            Text {
                id: pageIndicatorText
                color: "#7a8fa3"; font.pixelSize: 11
                text: qsTr("第 %1/%2 步").arg(swipeView.currentIndex + 1).arg(swipeView.count)
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: 0

        SwipeView {
            id: swipeView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 20
            clip: true
            currentIndex: 0
            interactive: false

            // ── Page 0: Welcome ──
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
                        Text { anchors.centerIn: parent; text: "\uD83D\uDDA8"; font.pixelSize: 34 }
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

            // ── Page 1: Printer Selection ──
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
                            Text { text: "\uD83D\uDDA8"; font.pixelSize: 14 }
                            ComboBox {
                                id: printerCombo
                                Layout.fillWidth: true
                                model: [
                                    qsTr("Creality K1C 0.4 nozzle"),
                                    qsTr("K1 Max 0.4 nozzle"),
                                    qsTr("Ender-3 S1 0.4 nozzle"),
                                    qsTr("CR-10 Smart Pro 0.4 nozzle")
                                ]
                                currentIndex: 0
                                background: Item {}
                                contentItem: Text {
                                    text: printerCombo.displayText
                                    color: "#c8d4e0"; font.pixelSize: 12
                                    verticalAlignment: Text.AlignVCenter
                                }
                                popup: Popup {
                                    y: printerCombo.height
                                    width: printerCombo.width
                                    implicitHeight: contentColumn.implicitHeight + 8
                                    padding: 4
                                    background: Rectangle { color: "#1a1f28"; radius: 5; border.color: "#2a3040" }
                                    contentItem: Column {
                                        id: contentColumn
                                        Repeater {
                                            model: printerCombo.model
                                            delegate: ItemDelegate {
                                                width: parent ? parent.width : 0
                                                height: 28
                                                contentItem: Text {
                                                    text: modelData
                                                    color: printerCombo.highlightedIndex === index ? "#18c75e" : "#c8d4e0"
                                                    font.pixelSize: 12
                                                }
                                                background: Rectangle {
                                                    color: printerCombo.highlightedIndex === index ? "#1e2a30" : "transparent"
                                                    radius: 3
                                                }
                                                highlighted: printerCombo.highlightedIndex === index
                                                onClicked: {
                                                    printerCombo.currentIndex = index
                                                    printerCombo.popup.close()
                                                }
                                            }
                                        }
                                    }
                                }
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
                            Text { text: "\uD83D\uDFE6"; font.pixelSize: 14 }
                            ComboBox {
                                id: bedCombo
                                Layout.fillWidth: true
                                model: [
                                    qsTr("\u5149\u6ED1 PEI \u677F"),
                                    qsTr("\u666E\u901A PEI \u677F"),
                                    qsTr("PC \u70ED\u5E8A"),
                                    qsTr("EP \u70ED\u5E8A")
                                ]
                                currentIndex: 0
                                background: Item {}
                                contentItem: Text {
                                    text: bedCombo.displayText
                                    color: "#c8d4e0"; font.pixelSize: 12
                                    verticalAlignment: Text.AlignVCenter
                                }
                                popup: Popup {
                                    y: bedCombo.height
                                    width: bedCombo.width
                                    implicitHeight: bedContentCol.implicitHeight + 8
                                    padding: 4
                                    background: Rectangle { color: "#1a1f28"; radius: 5; border.color: "#2a3040" }
                                    contentItem: Column {
                                        id: bedContentCol
                                        Repeater {
                                            model: bedCombo.model
                                            delegate: ItemDelegate {
                                                width: parent ? parent.width : 0
                                                height: 28
                                                contentItem: Text {
                                                    text: modelData
                                                    color: bedCombo.highlightedIndex === index ? "#18c75e" : "#c8d4e0"
                                                    font.pixelSize: 12
                                                }
                                                background: Rectangle {
                                                    color: bedCombo.highlightedIndex === index ? "#1e2a30" : "transparent"
                                                    radius: 3
                                                }
                                                highlighted: bedCombo.highlightedIndex === index
                                                onClicked: {
                                                    bedCombo.currentIndex = index
                                                    bedCombo.popup.close()
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Nozzle diameter display
                    Rectangle {
                        Layout.fillWidth: true; height: 32; radius: 4
                        color: "#111620"
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 10; spacing: 6
                            Text { text: qsTr("\u55B7\u5634\u76F4\u5F84:"); color: "#566070"; font.pixelSize: 11 }
                            Text { text: "0.4mm"; color: "#22c564"; font.pixelSize: 11; font.bold: true }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // ── Page 2: Filament Selection ──
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
                            Text { text: "\uD83D\uDFE1"; font.pixelSize: 14 }
                            ComboBox {
                                id: filamentCombo
                                Layout.fillWidth: true
                                model: ["PLA", "ABS", "PETG", "TPU", "ASA"]
                                currentIndex: 0
                                background: Item {}
                                contentItem: Text {
                                    text: filamentCombo.displayText
                                    color: "#c8d4e0"; font.pixelSize: 12
                                    verticalAlignment: Text.AlignVCenter
                                }
                                popup: Popup {
                                    y: filamentCombo.height
                                    width: filamentCombo.width
                                    implicitHeight: filContentCol.implicitHeight + 8
                                    padding: 4
                                    background: Rectangle { color: "#1a1f28"; radius: 5; border.color: "#2a3040" }
                                    contentItem: Column {
                                        id: filContentCol
                                        Repeater {
                                            model: filamentCombo.model
                                            delegate: ItemDelegate {
                                                width: parent ? parent.width : 0
                                                height: 28
                                                contentItem: Text {
                                                    text: modelData
                                                    color: filamentCombo.highlightedIndex === index ? "#18c75e" : "#c8d4e0"
                                                    font.pixelSize: 12
                                                }
                                                background: Rectangle {
                                                    color: filamentCombo.highlightedIndex === index ? "#1e2a30" : "transparent"
                                                    radius: 3
                                                }
                                                highlighted: filamentCombo.highlightedIndex === index
                                                onClicked: {
                                                    filamentCombo.currentIndex = index
                                                    filamentCombo.popup.close()
                                                }
                                            }
                                        }
                                    }
                                }
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
                                Text { text: qsTr("\u55B7\u5634\u6E29\u5EA6:"); color: "#566070"; font.pixelSize: 11 }
                                Text {
                                    id: nozzleTempText
                                    color: "#f5a623"; font.pixelSize: 12; font.bold: true
                                    text: {
                                        switch (filamentCombo.currentIndex) {
                                        case 0: return "210\u00B0C";  // PLA
                                        case 1: return "240\u00B0C";  // ABS
                                        case 2: return "230\u00B0C";  // PETG
                                        case 3: return "220\u00B0C";  // TPU
                                        case 4: return "250\u00B0C";  // ASA
                                        default: return "210\u00B0C";
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                spacing: 6
                                Text { text: qsTr("\u70ED\u5E8A\u6E29\u5EA6:"); color: "#566070"; font.pixelSize: 11 }
                                Text {
                                    id: bedTempText
                                    color: "#3b9eff"; font.pixelSize: 12; font.bold: true
                                    text: {
                                        switch (filamentCombo.currentIndex) {
                                        case 0: return "60\u00B0C";   // PLA
                                        case 1: return "100\u00B0C";  // ABS
                                        case 2: return "80\u00B0C";   // PETG
                                        case 3: return "50\u00B0C";   // TPU
                                        case 4: return "100\u00B0C";  // ASA
                                        default: return "60\u00B0C";
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
                                    case 0: return qsTr("PLA \u662F\u6700\u5E38\u7528\u7684\u8017\u6750\uFF0C\u6613\u4E8E\u6253\u5370\uFF0C\u9002\u5408\u521D\u5B66\u8005\u3002\u5EFA\u8BAE\u6253\u5370\u65F6\u5F00\u542F\u98CE\u6247\u51B7\u5374\u3002");
                                    case 1: return qsTr("ABS \u5F3A\u5EA6\u9AD8\uFF0C\u8010\u70ED\u6027\u597D\uFF0C\u9002\u5408\u5DE5\u7A0B\u96F6\u4EF6\u3002\u6253\u5370\u65F6\u5EFA\u8BAE\u5173\u95ED\u98CE\u6247\uFF0C\u9632\u6B62\u7FD8\u8FB9\u3002");
                                    case 2: return qsTr("PETG \u517C\u5177\u5F3A\u5EA6\u548C\u97E7\u6027\uFF0C\u900F\u660E\u5EA6\u53EF\u9009\uFF0C\u9002\u5408\u529F\u80FD\u6027\u96F6\u4EF6\u3002");
                                    case 3: return qsTr("TPU \u662F\u67D4\u6027\u8017\u6750\uFF0C\u9002\u5408\u6253\u5370\u5F39\u6027\u96F6\u4EF6\u3002\u5EFA\u8BAE\u964D\u4F4E\u6253\u5370\u901F\u5EA6\u4EE5\u83B7\u5F97\u66F4\u597D\u8D28\u91CF\u3002");
                                    case 4: return qsTr("ASA \u5177\u6709\u4F18\u5F02\u7684\u6237\u5916\u8010\u5019\u6027\uFF0C\u9002\u5408\u6237\u5916\u4F7F\u7528\u7684\u96F6\u4EF6\u3002\u6253\u5370\u65F6\u5EFA\u8BAE\u5173\u95ED\u98CE\u6247\u3002");
                                    default: return "";
                                    }
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // ── Page 3: Done ──
            Item {
                ColumnLayout {
                    anchors.fill: parent; spacing: 16

                    Item { Layout.fillHeight: true }

                    // Success icon
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 64; height: 64; radius: 32
                        color: "#0e6636"
                        Text { anchors.centerIn: parent; text: "\u2713"; color: "#18c75e"; font.pixelSize: 32; font.bold: true }
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("\u8BBE\u7F6E\u5B8C\u6210!")
                        color: "#e2e8f5"; font.pixelSize: 18; font.bold: true
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("\u60A8\u7684\u57FA\u672C\u914D\u7F6E\u5DF2\u4FDD\u5B58\uFF0C\u53EF\u4EE5\u5F00\u59CB\u4F7F\u7528\u4E86\u3002")
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
                                Text { text: qsTr("\u6253\u5370\u673A:"); color: "#566070"; font.pixelSize: 11; width: 70 }
                                Text { text: printerCombo.displayText; color: "#c8d4e0"; font.pixelSize: 11 }
                            }

                            Row {
                                spacing: 8
                                Text { text: qsTr("\u70ED\u5E8A:"); color: "#566070"; font.pixelSize: 11; width: 70 }
                                Text { text: bedCombo.displayText; color: "#c8d4e0"; font.pixelSize: 11 }
                            }

                            Row {
                                spacing: 8
                                Text { text: qsTr("\u8017\u6750:"); color: "#566070"; font.pixelSize: 11; width: 70 }
                                Text { text: filamentCombo.displayText; color: "#c8d4e0"; font.pixelSize: 11 }
                            }

                            Row {
                                spacing: 8
                                Text { text: qsTr("\u55B7\u5634\u6E29\u5EA6:"); color: "#566070"; font.pixelSize: 11; width: 70 }
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
                Text { anchors.centerIn: parent; text: qsTr("\u4E0A\u4E00\u6B65"); color: "#a8b5c8"; font.pixelSize: 12 }
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
                        if (swipeView.currentIndex === 0) return qsTr("\u5F00\u59CB\u8BBE\u7F6E");
                        if (swipeView.currentIndex === 3) return qsTr("\u5B8C\u6210");
                        return qsTr("\u4E0B\u4E00\u6B65");
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
