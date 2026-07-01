import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../controls"

// ParamsPage — reusable parameter page component driven by optionModel and categories.
// Used by SettingsPage for all 3 preset tiers (print / filament / printer).
Item {
    id: root

    property var optionModel: null
    property var configVm: null
    property var categories: []

    property string selectedCategory: categories.length > 0 ? categories[0] : qsTr("全部")
    property string searchText: ""
    property var filteredIndices: []

    Component.onCompleted: rebuildFilter()

    Connections {
        target: root.optionModel
        function onDataVersionChanged() { root.rebuildFilter() }
    }

    function rebuildFilter() {
        if (!root.configVm) { filteredIndices = []; return }
        filteredIndices = root.configVm.filterOptionIndices(root.selectedCategory, root.searchText)
    }

    onSelectedCategoryChanged: rebuildFilter()
    onSearchTextChanged: rebuildFilter()

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── Category sidebar ──
        Rectangle {
            Layout.preferredWidth: 200
            Layout.fillHeight: true
            color: Theme.bgPanel

            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: Theme.spacingLG
                spacing: 0

                Text {
                    Layout.leftMargin: 16
                    text: qsTr("参数分类")
                    color: Theme.accent
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                }

                Item { Layout.preferredHeight: Theme.spacingLG }

                Repeater {
                    model: [qsTr("全部")].concat(root.categories)
                    delegate: Rectangle {
                        required property string modelData
                        required property int index
                        Layout.fillWidth: true
                        Layout.leftMargin: Theme.spacingSM
                        Layout.rightMargin: Theme.spacingSM
                        height: 34
                        radius: 4
                        color: root.selectedCategory === modelData
                               ? "#1c2a3e"
                               : catHov.containsMouse ? "#161d28" : "transparent"
                        border.color: root.selectedCategory === modelData ? Theme.accent : "transparent"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: Theme.spacingSM
                            spacing: 0
                            Text {
                                text: modelData
                                color: root.selectedCategory === modelData ? Theme.accent : Theme.textSecondary
                                font.pixelSize: Theme.fontSizeMD
                            }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                readonly property int cnt: root.configVm && modelData !== qsTr("全部")
                                    ? root.optionModel.countForCategory(modelData)
                                    : (root.optionModel ? root.optionModel.count : 0)
                                visible: cnt > 0
                                width: cnt > 9 ? 26 : 20; height: 16; radius: 3
                                color: root.selectedCategory === modelData ? "#1e3828" : "#1e2535"
                                Text {
                                    anchors.centerIn: parent
                                    text: parent.cnt
                                    color: root.selectedCategory === parent.parent.parent.modelData ? Theme.accent : Theme.textDisabled
                                    font.pixelSize: Theme.fontSizeXS
                                }
                            }
                        }

                        HoverHandler { id: catHov }
                        TapHandler {
                            onTapped: root.selectedCategory = modelData
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        // ── Parameter list ──
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.bgBase

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Search bar
                Rectangle {
                    Layout.fillWidth: true
                    height: 44
                    color: Theme.bgSurface

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingLG
                        anchors.rightMargin: Theme.spacingLG
                        spacing: Theme.spacingLG

                        Text {
                            text: root.selectedCategory + " ·  " + root.filteredIndices.length + qsTr(" 项")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeLG
                            font.bold: true
                        }
                        Item { Layout.fillWidth: true }

                        Rectangle {
                            height: 28
                            width: resetBtnText.implicitWidth + 20
                            radius: 4
                            color: resetHov.containsMouse ? "#3e2535" : "#2e2030"
                            border.color: "#5e3040"
                            Text {
                                id: resetBtnText
                                anchors.centerIn: parent
                                text: qsTr("重置默认")
                                color: resetHov.containsMouse ? "#e8a0b0" : Theme.textSecondary
                                font.pixelSize: Theme.fontSizeSM
                            }
                            HoverHandler { id: resetHov }
                            TapHandler {
                                onTapped: {
                                    if (root.configVm)
                                        root.configVm.resetAllGlobalOptions()
                                }
                            }
                        }
                        CxTextField {
                            id: searchField
                            placeholderText: qsTr("搜索参数名...")
                            implicitWidth: 200
                            onTextChanged: root.searchText = text
                        }
                        Label {
                            visible: root.optionModel && root.optionModel.dirtyCount > 0
                            text: root.optionModel ? (root.optionModel.dirtyCount + qsTr(" 已修改")) : ""
                            color: Theme.statusWarning
                            font.pixelSize: Theme.fontSizeXS
                            font.bold: true
                        }
                        Label {
                            visible: root.searchText !== "" && root.filteredIndices.length >= 0
                            text: root.filteredIndices.length + qsTr(" 项匹配")
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeXS
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSubtle }

                // Parameter list view
                ListView {
                    id: paramList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: root.filteredIndices
                    spacing: 0

                    ScrollBar.vertical: ScrollBar { visible: paramList.contentHeight > paramList.height }

                    Text {
                        anchors.centerIn: parent
                        visible: root.filteredIndices.length === 0
                        text: qsTr("无匹配参数")
                        color: Theme.textDisabled; font.pixelSize: 13
                    }

                    delegate: Item {
                        id: paramDelegate
                        required property int index
                        required property var modelData

                        readonly property int optIdx: modelData
                        readonly property string oGroup: root.optionModel ? root.optionModel.optGroup(optIdx) : ""
                        readonly property bool showGroupHeader: oGroup !== "" && (
                            index === 0
                            || (root.optionModel && root.optionModel.optGroup(root.filteredIndices[index - 1]) !== oGroup)
                        )
                        readonly property int contentHeight: paramRow.oType === "double" || paramRow.oType === "int" ? 56
                                            : paramRow.oType === "string" ? 80 : 44

                        width: paramList.width
                        height: (showGroupHeader ? 28 : 0) + contentHeight

                        // Group header
                        Rectangle {
                            visible: paramDelegate.showGroupHeader
                            anchors.top: parent.top
                            width: parent.width
                            height: 28
                            color: Theme.bgSurface

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 20
                                anchors.verticalCenter: parent.verticalCenter
                                text: paramDelegate.oGroup
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSizeSM
                                font.bold: true
                            }

                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: 1
                                color: Theme.borderSubtle
                            }
                        }

                        // Parameter row
                        Rectangle {
                            id: paramRow
                            y: paramDelegate.showGroupHeader ? 28 : 0
                            width: parent.width
                            height: paramDelegate.contentHeight
                            color: (paramDelegate.index + (paramDelegate.showGroupHeader ? 1 : 0)) % 2 === 0 ? "transparent" : "#080b10"

                            readonly property string oType:  root.optionModel ? root.optionModel.optType(optIdx)     : ""
                            readonly property string oLabel: root.optionModel ? root.optionModel.optLabel(optIdx)    : ""
                            readonly property var    oVal:   root.optionModel ? root.optionModel.optValue(optIdx)    : 0
                            readonly property double oMin:   root.optionModel ? root.optionModel.optMin(optIdx)      : 0
                            readonly property double oMax:   root.optionModel ? root.optionModel.optMax(optIdx)      : 1
                            readonly property double oStep:  root.optionModel ? root.optionModel.optStep(optIdx)     : 1
                            readonly property bool   oRO:    root.optionModel ? root.optionModel.optReadonly(optIdx) : false
                            readonly property bool   oDirty: root.optionModel ? root.optionModel.optIsDirty(optIdx) : false
                            readonly property string oTip:   root.optionModel ? root.optionModel.optTooltip(optIdx) : ""

                            ToolTip.visible: oTip !== "" && tipMA.containsMouse
                            ToolTip.text: oTip
                            ToolTip.delay: 500

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 20
                                anchors.rightMargin: Theme.fontSizeLG
                                spacing: Theme.spacingLG

                                // Label
                                ColumnLayout {
                                    Layout.preferredWidth: 180
                                    spacing: 2
                                    RowLayout {
                                        spacing: Theme.spacingXS
                                        Rectangle {
                                            visible: paramRow.oDirty
                                            width: 6; height: 6; radius: 3
                                            color: Theme.statusWarning
                                            Layout.alignment: Qt.AlignVCenter
                                        }
                                        Text {
                                            text: paramRow.oLabel
                                            color: paramRow.oRO ? Theme.textDisabled : (paramRow.oDirty ? Theme.statusWarning : Theme.textSecondary)
                                            font.pixelSize: Theme.fontSizeMD
                                            elide: Text.ElideRight
                                            font.bold: paramRow.oDirty || root.searchText !== ""
                                            Layout.fillWidth: true
                                        }
                                    }
                                    Text {
                                        visible: paramRow.oRO
                                        text: qsTr("只读")
                                        color: "#3a4555"
                                        font.pixelSize: 9
                                    }
                                }

                                // Controls
                                Item {
                                    Layout.fillWidth: true
                                    height: paramRow.height - Theme.spacingSM

                                    // Slider (double/int)
                                    RowLayout {
                                        visible: paramRow.oType === "double" || paramRow.oType === "int"
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: parent.width
                                        spacing: Theme.spacingSM
                                        CxSlider {
                                            Layout.fillWidth: true
                                            from: paramRow.oMin; to: paramRow.oMax; stepSize: paramRow.oStep
                                            value: typeof paramRow.oVal === "number" ? paramRow.oVal : paramRow.oMin
                                            enabled: !paramRow.oRO
                                            onMoved: root.optionModel.setValue(paramRow.optIdx, value)
                                        }
                                        // 可编辑数值输入（对齐上游 SpinCtrl；Sidebar.qml 变换输入同款模式）
                                        // 此前为只读 Text——仅滑块可改值，无法直接键入（"在参数表中编辑隐藏"）。
                                        Rectangle {
                                            Layout.preferredWidth: 64
                                            height: 24
                                            radius: 4
                                            color: Theme.bgInset
                                            border.width: 1
                                            border.color: paramEdit.activeFocus ? Theme.accent
                                                          : (paramRow.oRO ? Theme.borderSubtle : Theme.borderInput)
                                            opacity: paramRow.oRO ? 0.6 : 1.0

                                            TextInput {
                                                id: paramEdit
                                                anchors.fill: parent
                                                anchors.leftMargin: 6
                                                anchors.rightMargin: 6
                                                verticalAlignment: Text.AlignVCenter
                                                horizontalAlignment: Text.AlignRight
                                                color: Theme.textPrimary
                                                font.pixelSize: Theme.fontSizeSM
                                                font.bold: true
                                                selectByMouse: true
                                                readOnly: paramRow.oRO
                                                validator: DoubleValidator {
                                                    decimals: paramRow.oType === "double" ? 3 : 0
                                                    notation: DoubleValidator.StandardNotation
                                                }
                                                text: typeof paramRow.oVal === "number"
                                                      ? Number(paramRow.oVal).toFixed(paramRow.oType === "double" ? 2 : 0)
                                                      : ""
                                                onEditingFinished: {
                                                    if (!root.optionModel || paramRow.oRO) return
                                                    var v = parseFloat(text)
                                                    if (isNaN(v)) return
                                                    if (v < paramRow.oMin) v = paramRow.oMin
                                                    if (v > paramRow.oMax) v = paramRow.oMax
                                                    root.optionModel.setValue(paramRow.optIdx, v)
                                                }
                                            }
                                        }
                                    }

                                    // Switch (bool)
                                    CxSwitch {
                                        visible: paramRow.oType === "bool"
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.right: parent.right
                                        checked: paramRow.oVal === true || paramRow.oVal === "true"
                                        enabled: !paramRow.oRO
                                        onToggled: root.optionModel.setValue(paramRow.optIdx, checked)
                                    }

                                    // ComboBox (enum)
                                    CxComboBox {
                                        visible: paramRow.oType === "enum"
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.right: parent.right
                                        width: 160
                                        enabled: !paramRow.oRO
                                        currentIndex: typeof paramRow.oVal === "number" ? paramRow.oVal : 0
                                        model: {
                                            if (!root.optionModel || paramRow.oType !== "enum") return []
                                            var arr = []
                                            var cnt = root.optionModel.optEnumCount(paramRow.optIdx)
                                            for (var j = 0; j < cnt; ++j)
                                                arr.push(root.optionModel.optEnumLabel(paramRow.optIdx, j))
                                            return arr
                                        }
                                        onActivated: (i) => root.optionModel.setValue(paramRow.optIdx, i)
                                    }

                                    // TextArea (string — G-code, notes)
                                    ScrollView {
                                        visible: paramRow.oType === "string"
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        height: 60
                                        TextArea {
                                            text: typeof paramRow.oVal === "string" ? paramRow.oVal : (paramRow.oVal ? paramRow.oVal.toString() : "")
                                            font.pixelSize: Theme.fontSizeSM
                                            font.family: "Consolas"
                                            color: Theme.textSecondary
                                            readOnly: paramRow.oRO
                                            wrapMode: TextArea.Wrap
                                            background: Rectangle {
                                                radius: 4; color: Theme.bgInset
                                                border.color: parent.activeFocus ? Theme.accent : Theme.borderInput
                                            }
                                            onTextChanged: {
                                                if (root.optionModel && activeFocus)
                                                    root.optionModel.setValue(paramRow.optIdx, text)
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Tooltip hover area
                        MouseArea {
                            id: tipMA
                            anchors.fill: paramRow
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                            z: -1
                        }
                    }
                }
            }
        }
    }
}
