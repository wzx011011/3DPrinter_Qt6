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
// Card-style presentation for non-programmer users: the assistant speaks in
// chat bubbles, every tool execution is an activity card with a human action
// label + plain-language detail (Chinese labels/details/risk sentences are
// attached by AiViewModel; QML never translates), raw JSON is tucked behind
// a collapsed "technical details" toggle, and destructive requests surface
// as an explicit allow/deny card.
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

    // ── Inline card components ────────────────────────────────────────────

    // Read-only query strip: quiet single line that never competes with
    // action cards for attention ("查看 · 应用状态 ✓").
    component QueryStrip: Rectangle {
        id: queryStrip
        required property var modelData
        Layout.fillWidth: true
        implicitHeight: queryRow.implicitHeight + Theme.spacingSM
        radius: Theme.radiusSM
        color: Theme.bgInset

        RowLayout {
            id: queryRow
            anchors.fill: parent
            anchors.leftMargin: Theme.spacingSM
            anchors.rightMargin: Theme.spacingSM
            spacing: Theme.spacingSM

            Text {
                text: qsTr("查看")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeXS
            }
            Text {
                text: queryStrip.modelData.toolLabel ?? queryStrip.modelData.toolName
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Text {
                visible: queryStrip.modelData.toolOk !== undefined
                text: queryStrip.modelData.toolOk === true ? "✓" : "✗"
                color: queryStrip.modelData.toolOk === false ? Theme.statusError
                                                             : Theme.statusSuccess
                font.pixelSize: Theme.fontSizeSM
            }
            CxBusyIndicator {
                running: queryStrip.modelData.toolOk === undefined
                size: Theme.fontSizeLG
                colorToken: Theme.textTertiary
            }
        }
    }

    // Activity card for mutating tools: label + plain-language detail +
    // one-line result; raw JSON collapsed behind a "technical details" toggle.
    component ToolCard: Rectangle {
        id: toolCard
        required property var modelData
        property bool techOpen: false
        Layout.fillWidth: true
        implicitHeight: toolCol.implicitHeight + Theme.spacingMD
        radius: Theme.radiusMD
        color: Theme.bgBase
        border.width: 1
        border.color: toolCard.modelData.toolOk === false ? Theme.statusError
                                                           : Theme.borderSubtle

        ColumnLayout {
            id: toolCol
            anchors.fill: parent
            anchors.margins: Theme.spacingSM
            spacing: Theme.spacingXS

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSM

                Text {
                    text: toolCard.modelData.toolOk === undefined ? "·"
                         : toolCard.modelData.toolOk === true ? "✓" : "✗"
                    color: toolCard.modelData.toolOk === undefined ? Theme.textTertiary
                         : toolCard.modelData.toolOk === false ? Theme.statusError
                                                               : Theme.statusSuccess
                    font.pixelSize: Theme.fontSizeLG
                    font.bold: true
                }
                Text {
                    text: toolCard.modelData.toolLabel ?? toolCard.modelData.toolName
                    color: toolCard.modelData.toolOk === false ? Theme.statusError
                                                               : Theme.textPrimary
                    font.pixelSize: Theme.fontSize13
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                CxBusyIndicator {
                    running: toolCard.modelData.toolOk === undefined
                    size: Theme.controlHeightSM - Theme.spacingMD
                }
            }

            Text {
                visible: (toolCard.modelData.toolDetail ?? "") !== ""
                Layout.fillWidth: true
                text: toolCard.modelData.toolDetail ?? ""
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
                elide: Text.ElideRight
            }

            Text {
                visible: toolCard.modelData.toolOk !== undefined
                         && (toolCard.modelData.toolResult ?? "") !== ""
                Layout.fillWidth: true
                text: toolCard.modelData.toolResult ?? ""
                color: toolCard.modelData.toolOk === false ? Theme.statusError
                                                           : Theme.textPrimary
                font.pixelSize: Theme.fontSizeSM
                wrapMode: Text.WordWrap
            }

            Text {
                id: techToggle
                visible: toolCard.modelData.toolOk !== undefined
                text: toolCard.techOpen ? qsTr("▾ 技术详情") : qsTr("▸ 技术详情")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeXS

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: toolCard.techOpen = !toolCard.techOpen
                }
            }
            Text {
                visible: toolCard.techOpen
                Layout.fillWidth: true
                text: (toolCard.modelData.toolName ?? "")
                      + (Object.keys(toolCard.modelData.toolInput ?? {}).length > 0
                             ? "\n" + JSON.stringify(toolCard.modelData.toolInput)
                             : "")
                      + ((toolCard.modelData.toolSummary ?? "") !== ""
                             ? "\n→ " + toolCard.modelData.toolSummary
                             : "")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeXS
                font.family: Theme.fontMono
                wrapMode: Text.WrapAnywhere
            }
        }
    }

    // Destructive-action confirmation card with explicit consequence text.
    component PermissionCard: Rectangle {
        id: permCard
        required property var modelData
        Layout.fillWidth: true
        implicitHeight: permCol.implicitHeight + Theme.spacingMD
        radius: Theme.radiusMD
        color: permCard.modelData.pending ? Theme.bgPanel : Theme.bgInset
        border.width: permCard.modelData.pending ? 1 : 0
        border.color: Theme.statusWarning

        ColumnLayout {
            id: permCol
            anchors.fill: parent
            anchors.margins: Theme.spacingSM
            spacing: Theme.spacingSM

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSM

                Text {
                    text: permCard.modelData.pending ? "⚠" : "✓"
                    color: permCard.modelData.pending ? Theme.statusWarning
                         : permCard.modelData.granted ? Theme.statusSuccess
                                                      : Theme.textTertiary
                    font.pixelSize: Theme.fontSizeLG
                    font.bold: true
                }
                Text {
                    Layout.fillWidth: true
                    text: permCard.modelData.pending
                          ? qsTr("AI 请求：%1").arg(
                                permCard.modelData.toolLabel
                                ?? permCard.modelData.toolName)
                          : (permCard.modelData.granted ? qsTr("已允许：%1")
                                                        : qsTr("已拒绝：%1"))
                                .arg(permCard.modelData.toolLabel
                                     ?? permCard.modelData.toolName)
                    color: permCard.modelData.pending ? Theme.textPrimary
                                                      : Theme.textSecondary
                    font.pixelSize: permCard.modelData.pending ? Theme.fontSize13
                                                               : Theme.fontSizeSM
                    font.bold: permCard.modelData.pending
                    wrapMode: Text.WordWrap
                }
            }

            Text {
                visible: permCard.modelData.pending
                         && (permCard.modelData.riskText ?? "") !== ""
                Layout.fillWidth: true
                text: permCard.modelData.riskText ?? ""
                color: Theme.statusWarning
                font.pixelSize: Theme.fontSizeSM
                wrapMode: Text.WordWrap
            }

            RowLayout {
                visible: permCard.modelData.pending
                spacing: Theme.spacingSM
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("拒绝")
                    cxStyle: CxButton.Style.Secondary
                    onClicked: aiVm.answerPermission(permCard.modelData.callId, false)
                }
                CxButton {
                    text: qsTr("允许")
                    cxStyle: CxButton.Style.Primary
                    onClicked: aiVm.answerPermission(permCard.modelData.callId, true)
                }
            }
        }
    }

    // ── Layout ────────────────────────────────────────────────────────────

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
                id: msgDelegate
                required property var modelData
                width: transcript.width
                height: msgCol.implicitHeight + Theme.spacingXS

                ColumnLayout {
                    id: msgCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: Theme.spacingXS

                    // user bubble — accent, right aligned (chat convention)
                    RowLayout {
                        Layout.fillWidth: true
                        visible: msgDelegate.modelData.kind === "user"
                        Item { Layout.fillWidth: true }
                        Rectangle {
                            Layout.maximumWidth: transcript.width * 0.85
                            implicitHeight: userText.implicitHeight + Theme.spacingMD
                            radius: Theme.radiusLG
                            color: Theme.accent

                            Text {
                                id: userText
                                anchors.fill: parent
                                anchors.margins: Theme.spacingSM
                                text: msgDelegate.modelData.text ?? ""
                                color: Theme.textOnAccent
                                font.pixelSize: Theme.fontSize13
                                wrapMode: Text.WordWrap
                            }
                        }
                    }

                    // assistant bubble — quiet surface, left aligned
                    Rectangle {
                        visible: msgDelegate.modelData.kind === "assistant"
                        Layout.fillWidth: true
                        Layout.maximumWidth: transcript.width * 0.92
                        implicitHeight: assistText.implicitHeight + Theme.spacingMD
                        radius: Theme.radiusLG
                        color: Theme.bgElevated
                        opacity: 0.85

                        Text {
                            id: assistText
                            anchors.fill: parent
                            anchors.margins: Theme.spacingSM
                            text: msgDelegate.modelData.text ?? ""
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSize13
                            wrapMode: Text.WordWrap
                        }
                    }

                    // error card
                    Rectangle {
                        visible: msgDelegate.modelData.kind === "error"
                        Layout.fillWidth: true
                        implicitHeight: errText.implicitHeight + Theme.spacingMD
                        radius: Theme.radiusMD
                        color: Theme.bgErrorSubtle
                        border.width: 1
                        border.color: Theme.statusError

                        Text {
                            id: errText
                            anchors.fill: parent
                            anchors.margins: Theme.spacingSM
                            text: msgDelegate.modelData.text ?? ""
                            color: Theme.statusError
                            font.pixelSize: Theme.fontSizeSM
                            wrapMode: Text.WordWrap
                        }
                    }

                    QueryStrip {
                        visible: msgDelegate.modelData.kind === "tool"
                                 && msgDelegate.modelData.readOnly === true
                        modelData: msgDelegate.modelData
                    }

                    ToolCard {
                        visible: msgDelegate.modelData.kind === "tool"
                                 && msgDelegate.modelData.readOnly !== true
                        modelData: msgDelegate.modelData
                    }

                    PermissionCard {
                        visible: msgDelegate.modelData.kind === "permission"
                        modelData: msgDelegate.modelData
                    }
                }
            }
        }

        // -- Empty state: welcome + suggestion chips -------------------------
        ColumnLayout {
            visible: aiVm && aiVm.enabled && root.sidecarInstalled
                     && (!aiVm.messages || aiVm.messages.length === 0)
            Layout.fillWidth: true
            spacing: Theme.spacingSM

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSize13
                text: qsTr("你好！我是 OWzx AI 助手，可以用大白话指挥我：加载模型、调参数、切片、排查问题都行。")
            }

            Repeater {
                model: [
                    qsTr("帮我加载一个模型"),
                    qsTr("把壁厚循环数调成 4"),
                    qsTr("切片当前板"),
                    qsTr("这条 G-code 预计要打多久？")
                ]
                delegate: Rectangle {
                    id: chip
                    required property string modelData
                    Layout.fillWidth: true
                    implicitHeight: chipText.implicitHeight + Theme.spacingMD
                    radius: Theme.radiusXL
                    color: chipMa.containsMouse ? Theme.accentSubtle : Theme.bgElevated
                    border.width: 1
                    border.color: chipMa.containsMouse ? Theme.accent : Theme.borderSubtle

                    Text {
                        id: chipText
                        anchors.centerIn: parent
                        text: chip.modelData
                        color: chipMa.containsMouse ? Theme.textPrimary
                                                    : Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                    }

                    MouseArea {
                        id: chipMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: aiVm.sendMessage(chip.modelData)
                    }
                }
            }
        }

        // -- Busy footer -------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            visible: aiVm && aiVm.busy
            spacing: Theme.spacingSM

            CxBusyIndicator { running: true }
            Text {
                text: qsTr("正在处理…（Esc 取消）")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
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
    }
}
