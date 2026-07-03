// GroupNavSidebar.qml — left option-group navigation with count + dirty badges.
// Left option-group navigation (Phase 56-03).
//
// Region IDs: SETPRINT-GROUPNAV, SETMAT-GROUPNAV

import QtQuick
import QtQuick.Layouts
import "../controls"

Rectangle {
    id: root

    // Required properties
    required property var optionModel
    required property QStringList groups

    // Selected group
    property string selectedGroup: ""
    signal groupSelected(string groupName)

    color: Theme.bgPanel

    CxScrollView {
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.topMargin: Theme.spacingLG
            anchors.leftMargin: Theme.spacingSM
            anchors.rightMargin: Theme.spacingSM
            spacing: 0

            // Section title
            Text {
                Layout.leftMargin: Theme.spacingSM
                text: qsTr("Option Groups")
                color: Theme.accent
                font.pixelSize: Theme.fontSizeSM
                font.bold: true
            }

            Item { Layout.preferredHeight: Theme.spacingLG }

            // Group items
            Repeater {
                model: [qsTr("All")].concat(root.groups)

                delegate: Rectangle {
                    required property string modelData
                    required property int index
                    Layout.fillWidth: true
                    height: 34
                    radius: Theme.radiusSM
                    color: root.selectedGroup === modelData
                           ? "#1c2a3e"
                           : grpHov.containsMouse ? "#161d28" : "transparent"
                    border.color: root.selectedGroup === modelData ? Theme.accent : "transparent"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: Theme.spacingSM
                        spacing: 0

                        Text {
                            text: modelData
                            color: root.selectedGroup === modelData ? Theme.accent : Theme.textSecondary
                            font.pixelSize: Theme.fontSizeMD
                        }

                        Item { Layout.fillWidth: true }

                        // Count badge
                        Rectangle {
                            readonly property int cnt: {
                                if (!root.optionModel) return 0
                                if (modelData === qsTr("All")) return root.optionModel.count
                                return root.optionModel.countForCategory(modelData)
                            }
                            visible: cnt > 0
                            width: cnt > 9 ? 26 : 20
                            height: 16
                            radius: 3
                            color: root.selectedGroup === modelData ? "#1e3828" : "#1e2535"
                            Text {
                                anchors.centerIn: parent
                                text: parent.cnt
                                color: root.selectedGroup === modelData ? Theme.accent : Theme.textDisabled
                                font.pixelSize: Theme.fontSizeXS
                            }
                        }
                    }

                    HoverHandler { id: grpHov }
                    TapHandler {
                        onTapped: {
                            root.selectedGroup = modelData
                            root.groupSelected(modelData)
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
