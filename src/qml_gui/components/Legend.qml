import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

Item {
    id: root
    required property var previewVm

    readonly property int legendType: root.previewVm ? root.previewVm.legendType : 0
    implicitHeight: legendLayout.implicitHeight

    // P17.10: imperial display — SettingsViewModel.units 1 = inch
    // (upstream use_inches, Preferences.cpp:1110). mm -> in (/25.4).
    readonly property bool imperialUnits: {
        const b = typeof backend !== "undefined" ? backend : null
        return b && b.settingsViewModel ? b.settingsViewModel.units === 1 : false
    }
    function lengthValue(mmText) {
        if (!root.imperialUnits)
            return mmText
        // mmText is a plain decimal mm string from the ViewModel stops.
        const mm = parseFloat(mmText)
        if (isNaN(mm))
            return mmText
        return (mm / 25.4).toFixed(2)
    }

    // P17.3: gradient stop color lookup by index into the VM's 10-step list.
    function stopColor(index) {
        if (!root.previewVm)
            return Theme.chromeBorder
        const stops = root.previewVm.legendGradientStops
        if (!stops || index >= stops.length)
            return Theme.chromeBorder
        return stops[index].color
    }

    function stopValue(index) {
        if (!root.previewVm)
            return "--"
        const stops = root.previewVm.legendGradientStops
        if (!stops || index >= stops.length)
            return "--"
        return stops[index].value
    }

    ColumnLayout {
        id: legendLayout
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 6

        Label {
            text: qsTr("图例")
            color: Theme.textPrimary
            font.bold: true
            font.pixelSize: Theme.fontSizeMD
        }

        Rectangle {
            Layout.fillWidth: true
            radius: 5
            color: Theme.bgCard
            border.width: 1
            border.color: Theme.borderSubtle
            implicitHeight: legendContent.implicitHeight + 16

            ColumnLayout {
                id: legendContent
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 8
                spacing: 7

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: root.legendType === 1

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 16
                        radius: 4
                        // P17.3: the 10 upstream Range_Color stops come from
                        // the ViewModel (legendGradientStops), replacing the
                        // earlier Theme-token approximation.
                        // P17.3: the 10 upstream Range_Color stops come from
                        // the ViewModel (legendGradientStops); Gradient is not
                        // an Item, so the stops are explicit bindings.
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: root.stopColor(0) }
                            GradientStop { position: 0.11; color: root.stopColor(1) }
                            GradientStop { position: 0.22; color: root.stopColor(2) }
                            GradientStop { position: 0.33; color: root.stopColor(3) }
                            GradientStop { position: 0.44; color: root.stopColor(4) }
                            GradientStop { position: 0.55; color: root.stopColor(5) }
                            GradientStop { position: 0.66; color: root.stopColor(6) }
                            GradientStop { position: 0.77; color: root.stopColor(7) }
                            GradientStop { position: 0.88; color: root.stopColor(8) }
                            GradientStop { position: 1.0; color: root.stopColor(9) }
                        }
                        border.width: 1
                        border.color: Theme.borderSubtle
                    }

                    // P17.3: per-step value labels (upstream append_range rows).
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Repeater {
                            model: root.previewVm ? root.previewVm.legendGradientStops : []
                            delegate: Text {
                                required property int index
                                Layout.fillWidth: true
                                horizontalAlignment: index === 0 ? Text.AlignLeft
                                    : (index === 9 ? Text.AlignRight : Text.AlignHCenter)
                                text: root.imperialUnits
                                          ? root.lengthValue(root.stopValue(index))
                                          : root.stopValue(index)
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeXS - 1
                                font.family: Theme.fontMono
                            }
                        }
                    }

                    // P17.3: the upstream min/max endpoints (legendGradient*
                    // Labels) remain bound for the collapsed two-value readout.
                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            // P17.10: min endpoint converts under imperial.
                            text: root.imperialUnits
                                      ? root.lengthValue(root.previewVm ? root.previewVm.legendGradientMinLabel : "--") + " in"
                                      : (root.previewVm ? root.previewVm.legendGradientMinLabel : "--")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeXS
                            font.family: Theme.fontMono
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: root.imperialUnits
                                      ? root.lengthValue(root.previewVm ? root.previewVm.legendGradientMaxLabel : "--") + " in"
                                      : (root.previewVm ? root.previewVm.legendGradientMaxLabel : "--")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeXS
                            font.family: Theme.fontMono
                        }
                    }
                }

                Repeater {
                    model: root.previewVm ? root.previewVm.legendItems : []
                    delegate: RowLayout {
                        id: legendRow
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 8
                        visible: root.legendType !== 1

                        Rectangle {
                            Layout.preferredWidth: 10
                            Layout.preferredHeight: 10
                            radius: 2
                            color: legendRow.modelData.color
                        }
                        Label {
                            Layout.fillWidth: true
                            text: legendRow.modelData.label + (legendRow.modelData.count > 0 ? " (" + legendRow.modelData.count + ")" : "")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeSM
                            elide: Text.ElideRight
                        }
                        // Phase 238 (PREV-06): per-extruder visibility toggle.
                        // The Filament (ColorPrint) / Tool legend rows carry
                        // extruderId; upstream toggles m_tool_visibles from the
                        // legend row click and refreshes the render paths
                        // (GCodeViewer.cpp:5088-5094). Applies in those two
                        // view modes only (upstream gates the skip on
                        // ColorPrint, GCodeViewer.cpp:3337).
                        CxCheckBox {
                            visible: root.legendType === 2
                                     && legendRow.modelData.extruderId !== undefined
                            checked: legendRow.modelData.visible !== undefined
                                     ? legendRow.modelData.visible : true
                            onToggled: if (visible && root.previewVm
                                            && legendRow.modelData.extruderId !== undefined)
                                           root.previewVm.toggleExtruderVisibility(
                                                       legendRow.modelData.extruderId)
                        }
                    }
                }

                Label {
                    visible: !root.previewVm || root.previewVm.legendItems.length === 0
                    text: qsTr("暂无图例数据")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeSM
                }

                // P17.4: FeatureType per-role Time / Percent / Used-filament
                // columns (upstream FeatureType legend rows,
                // GCodeViewer.cpp:4808-4845 + roles_times).
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    visible: root.legendType === 0
                             && root.previewVm
                             && root.previewVm.legendRoleColumns.length > 0

                    RowLayout {
                        Layout.fillWidth: true
                        Text { Layout.preferredWidth: 84; text: qsTr("类型"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS }
                        Text { Layout.preferredWidth: 64; text: qsTr("时间"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS }
                        Text { Layout.preferredWidth: 48; text: qsTr("%"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS }
                        Text { Layout.fillWidth: true; text: qsTr("耗材"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS }
                    }
                    Repeater {
                        model: root.previewVm ? root.previewVm.legendRoleColumns : []
                        delegate: RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            Text { Layout.preferredWidth: 84; text: modelData.label; color: Theme.textPrimary; font.pixelSize: Theme.fontSizeXS; elide: Text.ElideRight }
                            Text { Layout.preferredWidth: 64; text: modelData.time; color: Theme.textPrimary; font.pixelSize: Theme.fontSizeXS }
                            Text { Layout.preferredWidth: 48; text: modelData.percent; color: Theme.textPrimary; font.pixelSize: Theme.fontSizeXS }
                            Text { Layout.fillWidth: true; text: modelData.filament; color: Theme.textPrimary; font.pixelSize: Theme.fontSizeXS }
                        }
                    }
                }

                // P17.6: ColorPrint extras — filament-change count, Prepare
                // time, and the custom g-code overview table (upstream
                // GCodeViewer.cpp:5156-5159, :5479-5545, prepare_time).
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    visible: root.legendType === 2 && root.previewVm

                    Text {
                        text: qsTr("换色次数: %1").arg(root.previewVm ? root.previewVm.colorChangeCount : 0)
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                    }
                    Text {
                        text: qsTr("准备时间: %1").arg(root.previewVm ? root.previewVm.prepareTime : "--")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                    }

                    Text {
                        visible: root.previewVm && root.previewVm.customGcodeRows.length > 0
                        text: qsTr("自定义 G-code")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                    }
                    Repeater {
                        model: root.previewVm ? root.previewVm.customGcodeRows : []
                        delegate: RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            Text { Layout.preferredWidth: 80; text: modelData.type; color: Theme.textPrimary; font.pixelSize: Theme.fontSizeXS; elide: Text.ElideRight }
                            Text { Layout.preferredWidth: 48; text: qsTr("层 %1").arg(modelData.layer); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeXS }
                            Text { Layout.fillWidth: true; text: modelData.time; color: Theme.textPrimary; font.pixelSize: Theme.fontSizeXS }
                        }
                    }
                }
            }
        }
    }
}
