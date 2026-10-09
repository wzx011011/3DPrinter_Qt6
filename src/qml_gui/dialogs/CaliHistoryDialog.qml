import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// D4 -- CaliHistoryDialog: calibration history records dialog
// Usage: CaliHistoryDialog { id: historyDlg; calibrationVm: ... }  ->  historyDlg.open()
// CalibrationPage "History" button opens this dialog
// Aligns with upstream HistoryWindow (CaliHistoryDialog.cpp:109-197): 700x600
// window, "Flow Dynamics Calibration Result" title, New button, nozzle-
// diameter filter combo, single-line status tips, Name|Filament|Factor K|
// Action table (:364-399) with per-row Delete (:418-442) and Edit (:444-465).
// The New/Edit sub-dialogs live in this file (in-file dialogs, no new qrc
// entries), mirroring NewCalibrationHistoryDialog (:736-1003) and
// EditCalibrationHistoryDialog (:540-697). Upstream polls the device with a
// 200ms timer (:191-194); OWzx reloads on open/historyChanged (behavior
// adaptation, registered).
CxDialog {
    id: root
    required property var calibrationVm

    dialogTitle: qsTr("流动动力学校准结果")

    // Centering comes from the CxDialog default (anchors.centerIn:
    // Overlay.overlay, U01 global fix)
    // Upstream HISTORY_WINDOW_SIZE (CaliHistoryDialog.cpp:24)
    width: 700
    height: 600

    // Nozzle-diameter filter choices (CaliHistoryDialog.cpp:247, list
    // {0.2, 0.4, 0.6, 0.8} formatted "%1.1f mm")
    readonly property var _nozzleChoices: [0.2, 0.4, 0.6, 0.8]
    // Upstream PA K-value input range: 0 < K < 2, exclusive
    // (MIN/MAX_PA_K_VALUE, CalibUtils.cpp:27-28/:427)

    property var _allItems: []
    property var _historyItems: []

    // Phase 171 (CL-01): destructive-action confirm for 清空 (clear history).
    ConfirmDialog {
        id: clearConfirm
        dialogTitle: qsTr("清空校准历史")
        message: qsTr("确定要清空所有校准历史记录吗？此操作不可撤销。")
        confirmText: qsTr("清空")
        cancelText: qsTr("取消")
        destructive: true
        onAccepted: {
            if (root.calibrationVm)
                root.calibrationVm.clearHistory()
        }
    }

    // ---- U03: manual record sub-dialog (New button) ----
    // Mirrors upstream NewCalibrationHistoryDialog (CaliHistoryDialog.cpp:736-1003):
    // Name / Filament / Nozzle Diameter / Factor K, 250x24 inputs, validation
    // on OK. Upstream confirms a duplicate name with a YES/NO overwrite
    // prompt; the mock store has no dedup, so the duplicate is intercepted
    // with an inline warning instead (registered conservative deviation).
    CxDialog {
        id: newEntryDialog
        dialogTitle: qsTr("新建流动动力学校准")
        width: 440
        height: newCol.implicitHeight + 80

        contentItem: ColumnLayout {
            id: newCol
            width: newEntryDialog.width - 32
            spacing: Theme.spacingMD

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingXS
                Text { text: qsTr("名称"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM }
                CxTextField {
                    id: newNameField
                    Layout.fillWidth: true
                    placeholderText: qsTr("校准记录名称")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingXS
                Text { text: qsTr("耗材"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM }
                CxComboBox {
                    id: newFilamentCombo
                    Layout.fillWidth: true
                    model: root.calibrationVm ? root.calibrationVm.filamentPresetNames : []
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingXS
                Text { text: qsTr("喷嘴直径"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM }
                CxComboBox {
                    id: newNozzleCombo
                    Layout.fillWidth: true
                    textRole: "label"
                    model: root._nozzleChoices.map(function (v) { return { label: v.toFixed(1) + " mm", value: v } })
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingXS
                Text { text: qsTr("K 值"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM }
                CxTextField {
                    id: newKField
                    Layout.fillWidth: true
                    text: "0.020"
                    placeholderText: qsTr("0 < K < 2.0")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                }
            }

            Text {
                id: newErrorText
                Layout.fillWidth: true
                visible: text.length > 0
                color: Theme.statusError
                font.pixelSize: Theme.fontSizeXS
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("取消")
                    cxStyle: CxButton.Style.Secondary
                    onClicked: newEntryDialog.reject()
                }
                CxButton {
                    text: qsTr("保存")
                    cxStyle: CxButton.Style.Primary
                    onClicked: {
                        newErrorText.text = ""
                        // Name: non-empty, <= 40 chars (upstream
                        // validate_input_name, CalibUtils.cpp:392-405)
                        var name = newNameField.text.trim()
                        if (name.length === 0) { newErrorText.text = qsTr("名称不能为空"); return }
                        if (name.length > 40) { newErrorText.text = qsTr("名称不能超过 40 个字符"); return }
                        // Filament must be selected (upstream :929-933)
                        if (newFilamentCombo.currentIndex < 0) { newErrorText.text = qsTr("必须选择耗材"); return }
                        // K range: 0 < K < 2.0 exclusive (upstream
                        // validate_input_k_value, CalibUtils.cpp:407-430)
                        var k = parseFloat(newKField.text)
                        if (isNaN(k) || k <= 0.0 || k >= 2.0) {
                            newErrorText.text = qsTr("请输入有效值（K 介于 0.0~2.0）")
                            return
                        }
                        // Duplicate interception (upstream :969-993 asks to
                        // overwrite; the mock store has no dedup, so block)
                        var filament = newFilamentCombo.currentText
                        for (var i = 0; i < root._allItems.length; ++i) {
                            if (root._allItems[i].name === name
                                    && root._allItems[i].filamentId === filament) {
                                newErrorText.text = qsTr("同名且同耗材的记录已存在，请更换名称")
                                return
                            }
                        }
                        var nozzle = root._nozzleChoices[newNozzleCombo.currentIndex >= 0 ? newNozzleCombo.currentIndex : 1]
                        root.calibrationVm.addManualHistoryEntry(name, filament, nozzle, k)
                        newEntryDialog.close()
                    }
                }
            }
        }

        onOpened: {
            newErrorText.text = ""
            newNameField.text = ""
            newKField.text = "0.020"
            newFilamentCombo.currentIndex = -1 // upstream opens unselected (:773); OK requires an explicit pick (:929-933)
            // Default nozzle = the machine's current nozzle diameter
            // (upstream :834-840), snapped to the 0.2/0.4/0.6/0.8 list
            var cur = root.calibrationVm ? root.calibrationVm.currentNValue : 0.4
            var idx = Math.round((cur - 0.2) / 0.2)
            newNozzleCombo.currentIndex = Math.max(0, Math.min(3, idx))
            newNameField.forceActiveFocus()
        }
    }

    // ---- U03: edit sub-dialog ----
    // Mirrors upstream EditCalibrationHistoryDialog (CaliHistoryDialog.cpp:540-697):
    // Name + Factor K editable (160x24 inputs), Filament shown read-only,
    // K range validation + duplicate-name interception (:666-686).
    property int _editSrcIndex: -1
    CxDialog {
        id: editEntryDialog
        dialogTitle: qsTr("编辑流动动力学校准")
        width: 400
        height: editCol.implicitHeight + 80

        contentItem: ColumnLayout {
            id: editCol
            width: editEntryDialog.width - 32
            spacing: Theme.spacingMD

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingXS
                Text { text: qsTr("名称"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM }
                CxTextField {
                    id: editNameField
                    Layout.fillWidth: true
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingXS
                Text { text: qsTr("耗材"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM }
                Text {
                    id: editFilamentText
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingXS
                Text { text: qsTr("K 值"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM }
                CxTextField {
                    id: editKField
                    Layout.fillWidth: true
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                }
            }

            Text {
                id: editErrorText
                Layout.fillWidth: true
                visible: text.length > 0
                color: Theme.statusError
                font.pixelSize: Theme.fontSizeXS
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("取消")
                    cxStyle: CxButton.Style.Secondary
                    onClicked: editEntryDialog.reject()
                }
                CxButton {
                    text: qsTr("保存")
                    cxStyle: CxButton.Style.Primary
                    onClicked: {
                        editErrorText.text = ""
                        var name = editNameField.text.trim()
                        if (name.length === 0) { editErrorText.text = qsTr("名称不能为空"); return }
                        if (name.length > 40) { editErrorText.text = qsTr("名称不能超过 40 个字符"); return }
                        var k = parseFloat(editKField.text)
                        if (isNaN(k) || k <= 0.0 || k >= 2.0) {
                            editErrorText.text = qsTr("请输入有效值（K 介于 0.0~2.0）")
                            return
                        }
                        // Duplicate interception, excluding the entry itself
                        // (upstream :666-686: name must be unique within the
                        // same filament)
                        var item = root._allItems[root._editSrcIndex]
                        for (var i = 0; i < root._allItems.length; ++i) {
                            if (i === root._editSrcIndex) continue
                            if (root._allItems[i].name === name
                                    && root._allItems[i].filamentId === item.filamentId) {
                                editErrorText.text = qsTr("同名且同耗材的记录已存在，请更换名称")
                                return
                            }
                        }
                        root.calibrationVm.updateHistoryEntry(root._editSrcIndex, name, k)
                        editEntryDialog.close()
                    }
                }
            }
        }
    }

    function openEditDialog(srcIndex) {
        var item = root._allItems[srcIndex]
        if (!item) return
        root._editSrcIndex = srcIndex
        editNameField.text = item.name
        // Prefill the record's own K ("%.3f", upstream :612-613). A no-K
        // record shows "0.000" -- which then fails K validation on save --
        // never a fabricated default K.
        editKField.text = item.kValue.toFixed(3)
        editFilamentText.text = root.calibrationVm
                                ? root.calibrationVm.historyFilamentName(srcIndex) : item.filamentId
        editErrorText.text = ""
        editEntryDialog.open()
    }

    function reloadHistory() {
        var all = []
        var n = calibrationVm ? calibrationVm.historyCount() : 0
        for (var i = 0; i < n; ++i) {
            all.push({
                srcIndex: i,
                name: calibrationVm.historyName(i),
                filamentId: calibrationVm.historyFilamentId(i),
                kValue: calibrationVm.historyKValue(i),
                flowRate: calibrationVm.historyFlowRate(i),
                nozzleDiameter: calibrationVm.historyNozzleDiameter(i),
                timestamp: calibrationVm.historyTimestamp(i)
            })
        }
        _allItems = all
        applyFilter()
    }

    // U03: filter by the VM's nozzle-diameter filter (upstream requests
    // per-nozzle records from the device, :304-325; the OWzx history is
    // local, so the dialog filters the stored entries)
    function applyFilter() {
        var f = calibrationVm ? calibrationVm.historyNozzleFilter : 0
        var arr = []
        for (var i = 0; i < _allItems.length; ++i) {
            if (f <= 0 || Math.abs(_allItems[i].nozzleDiameter - f) < 1e-3)
                arr.push(_allItems[i])
        }
        _historyItems = arr
    }

    Connections {
        target: root.calibrationVm
        function onHistoryChanged() { reloadHistory() }
        function onHistoryFilterChanged() { applyFilter() }
    }

    onOpened: {
        // Default filter = the machine's current nozzle diameter
        // (upstream :247-255 selection default)
        if (root.calibrationVm) {
            var cur = root.calibrationVm.currentNValue
            var idx = Math.max(0, Math.min(3, Math.round((cur - 0.2) / 0.2)))
            root.calibrationVm.historyNozzleFilter = root._nozzleChoices[idx]
            nozzleFilterCombo.currentIndex = idx
        }
        reloadHistory()
    }

    // Single-line status text at the top of the list area (upstream m_tips,
    // CaliHistoryDialog.cpp:163-171/:227-234, colour 145,145,145 ->
    // Theme.textTertiary)
    readonly property string historyStatusText: {
        if (!root.calibrationVm || root._allItems.length === 0)
            return qsTr("暂无校准历史记录")
        if (root._historyItems.length === 0)
            return qsTr("当前喷嘴直径下暂无校准记录")
        return qsTr("成功获取校准历史记录（共 %1 条）").arg(root._allItems.length)
    }

    // Fixed column widths shared by the header row and the delegates
    readonly property int _filamentColWidth: 120
    readonly property int _kColWidth: 80
    readonly property int _actionColWidth: 190

    contentItem: ColumnLayout {
        id: contentCol
        width: root.width - 32
        spacing: Theme.spacingMD

        // New button (upstream :127-131, Confirm style, left margin 20)
        RowLayout {
            Layout.fillWidth: true
            CxButton {
                text: qsTr("新建")
                cxStyle: CxButton.Style.Primary
                width: 64; height: 26
                onClicked: newEntryDialog.open()
            }
            Item { Layout.fillWidth: true }

            // Nozzle Diameter filter (upstream :144-159/:247-255)
            Text {
                text: qsTr("喷嘴直径")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLG
                font.bold: true
            }
            CxComboBox {
                id: nozzleFilterCombo
                Layout.preferredWidth: 120
                textRole: "label"
                model: root._nozzleChoices.map(function (v) { return { label: v.toFixed(1) + " mm", value: v } })
                onActivated: function (index) {
                    if (root.calibrationVm)
                        root.calibrationVm.historyNozzleFilter = root._nozzleChoices[index]
                }
            }
        }

        // Status text (single grey line at the top of the list area)
        Text {
            Layout.fillWidth: true
            text: root.historyStatusText
            color: Theme.textTertiary
            font.pixelSize: Theme.fontSizeSM
            elide: Text.ElideRight
        }

        // Table header row: Name | Filament | Factor K | Action, 14px bold
        // (upstream Head_14 labels, CaliHistoryDialog.cpp:364-399)
        RowLayout {
            Layout.fillWidth: true
            visible: root._historyItems.length > 0
            spacing: Theme.spacingMD

            Text {
                text: qsTr("名称")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLG
                font.bold: true
                Layout.fillWidth: true
            }
            Text {
                text: qsTr("耗材")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLG
                font.bold: true
                Layout.preferredWidth: root._filamentColWidth
            }
            Text {
                text: qsTr("K 值")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLG
                font.bold: true
                Layout.preferredWidth: root._kColWidth
            }
            Text {
                text: qsTr("操作")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLG
                font.bold: true
                Layout.preferredWidth: root._actionColWidth
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.bgCard
            visible: root._historyItems.length > 0
        }

        // History list
        ListView {
            id: historyListView
            visible: root._historyItems.length > 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root._historyItems
            spacing: Theme.spacingXS
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: Rectangle {
                width: historyListView.width
                height: 68
                radius: Theme.radiusMD
                color: delegateHov.containsMouse ? Theme.bgCard : Theme.bgPanel
                border.color: Theme.borderInput
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingLG
                    anchors.rightMargin: Theme.spacingLG
                    spacing: Theme.spacingMD

                    // Type icon: tintable SVG glyph (replaces the coloured
                    // gear emoji)
                    Rectangle {
                        width: 36; height: 36; radius: Theme.radiusMD
                        color: Theme.chromePressed
                        Image {
                            anchors.centerIn: parent
                            source: "qrc:/qml/assets/icons/settings.svg"
                            sourceSize.width: 20
                            sourceSize.height: 20
                            fillMode: Image.PreserveAspectFit
                        }
                    }

                    // Name + meta
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingXS
                        Text {
                            text: modelData.name
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            spacing: Theme.spacingLG
                            Text {
                                visible: modelData.flowRate > 0
                                text: qsTr("Flow: %1").arg(modelData.flowRate.toFixed(3))
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSizeXS
                            }
                            Text {
                                text: qsTr("喷嘴: %1mm").arg(modelData.nozzleDiameter.toFixed(2))
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSizeXS
                            }
                            Text {
                                text: {
                                    var ts = modelData.timestamp
                                    if (!ts) return ""
                                    try {
                                        var dt = new Date(ts)
                                        return Qt.formatDateTime(dt, "yyyy-MM-dd hh:mm")
                                    } catch (e) {
                                        return ts
                                    }
                                }
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSizeXS
                            }
                        }
                    }

                    // Filament preset name resolved on the VM (upstream
                    // get_preset_name_by_filament_id)
                    Text {
                        text: root.calibrationVm
                              ? root.calibrationVm.historyFilamentName(modelData.srcIndex) : ""
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeXS
                        elide: Text.ElideRight
                        Layout.preferredWidth: root._filamentColWidth
                    }

                    // Factor K column (upstream "%.3f" format, :413)
                    Text {
                        text: modelData.kValue > 0 ? modelData.kValue.toFixed(3) : qsTr("--")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                        Layout.preferredWidth: root._kColWidth
                    }

                    // Action column: Load (OWzx keep) + Delete (Alert) + Edit
                    // (Confirm), upstream :418-465
                    RowLayout {
                        Layout.preferredWidth: root._actionColWidth
                        spacing: Theme.spacingXS

                        CxButton {
                            text: qsTr("加载")
                            cxStyle: CxButton.Style.Secondary
                            width: 52; height: 24
                            toolTipText: qsTr("将此记录载入校准参数")
                            onClicked: {
                                // index = the record's index in the full
                                // (unfiltered) history; the filtered view
                                // delegate position differs from it
                                var index = modelData.srcIndex
                                root.calibrationVm.loadHistoryEntry(index)
                                root.close()
                            }
                        }
                        CxButton {
                            text: qsTr("删除")
                            cxStyle: CxButton.Style.Danger
                            width: 52; height: 24
                            onClicked: {
                                var index = modelData.srcIndex
                                root.calibrationVm.deleteHistoryEntry(index)
                            }
                        }
                        CxButton {
                            text: qsTr("编辑")
                            cxStyle: CxButton.Style.Primary
                            width: 52; height: 24
                            onClicked: {
                                var index = modelData.srcIndex
                                root.openEditDialog(index)
                            }
                        }
                    }
                }

                HoverHandler { id: delegateHov }
            }
        }

        // Footer with clear button
        Rectangle {
            Layout.fillWidth: true; height: 1; color: Theme.bgCard
            visible: root._historyItems.length > 0
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root._historyItems.length > 0

            Text {
                text: qsTr("共 %1 条记录").arg(root._historyItems.length)
                color: Theme.borderActive
                font.pixelSize: Theme.fontSizeXS
            }

            Item { Layout.fillWidth: true }

            // U03: hover branch uses the token hover surface and the
            // semantic error text token (was a dead error-subtle fill with
            // a hardcoded "#ff9090")
            Rectangle {
                width: 72; height: 28; radius: Theme.radiusSM
                color: clearHov.containsMouse ? Theme.bgHover : "transparent"
                border.color: Theme.statusError
                border.width: 1
                Text { anchors.centerIn: parent; text: qsTr("清空"); color: Theme.statusError; font.pixelSize: Theme.fontSizeSM }
                MouseArea {
                    id: clearHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        // Phase 171 (CL-01): confirm before clearing (was firing immediately).
                        clearConfirm.open()
                    }
                }
            }
        }
    }
}
