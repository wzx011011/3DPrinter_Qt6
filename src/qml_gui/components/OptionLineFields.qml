// OptionLineFields.qml - the option fields of one upstream multi-option line.
//
// Presentation of Tab.cpp `Line line = {label, tooltip};
// line.append_option(...)`: one row label (drawn by the owning OptionRow),
// several fields. Members come from a ConfigOptionFilterProxy in lineHead
// mode (append_option order, view-mode filtered) and edit through inline
// OptionRow instances, so values, toggles and dirty state follow the same
// model paths as single-option rows.
//
// Side by side with the per-option label in front ("最小 [190 ℃] 最大 [230 ℃]")
// when the members fit; stacked with the label after the field (Orca's
// narrow sidebar: "[50 mm/s] 25%") when they do not.

import QtQuick
pragma ComponentBehavior: Bound
import OWzx

Item {
    id: root

    property var optionModel: null
    property string headKey: ""
    property int viewMode: 0
    property real availableWidth: 300
    property int fieldWidth: 72
    property int enumWidth: 112
    readonly property int memberHeight: 26
    readonly property int memberGap: 4
    readonly property int subLabelWidth: 44

    readonly property int memberCount: members.count
    readonly property real sideBySideWidth:
        memberCount * (subLabelWidth + 4 + fieldWidth) + Math.max(0, memberCount - 1) * 10
    readonly property bool stacked: sideBySideWidth > availableWidth
    // Stacked members keep room for the trailing per-option label
    // ("[50 mm/s] 25%") inside the narrow sidebar column.
    readonly property int memberFieldWidth:
        stacked ? Math.max(48, Math.min(fieldWidth, availableWidth - subLabelWidth - 4)) : fieldWidth

    implicitWidth: grid.implicitWidth
    implicitHeight: Math.max(memberHeight, grid.implicitHeight)

    ConfigOptionFilterProxy {
        id: members
        sourceModel: root.optionModel
        lineHead: root.headKey
        viewMode: root.viewMode
    }

    Grid {
        id: grid
        columns: root.stacked ? 1 : Math.max(1, root.memberCount)
        rowSpacing: root.memberGap
        columnSpacing: 10

        Repeater {
            model: members

            delegate: Row {
                id: member
                required property int index
                required property int optIdx
                required property string optType
                required property string optKey
                required property string optSubLabel
                required property var optValue
                required property double optMin
                required property double optMax
                required property double optStep
                required property bool optReadonly
                required property bool optEnabled
                required property bool optDirty
                required property string optTooltip
                required property string optUnit
                required property string optSidetext
                required property bool optNullable
                required property bool optIsVector
                required property var optEnumLabels

                readonly property int controlWidth:
                    optType === "bool" ? 24
                    : optType === "enum" ? root.enumWidth
                    : root.memberFieldWidth

                height: root.memberHeight
                spacing: 4

                Text {
                    visible: !root.stacked && member.optSubLabel !== ""
                    width: root.subLabelWidth
                    height: root.memberHeight
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                    text: member.optSubLabel
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                    elide: Text.ElideRight
                }

                OptionRow {
                    width: member.controlWidth
                    height: root.memberHeight
                    inlineMember: true
                    compact: true
                    compactFieldWidth: root.memberFieldWidth
                    compactEnumWidth: root.enumWidth
                    optionModel: root.optionModel
                    optIdx: member.optIdx
                    rowIndex: member.index
                    oType: member.optType
                    oKey: member.optKey
                    oLabel: member.optSubLabel
                    oVal: member.optValue
                    oMin: member.optMin
                    oMax: member.optMax
                    oStep: member.optStep
                    oRO: member.optReadonly
                    oEnabled: member.optEnabled
                    oDirty: member.optDirty
                    oTip: member.optTooltip
                    oUnit: member.optUnit
                    oSidetext: member.optSidetext
                    oNullable: member.optNullable
                    oIsVector: member.optIsVector
                    oEnumLabels: member.optEnumLabels
                }

                Text {
                    visible: root.stacked && member.optSubLabel !== ""
                    height: root.memberHeight
                    verticalAlignment: Text.AlignVCenter
                    text: member.optSubLabel
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                }
            }
        }
    }
}
