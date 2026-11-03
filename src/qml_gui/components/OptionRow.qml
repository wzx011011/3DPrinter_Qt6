// OptionRow.qml - compact typed-option renderer for settings dialogs.
//
// Presentation only: all durable option semantics stay in ConfigOptionModel
// and ConfigViewModel. Edits continue to route through optionModel.setValue().

import QtQuick
pragma ComponentBehavior: Bound
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import OWzx

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
    // Reference sidebar rows (Orca prepare sidebar): labels indent deeper
    // than the group icon column. Default keeps the settings-dialog inset.
    property int compactIndent: 10

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
            // Upstream fuzzy_skin group icon (Tab.cpp:3072), recoloured copy.
            "Fuzzy skin": "qrc:/qml/assets/icons/param_fuzzy_skin.svg",
            "G-code output": "qrc:/qml/assets/icons/param_gcode.svg",
            "Plugin Configuration": "qrc:/qml/assets/icons/param_gcode.svg"
        }
        return icons[group] || ""
    }

    // Preset JSON stores bools as "1"/"0" (read back as 1/0) and vector
    // bools as lists; QML edits write true/false. All read the same way.
    function boolValue(value) {
        if (Array.isArray(value) || (value && typeof value === "object" && value.length !== undefined))
            value = value.length > 0 ? value[0] : false
        if (typeof value === "boolean")
            return value
        if (typeof value === "number")
            return value !== 0
        if (typeof value === "string")
            return value === "1" || value.toLowerCase() === "true"
        return false
    }

    function normalizedNumber(value, fallbackValue) {
        if (typeof value === "number")
            return value
        var parsed = parseFloat(value)
        return isNaN(parsed) ? fallbackValue : parsed
    }

    function formattedNumber(value) {
        var numberValue = root.normalizedNumber(value, root.oMin)
        // Unset config floats arrive as the ±FLT_MAX sentinel (±3.4e38,
        // e.g. an Ender-5 Max profile's retraction length). Render an empty
        // field instead of a 39-digit number (2026-10-26 acceptance catch).
        if (!isFinite(numberValue) || Math.abs(numberValue) > 1e30)
            return ""
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
    readonly property string displayUnit: {
        var st = root.oSidetext !== "" ? root.oSidetext : root.oUnit
        // Schema sidetexts arrive in English ("mm or %"); the known set is
        // translated inline — everything else passes through untouched.
        var map = { "mm or %": qsTr("mm 或 %"), "mm/s or %": qsTr("mm/s 或 %"),
                    "mm/s² or %": qsTr("mm/s² 或 %"),
                    // OrcaSlicer_zh_CN.po
                    "money/h": qsTr("钱/时"), "money/kg": qsTr("金额/kg") }
        return map[st] !== undefined ? map[st] : st
    }
    // Upstream toggle_field: the line stays visible but its field is
    // disabled (ConfigManipulation / Tab::toggle_options rules, computed in
    // core and fed through the model's optEnabled role).
    property bool oEnabled: true
    // Upstream ConfigOptionDef::multiline: G-code / notes / post-process
    // use a multi-line editor; every other string is one line.
    property bool oMultiline: false
    // Upstream multi-option Line: when lineHead is set this row is the line
    // head -- oLabel carries the line label and the control cell renders
    // every member (OptionLineFields). inlineMember marks the per-member
    // field instances inside a line (no label column, no badge lane).
    property string lineHead: ""
    property int lineViewMode: 0
    property bool inlineMember: false
    readonly property bool isLine: root.lineHead !== ""
    readonly property bool isMultilineString: root.oType === "string" && root.oMultiline
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
    // Upstream GUIType::color options (extruder_colour,
    // default_filament_colour, ...): typed "color" by the model.
    readonly property bool isColorLike: root.oType === "color"
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
    // printable_area renders as the upstream "Set..." button alone
    // (Tab.cpp create_bed_shape_widget), so only G-code keeps the trailing
    // icon action.
    readonly property bool hasTrailingDialogAction: root.isGcodeOption
    readonly property bool hasBounds: root.isNumeric && root.oMax > root.oMin

    readonly property int headerHeight: root.showGroupHeader ? (root.compact ? 28 : 32) : 0
    // Compact row rhythm 26px (2026-10-26 pixel audit: upstream runs a
    // 13px label on a 26px pitch — exactly 2x — vs our previous 30px).
    readonly property int rowHeight:
        root.isLine ? Math.max(26, lineFieldsLoader.item ? lineFieldsLoader.item.implicitHeight : 26)
        : root.compact ? (root.isMultilineString ? 48 : 26)
        : root.isMultilineString ? 70
        : 44
    readonly property int contentHeight: root.rowHeight
    // Single-line field height shared by every control cluster.
    readonly property int fieldHeight: root.compact ? 26 : Theme.controlHeightSM
    readonly property int totalHeight: root.headerHeight + root.rowHeight
    readonly property int controlColumnWidth:
        root.compact
        ? Math.max(root.compactEnumWidth, root.compactFieldWidth + 76)
        : 230
    // State-indicator lane, 2026-10-26 pixel-audit pass: only RO stays as
    // a badge (sparse by nature). Source / inherit / vector / bounds folded
    // into the hover tooltip (metadataSummary) — they rendered as two
    // always-on badges per row in the compact sidebar, which upstream does
    // not show at all, and the fixed lane squeezed the value field against
    // the panel edge.
    readonly property int metadataLaneWidth: root.compact ? 30 : 44
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
            anchors.leftMargin: root.compact ? 12 : 14
            anchors.rightMargin: root.compact ? 16 : 18
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
            anchors.leftMargin: root.inlineMember ? 0 : root.compact ? root.compactIndent : 20
            anchors.rightMargin: root.inlineMember ? 0 : root.compact ? 10 : Theme.fontSizeLG
            spacing: root.inlineMember ? 0 : root.compact ? 8 : 12

            Item {
                visible: !root.inlineMember
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
                visible: !root.inlineMember
                Layout.preferredWidth: root.metadataLaneWidth
                Layout.minimumWidth: root.metadataLaneWidth
                Layout.maximumWidth: root.metadataLaneWidth
                Layout.fillHeight: true
                spacing: 4

                // RO only: sparse by nature (readonly rows are rare).
                // Source/inherit/vector/bounds metadata lives in the row
                // tooltip (metadataSummary) — see metadataLaneWidth above.
                CxBadge {
                    id: readOnlyBadge
                    visible: root.oRO
                    label: "RO"
                    colorToken: Theme.textDisabled
                    fillToken: Theme.bgPanel
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
                    active: root.oType === "bool" && !root.isLine
                    anchors.verticalCenter: parent.verticalCenter
                    // Reference sidebar: the checkbox sits at the field
                    // column's LEFT edge (Orca renders bool rows the same
                    // way), not hard against the panel edge.
                    anchors.left: parent.left
                    // Explicit size: a bare Loader has no implicit size and
                    // the fill-anchored CxCheckBox collapsed to 0x0 — bool
                    // options rendered with no checkbox at all.
                    width: 28
                    height: root.fieldHeight
                    sourceComponent: Component {
                        CxCheckBox {
                            anchors.fill: parent
                            text: ""
                            checked: root.boolValue(root.oVal)
                            enabled: !root.oRO && root.oEnabled
                            onToggled: root.optionModel.setValue(root.optIdx, checked)
                        }
                    }
                }

                Loader {
                    id: numericLoader
                    asynchronous: true
                    active: root.isNumeric && !root.isLine
                    anchors.verticalCenter: parent.verticalCenter
                    // Upstream OptionsGroup grid: every field starts at the
                    // same column right after the label (bool, number and
                    // enum alike). An explicit height keeps the loaded
                    // cluster centred on the label -- a bare Loader is 0px
                    // tall, which pushed the field half its height down.
                    anchors.left: parent.left
                    width: Math.min(parent.width, root.controlColumnWidth)
                    height: root.fieldHeight
                    sourceComponent: Component {
                        RowLayout {
                            id: numericCluster
                            anchors.fill: parent
                            spacing: 6

                            CxSpinBox {
                                visible: root.oType === "int" || root.oType === "percent"
                                Layout.preferredWidth: root.compact ? root.compactFieldWidth : 90
                                // G2: R11 compact field is 25px tall (was 24).
                                Layout.preferredHeight: root.compact ? 26 : Theme.controlHeightSM
                                value: root.clampNumber(root.oVal)
                                from: Math.round(root.oMin)
                                to: Math.round(root.oMax)
                                stepSize: Math.max(1, Math.round(root.oStep))
                                suffix: root.displayUnit
                                enabled: !root.oRO && root.oEnabled
                                editable: true
                                onValueModified: root.optionModel.setValue(root.optIdx, value)
                            }

                            CxNumericEdit {
                                visible: root.oType === "double"
                                Layout.preferredWidth: root.compact ? root.compactFieldWidth : 120
                                // G2: R11 compact field is 25px tall (was 24).
                                Layout.preferredHeight: root.compact ? 26 : Theme.controlHeightSM
                                decimals: root.oType === "int" || root.oType === "percent" ? 0 : 3
                                valueAlignment: root.compact ? Text.AlignLeft : Text.AlignRight
                                text: root.formattedNumber(root.oVal)
                                // Unit lives inside the field, right-aligned
                                // (upstream Field.cpp:923 combine_side_text).
                                suffix: root.displayUnit
                                enabled: !root.oRO && root.oEnabled
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
                    // 2026-10-26 light-mode catch: in compact rows the
                    // Min/Max summary had no room next to label + field and
                    // rendered overlapped ("最小 0 mm" wrapped under the
                    // spinbox). The range already lives in the row tooltip
                    // (metadataSummary) — wide settings rows keep it.
                    active: root.isRangeLike && !root.compact && !root.isLine
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: numericLoader.right
                    anchors.right: parent.right
                    anchors.leftMargin: 8
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
                    active: root.oType === "enum" && !root.isLine
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    width: Math.min(parent.width, root.compact ? root.compactEnumWidth : 160)
                    // Without a height the fill-anchored combo collapsed to
                    // 0px and lost its frame (only the text was painted).
                    height: root.fieldHeight
                    sourceComponent: Component {
                        CxComboBox {
                            anchors.fill: parent
                            enabled: !root.oRO && root.oEnabled
                            model: root.oEnumLabels
                            // Preset enums are key strings ("klipper"); the model
                            // maps any stored form to the combo row and writes
                            // the key back (upstream preset / deserialize form).
                            currentIndex: root.optionModel ? root.optionModel.enumIndexOf(root.optIdx, root.oVal) : 0
                            onActivated: (i) => root.optionModel.setEnumIndex(root.optIdx, i)
                        }
                    }
                }

                // Upstream ColourPicker (Field.cpp, GUIType::color): a
                // button filled with the colour -- click opens the colour
                // dialog, right click resets to undefined (transparent with
                // the #F1754E diagonal, ColourPicker::set_undef_value).
                Loader {
                    asynchronous: true
                    active: root.isColorLike && !root.isLine
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    width: root.compact ? root.compactFieldWidth : 120
                    height: root.fieldHeight
                    sourceComponent: Component {
                        Rectangle {
                            id: colorSwatchButton
                            readonly property string colourText: root.optionModel
                                ? root.optionModel.colourOf(root.oVal) : ""
                            readonly property bool defined: colorSwatchButton.colourText !== ""
                            radius: Theme.radiusSM
                            border.color: colourArea.containsMouse ? Theme.accent : Theme.borderInput
                            border.width: 1
                            color: Theme.bgInput
                            opacity: root.oRO || !root.oEnabled ? 0.5 : 1.0

                            Rectangle {
                                id: colorSwatch
                                anchors.fill: parent
                                anchors.margins: 3
                                radius: Theme.radiusXS
                                color: colorSwatchButton.defined ? colorSwatchButton.colourText : "transparent"
                            }

                            // Undefined colour: one diagonal stroke.
                            Rectangle {
                                visible: !colorSwatchButton.defined
                                anchors.centerIn: parent
                                width: Math.sqrt(parent.width * parent.width + parent.height * parent.height) - 6
                                height: 1
                                color: "#F1754E"
                                rotation: -Math.atan2(parent.height, parent.width) * 180 / Math.PI
                            }

                            MouseArea {
                                id: colourArea
                                anchors.fill: parent
                                hoverEnabled: true
                                enabled: !root.oRO && root.oEnabled
                                acceptedButtons: Qt.LeftButton | Qt.RightButton
                                cursorShape: Qt.PointingHandCursor
                                onClicked: (mouse) => {
                                    if (mouse.button === Qt.RightButton) {
                                        root.optionModel.setColour(root.optIdx, "")
                                    } else {
                                        colourDialog.selectedColor = colorSwatchButton.defined ? colorSwatchButton.colourText : "white"
                                        colourDialog.open()
                                    }
                                }
                            }

                            ToolTip.visible: colourArea.containsMouse
                            ToolTip.delay: 500
                            ToolTip.text: colorSwatchButton.defined ? colorSwatchButton.colourText : qsTr("未定义")

                            ColorDialog {
                                id: colourDialog
                                title: root.oLabel
                                onAccepted: root.optionModel.setColour(root.optIdx, selectedColor.toString().toUpperCase())
                            }
                        }
                    }
                }

                Loader {
                    asynchronous: true
                    active: root.oType === "string" && !root.isColorLike && !root.isMultilineString && !root.isLine
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.rightMargin: root.hasTrailingDialogAction ? 34 : 0
                    height: root.fieldHeight
                    sourceComponent: Component {
                        CxTextField {
                            text: typeof root.oVal === "string" ? root.oVal : (root.oVal ? root.oVal.toString() : "")
                            enabled: !root.oRO && root.oEnabled
                            onEditingFinished: root.optionModel.setValue(root.optIdx, text)
                        }
                    }
                }

                Loader {
                    asynchronous: true
                    active: root.isMultilineString && !root.isColorLike && !root.isLine
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
                            readOnly: root.oRO || !root.oEnabled
                            wrapMode: TextArea.Wrap
                            onTextChanged: {
                                if (root.optionModel && activeFocus)
                                    root.optionModel.setValue(root.optIdx, text)
                            }
                        }
                    }
                }

                // Upstream PointCtrl (Field.cpp): X / Y pair for coPoint and
                // per-extruder coPoints (first element). Values parse and
                // write back through the model's point helpers.
                Loader {
                    asynchronous: true
                    active: root.oType === "point" && !root.isLine
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    width: Math.min(parent.width, root.controlColumnWidth + 60)
                    height: root.fieldHeight
                    sourceComponent: Component {
                        RowLayout {
                            anchors.fill: parent
                            spacing: 6

                            Repeater {
                                model: 2
                                delegate: CxNumericEdit {
                                    required property int index
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: root.fieldHeight
                                    decimals: 3
                                    valueAlignment: Text.AlignLeft
                                    text: root.formattedNumber(
                                              root.optionModel ? root.optionModel.pointCoord(root.oVal, index) : 0)
                                    suffix: index === 0 ? "X" : "Y"
                                    enabled: !root.oRO && root.oEnabled
                                    onCommit: (valueText) => {
                                        var v = parseFloat(valueText)
                                        if (!isNaN(v) && root.optionModel)
                                            root.optionModel.setPointCoord(root.optIdx, index, v)
                                    }
                                }
                            }
                        }
                    }
                }

                // Polygon keys: bed_exclude_area / wrapping_exclude_area edit as
                // one "XxY, XxY, ..." line (Field.cpp:689-719); printable_area
                // is the "Set..." button that opens the bed shape dialog.
                Loader {
                    asynchronous: true
                    active: root.oType === "points" && !root.isLine
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    width: root.isBedShapeOption ? implicitWidth : parent.width
                    height: root.fieldHeight
                    sourceComponent: root.isBedShapeOption ? bedShapeButton : pointsField
                }

                Component {
                    id: bedShapeButton
                    CxButton {
                        compact: true
                        cxStyle: CxButton.Style.Secondary
                        text: qsTr("设置 ...")
                        enabled: !root.oRO && root.oEnabled
                        onClicked: {
                            if (typeof Backend !== "undefined" && Backend)
                                Backend.showBedShapeDialog()
                        }
                    }
                }

                Component {
                    id: pointsField
                    CxTextField {
                        text: root.optionModel ? root.optionModel.pointsText(root.oVal) : ""
                        enabled: !root.oRO && root.oEnabled
                        onEditingFinished: {
                            if (root.optionModel && !root.optionModel.setPointsText(root.optIdx, text))
                                text = root.optionModel.pointsText(root.oVal)
                        }
                    }
                }

                // Upstream PluginConfigField: "配置" / "配置 (N)" button
                // opening the preset's plugin capability override editor.
                Loader {
                    asynchronous: true
                    active: root.oType === "plugin_config" && !root.isLine
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    height: root.fieldHeight
                    sourceComponent: Component {
                        CxButton {
                            readonly property int overrideCount: root.optionModel
                                ? root.optionModel.pluginOverrideCount(root.oVal) : 0
                            compact: true
                            cxStyle: CxButton.Style.Secondary
                            text: overrideCount > 0 ? qsTr("配置 (%1)").arg(overrideCount) : qsTr("配置")
                            toolTipText: root.oTip
                            enabled: !root.oRO && root.oEnabled
                            onClicked: {
                                pluginsDialogLoader.active = true
                                pluginsDialogLoader.item.openWith(
                                    typeof root.oVal === "string" ? root.oVal : "")
                            }
                        }
                    }
                }

                Loader {
                    id: pluginsDialogLoader
                    active: false
                    sourceComponent: Component {
                        PluginsConfigDialog {
                            optionModel: root.optionModel
                            onFinished: (text) => {
                                const current = typeof root.oVal === "string" ? root.oVal : ""
                                if (root.optionModel && !root.optionModel.pluginOverridesEqual(current, text))
                                    root.optionModel.setValue(root.optIdx, text)
                            }
                        }
                    }
                }

                // Multi-option line: every member's field, loaded by URL so
                // OptionRow does not statically instantiate itself through
                // OptionLineFields.
                Loader {
                    id: lineFieldsLoader
                    active: root.isLine
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width
                    height: item ? item.implicitHeight : 26
                    source: root.isLine ? Qt.resolvedUrl("OptionLineFields.qml") : ""
                    onLoaded: {
                        item.optionModel = Qt.binding(() => root.optionModel)
                        item.headKey = Qt.binding(() => root.lineHead)
                        item.viewMode = Qt.binding(() => root.lineViewMode)
                        item.availableWidth = Qt.binding(() => lineFieldsLoader.width)
                        item.fieldWidth = Qt.binding(() => root.compactFieldWidth)
                        item.enumWidth = Qt.binding(() => root.compactEnumWidth)
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
                    active: root.hasTrailingDialogAction && !root.isLine
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
                                    if (typeof Backend === "undefined" || !Backend)
                                        return
                                    if (root.isGcodeOption)
                                        Backend.showEditGCodeDialog(
                                            root.oKey,
                                            typeof root.oVal === "string" ? root.oVal
                                                : (root.oVal ? root.oVal.toString() : ""))
                                    else
                                        Backend.showBedShapeDialog()
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
