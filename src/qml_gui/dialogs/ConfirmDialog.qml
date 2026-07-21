import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// ConfirmDialog.qml — Phase 169 (XD-01) shared destructive-action confirm.
//
// Pattern: every destructive trigger (delete / clear / remove / disconnect)
// routes through this dialog instead of firing immediately. Mirrors upstream
// OrcaSlicer's MessageDialog-based confirmation flow (GUI_Msg.cpp MessageCN).
//
// Usage:
//   ConfirmDialog {
//       id: deleteConfirm
//       title: qsTr("删除选中")
//       message: qsTr("确定要删除选中的对象吗？此操作不可撤销（但可通过撤销恢复）。")
//       confirmText: qsTr("删除")
//       cancelText: qsTr("取消")
//       onAccepted: // execute the destructive action
//       onRejected: // no-op (default)
//   }
//   // Caller:
//   deleteConfirm.openWithAction(function() { /* the destructive call */ })
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    dialogTitle: ""
    width: 420
    height: 200
    padding: 0

    // Dialog contents
    property string message: ""
    property string confirmText: qsTr("确定")
    property string cancelText: qsTr("取消")
    property bool destructive: true   // styles the confirm button as Danger

    // The action to run on accept. Set via openWithAction(fn).
    property var _action: null

    // Phase 171 hotfix: removed `signal accepted()` / `signal rejected()` —
    // these are inherited from QtQuick.Controls Dialog (the CxDialog base)
    // and redeclaring them triggered "Duplicate signal name" at QML compile
    // time, which made ConfirmDialog fail to register as a type, which made
    // HomePage (which uses ConfirmDialog) fail to load, which made the whole
    // app exit with -1. The base Dialog emits accepted()/rejected()
    // automatically when accept()/reject() are called.

    function openWithAction(action) {
        root._action = action
        root.open()
    }

    onRejected: {
        root._action = null
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingXL
        spacing: Theme.spacingLG

        Text {
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: root.message
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeMD
            wrapMode: Text.WordWrap
            verticalAlignment: Text.AlignVCenter
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight
            spacing: Theme.spacingMD

            CxButton {
                text: root.cancelText
                cxStyle: CxButton.Style.Secondary
                onClicked: {
                    root.rejected()
                    root.reject()
                }
            }
            CxButton {
                text: root.confirmText
                cxStyle: root.destructive ? CxButton.Style.Danger : CxButton.Style.Primary
                onClicked: {
                    if (root._action) {
                        try { root._action() } catch (e) { console.warn("ConfirmDialog action threw:", e) }
                    }
                    root.accepted()
                    root.accept()
                }
            }
        }
    }
}
