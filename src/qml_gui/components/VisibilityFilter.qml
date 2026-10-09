import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// FeatureType legend visibility list, ported 1:1 from the upstream
// GCodeViewer render_legend FeatureType case (GCodeViewer.cpp:3160,
// :3915-3958): a "Line Type | Time | % | Usage | Display" header row followed
// by the per-extrusion-role rows and the option rows, each toggled by an eye
// icon in the trailing Display column (GCodeViewer.cpp:3310-3318, glyphs
// im_visible/im_hidden). Upstream renders the list directly in the legend
// window with no CollapsingHeader (grep: 0 hits in GCodeViewer.cpp), so this
// component is a plain column too. The Travel option row stays in StatsPanel
// (upstream Travels is the first option row, GCodeViewer.cpp:3941-3956; the
// Qt6 toggle is test-locked there, QmlUiAuditTests.cpp:11042-11043).
Column {
    id: root
    required property var previewVm

    readonly property bool sectionAvailable: previewVm
        ? (previewVm.roleVisibilityAvailable || previewVm.moveVisibilityAvailable)
        : false

    // Geometry ported from the upstream FeatureType legend: swatch side is
    // icon_size - 2 where icon_size = GetTextLineHeight()*0.7 (GCodeViewer.cpp
    // :3205 + the ~1px inset AddRectFilled at :3267-3268); the Display column
    // reserves 16px (append_headers Dummy, :3366-3369); rows inset by
    // window_padding*3 on the left (:3263).
    readonly property int swatchSize: Math.max(1, Math.round(Theme.fontSizeSM * 0.7) - 2)
    readonly property int eyeCell: 16
    readonly property int colTime: 44
    readonly property int colPercent: 30
    readonly property int colUsageLength: 56
    readonly property int colUsageWeight: 44
    readonly property int rowInset: 12

    visible: sectionAvailable
    spacing: Theme.spacingXS

    // One toggle row for a non-extrusion move kind (upstream append_option_item
    // branches, GCodeViewer.cpp:3864-3910). The whole row is the hit area
    // (upstream BBLMenuItem spans the full row, imgui_widgets.cpp:8080-8114;
    // its hover highlight is explicitly transparent, GCodeViewer.cpp:3297-3298,
    // so there is no hover tint here either). Qt6 tracks no per-option time /
    // distance, so per the upstream empty-distance rule (GCodeViewer.cpp
    // :3874-3875) the move count lands in the first Usage column and the
    // remaining stat cells stay empty.
    component MoveKindRow: RowLayout {
        id: moveKindRow
        property string label: ""
        property string swatch: "#FFFFFF"
        property bool checked: true
        property string timeText: ""
        property string percentText: ""
        property string usageText: ""
        signal toggled(bool checked)

        width: parent.width
        spacing: Theme.spacingXS
        height: 22

        TapHandler {
            onTapped: moveKindRow.toggled(!moveKindRow.checked)
        }

        // window_padding*3 row inset (GCodeViewer.cpp:3263). RowLayout is not
        // a Control, so the inset is a spacer instead of leftPadding.
        Item {
            Layout.preferredWidth: root.rowInset
            Layout.preferredHeight: 1
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSM

            // Upstream Rect swatch: square, no corner radius (GCodeViewer.cpp:3266-3269).
            Rectangle {
                Layout.preferredWidth: root.swatchSize
                Layout.preferredHeight: root.swatchSize
                Layout.alignment: Qt.AlignVCenter
                radius: 0
                color: moveKindRow.swatch
            }
            Label {
                Layout.fillWidth: true
                text: moveKindRow.label
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeSM
                elide: Text.ElideRight
            }
        }
        Label {
            Layout.preferredWidth: root.colTime
            text: moveKindRow.timeText
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
            elide: Text.ElideRight
        }
        Label {
            Layout.preferredWidth: root.colPercent
            text: moveKindRow.percentText
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
            elide: Text.ElideRight
        }
        Label {
            Layout.preferredWidth: root.colUsageLength
            text: moveKindRow.usageText
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
            elide: Text.ElideRight
        }
        Label {
            Layout.preferredWidth: root.colUsageWeight
            text: ""
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
        }
        // Display column: eye icon, visible ? im_visible : im_hidden
        // (GCodeViewer.cpp:3317), 16px like the reserved header cell.
        Image {
            Layout.preferredWidth: root.eyeCell
            Layout.preferredHeight: root.eyeCell
            Layout.alignment: Qt.AlignVCenter
            source: moveKindRow.checked ? "qrc:/qml/assets/icons/im_visible.svg"
                                        : "qrc:/qml/assets/icons/im_hidden.svg"
            sourceSize: Qt.size(root.eyeCell, root.eyeCell)
            fillMode: Image.PreserveAspectFit
        }
    }

    // Column header row (append_headers, GCodeViewer.cpp:3364-3378 + the
    // FeatureType offsets at :3728-3730). The Display header is a 16px dummy
    // with no text (:3366-3369); the Usage header only spans the first usage
    // column.
    RowLayout {
        width: parent.width
        spacing: Theme.spacingXS

        Item {
            Layout.preferredWidth: root.rowInset
            Layout.preferredHeight: 1
        }
        Label {
            Layout.fillWidth: true
            // Align with the row labels, which sit one swatch + spacing in.
            leftPadding: root.swatchSize + Theme.spacingSM
            text: qsTr("线型")
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeSM
            font.bold: true
            elide: Text.ElideRight
        }
        Label {
            Layout.preferredWidth: root.colTime
            text: qsTr("时间")
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeSM
            font.bold: true
        }
        Label {
            Layout.preferredWidth: root.colPercent
            text: "%"
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeSM
            font.bold: true
        }
        Label {
            Layout.preferredWidth: root.colUsageLength
            text: qsTr("用量")
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeSM
            font.bold: true
            elide: Text.ElideRight
        }
        Item {
            Layout.preferredWidth: root.colUsageWeight
            Layout.preferredHeight: 1
        }
        // ORCA hides the Display header: 16px dummy spacer only.
        Item {
            Layout.preferredWidth: root.eyeCell
            Layout.preferredHeight: 1
        }
    }

    // Upstream ImGui::Separator after the header row (GCodeViewer.cpp:3377).
    Rectangle {
        width: parent.width
        height: 1
        color: Theme.borderSubtle
    }

    // Per-extrusion-role rows, in ascending canonical libvgcode index order.
    // roleVisibilities() already filters to the roles present in the loaded
    // gcode (upstream get_extrusion_roles, GCodeViewer.cpp:3920-3921).
    Repeater {
        model: root.previewVm && root.previewVm.roleVisibilityAvailable
               ? root.previewVm.roleVisibilities : []

        delegate: Rectangle {
            id: roleVisibilityRow
            required property var modelData
            width: parent.width
            height: 22
            color: "transparent"

            // Full-row hit area (upstream BBLMenuItem + callback,
            // GCodeViewer.cpp:3294-3309 -> toggle_extrusion_role_visibility).
            TapHandler {
                onTapped: if (root.previewVm)
                    root.previewVm.toggleRoleVisibility(roleVisibilityRow.modelData.roleIndex)
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: root.rowInset
                spacing: Theme.spacingXS

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSM

                    // Upstream Rect swatch: square, no corner radius
                    // (GCodeViewer.cpp:3266-3269).
                    Rectangle {
                        Layout.preferredWidth: root.swatchSize
                        Layout.preferredHeight: root.swatchSize
                        Layout.alignment: Qt.AlignVCenter
                        radius: 0
                        color: roleVisibilityRow.modelData.color
                    }
                    Label {
                        Layout.fillWidth: true
                        text: roleVisibilityRow.modelData.label
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                        elide: Text.ElideRight
                    }
                }
                Label {
                    Layout.preferredWidth: root.colTime
                    text: roleVisibilityRow.modelData.time
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    elide: Text.ElideRight
                }
                Label {
                    Layout.preferredWidth: root.colPercent
                    text: roleVisibilityRow.modelData.percent
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    elide: Text.ElideRight
                }
                Label {
                    Layout.preferredWidth: root.colUsageLength
                    text: roleVisibilityRow.modelData.usageLength
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    elide: Text.ElideRight
                }
                Label {
                    Layout.preferredWidth: root.colUsageWeight
                    text: roleVisibilityRow.modelData.usageWeight
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    elide: Text.ElideRight
                }
                // Display column eye icon (GCodeViewer.cpp:3310-3318).
                Image {
                    Layout.preferredWidth: root.eyeCell
                    Layout.preferredHeight: root.eyeCell
                    Layout.alignment: Qt.AlignVCenter
                    source: roleVisibilityRow.modelData.visible
                            ? "qrc:/qml/assets/icons/im_visible.svg"
                            : "qrc:/qml/assets/icons/im_hidden.svg"
                    sourceSize: Qt.size(root.eyeCell, root.eyeCell)
                    fillMode: Image.PreserveAspectFit
                }
            }
        }
    }

    // Option rows in the upstream EOptionType enum order (Types.hpp:164-175,
    // m_options is sorted, ViewerImpl.cpp:1054-1056): Wipes(1), Retractions(2),
    // Unretractions(3), Seams(4), ToolChanges(5). Travels(0) stays in
    // StatsPanel (test lock, QmlUiAuditTests.cpp:11042-11043). Colors are the
    // upstream DEFAULT_OPTIONS_COLORS values (ViewerImpl.cpp:307-319).
    MoveKindRow {
        label: qsTr("擦料 (Wipe)")
        swatch: "#FFFF00"  // upstream Wipes (255,255,0)
        checked: root.previewVm ? root.previewVm.showWipeMoves : false
        visible: root.previewVm && root.previewVm.moveVisibilityAvailable
                 && root.previewVm.moveCountOfKind(4) > 0
        usageText: root.previewVm ? String(root.previewVm.moveCountOfKind(4)) : ""
        onToggled: function(checked) { if (root.previewVm) root.previewVm.setShowWipeMoves(checked) }
    }
    MoveKindRow {
        label: qsTr("回抽 (Retract)")
        swatch: "#CD22D6"  // upstream Retractions (205,34,214)
        checked: root.previewVm ? root.previewVm.showRetractMoves : true
        visible: root.previewVm && root.previewVm.moveVisibilityAvailable
                 && root.previewVm.moveCountOfKind(2) > 0
        usageText: root.previewVm ? String(root.previewVm.moveCountOfKind(2)) : ""
        onToggled: function(checked) { if (root.previewVm) root.previewVm.setShowRetractMoves(checked) }
    }
    MoveKindRow {
        label: qsTr("取消回抽 (Unretract)")
        swatch: "#49ADCE"  // upstream Unretractions (73,173,207)
        checked: root.previewVm ? root.previewVm.showUnretractMoves : true
        visible: root.previewVm && root.previewVm.moveVisibilityAvailable
                 && root.previewVm.moveCountOfKind(3) > 0
        usageText: root.previewVm ? String(root.previewVm.moveCountOfKind(3)) : ""
        onToggled: function(checked) { if (root.previewVm) root.previewVm.setShowUnretractMoves(checked) }
    }
    MoveKindRow {
        label: qsTr("接缝 (Seam)")
        swatch: "#E6E6E6"  // upstream Seams (230,230,230)
        checked: root.previewVm ? root.previewVm.showSeamMarks : true
        visible: root.previewVm && root.previewVm.moveVisibilityAvailable
                 && root.previewVm.moveCountOfKind(5) > 0
        usageText: root.previewVm ? String(root.previewVm.moveCountOfKind(5)) : ""
        onToggled: function(checked) { if (root.previewVm) root.previewVm.setShowSeamMarks(checked) }
    }
    // Filament changes row (upstream ToolChanges branch, GCodeViewer.cpp
    // :3901-3904): present only when the gcode carries tool changes (upstream
    // m_options collects ToolChange vertices, ViewerImpl.cpp:1013-1015).
    MoveKindRow {
        label: qsTr("换料 (Filament changes)")
        swatch: "#C1BE63"  // upstream ToolChanges (193,190,99)
        checked: root.previewVm ? root.previewVm.showToolChanges : true
        visible: root.previewVm && root.previewVm.moveVisibilityAvailable
                 && root.previewVm.toolChangeCount > 0
        usageText: root.previewVm ? String(root.previewVm.toolChangeCount) : ""
        onToggled: function(checked) { if (root.previewVm) root.previewVm.setShowToolChanges(checked) }
    }
}
