import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// SettingsPage — 3-tier preset tab host (工艺参数 / 耗材参数 / 打印机参数)
Item {
    id: root
    required property var configVm

    readonly property var printCategories: [qsTr("质量"),qsTr("填充"),qsTr("速度"),qsTr("加速度"),qsTr("温度"),qsTr("支撑"),qsTr("底座"),qsTr("冷却"),qsTr("回退"),qsTr("其他")]
    readonly property var filamentCategories: [qsTr("基本"),qsTr("温度"),qsTr("冷却"),qsTr("速度"),qsTr("回退"),qsTr("G-code")]
    readonly property var machineCategories: [qsTr("打印空间"),qsTr("G-code"),qsTr("运动能力"),qsTr("挤出机"),qsTr("多材料"),qsTr("注释")]

    Rectangle { anchors.fill: parent; color: Theme.bgBase }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Tab bar ──
        Rectangle {
            Layout.fillWidth: true
            height: 44
            color: Theme.bgPanel

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingLG
                anchors.rightMargin: Theme.spacingLG
                spacing: 0

                Rectangle {
                    Layout.preferredWidth: 70; Layout.preferredHeight: 28; radius: 4
                    color: backHov.containsMouse ? Theme.bgHover : Theme.bgElevated
                    Text { anchors.centerIn: parent; text: qsTr("← 返回"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM }
                    HoverHandler { id: backHov }
                    TapHandler { onTapped: backend.setCurrentPage(1) }
                }

                Item { Layout.preferredWidth: Theme.spacingLG }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 24

                    Repeater {
                        model: [qsTr("工艺参数"), qsTr("耗材参数"), qsTr("打印机参数")]
                        delegate: Rectangle {
                            required property string modelData
                            required property int index
                            Layout.preferredHeight: 40
                            Layout.fillWidth: true
                            color: "transparent"
                            TapHandler { onTapped: tierBar.currentIndex = index }

                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                font.pixelSize: Theme.fontSizeMD
                                color: tierBar.currentIndex === index ? Theme.accent : Theme.textSecondary
                            }
                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width; height: 2
                                color: tierBar.currentIndex === index ? Theme.accent : "transparent"
                            }
                        }
                    }
                }

                Label {
                    visible: configVm !== null
                    text: {
                        if (!configVm) return ""
                        var tiers = [configVm.currentPrintPreset || configVm.currentPreset,
                                     configVm.currentFilamentPreset, configVm.currentPrinterPreset]
                        return tiers[tierBar.currentIndex] || ""
                    }
                    color: Theme.textTertiary; font.pixelSize: Theme.fontSizeSM
                }
                Label {
                    visible: configVm && configVm.isPresetDirty
                    text: qsTr("已修改")
                    color: Theme.statusWarning; font.pixelSize: Theme.fontSizeXS; font.bold: true
                    leftPadding: Theme.spacingSM; rightPadding: Theme.spacingSM
                    topPadding: 2; bottomPadding: 2
                    background: Rectangle { radius: 3; color: "#3a2e1a" }
                }
                // PRESET-01: 另存为预设按钮（对齐上游 SavePresetDialog 入口）
                CxButton {
                    text: qsTr("另存为...")
                    implicitHeight: 24
                    compact: true
                    enabled: configVm && configVm.isPresetDirty
                    onClicked: savePresetDialog.open()
                }
                // SEARCH-01: 搜索参数按钮（对齐上游 SearchCtrl）
                CxButton {
                    text: qsTr("🔍 搜索")
                    implicitHeight: 24
                    compact: true
                    enabled: !!configVm
                    onClicked: searchDialog.open()
                }
                // V21-02: 导出预设包按钮（对齐上游 ExportPresetBundleDialog）
                CxButton {
                    text: qsTr("导出预设...")
                    implicitHeight: 24
                    compact: true
                    enabled: !!configVm
                    onClicked: exportBundleDialog.open()
                }
            }
        }

        // PRESET-01: SavePresetDialog 实例
        SavePresetDialog {
            id: savePresetDialog
            configVm: root.configVm
            presetTier: {
                var tiers = ["print", "filament", "printer"]
                return tiers[tierBar.currentIndex] || "print"
            }
        }

        // PRESET-02: UnsavedChangesDialog 实例（切换 preset 守卫）
        UnsavedChangesDialog {
            id: unsavedChangesDialog
            configVm: root.configVm
            onAccepted: {
                if (action === "save") savePresetDialog.open()
                else if (action === "discard") { if (root.configVm) root.configVm.resetAllGlobalOptions() }
                // cancel: 不做任何事, 守卫由调用方判断
            }
        }

        // SEARCH-01: SearchDialog 实例（对齐上游 SearchCtrl）
        SearchDialog {
            id: searchDialog
            configVm: root.configVm
            onJumpToOption: function(optionIndex) {
                // 跳转到对应 tier + 滚动到该选项（简化：切换 tier）
                if (root.configVm) {
                    var page = root.configVm.searchResultPage(optionIndex)
                    if (page === "print") tierBar.currentIndex = 0
                    else if (page === "filament") tierBar.currentIndex = 1
                    else if (page === "machine") tierBar.currentIndex = 2
                }
            }
        }

        // V21-02: ExportPresetBundleDialog 实例
        ExportPresetBundleDialog {
            id: exportBundleDialog
            configVm: root.configVm
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSubtle }

        // Tab content
        StackLayout {
            id: tierBar
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: 0
            onCurrentIndexChanged: {
                if (configVm) {
                    var tiers = ["print", "filament", "printer"]
                    configVm.setActivePresetTier(tiers[currentIndex])
                }
            }

            Loader { active: true; source: "qrc:/qml/components/ParamsPage.qml"
                onLoaded: { item.optionModel = configVm ? configVm.printOptions : null; item.configVm = root.configVm; item.categories = root.printCategories } }
            Loader { active: true; source: "qrc:/qml/components/ParamsPage.qml"
                onLoaded: { item.optionModel = configVm ? configVm.filamentOptions : null; item.configVm = root.configVm; item.categories = root.filamentCategories } }
            Loader { active: true; source: "qrc:/qml/components/ParamsPage.qml"
                onLoaded: { item.optionModel = configVm ? configVm.machineOptions : null; item.configVm = root.configVm; item.categories = root.machineCategories } }
        }
    }
}
