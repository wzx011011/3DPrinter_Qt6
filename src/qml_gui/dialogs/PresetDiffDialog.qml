import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// PresetDiffDialog.qml — Phase 154 (CLOS-01) Preset Side-by-Side Diff
//
// Upstream: third_party/OrcaSlicer/src/slic3r/GUI/UnsavedChangesDialog.cpp
//   - "Compare presets" / diff view mode renders a 3-column key/valueA/valueB
//     table with added/removed/changed classification badges.
//
// OWzx implementation (Phase 154 minimal source-truth port):
//   - Non-modal CxDialog
//   - Two CxComboBox selectors (A, B) populated from the active scope's preset
//     list (printerPresetNames / filamentPresetNames / printPresetNames).
//   - Scope selector to choose which preset category to diff (mirrors
//     upstream Preset::Type radio group, 0=printer / 1=filament / 2=print).
//   - ListView renders the {key, valueA, valueB, status} entries produced by
//     ConfigViewModel.comparePresetsDetailed(A, B) (proxy to the Phase 149
//     PresetServiceMock::comparePresets primitive).
//   - Status badges: added (green), removed (red), changed (amber).
//   - Empty-state "No differences" placeholder when A == B or the diff is empty.
//
// The dialog only renders what the existing C++ primitive produces — no new
// product behavior, no fabricated rows.
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: false
    title: qsTr("Compare Presets")
    width: 720
    height: 520
    padding: 0

    property var configVm: null
    // Scope enum mirrors upstream Preset::Type: 0=printer, 1=filament, 2=print.
    property int selectedScope: 2

    // Local cache of the last computed diff so the ListView has a stable model.
    property var diffRows: []
    property string presetAName: ""
    property string presetBName: ""

    onOpened: {
        scopeCombo.currentIndex = root.selectedScope
        root.refreshPresetList()
        root.diffRows = []
    }

    // Pull the active scope's preset list and seed the two selectors with the
    // first two distinct entries (or fall back to identical when only one
    // preset exists).
    function refreshPresetList() {
        if (!root.configVm) {
            presetACombo.model = [qsTr("(none)")]
            presetBCombo.model = [qsTr("(none)")]
            return
        }
        var names = []
        if (root.selectedScope === 0) names = root.configVm.printerPresetNames || []
        else if (root.selectedScope === 1) names = root.configVm.filamentPresetNames || []
        else names = root.configVm.printPresetNames || []

        if (!names || names.length === 0) names = [qsTr("(none)")]

        presetACombo.model = names
        presetBCombo.model = names
        presetACombo.currentIndex = 0
        presetBCombo.currentIndex = names.length > 1 ? 1 : 0
        root.presetAName = names[0] || ""
        root.presetBName = names[Math.min(1, names.length - 1)] || ""
    }

    // Run the diff: calls ConfigViewModel.comparePresetsDetailed(A, B) which
    // proxies to the Phase 149 PresetServiceMock primitive.
    function runDiff() {
        if (!root.configVm || root.presetAName.length === 0 || root.presetBName.length === 0) {
            root.diffRows = []
            return
        }
        // Upstream behavior: comparing a preset to itself yields an empty diff
        // (the primitive returns [] because no keys differ). We rely on the
        // primitive's classification — no fabrication here.
        root.diffRows = root.configVm.comparePresetsDetailed(root.presetAName, root.presetBName) || []
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        // Header row: scope + preset A + preset B + Compare button
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: qsTr("Scope:")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSizeSM
            }
            CxComboBox {
                id: scopeCombo
                Layout.preferredWidth: 120
                model: [qsTr("Printer"), qsTr("Material"), qsTr("Process")]
                onActivated: {
                    root.selectedScope = currentIndex
                    root.refreshPresetList()
                    root.diffRows = []
                }
            }

            Item { Layout.preferredWidth: 12 } // spacer

            Text {
                text: qsTr("A:")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSizeSM
            }
            CxComboBox {
                id: presetACombo
                Layout.preferredWidth: 200
                onActivated: {
                    if (currentIndex >= 0 && currentIndex < presetACombo.model.length)
                        root.presetAName = presetACombo.model[currentIndex]
                }
            }

            Text {
                text: qsTr("B:")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSizeSM
            }
            CxComboBox {
                id: presetBCombo
                Layout.preferredWidth: 200
                onActivated: {
                    if (currentIndex >= 0 && currentIndex < presetBCombo.model.length)
                        root.presetBName = presetBCombo.model[currentIndex]
                }
            }

            Item { Layout.fillWidth: true } // spacer

            CxButton {
                text: qsTr("Compare")
                enabled: root.presetAName.length > 0 && root.presetBName.length > 0
                         && root.presetAName !== qsTr("(none)")
                         && root.presetBName !== qsTr("(none)")
                onClicked: root.runDiff()
            }
            CxButton {
                text: qsTr("Close")
                onClicked: root.reject()
            }
        }

        // Column header for the 3-column diff table
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            color: Theme.chromeSurface
            radius: 3

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 0

                Text {
                    Layout.preferredWidth: 240
                    text: qsTr("Key")
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSizeXS
                    font.bold: true
                }
                Text {
                    Layout.fillWidth: true
                    text: root.presetAName.length > 0 ? (qsTr("A: ") + root.presetAName) : qsTr("A")
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSizeXS
                    font.bold: true
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    text: root.presetBName.length > 0 ? (qsTr("B: ") + root.presetBName) : qsTr("B")
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSizeXS
                    font.bold: true
                    elide: Text.ElideRight
                }
            }
        }

        // Diff list
        ListView {
            id: diffList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.diffRows
            interactive: true

            // Empty-state placeholder (no differences / not yet compared).
            Text {
                anchors.centerIn: parent
                visible: diffList.count === 0
                text: root.presetAName.length === 0 && root.presetBName.length === 0
                      ? qsTr("Select two presets and click Compare")
                      : qsTr("No differences")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSizeMD
            }

            delegate: Rectangle {
                required property var modelData
                required property int index

                width: diffList.width
                height: 28
                color: index % 2 === 0 ? Theme.bgElevated : Theme.bgBase

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 0

                    // Key column
                    Text {
                        Layout.preferredWidth: 240
                        text: modelData.key || ""
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                        elide: Text.ElideRight
                    }
                    // Value A column
                    Text {
                        Layout.fillWidth: true
                        text: modelData.valueA || ""
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                        elide: Text.ElideRight
                    }
                    // Value B column
                    Text {
                        Layout.fillWidth: true
                        text: modelData.valueB || ""
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                        elide: Text.ElideRight
                    }
                    // Status badge (added/removed/changed).
                    Rectangle {
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: 18
                        radius: 9
                        color: modelData.status === "added"   ? Theme.accentDark
                             : modelData.status === "removed" ? Theme.statusErrorDark
                             : modelData.status === "changed" ? Theme.statusWarning
                                                              : Theme.bgPanel
                        Text {
                            anchors.centerIn: parent
                            text: modelData.status || ""
                            color: "white"
                            font.pixelSize: Theme.fontSizeXS
                            font.bold: true
                        }
                    }
                }
            }
        }

        // Footer: row count
        Text {
            Layout.fillWidth: true
            Layout.preferredHeight: 18
            text: diffList.count === 0
                  ? ""
                  : qsTr("%1 difference(s)").arg(diffList.count)
            color: Theme.textMuted
            font.pixelSize: Theme.fontSizeXS
            horizontalAlignment: Text.AlignRight
        }
    }
}
