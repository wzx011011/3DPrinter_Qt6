import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// 搜索对话框（对齐上游 SearchDialog / OptionsSearcher）
// 弹出式搜索面板，显示结果按 Page > Category > Group 分组
// 支持搜索 key/label/category/group，点击结果跳转到对应选项
Popup {
    id: root
    required property var configVm
    signal jumpToOption(int optionIndex)
    signal closed()

    anchors.centerIn: parent ? parent : undefined
    width: 420
    height: Math.min(480, root.parent ? root.parent.height * 0.7 : 480)
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 0

    background: Rectangle {
        radius: 8
        color: "#1a2030"
        border.width: 1
        border.color: Theme.borderDefault
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 }
        NumberAnimation { property: "scale"; from: 0.95; to: 1; duration: 150 }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 100 }
    }

    onClosed: root.closed()
    onAboutToShow: searchField.forceActiveFocus()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: "#161c28"
            radius: 8

            // Bottom-only radius override
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: Theme.borderSubtle
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8

                Text {
                    text: "\uD83D\uDD0D"
                    font.pixelSize: 14
                }

                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: qsTr("搜索配置选项...")
                    color: Theme.textPrimary
                    font.pixelSize: 12
                    background: null
                    selectByMouse: true
                    onTextChanged: {
                        searchTimer.restart()
                    }
                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Up) {
                            resultView.decrementCurrentIndex()
                            event.accepted = true
                        } else if (event.key === Qt.Key_Down) {
                            resultView.incrementCurrentIndex()
                            event.accepted = true
                        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            if (resultView.count > 0 && resultView.currentIndex >= 0) {
                                const idx = resultView.model.get(resultView.currentIndex).optIndex
                                root.jumpToOption(idx)
                                root.close()
                            }
                            event.accepted = true
                        }
                    }
                }

                // 匹配计数
                Text {
                    visible: resultView.count > 0
                    text: resultView.count + qsTr(" 项")
                    color: Theme.textDisabled
                    font.pixelSize: 10
                }

                // 关闭按钮
                Rectangle {
                    width: 24; height: 24; radius: 4
                    color: closeBtn.containsMouse ? "#2e1a1a" : "transparent"
                    Text { anchors.centerIn: parent; text: "\u2715"; color: "#8a96a8"; font.pixelSize: 11 }
                    MouseArea { id: closeBtn; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: root.close()
                    }
                }
            }
        }

        // Search results
        ListView {
            id: resultView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: ListModel { id: resultModel }
            currentIndex: -1
            highlightFollowsCurrentItem: true
            highlight: Rectangle {
                color: "#1c6e42"
                radius: 4
            }
            ScrollIndicator.vertical: ScrollIndicator {}

            delegate: Column {
                required property int index
                required property var modelData
                readonly property int optIndex: modelData.optIndex
                readonly property string optLabel: modelData.optLabel
                readonly property string optKey: modelData.optKey
                readonly property string optPath: modelData.optPath
                readonly property string valueSrc: modelData.valueSrc
                readonly property string matchField: modelData.matchField

                width: ListView.view.width
                spacing: 0

                // Category separator
                Rectangle {
                    width: parent.width
                    height: 24
                    color: "#141a24"
                    visible: index === 0 || resultModel.get(index - 1).optPath !== modelData.optPath

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 8
                        spacing: 4

                        Text {
                            text: modelData.optPath
                            color: Theme.accent
                            font.pixelSize: 9
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }

                // Result item
                Rectangle {
                    width: parent.width
                    height: 36
                    color: resultView.currentIndex === index ? "#1c6e42" :
                           resultDelegateMa.containsMouse ? "#1c2535" : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 8

                        // Value source indicator (对齐上游 Tab value source)
                        Rectangle {
                            width: 6; height: 6; radius: 3
                            visible: true
                            color: {
                                switch (valueSrc) {
                                case "printer": return "#6ea8d4"  // blue
                                case "filament": return "#d4a06e"  // orange
                                case "print": return "#6ed4a0"    // green
                                default: return "#555"            // gray (default)
                                }
                            }
                            Layout.alignment: Qt.AlignVCenter
                        }

                        // Label
                        Text {
                            Layout.fillWidth: true
                            text: optLabel
                            color: Theme.textPrimary
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }

                        // Key (dimmed)
                        Text {
                            text: optKey
                            color: Theme.textDisabled
                            font.pixelSize: 9
                            font.family: "Consolas, monospace"
                        }

                        // Source tag
                        Text {
                            text: {
                                switch (valueSrc) {
                                case "printer": return qsTr("P")
                                case "filament": return qsTr("F")
                                case "print": return qsTr("S")
                                default: return ""
                                }
                            }
                            color: {
                                switch (valueSrc) {
                                case "printer": return "#6ea8d4"
                                case "filament": return "#d4a06e"
                                case "print": return "#6ed4a0"
                                default: return Theme.textDisabled
                                }
                            }
                            font.pixelSize: 8
                            font.bold: true
                            Layout.preferredWidth: 10
                        }
                    }

                    MouseArea {
                        id: resultDelegateMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            resultView.currentIndex = index
                            root.jumpToOption(optIndex)
                            root.close()
                        }
                        onContainsMouseChanged: {
                            if (containsMouse)
                                resultView.currentIndex = index
                        }
                    }
                }
            }
        }

        // Empty state
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: resultView.count === 0 && !searchTimer.running

            Column {
                anchors.centerIn: parent
                spacing: 8

                Text {
                    text: searchField.text.length > 0 ? qsTr("未找到匹配的选项") : qsTr("输入关键词搜索配置选项")
                    color: Theme.textDisabled
                    font.pixelSize: 12
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Text {
                    visible: searchField.text.length === 0
                    text: qsTr("支持搜索名称、参数名、分类和分组")
                    color: Theme.textDisabled
                    font.pixelSize: 10
                    opacity: 0.6
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }

        // Footer hint
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: "#141a24"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 12

                Text {
                    text: "\u2191\u2193 " + qsTr("导航") + "  Enter " + qsTr("跳转") + "  Esc " + qsTr("关闭")
                    color: Theme.textDisabled
                    font.pixelSize: 9
                    opacity: 0.6
                }

                Item { Layout.fillWidth: true }

                // Source legend
                Row {
                    spacing: 6

                    Rectangle { width: 6; height: 6; radius: 3; color: "#6ea8d4"; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: qsTr("打印机"); color: Theme.textDisabled; font.pixelSize: 8; }

                    Rectangle { width: 6; height: 6; radius: 3; color: "#d4a06e"; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: qsTr("耗材"); color: Theme.textDisabled; font.pixelSize: 8; }

                    Rectangle { width: 6; height: 6; radius: 3; color: "#6ed4a0"; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: qsTr("打印"); color: Theme.textDisabled; font.pixelSize: 8; }

                    Rectangle { width: 6; height: 6; radius: 3; color: "#555"; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: qsTr("默认"); color: Theme.textDisabled; font.pixelSize: 8; }
                }
            }
        }
    }

    // Debounced search timer
    Timer {
        id: searchTimer
        interval: 150
        onTriggered: performSearch()
    }

    function performSearch() {
        if (!root.configVm) {
            resultModel.clear()
            return
        }

        const query = searchField.text
        if (query.length < 1) {
            resultModel.clear()
            return
        }

        const indices = root.configVm.searchOptions(query)
        resultModel.clear()

        // Sort by Page > Category > Group
        const sorted = []
        for (let i = 0; i < indices.length; i++) {
            const idx = indices[i]
            const opts = root.configVm.printOptions
            if (!opts) continue
            sorted.push({
                optIndex: idx,
                optLabel: opts.optLabel(idx),
                optKey: opts.optKey(idx),
                optPath: opts.optPage(idx) + " / " + opts.optCategory(idx),
                valueSrc: root.configVm.searchResultSource(i)
            })
        }

        sorted.sort((a, b) => a.optPath.localeCompare(b.optPath))

        for (let i = 0; i < sorted.length; i++) {
            resultModel.append(sorted[i])
        }

        if (resultModel.count > 0)
            resultView.currentIndex = 0
    }

    function openWithQuery(query) {
        searchField.text = query || ""
        root.open()
        searchField.forceActiveFocus()
        searchField.selectAll()
    }
}
