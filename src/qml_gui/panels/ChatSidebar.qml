import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ChatSidebar.qml - OWzx-only AI assistant chat panel (decision record:
// docs/ai-control.md). Pure presentation: every state comes from
// backend.aiViewModel (AiViewModel), every action routes back through its
// Q_INVOKABLEs. The harness itself is the embedded Claude Agent SDK sidecar
// driving the app over the in-app MCP server (AppToolRegistry).
//
// Message kinds rendered (AiViewModel entry contract):
//   user / assistant / tool / permission / error

Rectangle {
    id: root

    property var aiVm: null
    property bool sidecarInstalled: false
    signal closed()

    color: Theme.bgPanel
    border.width: 1
    border.color: Theme.borderSubtle

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingMD
        spacing: Theme.spacingMD

        // -- Header ----------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSM

            Text {
                text: qsTr("AI 助手")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLG
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            CxIconButton {
                iconSource: "qrc:/qml/assets/icons/trash.svg"
                toolTipText: qsTr("清空对话")
                enabled: aiVm && aiVm.messages.length > 0
                onClicked: aiVm.clearHistory()
            }
            CxIconButton {
                iconSource: "qrc:/qml/assets/icons/x.svg"
                toolTipText: qsTr("关闭")
                onClicked: root.closed()
            }
        }

        // -- Guidance states (disabled / not installed) ----------------------
        Text {
            Layout.fillWidth: true
            visible: !aiVm || !aiVm.enabled
            wrapMode: Text.WordWrap
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeMD
            text: qsTr("AI 助手未启用：请到 偏好设置 → AI 助手 填写 API Key 并打开开关。")
        }
        Text {
            Layout.fillWidth: true
            visible: aiVm && aiVm.enabled && !root.sidecarInstalled
            wrapMode: Text.WordWrap
            color: Theme.statusWarning
            font.pixelSize: Theme.fontSizeMD
            text: qsTr("AI 组件未安装：需要 ai_sidecar 目录（可选组件，见 docs/ai-control.md）。")
        }

        // -- Transcript ------------------------------------------------------
        ListView {
            id: transcript
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.spacingSM
            model: aiVm ? aiVm.messages : 0

            ScrollBar.vertical: ScrollBar {}

            delegate: Item {
                required property var modelData
                width: transcript.width
                height: bubbleCol.implicitHeight + Theme.spacingSM

                ColumnLayout {
                    id: bubbleCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 2

                    // user / assistant / error text bubbles
                    Rectangle {
                        visible: modelData.kind === "user"
                                 || modelData.kind === "assistant"
                                 || modelData.kind === "error"
                        Layout.fillWidth: true
                        implicitHeight: bodyText.implicitHeight + Theme.spacingMD
                        radius: 8
                        color: modelData.kind === "user" ? Theme.accent
                             : modelData.kind === "error" ? Theme.statusError
                             : Theme.bgBase
                        opacity: modelData.kind === "error" ? 0.15
                             : modelData.kind === "user" ? 1.0 : 0.6

                        Text {
                            id: bodyText
                            anchors.fill: parent
                            anchors.margins: Theme.spacingSM / 2
                            text: modelData.text ?? ""
                            color: modelData.kind === "user" ? "#FFFFFF" : Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                            wrapMode: Text.WordWrap
                        }
                    }

                    // tool execution card
                    Rectangle {
                        visible: modelData.kind === "tool"
                        Layout.fillWidth: true
                        implicitHeight: toolCol.implicitHeight + Theme.spacingSM
                        radius: 6
                        color: Theme.bgBase
                        border.width: 1
                        border.color: modelData.toolOk === false ? Theme.statusError : Theme.borderSubtle

                        ColumnLayout {
                            id: toolCol
                            anchors.fill: parent
                            anchors.margins: Theme.spacingSM / 2
                            spacing: 1

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.spacingSM
                                Text {
                                    text: (modelData.toolOk === undefined ? "▶ "
                                             : modelData.toolOk ? "✓ " : "✗ ")
                                          + modelData.toolName
                                    color: modelData.toolOk === false ? Theme.statusError : Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeSM
                                    font.family: "Consolas"
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    visible: modelData.toolSummary !== undefined
                                    text: (modelData.toolSummary ?? "").toString().slice(0, 80)
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSizeSM
                                    elide: Text.ElideRight
                                    Layout.maximumWidth: parent.width * 0.5
                                }
                            }
                            Text {
                                visible: Object.keys(modelData.toolInput ?? {}).length > 0
                                Layout.fillWidth: true
                                text: JSON.stringify(modelData.toolInput).slice(0, 160)
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeXS
                                font.family: "Consolas"
                                elide: Text.ElideRight
                            }
                        }
                    }

                    // destructive-action confirmation card
                    Rectangle {
                        visible: modelData.kind === "permission"
                        Layout.fillWidth: true
                        implicitHeight: permCol.implicitHeight + Theme.spacingSM
                        radius: 6
                        color: Theme.bgBase
                        border.width: 1
                        border.color: modelData.pending ? Theme.statusWarning : Theme.borderSubtle

                        ColumnLayout {
                            id: permCol
                            anchors.fill: parent
                            anchors.margins: Theme.spacingSM / 2
                            spacing: Theme.spacingSM

                            Text {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: (modelData.pending ? qsTr("AI 请求执行：") : qsTr("已处理："))
                                      + modelData.toolName
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeMD
                                font.bold: modelData.pending
                            }
                            Text {
                                visible: Object.keys(modelData.toolInput ?? {}).length > 0
                                Layout.fillWidth: true
                                text: JSON.stringify(modelData.toolInput).slice(0, 200)
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeXS
                                font.family: "Consolas"
                                elide: Text.ElideRight
                            }
                            RowLayout {
                                visible: modelData.pending
                                spacing: Theme.spacingSM
                                Item { Layout.fillWidth: true }
                                CxButton {
                                    text: qsTr("拒绝")
                                    cxStyle: CxButton.Style.Secondary
                                    onClicked: aiVm.answerPermission(modelData.callId, false)
                                }
                                CxButton {
                                    text: qsTr("允许")
                                    cxStyle: CxButton.Style.Primary
                                    onClicked: aiVm.answerPermission(modelData.callId, true)
                                }
                            }
                            Text {
                                visible: !modelData.pending
                                text: modelData.granted ? qsTr("已允许") : qsTr("已拒绝")
                                color: modelData.granted ? Theme.statusSuccess : Theme.textSecondary
                                font.pixelSize: Theme.fontSizeSM
                            }
                        }
                    }
                }
            }
        }

        // -- Input row -------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSM

            CxTextField {
                id: inputField
                Layout.fillWidth: true
                placeholderText: qsTr("提问、调参数、排查问题…")
                enabled: aiVm && aiVm.enabled && !aiVm.busy
                onAccepted: sendBtn.send()
                Keys.onEscapePressed: if (aiVm && aiVm.busy) aiVm.cancelTurn()
            }

            CxIconButton {
                id: sendBtn
                iconSource: "qrc:/qml/assets/icons/send-2.svg"
                toolTipText: qsTr("发送")
                enabled: aiVm && aiVm.enabled && !aiVm.busy
                         && inputField.text.trim().length > 0
                function send() {
                    if (!enabled)
                        return
                    aiVm.sendMessage(inputField.text)
                    inputField.text = ""
                }
                onClicked: send()
            }
        }
        Text {
            visible: aiVm && aiVm.busy
            text: qsTr("AI 正在工作…（Esc 取消）")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
        }
    }
}
