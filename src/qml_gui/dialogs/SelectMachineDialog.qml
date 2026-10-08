import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// SelectMachineDialog.qml — DEV-04 选择打印机发送 G-code
//
// 上游: SelectMachine.cpp SelectMachineDialog (Send print job)
// 结构对齐 (SelectMachine.cpp 行号):
//   :108      DPIDialog 标题 "Send print job"          → dialogTitle 700 宽画布
//   :150-154  wxScrolledWindow 700x600 scroll rate(0,20) → Flickable 视口 600
//   :156-157  m_line_top 1px                            → 顶部 1px 分隔线
//   :185-200  ThumbnailPanel 198x198                    → 板缩略图面板
//   :205-313  重命名行 / 时间重量行 / 多板翻页钮          → 基础信息右列
//   :6993-7084 PrinterInfoBox(标题+ComboBox 300x60+刷新+热床盒 68x68)
//   :364-417  Filament 分节 + 耗材面板(637 宽, 10 列网格 gap 7 边距 10)
//   :467-550  AMS 消息/建议/换料次数/烘干警告/外置进料复选框
//   :552-649  高级选项 2 列网格 5 个 PrintOption(分段选择器每段 44 宽)
//   :651-705  底部 645x32 三态条 prepare/sending/finish(无独立取消按钮)
//   :707-783  发送失败信息区 645x125(Error code/desc/extra + 网络链接)
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    // SelectMachine.cpp:108 _L("Send print job")
    dialogTitle: qsTr("发送打印任务")
    // m_scroll_area 700x600 (SelectMachine.cpp:153-154) + 底部三态条 32
    // (hpp:115 SIMBOOK_SIZE2) + 间距 10/18 (SelectMachine.cpp:816-818) + 头部 44
    width: 700
    height: 44 + 600 + 10 + 32 + 18 + (failInfo.visible ? 125 : 0)
    padding: 0

    required property var deviceVm   // MonitorViewModel
    required property string gcodePath  // 要发送的 G-code 路径

    // 板缩略图 / 项目名 / 时间重量来自编辑器 VM（只读，不回写其文件）
    readonly property var editorVm: (typeof backend !== "undefined" && backend !== null)
                                    ? backend.editorViewModel : null

    // 对话框内选中的设备（filtered index，同步到 deviceVm 全局选中）
    property int comboIndex: -1
    // 重命名状态（上游 m_rename_switch_panel 文本/输入两页切换, :208-261）
    property bool renameEditing: false
    property string renameDraft: ""
    readonly property var comboDevice: (deviceVm && comboIndex >= 0 && comboIndex < (deviceVm.filteredDeviceCount || 0))
                                       ? deviceVm.deviceAt(comboIndex) : null
    // 发送三态（SelectMachine.cpp:651-705）: 0=prepare 1=sending 2=finish
    readonly property int sendState: deviceVm ? deviceVm.sendJobState : 0

    // R-P1.E: require an online device AND a real G-code path. Offline
    // devices were selectable and accept() ran even without a device VM or
    // path (silent no-op shown as success).
    readonly property bool canSend: {
        if (!deviceVm || gcodePath === "" || comboIndex < 0)
            return false
        const dev = deviceVm.deviceAt(comboIndex)
        return !!dev && dev.online === true
    }

    // 上游 PrintOption 定义（SelectMachine.cpp:565-614）:
    // timelapse/pa_value 为 On/Off，其余 Auto/On/Off；初始仅 timelapse 可见
    // （SelectMachine.cpp:646-649），其余按设备能力条件显示。
    readonly property var printOptionDefs: [
        { key: "timelapse", title: qsTr("延时摄影"), ops: ["on", "off"],
          tip: qsTr("打印过程中拍摄延时摄影视频。") },
        { key: "bed_leveling", title: qsTr("自动热床调平"), ops: ["auto", "on", "off"],
          tip: qsTr("检查热床平面度，使挤出高度均匀。\n*自动模式：进行调平检查（约 10 秒），表面良好时跳过。") },
        { key: "flow_cali", title: qsTr("流量动态校准"), ops: ["auto", "on", "off"],
          tip: qsTr("测定动态流量值以提升打印质量。\n*自动模式：耗材近期已校准时跳过。") },
        { key: "nozzle_offset_cali", title: qsTr("喷嘴偏移校准"), ops: ["auto", "on", "off"],
          tip: qsTr("校准喷嘴偏移以提升打印质量。\n*自动模式：打印前检查校准，无需校准时跳过。") },
        { key: "pa_value", title: qsTr("共享 PA 配置文件"), ops: ["on", "off"],
          tip: qsTr("同类型喷嘴与耗材共享同一 PA 配置文件。") }
    ]

    // 缩略图数据 URL（PreparePage.qml:266-270 同款封装）
    function thumbnailSource(data) {
        if (!data || data.length === 0)
            return ""
        return data.indexOf("data:image/") === 0 ? data : "data:image/png;base64," + data
    }

    onOpened: {
        // 复位三态条并预选设备（上游每次打开重新进入 prepare 页）
        if (deviceVm)
            deviceVm.cancelSendJob()
        if (deviceVm && comboIndex < 0 && deviceVm.filteredDeviceCount > 0)
            comboIndex = 0
        if (deviceVm && comboIndex >= 0)
            deviceVm.selectDevice(comboIndex)
    }

    // 上游 on_cancel：发送中关闭窗口即中止发送 worker（SelectMachine.cpp:139）
    onRejected: {
        if (deviceVm && deviceVm.sendJobState === 1)
            deviceVm.cancelSendJob()
    }

    contentItem: Rectangle {
        color: Theme.bgElevated
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // ── m_scroll_area（SelectMachine.cpp:150-154, 700x600, 垂直滚动）──
            Flickable {
                id: scrollArea
                Layout.fillWidth: true
                Layout.preferredHeight: 600
                clip: true
                contentWidth: width
                contentHeight: scrollColumn.height
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    policy: scrollArea.contentHeight > scrollArea.height
                            ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                }

                ColumnLayout {
                    id: scrollColumn
                    width: scrollArea.width
                    spacing: 0

                    // m_line_top 1px（SelectMachine.cpp:156-157, 色 166,169,170）
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.leftMargin: 15
                        Layout.rightMargin: 15
                        Layout.preferredHeight: 1
                        color: Theme.borderSubtle
                    }
                    Item { Layout.fillWidth: true; Layout.preferredHeight: 11 }  // :786 wxTOP 11

                    // ── m_basic_panel（:179-361，左右边距 15）─────────────
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 15
                        Layout.rightMargin: 15
                        spacing: 8  // :357 wxLEFT 8

                        // 缩略图面板 198x198（SelectMachine.cpp:190-200）
                        Rectangle {
                            Layout.preferredWidth: 198
                            Layout.preferredHeight: 198
                            Layout.alignment: Qt.AlignTop
                            color: Theme.bgInset
                            radius: Theme.radiusMD

                            Image {
                                id: thumbImage
                                anchors.fill: parent
                                anchors.margins: 1
                                visible: root.editorVm !== null
                                         && root.thumbnailSource(root.editorVm.plateThumbnailBase64(root.editorVm.currentPlateIndex)) !== ""
                                source: visible ? root.thumbnailSource(
                                                      root.editorVm.plateThumbnailBase64(root.editorVm.currentPlateIndex))
                                                : ""
                                fillMode: Image.PreserveAspectFit
                            }
                            // 无缩略图时的占位（上游 ThumbnailPanel 空态）
                            Image {
                                anchors.centerIn: parent
                                width: 40; height: 40
                                visible: !thumbImage.visible
                                source: "qrc:/qml/assets/icons/bed_cool.png"
                                opacity: 0.5
                            }
                        }

                        // 基础信息右列（sizer_basic_right_info, :344-353）
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            spacing: 0

                            // 重命名行 360x25（:208-233；Head_14 = fontSizeLG）
                            Item {
                                Layout.preferredWidth: 360
                                Layout.preferredHeight: 25
                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 3  // :230 wxLEFT 3

                                    Text {
                                        visible: !root.renameEditing
                                        Layout.maximumWidth: 340  // :222 MaxSize 340 宽
                                        text: root.editorVm ? root.editorVm.projectName : ""
                                        color: Theme.textPrimary
                                        font.pixelSize: Theme.fontSizeLG  // Head_14
                                        elide: Text.ElideRight             // wxST_ELLIPSIZE_END
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    // rename_edit 13px 图标置于 20x20 热区（:223-227）
                                    Item {
                                        visible: !root.renameEditing
                                        Layout.preferredWidth: 20
                                        Layout.preferredHeight: 20
                                        Image {
                                            anchors.centerIn: parent
                                            width: 13; height: 13
                                            source: "qrc:/qml/assets/icons/rename_edit.svg"
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                root.renameDraft = root.editorVm ? root.editorVm.projectName : ""
                                                root.renameEditing = true
                                                renameInput.forceActiveFocus()
                                                renameInput.selectAll()
                                            }
                                        }
                                    }
                                    // 重命名输入 360x24（:241-252，Body_13 = fontSize13）
                                    TextField {
                                        id: renameInput
                                        visible: root.renameEditing
                                        Layout.preferredWidth: 360
                                        Layout.preferredHeight: 24
                                        text: root.renameDraft
                                        font.pixelSize: Theme.fontSize13
                                        color: Theme.textPrimary
                                        selectByMouse: true
                                        background: Rectangle {
                                            color: Theme.bgInset
                                            border.color: renameInput.activeFocus
                                                          ? Theme.borderFocus : Theme.borderInput
                                            border.width: 1
                                            radius: Theme.radiusSM
                                        }
                                        onAccepted: root.commitRename()
                                        onActiveFocusChanged: {
                                            if (!activeFocus && root.renameEditing)
                                                root.commitRename()
                                        }
                                    }
                                    Item { Layout.fillWidth: true }

                                    // 多板翻页钮 25x25（:301-308 构造隐藏，多板显示）
                                    CxIconButton {
                                        visible: root.editorVm !== null && root.editorVm.plateCount > 1
                                        enabled: root.editorVm !== null
                                                 && root.editorVm.currentPlateIndex > 0
                                        iconSource: "qrc:/qml/assets/icons/go_last_plate.svg"
                                        buttonSize: 25
                                        iconSize: 25
                                        onClicked: root.editorVm.setCurrentPlateIndex(
                                                       root.editorVm.currentPlateIndex - 1)
                                    }
                                    CxIconButton {
                                        visible: root.editorVm !== null && root.editorVm.plateCount > 1
                                        enabled: root.editorVm !== null
                                                 && root.editorVm.currentPlateIndex < root.editorVm.plateCount - 1
                                        iconSource: "qrc:/qml/assets/icons/go_next_plate.svg"
                                        buttonSize: 25
                                        iconSize: 25
                                        onClicked: root.editorVm.setCurrentPlateIndex(
                                                       root.editorVm.currentPlateIndex + 1)
                                    }
                                }
                            }
                            Item { Layout.fillWidth: true; Layout.preferredHeight: 5 }  // :345 wxTOP 5

                            // 预计时间/重量行（:281-297；18 图标 + Body_13，间隔 30）
                            RowLayout {
                                spacing: 0
                                Image {
                                    Layout.preferredWidth: 18
                                    Layout.preferredHeight: 18
                                    source: "qrc:/qml/assets/icons/print-time.svg"
                                }
                                Text {
                                    Layout.leftMargin: 6  // :294 wxLEFT 6
                                    text: root.editorVm
                                          ? (root.editorVm.sliceEstimatedTime !== ""
                                             ? root.editorVm.sliceEstimatedTime
                                             : root.editorVm.estimatedPrintTime) : ""
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSize13  // Body_13
                                }
                                Item { Layout.preferredWidth: 30; Layout.preferredHeight: 1 }  // :295
                                Image {
                                    Layout.preferredWidth: 18
                                    Layout.preferredHeight: 18
                                    source: "qrc:/qml/assets/icons/print-weight.svg"
                                }
                                Text {
                                    Layout.leftMargin: 6  // :297 wxLEFT 6
                                    text: root.editorVm ? root.editorVm.sliceResultWeight : ""
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSize13
                                }
                            }
                            Item { Layout.fillWidth: true; Layout.preferredHeight: 10 }  // :347 wxTOP 10

                            // ── PrinterInfoBox（:6993-7084）──────────────────
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                // 标题行: [Printer + 问号] + 分割线（:6996-7015）
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 5  // :7013
                                    Text {
                                        text: qsTr("打印机")
                                        color: Theme.textTertiary  // :6999 0x909090
                                        font.pixelSize: Theme.fontSize13  // Head_13
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    CxIconButton {
                                        iconSource: "qrc:/qml/assets/icons/icon_qusetion.svg"
                                        buttonSize: 18
                                        iconSize: 18
                                        toolTipText: qsTr("无法连接打印机时请点击此处")  // :7003
                                        onClicked: root.showPrinterHelp()
                                    }
                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 1
                                        color: Theme.borderSubtle  // :7008 0xeeeeee
                                    }
                                }
                                Item { Layout.fillWidth: true; Layout.preferredHeight: 8 }  // :7081 wxTOP 8

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 4  // :7073

                                    // printer_staticbox 338x68 边框 #CECECE（:7022-7025）
                                    Rectangle {
                                        Layout.preferredWidth: 338
                                        Layout.preferredHeight: 68
                                        radius: Theme.radiusMD
                                        color: "transparent"
                                        border.width: 1
                                        border.color: Theme.borderDefault  // #CECECE → 中性描边

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 7  // :7037 wxLEFT 7
                                            anchors.rightMargin: 7
                                            spacing: 4

                                            // 只读 ComboBox 300x60（:7027-7031，
                                            // 文本区 52px 预览图, 下拉项 32px, :6946-6952）
                                            Button {
                                                id: printerCombo
                                                Layout.preferredWidth: 300
                                                Layout.preferredHeight: 60
                                                enabled: root.deviceVm
                                                         && root.deviceVm.filteredDeviceCount > 0

                                                background: Rectangle {
                                                    radius: Theme.radiusMD
                                                    color: printerCombo.hovered
                                                           ? Theme.bgHover : Theme.bgInset
                                                    border.width: 1
                                                    border.color: printerCombo.hovered
                                                                  ? Theme.borderFocus : Theme.borderInput
                                                }
                                                contentItem: RowLayout {
                                                    spacing: Theme.spacingSM
                                                    Image {
                                                        Layout.preferredWidth: 52
                                                        Layout.preferredHeight: 52
                                                        source: "qrc:/qml/assets/icons/printer_preview_BL-P001.png"
                                                        fillMode: Image.PreserveAspectFit
                                                    }
                                                    ColumnLayout {
                                                        Layout.fillWidth: true
                                                        spacing: 0
                                                        Text {
                                                            Layout.fillWidth: true
                                                            text: root.comboDevice
                                                                  ? (root.comboDevice.name || root.comboDevice.model || qsTr("未知设备"))
                                                                  : qsTr("未选择打印机")
                                                            color: Theme.textPrimary
                                                            font.pixelSize: Theme.fontSize13
                                                            elide: Text.ElideRight
                                                        }
                                                        Text {
                                                            Layout.fillWidth: true
                                                            visible: root.comboDevice !== null
                                                            text: root.comboDevice
                                                                  ? (root.comboDevice.online ? qsTr("在线") : qsTr("离线"))
                                                                  : ""
                                                            color: root.comboDevice && root.comboDevice.online
                                                                   ? Theme.statusSuccess : Theme.textTertiary
                                                            font.pixelSize: Theme.fontSizeXS
                                                        }
                                                    }
                                                    Text {
                                                        text: "▾"
                                                        color: Theme.textMuted
                                                        font.pixelSize: Theme.fontSizeMD
                                                    }
                                                }
                                                onClicked: comboPopup.open()

                                                // 下拉弹层（DropDown::Item = 32px 图标 + 名称）
                                                Popup {
                                                    id: comboPopup
                                                    y: parent.height + 2
                                                    width: parent.width
                                                    height: Math.min(
                                                                root.deviceVm ? root.deviceVm.filteredDeviceCount * 40 : 0,
                                                                240) + 8
                                                    padding: 4
                                                    background: Rectangle {
                                                        color: Theme.bgPanel
                                                        border.color: Theme.borderDefault
                                                        border.width: 1
                                                        radius: Theme.radiusMD
                                                    }
                                                    contentItem: Flickable {
                                                        clip: true
                                                        contentHeight: comboCol.height
                                                        Column {
                                                            id: comboCol
                                                            width: comboPopup.availableWidth
                                                            spacing: 2
                                                            Repeater {
                                                                model: root.deviceVm ? root.deviceVm.filteredDeviceCount : 0
                                                                delegate: ItemDelegate {
                                                                    id: comboEntry
                                                                    required property int index
                                                                    width: comboPopup.availableWidth
                                                                    height: 36
                                                                    readonly property var dev: root.deviceVm
                                                                                               ? root.deviceVm.deviceAt(index) : null
                                                                    readonly property string entryName: dev ? (dev.name || dev.model || qsTr("未知设备")) : ""
                                                                    background: Rectangle {
                                                                        radius: Theme.radiusSM
                                                                        color: comboEntry.index === root.comboIndex
                                                                               ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.25)
                                                                               : comboEntry.hovered
                                                                                 ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.1)
                                                                                 : "transparent"
                                                                    }
                                                                    contentItem: RowLayout {
                                                                        spacing: Theme.spacingSM
                                                                        Image {
                                                                            Layout.preferredWidth: 32
                                                                            Layout.preferredHeight: 32
                                                                            source: "qrc:/qml/assets/icons/printer_preview_BL-P001.png"
                                                                            fillMode: Image.PreserveAspectFit
                                                                        }
                                                                        Text {
                                                                            Layout.fillWidth: true
                                                                            text: comboEntry.entryName
                                                                            color: Theme.textPrimary
                                                                            font.pixelSize: Theme.fontSize13
                                                                            elide: Text.ElideRight
                                                                        }
                                                                    }
                                                                    onClicked: {
                                                                        root.selectDeviceIndex(comboEntry.index)
                                                                        comboPopup.close()
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }

                                            // 刷新按钮（:7034-7035 refresh_printer）
                                            CxIconButton {
                                                iconSource: "qrc:/qml/assets/icons/refresh_printer.svg"
                                                buttonSize: 28
                                                iconSize: 18
                                                toolTipText: qsTr("刷新设备列表")
                                                enabled: root.deviceVm !== null
                                                onClicked: root.deviceVm.scanDevices()
                                            }
                                        }
                                    }

                                    // bed_staticbox 68x68 边框 #EEEEEE + bed_cool 40x40
                                    //（:7046-7054）
                                    Rectangle {
                                        Layout.preferredWidth: 68
                                        Layout.preferredHeight: 68
                                        radius: Theme.radiusMD
                                        color: "transparent"
                                        border.width: 1
                                        border.color: Theme.borderSubtle  // #EEEEEE → 中性描边
                                        Image {
                                            anchors.centerIn: parent
                                            width: 40; height: 40
                                            source: "qrc:/qml/assets/icons/bed_cool.png"
                                        }
                                    }
                                }

                                // m_text_printer_msg 420 宽消息板（:318-323，构造隐藏）
                                Text {
                                    visible: false
                                    Layout.preferredWidth: 420
                                    text: ""
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSize13
                                    wrapMode: Text.Wrap
                                }
                                Item { Layout.fillWidth: true; Layout.preferredHeight: 10 }  // :351
                                // m_text_printer_msg_tips 420x24（:325-330，构造隐藏）
                                Text {
                                    visible: false
                                    Layout.preferredWidth: 420
                                    Layout.preferredHeight: 24
                                    text: ""
                                    color: Theme.textSecondary  // :329 0x6B6B6B
                                    font.pixelSize: Theme.fontSize13
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                    Item { Layout.fillWidth: true; Layout.preferredHeight: 14 }  // :791 wxTOP 14

                    // ── Filament 分节（:364-417，左右边距 15）────────────────
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 15
                        Layout.rightMargin: 15
                        spacing: 8
                        Text {
                            text: qsTr("耗材")  // :367 _L("Filament")
                            color: Theme.textTertiary  // :369 0x909090
                            font.pixelSize: Theme.fontSize13  // Head_13
                            verticalAlignment: Text.AlignVCenter
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: Theme.borderSubtle  // :372 0xeeeeee
                        }
                        // Auto Refill（:377-389，构造隐藏，AMS 备料可用时显示）
                        Row {
                            visible: false
                            spacing: 3  // :385 wxALL 3
                            Image {
                                width: 16; height: 16
                                anchors.verticalCenter: parent.verticalCenter
                                source: "qrc:/qml/assets/icons/automatic_material_renewal.svg"
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: qsTr("自动续料")  // :377 _L("Auto Refill")
                                color: Theme.accent  // :379 #009688 → 品牌绿
                                font.pixelSize: Theme.fontSize13
                                anchors.verticalCenterOffset: 2  // :386 wxTOP 5 微调
                            }
                        }
                    }
                    Item { Layout.fillWidth: true; Layout.preferredHeight: 6 }  // :795 wxTOP 6

                    // 耗材映射面板 637 宽 #F8F8F8（:406-417）→ 中性灰内嵌面板；
                    // 有 AMS 槽位的设备显示（上游按设备能力 Show/Hide, :465）
                    Rectangle {
                        visible: root.deviceVm && root.comboDevice !== null
                                 && root.deviceVm.selectedAmsSlotCount > 0
                        Layout.preferredWidth: 637  // :409-410 Min/Max 637 宽
                        Layout.alignment: Qt.AlignHCenter
                        Layout.leftMargin: 15
                        Layout.rightMargin: 15
                        implicitHeight: mappingGrid.visible
                                        ? mappingGrid.implicitHeight + 20 : 0  // :414 wxALL 10
                        color: Theme.bgInset  // #F8F8F8 → 中性灰令牌
                        radius: Theme.radiusMD

                        GridLayout {
                            id: mappingGrid
                            visible: root.deviceVm !== null
                            anchors.fill: parent
                            anchors.margins: 10  // :414 wxALL FromDIP(10)
                            columns: 10          // :413 wxGridSizer(0, 10, 7, 7)
                            columnSpacing: 7
                            rowSpacing: 7

                            Repeater {
                                model: root.deviceVm ? root.deviceVm.selectedAmsSlotCount : 0
                                // MaterialItem 单元（AmsMappingPopup.cpp:25 65x50，
                                // 色轮 + 材质名）→ 简化为色块 + 类型 + 槽位
                                delegate: Rectangle {
                                    id: matCell
                                    required property int index
                                    readonly property var slot: root.deviceVm
                                                                ? root.deviceVm.amsSlotAt(index) : null
                                    Layout.preferredWidth: 65
                                    Layout.preferredHeight: 50
                                    radius: Theme.radiusSM
                                    color: Theme.bgPanel
                                    border.width: 1
                                    border.color: matCell.slot && matCell.slot.active
                                                  ? Theme.accent : Theme.borderSubtle

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 4
                                        spacing: 2
                                        Rectangle {
                                            Layout.preferredWidth: 17
                                            Layout.preferredHeight: 16
                                            radius: 2
                                            color: matCell.slot && matCell.slot.color && matCell.slot.color !== ""
                                                   ? matCell.slot.color : Theme.borderSubtle
                                            border.width: 1
                                            border.color: Theme.borderSubtle
                                        }
                                        Text {
                                            Layout.fillWidth: true
                                            text: matCell.slot && matCell.slot.filamentType && matCell.slot.filamentType !== ""
                                                  ? matCell.slot.filamentType : qsTr("空")
                                            color: matCell.slot && matCell.slot.filamentType && matCell.slot.filamentType !== ""
                                                   ? Theme.textPrimary : Theme.textTertiary
                                            font.pixelSize: Theme.fontSizeXS
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            Layout.fillWidth: true
                                            text: qsTr("槽位 %1").arg(matCell.index + 1)
                                            color: Theme.textTertiary
                                            font.pixelSize: Theme.fontSizeXS
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // m_statictext_ams_msg 655 宽（:467-472，构造隐藏）
                    Text {
                        visible: false
                        Layout.preferredWidth: 655
                        Layout.leftMargin: 15
                        text: ""
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSize13
                        wrapMode: Text.Wrap
                    }
                    Item { Layout.fillWidth: true; Layout.preferredHeight: 10 }  // :797 wxTOP 10

                    // AMS 映射建议 580 宽（:520-530，色 0xFF6F00 → 状态警示色）
                    Text {
                        visible: false
                        Layout.preferredWidth: 580
                        Layout.leftMargin: 15
                        text: qsTr("切片文件中的耗材分组方式不是最优方案。")
                        color: Theme.statusWarning
                        font.pixelSize: Theme.fontSize13
                        wrapMode: Text.Wrap
                    }
                    // 换料次数提示 580 宽（:532-542，同警示色）
                    Text {
                        visible: false
                        Layout.preferredWidth: 580
                        Layout.leftMargin: 15
                        text: ""
                        color: Theme.statusWarning
                        font.pixelSize: Theme.fontSize13
                        wrapMode: Text.Wrap
                    }
                    // 建议行: 重分组链接 + 外置进料复选框（:476-518，构造隐藏）
                    RowLayout {
                        visible: false
                        Layout.fillWidth: true
                        Layout.leftMargin: 15
                        Layout.rightMargin: 15
                        spacing: 10
                        Text {
                            text: qsTr("对耗材分组不满意？重新分组并切片 ->")  // :482
                            color: Theme.accent  // :478 #009688 → 品牌绿
                            font.pixelSize: Theme.fontSize13
                        }
                        Item { Layout.fillWidth: true }
                        CxCheckBox { id: extAssistCheck; checked: false }  // :501-505
                        Text {
                            Layout.maximumWidth: 200  // :509
                            text: qsTr("多色打印使用外置进料")  // :506
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSize13
                            wrapMode: Text.Wrap
                            ToolTip.visible: extAssistHover.hovered
                            ToolTip.text: qsTr("多色打印时打印过程中手动更换外置耗材")  // :504/:512
                            HoverHandler { id: extAssistHover }
                        }
                    }
                    Item { Layout.fillWidth: true; Layout.preferredHeight: 10 }  // :802 wxTOP 10
                    // 烘干降温警告（:544-550，色 #F09A17 → 状态警示色）
                    Text {
                        visible: false
                        Layout.leftMargin: 15
                        Layout.topMargin: 2
                        text: qsTr("为保证打印质量，打印过程中将降低烘干温度。")
                        color: Theme.statusWarning
                        font.pixelSize: Theme.fontSize13
                        wrapMode: Text.Wrap
                    }

                    // ── 高级选项分割线（:553-560，左右边距 15）──────────────
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.leftMargin: 15
                        Layout.rightMargin: 15
                        Layout.preferredHeight: 1
                        color: Theme.borderSubtle  // :555 0xEEEEEE
                    }
                    Item { Layout.fillWidth: true; Layout.preferredHeight: 10 }  // :805 wxTOP 10

                    // ── 高级选项 2 列网格（:616-628, vgap 5 hgap 10）─────────
                    GridLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 15
                        Layout.rightMargin: 15
                        columns: 2
                        columnSpacing: 10
                        rowSpacing: 5

                        Repeater {
                            model: root.printOptionDefs
                            delegate: RowLayout {
                                id: optRow
                                required property int index
                                required property var modelData
                                readonly property string optKey: optRow.modelData.key
                                readonly property var optOps: optRow.modelData.ops
                                // 读 printOptions map 属性（NOTIFY printOptionsChanged）
                                // 使分段选择器点击后重新求值。
                                readonly property string optValue: {
                                    const _opts = root.deviceVm ? root.deviceVm.printOptions : null
                                    return _opts && _opts[optRow.optKey] !== undefined
                                           ? String(_opts[optRow.optKey]) : ""
                                }
                                readonly property bool optSupported: root.deviceVm
                                                                     ? root.deviceVm.printOptionSupported(optRow.optKey)
                                                                     : false
                                visible: optSupported
                                Layout.fillWidth: true
                                Layout.preferredHeight: 28  // :6416-6417 PrintOption 高 28
                                spacing: 2  // :6439 wxLEFT 2

                                Text {
                                    text: optRow.modelData.title
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSize13  // Body_13
                                    verticalAlignment: Text.AlignVCenter
                                }
                                CxIconButton {
                                    iconSource: "qrc:/qml/assets/icons/icon_qusetion.svg"
                                    buttonSize: 18
                                    iconSize: 18
                                    toolTipText: optRow.modelData.tip  // :6447 update_tooltip
                                }
                                Item { Layout.fillWidth: true }

                                // PrintOptionItem 分段选择器（hpp:154 段宽 44，
                                // hpp:170-173 总宽 = n*44+8, 总高 22+8 = 30）
                                Row {
                                    spacing: 0
                                    Layout.preferredHeight: 30
                                    Repeater {
                                        model: optRow.optOps
                                        delegate: Rectangle {
                                            id: seg
                                            required property int index
                                            required property string modelData
                                            width: 44
                                            height: 30
                                            radius: index === 0 ? Theme.radiusSM : 0
                                            color: optRow.optValue === seg.modelData
                                                   ? Theme.accentSubtle : Theme.bgInset
                                            border.width: 1
                                            border.color: Theme.borderSubtle
                                            Text {
                                                anchors.centerIn: parent
                                                text: seg.modelData === "auto" ? qsTr("自动")
                                                      : seg.modelData === "on" ? qsTr("开") : qsTr("关")
                                                color: optRow.optValue === seg.modelData
                                                       ? Theme.textOnAccent : Theme.textSecondary
                                                font.pixelSize: Theme.fontSize13  // Body_13, :6436
                                            }
                                            MouseArea {
                                                anchors.fill: parent
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    if (root.deviceVm)
                                                        root.deviceVm.setPrintOptionValue(optRow.optKey, seg.modelData)
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Item { Layout.fillWidth: true; Layout.preferredHeight: 30 }  // :807 wxTOP 30

                    // R-P1.E: user-visible disclosure required by
                    // docs/依赖与协议边界审计.md -- the device stack is mock; real
                    // MQTT push to hardware is externally blocked.
                    // (tests/QmlUiAuditTests.cpp:11689 locks this disclosure.)
                    Text {
                        Layout.fillWidth: true
                        Layout.leftMargin: 15
                        Layout.rightMargin: 15
                        text: qsTr("演示模式：设备列表为本地模拟数据，发送为演示流程（真实设备推送依赖 MQTT，当前为外部阻塞项）。")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                        wrapMode: Text.Wrap
                    }
                }
            }

            // ── 底部三态条 m_simplebook 645x32（:651-705, hpp:115）────────
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                Layout.topMargin: 10  // :816 wxTOP 10

                // prepare 页：居中 Send 确认按钮（:661-666）
                RowLayout {
                    anchors.fill: parent
                    visible: root.sendState === 0
                    spacing: 0
                    Item { Layout.fillWidth: true }
                    CxButton {
                        // upstream Button _L("Send") ButtonStyle::Confirm
                        text: qsTr("发送")
                        cxStyle: CxButton.Style.Primary
                        enabled: root.canSend
                        onClicked: {
                            if (!root.canSend)
                                return
                            root.selectDeviceIndex(root.comboIndex)
                            root.deviceVm.startSendJob(root.comboIndex, root.gcodePath)
                        }
                    }
                    Item { Layout.fillWidth: true }
                }

                // sending 页：BBLStatusBarPrint 进度条（:676-678）
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15
                    visible: root.sendState === 1
                    spacing: Theme.spacingMD
                    Text {
                        text: qsTr("正在发送打印任务…")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSize13
                    }
                    CxProgressBar {
                        Layout.fillWidth: true
                        from: 0
                        to: 100
                        value: root.deviceVm ? root.deviceVm.sendJobProgress : 0
                        barHeight: 8
                    }
                    Text {
                        text: (root.deviceVm ? root.deviceVm.sendJobProgress : 0) + "%"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSize13
                    }
                    CxButton {
                        text: qsTr("取消发送")
                        cxStyle: CxButton.Style.Ghost
                        compact: true
                        onClicked: {
                            if (root.deviceVm)
                                root.deviceVm.cancelSendJob()
                        }
                    }
                }

                // finish 页："Send complete"（:683-705；背景 135,206,250 与文字
                // 0,150,136 为上游亮色主题 → OWzx 中性面板底 + 品牌绿前景）
                Rectangle {
                    anchors.fill: parent
                    visible: root.sendState === 2
                    color: Theme.bgInset
                    radius: Theme.radiusSM
                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 5  // :691 wxALL 5
                        Image {
                            Layout.preferredWidth: 25
                            Layout.preferredHeight: 25  // :690 completed 25x25
                            source: "qrc:/qml/assets/icons/completed.svg"
                        }
                        Text {
                            text: qsTr("发送完成")  // :693 _L("Send complete")
                            color: Theme.accent  // :695 (0,150,136) → 品牌绿
                            font.pixelSize: Theme.fontSize13
                        }
                    }
                }
            }

            // ── 发送失败信息区 645x125（:707-783，默认隐藏）──────────────
            Rectangle {
                id: failInfo
                visible: root.deviceVm !== null && root.deviceVm.sendJobErrorCode !== ""
                Layout.fillWidth: true
                Layout.preferredWidth: 645
                Layout.preferredHeight: 125
                Layout.topMargin: 18  // :818 wxTOP 18
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: 645
                color: "transparent"
                clip: true

                Flickable {
                    anchors.fill: parent
                    contentWidth: width
                    contentHeight: failCol.height
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true

                    ColumnLayout {
                        id: failCol
                        width: failInfo.width
                        spacing: 3  // :780/:782 wxTOP 3

                        // m_link_network_state（:775-778, Body_12）
                        Text {
                            Layout.leftMargin: 5
                            text: qsTr("检查当前系统服务状态")
                            color: Theme.accent
                            font.pixelSize: Theme.fontSizeSM  // Body_12
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: Qt.openUrlExternally("https://www.orcaslicer.com/wiki/")
                            }
                        }
                        // 三行: Error code / Error desc / Extra info
                        //（:722-772, 标题列 74 / 正文 500 / 0x909090）
                        RowLayout {
                            Layout.leftMargin: 5
                            spacing: 0
                            Text {
                                Layout.preferredWidth: 74
                                text: qsTr("错误代码") + ": "
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSize13
                            }
                            Text {
                                Layout.maximumWidth: 500
                                text: root.deviceVm ? root.deviceVm.sendJobErrorCode : ""
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSize13
                                wrapMode: Text.Wrap
                            }
                        }
                        RowLayout {
                            Layout.leftMargin: 5
                            spacing: 0
                            Text {
                                Layout.preferredWidth: 74
                                text: qsTr("错误描述") + ": "
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSize13
                            }
                            Text {
                                Layout.maximumWidth: 500
                                text: root.deviceVm ? root.deviceVm.sendJobErrorDesc : ""
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSize13
                                wrapMode: Text.Wrap
                            }
                        }
                        RowLayout {
                            Layout.leftMargin: 5
                            spacing: 0
                            Text {
                                Layout.preferredWidth: 74
                                text: qsTr("附加信息") + ": "
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSize13
                            }
                            Text {
                                Layout.maximumWidth: 500
                                text: root.deviceVm ? root.deviceVm.sendJobErrorExtra : ""
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSize13
                                wrapMode: Text.Wrap
                            }
                        }
                    }
                }
            }
        }
    }

    // ── 对话框行为 helper ─────────────────────────────────────────────

    // 选中设备并同步全局选中（ComboBox wxEVT_COMBOBOX → on_selection_changed）
    function selectDeviceIndex(filteredIndex) {
        comboIndex = filteredIndex
        if (deviceVm && filteredIndex >= 0)
            deviceVm.selectDevice(filteredIndex)
    }

    // 重命名提交（on_rename_enter → Plater 板名, :246-251）
    function commitRename() {
        renameEditing = false
        const name = renameInput.text.trim()
        if (!editorVm || name === "" || name === editorVm.projectName)
            return
        editorVm.renamePlate(editorVm.currentPlateIndex, name)
    }

    // PrinterInfoBox 问号（:7002 OnBtnQuestionClicked → wiki）
    function showPrinterHelp() {
        Qt.openUrlExternally("https://www.orcaslicer.com/wiki/")
    }
}
