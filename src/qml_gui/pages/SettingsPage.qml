import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// H1 — SettingsPage：三栏参数编辑（分类树 / 参数列表 / 搜索栏）
// 绑定 ConfigViewModel.printOptions (ConfigOptionModel)
Item {
    id: root
    required property var configVm

    property string selectedCategory: qsTr("质量")
    property string searchText:       ""
    property var    filteredIndices:  []

    readonly property var allCategories: [qsTr("质量"),qsTr("填充"),qsTr("速度"),qsTr("加速度"),qsTr("温度"),qsTr("支撑"),qsTr("底座"),qsTr("冷却"),qsTr("回退"),qsTr("其他")]

    Component.onCompleted: rebuildFilter()

    Connections {
        target: root.configVm ? root.configVm.printOptions : null
        function onDataVersionChanged() { root.rebuildFilter() }
    }

    function rebuildFilter() {
        if (!root.configVm) { filteredIndices = []; return }
        filteredIndices = root.configVm.filterOptionIndices(root.selectedCategory, root.searchText)
    }

    onSelectedCategoryChanged: rebuildFilter()
    onSearchTextChanged:       rebuildFilter()

    Rectangle { anchors.fill: parent; color: "#0d0f12" }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── 左侧分类树 ──────────────────────────────────────────
        Rectangle {
            Layout.preferredWidth: 200
            Layout.fillHeight: true
            color: "#0f1218"

            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: 10
                spacing: 0

                Text {
                    Layout.leftMargin: 16
                    text: qsTr("参数分类")
                    color: "#22c564"
                    font.pixelSize: 11
                    font.bold: true
                }

                Item { Layout.preferredHeight: 8 }

                Repeater {
                    model: [qsTr("全部")].concat(root.allCategories)
                    delegate: Rectangle {
                        required property string modelData
                        required property int    index
                        Layout.fillWidth: true
                        Layout.leftMargin: 6
                        Layout.rightMargin: 6
                        height: 34
                        radius: 4
                        color: root.selectedCategory === modelData
                               ? "#1c2a3e"
                               : catHov.containsMouse ? "#161d28" : "transparent"
                        border.color: root.selectedCategory === modelData ? "#18c75e" : "transparent"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 8
                            spacing: 0
                            Text {
                                text: modelData
                                color: root.selectedCategory === modelData ? "#18c75e" : "#c8d4e0"
                                font.pixelSize: 12
                            }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                readonly property int cnt: root.configVm && modelData !== qsTr("全部")
                                    ? root.configVm.printOptions.countForCategory(modelData)
                                    : (root.configVm ? root.configVm.printOptions.count : 0)
                                visible: cnt > 0
                                width: cnt > 9 ? 26 : 20; height: 16; radius: 3
                                color: root.selectedCategory === modelData ? "#1e3828" : "#1e2535"
                                Text {
                                    anchors.centerIn: parent
                                    text: parent.cnt
                                    color: root.selectedCategory === parent.parent.parent.modelData ? "#22c564" : "#566070"
                                    font.pixelSize: 10
                                }
                            }
                        }

                        HoverHandler { id: catHov }
                        TapHandler {
                            onTapped: {
                                root.selectedCategory = modelData
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                // 返回准备页
                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 6; Layout.rightMargin: 6; Layout.bottomMargin: 10
                    height: 28; radius: 4
                    color: backHov.containsMouse ? "#2e3540" : "#1e2535"
                    Text { anchors.centerIn: parent; text: qsTr("← 返回"); color: "#9daaba"; font.pixelSize: 11 }
                    MouseArea { id: backHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: backend.setCurrentPage(1) }
                }
            }
        }

        // ── 中间参数列表 ────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#0d0f12"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // 顶部搜索栏
                Rectangle {
                    Layout.fillWidth: true
                    height: 44
                    color: "#131720"

                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 12

                        Text {
                            text: root.selectedCategory + " ·  " + root.filteredIndices.length + qsTr(" 项")
                            color: "#e2e8f5"; font.pixelSize: 14; font.bold: true
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
                                color: resetHov.containsMouse ? "#e8a0b0" : "#9daaba"
                                font.pixelSize: 11
                            }
                            HoverHandler { id: resetHov }
                            TapHandler {
                                onTapped: {
                                    if (root.configVm && root.configVm.printOptions)
                                        root.configVm.printOptions.resetToDefaults()
                                }
                            }
                        }
                        TextField {
                            id: searchField
                            placeholderText: qsTr("搜索参数名...")
                            implicitWidth: 200; implicitHeight: 28
                            font.pixelSize: 11
                            color: "#c8d4e0"
                            background: Rectangle {
                                radius: 4; color: "#0f1318"
                                border.color: searchField.activeFocus ? "#22c564" : "#2e3848"
                            }
                            onTextChanged: root.searchText = text
                        }
                        // 脏选项计数
                        Label {
                            visible: root.configVm && root.configVm.printOptions && root.configVm.printOptions.dirtyCount > 0
                            text: root.configVm ? (root.configVm.printOptions.dirtyCount + qsTr(" 已修改")) : ""
                            color: "#f0883e"
                            font.pixelSize: 10
                            font.bold: true
                        }
                        // 搜索结果计数
                        Label {
                            visible: root.searchText !== "" && root.filteredIndices.length >= 0
                            text: root.filteredIndices.length + qsTr(" 项匹配")
                            color: "#6b7d94"
                            font.pixelSize: 10
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#1e2430" }

                // 参数列表
                ListView {
                    id: paramList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: root.filteredIndices
                    spacing: 0

                    ScrollBar.vertical: ScrollBar { visible: paramList.contentHeight > paramList.height }

                    // 空状态
                    Text {
                        anchors.centerIn: parent
                        visible: root.filteredIndices.length === 0
                        text: qsTr("无匹配参数")
                        color: "#566070"; font.pixelSize: 13
                    }

                    delegate: Item {
                        id: paramDelegate
                        required property int index
                        required property var modelData

                        readonly property int    optIdx: modelData
                        readonly property string oGroup: root.configVm ? root.configVm.printOptions.optGroup(optIdx) : ""
                        readonly property bool   showGroupHeader: oGroup !== "" && (
                            index === 0
                            || (root.configVm && root.configVm.printOptions.optGroup(root.filteredIndices[index - 1]) !== oGroup)
                        )
                        readonly property int    contentHeight: paramRow.oType === "double" || paramRow.oType === "int" ? 56 : 44

                        width: paramList.width
                        height: (showGroupHeader ? 28 : 0) + contentHeight

                        // Group header (对齐上游 ConfigOptionsGroup)
                        Rectangle {
                            visible: paramDelegate.showGroupHeader
                            anchors.top: parent.top
                            width: parent.width
                            height: 28
                            color: "#131720"

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 20
                                anchors.verticalCenter: parent.verticalCenter
                                text: paramDelegate.oGroup
                                color: "#6b7d94"
                                font.pixelSize: 11
                                font.bold: true
                            }

                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: 1
                                color: "#1e2430"
                            }
                        }

                        // Parameter row
                        Rectangle {
                            id: paramRow
                            y: paramDelegate.showGroupHeader ? 28 : 0
                            width: parent.width
                            height: paramDelegate.contentHeight
                            color: (paramDelegate.index + (paramDelegate.showGroupHeader ? 1 : 0)) % 2 === 0 ? "transparent" : "#080b10"

                            readonly property string oType:  root.configVm ? root.configVm.printOptions.optType(optIdx)     : ""
                            readonly property string oLabel: root.configVm ? root.configVm.printOptions.optLabel(optIdx)    : ""
                            readonly property var    oVal:   root.configVm ? root.configVm.printOptions.optValue(optIdx)    : 0
                            readonly property double oMin:   root.configVm ? root.configVm.printOptions.optMin(optIdx)      : 0
                            readonly property double oMax:   root.configVm ? root.configVm.printOptions.optMax(optIdx)      : 1
                            readonly property double oStep:  root.configVm ? root.configVm.printOptions.optStep(optIdx)     : 1
                            readonly property bool   oRO:    root.configVm ? root.configVm.printOptions.optReadonly(optIdx) : false
                            readonly property bool   oDirty: root.configVm ? root.configVm.printOptions.optIsDirty(optIdx) : false
                            readonly property string oTip:   root.configVm ? root.configVm.printOptions.optTooltip(optIdx) : ""

                            // Tooltip (帮助文案)
                            ToolTip.visible: oTip !== "" && tipMA.containsMouse
                            ToolTip.text: oTip
                            ToolTip.delay: 500

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 20
                            anchors.rightMargin: 16
                            spacing: 12

                            // 参数标签
                            ColumnLayout {
                                Layout.preferredWidth: 180
                                spacing: 2
                                RowLayout {
                                    spacing: 4
                                    // 脏标记（橙色圆点，对齐上游 Tab 的修改指示器）
                                    Rectangle {
                                        visible: paramRow.oDirty
                                        width: 6; height: 6; radius: 3
                                        color: "#f0883e"
                                        Layout.alignment: Qt.AlignVCenter
                                    }
                                    Text {
                                        text: paramRow.oLabel
                                        color: paramRow.oRO ? "#566070" : (paramRow.oDirty ? "#f0883e" : "#c8d4e0")
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                        font.bold: paramRow.oDirty
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

                            // 控件区
                            Item {
                                Layout.fillWidth: true
                                height: paramRow.height - 8

                                // Slider（double/int）
                                RowLayout {
                                    visible: paramRow.oType === "double" || paramRow.oType === "int"
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: parent.width
                                    spacing: 8
                                    Slider {
                                        Layout.fillWidth: true
                                        from: paramRow.oMin; to: paramRow.oMax; stepSize: paramRow.oStep
                                        value: typeof paramRow.oVal === "number" ? paramRow.oVal : paramRow.oMin
                                        enabled: !paramRow.oRO
                                        onMoved: {
                                            if (root.configVm)
                                                root.configVm.printOptions.setValue(paramRow.optIdx, value)
                                        }
                                    }
                                    Text {
                                        text: typeof paramRow.oVal === "number"
                                              ? Number(paramRow.oVal).toFixed(paramRow.oType === "double" ? 2 : 0)
                                              : "—"
                                        color: "#e2e8f1"; font.pixelSize: 11; font.bold: true
                                        Layout.preferredWidth: 50
                                    }
                                }

                                // Switch（bool）
                                Switch {
                                    visible: paramRow.oType === "bool"
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.right: parent.right
                                    checked: paramRow.oVal === true || paramRow.oVal === "true"
                                    enabled: !paramRow.oRO
                                    onToggled: {
                                        if (root.configVm)
                                            root.configVm.printOptions.setValue(paramRow.optIdx, checked)
                                    }
                                }

                                // ComboBox（enum）
                                ComboBox {
                                    visible: paramRow.oType === "enum"
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.right: parent.right
                                    width: 160
                                    enabled: !paramRow.oRO
                                    font.pixelSize: 11
                                    currentIndex: typeof paramRow.oVal === "number" ? paramRow.oVal : 0
                                    model: {
                                        if (!root.configVm || paramRow.oType !== "enum") return []
                                        var arr = []
                                        var cnt = root.configVm.printOptions.optEnumCount(paramRow.optIdx)
                                        for (var j = 0; j < cnt; ++j)
                                            arr.push(root.configVm.printOptions.optEnumLabel(paramRow.optIdx, j))
                                        return arr
                                    }
                                    onActivated: (i) => {
                                        if (root.configVm)
                                            root.configVm.printOptions.setValue(paramRow.optIdx, i)
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
