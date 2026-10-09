import QtQuick
import QtQuick.Controls
import ".."

// CommandPalette.qml — Ctrl+Shift+P fuzzy-ish command list (Phase 173, P3).
//
// Cross-industry baseline (VS Code / Figma / JetBrains): one overlay that
// filters a flat command list and executes via the owner. The palette owns
// presentation + filtering only; `commands` comes from main.qml (which knows
// backend/page enums/appSettings), and results are reported through
// commandTriggered(id). Escape / dim-click closes; ↑↓ moves, Enter runs.
Rectangle {
    id: root

    property var commands: []           // [{ id, label, hint }]
    property string filterText: ""
    readonly property var filteredCommands: {
        const f = filterText.toLowerCase()
        const out = []
        for (let i = 0; i < commands.length; ++i) {
            const c = commands[i]
            if (f.length === 0 || c.label.toLowerCase().indexOf(f) >= 0)
                out.push(c)
        }
        return out
    }
    signal commandTriggered(string id)
    signal closed()

    function open() {
        filterText = ""
        listView.currentIndex = 0
        visible = true
        searchField.forceActiveFocus()
    }
    function close() {
        visible = false
        closed()
    }

    visible: false
    anchors.fill: parent
    z: 9000
    color: Theme.overlayDim

    // Dim click dismisses
    MouseArea {
        anchors.fill: parent
        onClicked: root.close()
        hoverEnabled: true
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: Math.max(96, parent.height * 0.12)
        width: Math.min(560, parent.width - Theme.spacingXXL * 2)
        height: Math.min(440, searchField.height + listView.height + footerRow.height + Theme.spacingMD * 2)
        radius: Theme.radiusXL
        color: Theme.menuBackground
        border.color: Theme.borderDefault
        border.width: 1

        // Keep keystrokes inside the palette
        MouseArea { anchors.fill: parent; onClicked: searchField.forceActiveFocus() }

        Column {
            anchors.fill: parent
            anchors.margins: Theme.spacingMD

            TextField {
                id: searchField
                width: parent.width
                implicitHeight: Theme.controlHeightMD
                font.pixelSize: Theme.fontSizeLG
                color: Theme.textPrimary
                placeholderTextColor: Theme.textDisabled
                // Phase 173 (P3): the palette is a Cx-free hot path (it must
                // open with zero page dependencies), so the field is styled
                // inline against Theme tokens.
                background: Rectangle {
                    radius: Theme.radiusSM
                    color: Theme.bgInset
                    border.color: searchField.activeFocus ? Theme.borderFocus : Theme.borderInput
                    border.width: searchField.activeFocus ? 2 : 1
                }
                placeholderText: qsTr("输入命令…")
                onTextChanged: { root.filterText = text; listView.currentIndex = 0 }
                Keys.onDownPressed: listView.incrementCurrentIndex()
                Keys.onUpPressed: listView.decrementCurrentIndex()
                Keys.onReturnPressed: root.runCurrent()
                Keys.onEnterPressed: root.runCurrent()
                Keys.onEscapePressed: root.close()
            }

            ListView {
                id: listView
                width: parent.width
                height: root.visible
                        ? Math.min(contentHeight, 440 - searchField.height - footerRow.height - Theme.spacingMD * 3)
                        : 0
                clip: true
                spacing: Theme.spacingXXS
                model: root.filteredCommands
                currentIndex: 0
                delegate: Item {
                    width: listView.width
                    implicitHeight: Theme.controlHeightMD
                    Rectangle {
                        anchors.fill: parent
                        radius: Theme.radiusSM
                        color: listView.currentIndex === index ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.18)
                             : rowHover.containsMouse ? Theme.bgHover : "transparent"
                    }
                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingMD
                        anchors.rightMargin: Theme.spacingMD
                        Text {
                            width: parent.width - hintLabel.width
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            text: modelData.label
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                            elide: Text.ElideRight
                        }
                        Text {
                            id: hintLabel
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            text: modelData.hint || ""
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeSM
                            font.family: Theme.fontMono
                        }
                    }
                    MouseArea {
                        id: rowHover
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: { listView.currentIndex = index; root.runCurrent() }
                        onPositionChanged: listView.currentIndex = index
                    }
                }
            }

            Row {
                id: footerRow
                spacing: Theme.spacingMD
                Text {
                    text: qsTr("↑↓ 选择 · Enter 执行 · Esc 关闭")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeXS
                }
            }
        }
    }

    function runCurrent() {
        const c = filteredCommands[listView.currentIndex]
        if (!c)
            return
        close()
        commandTriggered(c.id)
    }
}
