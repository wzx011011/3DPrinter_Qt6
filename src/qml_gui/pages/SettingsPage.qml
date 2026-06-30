import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// SettingsPage - 3-tier preset tab host
Item {
    id: root
    required property var configVm

    function requestBackToPrepare() {
        backend.setCurrentPage(1)
    }

    readonly property var printCategories: [qsTr("Quality"), qsTr("Strength"), qsTr("Speed"), qsTr("Support"), qsTr("Filament"), qsTr("Cooling"), qsTr("Infill"), qsTr("Skirt"), qsTr("Advanced"), qsTr("G-code")]
    readonly property var filamentCategories: [qsTr("Filament"), qsTr("Temperature"), qsTr("Cooling"), qsTr("Retraction"), qsTr("Advanced"), qsTr("G-code")]
    readonly property var machineCategories: [qsTr("Machine"), qsTr("G-code"), qsTr("Motion"), qsTr("Extruder"), qsTr("Cooling"), qsTr("General")]

    Rectangle { anchors.fill: parent; color: Theme.bgBase }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Tab bar
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
                    Text { anchors.centerIn: parent; text: qsTr("Back"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM }
                    HoverHandler { id: backHov }
                    TapHandler { onTapped: root.requestBackToPrepare() }
                }

                Item { Layout.preferredWidth: Theme.spacingLG }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 24

                    Repeater {
                        model: [qsTr("Print"), qsTr("Filament"), qsTr("Printer")]
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
                    text: qsTr("Modified")
                    color: Theme.statusWarning; font.pixelSize: Theme.fontSizeXS; font.bold: true
                    leftPadding: Theme.spacingSM; rightPadding: Theme.spacingSM
                    topPadding: 2; bottomPadding: 2
                    background: Rectangle { radius: 3; color: "#3a2e1a" }
                }
                // PRESET-01: save preset entry
                CxButton {
                    text: qsTr("Save As...")
                    implicitHeight: 24
                    compact: true
                    enabled: configVm && configVm.isPresetDirty
                    onClicked: savePresetDialog.open()
                }
                // SEARCH-01: search dialog entry
                CxButton {
                    text: qsTr("Search")
                    implicitHeight: 24
                    compact: true
                    enabled: !!configVm
                    onClicked: searchDialog.open()
                }
                // V21-02: export preset bundle dialog
                CxButton {
                    text: qsTr("Export...")
                    implicitHeight: 24
                    compact: true
                    enabled: !!configVm
                    onClicked: exportBundleDialog.open()
                }
            }
        }

        // PRESET-01: save preset dialog
        SavePresetDialog {
            id: savePresetDialog
            configVm: root.configVm
            presetTier: {
                var tiers = ["print", "filament", "printer"]
                return tiers[tierBar.currentIndex] || "print"
            }
        }

        // PRESET-02: unsaved changes dialog
        UnsavedChangesDialog {
            id: unsavedChangesDialog
            configVm: root.configVm
            onAccepted: {
                if (!root.configVm)
                    return
                if (action === "save") {
                    if (!root.configVm.requestSavePendingChanges())
                        savePresetDialog.open()
                } else if (action === "discard") {
                    root.configVm.requestDiscardPendingChanges()
                } else {
                    root.configVm.requestCancelPendingChanges()
                }
            }
            onRejected: if (root.configVm) root.configVm.requestCancelPendingChanges()
        }

        // SEARCH-01: search dialog
        SearchDialog {
            id: searchDialog
            configVm: root.configVm
            onJumpToOption: function(optionIndex) {
                if (root.configVm) {
                    var page = root.configVm.searchResultPage(optionIndex)
                    if (page === "print") tierBar.currentIndex = 0
                    else if (page === "filament") tierBar.currentIndex = 1
                    else if (page === "machine") tierBar.currentIndex = 2
                }
            }
        }

        // V21-02: export bundle dialog
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
