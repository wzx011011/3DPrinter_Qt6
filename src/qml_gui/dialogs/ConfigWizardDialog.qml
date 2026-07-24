import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P8.1 / Phase 200 (WIZ-02) -- ConfigWizardDialog: first-run configuration
// wizard. Single-vendor wizard driven by PresetServiceMock enumeration.
//
// Multi-page wizard: Welcome -> Printer -> Filament -> Done
// Printer / filament / bed lists come from backend.presetServiceMock
// (Phase 199 WIZ-01), replacing the prior hard-coded mock. This wizard is
// single-vendor by design: it picks the first vendor returned by
// vendors() and enumerates only that vendor's printer models and
// materials. Multi-vendor selection + PresetUpdater is Deferred.
//
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

    // Phase 200 (WIZ-02): single-vendor wizard data bindings.
    // Lazily resolve the preset service; null-safe (typeof guard matches the
    // PreparePage.qml convention) so the dialog still renders in designer /
    // contexts without a backend context property.
    readonly property var presetSvc: typeof backend !== "undefined" && backend
        ? backend.presetServiceMock : null
    readonly property var vendorList: presetSvc ? presetSvc.vendors() : []
    // Single vendor: the first one returned. Empty when no presets loaded.
    readonly property string activeVendor: vendorList.length > 0 ? vendorList[0] : ""
    readonly property var printerModelList: presetSvc && activeVendor.length > 0
        ? presetSvc.printerModelsForVendor(activeVendor) : []
    readonly property var materialList: presetSvc && activeVendor.length > 0
        ? presetSvc.materialsForVendor(activeVendor) : []
    readonly property var bedTypeList: presetSvc
        ? presetSvc.defaultBedTypes() : []

    contentItem: ColumnLayout {
        spacing: Theme.spacingXS
        // Page indicator (replaces header page count display)
        RowLayout {
            Layout.fillWidth: true
            Layout.rightMargin: Theme.spacingXL
            spacing: Theme.spacingMD
            Item { Layout.fillWidth: true }

            Text {
                id: pageIndicatorText
                color: Theme.textTertiary; font.pixelSize: Theme.fontSizeSM
                text: qsTr("第 %1/%2 步").arg(swipeView.currentIndex + 1).arg(swipeView.count)
            }
        }

        SwipeView {
            id: swipeView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.spacingXXL
            clip: true
            currentIndex: 0
            interactive: false

            // -- Page 0: Welcome --
            Item {
                ColumnLayout {
                    anchors.fill: parent; spacing: Theme.spacingXL
                    Item { Layout.fillHeight: true }

                    // Logo
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 72; height: 72; radius: 14
                        color: Theme.chromeSurface
                        border.color: Theme.accent; border.width: 2
                        Text { anchors.centerIn: parent; text: "🖨"; font.pixelSize: 34 }
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("欢迎使用 3D 打印切片软件")
                        color: Theme.textPrimary; font.pixelSize: 18; font.bold: true
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 340
                        text: qsTr("我们将引导您完成基本配置，选择您的打印机和耗材，以获得最佳切片体验。")
                        color: Theme.textTertiary; font.pixelSize: Theme.fontSizeMD
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
                    anchors.fill: parent; spacing: Theme.spacingXL
                    Text {
                        text: qsTr("选择打印机")
                        color: Theme.textPrimary; font.pixelSize: Theme.fontSizeXL; font.bold: true
                    }

                    Text {
                        text: activeVendor.length > 0
                            ? qsTr("请选择您使用的 %1 打印机型号：").arg(activeVendor)
                            : qsTr("请选择您使用的打印机型号：")
                        color: Theme.textTertiary; font.pixelSize: Theme.fontSizeSM
                        visible: printerModelList.length > 0
                    }

                    // Printer preset combo (Phase 200: dynamic from enumeration)
                    Rectangle {
                        Layout.fillWidth: true; height: 36; radius: 5
                        color: Theme.bgInset; border.color: Theme.switchTrackOff
                        visible: printerModelList.length > 0

                        RowLayout {
                            anchors.fill: parent; anchors.margins: 8; spacing: Theme.spacingMD
                            Text { text: "🖨"; font.pixelSize: Theme.fontSizeLG }
                            CxComboBox {
                                id: printerCombo
                                Layout.fillWidth: true
                                model: printerModelList
                                currentIndex: 0
                            }
                        }
                    }

                    // Empty-state hint when no printer preset is available
                    Text {
                        Layout.fillWidth: true
                        visible: printerModelList.length === 0
                        wrapMode: Text.WordWrap
                        color: Theme.statusWarning; font.pixelSize: Theme.fontSizeSM
                        text: qsTr("未发现打印机预设，请稍后在设置中导入厂商配置。")
                    }

                    // Bed type
                    Text {
                        text: qsTr("热床类型：")
                        color: Theme.textTertiary; font.pixelSize: Theme.fontSizeSM
                        visible: bedTypeList.length > 0
                    }

                    Rectangle {
                        Layout.fillWidth: true; height: 36; radius: 5
                        color: Theme.bgInset; border.color: Theme.switchTrackOff
                        visible: bedTypeList.length > 0

                        RowLayout {
                            anchors.fill: parent; anchors.margins: 8; spacing: Theme.spacingMD
                            Text { text: "🟦"; font.pixelSize: Theme.fontSizeLG }
                            CxComboBox {
                                id: bedCombo
                                Layout.fillWidth: true
                                model: bedTypeList
                                currentIndex: 0
                            }
                        }
                    }

                    // Nozzle diameter display
                    Rectangle {
                        Layout.fillWidth: true; height: 32; radius: 4
                        color: Theme.bgSurface
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 10; spacing: Theme.spacingSM
                            Text { text: qsTr("喷嘴直径:"); color: Theme.textDisabled; font.pixelSize: Theme.fontSizeSM }
                            Text { text: "0.4mm"; color: Theme.accent; font.pixelSize: Theme.fontSizeSM; font.bold: true }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // -- Page 2: Filament Selection --
            Item {
                ColumnLayout {
                    anchors.fill: parent; spacing: Theme.spacingXL
                    Text {
                        text: qsTr("选择耗材")
                        color: Theme.textPrimary; font.pixelSize: Theme.fontSizeXL; font.bold: true
                    }

                    Text {
                        text: qsTr("请选择您常用的耗材类型：")
                        color: Theme.textTertiary; font.pixelSize: Theme.fontSizeSM
                        visible: materialList.length > 0
                    }

                    // Filament preset combo (Phase 200: dynamic from enumeration)
                    Rectangle {
                        Layout.fillWidth: true; height: 36; radius: 5
                        color: Theme.bgInset; border.color: Theme.switchTrackOff
                        visible: materialList.length > 0

                        RowLayout {
                            anchors.fill: parent; anchors.margins: 8; spacing: Theme.spacingMD
                            Text { text: "🟡"; font.pixelSize: Theme.fontSizeLG }
                            CxComboBox {
                                id: filamentCombo
                                Layout.fillWidth: true
                                model: materialList
                                currentIndex: 0
                            }
                        }
                    }

                    // Empty-state hint when no material preset is available
                    Text {
                        Layout.fillWidth: true
                        visible: materialList.length === 0
                        wrapMode: Text.WordWrap
                        color: Theme.statusWarning; font.pixelSize: Theme.fontSizeSM
                        text: qsTr("未发现耗材预设，请稍后在设置中导入厂商配置。")
                    }

                    // Temperature info card. Preset names are now the source
                    // of truth; temperatures come from the selected preset's
                    // stored values (nozzle_temp / bed_temp) instead of the
                    // prior switch-case mock.
                    Rectangle {
                        Layout.fillWidth: true; radius: 6; color: Theme.bgSurface; border.color: Theme.bgCard
                        Layout.preferredHeight: tempInfo.implicitHeight + 24
                        visible: materialList.length > 0

                        ColumnLayout {
                            id: tempInfo
                            anchors.fill: parent; anchors.margins: 12; spacing: Theme.spacingMD
                            RowLayout {
                                spacing: Theme.spacingSM
                                Text { text: qsTr("喷嘴温度:"); color: Theme.textDisabled; font.pixelSize: Theme.fontSizeSM }
                                Text {
                                    color: Theme.statusWarning; font.pixelSize: Theme.fontSizeMD; font.bold: true
                                    text: {
                                        var t = currentNozzleTemp();
                                        return t >= 0 ? (t + "°C") : "--";
                                    }
                                }
                            }

                            RowLayout {
                                spacing: Theme.spacingSM
                                Text { text: qsTr("热床温度:"); color: Theme.textDisabled; font.pixelSize: Theme.fontSizeSM }
                                Text {
                                    color: Theme.statusInfo; font.pixelSize: Theme.fontSizeMD; font.bold: true
                                    text: {
                                        var t = currentBedTemp();
                                        return t >= 0 ? (t + "°C") : "--";
                                    }
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS
                                wrapMode: Text.WordWrap
                                lineHeight: 1.5
                                text: currentFilamentDescription()
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // -- Page 3: Done --
            Item {
                ColumnLayout {
                    anchors.fill: parent; spacing: Theme.spacingXL
                    Item { Layout.fillHeight: true }

                    // Success icon
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 64; height: 64; radius: 32
                        color: Theme.accentSubtle
                        Text { anchors.centerIn: parent; text: "✓"; color: Theme.accent; font.pixelSize: 32; font.bold: true }
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("设置完成!")
                        color: Theme.textPrimary; font.pixelSize: 18; font.bold: true
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("您的基本配置已保存，可以开始使用了。")
                        color: Theme.textTertiary; font.pixelSize: Theme.fontSizeMD
                    }

                    // Summary card
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 320
                        radius: 6; color: Theme.bgSurface; border.color: Theme.bgCard
                        Layout.preferredHeight: summaryCol.implicitHeight + 24

                        ColumnLayout {
                            id: summaryCol
                            anchors.fill: parent; anchors.margins: 12; spacing: Theme.spacingMD
                            Row {
                                spacing: Theme.spacingMD
                                visible: printerModelList.length > 0
                                Text { text: qsTr("打印机:"); color: Theme.textDisabled; font.pixelSize: Theme.fontSizeSM; width: 70 }
                                Text { text: printerCombo.displayText; color: Theme.chromeText; font.pixelSize: Theme.fontSizeSM }
                            }

                            Row {
                                spacing: Theme.spacingMD
                                visible: bedTypeList.length > 0
                                Text { text: qsTr("热床:"); color: Theme.textDisabled; font.pixelSize: Theme.fontSizeSM; width: 70 }
                                Text { text: bedCombo.displayText; color: Theme.chromeText; font.pixelSize: Theme.fontSizeSM }
                            }

                            Row {
                                spacing: Theme.spacingMD
                                visible: materialList.length > 0
                                Text { text: qsTr("耗材:"); color: Theme.textDisabled; font.pixelSize: Theme.fontSizeSM; width: 70 }
                                Text { text: filamentCombo.displayText; color: Theme.chromeText; font.pixelSize: Theme.fontSizeSM }
                            }

                            Row {
                                spacing: Theme.spacingMD
                                visible: materialList.length > 0
                                Text { text: qsTr("喷嘴温度:"); color: Theme.textDisabled; font.pixelSize: Theme.fontSizeSM; width: 70 }
                                Text {
                                    color: Theme.statusWarning; font.pixelSize: Theme.fontSizeSM
                                    text: {
                                        var t = currentNozzleTemp();
                                        return t >= 0 ? (t + "°C") : "--";
                                    }
                                }
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
            Layout.topMargin: Theme.spacingMD
            Layout.bottomMargin: Theme.spacingXS
            spacing: Theme.spacingMD
            Repeater {
                model: swipeView.count
                Rectangle {
                    width: swipeView.currentIndex === index ? 20 : 8
                    height: 8
                    radius: 4
                    color: swipeView.currentIndex === index ? Theme.accent : Theme.borderInput
                    Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.InOutQuad } }
                    Behavior on color { ColorAnimation { duration: 200 } }
                }
            }
        }

        // Navigation buttons
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.bgCard }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingXXL
            Layout.rightMargin: Theme.spacingXXL
            Layout.topMargin: Theme.spacingLG
            Layout.bottomMargin: Theme.spacingXL
            spacing: Theme.spacingLG
            // Back button
            Rectangle {
                visible: swipeView.currentIndex > 0
                Layout.preferredWidth: 80
                Layout.preferredHeight: 30
                radius: 4
                color: backHov.containsMouse ? Theme.borderInput : Theme.bgCard
                Text { anchors.centerIn: parent; text: qsTr("上一步"); color: Theme.chromeTextMuted; font.pixelSize: Theme.fontSizeMD }
                MouseArea { id: backHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: swipeView.decrementCurrentIndex(); }
            }

            Item { Layout.fillWidth: true }

            // Next / Finish button
            Rectangle {
                Layout.preferredWidth: 100
                Layout.preferredHeight: 30
                radius: 4
                color: nextHov.containsMouse ? Theme.accentDark : Theme.accentSubtle
                Text {
                    anchors.centerIn: parent
                    text: {
                        if (swipeView.currentIndex === 0) return qsTr("开始设置");
                        if (swipeView.currentIndex === 3) return qsTr("完成");
                        return qsTr("下一步");
                    }
                    color: "white"; font.pixelSize: Theme.fontSizeMD; font.bold: true
                }
                MouseArea {
                    id: nextHov
                    z: 999
                    anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (swipeView.currentIndex === 3) {
                            // Phase 200 (WIZ-02): close the loop. Persist the
                            // wizard-completed flag via the WRITE setter
                            // (BackendContext::setConfigWizardCompleted),
                            // which stores "wizard/completed" in QSettings
                            // and emits configWizardCompletedChanged so
                            // main.qml won't re-open the wizard on restart.
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

    // Phase 200 (WIZ-02): temperature helpers backed by the live preset
    // store. Reads nozzle_temp / bed_temp from the currently selected
    // material preset; returns -1 when unknown so the UI shows "--".
    function currentNozzleTemp() {
        if (!presetSvc || filamentCombo.currentText.length === 0)
            return -1;
        var v = presetSvc.presetValue(filamentCombo.currentText, "nozzle_temp");
        var n = parseFloat(v);
        return isNaN(n) ? -1 : Math.round(n);
    }

    function currentBedTemp() {
        if (!presetSvc || filamentCombo.currentText.length === 0)
            return -1;
        var v = presetSvc.presetValue(filamentCombo.currentText, "bed_temp");
        var n = parseFloat(v);
        return isNaN(n) ? -1 : Math.round(n);
    }

    // Short human-readable hint derived from the material preset name.
    // Keeps the wizard self-explanatory without re-introducing the prior
    // hard-coded per-material switch-case.
    function currentFilamentDescription() {
        var name = filamentCombo.currentText;
        if (name.length === 0)
            return "";
        if (name.indexOf("PLA") >= 0)
            return qsTr("PLA 易于打印，适合初学者。建议打印时开启风扇冷却。");
        if (name.indexOf("ABS") >= 0)
            return qsTr("ABS 强度高、耐热性好，适合工程零件。打印时建议关闭风扇以防翘边。");
        if (name.indexOf("PETG") >= 0)
            return qsTr("PETG 兼具强度与韧性，适合功能性零件。");
        if (name.indexOf("TPU") >= 0)
            return qsTr("TPU 为柔性耗材，适合弹性零件。建议降低打印速度以获得更好质量。");
        if (name.indexOf("ASA") >= 0)
            return qsTr("ASA 具有优异的户外耐候性，适合户外零件。打印时建议关闭风扇。");
        return qsTr("当前耗材：%1。").arg(name);
    }
}
