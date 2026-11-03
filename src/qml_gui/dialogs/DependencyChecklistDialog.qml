import QtQuick
pragma ComponentBehavior: Bound
import QtQuick.Controls
import QtQuick.Layouts
import OWzx

// DependencyChecklistDialog.qml - filament Dependencies "Set ..." list.
//
// Presentation of the upstream MultiChoiceDialog opened by
// Tab::compatible_widget_create: one checkbox per printer / process preset,
// pre-checked from the preset's compatible_printers / compatible_prints.
// OK hands the checked names back; ConfigOptionModel::setDependencySelection
// stores the empty list ("All") when every entry is checked.
CxDialog {
    id: root

    property string label: ""
    property var candidates: []
    property var checkedNames: []

    signal selectionAccepted(var names)

    function openWith(title, labelText, names, selection) {
        root.dialogTitle = title
        root.label = labelText
        root.candidates = names
        root.checkedNames = selection.slice()
        root.open()
    }

    function toggle(name, on) {
        var next = root.checkedNames.filter(function(entry) { return entry !== name })
        if (on)
            next.push(name)
        root.checkedNames = next
    }

    width: 360
    height: 480

    contentItem: ColumnLayout {
        spacing: Theme.spacingSM

        Text {
            Layout.fillWidth: true
            text: root.label
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeMD
        }

        ListView {
            id: candidateList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.candidates
            delegate: CxCheckBox {
                required property string modelData
                width: candidateList.width
                text: modelData
                checked: root.checkedNames.indexOf(modelData) >= 0
                onToggled: root.toggle(modelData, checked)
            }
            ScrollBar.vertical: ScrollBar {}
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSM
            Item { Layout.fillWidth: true }
            CxButton {
                text: qsTr("确定")
                compact: true
                onClicked: {
                    root.selectionAccepted(root.checkedNames)
                    root.close()
                }
            }
            CxButton {
                text: qsTr("取消")
                compact: true
                cxStyle: CxButton.Style.Secondary
                onClicked: root.close()
            }
        }
    }
}
