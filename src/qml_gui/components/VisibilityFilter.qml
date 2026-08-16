import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

Item {
    id: root
    required property var previewVm

    implicitHeight: filterSection.height

    // Phase 238 (PREV-03): one toggle row for a non-extrusion move kind
    // (see the MoveKindRow instances below).
    component MoveKindRow: RowLayout {
        id: moveKindRow
        property string label: ""
        property string swatch: "#FFFFFF"
        property bool checked: true
        signal toggled(bool checked)

        Layout.fillWidth: true
        spacing: Theme.spacingXS

        Rectangle {
            Layout.preferredWidth: 10
            Layout.preferredHeight: 10
            radius: 2
            color: moveKindRow.swatch
        }
        Label {
            Layout.fillWidth: true
            text: moveKindRow.label
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeSM
            elide: Text.ElideRight
        }
        CxCheckBox {
            checked: moveKindRow.checked
            onToggled: moveKindRow.toggled(checked)
        }
    }

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

            // Phase 238 (PREV-03): non-extrusion move-type toggles aligned
            // with the upstream GCodeViewer options_items checkboxes
            // (Travel/Retract/Unretract/Wipe/Seam, GCodeViewer.cpp:913-921 +
            // 4936-4950). Travel stays in StatsPanel (existing layout); the
            // four new kinds live here. Colors are the upstream
            // Options_Colors / Wipe_Color values.
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Theme.borderSubtle
                visible: retractRow.visible || unretractRow.visible
            }

            MoveKindRow {
                id: retractRow
                label: qsTr("回抽 (Retract)")
                swatch: "#CD22D6"  // upstream Retractions (0.803,0.135,0.839)
                checked: root.previewVm ? root.previewVm.showRetractMoves : true
                visible: root.previewVm && root.previewVm.moveCountOfKind(2) > 0
                onToggled: function(checked) { if (root.previewVm) root.previewVm.setShowRetractMoves(checked) }
            }
            MoveKindRow {
                id: unretractRow
                label: qsTr("取消回抽 (Unretract)")
                swatch: "#49ADCE"  // upstream Unretractions (0.287,0.679,0.810)
                checked: root.previewVm ? root.previewVm.showUnretractMoves : true
                visible: root.previewVm && root.previewVm.moveCountOfKind(3) > 0
                onToggled: function(checked) { if (root.previewVm) root.previewVm.setShowUnretractMoves(checked) }
            }
            MoveKindRow {
                label: qsTr("擦料 (Wipe)")
                swatch: "#FFFF00"  // upstream Wipe_Color YELLOW
                checked: root.previewVm ? root.previewVm.showWipeMoves : false
                visible: root.previewVm && root.previewVm.moveCountOfKind(4) > 0
                onToggled: function(checked) { if (root.previewVm) root.previewVm.setShowWipeMoves(checked) }
            }
            MoveKindRow {
                label: qsTr("接缝 (Seam)")
                swatch: "#E6E6E6"  // upstream Seams (0.9,0.9,0.9)
                checked: root.previewVm ? root.previewVm.showSeamMarks : true
                visible: root.previewVm && root.previewVm.moveCountOfKind(5) > 0
                onToggled: function(checked) { if (root.previewVm) root.previewVm.setShowSeamMarks(checked) }
            }
        }
    }
}
