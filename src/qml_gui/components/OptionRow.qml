// OptionRow.qml — typed-option renderer for all 7 option kinds.
// Typed option renderer (Phase 56-03).
//
// Dispatches by optType: bool, int, double, percent, enum, string.
// Nullable and multi-value are surfaced via value-source indicator and
// per-extruder selector (not separate dispatch branches).
//
// Region IDs: SETPRINT-OPTIONS, SETMAT-OPTIONS

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

Item {
    id: root

    // Required properties (passed in by ListView delegate)
    required property var optionModel
    required property int optIdx
    required property int rowIndex

    // Optional properties (set by SettingsDialog delegate)
    property string searchText: ""
    property bool showGroupHeader: false
    property string oGroup: ""
    property string valueSource: ""
    property bool compact: false
    property int compactLabelWidth: 112
    property int compactFieldWidth: 72
    property int compactEnumWidth: 112

    function displayValueSource(sourceKey) {
        if (sourceKey === "default") return qsTr("默认")
        if (sourceKey === "print") return qsTr("打印")
        if (sourceKey === "filament") return qsTr("耗材")
        if (sourceKey === "printer") return qsTr("打印机")
        return sourceKey
    }

    function displayOptionLabel(key, label) {
        var labels = {
            "layer_height": qsTr("层高"),
            "initial_layer_print_height": qsTr("首层层高"),
            "line_width": qsTr("线宽"),
            "initial_layer_line_width": qsTr("首层线宽"),
            "wall_loops": qsTr("墙层数"),
            "top_shell_layers": qsTr("顶层层数"),
            "bottom_shell_layers": qsTr("底层层数"),
            "sparse_infill_density": qsTr("填充密度"),
            "sparse_infill_pattern": qsTr("填充图案"),
            "outer_wall_speed": qsTr("外墙速度"),
            "inner_wall_speed": qsTr("内墙速度"),
            "sparse_infill_speed": qsTr("填充速度"),
            "top_surface_speed": qsTr("顶面速度"),
            "travel_speed": qsTr("空驶速度"),
            "initial_layer_speed": qsTr("首层速度"),
            "enable_support": qsTr("启用支撑"),
            "support_density": qsTr("支撑密度"),
            "support_type": qsTr("支撑类型")
        }
        return labels[key] || label
    }

    // Computed accessors (mirrors ParamsPage paramRow pattern)
    readonly property string oType:   root.optionModel ? root.optionModel.optType(optIdx) : ""
    readonly property string oKey:    root.optionModel ? root.optionModel.optKey(optIdx) : ""
    readonly property string oLabel:  root.optionModel ? root.displayOptionLabel(root.oKey, root.optionModel.optLabel(optIdx)) : ""
    readonly property var    oVal:    root.optionModel ? root.optionModel.optValue(optIdx) : 0
    readonly property double oMin:    root.optionModel ? root.optionModel.optMin(optIdx) : 0
    readonly property double oMax:    root.optionModel ? root.optionModel.optMax(optIdx) : 1
    readonly property double oStep:   root.optionModel ? root.optionModel.optStep(optIdx) : 1
    readonly property bool   oRO:     root.optionModel ? root.optionModel.optReadonly(optIdx) : false
    readonly property bool   oDirty:  root.optionModel ? root.optionModel.optIsDirty(optIdx) : false
    readonly property string oTip:    root.optionModel ? root.optionModel.optTooltip(optIdx) : ""
    readonly property string oUnit:   root.optionModel ? root.optionModel.optUnit(optIdx) : ""
    readonly property bool   oNullable: root.optionModel ? root.optionModel.optNullable(optIdx) : false
    readonly property bool   oIsVector: root.optionModel ? root.optionModel.optIsVector(optIdx) : false
    readonly property var    oEnumLabels: {
        if (!root.optionModel || root.oType !== "enum") return []
        return root.optionModel.optEnumLabelsList(root.optIdx)
    }

    // Content height dispatch (bool/enum 44px, numeric 56px, string 80px)
    readonly property int contentHeight:
        root.compact ? (root.oType === "string" ? 52 : 30) :
        (root.oType === "double" || root.oType === "int" || root.oType === "percent") ? 56
        : root.oType === "string" ? 80 : 44

    // Total height includes optional group header
    readonly property int totalHeight: (root.showGroupHeader ? (root.compact ? 22 : 28) : 0) + root.contentHeight

    // Tooltip
    ToolTip.visible: root.oTip !== "" && tipMA.containsMouse
    ToolTip.text: root.oTip
    ToolTip.delay: 500

    // Group header (visible when showGroupHeader)
    Rectangle {
        visible: root.showGroupHeader
        anchors.top: parent.top
        width: parent.width
        height: root.compact ? 22 : 28
        color: root.compact ? "transparent" : Theme.bgSurface

        Text {
            anchors.left: parent.left
            anchors.leftMargin: root.compact ? 10 : 20
            anchors.verticalCenter: parent.verticalCenter
            text: root.oGroup
            color: root.compact ? Theme.accent : Theme.textTertiary
            font.pixelSize: Theme.fontSizeSM
            font.bold: true
        }

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.borderSubtle
        }
    }

    // Parameter row
    Rectangle {
        id: paramRow
        y: root.showGroupHeader ? (root.compact ? 22 : 28) : 0
        width: parent.width
        height: root.contentHeight
        // Zebra striping
        color: (root.rowIndex + (root.showGroupHeader ? 1 : 0)) % 2 === 0 ? "transparent" : "#080b10"
        opacity: root.oRO ? 0.6 : 1.0

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: root.compact ? 10 : 20
            anchors.rightMargin: root.compact ? 10 : Theme.fontSizeLG
            spacing: root.compact ? Theme.spacingSM : Theme.spacingLG

            // Label column
            ColumnLayout {
                Layout.preferredWidth: root.compact ? root.compactLabelWidth : 180
                spacing: 2

                RowLayout {
                    spacing: Theme.spacingXS

                    // Dirty dot
                    Rectangle {
                        visible: root.oDirty
                        width: 6
                        height: 6
                        radius: 3
                        color: Theme.statusWarning
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Text {
                        text: root.oLabel
                        color: root.oRO ? Theme.textDisabled
                              : (root.oDirty ? Theme.statusWarning : Theme.textSecondary)
                        font.pixelSize: root.compact ? Theme.fontSizeSM : Theme.fontSizeMD
                        elide: Text.ElideRight
                        font.bold: root.oDirty || root.searchText !== ""
                        Layout.fillWidth: true
                    }
                }

                // Value-source indicator (for nullable options)
                Text {
                    visible: root.valueSource !== ""
                    text: root.displayValueSource(root.valueSource)
                    color: Theme.textTertiary
                    font.pixelSize: root.compact ? 9 : Theme.fontSizeXS
                }

                // Read-only tag
                Text {
                    visible: root.oRO
                    text: qsTr("Read-only")
                    color: "#3a4555"
                    font.pixelSize: 9
                }
            }

            // Control column
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // Bool: CxSwitch
                CxSwitch {
                    visible: root.oType === "bool"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    checked: root.oVal === true || root.oVal === "true"
                    enabled: !root.oRO
                    onToggled: root.optionModel.setValue(root.optIdx, checked)
                }

                // Int: CxSlider + CxSpinBox
                RowLayout {
                    visible: root.oType === "int"
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width
                    spacing: Theme.spacingSM

                    CxSlider {
                        visible: !root.compact
                        Layout.fillWidth: true
                        from: root.oMin
                        to: root.oMax
                        stepSize: root.oStep
                        value: typeof root.oVal === "number" ? root.oVal : root.oMin
                        enabled: !root.oRO
                        onMoved: root.optionModel.setValue(root.optIdx, value)
                    }

                    CxSpinBox {
                        width: root.compact ? root.compactFieldWidth : 90
                        height: root.compact ? 22 : Theme.controlHeightSM
                        value: typeof root.oVal === "number" ? Math.round(root.oVal) : 0
                        from: root.oMin
                        to: root.oMax
                        suffix: root.oUnit
                        enabled: !root.oRO
                        onValueModified: root.optionModel.setValue(root.optIdx, value)
                    }
                }

                // Double / Percent: CxSlider + TextInput(DoubleValidator) + suffix Text
                RowLayout {
                    visible: root.oType === "double" || root.oType === "percent"
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width
                    spacing: Theme.spacingSM

                    CxSlider {
                        visible: !root.compact
                        Layout.fillWidth: true
                        from: root.oMin
                        to: root.oMax
                        stepSize: root.oType === "percent" ? 0.01 : root.oStep
                        value: typeof root.oVal === "number" ? root.oVal : root.oMin
                        enabled: !root.oRO
                        onMoved: root.optionModel.setValue(root.optIdx, value)
                    }

                    Rectangle {
                        Layout.preferredWidth: root.compact ? root.compactFieldWidth : 64
                        height: root.compact ? 22 : 24
                        radius: Theme.radiusSM
                        color: Theme.bgInset
                        border.width: 1
                        border.color: numEdit.activeFocus
                                      ? Theme.borderFocus
                                      : (root.oRO ? Theme.borderSubtle : Theme.borderInput)

                        TextInput {
                            id: numEdit
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignRight
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeSM
                            font.bold: true
                            selectByMouse: true
                            readOnly: root.oRO
                            validator: DoubleValidator {
                                decimals: root.oType === "percent" ? 0 : 3
                                notation: DoubleValidator.StandardNotation
                            }
                            text: {
                                if (typeof root.oVal !== "number") return ""
                                return root.oType === "percent"
                                    ? Number(root.oVal).toFixed(0)
                                    : Number(root.oVal).toFixed(2)
                            }
                            onEditingFinished: {
                                if (!root.optionModel || root.oRO) return
                                var v = parseFloat(text)
                                if (isNaN(v)) return
                                if (v < root.oMin) v = root.oMin
                                if (v > root.oMax) v = root.oMax
                                root.optionModel.setValue(root.optIdx, v)
                            }
                        }
                    }

                    // Unit suffix text
                    Text {
                        visible: root.oUnit !== ""
                        text: root.oUnit
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                    }
                }

                // Enum: CxComboBox
                CxComboBox {
                    visible: root.oType === "enum"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    width: root.compact ? root.compactEnumWidth : 160
                    enabled: !root.oRO
                    model: root.oEnumLabels
                    currentIndex: typeof root.oVal === "number" ? root.oVal : 0
                    onActivated: (i) => root.optionModel.setValue(root.optIdx, i)
                }

                // String: ScrollView + CxTextArea
                ScrollView {
                    visible: root.oType === "string"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: root.compact ? 44 : 60
                    clip: true

                    CxTextArea {
                        text: typeof root.oVal === "string" ? root.oVal : (root.oVal ? root.oVal.toString() : "")
                        font.pixelSize: Theme.fontSizeSM
                        font.family: "Consolas"
                        color: Theme.textSecondary
                        readOnly: root.oRO
                        wrapMode: TextArea.Wrap
                        background: Rectangle {
                            radius: 4
                            color: Theme.bgInset
                            border.color: parent.activeFocus ? Theme.accent : Theme.borderInput
                            border.width: 1
                        }
                        onTextChanged: {
                            if (root.optionModel && activeFocus)
                                root.optionModel.setValue(root.optIdx, text)
                        }
                    }
                }
            }
        }
    }

    // Tooltip hover area
    MouseArea {
        id: tipMA
        anchors.fill: paramRow
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        z: -1
    }
}
