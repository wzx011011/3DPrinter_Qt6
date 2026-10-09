import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// D2 -- PrintDialog: send print (upstream SendToPrinterDialog, SendToPrinter.cpp)
// Usage: PrintDialog { id: printDlg; editorVm: ...; monitorVm: ... }
// Trigger: printDlg.open()  (PreparePage.openPrintDialog)
//
// Structure mirrors upstream SendToPrinter.cpp:214-592 1:1 (sizes, spacing,
// row order): 256x256 thumbnail -> time/weight row -> 420x25 rename
// simplebook -> divider -> printer combo row (30 DIP margins) -> storage
// radios -> status message -> connecting row -> divider -> 370x64 state
// simplebook (prepare / sending / finish) -> 380x125 failed-info panel.
// Colors stay on the OWzx token set (Theme.accent green + neutral grey
// surfaces); the upstream teal/white surfaces are mapped to tokens, not
// copied.
CxDialog {
    id: root
    required property var editorVm
    property var monitorVm: null  // v2.5 DEV-05: device list / send path
    // G-03: true while an on-demand slice requested from this dialog is in
    // flight; printSliceReady() then refreshes the in-dialog slice info.
    property bool slicingForPrint: false

    // Upstream caption _L("Send to Printer storage") (SendToPrinter.cpp:215),
    // zh_CN po:10078-10079 -> "发送到打印机存储". The wxCAPTION frame carries
    // no caption icon, so titleIcon stays empty.
    dialogTitle: qsTr("发送到打印机存储")

    width: 430
    padding: 0
    height: contentCol.implicitHeight + 80

    // -- Dialog state machine (mirrors upstream m_simplebook pages) --
    // 0 = prepare (Send button), 1 = sending (status bar), 2 = finish.
    property int sendState: 0
    property real sendProgress: 0  // sending page gauge, 0-100
    // Printer combo model: online devices only (upstream update_user_printer
    // SendToPrinter.cpp:1087-1166 filters is_online/is_connected).
    property var printerEntries: []   // [{filteredIndex, name, status}]
    property var printerNames: []
    // Connecting feedback (upstream m_connecting_panel "Try to connect",
    // SendToPrinter.cpp:308-326 + PrintStatusConnecting/Reconnecting).
    property bool connecting: false
    property bool connectFailed: false
    // Storage radios (upstream update_storage_list SendToPrinter.cpp:607-667).
    property var storageEntries: []   // [{key, enabled}]
    property string selectedStorageKey: ""
    property string _storageSig: ""   // re-default the selection only when the list changes
    // Rename (upstream m_rename_switch_panel, m_current_project_name is
    // dialog-local state initialized from the export file name and used as
    // the send-job display name, SendToPrinter.cpp:476-544,1527-1550).
    property string sendName: ""
    property bool renaming: false
    // Failed info panel (upstream m_sw_print_failed_info, hidden by default
    // SendToPrinter.cpp:385-458,581).
    property bool sendFailed: false
    property string failedCode: ""
    property string failedDesc: ""
    property string failedExtra: ""
    // Status line under the storage row (upstream m_statictext_printer_msg,
    // hidden by default, warning colour when is_warning SendToPrinter.cpp:739-781).
    property string statusMsg: ""
    property bool statusWarning: false
    // Thumbnail re-eval tick (plateThumbnailBase64 is an invokable, so the
    // binding needs a dependency that changes when a new slice lands).
    property int thumbTick: 0

    function showStatus(msg, warning) {
      statusMsg = msg
      statusWarning = warning === true
    }

    // Current-plate thumbnail as a data URL; re-evaluated when thumbTick
    // changes (onOpened / printSliceReady).
    readonly property string thumbSource: {
      if (root.thumbTick < 0 || !root.editorVm)
        return ""
      const data = root.editorVm.plateThumbnailBase64(root.editorVm.currentPlateIndex)
      if (!data || data.length === 0)
        return ""
      return data.indexOf("data:image/") === 0 ? data : "data:image/png;base64," + data
    }

    // Upstream set_default filters "<>[]:/\|?*\"" from the file name
    // (SendToPrinter.cpp:1547).
    function filterNameChars(name) {
      return name.replace(/[<>\[\]:\/\\|?*"]/g, "")
    }

    function rebuildPrinterModel() {
      const entries = []
      if (root.monitorVm) {
        const n = root.monitorVm.filteredDeviceCount
        for (let i = 0; i < n; i++) {
          const dev = root.monitorVm.deviceAt(i)
          if (dev && dev.online === true)
            entries.push({
              filteredIndex: i,
              name: dev.name || dev.model || qsTr("未知设备"),
              status: dev.status || ""
            })
        }
      }
      const names = []
      for (let j = 0; j < entries.length; j++)
        names.push(entries[j].name)
      printerEntries = entries
      printerNames = names
      if (printerCombo.currentIndex >= entries.length)
        printerCombo.currentIndex = -1
    }

    function rebuildStorages() {
      // Upstream reads the per-device storage list on selection/show
      // (SendToPrinter.cpp:607-667); no reachable device -> empty row.
      if (!root.monitorVm || root.printerEntries.length === 0) {
        storageEntries = []
        selectedStorageKey = ""
        _storageSig = ""
        return
      }
      const entries = root.monitorVm.selectedDeviceStorages()
      const sig = JSON.stringify(entries)
      const changed = sig !== _storageSig
      _storageSig = sig
      storageEntries = entries
      // First option preselected when the list changed; an existing valid
      // selection survives telemetry ticks (m_selected_storage defaulting,
      // SendToPrinter.cpp:655-661).
      let stillValid = false
      for (let i = 0; i < entries.length; i++) {
        if (entries[i].key === selectedStorageKey) {
          stillValid = true
          break
        }
      }
      if (changed || !stillValid)
        selectedStorageKey = entries.length > 0 ? entries[0].key : ""
    }

    // Upstream on_selection_changed (SendToPrinter.cpp:1179-1219): selecting
    // a printer queries it and shows the connecting feedback until the
    // device answers.
    function onPrinterActivated(index) {
      if (index < 0 || index >= printerEntries.length)
        return
      connectFailed = false
      connecting = true
      showStatus("", false)
      connectFallbackTimer.restart()
      root.monitorVm.connectDevice(printerEntries[index].filteredIndex)
    }

    function finishConnect(ok) {
      connecting = false
      connectFailed = !ok
      connectFallbackTimer.stop()
      rebuildStorages()
      if (ok)
        showStatus("", false)
      else
        showStatus(qsTr("连接失败，点击刷新图标重试。"), true)
    }

    // Upstream on_rename_enter validation (SendToPrinter.cpp:135-212);
    // on failure the message surfaces on the status line and the edit reverts
    // to the previous name on page 0 (upstream MessageDialog -> SetSelection(0)).
    function commitRename() {
      const name = renameInput.text
      let invalidMsg = ""
      if (name.length === 0)
        invalidMsg = qsTr("文件名不能为空。")
      else if (name.charAt(0) === " ")
        invalidMsg = qsTr("文件名不能以空格开头。")
      else if (name.charAt(name.length - 1) === " ")
        invalidMsg = qsTr("文件名不能以空格结尾。")
      else if (name.length >= 100)
        invalidMsg = qsTr("文件名长度超出限制。")
      if (invalidMsg !== "") {
        showStatus(invalidMsg, true)
        cancelRename()
        return
      }
      sendName = name
      renaming = false
      if (statusWarning)
        showStatus("", false)
    }

    function cancelRename() {
      renameInput.text = sendName
      renaming = false
    }

    readonly property bool hasSlicePath: root.editorVm !== null
        && ((root.editorVm.lastGcodePath || "") !== "")
    readonly property bool printerSelected: printerCombo.currentIndex >= 0
        && printerCombo.currentIndex < printerEntries.length
    // Send gating mirrors upstream show_status / Enable_Send_Button: disabled
    // while connecting/sending and when the target is not printable.
    readonly property bool devicePrintable: root.monitorVm !== null
        && root.monitorVm.selectedDeviceOnline
        && root.monitorVm.selectedDeviceStatus !== "printing"
    readonly property bool canSend: root.editorVm !== null
        && root.monitorVm !== null
        && sendState === 0
        && !connecting
        && !slicingForPrint
        && printerSelected
        && devicePrintable
        && hasSlicePath

    // Upstream on_ok (SendToPrinter.cpp:809-835): with an empty machine list
    // upstream silently no-ops; here the two-stage device-list dialog stays
    // reachable on that path because tests/QmlUiAuditTests.cpp:10412,:10434
    // lock the "selectMachineDialog.open()" trigger + instantiation tokens to
    // live outside SelectMachineDialog.qml (and the audit table cannot be
    // edited within this task's file scope).
    function startSend() {
      if (root.editorVm === null || root.monitorVm === null)
        return
      if (sendState !== 0 || connecting || slicingForPrint)
        return
      if (!printerSelected) {
        selectMachineDialog.gcodePath = root.editorVm.lastGcodePath || ""
        selectMachineDialog.open()
        root.close()
        return
      }
      // G-03: no slice result yet -> slice on demand, the flow continues in
      // this dialog when printSliceReady fires (upstream slices from the
      // print flow, Plater.cpp:13006-13017).
      if (!hasSlicePath) {
        slicingForPrint = true
        showStatus(qsTr("正在切片，完成后即可发送…"), false)
        root.editorVm.requestSlice()
        return
      }
      if (!devicePrintable) {
        showStatus(root.monitorVm.selectedDeviceStatus === "printing"
                   ? qsTr("打印机正在执行任务，无法接收新打印。")
                   : qsTr("所选打印机离线，无法发送。"), true)
        return
      }
      // Demo send over the mock device stack: the job lands through
      // MonitorViewModel::startPrint (DeviceServiceMock), then the sending
      // page animates the upload and settles on the finish page.
      root.monitorVm.startPrint(printerEntries[printerCombo.currentIndex].filteredIndex,
                                root.editorVm.lastGcodePath)
      sendFailed = false
      sendProgress = 0
      sendState = 1
      sendAnim.restart()
    }

    // Upstream EVT_PRINT_JOB_CANCEL -> on_print_job_cancel -> prepare_mode
    // (SendToPrinter.cpp:1055-1085).
    function cancelSend() {
      sendAnim.stop()
      sendProgress = 0
      sendState = 0
    }

    onOpened: {
      // Upstream Show() -> update_storage_list / set_default /
      // update_user_machine_list / Reset (SendToPrinter.cpp:1637-1658).
      sendState = 0
      sendProgress = 0
      sendAnim.stop()
      sendFailed = false
      failedCode = ""
      failedDesc = ""
      failedExtra = ""
      connecting = false
      connectFailed = false
      showStatus("", false)
      renaming = false
      sendName = filterNameChars(root.editorVm && root.editorVm.projectName
                                 ? root.editorVm.projectName : qsTr("未命名"))
      thumbTick++
      rebuildPrinterModel()
      rebuildStorages()
    }

    contentItem: ColumnLayout {
        id: contentCol
        width: root.width
        spacing: 0

        // Top gap (upstream m_sizer_main Add wxTOP 10, SendToPrinter.cpp:562)
        Item { Layout.preferredHeight: 10 }

        // -- Plate thumbnail 256x256, centered
        // (m_panel_image + m_thumbnailPanel, SendToPrinter.cpp:240-250,470) --
        Item {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 256
            Layout.preferredHeight: 256
            Image {
                anchors.fill: parent
                source: root.thumbSource
                fillMode: Image.PreserveAspectFit
                smooth: true
                visible: root.thumbSource !== ""
            }
        }

        // Gap after the thumbnail (SendToPrinter.cpp:471)
        Item { Layout.preferredHeight: 10 }

        // -- Print time / weight row from the real slice result
        // (m_sizer_basic, SendToPrinter.cpp:252-267,472; values at :1619-1634) --
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 0
            Item { Layout.preferredWidth: 5 }
            Image {
                Layout.preferredWidth: 18; Layout.preferredHeight: 18
                Layout.margins: Theme.spacingXS
                source: "qrc:/qml/assets/icons/print-time.svg"
                sourceSize.width: 18; sourceSize.height: 18
            }
            Text {
                Layout.preferredWidth: 72
                Layout.margins: Theme.spacingXS
                horizontalAlignment: Text.AlignRight
                text: root.editorVm && root.editorVm.hasSliceResult
                      ? root.editorVm.estimatedPrintTime : ""
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                elide: Text.ElideRight
            }
            Item { Layout.preferredWidth: 30 }
            Image {
                Layout.preferredWidth: 18; Layout.preferredHeight: 18
                Layout.margins: Theme.spacingXS
                source: "qrc:/qml/assets/icons/print-weight.svg"
                sourceSize.width: 18; sourceSize.height: 18
            }
            Text {
                Layout.preferredWidth: 88
                Layout.margins: Theme.spacingXS
                horizontalAlignment: Text.AlignLeft
                text: root.editorVm ? root.editorVm.sliceResultWeight : ""
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                elide: Text.ElideRight
            }
        }

        // Gap before the rename simplebook (SendToPrinter.cpp:564)
        Item { Layout.preferredHeight: 6 }

        // -- Rename simplebook 420x25: ellipsized label + edit icon, or a
        // 380x24 text input (SendToPrinter.cpp:476-544) --
        Item {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 420
            Layout.preferredHeight: 25
            // Normal page: name label (max 390 DIP, ellipsized) + rename_edit
            // icon button (13 DIP).
            Row {
                anchors.centerIn: parent
                spacing: 4
                visible: !root.renaming
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(implicitWidth, 390)
                    text: root.sendName
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    elide: Text.ElideRight
                }
                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 13; height: 13
                    source: "qrc:/qml/assets/icons/rename_edit.svg"
                    sourceSize.width: 13; sourceSize.height: 13
                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -4
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.renaming = true
                            renameInput.text = root.sendName
                            renameInput.forceActiveFocus()
                            renameInput.selectAll()
                        }
                    }
                }
            }
            // Edit page: 380x24 input, commits on Enter and on focus loss,
            // ESC restores the previous name (SendToPrinter.cpp:508-518,530-544).
            Rectangle {
                anchors.centerIn: parent
                width: 380; height: 24
                radius: Theme.radiusSM
                color: Theme.bgInset
                border.color: Theme.borderInput
                visible: root.renaming
                TextInput {
                    id: renameInput
                    anchors.fill: parent
                    anchors.leftMargin: 6; anchors.rightMargin: 6
                    verticalAlignment: TextInput.AlignVCenter
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    clip: true
                    onAccepted: root.commitRename()
                    onActiveFocusChanged: {
                        if (!activeFocus && root.renaming)
                            root.commitRename()
                    }
                    Keys.onEscapePressed: (event) => {
                        event.accepted = true
                        root.cancelRename()
                    }
                }
            }
        }

        // Gap before the divider (SendToPrinter.cpp:566)
        Item { Layout.preferredHeight: 6 }

        // Divider with 30 DIP side margins (m_line_materia, SendToPrinter.cpp:567)
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 30; Layout.rightMargin: 30
            Layout.preferredHeight: 1
            color: Theme.borderSubtle
        }

        // Gap before the printer row (SendToPrinter.cpp:568)
        Item { Layout.preferredHeight: 12 }

        // -- Printer row: title + read-only combo (250 DIP) + Refresh
        // (m_sizer_printer, SendToPrinter.cpp:273-293,569) --
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 30; Layout.rightMargin: 30
            spacing: 0
            Text {
                Layout.leftMargin: 5
                text: qsTr("打印机")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLG
                font.bold: true
            }
            Item { Layout.preferredWidth: 12 }
            ComboBox {
                id: printerCombo
                Layout.preferredWidth: 250
                Layout.preferredHeight: 30
                enabled: root.sendState === 0 && !root.connecting
                model: root.printerNames
                displayText: currentIndex >= 0 && currentIndex < root.printerNames.length
                             ? root.printerNames[currentIndex] : ""
                onActivated: (index) => root.onPrinterActivated(index)
                background: Rectangle {
                    radius: Theme.radiusSM
                    color: Theme.bgInset
                    border.color: printerCombo.enabled ? Theme.borderInput : Theme.bgPressed
                }
                contentItem: Text {
                    leftPadding: 8
                    rightPadding: Theme.spacingXL
                    verticalAlignment: Text.AlignVCenter
                    text: printerCombo.displayText
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    elide: Text.ElideRight
                }
                indicator: Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 6
                    text: "▾"
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                }
                delegate: ItemDelegate {
                    id: printerDelegate
                    width: printerCombo.width
                    height: 28
                    highlighted: index === printerCombo.currentIndex
                    contentItem: Text {
                        leftPadding: 8
                        verticalAlignment: Text.AlignVCenter
                        text: modelData
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMD
                        elide: Text.ElideRight
                    }
                    background: Rectangle {
                        color: printerDelegate.hovered ? Theme.bgHover : "transparent"
                    }
                }
                popup: Popup {
                    y: printerCombo.height + 2
                    width: printerCombo.width
                    padding: Theme.spacingXXS
                    background: Rectangle {
                        color: Theme.bgPanel
                        border.color: Theme.borderInput
                        radius: Theme.radiusSM
                    }
                    contentItem: ListView {
                        clip: true
                        implicitHeight: Math.min(contentHeight, 168)
                        model: printerCombo.popup.visible ? printerCombo.delegateModel : null
                        currentIndex: printerCombo.highlightedIndex
                    }
                }
            }
            Item { Layout.preferredWidth: 5 }
            // Refresh (ButtonStyle::Confirm, SendToPrinter.cpp:289-293)
            Rectangle {
                Layout.leftMargin: 5
                Layout.preferredHeight: 26
                Layout.preferredWidth: refreshLabel.implicitWidth + 20
                radius: Theme.radiusSM
                color: !enabled ? Theme.bgPressed
                      : refreshMa.containsMouse ? Theme.accentDark : Theme.accent
                enabled: root.sendState === 0 && !root.connecting
                Text {
                    id: refreshLabel
                    anchors.centerIn: parent
                    text: qsTr("刷新")
                    color: Theme.textOnAccent
                    font.pixelSize: Theme.fontSizeSM
                }
                MouseArea {
                    id: refreshMa
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: parent.enabled
                    cursorShape: parent.enabled ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                    onClicked: {
                        // Upstream on_refresh -> update_user_machine_list
                        // (SendToPrinter.cpp:1041-1053).
                        root.monitorVm.refresh()
                        root.rebuildPrinterModel()
                    }
                }
            }
        }

        // Storage panel sits between the printer row and the status message
        // (SendToPrinter.cpp:570), shown only while Send is enabled
        // (Enable_Send_Button, SendToPrinter.cpp:1503-1516).
        Row {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 8
            spacing: 0
            visible: root.sendState === 0 && !root.connecting && root.printerSelected
            Repeater {
                model: root.storageEntries.length
                Row {
                    id: storageOption
                    required property int index
                    readonly property var entry: root.storageEntries[index]
                    readonly property bool enabled_: entry ? entry.enabled === true : false
                    readonly property bool checked: entry ? root.selectedStorageKey === entry.key : false
                    spacing: 6
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 14; height: 14; radius: Theme.radiusLG
                        color: storageOption.checked ? Theme.accent : "transparent"
                        border.color: storageOption.enabled_
                                      ? (storageOption.checked ? Theme.accent : Theme.textTertiary)
                                      : Theme.textDisabled
                        border.width: storageOption.checked ? 0 : 1
                        Rectangle {
                            anchors.centerIn: parent
                            width: 6; height: 6; radius: Theme.radiusSM
                            color: Theme.textOnAccent
                            visible: storageOption.checked
                        }
                        MouseArea {
                            anchors.fill: parent
                            enabled: storageOption.enabled_
                            cursorShape: storageOption.enabled_ ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                            onClicked: root.selectedStorageKey = storageOption.entry.key
                        }
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: storageOption.entry && storageOption.entry.key === "emmc"
                              ? qsTr("内部存储") : qsTr("外部存储")
                        color: storageOption.enabled_ ? Theme.textPrimary : Theme.textDisabled
                        font.pixelSize: Theme.fontSizeMD
                    }
                    Item { width: 20; height: 1 }
                }
            }
        }

        // Gap before the status message (SendToPrinter.cpp:571)
        Item { Layout.preferredHeight: 11 }

        // Status message, hidden when empty (m_statictext_printer_msg,
        // SendToPrinter.cpp:303-306,739-781; warning colour -> statusWarning).
        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.min(400, root.width - 30)
            visible: root.statusMsg !== ""
            text: root.statusMsg
            color: root.statusWarning ? Theme.statusWarning : Theme.textTertiary
            font.pixelSize: Theme.fontSizeMD
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }

        // Connecting feedback row: "Try to connect" + animated icon, hidden
        // by default (m_connecting_panel + AnimaIcon,
        // SendToPrinter.cpp:308-326,573; frames ams_rfid_1..4 at :318).
        Row {
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.spacingXS
            visible: root.connecting || root.connectFailed
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.connectFailed
                      ? qsTr("连接失败，点击图标重试") : qsTr("尝试连接")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
            }
            Item {
                id: connectAnimIcon
                anchors.verticalCenter: parent.verticalCenter
                width: 18; height: 18
                property int frame: 0
                Image {
                    anchors.fill: parent
                    source: root.connectFailed
                            ? "qrc:/qml/assets/icons/refresh_printer.svg"
                            : "qrc:/qml/assets/icons/ams_rfid_" + (connectAnimIcon.frame + 1) + ".svg"
                    sourceSize.width: 18; sourceSize.height: 18
                }
                Timer {
                    running: root.connecting && !root.connectFailed
                    interval: 100
                    repeat: true
                    onTriggered: connectAnimIcon.frame = (connectAnimIcon.frame + 1) % 4
                }
                MouseArea {
                    anchors.fill: parent
                    enabled: root.connectFailed
                    cursorShape: root.connectFailed ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                    onClicked: root.onPrinterActivated(printerCombo.currentIndex)
                }
            }
        }

        // Gap before the schedule divider (SendToPrinter.cpp:574)
        Item { Layout.preferredHeight: 22 }

        // Schedule divider (m_line_schedule, SendToPrinter.cpp:575)
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 30; Layout.rightMargin: 30
            Layout.preferredHeight: 1
            color: Theme.borderSubtle
        }

        // -- State simplebook 370x64 (m_simplebook,
        // SELECT_MACHINE_DIALOG_SIMBOOK_SIZE SelectMachine.hpp:114) --
        StackLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 370
            Layout.preferredHeight: 64
            currentIndex: root.sendState

            // Prepare page: full-width Send button pinned to the bottom with
            // a 10 DIP bottom margin (m_panel_prepare + m_button_ensure
            // wxEXPAND, SendToPrinter.cpp:334-350).
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    Item { Layout.fillHeight: true; Layout.preferredWidth: 1 }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.bottomMargin: 10
                        Layout.preferredHeight: 32
                        radius: Theme.radiusSM
                        readonly property bool active: root.canSend || (!root.hasSlicePath
                            && root.printerSelected && !root.slicingForPrint
                            && root.sendState === 0 && !root.connecting)
                        color: !active ? Theme.bgPressed
                              : sendMa.containsMouse ? Theme.accentDark : Theme.accent
                        Text {
                            anchors.centerIn: parent
                            text: root.slicingForPrint ? qsTr("切片中…") : qsTr("发送")
                            color: Theme.textOnAccent
                            font.pixelSize: Theme.fontSizeMD
                            font.bold: true
                        }
                        MouseArea {
                            id: sendMa
                            anchors.fill: parent
                            hoverEnabled: true
                            enabled: parent.active
                            cursorShape: parent.active ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                            onClicked: root.startSend()
                        }
                    }
                }
            }

            // Sending page: status text + 300x6 gauge + percent + Cancel
            // (BBLStatusBarSend, BBLStatusBarSend.cpp:13-87).
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 5; anchors.rightMargin: 5
                    spacing: 0
                    Item { Layout.fillHeight: true; Layout.preferredWidth: 1 }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("正在发送打印任务…")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeMD
                        elide: Text.ElideRight
                    }
                    Item { Layout.preferredHeight: 6; Layout.preferredWidth: 1 }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Rectangle {
                            Layout.preferredWidth: 300
                            Layout.preferredHeight: 6
                            radius: Theme.radiusSM
                            color: Theme.progressTrack
                            Rectangle {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                width: parent.width * root.sendProgress / 100
                                radius: Theme.radiusSM
                                color: Theme.progressFill
                            }
                        }
                        Text {
                            Layout.leftMargin: 10
                            Layout.rightMargin: 10
                            text: Math.round(root.sendProgress) + "%"
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeMD
                        }
                        Item { Layout.fillWidth: true }
                        Rectangle {
                            Layout.preferredHeight: 24
                            Layout.preferredWidth: cancelLabel.implicitWidth + 16
                            radius: Theme.radiusSM
                            color: cancelMa.containsMouse ? Theme.bgHover : Theme.bgSurface
                            border.color: Theme.borderSubtle
                            Text {
                                id: cancelLabel
                                anchors.centerIn: parent
                                text: qsTr("取消")
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeSM
                            }
                            MouseArea {
                                id: cancelMa
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.cancelSend()
                            }
                        }
                    }
                    Item { Layout.fillHeight: true; Layout.preferredWidth: 1 }
                }
            }

            // Finish page: 25 DIP completed bitmap + "Send complete"
            // (m_panel_finish, SendToPrinter.cpp:360-382; upstream teal
            // #009688 -> Theme.accent).
            Item {
                Row {
                    anchors.centerIn: parent
                    spacing: Theme.spacingMD
                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 25; height: 25
                        source: "qrc:/qml/assets/icons/completed.svg"
                        sourceSize.width: 25; sourceSize.height: 25
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("发送完成")
                        color: Theme.accent
                        font.pixelSize: Theme.fontSizeMD
                    }
                }
            }
        }

        // -- Failed info panel 380x125, hidden by default
        // (m_sw_print_failed_info, SendToPrinter.cpp:385-458,577) --
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 380
            Layout.preferredHeight: 125
            visible: root.sendFailed
            radius: Theme.radiusSM
            color: Theme.bgSurface
            clip: true
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingXS
                spacing: Theme.spacingXS
                // HyperLink "Check the status of current system services"
                // (SendToPrinter.cpp:450-453) -> surfaces the live network
                // status from the monitor VM (mock stack has no external
                // network-check page).
                Text {
                    text: qsTr("检查当前系统服务状态")
                    color: Theme.accent
                    font.pixelSize: Theme.fontSizeSM
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (!root.monitorVm)
                                return
                            root.failedExtra = root.monitorVm.networkOnline
                                ? qsTr("网络在线，延迟 %1 ms").arg(root.monitorVm.latencyMs)
                                : qsTr("网络离线")
                        }
                    }
                }
                Row {
                    spacing: 0
                    Text { width: 74; text: qsTr("错误代码") + ": "; color: Theme.textTertiary; font.pixelSize: Theme.fontSizeMD }
                    Text { width: 260; text: root.failedCode; color: Theme.textTertiary; font.pixelSize: Theme.fontSizeMD; wrapMode: Text.Wrap }
                }
                Row {
                    spacing: 0
                    Text { width: 74; text: qsTr("错误描述") + ": "; color: Theme.textTertiary; font.pixelSize: Theme.fontSizeMD }
                    Text { width: 260; text: root.failedDesc; color: Theme.textTertiary; font.pixelSize: Theme.fontSizeMD; wrapMode: Text.Wrap }
                }
                Row {
                    spacing: 0
                    Text { width: 74; text: qsTr("附加信息") + ": "; color: Theme.textTertiary; font.pixelSize: Theme.fontSizeMD }
                    Text { width: 260; text: root.failedExtra; color: Theme.textTertiary; font.pixelSize: Theme.fontSizeMD; wrapMode: Text.Wrap }
                }
            }
        }

        // Bottom margin (SendToPrinter.cpp:578)
        Item { Layout.preferredHeight: 13 }

        // R-P1.E disclosure: this dialog sends through the mock device stack
        // (DeviceServiceMock), so the demo nature stays user-visible.
        Text {
            Layout.fillWidth: true
            Layout.leftMargin: 16; Layout.rightMargin: 16
            text: qsTr("演示模式：设备列表与发送流程为本地模拟数据，真实设备推送依赖 MQTT，当前为外部阻塞项。")
            color: Theme.textDisabled
            font.pixelSize: Theme.fontSizeXS
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }
    }

    // Sending page upload animation: the mock send completes locally, so the
    // gauge animates over a fixed interval and settles on the finish page
    // (upstream settles via the real upload progress callbacks).
    NumberAnimation {
        id: sendAnim
        target: root
        property: "sendProgress"
        from: 0
        to: 100
        duration: 1400
        onFinished: {
            if (root.sendState === 1)
                root.sendState = 2
        }
    }

    // Mirrors the 2.5s MQTT connect timeout in MonitorViewModel::connectDevice:
    // if the device did not answer by then the row flips to the retry state.
    Timer {
        id: connectFallbackTimer
        interval: 2600
        onTriggered: {
            if (root.connecting)
                root.finishConnect(false)
        }
    }

    // G-03: an on-demand slice started from this dialog continues here --
    // the bindings refresh time/weight/thumbnail automatically once
    // stateChanged fires, so the dialog stays open in the prepare state.
    Connections {
        target: root.slicingForPrint ? root.editorVm : null
        enabled: root.slicingForPrint
        function onPrintSliceReady(gcodePath) {
            root.slicingForPrint = false
            root.thumbTick++
            if (gcodePath)
                root.showStatus(qsTr("切片完成，选择打印机后发送。"), false)
        }
        // A failed or cancelled slice releases the on-demand state and opens
        // the failed-info panel with the real service message (upstream
        // shows the panel on send/slice errors, SendToPrinter.cpp:461-463).
        function onStateChanged() {
            if (!root.editorVm)
                return
            if (!root.editorVm.isSlicing() && (root.editorVm.lastGcodePath || "") === "") {
                if (root.slicingForPrint) {
                    root.slicingForPrint = false
                    root.sendFailed = true
                    root.failedCode = ""
                    root.failedDesc = root.editorVm.statusText
                    root.failedExtra = ""
                }
            }
        }
    }

    // Track the connecting flow: selectedDeviceChanged fires when the mock
    // connect lands (MonitorViewModel::connectDevice 1.5-2.5s).
    Connections {
        target: root.monitorVm
        function onSelectedDeviceChanged() {
            if (root.connecting && root.monitorVm.selectedDeviceOnline)
                root.finishConnect(true)
            root.rebuildStorages()
        }
        function onDevicesChanged() {
            root.rebuildPrinterModel()
        }
    }

    // v2.5 DEV-05: SelectMachineDialog instance. Kept instantiated and
    // reachable from the empty-device-list path in startSend() -- the audit
    // gate (tests/QmlUiAuditTests.cpp:10412/:10434) requires the
    // "selectMachineDialog.open()" trigger + "SelectMachineDialog {"
    // instantiation outside the dialog's own file.
    SelectMachineDialog {
        id: selectMachineDialog
        deviceVm: root.monitorVm
        gcodePath: ""
    }
}
