import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// SingleChoiceDialog.qml — Phase 236 (DLG-03) shared single-choice dialog.
//
// Upstream: OrcaSlicer uses message dialogs with a wxChoice / radio list for
// "pick one of N" flows (e.g. data path selection, recenter prompts). OWzx
// centralizes the pattern here so feature dialogs (Recenter, future pickers)
// share one implementation.
//
// Usage:
//   SingleChoiceDialog {
//       dialogTitle: qsTr("选择...")
//       choiceModel: [qsTr("选项 A"), qsTr("选项 B")]
//       onSelected: function(index, value) { /* ... */ }
//   }
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    width: 380
    height: 320
    padding: 0

    // Public API
    property var choiceModel: []              // list of strings (or {label, value})
    property string message: ""
    property string confirmText: qsTr("确定")
    property string cancelText: qsTr("取消")
    readonly property int currentIndex: choiceList.currentIndex
    signal selected(int index, var value)

    function itemLabel(item) {
        return (item && item.label !== undefined) ? item.label : item
    }
    function itemValue(item) {
        return (item && item.value !== undefined) ? item.value : item
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacingMD
        anchors.margins: Theme.spacingXL

        Text {
            visible: root.message !== ""
            Layout.fillWidth: true
            text: root.message
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
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
                id: choiceList
                anchors.fill: parent
                anchors.margins: Theme.spacingXS
                clip: true
                model: root.choiceModel
                spacing: 2
                currentIndex: 0

                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    width: choiceList.width
                    height: 30
                    radius: 3
                    color: choiceList.currentIndex === index ? Theme.accentSubtle
                           : (choiceHover.containsMouse ? Theme.bgHover : "transparent")

                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: Theme.spacingSM
                        spacing: Theme.spacingSM

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: choiceList.currentIndex === index ? "●" : "○"
                            color: choiceList.currentIndex === index ? Theme.accent : Theme.textTertiary
                            font.pixelSize: Theme.fontSizeSM
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: root.itemLabel(modelData)
                            color: choiceList.currentIndex === index ? Theme.textPrimary : Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSM
                            elide: Text.ElideRight
                            width: parent.width - 24
                        }
                    }

                    HoverHandler { id: choiceHover }
                    TapHandler {
                        onTapped: choiceList.currentIndex = index
                        onDoubleTapped: {
                            choiceList.currentIndex = index
                            root.selected(index, root.itemValue(modelData))
                            root.accept()
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight
            spacing: Theme.spacingMD

            CxButton {
                text: root.cancelText
                cxStyle: CxButton.Style.Secondary
                onClicked: root.reject()
            }
            CxButton {
                text: root.confirmText
                cxStyle: CxButton.Style.Primary
                enabled: choiceList.currentIndex >= 0 && choiceList.count > 0
                onClicked: {
                    root.selected(choiceList.currentIndex,
                                  root.itemValue(root.choiceModel[choiceList.currentIndex]))
                    root.accept()
                }
            }
        }
    }
}
