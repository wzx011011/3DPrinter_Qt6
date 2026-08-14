import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P8.6b -- WipeTowerDialog (aligns with upstream WipeTowerDialog)
// Wipe tower configuration for multi-extruder: ramming, flushing volumes
// Usage: WipeTowerDialog { id: dlg }  ->  dlg.open()

CxDialog {
    id: root

    closePolicy: Popup.NoAutoClose

    dialogTitle: qsTr("擦料塔设置")

    anchors.centerIn: parent
    width: 420
    height: 380

    property bool advancedMode: false

    // v5.12 gap-closure: flush volume matrix computed from filament colours
    // via PresetServiceMock::calculateFlushMatrix (FlushVolCalculator). Falls
    // back to a flat default when no preset service is available.
    property var presetSvc: typeof backend !== "undefined" && backend
        ? backend.presetServiceMock : null
    property var flushMatrix: defaultMatrix(4)
    property var extruderNames: [qsTr("耗材1"), qsTr("耗材2"), qsTr("耗材3"), qsTr("耗材4")]
    property var extruderColors: [Theme.statusInfo, Theme.statusError, Theme.accent, Theme.statusWarning]

    // Default (uniform) matrix for N extruders.
    function defaultMatrix(n) {
        var m = []
        for (var i = 0; i < n; ++i) {
            var row = []
            for (var j = 0; j < n; ++j)
                row.push(i === j ? 0 : 140)
            m.push(row)
        }
        return m
    }
    // Convert a flat QVariantList (row-major N*N) to a 2D array.
    function flatToMatrix(flat, n) {
        if (!flat || flat.length === 0) return defaultMatrix(n)
        var m = []
        for (var i = 0; i < n; ++i) {
            var row = []
            for (var j = 0; j < n; ++j)
                row.push(flat[i * n + j])
            m.push(row)
        }
        return m
    }
    Component.onCompleted: {
        if (presetSvc) {
            var flat = presetSvc.calculateFlushMatrix()
            if (flat && flat.length > 0) {
                var n = Math.sqrt(flat.length)
                if (n > 0) {
                    extruderNames = []
                    for (var i = 0; i < n; ++i)
                        extruderNames.push(qsTr("耗材") + (i + 1))
                    flushMatrix = flatToMatrix(flat, n)
                }
            }
        }
    }

    // Ramming settings
    property real rammingVolume: 10.0
    property real rammingLineWidthMulti: 1.0
    property real rammingStepMulti: 1.0

    // Wiping settings
    property real flushMultiplier: 1.0
    property real minFlushVolume: 80.0

    contentItem: ColumnLayout {
        width: root.width
        spacing: Theme.spacingMD
        anchors.margins: Theme.spacingXL
        // Mode toggle
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            Text {
                text: qsTr("模式")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
            }

            Rectangle {
                width: simpleText.implicitWidth + 16
                height: 24
                radius: 4
                color: !root.advancedMode ? Theme.accent : Theme.bgElevated
                border.width: 1
                border.color: !root.advancedMode ? Theme.accent : Theme.borderSubtle

                Text {
                    id: simpleText
                    anchors.centerIn: parent
                    text: qsTr("简单")
                    color: !root.advancedMode ? Theme.textOnAccent : Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.advancedMode = false
                }
            }

            Rectangle {
                width: advText.implicitWidth + 16
                height: 24
                radius: 4
                color: root.advancedMode ? Theme.accent : Theme.bgElevated
                border.width: 1
                border.color: root.advancedMode ? Theme.accent : Theme.borderSubtle

                Text {
                    id: advText
                    anchors.centerIn: parent
                    text: qsTr("高级")
                    color: root.advancedMode ? Theme.textOnAccent : Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.advancedMode = true
                }
            }
        }

        // -- Ramming Section --
        Text {
            text: qsTr("撞击设置")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
            font.bold: true
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 4
            columnSpacing: 8
            rowSpacing: 6

            Text { text: qsTr("体积"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS }
            CxTextField { Layout.fillWidth: true; implicitHeight: 22; font.pixelSize: Theme.fontSizeXS; text: root.rammingVolume.toFixed(1) }

            Text { text: qsTr("线宽倍率"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS }
            CxTextField { Layout.fillWidth: true; implicitHeight: 22; font.pixelSize: Theme.fontSizeXS; text: root.rammingLineWidthMulti.toFixed(2) }

            Text { text: qsTr("步进倍率"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS }
            CxTextField { Layout.fillWidth: true; implicitHeight: 22; font.pixelSize: Theme.fontSizeXS; text: root.rammingStepMulti.toFixed(2) }
        }

        // -- Wiping Section --
        Text {
            text: qsTr("擦洗设置")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
            font.bold: true
            topPadding: 4
        }

        // Flush volume matrix (simplified)
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: flushGrid.implicitHeight + 12
            radius: 4
            color: Theme.bgPanel
            border.color: Theme.borderInput
            border.width: 1

            Grid {
                id: flushGrid
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: Theme.spacingSM
                columns: 5
                rowSpacing: 4
                columnSpacing: 4

                // Header row
                Text { width: 50; height: 20; text: ""; color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS }
                Repeater {
                    model: root.extruderNames.length
                    Rectangle {
                        width: 60
                        height: 20
                        radius: 3
                        color: root.extruderColors[index]
                        opacity: 0.3
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("耗材%1").arg(index + 1)
                            color: root.extruderColors[index]
                            font.pixelSize: 8
                            font.bold: true
                        }
                    }
                }

                // Data rows
                Repeater {
                    model: root.extruderNames.length

                    Row {
                        spacing: Theme.spacingXS
                        // Row header
                        Rectangle {
                            width: 50
                            height: 22
                            radius: 3
                            color: root.extruderColors[index]
                            opacity: 0.3
                            Text {
                                anchors.centerIn: parent
                                text: qsTr("耗材%1").arg(index + 1)
                                color: root.extruderColors[index]
                                font.pixelSize: 8
                                font.bold: true
                            }
                        }

                        // Flush volume cells
                        Repeater {
                            model: root.extruderNames.length
                            CxTextField {
                                width: 60
                                implicitHeight: 22
                                font.pixelSize: Theme.fontSizeXS
                                horizontalAlignment: Text.AlignHCenter
                                text: parent.Repeater ? (root.flushMatrix[index] !== undefined
                                    ? root.flushMatrix[index][modelData] : "0") : "0"
                                enabled: index !== parent.parent.parent.index
                            }
                        }
                    }
                }
            }
        }

        // Multiplier + min volume
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            Text {
                Layout.preferredWidth: 70
                text: qsTr("擦洗倍率")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeXS
            }
            CxTextField {
                Layout.preferredWidth: 60
                implicitHeight: 22
                font.pixelSize: Theme.fontSizeXS
                text: root.flushMultiplier.toFixed(2)
            }

            Text {
                Layout.preferredWidth: 70
                text: qsTr("最小体积")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeXS
            }
            CxTextField {
                Layout.preferredWidth: 60
                implicitHeight: 22
                font.pixelSize: Theme.fontSizeXS
                text: root.minFlushVolume.toFixed(1)
            }

            Item { Layout.fillWidth: true }

            CxButton {
                text: qsTr("计算")
                cxStyle: CxButton.Style.Secondary
                compact: true
                // v5.12: recompute flush matrix from filament colours.
                enabled: presetSvc !== null
                onClicked: {
                    if (!presetSvc) return
                    var flat = presetSvc.calculateFlushMatrix()
                    if (flat && flat.length > 0) {
                        var n = Math.sqrt(flat.length)
                        if (n > 0)
                            flushMatrix = flatToMatrix(flat, n)
                    }
                }
            }

            CxButton {
                text: qsTr("重置")
                cxStyle: CxButton.Style.Secondary
                compact: true
                onClicked: flushMatrix = defaultMatrix(extruderNames.length)
            }
        }

        Item { Layout.fillHeight: true }
    }

    footer: Rectangle {
        width: parent.width
        height: 48
        color: Theme.bgSurface
        radius: 8
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
                onClicked: root.accept()
            }

            CxButton {
                text: qsTr("取消")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.reject()
            }
        }
    }
}
