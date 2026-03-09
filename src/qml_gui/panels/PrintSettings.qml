import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// G4 — 打印设置面板（动态 ConfigOptionModel，ListView + Q_INVOKABLE 访问器）
Item {
    id: root
    required property var configVm

    // ── 预设选择栏（使用 PresetListModel via Q_INVOKABLE）────────
    Rectangle {
        id: presetBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 38
        radius: 12
        color: Theme.bgPanel
        border.width: 1
        border.color: Theme.borderSubtle

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 6
            Text { text: qsTr("质量预设"); color: Theme.textSecondary; font.pixelSize: 11 }
            ComboBox {
                id: presetCombo
                Layout.fillWidth: true
                font.pixelSize: 11
                // 从 PresetListModel 读取"打印质量"分类预设
                model: {
                    if (!root.configVm || !root.configVm.presetList) return []
                    var pl = root.configVm.presetList
                    var n  = pl.countByCategory(qsTr("打印质量"))
                    var arr = []
                    for (var i = 0; i < n; ++i) arr.push(pl.presetName(pl.globalIndex(qsTr("打印质量"), i)))
                    return arr
                }
            }
        }
    }

    // ── 参数列表（动态 ConfigOptionModel ListView）──────────────
    // 不使用 section.property 避免 Qt Quick 渲染器深层 section 机制潜在崩溃
    ListView {
        id: paramList
        anchors.top: presetBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: sliceBtn.top
        anchors.topMargin: 2
        anchors.bottomMargin: 4
        clip: true
        model: root.configVm ? root.configVm.printOptions : null

        ScrollBar.vertical: ScrollBar { visible: paramList.contentHeight > paramList.height }

        delegate: Rectangle {
            id: pRow
            required property int index
            // 通过 Q_INVOKABLE 访问，不使用 roleData（避免 QVariantList 绑定问题）
            readonly property var opts:   root.configVm ? root.configVm.printOptions : null
            readonly property string oType:  opts ? opts.optType(index)     : ""
            readonly property string oLabel: opts ? opts.optLabel(index)    : ""
            readonly property var    oVal:   opts ? opts.optValue(index)    : 0
            readonly property double oMin:   opts ? opts.optMin(index)      : 0
            readonly property double oMax:   opts ? opts.optMax(index)      : 1
            readonly property double oStep:  opts ? opts.optStep(index)     : 1
            readonly property bool   oRO:    opts ? opts.optReadonly(index) : false

            width:  paramList.width
            height: (oType === "double" || oType === "int") ? 56 : 40
            radius: 10
            color:  pHover.containsMouse ? Theme.bgHover : (index % 2 === 0 ? "transparent" : "#0a0d12")

            MouseArea {
                id: pHover
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
            }

            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 8; spacing: 8

                Text {
                    Layout.preferredWidth: 90
                    text: pRow.oLabel; color: pRow.oRO ? Theme.textDisabled : Theme.textSecondary
                    font.pixelSize: 11; elide: Text.ElideRight
                }

                // Slider（double / int）
                RowLayout {
                    visible: pRow.oType === "double" || pRow.oType === "int"
                    Layout.fillWidth: true; spacing: 4
                    Slider {
                        Layout.fillWidth: true
                        from: pRow.oMin; to: pRow.oMax; stepSize: pRow.oStep
                        value: typeof pRow.oVal === "number" ? pRow.oVal : pRow.oMin
                        enabled: !pRow.oRO
                        onMoved: { if (pRow.opts) pRow.opts.setValue(pRow.index, value) }
                    }
                    Text {
                        text: typeof pRow.oVal === "number"
                              ? Number(pRow.oVal).toFixed(pRow.oType === "double" ? 2 : 0)
                              : "—"
                        color: Theme.textPrimary; font.pixelSize: 10; font.bold: true
                        Layout.preferredWidth: 38
                    }
                }

                // Switch（bool）
                Switch {
                    visible: pRow.oType === "bool"
                    Layout.alignment: Qt.AlignRight
                    checked: pRow.oVal === true || pRow.oVal === "true"
                    enabled: !pRow.oRO
                    onToggled: { if (pRow.opts) pRow.opts.setValue(pRow.index, checked) }
                }

                // ComboBox（enum）
                ComboBox {
                    visible: pRow.oType === "enum"
                    Layout.fillWidth: true
                    enabled: !pRow.oRO; font.pixelSize: 10
                    currentIndex: typeof pRow.oVal === "number" ? pRow.oVal : 0
                    model: {
                        if (!pRow.opts || pRow.oType !== "enum") return []
                        var arr = []; var n = pRow.opts.optEnumCount(pRow.index)
                        for (var j = 0; j < n; ++j) arr.push(pRow.opts.optEnumLabel(pRow.index, j))
                        return arr
                    }
                    onActivated: (i) => { if (pRow.opts) pRow.opts.setValue(pRow.index, i) }
                }
            }
        }
    }

    // ── 切片按钮 ─────────────────────────────────────────────────
    Rectangle {
        id: sliceBtn
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8
        height: 36
        radius: 12
        color: sliceMA.containsMouse ? Theme.accentLight : Theme.accent

        Text {
            anchors.centerIn: parent
            text: qsTr("▶ 开始切片")
            color: Theme.textOnAccent; font.pixelSize: 12; font.bold: true
        }

        MouseArea {
            id: sliceMA
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
        }
    }
}
