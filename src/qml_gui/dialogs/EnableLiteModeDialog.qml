import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P10.2 -- EnableLiteModeDialog (fork-only dialog; no upstream counterpart)
// G-code preview lite mode toggle for low-memory systems
// Usage: EnableLiteModeDialog { id: dlg }  ->  dlg.open()

CxDialog {
    id: root

    // U04: follows the CxDialog family default (Popup.CloseOnEscape) — the
    // previous Popup.NoAutoClose override even blocked Esc on this fork-only
    // dialog (no upstream EnableLiteModeDialog exists to contradict).

    dialogTitle: qsTr("预览模式设置")

    anchors.centerIn: parent
    width: 400
    // U04 (revised in review): static heights clip this content — the former
    // 280px viewport showed only 188px of ≈249px content, and even 320px
    // leaves 224px (320 - header 44 - footer 52). Size from the body column
    // like WipeTowerDialog.qml:60-65 so the toggle row can never clip behind
    // the footer.
    height: bodyColumn.implicitHeight + 2 * Theme.spacingXXL
            + Theme.dialogHeaderHeight + Theme.dialogFooterHeight

    // Lite mode state (fork-only dialog: the pinned upstream has no
    // EnableLiteModeDialog and no gcode_preview_lite_mode config key — the
    // flag persists under that key name in this fork's QSettings).
    // Read back from the persisted BackendContext flag on completion; written
    // through the same flag on confirm.
    property bool liteModeEnabled: false

    Component.onCompleted: {
        liteModeEnabled = backend.gcodePreviewLiteMode
    }

    contentItem: ColumnLayout {
        id: bodyColumn
        anchors.fill: parent
        anchors.margins: Theme.spacingXXL
        spacing: Theme.spacingLG
        // Info icon + description
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingLG
            Rectangle {
                width: 40
                height: 40
                radius: Theme.radiusLG
                color: Qt.alpha(Theme.accent, 0.12)

                Text {
                    anchors.centerIn: parent
                    text: "⚡"
                    font.pixelSize: Theme.fontSizeXXL
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingXS
                Text {
                    text: qsTr("精简预览模式")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSize13
                    font.bold: true
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("启用精简模式可隐藏内部填充结构，仅显示关键工具路径，显著提升预览响应速度。建议在内存较低的设备上启用。")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeSM
                    wrapMode: Text.Wrap
                    lineHeight: 1.5
                }
            }
        }

        // Feature comparison
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: featureCol.implicitHeight + 16
            radius: Theme.radiusMD
            // U04: scrollBarTrackColor was a scrollbar-token misuse for a card
            // surface — the inset surface token is the semantic fit.
            color: Theme.bgInset
            border.color: Theme.borderInput
            border.width: 1

            ColumnLayout {
                id: featureCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: Theme.spacingMD
                spacing: Theme.spacingSM
                Text {
                    text: qsTr("模式对比")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingMD
                    Text { Layout.preferredWidth: 100; text: qsTr("功能"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS }
                    Text { Layout.preferredWidth: 80; text: qsTr("完整模式"); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeXS; font.bold: true }
                    Text { Layout.fillWidth: true; text: qsTr("精简模式"); color: Theme.accent; font.pixelSize: Theme.fontSizeXS; font.bold: true }
                }

                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.borderInput }

                Repeater {
                    model: [
                        { feature: qsTr("外壳轮廓"), full: "✓", lite: "✓" },
                        { feature: qsTr("内部填充"), full: "✓", lite: "✗" },
                        { feature: qsTr("支撑结构"), full: "✓", lite: "✗" },
                        { feature: qsTr("工具路径"), full: "✓", lite: "✓" },
                        { feature: qsTr("移动路径"), full: "✓", lite: "✗" }
                    ]

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingMD
                        Text { Layout.preferredWidth: 100; text: modelData.feature; color: Theme.textSecondary; font.pixelSize: Theme.fontSizeXS }
                        Text { Layout.preferredWidth: 80; text: modelData.full; color: Theme.textPrimary; font.pixelSize: Theme.fontSizeXS }
                        Text { Layout.fillWidth: true; text: modelData.lite; color: modelData.lite === "✓" ? Theme.accent : Theme.statusError; font.pixelSize: Theme.fontSizeXS }
                    }
                }
            }
        }

        // Toggle
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingLG
            CxCheckBox {
                text: qsTr("启用精简预览模式")
                font.pixelSize: Theme.fontSizeMD
                // QML-BINDING-LOOPS: onToggled instead of onCheckedChanged --
                // the programmatic checked refresh (liteModeEnabled write)
                // no longer writes back, breaking the write-back cycle.
                checked: root.liteModeEnabled
                onToggled: root.liteModeEnabled = checked
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("可在设置中随时切换")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeXS
            }
        }
    }

    footer: Rectangle {
        width: parent.width
        // U04: dialog footer height token (52px; Theme.qml:196).
        height: Theme.dialogFooterHeight
        color: Theme.bgSurface
        radius: Theme.radiusLG
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 12
            color: parent.color
        }

        RowLayout {
            anchors.fill: parent
            anchors.rightMargin: Theme.spacingXL
            spacing: Theme.spacingMD
            Item { Layout.fillWidth: true }

            CxButton {
                text: qsTr("确定")
                cxStyle: CxButton.Style.Primary
                // U04: persist the toggle through the BackendContext flag
                // (QSettings "gcode_preview_lite_mode") BEFORE accepting — the
                // previous accept() dropped the choice on the floor. Cancel
                // rejects without writing.
                onClicked: {
                    backend.setGcodePreviewLiteMode(root.liteModeEnabled)
                    root.accept()
                }
            }

            CxButton {
                text: qsTr("取消")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.reject()
            }
        }
    }
}
