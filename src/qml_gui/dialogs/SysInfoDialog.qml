import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// SysInfoDialog.qml — Phase 236 (DLG-03) system information dump.
//
// Upstream: Help > About > System Information opens a dialog with the
// application, Qt, OpenGL and configuration-path dump used for support
// tickets. OWzx assembles it via BackendContext::systemInfo() —
// compile-time constants, the graphics API / surface format, GL
// vendor/renderer/version strings (when a context is current), and the key
// configuration paths (AppDataLocation, user preset dir).
//
// Usage: SysInfoDialog { } -> opened via backend.showSysInfoDialogRequested
// (Help menu 系统信息).
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    dialogTitle: qsTr("系统信息")
    width: 480
    height: 460
    padding: 0

    property var info: ({})
    readonly property var infoKeys: [
        { key: "appName",        label: qsTr("应用") },
        { key: "appVersion",     label: qsTr("版本") },
        { key: "qtVersion",      label: qsTr("Qt 版本") },
        { key: "buildDate",      label: qsTr("构建日期") },
        { key: "platform",       label: qsTr("平台") },
        { key: "graphicsApi",    label: qsTr("图形 API") },
        { key: "surfaceFormat",  label: qsTr("Surface 格式") },
        { key: "glVersion",      label: qsTr("GL 版本") },
        { key: "glVendor",       label: qsTr("GL 厂商") },
        { key: "glRenderer",     label: qsTr("GL 渲染器") },
        { key: "appDataLocation", label: qsTr("配置目录") },
        { key: "userPresetDir",  label: qsTr("用户预设目录") }
    ]

    onAboutToShow: {
        info = (typeof backend !== "undefined" && backend && backend.systemInfo)
            ? backend.systemInfo() : {}
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacingMD
        anchors.margins: Theme.spacingXL

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.bgInset
            radius: 4
            border.color: Theme.borderSubtle
            border.width: 1
            clip: true

            ListView {
                anchors.fill: parent
                anchors.margins: Theme.spacingXS
                clip: true
                model: root.infoKeys
                spacing: 2

                delegate: Rectangle {
                    required property var modelData
                    width: parent.width
                    height: 30
                    radius: 3
                    color: index % 2 === 0 ? "transparent" : Theme.bgBase

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingSM
                        anchors.rightMargin: Theme.spacingSM
                        spacing: Theme.spacingSM

                        Text {
                            Layout.preferredWidth: 120
                            text: modelData.label
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeSM
                        }

                        Text {
                            Layout.fillWidth: true
                            text: root.info[modelData.key] !== undefined
                                ? root.info[modelData.key] : qsTr("不可用")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSM
                            elide: Text.ElideMiddle
                            wrapMode: Text.WrapAnywhere
                            maximumLineCount: 2
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight
            spacing: Theme.spacingMD

            CxButton {
                text: qsTr("复制")
                cxStyle: CxButton.Style.Secondary
                onClicked: {
                    var lines = []
                    for (var i = 0; i < root.infoKeys.length; ++i) {
                        var entry = root.infoKeys[i]
                        lines.push(entry.label + ": " +
                            (root.info[entry.key] !== undefined ? root.info[entry.key] : ""))
                    }
                    root.copyToClipboard(lines.join("\n"))
                }
            }

            CxButton {
                text: qsTr("关闭")
                cxStyle: CxButton.Style.Primary
                onClicked: root.close()
            }
        }
    }

    // Clipboard helper kept as a named function so hosts/tests can override.
    // Uses a hidden TextInput's selection copy (QML has no direct clipboard
    // API; this is the standard Quick idiom and stays within the object tree).
    function copyToClipboard(text) {
        var temp = Qt.createQmlObject(
            'import QtQuick; TextInput { visible: false }', root, "clipboardHelper")
        temp.text = text
        temp.selectAll()
        temp.copy()
        temp.destroy()
    }
}
