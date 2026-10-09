import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// Custom G-code input dialog, ported 1:1 from the upstream ImGui popup
// (render_input_custom_gcode, IMSlider.cpp:1294-1369).
CxDialog {
    id: root

    property alias gcodeText: gcodeArea.text
    property int targetLayer: -1
    property var previewVm: null
    property bool isEditMode: false

    dialogTitle: qsTr("Custom G-code")

    // Neutralize the style default padding: the upstream WindowPadding(20, 10)
    // gutter (IMSlider.cpp:1305) is expressed via Layout margins below so the
    // shared CxDialog header chrome stays untouched.
    padding: 0

    // IMSlider.cpp:1311-1316 AlwaysAutoResize: the window width follows the
    // content; the button row (280px offset + OK + 10px gap + Cancel) is the
    // widest line and drives the implicit width.
    width: body.implicitWidth

    ColumnLayout {
        id: body

        // ItemSpacing(10, 7) (IMSlider.cpp:1308): 7px vertical rhythm.
        spacing: Theme.spacingMD

        // IMSlider.cpp:1319: one static prompt for both the add and the edit
        // path; it never mentions the layer number.
        Text {
            text: qsTr("Enter Custom G-code used on current layer:")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeMD
            // WindowPadding(20, 10) (IMSlider.cpp:1305): 20px side gutters and
            // a 10px band above the content.
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.topMargin: 10
        }

        CxTextArea {
            id: gcodeArea
            // ImVec2(-1, GetTextLineHeight() * 6) (IMSlider.cpp:1328-1330):
            // width -1 fills the window, height is 6 text lines
            // (GetTextLineHeight() == font size).
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.preferredHeight: 6 * gcodeArea.font.pixelSize
            font.pixelSize: Theme.fontSizeMD
            font.family: Theme.fontMono
            // Hard cap of 1024 chars: char m_custom_gcode[1024] is passed as
            // sizeof to InputTextMultiline (IMSlider.hpp:231 +
            // IMSlider.cpp:1330), so strcpy truncates the prefilled buffer too.
            // TextArea exposes no maximumLength (TextField-only), so truncate.
            onTextChanged: {
                if (gcodeArea.text.length > 1024) {
                    const p = gcodeArea.cursorPosition
                    gcodeArea.text = gcodeArea.text.substring(0, 1024)
                    gcodeArea.cursorPosition = Math.min(p, 1024)
                }
            }
        }

        RowLayout {
            // NewLine + SameLine(WindowPadding.x * 14) (IMSlider.cpp:1332-1333):
            // the button line sits at a 280px offset from the window's left
            // edge; OK is drawn first, then Cancel on the same line.
            Layout.leftMargin: 280
            // WindowPadding(20, 10) (IMSlider.cpp:1305): 10px band below the
            // content.
            Layout.bottomMargin: 10
            // ItemSpacing.x = 10 (IMSlider.cpp:1308): gap between OK and
            // Cancel.
            spacing: Theme.spacingMD

            CxButton {
                text: qsTr("OK")
                highlighted: true
                // OK is disabled while the buffer is empty
                // (PushItemFlag ImGuiItemFlags_Disabled + disable style,
                // IMSlider.cpp:1336-1342).
                enabled: gcodeArea.text.length > 0
                onClicked: root.accept()
            }
            CxButton {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }

    onAccepted: {
        if (!root.previewVm || root.targetLayer < 0 || gcodeArea.text.length === 0)
            return
        if (root.isEditMode)
            root.previewVm.editCustomGcodeAtLayer(root.targetLayer, gcodeArea.text)
        else
            root.previewVm.addCustomGcodeAtLayer(root.targetLayer, gcodeArea.text)
    }

    onOpened: gcodeArea.forceActiveFocus()
}
