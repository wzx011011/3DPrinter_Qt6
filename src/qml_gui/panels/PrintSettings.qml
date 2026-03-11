import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"
import "../components"

// G4 — 打印设置面板（动态 ConfigOptionModel，ListView + Q_INVOKABLE 访问器）
Item {
    id: root
    required property var configVm
    required property var editorVm

    property bool advancedEnabled: false
    property string searchText: ""
    property var filteredIndices: []
    readonly property var visibleCategories: [qsTr("质量"), qsTr("接缝"), qsTr("精度"), qsTr("墙壁和表面"), qsTr("填充"), qsTr("支撑"), qsTr("底座"), qsTr("速度"), qsTr("温度"), qsTr("其他")]
    readonly property string selectedScope: root.configVm ? root.configVm.settingsScope : "global"
    readonly property string settingsTargetType: root.configVm ? root.configVm.settingsTargetType : ""
    readonly property string settingsTargetName: root.configVm ? root.configVm.settingsTargetName : ""
    readonly property int settingsTargetObjectIndex: root.configVm ? root.configVm.settingsTargetObjectIndex : -1
    readonly property int settingsTargetVolumeIndex: root.configVm ? root.configVm.settingsTargetVolumeIndex : -1

    function scopeTitle() {
        if (root.selectedScope !== "object")
            return qsTr("全局打印参数")
        return root.settingsTargetType === "volume" ? qsTr("部件打印参数") : qsTr("对象打印参数")
    }

    function scopeSubtitle() {
        if (root.selectedScope !== "object" || root.settingsTargetName.length === 0)
            return qsTr("当前预设作用于整个工程")
        return (root.settingsTargetType === "volume"
                ? qsTr("当前目标：部件 · %1")
                : qsTr("当前目标：对象 · %1")).arg(root.settingsTargetName)
    }

    function rebuildFilter() {
        if (!root.configVm || !root.configVm.printOptions) {
            filteredIndices = []
            return
        }
        var opts = root.configVm.printOptions
        var needle = root.searchText.toLowerCase()
        var result = []
        for (var i = 0; i < opts.count; ++i) {
            var matchText = needle === ""
                || opts.optLabel(i).toLowerCase().indexOf(needle) >= 0
                || opts.optKey(i).toLowerCase().indexOf(needle) >= 0
            if (matchText)
                result.push(i)
        }
        filteredIndices = result
    }

    function materialPresetAt(localIndex) {
        if (!root.configVm || !root.configVm.presetList)
            return ""
        var globalIndex = root.configVm.presetList.globalIndex(qsTr("耗材丝"), localIndex)
        return globalIndex >= 0 ? root.configVm.presetList.presetName(globalIndex) : ""
    }

    function groupedIndices(category) {
        var result = []
        if (!root.configVm || !root.configVm.printOptions)
            return result
        var opts = root.configVm.printOptions
        for (var optIdx of filteredIndices) {
            if (opts.optCategory(optIdx) === category)
                result.push(optIdx)
        }
        return result
    }

    function enumOptionLabels(opts, optIdx, optType) {
        if (!opts || optType !== "enum")
            return []
        var values = []
        var count = opts.optEnumCount(optIdx)
        for (var i = 0; i < count; ++i)
            values.push(opts.optEnumLabel(optIdx, i))
        return values
    }

    Component.onCompleted: rebuildFilter()
    onSearchTextChanged: rebuildFilter()

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        CxPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 108
            cxSurface: CxPanel.Surface.Panel
            radius: 14

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: qsTr("耗材丝")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeLG
                        font.bold: true
                    }
                    Item { Layout.fillWidth: true }
                    Text { text: "◔◔"; color: Theme.accent; font.pixelSize: 11 }
                }

                GridLayout {
                    columns: 3
                    rowSpacing: 8
                    columnSpacing: 8

                    Repeater {
                        model: 5
                        delegate: Rectangle {
                            required property int index
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44
                            radius: 8
                            color: index === 0 ? "#b97914" : index === 3 ? "#214bc2" : index === 4 ? "#d63a21" : "#b9b9b9"
                            border.width: 1
                            border.color: index === 0 || index === 3 || index === 4 ? "transparent" : "#a8a8a8"

                            Column {
                                anchors.centerIn: parent
                                spacing: 1

                                Text {
                                    text: (index + 1).toString()
                                    color: "#101114"
                                    font.pixelSize: 12
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    width: 70
                                }
                                Text {
                                    text: root.materialPresetAt(index)
                                    color: "#101114"
                                    font.pixelSize: 11
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    width: 70
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }
        }

        CxPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 84
            cxSurface: CxPanel.Surface.Panel
            radius: 12

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                anchors.topMargin: 10
                anchors.bottomMargin: 10
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text {
                        text: root.scopeTitle()
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeLG
                        font.bold: true
                    }

                    Rectangle {
                        width: 38
                        height: 22
                        radius: 11
                        color: root.selectedScope === "global" ? Theme.accent : "transparent"
                        border.width: 1
                        border.color: root.selectedScope === "global" ? "transparent" : Theme.borderDefault

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("全局")
                            color: root.selectedScope === "global" ? Theme.textOnAccent : Theme.textSecondary
                            font.pixelSize: 10
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (root.configVm)
                                    root.configVm.activateGlobalScope()
                            }
                        }
                    }

                    Rectangle {
                        width: 38
                        height: 22
                        radius: 11
                        enabled: root.settingsTargetName.length > 0
                        color: root.selectedScope === "object" ? Theme.bgElevated : "transparent"
                        border.width: 1
                        border.color: root.selectedScope === "object" ? Theme.borderDefault : "transparent"
                        opacity: enabled ? 1.0 : 0.45

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("对象")
                            color: root.selectedScope === "object" ? Theme.textPrimary : Theme.textSecondary
                            font.pixelSize: 10
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: parent.enabled
                            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: {
                                if (root.configVm && root.settingsTargetName.length > 0)
                                    root.configVm.activateObjectScope(root.settingsTargetType,
                                                                      root.settingsTargetName,
                                                                      root.settingsTargetObjectIndex,
                                                                      root.settingsTargetVolumeIndex)
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: qsTr("高级")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMD
                        font.bold: true
                    }

                    Switch {
                        checked: root.advancedEnabled
                        onToggled: root.advancedEnabled = checked
                    }

                    Text { text: "◎"; color: Theme.textSecondary; font.pixelSize: 12 }
                    Text { text: "⌘"; color: Theme.textSecondary; font.pixelSize: 12 }
                }

                Text {
                    Layout.fillWidth: true
                    text: root.scopeSubtitle()
                    color: Theme.textDisabled
                    font.pixelSize: Theme.fontSizeSM
                    elide: Text.ElideRight
                }
            }
        }

        CxPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 42
            cxSurface: CxPanel.Surface.Panel
            radius: 12

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 6

                CxComboBox {
                    id: presetCombo
                    Layout.fillWidth: true
                    model: {
                        if (!root.configVm || !root.configVm.presetList) return []
                        var pl = root.configVm.presetList
                        var n = pl.countByCategory(qsTr("打印质量"))
                        var arr = []
                        for (var i = 0; i < n; ++i)
                            arr.push(pl.presetName(pl.globalIndex(qsTr("打印质量"), i)))
                        return arr
                    }
                    currentIndex: model.indexOf(root.configVm ? root.configVm.currentPreset : "")
                    onActivated: (index) => {
                        if (root.configVm && index >= 0)
                            root.configVm.setCurrentPreset(model[index])
                    }
                }

                CxIconButton {
                    buttonSize: 28
                    iconSize: 14
                    iconSource: "qrc:/qml/assets/icons/device-floppy.svg"
                    toolTipText: qsTr("保存预设")
                }

                CxIconButton {
                    buttonSize: 28
                    iconSize: 14
                    iconSource: "qrc:/qml/assets/icons/dots.svg"
                    toolTipText: qsTr("搜索")
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Column {
                width: parent.width
                spacing: 8

                Repeater {
                    model: root.visibleCategories

                    delegate: CxPanel {
                        required property string modelData
                        readonly property var sectionIndices: root.groupedIndices(modelData)

                        visible: sectionIndices.length > 0
                        width: parent.width
                        cxSurface: CxPanel.Surface.Transparent
                        border.width: 1
                        border.color: Theme.accent
                        radius: 8

                        Column {
                            width: parent.width
                            spacing: 0

                            Rectangle {
                                width: parent.width
                                height: 34
                                color: "transparent"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 8

                                    Text {
                                        text: modelData
                                        color: Theme.textPrimary
                                        font.pixelSize: Theme.fontSizeLG
                                        font.bold: true
                                    }
                                    Item { Layout.fillWidth: true }
                                }
                            }

                            Repeater {
                                model: parent.parent.sectionIndices

                                delegate: Rectangle {
                                    required property int modelData
                                    readonly property int optIdx: modelData
                                    readonly property var opts: root.configVm ? root.configVm.printOptions : null
                                    readonly property string oType: opts ? opts.optType(optIdx) : ""
                                    readonly property string oLabel: opts ? opts.optLabel(optIdx) : ""
                                    readonly property var oVal: opts ? opts.optValue(optIdx) : 0
                                    readonly property double oMin: opts ? opts.optMin(optIdx) : 0
                                    readonly property double oMax: opts ? opts.optMax(optIdx) : 1
                                    readonly property double oStep: opts ? opts.optStep(optIdx) : 1
                                    readonly property bool oRO: opts ? opts.optReadonly(optIdx) : false

                                    width: parent.width
                                    height: (oType === "double" || oType === "int") ? 44 : 38
                                    color: "transparent"

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 10
                                        anchors.rightMargin: 10
                                        spacing: 8

                                        Text {
                                            Layout.fillWidth: true
                                            text: oLabel
                                            color: oRO ? Theme.textDisabled : Theme.textSecondary
                                            font.pixelSize: Theme.fontSizeLG
                                            elide: Text.ElideRight
                                        }

                                        CxSlider {
                                            visible: oType === "double" || oType === "int"
                                            Layout.preferredWidth: 100
                                            from: oMin
                                            to: oMax
                                            stepSize: oStep
                                            value: typeof oVal === "number" ? oVal : oMin
                                            enabled: !oRO
                                            onMoved: {
                                                if (opts)
                                                    opts.setValue(optIdx, value)
                                            }
                                        }

                                        Rectangle {
                                            visible: oType === "double" || oType === "int"
                                            width: 58
                                            height: 28
                                            radius: 8
                                            color: Theme.bgElevated
                                            border.width: 1
                                            border.color: Theme.borderSubtle

                                            Text {
                                                anchors.centerIn: parent
                                                text: typeof oVal === "number" ? Number(oVal).toFixed(oType === "double" ? 1 : 0) : "-"
                                                color: Theme.textPrimary
                                                font.pixelSize: Theme.fontSizeMD
                                                font.bold: true
                                            }
                                        }

                                        Text {
                                            visible: oType === "double" || oType === "int"
                                            text: "mm"
                                            color: Theme.textDisabled
                                            font.pixelSize: Theme.fontSizeSM
                                        }

                                        CxCheckBox {
                                            visible: oType === "bool"
                                            checked: oVal === true || oVal === "true"
                                            enabled: !oRO
                                            text: ""
                                            onToggled: {
                                                if (opts)
                                                    opts.setValue(optIdx, checked)
                                            }
                                        }

                                        CxComboBox {
                                            visible: oType === "enum"
                                            Layout.preferredWidth: 110
                                            enabled: !oRO
                                            currentIndex: typeof oVal === "number" ? oVal : 0
                                            model: root.enumOptionLabels(opts, optIdx, oType)
                                            onActivated: {
                                                if (opts)
                                                    opts.setValue(optIdx, currentIndex)
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
    }
}
