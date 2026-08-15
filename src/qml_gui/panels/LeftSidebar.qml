pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"
import "../components"

Rectangle {
    id: root
    required property var editorVm
    required property var configVm
    property string processCategory: ""
    signal exportRequested()

    readonly property int targetSidebarWidth: 320
    readonly property color panelSurface: Theme.bgElevated
    readonly property color sectionSurface: Theme.bgHover
    readonly property color controlSurface: Theme.borderDefault
    readonly property color fieldSurface: Theme.chromePressed
    readonly property color dividerColor: Theme.bgPressed
    readonly property color mutedText: Theme.textSecondary

    property string paramsCurrentTab: "Quality"
    property string paramsSearchText: ""
    readonly property var paramsOptionModel: {
        if (!root.configVm) return null
        return root.configVm.printOptions
    }
    readonly property string paramsTier: "print"
    property var paramsFilteredIndices: []

    color: panelSurface
    radius: 0
    border.width: 0

    function rebuildParamsFilter() {
        if (!root.configVm || !root.paramsOptionModel) {
            root.paramsFilteredIndices = []
            return
        }
        var indices = root.configVm.filterOptionIndices(
                    root.paramsTier, root.paramsSearchText, true)
        if (root.paramsCurrentTab !== "")
            indices = root.paramsOptionModel.filterIndicesByPage(indices, root.paramsCurrentTab)
        root.paramsFilteredIndices = indices
    }

    CxScrollView {
        id: sidebarScroll
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        anchors.topMargin: 10
        anchors.bottomMargin: 8
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: Math.max(0, sidebarScroll.availableWidth)
            spacing: 8

            PixelHeader {
                Layout.fillWidth: true
                title: qsTr("打印机")
                iconSource: "qrc:/qml/assets/icons/printer.svg"
                actionIcon: "qrc:/qml/assets/icons/settings.svg"
                actionToolTip: qsTr("打印机设置")
                onActionTriggered: backend.forwardSettingsRequest("printer")
            }

            // v5.14: compact preset row replaces the 76px hero card (screenshot
            // truth shows preset selectors as dense single rows, matching the
            // filament/process rows below).
            Rectangle {
                id: printerPresetRow
                Layout.fillWidth: true
                Layout.preferredHeight: 38
                radius: 4
                color: root.sectionSurface
                border.width: 1
                border.color: root.dividerColor

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 7

                    Image {
                        Layout.preferredWidth: 18
                        Layout.preferredHeight: 18
                        source: "qrc:/qml/assets/icons/printer.svg"
                        fillMode: Image.PreserveAspectFit
                        opacity: 0.8
                    }

                    CxComboBox {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 28
                        font.pixelSize: Theme.fontSizeSM
                        // v5.16 (PSET2-05): decorated list — section
                        // separators + incompatibility gray-out.
                        model: root.configVm ? root.configVm.decoratedPrinterPresetNames : []
                        currentIndex: {
                            if (!root.configVm) return -1
                            return root.configVm.decoratedPrinterPresetNames.indexOf(root.configVm.currentPrinterPreset)
                        }
                        onActivated: (i) => {
                            if (!root.configVm) return
                            if (i >= 0 && i < model.length)
                                root.configVm.requestCurrentPrinterPreset(
                                    root.configVm.plainPresetName(model[i]))
                        }
                    }

                    Rectangle {
                        visible: !!root.configVm && root.configVm.isPresetDirty
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 8
                        radius: 4
                        color: Theme.accent
                        ToolTip.text: qsTr("预设已修改（未保存）")
                        ToolTip.visible: printerDirtyMA.containsMouse
                        MouseArea { id: printerDirtyMA; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                    }

                    // v5.16 (PSET2-07): per-row preset edit affordance
                    // (rename/delete, upstream preset combo right-click menu).
                    PixelIconButton {
                        iconSource: "qrc:/qml/assets/icons/dots.svg"
                        toolTipText: qsTr("预设操作")
                        onClicked: root.editPreset(2, root.configVm ? root.configVm.currentPrinterPreset : "")
                    }

                    PixelIconButton {
                        iconSource: "qrc:/qml/assets/icons/settings.svg"
                        toolTipText: qsTr("编辑打印机预设")
                        onClicked: backend.forwardSettingsRequest("printer")
                    }

                    PixelIconButton {
                        iconSource: "qrc:/qml/assets/icons/send-2.svg"
                        toolTipText: qsTr("打印机连接")
                        onClicked: backend.showPrintHostDialog()
                    }
                }
            }

            PixelHeader {
                Layout.fillWidth: true
                title: qsTr("耗材")
                iconText: "F"
                actionIcon: "qrc:/qml/assets/icons/settings.svg"
                actionToolTip: qsTr("耗材设置")
                onActionTriggered: backend.forwardSettingsRequest("filament")
            }

            Repeater {
                model: Math.max(1, root.editorVm ? root.editorVm.extruderCount : 1)

                delegate: Rectangle {
                    id: filamentPixelRow
                    required property int index
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    radius: 4
                    color: root.sectionSurface
                    border.width: 1
                    border.color: root.configVm && !root.configVm.isFilamentCompatibleForSlot(filamentPixelRow.index) ? Theme.statusError : root.dividerColor

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        spacing: 7

                        Rectangle {
                            Layout.preferredWidth: 18
                            Layout.preferredHeight: 18
                            radius: 9
                            color: root.filamentColor(filamentPixelRow.index)
                            border.width: 1
                            border.color: Qt.lighter(root.filamentColor(filamentPixelRow.index), 1.25)
                        }

                        Text {
                            text: String(filamentPixelRow.index + 1)
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                            font.bold: true
                        }

                        CxComboBox {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 28
                            font.pixelSize: Theme.fontSizeSM
                            // v5.16 (PSET2-05): decorated list — section
                            // separators + incompatibility gray-out.
                            model: root.configVm ? root.configVm.decoratedFilamentPresetNames : []
                            currentIndex: {
                                if (!root.configVm) return -1
                                // v5.16 (CIRC-04): each slot shows ITS OWN preset
                                // (was: the category list's Nth entry for all slots).
                                // PSET2-05: compare against plain names since the
                                // list entries may carry the display suffix.
                                var slotPreset = root.configVm.filamentPresetForSlot(filamentPixelRow.index)
                                var names = root.configVm.decoratedFilamentPresetNames
                                for (var i = 0; i < names.length; i++) {
                                    if (root.configVm.plainPresetName(names[i]) === slotPreset) return i
                                }
                                return -1
                            }
                            onActivated: (i) => {
                                if (!root.configVm) return
                                var names = root.configVm.decoratedFilamentPresetNames
                                if (i >= 0 && i < names.length)
                                    root.configVm.requestFilamentPresetForSlot(
                                        filamentPixelRow.index,
                                        root.configVm.plainPresetName(names[i]))
                            }
                        }

                        // v5.16 (PSET2-07): per-row preset edit affordance
                        // (renames/deletes the GLOBAL filament preset of this
                        // slot's selection, upstream preset combo menu).
                        PixelIconButton {
                            iconSource: "qrc:/qml/assets/icons/dots.svg"
                            toolTipText: qsTr("预设操作")
                            onClicked: root.editPreset(1, root.configVm
                                ? root.configVm.filamentPresetForSlot(filamentPixelRow.index) : "")
                        }

                        PixelIconButton {
                            iconSource: "qrc:/qml/assets/icons/settings.svg"
                            toolTipText: qsTr("编辑耗材预设")
                            onClicked: backend.forwardSettingsRequest("filament")
                        }

                        Rectangle {
                            visible: !!root.configVm && !root.configVm.isFilamentCompatibleForSlot(filamentPixelRow.index)
                            Layout.preferredWidth: 8
                            Layout.preferredHeight: 8
                            radius: 4
                            color: Theme.statusError
                            ToolTip.text: root.configVm ? root.configVm.currentPresetCompatibilityMessage : ""
                            ToolTip.visible: filamentCompatMA.containsMouse
                            MouseArea { id: filamentCompatMA; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                        }
                    }
                }
            }

            PixelHeader {
                Layout.fillWidth: true
                title: qsTr("工艺")
                iconSource: "qrc:/qml/assets/icons/list-details.svg"
                actionIcon: "qrc:/qml/assets/icons/settings.svg"
                actionToolTip: qsTr("工艺设置")
                onActionTriggered: backend.forwardSettingsRequest("process")
            }

            RowLayout {
                id: processScopeBar
                Layout.fillWidth: true
                spacing: 6

                PixelSegment {
                    Layout.fillWidth: true
                    text: qsTr("全局")
                    selected: root.configVm && root.configVm.settingsScope === "global"
                    onClicked: if (root.configVm) root.configVm.requestGlobalScope()
                }

                PixelSegment {
                    Layout.fillWidth: true
                    text: qsTr("对象")
                    selected: root.configVm && root.configVm.settingsScope !== "global" && root.configVm.settingsScope !== "plate"
                    onClicked: {
                        if (root.editorVm && root.editorVm.selectedObjectIndex >= 0 && root.configVm) {
                            root.configVm.requestObjectScope("object", "",
                                root.editorVm.selectedObjectIndex, -1)
                        }
                    }
                }

                PixelSegment {
                    Layout.fillWidth: true
                    text: qsTr("盘")
                    selected: root.configVm && root.configVm.settingsScope === "plate"
                    enabled: root.editorVm && root.editorVm.currentPlateIndex >= 0
                    onClicked: {
                        if (root.editorVm && root.editorVm.currentPlateIndex >= 0 && root.configVm)
                            root.configVm.requestPlateScope(root.editorVm.currentPlateIndex)
                    }
                }
            }

            Rectangle {
                id: processPresetRow
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                radius: 4
                color: root.sectionSurface
                border.width: 1
                border.color: root.dividerColor

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 7

                    CxComboBox {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 28
                        font.pixelSize: Theme.fontSizeSM
                        // v5.16 (PSET2-05): decorated list — section
                        // separators + incompatibility gray-out.
                        model: root.configVm ? root.configVm.decoratedPrintPresetNames : []
                        currentIndex: {
                            if (!root.configVm) return -1
                            return root.configVm.decoratedPrintPresetNames.indexOf(root.configVm.currentPrintPreset)
                        }
                        onActivated: (i) => {
                            if (root.configVm && i >= 0)
                                root.configVm.requestCurrentPrintPreset(
                                    root.configVm.plainPresetName(model[i]))
                        }
                    }

                    Rectangle {
                        visible: !!root.configVm && root.configVm.isPresetDirty
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 8
                        radius: 4
                        color: Theme.accent
                        ToolTip.text: qsTr("预设已修改（未保存）")
                        ToolTip.visible: processDirtyMA.containsMouse
                        MouseArea { id: processDirtyMA; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                    }

                    // v5.16 (PSET2-07): per-row preset edit affordance.
                    PixelIconButton {
                        iconSource: "qrc:/qml/assets/icons/dots.svg"
                        toolTipText: qsTr("预设操作")
                        onClicked: root.editPreset(0, root.configVm ? root.configVm.currentPrintPreset : "")
                    }

                    PixelIconButton {
                        iconSource: "qrc:/qml/assets/icons/settings.svg"
                        toolTipText: qsTr("编辑工艺预设")
                        onClicked: backend.forwardSettingsRequest("process")
                    }
                }
            }

            Rectangle {
                id: searchBox
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                radius: 4
                color: root.fieldSurface
                border.width: 1
                border.color: searchField.activeFocus ? Theme.accent : root.dividerColor

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 6

                    Image {
                        Layout.preferredWidth: 14
                        Layout.preferredHeight: 14
                        source: "qrc:/qml/assets/icons/list-details.svg"
                        opacity: 0.55
                    }

                    TextField {
                        id: searchField
                        Layout.fillWidth: true
                        placeholderText: qsTr("搜索设置...")
                        color: Theme.textPrimary
                        placeholderTextColor: Theme.textDisabled
                        font.pixelSize: Theme.fontSizeSM
                        background: Item {}
                        selectByMouse: true
                        onAccepted: root.rebuildParamsFilter()
                        onTextChanged: {
                            root.paramsSearchText = text.trim()
                            root.rebuildParamsFilter()
                        }
                    }
                }
            }

            Rectangle {
                id: paramsInlinePanel
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(760, Math.max(420, paramsList.contentHeight + 36))
                radius: 4
                color: root.sectionSurface
                border.width: 1
                border.color: root.dividerColor

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 3

                        Repeater {
                            model: [
                                { key: "Quality", label: qsTr("质量") },
                                { key: "Strength", label: qsTr("强度") },
                                { key: "Support", label: qsTr("支撑") },
                                { key: "Temperature", label: qsTr("材料") },
                                { key: "Other", label: qsTr("其他") }
                            ]
                            delegate: Rectangle {
                                id: paramsTabDelegate
                                required property var modelData
                                Layout.fillWidth: true
                                Layout.preferredHeight: 25
                                radius: 3
                                color: paramsTabDelegate.modelData.key === root.paramsCurrentTab ? Theme.accent : root.fieldSurface
                                border.width: 1
                                border.color: paramsTabDelegate.modelData.key === root.paramsCurrentTab ? Theme.accent : root.dividerColor

                                Text {
                                    anchors.centerIn: parent
                                    text: paramsTabDelegate.modelData.label
                                    color: paramsTabDelegate.modelData.key === root.paramsCurrentTab ? Theme.textOnAccent : root.mutedText
                                    font.pixelSize: Theme.fontSizeXS
                                    font.bold: paramsTabDelegate.modelData.key === root.paramsCurrentTab
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        root.paramsCurrentTab = paramsTabDelegate.modelData.key
                                        root.rebuildParamsFilter()
                                    }
                                }
                            }
                        }
                    }

                    ListView {
                        id: paramsList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: root.paramsFilteredIndices
                        spacing: 0

                        ScrollBar.vertical: ScrollBar {
                            visible: paramsList.contentHeight > paramsList.height
                        }

                        delegate: Item {
                            id: paramsDelegate
                            required property int index
                            required property var modelData

                            readonly property int optIdx: modelData
                            readonly property string optGroup: root.paramsOptionModel
                                ? root.paramsOptionModel.optGroup(optIdx) : ""
                            readonly property bool showGroupHeader: {
                                if (paramsDelegate.index === 0) return optGroup !== ""
                                var prevGroup = root.paramsOptionModel
                                    ? root.paramsOptionModel.optGroup(root.paramsFilteredIndices[paramsDelegate.index - 1]) : ""
                                return optGroup !== "" && optGroup !== prevGroup
                            }

                            width: paramsList.width
                            height: optRow.totalHeight

                            OptionRow {
                                id: optRow
                                anchors.left: parent.left
                                anchors.right: parent.right
                                optionModel: root.paramsOptionModel
                                optIdx: paramsDelegate.optIdx
                                rowIndex: paramsDelegate.index
                                searchText: root.paramsSearchText
                                showGroupHeader: paramsDelegate.showGroupHeader
                                oGroup: paramsDelegate.optGroup
                                compact: true
                                compactLabelWidth: 148
                                compactFieldWidth: 86
                                compactEnumWidth: 132
                                valueSource: {
                                    if (!root.configVm || !root.paramsOptionModel) return ""
                                    var key = root.paramsOptionModel.optKey(paramsDelegate.optIdx)
                                    return root.configVm.valueSourceForKey(key)
                                }
                            }
                        }
                    }
                }

                Component.onCompleted: root.rebuildParamsFilter()
            }

            Item { Layout.preferredHeight: 18 }
        }
    }

    // ── v5.16 (PSET2-07): preset rename/delete affordance ────────────────
    // Upstream exposes rename/delete on each preset combo's context menu;
    // here each preset row's "⋮" button opens the same actions.
    property int presetEditCategory: -1
    property string presetEditName: ""

    function editPreset(category, currentName) {
        if (!root.configVm || !currentName || currentName.length === 0)
            return
        root.presetEditCategory = category
        root.presetEditName = currentName
        presetEditMenu.popup()
    }

    CxMenu {
        id: presetEditMenu

        CxMenuItem {
            text: qsTr("重命名…")
            onTriggered: {
                renamePresetField.text = root.presetEditName
                renamePresetError.visible = false
                renamePresetDialog.open()
            }
        }
        CxMenuItem {
            text: qsTr("删除…")
            onTriggered: {
                // canDeletePreset guards read-only/built-ins (service-side
                // delete would reject them too); in-use presets warn first.
                if (!root.configVm)
                    return
                if (!root.configVm.canDeletePreset(root.presetEditName)) {
                    backend.postError(root.configVm.presetActionBlocker(
                        root.presetEditCategory, root.presetEditName, "delete"), 1)
                    return
                }
                deleteConfirmInUse.visible = root.configVm.isPresetInUse(root.presetEditName)
                deletePresetDialog.open()
            }
        }
    }

    // Inline rename dialog (upstream SavePresetDialog rename path).
    CxDialog {
        id: renamePresetDialog
        modal: true
        dialogTitle: qsTr("重命名预设")
        width: 380
        height: 160
        padding: 0

        onAccepted: {
            if (!root.configVm)
                return
            const newName = renamePresetField.text.trim()
            if (newName.length === 0 || newName === root.presetEditName)
                return
            if (!root.configVm.renamePreset(root.presetEditCategory, root.presetEditName, newName)) {
                renamePresetError.visible = true
                renamePresetDialog.open()
            } else {
                root.presetEditName = newName
            }
        }

        contentItem: Rectangle {
            color: Theme.bgPanel
            anchors.fill: parent

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingXL
                spacing: Theme.spacingMD

                Text {
                    text: root.presetEditName
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                CxTextField {
                    id: renamePresetField
                    Layout.fillWidth: true
                    implicitHeight: 28
                    font.pixelSize: Theme.fontSizeSM
                    placeholderText: qsTr("输入新的预设名称")
                    onAccepted: renamePresetDialog.accept()
                }

                Text {
                    id: renamePresetError
                    text: qsTr("重命名失败（名称为空、重复或内置预设）")
                    color: Theme.statusError
                    font.pixelSize: Theme.fontSizeXS
                    visible: false
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: Theme.spacingMD
                    CxButton {
                        text: qsTr("取消")
                        onClicked: renamePresetDialog.reject()
                    }
                    CxButton {
                        text: qsTr("确定")
                        cxStyle: CxButton.Style.Primary
                        onClicked: renamePresetDialog.accept()
                    }
                }
            }
        }
    }

    // Delete confirmation (upstream deletes the selected user preset and
    // falls the selection back to the category default).
    CxDialog {
        id: deletePresetDialog
        modal: true
        dialogTitle: qsTr("删除预设")
        width: 380
        height: 170
        padding: 0

        onAccepted: {
            if (root.configVm)
                root.configVm.deletePreset(root.presetEditCategory, root.presetEditName)
        }

        contentItem: Rectangle {
            color: Theme.bgPanel
            anchors.fill: parent

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingXL
                spacing: Theme.spacingMD

                Text {
                    Layout.fillWidth: true
                    text: qsTr("确定删除预设 “%1”？").arg(root.presetEditName)
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    wrapMode: Text.WordWrap
                }

                Text {
                    id: deleteConfirmInUse
                    Layout.fillWidth: true
                    text: qsTr("该预设正在使用，删除后将切换回默认预设。")
                    color: Theme.statusWarning
                    font.pixelSize: Theme.fontSizeXS
                    wrapMode: Text.WordWrap
                    visible: false
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: Theme.spacingMD
                    CxButton {
                        text: qsTr("取消")
                        onClicked: deletePresetDialog.reject()
                    }
                    CxButton {
                        text: qsTr("删除")
                        cxStyle: CxButton.Style.Primary
                        onClicked: deletePresetDialog.accept()
                    }
                }
            }
        }
    }

    function bedTypeName(index) {
        var names = [qsTr("PEI"), qsTr("EP"), qsTr("PC"), qsTr("纹理 PEI"), qsTr("自定义")]
        if (index >= 0 && index < names.length)
            return names[index]
        return names[0]
    }

    function filamentColor(index) {
        // v5.16 (CIRC-04): configured filament colours (same source as the
        // MMU paint palette) instead of a hardcoded theme palette.
        if (root.editorVm && root.editorVm.extrudersColors
            && index < root.editorVm.extrudersColors.length)
            return root.editorVm.extrudersColors[index]
        var colors = [Theme.statusWarning, Theme.textSecondary, Theme.textSecondary, "#214bc2", Theme.chromeDangerHover]
        return index < colors.length ? colors[index] : Theme.textSecondary
    }

    component PixelHeader: Item {
        id: headerRoot
        property string title: ""
        property string iconText: ""
        property url iconSource: ""
        property url actionIcon: ""
        property string actionToolTip: ""
        signal actionTriggered()

        implicitHeight: 24

        RowLayout {
            anchors.fill: parent
            spacing: 6

            Rectangle {
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18
                radius: 4
                color: root.controlSurface
                border.width: 1
                border.color: root.dividerColor

                Image {
                    visible: headerRoot.iconSource !== ""
                    anchors.centerIn: parent
                    width: 12
                    height: 12
                    source: headerRoot.iconSource
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.75
                }

                Text {
                    visible: headerRoot.iconSource === ""
                    anchors.centerIn: parent
                    text: headerRoot.iconText
                    color: Theme.accent
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                }
            }

            Text {
                Layout.fillWidth: true
                text: headerRoot.title
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                font.bold: true
                elide: Text.ElideRight
            }

            PixelIconButton {
                visible: headerRoot.actionIcon !== ""
                iconSource: headerRoot.actionIcon
                toolTipText: headerRoot.actionToolTip
                enabled: headerRoot.enabled
                onClicked: headerRoot.actionTriggered()
            }
        }
    }

    component PixelIconButton: Rectangle {
        id: iconButtonRoot
        property url iconSource: ""
        property string toolTipText: ""
        signal clicked()

        Layout.preferredWidth: 24
        Layout.preferredHeight: 24
        implicitWidth: 24
        implicitHeight: 24
        radius: 4
        color: !enabled ? root.fieldSurface
              : iconMA.containsMouse ? Theme.bgPressed
              : root.controlSurface
        border.width: 1
        border.color: root.dividerColor
        opacity: enabled ? 1.0 : 0.45

        Image {
            anchors.centerIn: parent
            width: 13
            height: 13
            source: iconButtonRoot.iconSource
            fillMode: Image.PreserveAspectFit
            opacity: 0.8
        }

        MouseArea {
            id: iconMA
            anchors.fill: parent
            hoverEnabled: true
            enabled: iconButtonRoot.enabled
            cursorShape: Qt.PointingHandCursor
            onClicked: iconButtonRoot.clicked()
        }

        ToolTip.visible: iconMA.containsMouse && iconButtonRoot.toolTipText.length > 0
        ToolTip.text: iconButtonRoot.toolTipText
        ToolTip.delay: 400
    }

    component PixelSegment: Rectangle {
        id: segmentRoot
        property string text: ""
        property bool selected: false
        signal clicked()

        Layout.preferredHeight: 28
        radius: 4
        color: segmentRoot.selected ? Theme.accent : root.sectionSurface
        border.width: 1
        border.color: segmentRoot.selected ? Theme.accent : root.dividerColor
        opacity: enabled ? 1.0 : 0.45

        Text {
            anchors.centerIn: parent
            text: segmentRoot.text
            color: segmentRoot.selected ? Theme.textOnAccent : root.mutedText
            font.pixelSize: Theme.fontSizeSM
            font.bold: segmentRoot.selected
        }

        MouseArea {
            anchors.fill: parent
            enabled: segmentRoot.enabled
            cursorShape: Qt.PointingHandCursor
            onClicked: segmentRoot.clicked()
        }
    }
}
