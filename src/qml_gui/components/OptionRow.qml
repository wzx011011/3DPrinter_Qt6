// OptionRow.qml - compact typed-option renderer for settings dialogs.
//
// Presentation only: all durable option semantics stay in ConfigOptionModel
// and ConfigViewModel. Edits continue to route through optionModel.setValue().

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OWzx.Models 1.0
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
        if (sourceKey === "default") return qsTr("默认")
        if (sourceKey === "print") return qsTr("工艺")
        if (sourceKey === "filament") return qsTr("耗材")
        if (sourceKey === "printer") return qsTr("打印机")
        return sourceKey
    }

    // zh_CN group titles for the upstream Tab.cpp option groups
    // (new_optgroup titles, OrcaSlicer_zh_CN.po translations).
    function displayGroupLabel(group) {
        var groups = {
            "Layer height": qsTr("层高"),
            "Line width": qsTr("线宽"),
            "Seam": qsTr("接缝"),
            "Precision": qsTr("精度"),
            "Ironing": qsTr("熨烫"),
            "Z contouring": qsTr("Z 层抗锯齿"),
            "Wall generator": qsTr("墙生成器"),
            "Walls and surfaces": qsTr("墙壁和表面"),
            "Bridging": qsTr("搭桥"),
            "Overhangs": qsTr("悬垂"),
            "Walls": qsTr("墙"),
            "Top/bottom shells": qsTr("顶部/底部外壳"),
            "Infill": qsTr("填充"),
            "Advanced": qsTr("高级"),
            "First layer speed": qsTr("首层速度"),
            "Other layers speed": qsTr("其他层速度"),
            "Overhang speed": qsTr("悬垂速度"),
            "Travel speed": qsTr("空驶速度"),
            "Acceleration": qsTr("加速度"),
            "Junction Deviation": qsTr("结点偏差"),
            "Jerk(XY)": qsTr("抖动（XY轴）"),
            "Support": qsTr("支撑"),
            "Raft": qsTr("筏层"),
            "Filament for Supports": qsTr("支撑耗材"),
            "Support ironing": qsTr("支撑熨烫"),
            "Tree supports": qsTr("树状支撑"),
            "Prime tower": qsTr("擦拭塔"),
            "Filament for Features": qsTr("特征用耗材"),
            "Ooze prevention": qsTr("Ooze 预防"),
            "Flush options": qsTr("换料冲刷选项"),
            "Skirt": qsTr("裙边"),
            "Brim": qsTr("Brim"),
            "Special mode": qsTr("特殊模式"),
            "Fuzzy skin": qsTr("绒毛表面"),
            "G-code output": qsTr("G-code 输出"),
            "Plugin Configuration": qsTr("插件配置")
        }
        return groups[group] || group
    }

    // Group icons matching the upstream new_optgroup icon names
    // (Tab.cpp:2638-3125). Static decoration only.
    function groupIconSource(group) {
        var icons = {
            "Layer height": "qrc:/qml/assets/icons/param_layer_height.svg",
            "Line width": "qrc:/qml/assets/icons/param_line_width.svg",
            "Seam": "qrc:/qml/assets/icons/param_seam.svg",
            "Precision": "qrc:/qml/assets/icons/param_precision.svg",
            "Ironing": "qrc:/qml/assets/icons/param_ironing.svg",
            "Z contouring": "qrc:/qml/assets/icons/param_z_contouring.svg",
            "Wall generator": "qrc:/qml/assets/icons/param_wall_generator.svg",
            "Walls and surfaces": "qrc:/qml/assets/icons/param_wall_surface.svg",
            "Bridging": "qrc:/qml/assets/icons/param_bridge.svg",
            "Overhangs": "qrc:/qml/assets/icons/param_overhang.svg",
            "Walls": "qrc:/qml/assets/icons/param_wall.svg",
            "Top/bottom shells": "qrc:/qml/assets/icons/param_shell.svg",
            "Infill": "qrc:/qml/assets/icons/param_infill.svg",
            "Advanced": "qrc:/qml/assets/icons/param_advanced.svg",
            "First layer speed": "qrc:/qml/assets/icons/param_speed_first.svg",
            "Other layers speed": "qrc:/qml/assets/icons/param_speed.svg",
            "Overhang speed": "qrc:/qml/assets/icons/param_overhang_speed.svg",
            "Travel speed": "qrc:/qml/assets/icons/param_travel_speed.svg",
            "Acceleration": "qrc:/qml/assets/icons/param_acceleration.svg",
            "Junction Deviation": "qrc:/qml/assets/icons/param_junction_deviation.svg",
            "Jerk(XY)": "qrc:/qml/assets/icons/param_jerk.svg",
            "Support": "qrc:/qml/assets/icons/param_support.svg",
            "Raft": "qrc:/qml/assets/icons/param_raft.svg",
            "Filament for Supports": "qrc:/qml/assets/icons/param_support_filament.svg",
            "Support ironing": "qrc:/qml/assets/icons/param_ironing.svg",
            "Tree supports": "qrc:/qml/assets/icons/param_support_tree.svg",
            "Prime tower": "qrc:/qml/assets/icons/param_tower.svg",
            "Filament for Features": "qrc:/qml/assets/icons/param_filament_for_features.svg",
            "Ooze prevention": "qrc:/qml/assets/icons/param_ooze_prevention.svg",
            "Flush options": "qrc:/qml/assets/icons/param_flush.svg",
            "Skirt": "qrc:/qml/assets/icons/param_skirt.svg",
            "Brim": "qrc:/qml/assets/icons/param_adhension.svg",
            "Special mode": "qrc:/qml/assets/icons/param_special.svg",
            // Upstream fuzzy_skin group icon (Tab.cpp:3072) has no
            // counterpart in the project icon set (fuzzy_skin.svg is absent
            // from disk and from qml.qrc); param_rectilinear is the closest
            // registered line-texture glyph so the header stays filled.
            "Fuzzy skin": "qrc:/qml/assets/icons/param_rectilinear.svg",
            "G-code output": "qrc:/qml/assets/icons/param_gcode.svg",
            "Plugin Configuration": "qrc:/qml/assets/icons/param_gcode.svg"
        }
        return icons[group] || ""
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

    // Row data is role-fed by the owning delegate (ConfigOptionFilterProxy
    // roles over ConfigOptionModel). The defaults keep the historical
    // no-model fallback semantics so a standalone instantiation still
    // renders harmlessly.
    property string oType: ""
    property string oKey: ""
    property string oLabel: ""
    property var oVal: 0
    property double oMin: 0.0
    property double oMax: 1.0
    property double oStep: 1.0
    property bool oRO: false
    property bool oDirty: false
    property string oTip: ""
    property string oUnit: ""
    property string oSidetext: ""
    readonly property string displayUnit: root.oSidetext !== "" ? root.oSidetext : root.oUnit
    property bool oNullable: false
    property bool oIsVector: false
    property var oEnumLabels: []

    readonly property bool isNumeric: root.oType === "int" || root.oType === "double" || root.oType === "percent"
    // optMin/optMax are ConfigOption schema bounds, not a two-value option.
    // Keep range-like keys identifiable so their permitted interval can be
    // shown without presenting the bounds as independently editable values.
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
    // Phase 236 (DLG-01): keys whose value is edited in a dedicated dialog
    // rather than the inline field — G-code fields (machine_start_gcode,
    // machine_end_gcode, ...) open EditGCodeDialog; the bed geometry keys
    // (printable_area / bed_shape) open BedShapeDialog. Mirrors upstream
    // ConfigOptionsGroup button pickers for these option types.
    readonly property bool isGcodeOption:
        root.oType === "string" && root.oKey.length > 6
        && root.oKey.slice(-6) === "_gcode"
    readonly property bool isBedShapeOption:
        root.oKey === "printable_area" || root.oKey === "bed_shape"
    readonly property bool hasTrailingDialogAction: root.isGcodeOption || root.isBedShapeOption
    readonly property bool hasBounds: root.isNumeric && root.oMax > root.oMin

    readonly property int headerHeight: root.showGroupHeader ? (root.compact ? 28 : 32) : 0
    // G3: R11 compact row rhythm is 30px (was 34).
    readonly property int rowHeight:
        root.compact ? (root.oType === "string" ? 48 : 30)
        : root.oType === "string" ? 70
        : 44
    readonly property int contentHeight: root.rowHeight
    readonly property int totalHeight: root.headerHeight + root.rowHeight
    readonly property int controlColumnWidth:
        root.compact
        ? Math.max(root.compactEnumWidth, root.compactFieldWidth + 76)
        : 230
    // BUILDGATE restore (2026-09-24): the fixed state-indicator lane is back
    // (settingsOptionRowsRestorePhase86ControlContract locks its ids; the
    // hover tooltip below keeps summarizing the same metadata).
    readonly property int metadataLaneWidth: root.compact ? 88 : 112
    // Badge count currently rendered in the lane. Four simultaneous badges
    // (~120px of implicit widths) overflow the fixed 88px compact lane, so
    // the bounds badge stands down on crowded rows -- its interval stays
    // available in the row tooltip (metadataSummary below).
    readonly property int metadataBadgeCount:
        (root.valueSource !== "" ? 1 : 0)
        + (root.oRO ? 1 : 0)
        + (root.oNullable ? 1 : 0)
        + (root.oIsVector ? 1 : 0)
        + (root.hasBounds ? 1 : 0)
    // Row metadata (value source / RO / inherit / vector / bounds) that used
    // to render as an inline badge lane now folds into the hover tooltip
    // (ref layout: label + single field only).
    readonly property string metadataSummary: {
        var parts = []
        if (root.valueSource !== "")
            parts.push(root.displayValueSource(root.valueSource))
        if (root.oRO)
            parts.push("RO")
        if (root.oNullable)
            parts.push(qsTr("继承"))
        if (root.oIsVector)
            parts.push(qsTr("多值"))
        if (root.hasBounds)
            parts.push(qsTr("范围") + " " + root.formattedNumber(root.oMin)
                       + " - " + root.formattedNumber(root.oMax))
        return parts.join(" · ")
    }
    readonly property string rowTooltip:
        root.oTip !== "" && root.metadataSummary !== ""
        ? root.oTip + "\n" + root.metadataSummary
        : (root.metadataSummary !== "" ? root.metadataSummary : root.oTip)

    ToolTip.visible: root.rowTooltip !== "" && tipMA.containsMouse
    ToolTip.text: root.rowTooltip
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
            spacing: Theme.spacingXS  // upstream StaticLine: icon + 5px + title + line

            // Upstream OptionsGroup renders its title through StaticLine with
            // an 18px group icon (StaticLine.cpp:37,93-115). Still a static
            // title -- the icon is decoration with no click affordance.
            Image {
                visible: root.groupIconSource(root.oGroup) !== ""
                source: root.groupIconSource(root.oGroup)
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18
                fillMode: Image.PreserveAspectFit
            }

            Text {
                text: root.displayGroupLabel(root.oGroup)
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLG
                font.bold: true
                elide: Text.ElideRight
                Layout.alignment: Qt.AlignVCenter
            }

            Rectangle {
                id: sectionDivider
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Theme.bgPanel
            }
        }
    }

    Rectangle {
        id: paramRow
        y: root.headerHeight
        width: parent.width
        height: root.rowHeight
        // Ref/upstream rows share the panel background -- no zebra striping.
        color: "transparent"
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
                        radius: Theme.radiusSM
                        color: Theme.statusWarning
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.oLabel
                        color: root.oRO ? Theme.textDisabled
                              : root.oDirty ? Theme.statusWarning
                              : root.compact ? Theme.textPrimary
                              : Theme.textSecondary
                        font.pixelSize: root.compact ? Theme.fontSizeLG : Theme.fontSizeMD
                        font.bold: root.oDirty || root.searchText !== ""
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            // BUILDGATE restore (2026-09-24): the Phase 86 fixed badge lane,
            // verbatim from the last committed revision — the audit
            // (settingsOptionRowsRestorePhase86ControlContract) still locks
            // these ids even though the summary also folds into the tooltip.
            RowLayout {
                id: metadataLane
                Layout.preferredWidth: root.metadataLaneWidth
                Layout.minimumWidth: root.metadataLaneWidth
                Layout.maximumWidth: root.metadataLaneWidth
                Layout.fillHeight: true
                spacing: 4

                CxBadge {
                    id: sourceBadge
                    visible: root.valueSource !== ""
                    label: root.displayValueSource(root.valueSource)
                    colorToken: Theme.textTertiary
                    fillToken: Theme.bgInset
                }

                CxBadge {
                    id: readOnlyBadge
                    visible: root.oRO
                    label: "RO"
                    colorToken: Theme.textDisabled
                    fillToken: Theme.bgPanel
                }

                CxBadge {
                    id: nullableBadge
                    visible: root.oNullable
                    label: "Inh"
                    colorToken: Theme.statusInfo
                    fillToken: Theme.bgInset
                }

                CxBadge {
                    id: vectorBadge
                    visible: root.oIsVector
                    label: "E"
                    colorToken: Theme.accent
                    fillToken: Theme.bgInset
                }

                CxBadge {
                    id: boundsBadge
                    visible: root.hasBounds && root.metadataBadgeCount <= 3
                    label: "rng"
                    colorToken: Theme.textTertiary
                    fillToken: Theme.bgInset
                }
            }

            Item {
                id: controlCell
                Layout.fillWidth: true
                Layout.fillHeight: true

                // Per-type clusters live behind Loaders so a row only
                // instantiates the control cluster its option type needs
                // (category switches rebuild the whole delegate set; paying
                // for every cluster per row dominated that rebuild).
                // Geometry (anchors/width/z) moved verbatim onto the Loader;
                // the loaded root fills it. All load asynchronously: a tab
                // switch rebuilds every visible row at once, so the shell
                // (label + badges) paints first and controls fill in after.
                Loader {
                    asynchronous: true
                    active: root.oType === "bool"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    sourceComponent: Component {
                        CxCheckBox {
                            anchors.fill: parent
                            text: ""
                            checked: root.oVal === true || root.oVal === "true"
                            enabled: !root.oRO
                            onToggled: root.optionModel.setValue(root.optIdx, checked)
                        }
                    }
                }

                Loader {
                    id: numericLoader
                    asynchronous: true
                    active: root.isNumeric
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    width: Math.min(parent.width, root.controlColumnWidth)
                    sourceComponent: Component {
                        RowLayout {
                            id: numericCluster
                            anchors.fill: parent
                            spacing: 6

                            CxSpinBox {
                                visible: root.oType === "int" || root.oType === "percent"
                                Layout.preferredWidth: root.compact ? root.compactFieldWidth : 90
                                // G2: R11 compact field is 25px tall (was 24).
                                Layout.preferredHeight: root.compact ? 25 : Theme.controlHeightSM
                                value: root.clampNumber(root.oVal)
                                from: Math.round(root.oMin)
                                to: Math.round(root.oMax)
                                stepSize: Math.max(1, Math.round(root.oStep))
                                suffix: root.displayUnit
                                enabled: !root.oRO
                                editable: true
                                onValueModified: root.optionModel.setValue(root.optIdx, value)
                            }

                            CxNumericEdit {
                                visible: root.oType === "double"
                                Layout.preferredWidth: root.compact ? root.compactFieldWidth : 120
                                // G2: R11 compact field is 25px tall (was 24).
                                Layout.preferredHeight: root.compact ? 25 : Theme.controlHeightSM
                                decimals: root.oType === "int" || root.oType === "percent" ? 0 : 3
                                text: root.formattedNumber(root.oVal)
                                // Unit lives inside the field, right-aligned
                                // (upstream Field.cpp:923 combine_side_text).
                                suffix: root.displayUnit
                                enabled: !root.oRO
                                onCommit: (valueText) => root.setNumericValue(valueText)
                            }
                        }
                    }
                }

                // isRangeLike implies isNumeric, so numericLoader is loaded
                // whenever this cluster exists and the cross-cluster anchor
                // target is always a live Item.
                Loader {
                    id: rangeLoader
                    asynchronous: true
                    active: root.isRangeLike
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: numericLoader.left
                    anchors.rightMargin: 8
                    sourceComponent: Component {
                        RowLayout {
                            id: rangeCluster
                            anchors.fill: parent
                            spacing: 4

                            Text {
                                id: rangeMinLabel
                                text: qsTr("Min")
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSizeXS
                                Layout.alignment: Qt.AlignVCenter
                            }

                            Text {
                                id: rangeMinEditor
                                text: root.formattedNumber(root.oMin)
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeXS
                                Layout.alignment: Qt.AlignVCenter
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

                            Text {
                                id: rangeMaxEditor
                                text: root.formattedNumber(root.oMax)
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeXS
                                Layout.alignment: Qt.AlignVCenter
                            }

                            Text {
                                visible: root.displayUnit !== ""
                                text: root.displayUnit
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSizeXS
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }
                    }
                }

                Loader {
                    asynchronous: true
                    active: root.oType === "enum"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    width: root.compact ? root.compactEnumWidth : 160
                    sourceComponent: Component {
                        CxComboBox {
                            anchors.fill: parent
                            enabled: !root.oRO
                            model: root.oEnumLabels
                            currentIndex: typeof root.oVal === "number" ? root.oVal : 0
                            onActivated: (i) => root.optionModel.setValue(root.optIdx, i)
                        }
                    }
                }

                Loader {
                    asynchronous: true
                    active: root.oType === "string" && root.isColorLike
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    width: Math.min(parent.width, root.controlColumnWidth)
                    sourceComponent: Component {
                        RowLayout {
                            anchors.fill: parent
                            spacing: 6

                            Rectangle {
                                id: colorSwatchButton
                                Layout.preferredWidth: root.compact ? 24 : 28
                                Layout.preferredHeight: root.compact ? 24 : 28
                                color: "transparent"

                                Rectangle {
                                    id: colorSwatch
                                    anchors.centerIn: parent
                                    width: 14
                                    height: 14
                                    radius: Theme.radiusXS
                                    color: (typeof root.oVal === "string" && root.oVal.length > 0)
                                           ? root.oVal : Theme.accent
                                    border.color: Theme.borderDefault
                                    border.width: 1
                                }

                                HoverHandler { id: colorSwatchHover }
                                ToolTip.visible: colorSwatchHover.hovered
                                ToolTip.text: qsTr("Edit the color value in the field")
                                ToolTip.delay: 500
                            }

                            CxTextField {
                                Layout.fillWidth: true
                                // G2: R11 compact field is 25px tall (was 24).
                                Layout.preferredHeight: root.compact ? 25 : Theme.controlHeightSM
                                text: typeof root.oVal === "string" ? root.oVal : ""
                                enabled: !root.oRO
                                onEditingFinished: root.optionModel.setValue(root.optIdx, text)
                            }
                        }
                    }
                }

                Loader {
                    asynchronous: true
                    active: root.oType === "string" && !root.isColorLike
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.rightMargin: root.hasTrailingDialogAction ? 34 : 0
                    height: root.compact ? 42 : 60
                    sourceComponent: Component {
                        CxTextArea {
                            anchors.fill: parent
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

                // Phase 236 (DLG-01): whole-option editor affordance. The
                // "edit" button next to G-code / bed-shape rows requests the
                // dedicated dialog from BackendContext, which routes the
                // current key + value through showEditGCodeDialogRequested /
                // showBedShapeDialogRequested (value forwarded so the dialog
                // opens with the preset's current text).
                Loader {
                    // Coexists with the string cluster (textarea gives it the
                    // 34px right margin above), hence the z lift.
                    asynchronous: true
                    active: root.hasTrailingDialogAction
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 2
                    z: 2
                    sourceComponent: Component {
                        Row {
                            anchors.fill: parent
                            spacing: 4

                            CxIconButton {
                                buttonSize: 24
                                iconSize: 13
                                cxStyle: CxIconButton.Style.Ghost
                                iconSource: root.isGcodeOption
                                    ? "qrc:/qml/assets/icons/clipboard.svg"
                                    : "qrc:/qml/assets/icons/settings.svg"
                                toolTipText: root.isGcodeOption
                                    ? qsTr("编辑 G-code…")
                                    : qsTr("编辑热床形状…")
                                enabled: true
                                onClicked: {
                                    if (typeof backend === "undefined" || !backend)
                                        return
                                    if (root.isGcodeOption)
                                        backend.showEditGCodeDialog(
                                            root.oKey,
                                            typeof root.oVal === "string" ? root.oVal
                                                : (root.oVal ? root.oVal.toString() : ""))
                                    else
                                        backend.showBedShapeDialog()
                                }
                            }
                        }
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
}
