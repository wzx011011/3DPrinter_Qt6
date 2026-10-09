import QtQuick
import ".."
import "../controls"

// -----------------------------------------------------------------------------
// RecenterDialog.qml - out-of-bounds prompt, ported 1:1 from upstream
// OrcaSlicer RecenterDialog.cpp (third_party/OrcaSlicer/src/slic3r/GUI).
//
// Upstream structure (RecenterDialog.cpp):
//   - Window 475 wide with content-fit height: SetSize/SetMinSize(475,-1) +
//     Fit() over a 1px top line (m_line_top) + a 100px draw spacer
//     (DRAW_PANEL_SIZE.y) + the button row (:10, :28, :35-37, :43-46).
//   - Self-drawn hint starting at BORDER=25: "Please home all axes (click" +
//     an inline 24px monitor_axis_home_icon bitmap + ") to locate the
//     toolhead's position. This prevents device moving beyond the printable
//     boundary and causing equipment wear." Label::Body_14 on (107,107,107),
//     wrapped to the 475-2*25 text width (:9-12, :18-19, :57, :65-146).
//     Upstream has NO object list, NO panel border, NO empty state.
//   - DialogButtons {"OK","Cancel"} relabeled "Go Home"/"Close" (:31-33):
//     OK = Confirm/primary, Cancel = Regular, right-aligned, 10 gap between
//     buttons, each with 10 top/bottom/right margins (DialogButtons.cpp:135-151,
//     Button.hpp:12); the panel itself adds 10 bottom/right border in the main
//     sizer (:37), so buttons sit 20 from the dialog edge.
//   - Title is the generic _L("Confirm") (:16); style wxCLOSE_BOX|wxCAPTION
//     (RecenterDialog.hpp:30) -> closes via Esc / title-bar close box.
//   - OK ends the modal and the caller issues the go-home command
//     (StatusPanel.cpp:2932-2934 command_go_home).
//
// OWzx mapping: the CxDialog 44px header carries the 1px separator that stands
// in for m_line_top (CxDialog.qml:45-55). The dialog is raised by the
// outside-bed check (BackendContext.cpp:249-250 recenterPromptRequested;
// trigger chain locked by tests/QmlUiAuditTests.cpp:10413), so the primary
// button routes through main.qml's onRecenterRequested ->
// EditorViewModel::recenterObjectsOutsideBed (the objectsOutsideBed model
// itself stays on EditorViewModel; tests/ViewModelSmokeTests.cpp:1184-1194).
//
// Usage (unchanged wiring in main.qml):
//   RecenterDialog {
//       editorVm: backend.editorViewModel
//       onRecenterRequested: backend.editorViewModel.recenterObjectsOutsideBed()
//   }
// -----------------------------------------------------------------------------

CxDialog {
    id: root
    modal: true
    // U01: upstream RecenterDialog is a wxCLOSE_BOX|wxCAPTION modal closed by
    // Esc / the close box / its buttons (RecenterDialog.hpp:30). Keep the
    // CxDialog default (CloseOnEscape) -- do not override closePolicy here.
    dialogTitle: qsTr("Confirm")   // upstream DPIDialog title _L("Confirm") (:16)
    width: 475                     // upstream DRAW_PANEL_SIZE.x (:10, :43-44)
    // Upstream Fit() height (RecenterDialog.cpp:43-46): the 1px top line is
    // already the CxDialog header separator; then the 100px draw area (:36),
    // 10 button top margin, the button row, and the 20 bottom inset
    // (10 DialogButtons margin + 10 sizer border, :37 + DialogButtons.cpp:140-151).
    // QQC2 Dialog does not adopt a custom contentItem's implicit size for the
    // popup itself, so the height is computed here instead of left implicit.
    height: 44 + 100 + 10 + btnRow.implicitHeight + 20
    padding: 0

    // Injected by main.qml (editorVm: backend.editorViewModel). Wiring-only:
    // the upstream dialog shows no object list, so nothing reads the model
    // here; the objectsOutsideBed property stays on EditorViewModel.
    required property var editorVm

    signal recenterRequested()

    // Content metrics are upstream literals: DRAW_PANEL_SIZE.y=100, BORDER=25
    // (RecenterDialog.cpp:9-10); the button row keeps 10 top margin, and
    // 20 right/bottom insets = DialogButtons inner margin 10 + the panel's
    // 10 sizer border (:37, DialogButtons.cpp:140-151).
    contentItem: Item {
        implicitWidth: 475
        implicitHeight: 100 + 10 + btnRow.implicitHeight + 20

        // The 100px upstream draw area with the hint at (25,25); the 24px
        // inline bitmap mirrors ScalableBitmap("monitor_axis_home_icon", 24)
        // (:57), font mirrors Label::Body_14, color uses the dark-theme
        // neutral text token for the upstream gray (107,107,107) (:12, :68).
        Text {
            x: 25
            y: 25
            width: parent.width - 50
            textFormat: Text.RichText
            text: qsTr("Please home all axes (click <img src=\"qrc:/qml/assets/icons/monitor_axis_home_icon.svg\" width=\"24\" height=\"24\" valign=\"middle\">) to locate the toolhead's position. This prevents device moving beyond the printable boundary and causing equipment wear.")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeLG   // upstream Label::Body_14
            wrapMode: Text.WordWrap
        }

        // Upstream row: [stretch] Go Home(Confirm/primary) 10 Close(Regular),
        // right-aligned with 10 button gap (DialogButtons.cpp:145-151).
        Row {
            id: btnRow
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20
            spacing: Theme.spacingMD   // upstream ChoiceButtonGap() = 10 (Button.hpp:12)

            CxButton {
                text: qsTr("Go Home")   // upstream OK relabel (:32)
                cxStyle: CxButton.Style.Primary
                onClicked: {
                    // Upstream OK ends the modal and the caller sends the
                    // go-home command (StatusPanel.cpp:2932-2934); in OWzx the
                    // prompt is raised by the outside-bed check, so confirm
                    // routes to the recenter action via main.qml.
                    root.recenterRequested()
                    root.accept()
                }
            }
            CxButton {
                text: qsTr("Close")   // upstream CANCEL relabel (:33)
                cxStyle: CxButton.Style.Secondary
                onClicked: root.reject()
            }
        }
    }
}
