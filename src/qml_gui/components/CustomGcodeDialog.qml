import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// Custom G-code input dialog (对齐上游 IMSlider custom gcode window)
CxDialog {
    id: root

    property alias gcodeText: gcodeArea.text
    property int targetLayer: -1
    property var previewVm: null
    property bool isEditMode: false

    dialogTitle: qsTr("Custom G-code")
    width: 380

    ColumnLayout {
        spacing: Theme.spacingSM

        Text {
            text: root.isEditMode
                   ? qsTr("Edit G-code at layer %1:").arg(root.targetLayer)
                   : qsTr("Enter G-code to insert at layer %1:").arg(root.targetLayer)
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeMD
        }

        CxTextArea {
            id: gcodeArea
            Layout.fillWidth: true
            Layout.preferredHeight: 150
            font.pixelSize: Theme.fontSizeMD
            font.family: "monospace"
            placeholderText: qsTr("; Enter custom G-code here")
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: Theme.spacingSM

            CxButton {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
            CxButton {
                text: qsTr("OK")
                highlighted: true
                onClicked: root.accept()
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
