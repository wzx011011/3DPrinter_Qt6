import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

/// MonitorPage — device monitoring dashboard
///
/// Upstream alignment: third_party/CrealityPrint/src/slic3r/GUI/Monitor.cpp
///   - Left sidebar: device list with printer name, type, online indicator, signal
///   - Right content: status panel, media files, update, HMS tabs
///   - 1-second refresh timer (REFRESH_INTERVAL = 1000)
///
/// Current scope (P5.1): device list, card UI, selection, search/filter,
/// basic status display for the selected device.
Item {
    id: root
    required property var monitorVm

    // ── Mock refresh timer (mirrors upstream 1000ms REFRESH_INTERVAL) ──
    Timer {
        interval: 3000
        running: true
        repeat: true
        onTriggered: root.monitorVm.refresh()
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.bgBase
    }

    // ── Main horizontal split: device list (left) + detail panel (right) ──
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ══════════════════════════════════════════════════════════
        // LEFT PANEL — Device list with search (aligns with upstream
        // MonitorBasePanel left panel: 182px sidebar with printer name,
        // signal icon, status, time-lapse, video, task list entries)
        // ══════════════════════════════════════════════════════════
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 280
            color: Theme.bgSurface
            Layout.rightMargin: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // ── Header: "Devices" + refresh + add buttons ──
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    color: Theme.bgSurface

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingLG
                        anchors.rightMargin: Theme.spacingLG
                        spacing: Theme.spacingMD

                        Text {
                            text: qsTr("设备")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeXL
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }

                        // Refresh button
                        Rectangle {
                            width: 28; height: 28; radius: Theme.radiusLG
                            color: refreshHover.containsMouse ? Theme.bgHover : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: "\u21BB"
                                color: Theme.textSecondary
                                font.pixelSize: 14
                            }
                            HoverHandler { id: refreshHover }
                            TapHandler { onTapped: root.monitorVm.refresh() }
                        }

                        // Scan / Add button（对齐上游 SSDP scan）
                        Rectangle {
                            width: 72; height: 26; radius: 4
                            color: scanMA.containsMouse ? "#dbeafe" : Theme.accentSubtle
                            border.width: 1; border.color: Theme.accent
                            Text {
                                anchors.centerIn: parent
                                text: qsTr("+ 添加")
                                color: Theme.accent
                                font.pixelSize: Theme.fontSizeSM
                            }
                            MouseArea {
                                id: scanMA
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.monitorVm.scanDevices()
                            }
                        }
                    }
                }

                // Separator
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.borderSubtle
                }

                // ── Search bar ──
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    Layout.topMargin: Theme.spacingSM
                    Layout.leftMargin: Theme.spacingLG
                    Layout.rightMargin: Theme.spacingLG

                    Rectangle {
                        anchors.fill: parent
                        radius: Theme.radiusMD
                        color: Theme.bgElevated
                        border.width: 1
                        border.color: searchField.activeFocus ? Theme.borderFocus : Theme.borderSubtle

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 6

                            Text {
                                text: "\u2315"
                                color: Theme.textTertiary
                                font.pixelSize: 13
                            }

                            TextField {
                                id: searchField
                                Layout.fillWidth: true
                                background: null
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeMD
                                placeholderText: qsTr("搜索设备...")
                                placeholderTextColor: Theme.textTertiary
                                selectByMouse: true
                                onTextChanged: root.monitorVm.searchText = text
                            }

                            // Clear button
                            Text {
                                visible: searchField.text.length > 0
                                text: "\u2715"
                                color: Theme.textTertiary
                                font.pixelSize: 10
                                TapHandler {
                                    onTapped: {
                                        searchField.text = ""
                                        searchField.forceActiveFocus()
                                    }
                                }
                            }
                        }
                    }
                }

                // ── Device count label ──
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 24
                    Layout.topMargin: Theme.spacingXS

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: Theme.spacingLG + Theme.spacingSM
                        text: qsTr("%1 台设备").arg(root.monitorVm.filteredDeviceCount)
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeSM
                    }
                }

                // ── Device list (scrollable) ──
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.topMargin: Theme.spacingXS

                    ScrollView {
                        anchors.fill: parent
                        clip: true
                        ScrollBar.vertical.policy: ScrollBar.AsNeeded

                        Column {
                            id: deviceListColumn
                            width: parent.width - 4
                            spacing: 2

                            Repeater {
                                model: root.monitorVm.filteredDeviceCount

                                delegate: Rectangle {
                                    id: deviceDelegate
                                    width: deviceListColumn.width
                                    height: 78
                                    radius: Theme.radiusLG
                                    color: {
                                        if (isSelected) return Theme.bgFloating
                                        if (delegateHover.containsMouse) return Theme.bgHover
                                        return "transparent"
                                    }
                                    border.width: isSelected ? 1 : 0
                                    border.color: Theme.accent

                                    readonly property bool isSelected: root.monitorVm.selectedDeviceIndex === modelData
                                    property var devData: root.monitorVm.deviceAt(modelData)

                                    HoverHandler { id: delegateHover }
                                    TapHandler {
                                        onTapped: root.monitorVm.selectDevice(modelData)
                                    }

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: Theme.spacingLG
                                        anchors.rightMargin: Theme.spacingLG
                                        spacing: Theme.spacingMD

                                        // Online status indicator dot
                                        Rectangle {
                                            width: 8; height: 8; radius: 4
                                            color: devData.online ? Theme.statusSuccess : Theme.textDisabled
                                            Layout.alignment: Qt.AlignTop
                                            Layout.topMargin: 12
                                        }

                                        // Device info column
                                        Column {
                                            Layout.fillWidth: true
                                            spacing: 2

                                            // Device name (bold)
                                            Text {
                                                text: devData.name || ""
                                                color: Theme.textPrimary
                                                font.pixelSize: Theme.fontSizeLG
                                                font.bold: true
                                                elide: Text.ElideRight
                                                width: parent.width
                                            }

                                            // Model type
                                            Text {
                                                text: devData.model || ""
                                                color: Theme.textTertiary
                                                font.pixelSize: Theme.fontSizeSM
                                                elide: Text.ElideRight
                                                width: parent.width
                                            }

                                            // Status line
                                            Row {
                                                spacing: Theme.spacingSM
                                                topPadding: 2

                                                // Status badge
                                                Rectangle {
                                                    width: statusText.implicitWidth + 12
                                                    height: 18
                                                    radius: 9
                                                    color: {
                                                        if (devData.status === "printing") return "#1a3328"
                                                        if (devData.status === "idle")     return "#1a2840"
                                                        if (devData.status === "offline")  return "#2a2020"
                                                        if (devData.status === "connecting") return "#2a2a18"
                                                        return Theme.bgElevated
                                                    }
                                                    visible: devData.status !== undefined

                                                    Text {
                                                        id: statusText
                                                        anchors.centerIn: parent
                                                        text: {
                                                            if (devData.status === "printing")   return qsTr("打印中")
                                                            if (devData.status === "idle")       return qsTr("空闲")
                                                            if (devData.status === "offline")    return qsTr("离线")
                                                            if (devData.status === "connecting") return qsTr("连接中")
                                                            return devData.status || ""
                                                        }
                                                        color: {
                                                            if (devData.status === "printing")   return Theme.statusSuccess
                                                            if (devData.status === "idle")       return Theme.statusInfo
                                                            if (devData.status === "offline")    return Theme.textDisabled
                                                            if (devData.status === "connecting") return Theme.statusWarning
                                                            return Theme.textSecondary
                                                        }
                                                        font.pixelSize: Theme.fontSizeXS
                                                    }
                                                }

                                                // Signal strength indicator
                                                Row {
                                                    spacing: 1
                                                    visible: devData.online
                                                    Repeater {
                                                        model: 4
                                                        Rectangle {
                                                            width: 3
                                                            height: 4 + index * 2
                                                            radius: 1
                                                            color: index < (devData.signalStrength || 0)
                                                                   ? Theme.statusSuccess
                                                                   : Theme.borderSubtle
                                                            anchors.bottom: parent.bottom
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        // Progress indicator (for printing devices)
                                        Column {
                                            visible: devData.status === "printing"
                                            Layout.alignment: Qt.AlignVCenter
                                            spacing: 2

                                            Text {
                                                text: devData.progress + "%"
                                                color: Theme.accent
                                                font.pixelSize: Theme.fontSizeMD
                                                font.bold: true
                                                horizontalAlignment: Text.AlignHCenter
                                                width: 42
                                            }

                                            // Mini progress bar
                                            Rectangle {
                                                width: 42; height: 3; radius: 1.5
                                                color: Theme.borderSubtle

                                                Rectangle {
                                                    width: parent.width * (devData.progress / 100)
                                                    height: parent.height
                                                    radius: 1.5
                                                    color: Theme.accent
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // Empty state when no devices match filter
                            Item {
                                width: deviceListColumn.width
                                height: root.monitorVm.filteredDeviceCount === 0 ? 200 : 0
                                visible: root.monitorVm.filteredDeviceCount === 0

                                Column {
                                    anchors.centerIn: parent
                                    spacing: 8

                                    Text {
                                        text: "\u{1F4E1}"
                                        color: Theme.textDisabled
                                        font.pixelSize: 36
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                    Text {
                                        text: qsTr("未找到匹配的设备")
                                        color: Theme.textTertiary
                                        font.pixelSize: Theme.fontSizeMD
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                }
                            }
                        }
                    }
                }

                // Separator at bottom
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.borderSubtle
                }

                // ── Bottom status bar ──
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingLG
                        anchors.rightMargin: Theme.spacingLG
                        spacing: Theme.spacingMD

                        // Network status dot
                        Rectangle {
                            width: 6; height: 6; radius: 3
                            color: root.monitorVm.networkOnline ? Theme.statusSuccess : Theme.statusError
                        }

                        Text {
                            text: root.monitorVm.networkOnline
                                  ? qsTr("网络已连接")
                                  : qsTr("网络未连接")
                            color: root.monitorVm.networkOnline ? Theme.textTertiary : Theme.statusError
                            font.pixelSize: Theme.fontSizeXS
                        }

                        Item { Layout.fillWidth: true }

                        Text {
                            text: root.monitorVm.latencyMs > 0
                                  ? qsTr("%1ms").arg(root.monitorVm.latencyMs)
                                  : ""
                            color: Theme.textDisabled
                            font.pixelSize: Theme.fontSizeXS
                        }
                    }
                }
            }
        }

        // ══════════════════════════════════════════════════════════
        // RIGHT PANEL — Device detail / status area
        // Aligns with upstream MonitorBasePanel right panel (1258px)
        // which hosts StatusPanel, MediaFilePanel, UpgradePanel, HMSPanel
        // via Tabbook tabs (PT_STATUS, PT_MEDIA, PT_UPDATE, PT_HMS)
        // ══════════════════════════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.bgBase

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // ── Device header ──
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 64
                    color: Theme.bgSurface

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingXXL
                        anchors.rightMargin: Theme.spacingXXL
                        spacing: Theme.spacingMD

                        // Printer icon placeholder
                        Rectangle {
                            width: 36; height: 36; radius: Theme.radiusLG
                            color: Theme.bgElevated
                            border.width: 1; border.color: Theme.borderSubtle
                            Text {
                                anchors.centerIn: parent
                                text: "\u{1F5A5}"
                                font.pixelSize: 18
                            }
                        }

                        Column {
                            spacing: 2
                            Text {
                                text: root.monitorVm.selectedDeviceName || qsTr("未选择设备")
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeXXL
                                font.bold: true
                            }
                            Text {
                                text: root.monitorVm.selectedDeviceModel || ""
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSizeMD
                            }
                        }

                        Item { Layout.fillWidth: true }

                        // Signal indicator
                        Row {
                            spacing: 2
                            visible: root.monitorVm.selectedDeviceOnline
                            Repeater {
                                model: 4
                                Rectangle {
                                    width: 3
                                    height: 5 + index * 2.5
                                    radius: 1
                                    color: index < root.monitorVm.selectedDeviceSignalStrength
                                           ? Theme.statusSuccess
                                           : Theme.borderSubtle
                                    anchors.bottom: parent.bottom
                                }
                            }
                        }

                        // Online/Offline badge
                        Rectangle {
                            width: onlineText.implicitWidth + 16; height: 22; radius: 11
                            color: root.monitorVm.selectedDeviceOnline ? "#1a3328" : "#2a2020"
                            border.width: 1
                            border.color: root.monitorVm.selectedDeviceOnline
                                         ? Theme.statusSuccess : Theme.textDisabled

                            Text {
                                id: onlineText
                                anchors.centerIn: parent
                                text: root.monitorVm.selectedDeviceOnline ? qsTr("在线") : qsTr("离线")
                                color: root.monitorVm.selectedDeviceOnline ? Theme.statusSuccess : Theme.textDisabled
                                font.pixelSize: Theme.fontSizeSM
                            }
                        }
                    }
                }

                // Separator
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.borderSubtle
                }

                // ── Tab bar (aligns with upstream Tabbook: Status / SD Card / Update / HMS) ──
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    color: Theme.bgSurface

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingXXL
                        spacing: 0

                        Repeater {
                            model: [
                                qsTr("状态"),
                                qsTr("SD 卡"),
                                qsTr("视频"),
                                qsTr("HMS")
                            ]

                            delegate: Rectangle {
                                width: tabText.implicitWidth + 24 + (index === 3 && root.monitorVm.selectedUnreadHmsCount > 0 ? 18 : 0)
                                height: parent.height
                                color: tabBar.currentIndex === index ? Theme.bgBase : "transparent"

                                // Bottom highlight for active tab
                                Rectangle {
                                    width: parent.width; height: 2
                                    anchors.bottom: parent.bottom
                                    color: tabBar.currentIndex === index ? Theme.accent : "transparent"
                                }

                                Text {
                                    id: tabText
                                    anchors.centerIn: parent
                                    text: modelData + (index === 3 && root.monitorVm.selectedUnreadHmsCount > 0
                                            ? " (" + root.monitorVm.selectedUnreadHmsCount + ")" : "")
                                    color: tabBar.currentIndex === index
                                           ? Theme.textPrimary : Theme.textTertiary
                                    font.pixelSize: Theme.fontSizeMD
                                    font.bold: tabBar.currentIndex === index
                                }

                                TapHandler {
                                    onTapped: tabBar.currentIndex = index
                                }
                            }
                        }
                    }

                    // Invisible tab state holder
                    Item {
                        id: tabBar
                        property int currentIndex: 0
                    }
                }

                // Separator
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.borderSubtle
                }

                // ── Status tab content (main area) ──
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: Theme.spacingXXL

                    // Tab 0: Status panel (active by default)
                    Item {
                        anchors.fill: parent
                        visible: tabBar.currentIndex === 0

                        // No device selected state
                        Column {
                            anchors.centerIn: parent
                            spacing: 12
                            visible: root.monitorVm.selectedDeviceName === ""

                            Text {
                                text: "\u{1F5A5}"
                                font.pixelSize: 48
                                color: Theme.textDisabled
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Text {
                                text: qsTr("请从左侧选择一台设备")
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSizeLG
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }

                        // Device status content
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: Theme.spacingXL
                            visible: root.monitorVm.selectedDeviceName !== ""

                            // ── Status row ──
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 72
                                radius: Theme.radiusXL
                                color: Theme.bgPanel
                                border.width: 1; border.color: Theme.borderSubtle

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: Theme.spacingLG
                                    spacing: Theme.spacingXXL

                                    // Status
                                    Column {
                                        spacing: 2
                                        Text {
                                            text: qsTr("状态")
                                            color: Theme.textTertiary
                                            font.pixelSize: Theme.fontSizeSM
                                        }
                                        Text {
                                            text: {
                                                var s = root.monitorVm.selectedDeviceStatus
                                                if (s === "printing")   return qsTr("打印中")
                                                if (s === "idle")       return qsTr("空闲")
                                                if (s === "offline")    return qsTr("离线")
                                                if (s === "connecting") return qsTr("连接中")
                                                return s || ""
                                            }
                                            color: {
                                                var s = root.monitorVm.selectedDeviceStatus
                                                if (s === "printing")   return Theme.statusSuccess
                                                if (s === "idle")       return Theme.statusInfo
                                                if (s === "offline")    return Theme.textDisabled
                                                if (s === "connecting") return Theme.statusWarning
                                                return Theme.textPrimary
                                            }
                                            font.pixelSize: Theme.fontSizeLG
                                            font.bold: true
                                        }
                                    }

                                    // Separator
                                    Rectangle {
                                        Layout.fillHeight: true
                                        Layout.preferredWidth: 1
                                        color: Theme.borderSubtle
                                    }

                                    // Temperature
                                    Column {
                                        spacing: 2
                                        Text {
                                            text: qsTr("喷头温度")
                                            color: Theme.textTertiary
                                            font.pixelSize: Theme.fontSizeSM
                                        }
                                        Text {
                                            text: root.monitorVm.selectedDeviceTemperature > 0
                                                  ? root.monitorVm.selectedDeviceTemperature + "\u00B0C"
                                                  : "--"
                                            color: root.monitorVm.selectedDeviceOnline
                                                   ? Theme.textPrimary : Theme.textDisabled
                                            font.pixelSize: Theme.fontSizeLG
                                            font.bold: true
                                        }
                                    }

                                    // Separator
                                    Rectangle {
                                        Layout.fillHeight: true
                                        Layout.preferredWidth: 1
                                        color: Theme.borderSubtle
                                    }

                                    // IP address
                                    Column {
                                        spacing: 2
                                        Text {
                                            text: qsTr("IP 地址")
                                            color: Theme.textTertiary
                                            font.pixelSize: Theme.fontSizeSM
                                        }
                                        Text {
                                            text: root.monitorVm.selectedDeviceIp || "--"
                                            color: Theme.textSecondary
                                            font.pixelSize: Theme.fontSizeLG
                                        }
                                    }

                                    Item { Layout.fillWidth: true }

                                    // Refresh button
                                    Rectangle {
                                        width: refreshBtn.implicitWidth + 16
                                        height: 28; radius: 4
                                        color: refreshBtnHover.containsMouse ? Theme.bgHover : Theme.bgElevated
                                        border.width: 1; border.color: Theme.borderSubtle
                                        Text {
                                            id: refreshBtn
                                            anchors.centerIn: parent
                                            text: qsTr("\u21BB 刷新状态")
                                            color: Theme.textSecondary
                                            font.pixelSize: Theme.fontSizeSM
                                        }
                                        HoverHandler { id: refreshBtnHover }
                                        TapHandler { onTapped: root.monitorVm.refresh() }
                                    }
                                }
                            }

                            // ── Print progress card (visible when printing) ──
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 88
                                radius: Theme.radiusXL
                                color: Theme.bgPanel
                                border.width: 1; border.color: Theme.borderSubtle
                                visible: root.monitorVm.selectedDeviceStatus === "printing"

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: Theme.spacingLG
                                    spacing: Theme.spacingSM

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: Theme.spacingMD

                                        Text {
                                            text: qsTr("当前任务")
                                            color: Theme.textSecondary
                                            font.pixelSize: Theme.fontSizeMD
                                        }
                                        Item { Layout.fillWidth: true }
                                        Text {
                                            text: root.monitorVm.selectedDeviceProgress + "%"
                                            color: Theme.accent
                                            font.pixelSize: Theme.fontSizeXXL
                                            font.bold: true
                                        }
                                    }

                                    // Task name
                                    Text {
                                        Layout.fillWidth: true
                                        text: root.monitorVm.selectedDeviceTaskName || qsTr("未知任务")
                                        color: Theme.textPrimary
                                        font.pixelSize: Theme.fontSizeMD
                                        elide: Text.ElideMiddle
                                    }

                                    // Progress bar
                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 6
                                        radius: 3
                                        color: Theme.borderSubtle

                                        Rectangle {
                                            width: parent.width * (root.monitorVm.selectedDeviceProgress / 100)
                                            height: parent.height
                                            radius: 3
                                            color: Theme.accent
                                        }
                                    }
                                }
                            }

                            // ── Print control row (对齐上游 Monitor StatusPanel 打印控制) ──
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                radius: Theme.radiusXL
                                color: Theme.bgPanel
                                border.width: 1; border.color: Theme.borderSubtle
                                visible: root.monitorVm.selectedDeviceOnline

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: Theme.spacingLG
                                    anchors.rightMargin: Theme.spacingLG
                                    spacing: Theme.spacingSM

                                    // Print stage + layer info
                                    Text {
                                        text: {
                                            var stage = root.monitorVm.selectedPrintStage
                                            if (stage === "heating") return qsTr("加热中...")
                                            if (stage === "mop_first_layer") return qsTr("首层打印中")
                                            if (stage === "cooling") return qsTr("冷却中...")
                                            if (stage) return qsTr("打印中")
                                            return ""
                                        }
                                        color: Theme.accent
                                        font.pixelSize: Theme.fontSizeMD
                                        font.bold: true
                                        visible: root.monitorVm.selectedDeviceStatus === "printing"
                                    }

                                    Text {
                                        text: root.monitorVm.selectedPrintTimeLeft > 0
                                              ? qsTr("剩余 %1").arg(formatTimeLeft(root.monitorVm.selectedPrintTimeLeft))
                                              : ""
                                        color: Theme.textSecondary
                                        font.pixelSize: Theme.fontSizeSM
                                        visible: root.monitorVm.selectedDeviceStatus === "printing"
                                    }

                                    Text {
                                        text: root.monitorVm.selectedPrintLayer > 0
                                              ? qsTr("层 %1").arg(root.monitorVm.selectedPrintLayer)
                                              : ""
                                        color: Theme.textTertiary
                                        font.pixelSize: Theme.fontSizeSM
                                        visible: root.monitorVm.selectedDeviceStatus === "printing"
                                    }

                                    Item { Layout.fillWidth: true }

                                    // Connect/Disconnect button
                                    Rectangle {
                                        width: 70; height: 26; radius: 4
                                        color: connectMA.containsMouse ? "#dc2626" : "#ef4444"
                                        visible: root.monitorVm.selectedDeviceOnline && root.monitorVm.selectedDeviceStatus !== "printing"
                                        Text {
                                            anchors.centerIn: parent
                                            text: qsTr("断开")
                                            color: "white"
                                            font.pixelSize: 11
                                        }
                                        MouseArea {
                                            id: connectMA
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.monitorVm.disconnectDevice(0)
                                        }
                                    }

                                    // Start print button
                                    Rectangle {
                                        width: 70; height: 26; radius: 4
                                        color: startMA.containsMouse ? "#16a34a" : "#22c55e"
                                        visible: root.monitorVm.selectedDeviceOnline && root.monitorVm.selectedDeviceStatus === "idle"
                                        Text {
                                            anchors.centerIn: parent
                                            text: qsTr("打印")
                                            color: "white"
                                            font.pixelSize: 11
                                        }
                                        MouseArea {
                                            id: startMA
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.monitorVm.startPrint(0, "")
                                        }
                                    }

                                    // Pause button
                                    Rectangle {
                                        width: 60; height: 26; radius: 4
                                        color: pauseMA.containsMouse ? "#d97706" : "#f59e0b"
                                        visible: root.monitorVm.selectedDeviceStatus === "printing"
                                        Text {
                                            anchors.centerIn: parent
                                            text: qsTr("暂停")
                                            color: "white"
                                            font.pixelSize: 11
                                        }
                                        MouseArea {
                                            id: pauseMA
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.monitorVm.pausePrint(0)
                                        }
                                    }

                                    // Resume button
                                    Rectangle {
                                        width: 60; height: 26; radius: 4
                                        color: resumeMA.containsMouse ? "#16a34a" : "#22c55e"
                                        visible: root.monitorVm.selectedDeviceStatus === "paused"
                                        Text {
                                            anchors.centerIn: parent
                                            text: qsTr("继续")
                                            color: "white"
                                            font.pixelSize: 11
                                        }
                                        MouseArea {
                                            id: resumeMA
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.monitorVm.resumePrint(0)
                                        }
                                    }

                                    // Stop button
                                    Rectangle {
                                        width: 60; height: 26; radius: 4
                                        color: stopMA.containsMouse ? "#dc2626" : "#ef4444"
                                        visible: root.monitorVm.selectedDeviceStatus === "printing" || root.monitorVm.selectedDeviceStatus === "paused"
                                        Text {
                                            anchors.centerIn: parent
                                            text: qsTr("停止")
                                            color: "white"
                                            font.pixelSize: 11
                                        }
                                        MouseArea {
                                            id: stopMA
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.monitorVm.stopPrint(0)
                                        }
                                    }

                                    // Connect offline device button
                                    Rectangle {
                                        width: 70; height: 26; radius: 4
                                        color: connOnMA.containsMouse ? "#2563eb" : "#3b82f6"
                                        visible: !root.monitorVm.selectedDeviceOnline && root.monitorVm.selectedDeviceStatus === "offline"
                                        Text {
                                            anchors.centerIn: parent
                                            text: qsTr("连接")
                                            color: "white"
                                            font.pixelSize: 11
                                        }
                                        MouseArea {
                                            id: connOnMA
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.monitorVm.connectDevice(0)
                                        }
                                    }
                                }

                                function formatTimeLeft(secs) {
                                    if (secs <= 0) return "0s"
                                    var h = Math.floor(secs / 3600)
                                    var m = Math.floor((secs % 3600) / 60)
                                    if (h > 0) return h + "h" + m + "m"
                                    return m + "m" + (secs % 60) + "s"
                                }
                            }

                            // ── Device info cards row ──
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: Theme.spacingLG

                                // Connection type card
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: Theme.radiusXL
                                    color: Theme.bgPanel
                                    border.width: 1; border.color: Theme.borderSubtle

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: Theme.spacingLG
                                        spacing: Theme.spacingMD

                                        Text {
                                            text: qsTr("连接方式")
                                            color: Theme.textTertiary
                                            font.pixelSize: Theme.fontSizeSM
                                        }
                                        Text {
                                            text: "LAN"
                                            color: Theme.textPrimary
                                            font.pixelSize: Theme.fontSizeXL
                                            font.bold: true
                                        }
                                        Item { Layout.fillHeight: true }
                                        Text {
                                            text: qsTr("局域网直连")
                                            color: Theme.textTertiary
                                            font.pixelSize: Theme.fontSizeXS
                                        }
                                    }
                                }

                                // Signal card
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: Theme.radiusXL
                                    color: Theme.bgPanel
                                    border.width: 1; border.color: Theme.borderSubtle

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: Theme.spacingLG
                                        spacing: Theme.spacingMD

                                        Text {
                                            text: qsTr("信号强度")
                                            color: Theme.textTertiary
                                            font.pixelSize: Theme.fontSizeSM
                                        }

                                        Row {
                                            spacing: 4
                                            Repeater {
                                                model: 4
                                                Rectangle {
                                                    width: 6
                                                    height: 8 + index * 4
                                                    radius: 1
                                                    color: index < root.monitorVm.selectedDeviceSignalStrength
                                                           ? Theme.statusSuccess : Theme.borderSubtle
                                                    anchors.bottom: parent.bottom
                                                }
                                            }
                                        }

                                        Item { Layout.fillHeight: true }

                                        Text {
                                            text: {
                                                var s = root.monitorVm.selectedDeviceSignalStrength
                                                if (s >= 3) return qsTr("信号优秀")
                                                if (s >= 2) return qsTr("信号良好")
                                                if (s >= 1) return qsTr("信号较弱")
                                                return qsTr("无信号")
                                            }
                                            color: Theme.textTertiary
                                            font.pixelSize: Theme.fontSizeXS
                                        }
                                    }
                                }

                                // SN card
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: Theme.radiusXL
                                    color: Theme.bgPanel
                                    border.width: 1; border.color: Theme.borderSubtle

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: Theme.spacingLG
                                        spacing: Theme.spacingMD

                                        Text {
                                            text: qsTr("序列号")
                                            color: Theme.textTertiary
                                            font.pixelSize: Theme.fontSizeSM
                                        }
                                        Text {
                                            text: "CP\u2026\u2026"
                                            color: Theme.textPrimary
                                            font.pixelSize: Theme.fontSizeXL
                                            font.bold: true
                                        }
                                        Item { Layout.fillHeight: true }
                                        Text {
                                            text: qsTr("固件版本: 1.0.0")
                                            color: Theme.textTertiary
                                            font.pixelSize: Theme.fontSizeXS
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Tab 1: SD Card (对齐上游 MediaFilePanel)
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12
                        visible: tabBar.currentIndex === 1

                        // Header row
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: qsTr("SD 卡文件管理"); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeLG; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text { text: "8 " + qsTr("个文件"); color: Theme.textTertiary; font.pixelSize: 11 }
                        }

                        // Storage bar
                        Rectangle {
                            Layout.fillWidth: true
                            height: 24
                            radius: 6
                            color: Theme.bgElevated
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 4
                                spacing: 8
                                Text { text: qsTr("已用"); color: Theme.textTertiary; font.pixelSize: 10 }
                                Rectangle {
                                    Layout.fillWidth: true; height: 8; radius: 4; color: "#2a3040"
                                    Rectangle { width: parent.width * 0.35; height: parent.height; radius: 4; color: "#22c55e" }
                                }
                                Text { text: "1.8 / 5.2 GB"; color: Theme.textSecondary; font.pixelSize: 10 }
                            }
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSubtle }

                        // File list header
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: qsTr("文件名"); color: Theme.textTertiary; font.pixelSize: 10; Layout.fillWidth: true }
                            Text { text: qsTr("大小"); color: Theme.textTertiary; font.pixelSize: 10; Layout.preferredWidth: 60 }
                            Text { text: qsTr("日期"); color: Theme.textTertiary; font.pixelSize: 10; Layout.preferredWidth: 90 }
                        }

                        // Mock file list (对齐 upstream MediaFilePanel file table)
                        Repeater {
                            model: [
                                { name: "test_cube.gcode", size: "2.3 MB", date: "2026-03-14" },
                                { name: "benchy_0.2mm.gcode", size: "8.7 MB", date: "2026-03-13" },
                                { name: "calibration_flow.gcode", size: "1.1 MB", date: "2026-03-12" },
                                { name: "dragon_v2.gcode", size: "15.2 MB", date: "2026-03-11" },
                                { name: "support_test.gcode", size: "3.4 MB", date: "2026-03-10" },
                                { name: "multi_part.gcode", size: "22.1 MB", date: "2026-03-09" },
                                { name: "tower_test.gcode", size: "4.5 MB", date: "2026-03-08" },
                                { name: "random_box.gcode", size: "1.8 MB", date: "2026-03-07" }
                            ]
                            delegate: Rectangle {
                                Layout.fillWidth: true
                                height: 34
                                radius: 6
                                color: fileMA.containsMouse ? Theme.bgHover : "transparent"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 8
                                    spacing: 4

                                    Text { text: modelData.name; color: Theme.textPrimary; font.pixelSize: 11; font.family: "monospace"; Layout.fillWidth: true; elide: Text.ElideRight }
                                    Text { text: modelData.size; color: Theme.textSecondary; font.pixelSize: 10; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight }
                                    Text { text: modelData.date; color: Theme.textDisabled; font.pixelSize: 10; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight }
                                }

                                MouseArea {
                                    id: fileMA
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        // Bottom actions
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                Layout.preferredWidth: 80; height: 28; radius: 6
                                color: importMA.containsMouse ? Theme.bgHover : Theme.bgElevated
                                border.width: 1; border.color: Theme.borderSubtle
                                Text { anchors.centerIn: parent; text: qsTr("导入"); color: Theme.textSecondary; font.pixelSize: 11 }
                                MouseArea { id: importMA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor }
                            }
                            Rectangle {
                                Layout.preferredWidth: 80; height: 28; radius: 6
                                color: deleteMA.containsMouse ? "#7d2020" : Theme.bgElevated
                                border.width: 1; border.color: Theme.borderSubtle
                                Text { anchors.centerIn: parent; text: qsTr("删除"); color: deleteMA.containsMouse ? "#ffaaaa" : Theme.textSecondary; font.pixelSize: 11 }
                                MouseArea { id: deleteMA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor }
                            }
                        }
                    }

                    // Tab 2: Video / Camera（对齐 upstream CameraPopup / MediaPlayCtrl）
                    Item {
                        visible: tabBar.currentIndex === 2
                        anchors.fill: parent

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Theme.spacingXL
                            spacing: Theme.spacingLG

                            // ── Camera preview area ──
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: Theme.radiusXL
                                color: Theme.bgPanel
                                border.width: 1; border.color: Theme.borderSubtle

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: Theme.spacingLG
                                    spacing: Theme.spacingMD

                                    // Video viewport (mock placeholder)
                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        radius: Theme.radiusMD
                                        color: "#0a0e14"
                                        border.width: 1; border.color: "#1e2836"

                                        Column {
                                            anchors.centerIn: parent
                                            spacing: 12

                                            // Stream status icon
                                            Text {
                                                text: {
                                                    var s = root.monitorVm.cameraStreamStatus
                                                    if (s === 1) return "\u{1F50C}" // connecting
                                                    if (s === 2) return "\u2705"     // connected
                                                    if (s === 3) return "\u{1F3A5}" // streaming
                                                    if (s === 4) return "\u274C"     // error
                                                    return "\u{1F4F7}"              // disconnected
                                                }
                                                font.pixelSize: 48
                                                horizontalAlignment: Text.AlignHCenter
                                            }

                                            // Status text
                                            Text {
                                                text: {
                                                    var s = root.monitorVm.cameraStreamStatus
                                                    if (s === 0) return qsTr("摄像头未连接")
                                                    if (s === 1) return qsTr("正在连接...")
                                                    if (s === 2) return qsTr("已连接，等待视频流")
                                                    if (s === 3) return qsTr("视频流传输中")
                                                    return qsTr("连接失败")
                                                }
                                                color: Theme.textSecondary
                                                font.pixelSize: Theme.fontSizeLG
                                                horizontalAlignment: Text.AlignHCenter
                                            }

                                            // Error message
                                            Text {
                                                text: root.monitorVm.cameraErrorMessage
                                                color: "#ef4444"
                                                font.pixelSize: Theme.fontSizeSM
                                                horizontalAlignment: Text.AlignHCenter
                                                visible: root.monitorVm.cameraErrorMessage !== ""
                                            }
                                        }

                                        // Recording indicator overlay (对齐 upstream CameraRecordingStatus)
                                        Rectangle {
                                            anchors.top: parent.top
                                            anchors.right: parent.right
                                            anchors.margins: 8
                                            width: recordingLabel.implicitWidth + 16
                                            height: 24
                                            radius: 12
                                            color: root.monitorVm.cameraRecordingStatus >= 3
                                                    ? "rgba(239,68,68,0.85)" : "transparent"
                                            visible: root.monitorVm.cameraRecordingStatus >= 3

                                            RowLayout {
                                                anchors.centerIn: parent
                                                spacing: 4
                                                Rectangle { width: 8; height: 8; radius: 4; color: "#ff0000" }
                                                Text {
                                                    id: recordingLabel
                                                    text: qsTr("REC")
                                                    color: "white"
                                                    font.pixelSize: 11
                                                    font.bold: true
                                                }
                                            }
                                        }

                                        // Timelapse indicator (对齐 upstream CameraTimelapseStatus)
                                        Rectangle {
                                            anchors.top: parent.top
                                            anchors.left: parent.left
                                            anchors.margins: 8
                                            width: timelapseLabel.implicitWidth + 16
                                            height: 24
                                            radius: 12
                                            color: root.monitorVm.cameraTimelapseStatus >= 3
                                                    ? "rgba(59,130,246,0.85)" : "transparent"
                                            visible: root.monitorVm.cameraTimelapseStatus >= 3

                                            RowLayout {
                                                anchors.centerIn: parent
                                                spacing: 4
                                                Text {
                                                    text: "\u{1F4F8}"
                                                    font.pixelSize: 11
                                                }
                                                Text {
                                                    id: timelapseLabel
                                                    text: qsTr("延时摄影")
                                                    color: "white"
                                                    font.pixelSize: 11
                                                    font.bold: true
                                                }
                                            }
                                        }
                                    }

                                    // ── Camera control toolbar (对齐 upstream CameraPopup controls) ──
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: Theme.spacingSM

                                        // Start/Stop stream
                                        Rectangle {
                                            width: 70; height: 28; radius: 4
                                            color: streamMA.containsMouse
                                                    ? (root.monitorVm.cameraStreamStatus <= 1 ? "#dc2626" : "#6b7280")
                                                    : (root.monitorVm.cameraStreamStatus <= 1 ? "#ef4444" : "#4b5563")
                                            Text {
                                                anchors.centerIn: parent
                                                text: root.monitorVm.cameraStreamStatus <= 1
                                                       ? qsTr("连接") : qsTr("断开")
                                                color: "white"
                                                font.pixelSize: 11
                                            }
                                            MouseArea {
                                                id: streamMA
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    if (root.monitorVm.cameraStreamStatus <= 1)
                                                        root.monitorVm.startCameraStream()
                                                    else
                                                        root.monitorVm.stopCameraStream()
                                                }
                                            }
                                        }

                                        // Toggle recording
                                        Rectangle {
                                            width: 70; height: 28; radius: 4
                                            color: recMA.containsMouse
                                                    ? (root.monitorVm.cameraRecordingStatus >= 3 ? "#dc2626" : "#16a34a")
                                                    : (root.monitorVm.cameraRecordingStatus >= 3 ? "#ef4444" : "#22c55e")
                                            visible: root.monitorVm.cameraStreamStatus === 3
                                            Text {
                                                anchors.centerIn: parent
                                                text: root.monitorVm.cameraRecordingStatus >= 3
                                                       ? qsTr("停止录像") : qsTr("录像")
                                                color: "white"
                                                font.pixelSize: 11
                                            }
                                            MouseArea {
                                                id: recMA
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: root.monitorVm.toggleCameraRecording()
                                            }
                                        }

                                        // Toggle timelapse
                                        Rectangle {
                                            width: 80; height: 28; radius: 4
                                            color: tlMA.containsMouse
                                                    ? (root.monitorVm.cameraTimelapseStatus >= 3 ? "#d97706" : "#2563eb")
                                                    : (root.monitorVm.cameraTimelapseStatus >= 3 ? "#f59e0b" : "#3b82f6")
                                            visible: root.monitorVm.cameraStreamStatus === 3
                                            Text {
                                                anchors.centerIn: parent
                                                text: root.monitorVm.cameraTimelapseStatus >= 3
                                                       ? qsTr("停止延时") : qsTr("延时摄影")
                                                color: "white"
                                                font.pixelSize: 11
                                            }
                                            MouseArea {
                                                id: tlMA
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: root.monitorVm.toggleCameraTimelapse()
                                            }
                                        }

                                        Item { Layout.fillWidth: true }

                                        // Switch camera
                                        Rectangle {
                                            width: 60; height: 28; radius: 4
                                            color: switchMA.containsMouse ? "#374151" : "#1f2937"
                                            visible: root.monitorVm.cameraStreamStatus === 3
                                            Text {
                                                anchors.centerIn: parent
                                                text: qsTr("切换")
                                                color: Theme.textSecondary
                                                font.pixelSize: 11
                                            }
                                            MouseArea {
                                                id: switchMA
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: root.monitorVm.switchCameraView()
                                            }
                                        }

                                        // Screenshot
                                        Rectangle {
                                            width: 50; height: 28; radius: 4
                                            color: shotMA.containsMouse ? "#374151" : "#1f2937"
                                            visible: root.monitorVm.cameraStreamStatus === 3
                                            Text {
                                                anchors.centerIn: parent
                                                text: "\u{1F4F7}"
                                                font.pixelSize: 14
                                            }
                                            MouseArea {
                                                id: shotMA
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: root.monitorVm.takeCameraScreenshot()
                                            }
                                        }
                                    }

                                    // ── Resolution + URL settings (对齐 upstream CameraPopup resolution switch) ──
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: Theme.spacingMD

                                        Text {
                                            text: qsTr("分辨率:")
                                            color: Theme.textTertiary
                                            font.pixelSize: 11
                                        }
                                        Row {
                                            spacing: 4
                                            Repeater {
                                                model: ["720P", "1080P"]
                                                Rectangle {
                                                    width: 52; height: 22; radius: 4
                                                    color: index === root.monitorVm.cameraResolution
                                                           ? Theme.accent : "#1f2937"
                                                    border.width: 1
                                                    border.color: index === root.monitorVm.cameraResolution
                                                                ? Theme.accent : "#374151"
                                                    Text {
                                                        anchors.centerIn: parent
                                                        text: modelData
                                                        color: index === root.monitorVm.cameraResolution
                                                               ? "white" : Theme.textTertiary
                                                        font.pixelSize: 11
                                                    }
                                                    MouseArea {
                                                        anchors.fill: parent
                                                        cursorShape: Qt.PointingHandCursor
                                                        onClicked: root.monitorVm.setCameraResolution(index)
                                                    }
                                                }
                                            }
                                        }

                                        Item { Layout.fillWidth: true }

                                        Text {
                                            text: qsTr("URL:")
                                            color: Theme.textTertiary
                                            font.pixelSize: 11
                                        }
                                        TextField {
                                            Layout.preferredWidth: 180
                                            Layout.preferredHeight: 22
                                            text: root.monitorVm.cameraUrl
                                            placeholderText: "rtsp://..."
                                            color: Theme.textPrimary
                                            font.pixelSize: 11
                                            onEditingFinished: root.monitorVm.setCameraUrl(text)
                                            background: Rectangle {
                                                radius: 4
                                                color: "#1e2229"
                                                border.color: parent.activeFocus ? Theme.accent : "#2e3540"
                                                border.width: 1
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Tab 3: HMS（对齐上游 HMSPanel / DeviceManager hms_list）
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.spacingXL
                        spacing: Theme.spacingLG
                        visible: tabBar.currentIndex === 3

                        // Header row
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: qsTr("设备健康监控 (HMS)"); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeLG; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: root.monitorVm.selectedUnreadHmsCount > 0
                                      ? qsTr("%1 条未读").arg(root.monitorVm.selectedUnreadHmsCount)
                                      : qsTr("暂无告警")
                                color: root.monitorVm.selectedUnreadHmsCount > 0 ? "#f59e0b" : Theme.textTertiary
                                font.pixelSize: Theme.fontSizeSM
                            }
                        }

                        // HMS item count
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("共 %1 条记录").arg(root.monitorVm.selectedHmsCount)
                            color: Theme.textDisabled
                            font.pixelSize: Theme.fontSizeXS
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSubtle }

                        // HMS list
                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            ScrollBar.vertical.policy: ScrollBar.AsNeeded

                            Column {
                                width: parent.width
                                spacing: 6

                                Repeater {
                                    model: root.monitorVm.selectedHmsCount

                                    delegate: Rectangle {
                                        width: parent.width - 4
                                        height: 64
                                        radius: Theme.radiusLG
                                        color: hmsData.alreadyRead ? Theme.bgElevated : "#1a1e25"
                                        border.width: 1
                                        border.color: {
                                            if (hmsData.msgLevel === 1) return "#ef4444"
                                            if (hmsData.msgLevel === 2) return "#f97316"
                                            if (hmsData.msgLevel === 3) return "#f59e0b"
                                            return "#22c55e"
                                        }
                                        opacity: hmsData.alreadyRead ? 0.3 : 1.0

                                        property var hmsData: root.monitorVm.hmsAt(index)

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: Theme.spacingLG
                                            anchors.rightMargin: Theme.spacingLG
                                            spacing: Theme.spacingMD

                                            // Severity icon
                                            Rectangle {
                                                width: 32; height: 32; radius: Theme.radiusMD
                                                color: {
                                                    if (hmsData.msgLevel === 1) return "#2a1418"
                                                    if (hmsData.msgLevel === 2) return "#2a1e10"
                                                    if (hmsData.msgLevel === 3) return "#2a2510"
                                                    return "#1a2218"
                                                }
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: {
                                                        if (hmsData.msgLevel === 1) return "\u{1F6A8}"
                                                        if (hmsData.msgLevel === 2) return "\u26A0"
                                                        if (hmsData.msgLevel === 3) return "\u26A0"
                                                        return "\u2139"
                                                    }
                                                    font.pixelSize: 16
                                                    color: {
                                                        if (hmsData.msgLevel === 1) return "#ef4444"
                                                        if (hmsData.msgLevel === 2) return "#f97316"
                                                        if (hmsData.msgLevel === 3) return "#f59e0b"
                                                        return "#22c55e"
                                                    }
                                                }
                                            }

                                            // Message content
                                            Column {
                                                Layout.fillWidth: true
                                                spacing: 3

                                                Row {
                                                    spacing: Theme.spacingSM
                                                    Rectangle {
                                                        width: levelBadge.implicitWidth + 10; height: 16; radius: 8
                                                        color: {
                                                            if (hmsData.msgLevel === 1) return "#2a1418"
                                                            if (hmsData.msgLevel === 2) return "#2a1e10"
                                                            if (hmsData.msgLevel === 3) return "#2a2510"
                                                            return "#1a2218"
                                                        }
                                                        Text {
                                                            id: levelBadge
                                                            anchors.centerIn: parent
                                                            text: {
                                                                if (hmsData.msgLevel === 1) return qsTr("严重")
                                                                if (hmsData.msgLevel === 2) return qsTr("警告")
                                                                if (hmsData.msgLevel === 3) return qsTr("一般")
                                                                return qsTr("信息")
                                                            }
                                                            color: {
                                                                if (hmsData.msgLevel === 1) return "#ef4444"
                                                                if (hmsData.msgLevel === 2) return "#f97316"
                                                                if (hmsData.msgLevel === 3) return "#f59e0b"
                                                                return "#22c55e"
                                                            }
                                                            font.pixelSize: Theme.fontSizeXS
                                                            font.bold: true
                                                        }
                                                    }

                                                    Rectangle {
                                                        width: 6; height: 6; radius: 3
                                                        visible: !hmsData.alreadyRead
                                                        color: Theme.accent
                                                        anchors.verticalCenter: parent.verticalCenter
                                                    }

                                                    Text {
                                                        visible: hmsData.msgCode > 0
                                                        text: "#" + hmsData.msgCode
                                                        color: Theme.textDisabled
                                                        font.pixelSize: Theme.fontSizeXS
                                                        anchors.verticalCenter: parent.verticalCenter
                                                    }
                                                }

                                                Text {
                                                    Layout.fillWidth: true
                                                    text: hmsData.message || ""
                                                    color: Theme.textPrimary
                                                    font.pixelSize: Theme.fontSizeSM
                                                    wrapMode: Text.Wrap
                                                    maximumLineCount: 2
                                                    elide: Text.ElideRight
                                                }
                                            }

                                            Item { Layout.fillWidth: true }

                                            Text {
                                                text: {
                                                    if (hmsData.moduleId === 0) return qsTr("喷头")
                                                    if (hmsData.moduleId === 1) return qsTr("热床")
                                                    if (hmsData.moduleId === 2) return qsTr("挤出机")
                                                    return qsTr("其他")
                                                }
                                                color: Theme.textDisabled
                                                font.pixelSize: Theme.fontSizeXS
                                            }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.monitorVm.markHmsRead(index)
                                        }
                                    }
                                }

                                // Empty state
                                Column {
                                    width: parent.width
                                    visible: root.monitorVm.selectedHmsCount === 0
                                    spacing: 8
                                    anchors.horizontalCenter: parent.horizontalCenter

                                    Text {
                                        text: "\u2705"
                                        font.pixelSize: 36
                                        color: Theme.textDisabled
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                    Text {
                                        text: qsTr("设备运行正常，暂无告警")
                                        color: Theme.textTertiary
                                        font.pixelSize: Theme.fontSizeMD
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                }
                            }
                        }

                        // Clear all button
                        Rectangle {
                            Layout.alignment: Qt.AlignRight
                            visible: root.monitorVm.selectedHmsCount > 0
                            width: clearHmsMA.containsMouse ? Theme.bgHover : Theme.bgElevated
                            height: 28; radius: 6
                            border.width: 1; border.color: Theme.borderSubtle
                            Text {
                                anchors.centerIn: parent
                                text: qsTr("全部标为已读")
                                color: Theme.textSecondary
                                font.pixelSize: 11
                            }
                            MouseArea {
                                id: clearHmsMA
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    for (var i = 0; i < root.monitorVm.selectedHmsCount; ++i)
                                        root.monitorVm.markHmsRead(i)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
