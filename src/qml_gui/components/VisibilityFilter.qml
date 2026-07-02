import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// Per-role line-type visibility filter (GCODE-02).
// Mirrors StatsPanel.qml structure (Item root + required previewVm + implicitHeight).
// Body is a CollapsibleSection card wrapping a Repeater of role rows from
// previewVm.roleVisibilities. Toggling a CxCheckBox calls the Q_INVOKABLE
// previewVm.toggleRoleVisibility(roleIndex); render-side filtering fires via
// the GLViewport.roleVisibility binding in PreviewPage.qml. No business logic
// lives here (QML boundary rule).
Item {
    id: root
    required property var previewVm

    implicitHeight: filterSection.height

    CollapsibleSection {
        id: filterSection
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        title: qsTr("可见线条类型")
        expanded: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSM

            Repeater {
                model: root.previewVm ? root.previewVm.roleVisibilities : []

                // One row per extrusion role: color swatch + label + checkbox.
                // Rectangle root gives the hover background (mirrors the camera
                // preset button pattern in PreviewPage.qml).
                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 22
                    radius: Theme.radiusSM
                    color: rowHover.hovered ? Theme.bgHover : "transparent"

                    HoverHandler { id: rowHover }

                    RowLayout {
                        anchors.fill: parent
                        spacing: Theme.spacingXS

                        // Role FeatureType color swatch (mirrors Legend.qml).
                        Rectangle {
                            width: 10
                            height: 10
                            radius: 2
                            color: modelData.color
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr(modelData.label)
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeSM
                            elide: Text.ElideRight
                        }

                        CxCheckBox {
                            checked: modelData.visible
                            onToggled: if (root.previewVm)
                                root.previewVm.toggleRoleVisibility(modelData.roleIndex)
                        }
                    }
                }
            }
        }
    }
}
