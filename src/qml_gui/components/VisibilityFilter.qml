import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

Item {
    id: root
    required property var previewVm

    implicitHeight: filterSection.height

    CollapsibleSection {
        id: filterSection
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        title: qsTr("线型可见性")
        expanded: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingXS

            Repeater {
                model: root.previewVm ? root.previewVm.roleVisibilities : []

                delegate: Rectangle {
                    id: roleVisibilityRow
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 22
                    radius: Theme.radiusSM
                    color: rowHover.hovered ? Theme.bgHover : "transparent"

                    HoverHandler { id: rowHover }

                    RowLayout {
                        anchors.fill: parent
                        spacing: Theme.spacingXS

                        Rectangle {
                            Layout.preferredWidth: 10
                            Layout.preferredHeight: 10
                            radius: 2
                            color: roleVisibilityRow.modelData.color
                        }

                        Label {
                            Layout.fillWidth: true
                            text: roleVisibilityRow.modelData.label
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeSM
                            elide: Text.ElideRight
                        }

                        CxCheckBox {
                            checked: roleVisibilityRow.modelData.visible
                            onToggled: if (root.previewVm)
                                root.previewVm.toggleRoleVisibility(roleVisibilityRow.modelData.roleIndex)
                        }
                    }
                }
            }
        }
    }
}
