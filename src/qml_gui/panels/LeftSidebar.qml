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

    readonly property int targetSidebarWidth: 392
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
    property bool printerExpanded: true

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

            Rectangle {
                id: printerHeroCard
                Layout.fillWidth: true
                Layout.preferredHeight: 76
                radius: 4
                color: root.sectionSurface
                border.width: 1
                border.color: root.dividerColor

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    Rectangle {
                        id: printerThumbnail
                        Layout.preferredWidth: 52
                        Layout.preferredHeight: 52
                        radius: 4
                        color: Theme.chromeText
                        border.width: 1
                        border.color: Theme.textSecondary

                        Image {
                            anchors.centerIn: parent
                            width: 28
                            height: 28
                            source: "qrc:/qml/assets/icons/printer.svg"
                            fillMode: Image.PreserveAspectFit
                            opacity: 0.75
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 5

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Text {
                                Layout.fillWidth: true
                                text: root.configVm && root.configVm.currentPrinterPreset !== ""
                                      ? root.configVm.currentPrinterPreset
                                      : qsTr("Creality K2 Plus")
                                color: Theme.textPrimary
                                font.pixelSize: 12
                                font.bold: true
                                elide: Text.ElideRight
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
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 5

                            InfoTile {
                                id: nozzleTile
                                Layout.fillWidth: true
                                label: qsTr("喷嘴")
                                value: "0.4 mm"
                            }
                            InfoTile {
                                id: buildPlateTile
                                Layout.fillWidth: true
                                label: qsTr("热床")
                                value: root.editorVm ? root.bedTypeName(root.editorVm.plateBedType(root.editorVm.currentPlateIndex)) : "PEI"
                            }
                            InfoTile {
                                id: heaterTile
                                Layout.fillWidth: true
                                label: qsTr("温度")
                                value: root.configVm ? (root.configVm.nozzleTemp + " / " + root.configVm.bedTemp) : "220 / 65"
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.preferredWidth: 28
                        spacing: 6
                        Layout.alignment: Qt.AlignTop

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
                    border.color: root.configVm && !root.configVm.isFilamentCompatible(root.configVm.materialPresetName(filamentPixelRow.index)) ? Theme.statusError : root.dividerColor

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
                            font.pixelSize: 12
                            font.bold: true
                        }

                        CxComboBox {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 28
                            font.pixelSize: 11
                            model: root.configVm ? root.configVm.filamentPresetNames : []
                            currentIndex: {
                                if (!root.configVm) return -1
                                var names = root.configVm.filamentPresetNames
                                for (var i = 0; i < names.length; i++) {
                                    if (names[i] === root.configVm.materialPresetName(filamentPixelRow.index)) return i
                                }
                                return -1
                            }
                            onActivated: (i) => {
                                if (!root.configVm) return
                                var names = root.configVm.filamentPresetNames
                                if (i >= 0 && i < names.length)
                                    root.configVm.requestCurrentFilamentPreset(names[i])
                            }
                        }

                        PixelIconButton {
                            iconSource: "qrc:/qml/assets/icons/settings.svg"
                            toolTipText: qsTr("编辑耗材预设")
                            onClicked: backend.forwardSettingsRequest("filament")
                        }

                        Rectangle {
                            visible: !!root.configVm && !root.configVm.isFilamentCompatible(root.configVm.materialPresetName(filamentPixelRow.index))
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
                        font.pixelSize: 11
                        model: root.configVm ? root.configVm.printPresetNames : []
                        currentIndex: {
                            if (!root.configVm) return -1
                            return root.configVm.printPresetNames.indexOf(root.configVm.currentPrintPreset)
                        }
                        onActivated: (i) => {
                            if (root.configVm && i >= 0)
                                root.configVm.requestCurrentPrintPreset(model[i])
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
                        font.pixelSize: 11
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
                                    font.pixelSize: 10
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

    function bedTypeName(index) {
        var names = [qsTr("PEI"), qsTr("EP"), qsTr("PC"), qsTr("纹理 PEI"), qsTr("自定义")]
        if (index >= 0 && index < names.length)
            return names[index]
        return names[0]
    }

    function filamentColor(index) {
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
                    font.pixelSize: 11
                    font.bold: true
                }
            }

            Text {
                Layout.fillWidth: true
                text: headerRoot.title
                color: Theme.textPrimary
                font.pixelSize: 12
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

    component InfoTile: Rectangle {
        id: tileRoot
        property string label: ""
        property string value: ""

        implicitHeight: 30
        radius: 3
        color: root.fieldSurface
        border.width: 1
        border.color: root.dividerColor

        Column {
            anchors.fill: parent
            anchors.leftMargin: 6
            anchors.rightMargin: 6
            anchors.topMargin: 3
            spacing: 0

            Text {
                width: parent.width
                text: tileRoot.label
                color: root.mutedText
                font.pixelSize: 10
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                text: tileRoot.value
                color: Theme.textPrimary
                font.pixelSize: 10
                font.bold: true
                elide: Text.ElideRight
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
            font.pixelSize: 11
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
