import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// RecenterDialog.qml — Phase 236 (DLG-03) out-of-bed objects prompt.
//
// Upstream: OrcaSlicer warns when objects fall outside the print bed
// (Plater::check_outside_state / the outside-bed warning flow) and offers to
// bring them back. OWzx surfaces the offending object list and a single
// "全部居中" action backed by EditorViewModel::recenterObjectsOutsideBed
// (clamps every outside object's footprint back into the printable area).
//
// Usage (data comes from editorVm.objectsOutsideBed, refreshed by
// editorVm.checkObjectsOutsideBed()):
//   RecenterDialog {
//       editorVm: backend.editorViewModel
//       onRecenterRequested: editorVm.recenterObjectsOutsideBed()
//   }
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    closePolicy: Popup.NoAutoClose
    dialogTitle: qsTr("对象超出打印范围")
    width: 420
    height: 340
    padding: 0

    required property var editorVm

    signal recenterRequested()
    signal ignored()

    contentItem: ColumnLayout {
        spacing: Theme.spacingMD
        anchors.margins: Theme.spacingXL

        Text {
            Layout.fillWidth: true
            text: qsTr("以下对象位于打印范围之外，将无法切片。是否将其移回打印范围？")
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeMD
            wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.bgInset
            radius: 4
            border.color: Theme.borderSubtle
            border.width: 1
            clip: true

            ListView {
                anchors.fill: parent
                anchors.margins: Theme.spacingXS
                clip: true
                model: root.editorVm ? root.editorVm.objectsOutsideBed : []

                delegate: Rectangle {
                    required property var modelData
                    width: parent.width
                    height: 28
                    radius: 3
                    color: index % 2 === 0 ? "transparent" : Theme.bgBase

                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: Theme.spacingSM
                        spacing: Theme.spacingSM

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "⚠"
                            color: Theme.statusWarning
                            font.pixelSize: Theme.fontSizeSM
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.name !== undefined ? modelData.name : ""
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSM
                            elide: Text.ElideRight
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: !parent.model || parent.model.length === 0
                    text: qsTr("无对象")
                    color: Theme.textDisabled
                    font.pixelSize: Theme.fontSizeSM
                }

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight
            spacing: Theme.spacingMD

            CxButton {
                text: qsTr("忽略")
                cxStyle: CxButton.Style.Secondary
                onClicked: {
                    root.ignored()
                    root.reject()
                }
            }
            CxButton {
                text: qsTr("全部居中")
                cxStyle: CxButton.Style.Primary
                onClicked: {
                    root.recenterRequested()
                    root.accept()
                }
            }
        }
    }
}
