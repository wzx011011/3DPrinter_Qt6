import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Tool-position tooltip, source truth of upstream GCodeViewer::
// SequentialView::Marker::render_position_window (GCodeViewer.cpp:326-714).
// Folded: only the bottom row (fold glyph button + two-line info group).
// Expanded: the 14-row property table (GCodeViewer.cpp:430-475), the
// "Actual speed profile" Show/Hide row (GCodeViewer.cpp:543-555) and a
// Separator (GCodeViewer.cpp:638) stacked above the bottom row. The speed
// profile itself renders in a companion window to the left (upstream
// "ToolPositionTableWnd", GCodeViewer.cpp:595-632).
Rectangle {
    id: root
    required property var previewVm

    // upstream static properties_shown (GCodeViewer.cpp:329)
    property bool expanded: false
    // upstream static table_shown (GCodeViewer.cpp:560)
    property bool speedProfileShown: false
    // Hovered segment in the speed-profile plot (upstream plot() return id).
    property int plotHoverId: -1

    readonly property var vm: root.previewVm
    // upstream NA_TXT (GCodeViewer.cpp:329)
    readonly property string naText: qsTr("N/A")
    readonly property bool speedProfileVisible: root.expanded && root.speedProfileShown
        && root.vm && root.vm.toolActualSpeedExist
        && root.vm.toolActualSpeedProfile.length >= 2
    // Fixed columns of the companion profile table (upstream column headers,
    // GCodeViewer.cpp:607-609).
    readonly property real profileCol0Width: profilePosMetrics.width + 18
    readonly property real profileCol1Width: profileSpeedMetrics.width + 18
    readonly property real profileTableWidth: root.profileCol0Width + root.profileCol1Width + 9
    // Plot window width: 16:9 of the 135px plot height (GCodeViewer.cpp:600-602).
    readonly property real plotWidth: Math.max(135 * 16 / 9, root.profileTableWidth)
    readonly property font monoFont: Qt.font({ family: Theme.fontMono, pixelSize: Theme.fontSizeSM })

    visible: root.vm && root.vm.hasToolPosition
    width: contentColumn.implicitWidth + 20
    height: contentColumn.implicitHeight + 20
    radius: 8                    // upstream WindowRounding 8.0f*m_scale (:341)
    color: "#11151dcc"           // upstream SetNextWindowBgAlpha(0.8f) (:343)
    border.width: 0              // upstream ImGuiCol_Border fully transparent (:339)
    opacity: visible ? 0.94 : 0

    Behavior on opacity { NumberAnimation { duration: 120 } }

    // Upstream precision policy (GCodeViewer.cpp:677-678): round(max(x,y,z)),
    // "%.1f" beyond 9999, "%.2f" beyond 999, else "%.3f".
    function coordText(value) {
        if (!root.vm)
            return value.toFixed(3);
        const maxV = Math.round(Math.max(root.vm.toolX, root.vm.toolY, root.vm.toolZ));
        const decimals = maxV > 9999 ? 1 : maxV > 999 ? 2 : 3;
        return value.toFixed(decimals);
    }

    // Hovered plot segment at x within a plot of plotWidth (inverse of the
    // plot's pos->x mapping; upstream hover_id semantics).
    function hoverIndexAt(x, plotAreaWidth) {
        if (!root.vm || plotAreaWidth <= 20)
            return -1;
        const data = root.vm.toolActualSpeedProfile;
        if (!data || data.length < 2)
            return -1;
        const x0 = data[0].pos;
        const x1 = data[data.length - 1].pos;
        const xSpan = Math.max(x1 - x0, 1e-6);
        const clamped = Math.max(0, Math.min(x - 10, plotAreaWidth - 20));
        const pos = x0 + clamped / (plotAreaWidth - 20) * xSpan;
        let idx = -1;
        for (let n = 0; n < data.length; ++n) {
            if (data[n].pos <= pos)
                idx = n;
            else
                break;
        }
        return idx;
    }

    // The fixed 14-row property table (upstream add_row calls,
    // GCodeViewer.cpp:430-475): every row is always present, non-applicable
    // cells show N/A. toolKind: 0=Extrude 1=Travel 4=Wipe (upstream
    // is_extrusion/is_travel/is_wipe gates). toolFeedrate is the parsed F
    // word in mm/min; upstream vertex.feedrate is mm/s, hence the /60.
    readonly property var tableRows: {
        if (!root.vm)
            return [];
        const k = root.vm.toolKind;
        const isExtrusion = k === 0;
        const hasLength = (k === 0 || k === 1 || k === 4) && root.vm.toolLength >= 0;
        return [
            { label: qsTr("Type"), value: root.vm.toolTypeText },
            { label: qsTr("Line Type"), value: isExtrusion ? root.vm.toolLineTypeText : root.naText },
            { label: qsTr("Width"), value: isExtrusion ? qsTr("%1 mm").arg(root.vm.toolWidth.toFixed(3)) : root.naText },
            { label: qsTr("Height"), value: isExtrusion ? qsTr("%1 mm").arg(root.vm.toolHeight.toFixed(3)) : root.naText },
            { label: qsTr("Length"), value: hasLength ? qsTr("%1 mm").arg(root.vm.toolLength.toFixed(3)) : root.naText },
            { label: qsTr("Layer"), value: String(root.vm.toolLayer + 1) },
            { label: qsTr("Speed"), value: qsTr("%1 mm/s").arg((root.vm.toolFeedrate / 60).toFixed(1)) },
            { label: qsTr("Acceleration"), value: qsTr("%1 mm/s\u00B2").arg(root.vm.toolAcceleration.toFixed(0)) },
            { label: qsTr("Jerk"), value: qsTr("%1 mm/s").arg(root.vm.toolJerk.toFixed(1)) },
            { label: qsTr("Flow rate"), value: isExtrusion ? qsTr("%1 mm\u00B3/s").arg(root.vm.toolFlowRate.toFixed(3)) : root.naText },
            { label: qsTr("Fan speed"), value: qsTr("%1 %").arg(root.vm.toolFanSpeed.toFixed(0)) },
            { label: qsTr("Temperature"), value: qsTr("%1 \u2103").arg(root.vm.toolTemperature.toFixed(0)) },
            { label: qsTr("Pressure Advance"), value: root.vm.toolPressureAdvance.toFixed(4) },
            { label: qsTr("Time"), value: qsTr("%1 (%2s)").arg(root.vm.toolEstimatedTime).arg(root.vm.toolMoveTimeSecs.toFixed(3)) }
        ];
    }
    readonly property var tableLabels: {
        const labels = [];
        for (let i = 0; i < root.tableRows.length; ++i)
            labels.push(root.tableRows[i].label);
        return labels;
    }
    readonly property var tableValues: {
        const values = [];
        for (let i = 0; i < root.tableRows.length; ++i)
            values.push(root.tableRows[i].value);
        return values;
    }

    // Column metrics of the bottom info group (upstream :481-483:
    // "sp" spacing, "X 999.999" column width, "Speed: 9999  " speed width).
    TextMetrics {
        id: axesMetrics
        font: root.monoFont
        text: "X 999.999"
    }
    TextMetrics {
        id: axesSpacingMetrics
        font: root.monoFont
        text: "sp"
    }
    TextMetrics {
        id: speedMetrics
        font: root.monoFont
        text: "Speed: 9999  "
    }
    TextMetrics {
        id: profilePosMetrics
        font: root.monoFont
        text: qsTr("Position (mm)")
    }
    TextMetrics {
        id: profileSpeedMetrics
        font: root.monoFont
        text: qsTr("Speed (mm/s)")
    }

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 10   // upstream WindowPadding (10,10)*m_scale (:342)
        spacing: 4

        // Property table (upstream BeginTable("Properties", 2) :523-537):
        // two aligned columns, labels colored, values default text.
        Row {
            visible: root.expanded
            spacing: 10
            Column {
                spacing: 3
                Repeater {
                    model: root.tableLabels
                    Label {
                        text: modelData
                        color: Theme.accent   // upstream text_colored(COL_ORCA) -- brand accent
                        font.pixelSize: Theme.fontSizeSM
                    }
                }
            }
            Column {
                spacing: 3
                Repeater {
                    model: root.tableValues
                    Label {
                        text: modelData
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                    }
                }
            }
        }

        // "Actual speed profile" Show/Hide row (upstream :543-555), rendered
        // while expanded; the button disables without profile data.
        Row {
            id: profileButtonRow
            visible: root.expanded
            spacing: 8
            readonly property bool profileDataAvailable: root.vm && root.vm.toolActualSpeedExist
            Rectangle {
                width: showHideLabel.implicitWidth + 16
                height: showHideLabel.implicitHeight + 6
                radius: 3
                color: !profileButtonRow.profileDataAvailable
                       ? Theme.textDisabled
                       : (showHideArea.containsMouse ? Theme.accentLight : Theme.accent)
                opacity: !profileButtonRow.profileDataAvailable ? 0.45 : 1
                Label {
                    id: showHideLabel
                    anchors.centerIn: parent
                    text: root.speedProfileShown ? qsTr("Hide") : qsTr("Show")
                    color: Theme.textOnAccent
                    font.pixelSize: Theme.fontSizeSM
                }
                MouseArea {
                    id: showHideArea
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: profileButtonRow.profileDataAvailable
                    onClicked: root.speedProfileShown = !root.speedProfileShown
                }
            }
            Label {
                text: qsTr("Actual speed profile")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeSM
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        // Separator (upstream ImGui::Separator() :638; ImGuiCol_Separator is
        // pushed to (1,1,1,0.6) at :336).
        Rectangle {
            visible: root.expanded
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#ffffff99"
        }

        // Bottom row: fold glyph button + two-line info group
        // (upstream :646-711). The fold button toggles the property table;
        // the info group shows X/Y/Z on line 1 and "Speed: N" + per-view
        // detail on line 2.
        Row {
            spacing: 8
            Item {
                width: 16
                height: 16
                Rectangle {
                    anchors.fill: parent
                    radius: 3
                    color: Theme.bgHover
                    visible: foldArea.containsMouse
                }
                Canvas {
                    id: foldCanvas
                    anchors.fill: parent
                    contextType: "2d"
                    onPaint: {
                        const ctx = getContext("2d");
                        if (!ctx)
                            return;
                        ctx.reset();
                        ctx.strokeStyle = "#f5f5f5";
                        ctx.lineWidth = 1.5;
                        ctx.lineCap = "round";
                        ctx.lineJoin = "round";
                        // Double chevron glyph, drawn after the upstream
                        // im_fold.svg (chevrons up while collapsed) and
                        // im_unfold.svg (chevrons down while expanded).
                        const up = !root.expanded;
                        const apex1 = up ? 2.5 : 6.5;
                        const end1 = up ? 6.5 : 2.5;
                        const apex2 = up ? 9.5 : 13.5;
                        const end2 = up ? 13.5 : 9.5;
                        ctx.beginPath();
                        ctx.moveTo(1.5, end1);
                        ctx.lineTo(8, apex1);
                        ctx.lineTo(14.5, end1);
                        ctx.moveTo(1.5, end2);
                        ctx.lineTo(8, apex2);
                        ctx.lineTo(14.5, end2);
                        ctx.stroke();
                    }
                }
                Connections {
                    target: root
                    function onExpandedChanged() { foldCanvas.requestPaint(); }
                }
                MouseArea {
                    id: foldArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.expanded = !root.expanded
                }
            }
            Column {
                spacing: 4
                // Line 1: X/Y/Z at fixed column offsets (upstream :681-701).
                Item {
                    implicitWidth: 3 * (axesMetrics.width + axesSpacingMetrics.width)
                    implicitHeight: axesMetrics.boundingRect.height
                    Repeater {
                        model: [
                            { axis: "X", value: root.vm ? root.coordText(root.vm.toolX) : "0.000" },
                            { axis: "Y", value: root.vm ? root.coordText(root.vm.toolY) : "0.000" },
                            { axis: "Z", value: root.vm ? root.coordText(root.vm.toolZ) : "0.000" }
                        ]
                        delegate: Row {
                            x: index * (axesMetrics.width + axesSpacingMetrics.width)
                            spacing: 0
                            Label {
                                text: modelData.axis + " "
                                color: Theme.textTertiary
                                font: root.monoFont
                            }
                            Label {
                                text: modelData.value
                                color: Theme.textPrimary
                                font: root.monoFont
                            }
                        }
                    }
                }
                // Line 2: "Speed: N" (integer, no unit) with the per-view
                // detail appended at the fixed speed column offset
                // (upstream :703-710).
                Item {
                    implicitWidth: speedMetrics.width + detailLabel.implicitWidth
                    implicitHeight: speedLabel.implicitHeight
                    Label {
                        id: speedLabel
                        x: 0
                        text: root.vm ? qsTr("Speed: %1").arg(Math.round(root.vm.toolFeedrate / 60)) : ""
                        color: Theme.textPrimary
                        font: root.monoFont
                    }
                    Label {
                        id: detailLabel
                        x: speedMetrics.width
                        visible: root.vm && root.vm.toolDetailText !== ""
                        text: root.vm ? root.vm.toolDetailText : ""
                        color: Theme.textPrimary
                        font: root.monoFont
                    }
                }
            }
        }
    }

    // Companion actual-speed profile window (upstream ToolPositionTableWnd,
    // GCodeViewer.cpp:595-632): right edge 5px left of the main window,
    // bottom edge on the canvas bottom (set_next_window_pos pivot 1.0, 1.0).
    Rectangle {
        id: profileWindow
        visible: root.speedProfileVisible
        x: -width - 5
        y: root.height - height
        width: root.plotWidth + 20
        height: profileColumn.implicitHeight + 20
        radius: 8
        color: "#11151dcc"
        border.width: 0

        Column {
            id: profileColumn
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            // Actual speed plot (upstream ActualSpeedImguiWidget::plot,
            // GCodeViewer.cpp:152-215): vertical grid per data point,
            // profile polyline, hover picks the highlighted segment.
            Item {
                width: root.plotWidth
                height: 135
                Rectangle {
                    anchors.fill: parent
                    radius: 3
                    color: Theme.bgInset
                }
                Canvas {
                    id: profilePlot
                    anchors.fill: parent
                    contextType: "2d"
                    onPaint: {
                        const ctx = getContext("2d");
                        if (!ctx || !root.vm)
                            return;
                        const data = root.vm.toolActualSpeedProfile;
                        ctx.reset();
                        if (!data || data.length < 2)
                            return;
                        const pad = 10;   // upstream offset (10, 0)
                        const w = width;
                        const h = height;
                        let yMin = root.vm.toolActualSpeedYMin;
                        let yMax = root.vm.toolActualSpeedYMax;
                        if (yMax - yMin < 1e-6)
                            yMax = yMin + 1;
                        const x0 = data[0].pos;
                        const x1 = data[data.length - 1].pos;
                        const xSpan = Math.max(x1 - x0, 1e-6);
                        const toX = (pos) => pad + (pos - x0) / xSpan * (w - pad);
                        const toY = (speed) => h * (1 - (speed - yMin) / (yMax - yMin));
                        // Vertical grid lines per data point: internal (zero
                        // duration) points in the brand accent, others gray
                        // (upstream :183-192; upstream teal -> OWzx accent).
                        ctx.lineWidth = 1;
                        for (let n = 0; n < data.length; ++n) {
                            ctx.strokeStyle = data[n].internal ? "#8018c75e" : "#80808080";
                            ctx.beginPath();
                            ctx.moveTo(toX(data[n].pos), 0);
                            ctx.lineTo(toX(data[n].pos), h);
                            ctx.stroke();
                        }
                        // Profile polyline (upstream :196-205), hovered
                        // segment in the brand accent.
                        ctx.lineWidth = 2;
                        for (let n = 0; n < data.length - 1; ++n) {
                            ctx.strokeStyle = n === root.plotHoverId ? "#18c75e" : "#cccccc";
                            ctx.beginPath();
                            ctx.moveTo(toX(data[n].pos), toY(data[n].speed));
                            ctx.lineTo(toX(data[n + 1].pos), toY(data[n + 1].speed));
                            ctx.stroke();
                        }
                    }
                    Connections {
                        target: root
                        function onPlotHoverIdChanged() { profilePlot.requestPaint(); }
                        function onSpeedProfileVisibleChanged() { profilePlot.requestPaint(); }
                    }
                    Connections {
                        target: root.vm
                        function onStateChanged() { profilePlot.requestPaint(); }
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.NoButton
                    onPositionChanged: function(mouse) {
                        root.plotHoverId = root.hoverIndexAt(mouse.x, width);
                        profilePlot.requestPaint();
                    }
                    onExited: {
                        root.plotHoverId = -1;
                        profilePlot.requestPaint();
                    }
                }
            }

            // Two-column table Position (mm) / Speed (mm/s) with per-row
            // background (upstream BeginTable("ToolPositionTable", 2)
            // :609-632).
            Column {
                spacing: 0
                Row {
                    spacing: 9   // upstream CellPadding.x 9*scale
                    Label {
                        width: root.profileCol0Width
                        text: qsTr("Position (mm)")
                        color: Theme.textSecondary
                        font: root.monoFont
                    }
                    Label {
                        text: qsTr("Speed (mm/s)")
                        color: Theme.textSecondary
                        font: root.monoFont
                    }
                }
                Flickable {
                    width: root.profileTableWidth
                    height: Math.min(rowsColumn.implicitHeight, 200)
                    clip: true
                    contentHeight: rowsColumn.implicitHeight
                    Column {
                        id: rowsColumn
                        spacing: 0
                        Repeater {
                            model: root.vm ? root.vm.toolActualSpeedProfile : []
                            delegate: Rectangle {
                                readonly property bool rowHovered:
                                    index === root.plotHoverId || index === root.plotHoverId + 1
                                readonly property bool rowInternal: modelData.internal === true
                                width: root.profileTableWidth
                                height: profilePosLabel.implicitHeight + 2
                                // upstream TableSetBgColor: internal (0,150/255,136/255,0.15)
                                // -> brand accent, else (0.2,0.2,0.2,0.25)
                                color: rowInternal ? "#2618c75e" : "#40333333"
                                Row {
                                    spacing: 9
                                    Label {
                                        id: profilePosLabel
                                        width: root.profileCol0Width
                                        text: modelData.pos.toFixed(3)
                                        color: rowHovered ? Theme.accent : Theme.textPrimary
                                        font: root.monoFont
                                    }
                                    Label {
                                        text: modelData.speed.toFixed(1)
                                        color: rowHovered ? Theme.accent : Theme.textPrimary
                                        font: root.monoFont
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
