import QtQuick
pragma ComponentBehavior: Bound
import QtQuick.Layouts
import OWzx

// -----------------------------------------------------------------------------
// StepMeshDialog.qml - STEP import mesh parameters, ported from upstream
// OrcaSlicer src/slic3r/GUI/StepMeshDialog.cpp.
//
// Upstream structure:
//   - Title "Step file import parameters"; a tip box with the deflection hint
//     and a "Wiki Guide" link (import_export#step).
//   - Linear Deflection: slider 1..100 (x0.001) + text input "%.3f", valid
//     range [0.001, 0.1]; Angle Deflection: slider 1..100 (x0.01) + "%.2f",
//     valid range [0.01, 1]. An invalid entry warns and reverts to the last
//     valid value; releasing a slider or committing a value recounts.
//   - "Split compound and compsolid into multiple objects" (initial value =
//     is_split_compound).
//   - "Number of triangular facets": "Calculating, please wait..." while the
//     loaded Step is meshed on a worker (Step::get_triangle_num).
//   - "Save these settings as default", "Don't show again", OK / Cancel.
//     OK validates both values; Cancel / close aborts the import.
//
// Behaviour lives in StepMeshPrompt (src/core/services); this file only
// presents it. The prompt object is EditorViewModel.stepMeshPrompt.
// -----------------------------------------------------------------------------

CxDialog {
    id: root
    modal: true
    dialogTitle: qsTr("Step file import parameters")
    width: 520
    padding: 20
    // QQC2 Dialog does not adopt a custom contentItem's implicit height
    // (see RecenterDialog): header 44 + content + padding.
    height: 44 + contentColumn.implicitHeight + topPadding + bottomPadding

    required property var prompt

    property string linearText: "0.003"
    property string angleText: "0.50"
    property string warningText: ""
    property bool answered: false

    function formatLinear(v) { return Number(v).toFixed(3) }
    function formatAngle(v) { return Number(v).toFixed(2) }

    function reset() {
        answered = false
        warningText = ""
        linearText = formatLinear(prompt.initialLinear)
        angleText = formatAngle(prompt.initialAngle)
        splitBox.checked = prompt.initialSplitCompound
        saveDefaultBox.checked = false
        dontShowBox.checked = false
    }

    function recount() {
        prompt.requestFacetCount(Number(linearText), Number(angleText))
    }

    function commitLinear(text) {
        var v = Number(text)
        if (text.length > 0 && !isNaN(v) && prompt.linearInRange(v)) {
            linearText = text
            warningText = ""
            recount()
        } else {
            warningText = qsTr("Please input a valid value (0.001 < linear deflection < 0.1)")
            linearInput.text = linearText
        }
    }

    function commitAngle(text) {
        var v = Number(text)
        if (text.length > 0 && !isNaN(v) && prompt.angleInRange(v)) {
            angleText = text
            warningText = ""
            recount()
        } else {
            warningText = qsTr("Please input a valid value (0.01 < angle deflection < 1.0)")
            angleInput.text = angleText
        }
    }

    onAboutToShow: reset()
    onClosed: {
        // Esc / close box == Cancel (upstream wxEVT_CLOSE_WINDOW).
        if (!answered && prompt.active)
            prompt.finish(false, Number(linearText), Number(angleText), splitBox.checked, false, false)
    }

    contentItem: ColumnLayout {
        id: contentColumn
        spacing: 12

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: tipColumn.implicitHeight + 20
            radius: 6
            color: Theme.bgSurface
            ColumnLayout {
                id: tipColumn
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Smaller linear and angular deflections result in higher-quality transformations but increase the processing time.")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeLG
                    wrapMode: Text.WordWrap
                }
                Text {
                    text: "<a href=\"https://www.orcaslicer.com/wiki/import_export#step\">" + qsTr("Wiki Guide") + "</a>"
                    textFormat: Text.RichText
                    color: Theme.accent
                    linkColor: Theme.accent
                    font.pixelSize: Theme.fontSizeLG
                    onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Text {
                text: qsTr("Linear Deflection") + ": "
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLG
                Layout.fillWidth: true
            }
            CxSlider {
                id: linearSlider
                Layout.preferredWidth: 220
                from: 1; to: 100; stepSize: 1
                value: Math.round(Number(root.linearText) / 0.001)
                onMoved: {
                    root.linearText = root.formatLinear(value * 0.001)
                    linearInput.text = root.linearText
                }
                onPressedChanged: if (!pressed) root.recount()
            }
            CxTextField {
                id: linearInput
                objectName: "stepLinearInput"
                Layout.preferredWidth: 100
                horizontalAlignment: TextInput.AlignHCenter
                text: root.linearText
                onEditingFinished: root.commitLinear(text)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Text {
                text: qsTr("Angle Deflection") + ": "
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLG
                Layout.fillWidth: true
            }
            CxSlider {
                id: angleSlider
                Layout.preferredWidth: 220
                from: 1; to: 100; stepSize: 1
                value: Math.round(Number(root.angleText) / 0.01)
                onMoved: {
                    root.angleText = root.formatAngle(value * 0.01)
                    angleInput.text = root.angleText
                }
                onPressedChanged: if (!pressed) root.recount()
            }
            CxTextField {
                id: angleInput
                objectName: "stepAngleInput"
                Layout.preferredWidth: 100
                horizontalAlignment: TextInput.AlignHCenter
                text: root.angleText
                onEditingFinished: root.commitAngle(text)
            }
        }

        Text {
            Layout.fillWidth: true
            visible: root.warningText.length > 0
            text: root.warningText
            color: Theme.statusError
            font.pixelSize: Theme.fontSizeMD
            wrapMode: Text.WordWrap
        }

        CxCheckBox {
            id: splitBox
            text: qsTr("Split compound and compsolid into multiple objects")
        }

        RowLayout {
            spacing: 4
            Text {
                text: qsTr("Number of triangular facets") + ": "
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLG
            }
            Text {
                objectName: "stepFacetCount"
                text: root.prompt.facetCount < 0 ? qsTr("Calculating, please wait...")
                                                 : String(root.prompt.facetCount)
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLG
            }
        }

        CxCheckBox {
            id: saveDefaultBox
            text: qsTr("Save these settings as default")
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 8
            spacing: 10
            CxCheckBox {
                id: dontShowBox
                text: qsTr("Don't show again")
            }
            Item { Layout.fillWidth: true }
            CxButton {
                text: qsTr("OK")
                cxStyle: CxButton.Style.Primary
                onClicked: {
                    if (root.prompt.finish(true, Number(root.linearText), Number(root.angleText),
                                           splitBox.checked, saveDefaultBox.checked, dontShowBox.checked)) {
                        root.answered = true
                        root.close()
                    }
                }
            }
            CxButton {
                text: qsTr("Cancel")
                cxStyle: CxButton.Style.Secondary
                onClicked: {
                    root.prompt.finish(false, Number(root.linearText), Number(root.angleText),
                                       splitBox.checked, false, false)
                    root.answered = true
                    root.close()
                }
            }
        }
    }
}
