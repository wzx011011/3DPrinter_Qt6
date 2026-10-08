import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// SysInfoDialog.qml — Phase 236 (DLG-03) system information dump.
//
// Upstream: SysInfoDialog.cpp — dialog title
// "SLIC3R_APP_FULL_NAME + " - " + System Information" (:83), a brand row with
// the 192px logo left of the bold app title (:100 ScalableBitmap(..., 192),
// :108-114 title bold), a main info block (version / build / OS / total RAM
// "Total RAM size [MB]" :59) and an OpenGL info block (gl strings, GLSL
// version). OWzx assembles it via BackendContext::systemInfo().
// U04: brand row uses the existing printer icon as a logo placeholder
// (brand asset deferred — no logo asset in assets/ yet).
//
// Usage: SysInfoDialog { } -> opened via backend.showSysInfoDialogRequested
// (Help menu 系统信息).
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    // Upstream SysInfoDialog.cpp:83: "<app full name> - System Information".
    dialogTitle: qsTr("OWzx Slicer - 系统信息")
    width: 650
    height: 550
    padding: 0

    property var info: ({})
    readonly property var infoKeys: [
        { key: "appName",        label: qsTr("应用") },
        { key: "appVersion",     label: qsTr("版本") },
        { key: "buildCommit",    label: qsTr("Build") },
        { key: "qtVersion",      label: qsTr("Qt 版本") },
        { key: "buildDate",      label: qsTr("构建日期") },
        { key: "platform",       label: qsTr("平台") },
        { key: "totalRamMb",     label: qsTr("总内存 (MB)") },
        { key: "graphicsApi",    label: qsTr("图形 API") },
        { key: "surfaceFormat",  label: qsTr("Surface 格式") },
        { key: "glVersion",      label: qsTr("GL 版本") },
        { key: "glslVersion",    label: qsTr("GLSL 版本") },
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

        // Brand row — upstream SysInfoDialog.cpp:100/:108-114 (192px logo +
        // bold app title). Logo asset deferred: printer icon placeholder.
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingLG

            Image {
                Layout.preferredWidth: 192
                Layout.preferredHeight: 192
                source: "qrc:/qml/assets/icons/printer.svg"
                fillMode: Image.PreserveAspectFit
                sourceSize: Qt.size(192, 192)
            }

            Text {
                text: "OWzx Slicer"
                color: Theme.textPrimary
                font.pixelSize: 29
                font.bold: true
            }
        }

        Rectangle {
            id: infoCard
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.bgInset
            radius: 4
            border.color: Theme.borderSubtle
            border.width: 1
            clip: true

            CxScrollView {
                id: infoScroll
                anchors.fill: parent
                anchors.margins: Theme.spacingXS
                clip: true

                ColumnLayout {
                    width: infoScroll.availableWidth
                    spacing: 2

                    Repeater {
                        model: root.infoKeys

                        Rectangle {
                            required property int index
                            required property var modelData
                            Layout.fillWidth: true
                            // R11 row rhythm: fixed 30px row inside the
                            // ColumnLayout (height alone would be overridden
                            // by the layout's preferred-height logic).
                            Layout.preferredHeight: 30
                            implicitHeight: 30
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
                                    text: {
                                        var v = root.info[modelData.key]
                                        return v !== undefined && v !== "" ? v : qsTr("不可用")
                                    }
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSizeSM
                                    elide: Text.ElideMiddle
                                    wrapMode: Text.WrapAnywhere
                                    maximumLineCount: 2
                                }
                            }
                        }
                    }
                }
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
                        var value = root.info[entry.key]
                        lines.push(entry.label + ": " + (value !== undefined ? value : ""))
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
