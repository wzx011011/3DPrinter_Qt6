// OptionRow.qml - compact typed-option renderer for settings dialogs.
//
// Presentation only: all durable option semantics stay in ConfigOptionModel
// and ConfigViewModel. Edits continue to route through optionModel.setValue().

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

Item {
    id: root

    required property var optionModel
    required property int optIdx
    required property int rowIndex

    property string searchText: ""
    property bool showGroupHeader: false
    property string oGroup: ""
    property string valueSource: ""
    property bool compact: false
    property int compactLabelWidth: 112
    property int compactFieldWidth: 72
    property int compactEnumWidth: 112

    function displayValueSource(sourceKey) {
        if (sourceKey === "default") return qsTr("Default")
        if (sourceKey === "print") return qsTr("Process")
        if (sourceKey === "filament") return qsTr("Material")
        if (sourceKey === "printer") return qsTr("Printer")
        return sourceKey
    }

    function displayOptionLabel(key, label) {
        var labels = {
            "layer_height": qsTr("Layer height"),
            "initial_layer_print_height": qsTr("Initial layer height"),
            "line_width": qsTr("Line width"),
            "initial_layer_line_width": qsTr("Initial layer line width"),
            "wall_loops": qsTr("Wall loops"),
            "top_shell_layers": qsTr("Top shell layers"),
            "bottom_shell_layers": qsTr("Bottom shell layers"),
            "sparse_infill_density": qsTr("Sparse infill density"),
            "sparse_infill_pattern": qsTr("Sparse infill pattern"),
            "outer_wall_speed": qsTr("Outer wall speed"),
            "inner_wall_speed": qsTr("Inner wall speed"),
            "sparse_infill_speed": qsTr("Sparse infill speed"),
            "top_surface_speed": qsTr("Top surface speed"),
            "travel_speed": qsTr("Travel speed"),
            "initial_layer_speed": qsTr("Initial layer speed"),
            "enable_support": qsTr("Enable support"),
            "support_density": qsTr("Support density"),
            "support_type": qsTr("Support type")
        }
        return labels[key] || label
    }

    function normalizedNumber(value, fallbackValue) {
        if (typeof value === "number")
            return value
        var parsed = parseFloat(value)
        return isNaN(parsed) ? fallbackValue : parsed
    }

    function formattedNumber(value) {
        var numberValue = root.normalizedNumber(value, root.oMin)
        if (root.oType === "int" || root.oType === "percent")
            return Math.round(numberValue).toString()
        var rounded = Math.round(numberValue * 1000) / 1000
        return rounded.toFixed(rounded % 1 === 0 ? 0 : 2)
    }

    function clampNumber(value) {
        var numberValue = root.normalizedNumber(value, root.oMin)
        if (numberValue < root.oMin) numberValue = root.oMin
        if (numberValue > root.oMax) numberValue = root.oMax
        return root.oType === "int" || root.oType === "percent"
            ? Math.round(numberValue)
            : numberValue
    }

    function setNumericValue(value) {
        if (!root.optionModel || root.oRO)
            return
        root.optionModel.setValue(root.optIdx, root.clampNumber(value))
    }

    readonly property string oType: root.optionModel ? root.optionModel.optType(optIdx) : ""
    readonly property string oKey: root.optionModel ? root.optionModel.optKey(optIdx) : ""
    readonly property string oLabel: root.optionModel ? root.displayOptionLabel(root.oKey, root.optionModel.optLabel(optIdx)) : ""
    readonly property var oVal: root.optionModel ? root.optionModel.optValue(optIdx) : 0
    readonly property double oMin: root.optionModel ? root.optionModel.optMin(optIdx) : 0
    readonly property double oMax: root.optionModel ? root.optionModel.optMax(optIdx) : 1
    readonly property double oStep: root.optionModel ? root.optionModel.optStep(optIdx) : 1
    readonly property bool oRO: root.optionModel ? root.optionModel.optReadonly(optIdx) : false
    readonly property bool oDirty: root.optionModel ? root.optionModel.optIsDirty(optIdx) : false
    readonly property string oTip: root.optionModel ? root.optionModel.optTooltip(optIdx) : ""
    readonly property string oUnit: root.optionModel ? root.optionModel.optUnit(optIdx) : ""
    readonly property string oSidetext: root.optionModel ? root.optionModel.optSidetext(root.optIdx) : ""
    readonly property string displayUnit: root.oSidetext !== "" ? root.oSidetext : root.oUnit
    readonly property bool oNullable: root.optionModel ? root.optionModel.optNullable(optIdx) : false
    readonly property bool oIsVector: root.optionModel ? root.optionModel.optIsVector(optIdx) : false
    readonly property var oEnumLabels: {
        if (!root.optionModel || root.oType !== "enum") return []
        return root.optionModel.optEnumLabelsList(root.optIdx)
    }

    readonly property bool isNumeric: root.oType === "int" || root.oType === "double" || root.oType === "percent"
    readonly property bool isRangeLike:
        root.isNumeric && (root.oKey.indexOf("_range") >= 0
            || root.oKey.indexOf("_min") >= 0
            || root.oKey.indexOf("_max") >= 0
            || root.oKey === "fan_min_speed"
            || root.oKey === "fan_max_speed"
            || root.oKey === "nozzle_temperature_range"
            || root.oLabel.toLowerCase().indexOf("range") >= 0)
    readonly property bool isColorLike:
        root.oKey.toLowerCase().indexOf("colour") >= 0
        || root.oKey.toLowerCase().indexOf("color") >= 0
    readonly property bool hasBounds: root.isNumeric && root.oMax > root.oMin

    readonly property int headerHeight: root.showGroupHeader ? (root.compact ? 28 : 32) : 0
    readonly property int rowHeight:
        root.compact ? (root.oType === "string" ? 48 : 34)
        : root.oType === "string" ? 70
        : 44
    readonly property int contentHeight: root.rowHeight
    readonly property int totalHeight: root.headerHeight + root.rowHeight
    readonly property int metadataLaneWidth: root.compact ? 88 : 112
    readonly property int controlColumnWidth:
        root.compact
        ? Math.max(root.compactEnumWidth, root.compactFieldWidth + 76)
        : 230

    ToolTip.visible: root.oTip !== "" && tipMA.containsMouse
    ToolTip.text: root.oTip
    ToolTip.delay: 500

    Rectangle {
        id: sectionHeader
        visible: root.showGroupHeader
        anchors.top: parent.top
        width: parent.width
        height: root.headerHeight
        color: "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: root.compact ? 6 : 14
            anchors.rightMargin: root.compact ? 10 : 18
            spacing: 8

            Rectangle {
                id: sectionIconRail
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18
                radius: 3
                color: "transparent"
                border.width: 1
                border.color: Theme.accent

                Text {
                    id: sectionGlyph
                    anchors.centerIn: parent
                    text: "+"
                    color: Theme.accent
                    font.pixelSize: 12
                    font.bold: true
                }
            }

            Text {
                text: root.oGroup
                color: Theme.textSecondary
                font.pixelSize: root.compact ? Theme.fontSizeSM : Theme.fontSizeMD
                font.bold: true
                elide: Text.ElideRight
                Layout.alignment: Qt.AlignVCenter
            }

            Rectangle {
                id: sectionDivider
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Theme.borderSubtle
                opacity: 0.85
            }
        }
    }

    Rectangle {
        id: paramRow
        y: root.headerHeight
        width: parent.width
        height: root.rowHeight
        color: (root.rowIndex + (root.showGroupHeader ? 1 : 0)) % 2 === 0
               ? "transparent" : Theme.bgBase
        opacity: root.oRO ? 0.72 : 1.0

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: root.compact ? 10 : 20
            anchors.rightMargin: root.compact ? 10 : Theme.fontSizeLG
            spacing: root.compact ? 8 : 12

            Item {
                Layout.preferredWidth: root.compact ? root.compactLabelWidth : 180
                Layout.fillHeight: true

                RowLayout {
                    anchors.fill: parent
                    spacing: 6

                    Rectangle {
                        id: dirtyBadge
                        visible: root.oDirty
                        Layout.preferredWidth: 6
                        Layout.preferredHeight: 6
                        radius: 3
                        color: Theme.statusWarning
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.oLabel
                        color: root.oRO ? Theme.textDisabled
                              : root.oDirty ? Theme.statusWarning
                              : Theme.textSecondary
                        font.pixelSize: root.compact ? Theme.fontSizeSM : Theme.fontSizeMD
                        font.bold: root.oDirty || root.searchText !== ""
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            RowLayout {
                id: metadataLane
                Layout.preferredWidth: root.metadataLaneWidth
                Layout.minimumWidth: root.metadataLaneWidth
                Layout.maximumWidth: root.metadataLaneWidth
                Layout.fillHeight: true
                spacing: 4

                Badge {
                    id: sourceBadge
                    visible: root.valueSource !== ""
                    label: root.displayValueSource(root.valueSource)
                    colorToken: Theme.textTertiary
                    fillToken: Theme.bgInset
                }

                Badge {
                    id: readOnlyBadge
                    visible: root.oRO
                    label: "RO"
                    colorToken: Theme.textDisabled
                    fillToken: Theme.bgPanel
                }

                Badge {
                    id: nullableBadge
                    visible: root.oNullable
                    label: "Inh"
                    colorToken: Theme.statusInfo
                    fillToken: Theme.bgInset
                }

                Badge {
                    id: vectorBadge
                    visible: root.oIsVector
                    label: "E"
                    colorToken: Theme.accent
                    fillToken: Theme.bgInset
                }

                Badge {
                    id: boundsBadge
                    visible: root.hasBounds
                    label: "rng"
                    colorToken: Theme.textTertiary
                    fillToken: Theme.bgInset
                }
            }

            Item {
                id: controlCell
                Layout.fillWidth: true
                Layout.fillHeight: true

                CxCheckBox {
                    visible: root.oType === "bool"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    text: ""
                    checked: root.oVal === true || root.oVal === "true"
                    enabled: !root.oRO
                    onToggled: root.optionModel.setValue(root.optIdx, checked)
                }

                RowLayout {
                    id: numericCluster
                    visible: root.isNumeric && !root.isRangeLike
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    width: Math.min(parent.width, root.controlColumnWidth)
                    spacing: 6

                    CxSpinBox {
                        visible: root.oType === "int" || root.oType === "percent"
                        Layout.preferredWidth: root.compact ? root.compactFieldWidth : 90
                        Layout.preferredHeight: root.compact ? 24 : Theme.controlHeightSM
                        value: root.clampNumber(root.oVal)
                        from: Math.round(root.oMin)
                        to: Math.round(root.oMax)
                        stepSize: Math.max(1, Math.round(root.oStep))
                        suffix: root.displayUnit
                        enabled: !root.oRO
                        editable: true
                        onValueModified: root.optionModel.setValue(root.optIdx, value)
                    }

                    NumericEdit {
                        visible: root.oType === "double"
                        Layout.preferredWidth: root.compact ? root.compactFieldWidth : 90
                        height: root.compact ? 24 : Theme.controlHeightSM
                        text: root.formattedNumber(root.oVal)
                        enabled: !root.oRO
                        onCommit: (valueText) => root.setNumericValue(valueText)
                    }

                    Text {
                        visible: root.displayUnit !== "" && root.oType === "double"
                        text: root.displayUnit
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                        Layout.alignment: Qt.AlignVCenter
                    }
                }

                RowLayout {
                    id: rangeCluster
                    visible: root.isRangeLike
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    width: Math.min(parent.width, root.compact ? 360 : 420)
                    spacing: 8

                    Text {
                        id: rangeMinLabel
                        text: qsTr("Min")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                        Layout.alignment: Qt.AlignVCenter
                    }

                    NumericEdit {
                        id: rangeMinEditor
                        Layout.preferredWidth: root.compactFieldWidth
                        height: root.compact ? 24 : Theme.controlHeightSM
                        text: root.formattedNumber(root.oVal)
                        enabled: !root.oRO
                        onCommit: (valueText) => root.setNumericValue(valueText)
                    }

                    Text {
                        visible: root.displayUnit !== ""
                        text: root.displayUnit
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Text {
                        id: rangeMaxLabel
                        text: qsTr("Max")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                        Layout.alignment: Qt.AlignVCenter
                    }

                    NumericEdit {
                        id: rangeMaxEditor
                        Layout.preferredWidth: root.compactFieldWidth
                        height: root.compact ? 24 : Theme.controlHeightSM
                        text: root.formattedNumber(root.oMax)
                        enabled: !root.oRO
                        onCommit: (valueText) => root.setNumericValue(valueText)
                    }

                    Text {
                        visible: root.displayUnit !== ""
                        text: root.displayUnit
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                        Layout.alignment: Qt.AlignVCenter
                    }
                }

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

                RowLayout {
                    visible: root.oType === "string" && root.isColorLike
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    width: Math.min(parent.width, root.controlColumnWidth)
                    spacing: 6

                    CxIconButton {
                        id: colorSwatchButton
                        buttonSize: root.compact ? 24 : 28
                        iconSize: 0
                        enabled: !root.oRO
                        toolTipText: qsTr("Color")
                        onClicked: root.optionModel.setValue(root.optIdx, root.oVal)

                        Rectangle {
                            id: colorSwatch
                            anchors.centerIn: parent
                            width: 14
                            height: 14
                            radius: 2
                            color: (typeof root.oVal === "string" && root.oVal.length > 0)
                                   ? root.oVal : Theme.accent
                            border.color: Theme.borderDefault
                            border.width: 1
                        }
                    }

                    CxTextField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.compact ? 24 : Theme.controlHeightSM
                        text: typeof root.oVal === "string" ? root.oVal : ""
                        enabled: !root.oRO
                        onEditingFinished: root.optionModel.setValue(root.optIdx, text)
                    }
                }

                CxTextArea {
                    visible: root.oType === "string" && !root.isColorLike
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: root.compact ? 42 : 60
                    text: typeof root.oVal === "string" ? root.oVal : (root.oVal ? root.oVal.toString() : "")
                    font.pixelSize: Theme.fontSizeSM
                    readOnly: root.oRO
                    wrapMode: TextArea.Wrap
                    onTextChanged: {
                        if (root.optionModel && activeFocus)
                            root.optionModel.setValue(root.optIdx, text)
                    }
                }
            }
        }
    }

    MouseArea {
        id: tipMA
        anchors.fill: paramRow
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        z: -1
    }

    component Badge: Rectangle {
        property string label: ""
        property color colorToken: Theme.textTertiary
        property color fillToken: Theme.bgInset

        Layout.preferredWidth: Math.max(18, badgeText.implicitWidth + 8)
        Layout.preferredHeight: 16
        radius: 3
        color: fillToken
        border.width: 1
        border.color: Theme.borderSubtle

        Text {
            id: badgeText
            anchors.centerIn: parent
            text: parent.label
            color: parent.colorToken
            font.pixelSize: 9
            font.bold: true
        }
    }

    component NumericEdit: Rectangle {
        id: editor
        property alias text: edit.text
        signal commit(string valueText)

        radius: Theme.radiusSM
        color: Theme.bgInset
        border.width: 1
        border.color: edit.activeFocus ? Theme.borderFocus
                     : enabled ? Theme.borderInput
                     : Theme.borderSubtle
        opacity: enabled ? 1.0 : 0.55

        TextInput {
            id: edit
            anchors.fill: parent
            anchors.leftMargin: 6
            anchors.rightMargin: 6
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignRight
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeSM
            font.bold: true
            selectByMouse: true
            readOnly: !editor.enabled
            validator: DoubleValidator {
                decimals: root.oType === "int" || root.oType === "percent" ? 0 : 3
                notation: DoubleValidator.StandardNotation
            }
            onEditingFinished: editor.commit(text)
        }
    }
}
