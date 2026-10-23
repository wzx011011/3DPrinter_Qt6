import QtQuick
pragma ComponentBehavior: Bound
import QtQuick.Controls
import QtQuick.Layouts
import OWzx

// CloudDownloadDialog (feature/cloud-model-library, M4) — progress dialog
// for cloud model downloads, the Qt port of the official
// CloudDownloadProgressDialog (200ms-poll → direct property binding):
// - progress binds Backend.modelDownloadService.progressPercent
// - cancel maps to cancelLatest() (upstream cancel_3mf_download)
// - the dialog closes itself when the queue drains; the C++ side owns the
//   "close dialog, then import" ordering for completed 3mfs (deferred
//   import in BackendContext — upstream CallAfter parity).
CxDialog {
    id: root

    signal dismissed()

    dialogTitle: qsTr("下载云模型")
    width: 420
    closePolicy: Popup.NoAutoClose

    readonly property QtObject downloads: Backend ? Backend.modelDownloadService : null

    onOpened: progressTimer.restart()
    // Self-close when the queue drains (success, failure or cancel — the
    // failure toast rides the bridge/panel status surface in M5).
    Connections {
        target: root.downloads
        function onStateChanged() {
            if (root.downloads && !root.downloads.busy && root.visible)
                closeTimer.restart()
        }
    }

    Timer {
        id: progressTimer
        interval: 200
        repeat: true
        running: false
        onTriggered: if (root.downloads) root.downloads.progressPercent
        // property-read poll keeps the binding honest for indeterminate
        // totals (server without Content-Length); the signal path covers
        // the normal case.
    }
    Timer {
        id: closeTimer
        interval: 400
        onTriggered: {
            root.close()
            root.dismissed()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingMD

        Label {
            Layout.fillWidth: true
            text: root.downloads && root.downloads.busy
                      ? qsTr("正在下载：%1").arg(root.downloads.currentName)
                      : qsTr("下载完成")
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeMD
            elide: Text.ElideRight
        }

        ProgressBar {
            Layout.fillWidth: true
            from: 0
            to: 100
            value: root.downloads ? root.downloads.progressPercent : 0
        }

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignRight
            text: (root.downloads ? root.downloads.progressPercent : 0) + "%"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 8

            CxButton {
                text: qsTr("取消下载")
                visible: root.downloads ? root.downloads.busy : false
                onClicked: {
                    if (root.downloads) root.downloads.cancelLatest()
                }
            }
            CxButton {
                text: qsTr("关闭")
                highlighted: true
                visible: root.downloads ? !root.downloads.busy : true
                onClicked: {
                    root.close()
                    root.dismissed()
                }
            }
        }
    }
}
