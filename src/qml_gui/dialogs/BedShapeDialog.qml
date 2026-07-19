import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P8.2 -- BedShapeDialog: bed shape configuration (aligns with upstream BedShapeDialog)
// Shape selection: Rectangle / Circle / Custom
// Dimension inputs: width, depth, height, origin offset, diameter
// Preview canvas showing the bed outline
// Usage: BedShapeDialog { id: dlg }  ->  dlg.open()

CxDialog {
    id: root

    closePolicy: Popup.NoAutoClose

    dialogTitle: qsTr("热床形状设置")

    anchors.centerIn: parent
    width: 400
    height: 380

    required property var editorVm

    contentItem: RowLayout {
        spacing: Theme.spacingXL
        anchors.fill: parent
        anchors.margins: Theme.spacingXL
        // -- Left: shape options + dimension inputs --
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingMD
            // Shape type (aligns with upstream BedShape::PageType)
            Text {
                text: qsTr("热床形状")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
            }

            RowLayout {
                spacing: Theme.spacingSM
                Repeater {
                    model: [
                        qsTr("矩形"),
                        qsTr("圆形"),
                        qsTr("自定义")
                    ]

                    Rectangle {
                        required property int index
                        required property string modelData
                        property bool checked: root.editorVm
                            ? root.editorVm.bedShapeType === index
                            : (index === 0)
                        width: shapeText.implicitWidth + 20
                        height: 26
                        radius: 4
                        color: checked ? Theme.accent : Theme.bgElevated
                        border.width: 1
                        border.color: checked ? Theme.accent : Theme.borderSubtle

                        Text {
                            id: shapeText
                            anchors.centerIn: parent
                            text: modelData
                            color: checked ? Theme.textOnAccent : Theme.textPrimary
                            font.pixelSize: Theme.fontSizeSM
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (root.editorVm)
                                    root.editorVm.bedShapeType = index
                            }
                        }
                    }
                }
            }

            // Dimension section header
            Text {
                text: qsTr("尺寸")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
                topPadding: 4
            }

            // Width (rectangle) or Diameter (circle)
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD
                visible: root.editorVm ? root.editorVm.bedShapeType !== 2 : true

                Text {
                    width: 50
                    text: root.editorVm && root.editorVm.bedShapeType === 1
                        ? qsTr("直径")
                        : qsTr("宽度")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeSM
                }
                CxTextField {
                    id: widthField
                    Layout.fillWidth: true
                    text: {
                        if (!root.editorVm) return "220"
                        return root.editorVm.bedShapeType === 1
                            ? root.editorVm.bedDiameter.toFixed(1)
                            : root.editorVm.bedWidth.toFixed(1)
                    }
                    onEditingFinished: {
                        if (!root.editorVm) return
                        var v = parseFloat(text) || 0
                        if (root.editorVm.bedShapeType === 1)
                            root.editorVm.bedDiameter = v
                        else
                            root.editorVm.bedWidth = v
                    }
                }
            }

            // Depth (rectangle only)
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD
                visible: root.editorVm ? root.editorVm.bedShapeType === 0 : true

                Text {
                    width: 50
                    text: qsTr("深度")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeSM
                }
                CxTextField {
                    id: depthField
                    Layout.fillWidth: true
                    text: root.editorVm ? root.editorVm.bedDepth.toFixed(1) : "220"
                    onEditingFinished: {
                        if (!root.editorVm) return
                        root.editorVm.bedDepth = parseFloat(text) || 0
                    }
                }
            }

            // Height
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD
                visible: root.editorVm ? root.editorVm.bedShapeType !== 2 : true

                Text {
                    width: 50
                    text: qsTr("高度")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeSM
                }
                CxTextField {
                    id: heightField
                    Layout.fillWidth: true
                    text: root.editorVm ? root.editorVm.bedMaxHeight.toFixed(1) : "300"
                    onEditingFinished: {
                        if (!root.editorVm) return
                        root.editorVm.bedMaxHeight = parseFloat(text) || 0
                    }
                }
            }

            // Origin section
            Text {
                text: qsTr("原点偏移")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
                topPadding: 4
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD
                Text {
                    width: 50
                    text: "X"
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeSM
                }
                CxTextField {
                    id: originXField
                    Layout.fillWidth: true
                    text: root.editorVm ? root.editorVm.bedOriginX.toFixed(1) : "0"
                    onEditingFinished: {
                        if (!root.editorVm) return
                        root.editorVm.bedOriginX = parseFloat(text) || 0
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD
                Text {
                    width: 50
                    text: "Y"
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeSM
                }
                CxTextField {
                    id: originYField
                    Layout.fillWidth: true
                    text: root.editorVm ? root.editorVm.bedOriginY.toFixed(1) : "0"
                    onEditingFinished: {
                        if (!root.editorVm) return
                        root.editorVm.bedOriginY = parseFloat(text) || 0
                    }
                }
            }
        }

        // -- Right: bed shape preview canvas --
        Canvas {
            id: previewCanvas
            Layout.preferredWidth: 130
            Layout.fillHeight: true

            property real bedW: root.editorVm ? root.editorVm.bedWidth : 220
            property real bedD: root.editorVm ? root.editorVm.bedDepth : 220
            property real bedDiam: root.editorVm ? root.editorVm.bedDiameter : 220
            property int shapeType: root.editorVm ? root.editorVm.bedShapeType : 0

            onBedWChanged: requestPaint()
            onBedDChanged: requestPaint()
            onBedDiamChanged: requestPaint()
            onShapeTypeChanged: requestPaint()
            onPainted: {}
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            function draw() {
                requestPaint()
            }

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                var w = width
                var h = height
                var pad = 10
                var drawW = w - pad * 2
                var drawH = h - pad * 2

                // Grid background
                ctx.strokeStyle = Theme.scrollBarTrackColor
                ctx.lineWidth = 0.5
                for (var gx = pad; gx <= pad + drawW; gx += 15) {
                    ctx.beginPath()
                    ctx.moveTo(gx, pad)
                    ctx.lineTo(gx, pad + drawH)
                    ctx.stroke()
                }
                for (var gy = pad; gy <= pad + drawH; gy += 15) {
                    ctx.beginPath()
                    ctx.moveTo(pad, gy)
                    ctx.lineTo(pad + drawW, gy)
                    ctx.stroke()
                }

                // Origin crosshair
                var ox = pad + drawW / 2
                var oy = pad + drawH / 2
                ctx.strokeStyle = Theme.bgPressed
                ctx.lineWidth = 0.5
                ctx.beginPath()
                ctx.moveTo(ox, pad)
                ctx.lineTo(ox, pad + drawH)
                ctx.moveTo(pad, oy)
                ctx.lineTo(pad + drawW, oy)
                ctx.stroke()

                // Bed shape outline
                ctx.strokeStyle = Theme.accent
                ctx.lineWidth = 2
                ctx.fillStyle = "rgba(24, 199, 94, 0.08)"

                if (shapeType === 1) {
                    // Circle
                    var maxR = Math.min(drawW, drawH) / 2
                    var r = Math.min(maxR, (bedDiam / 2) * (maxR / 120))
                    r = Math.max(1, r)
                    ctx.beginPath()
                    ctx.arc(ox, oy, r, 0, 2 * Math.PI)
                    ctx.fill()
                    ctx.stroke()
                } else if (shapeType === 0) {
                    // Rectangle
                    var scaleX = drawW / 300
                    var scaleY = drawH / 300
                    var scale = Math.min(scaleX, scaleY)
                    var rw = Math.max(1, bedW * scale)
                    var rh = Math.max(1, bedD * scale)
                    var rx = ox - rw / 2
                    var ry = oy - rh / 2
                    ctx.beginPath()
                    ctx.roundRect(rx, ry, rw, rh, 3)
                    ctx.fill()
                    ctx.stroke()

                    // Dimension labels
                    ctx.fillStyle = Theme.textTertiary
                    ctx.font = "9px sans-serif"
                    ctx.textAlign = "center"
                    ctx.fillText(bedW.toFixed(0) + "mm", ox, ry - 4)
                    ctx.save()
                    ctx.translate(rx - 4, oy)
                    ctx.rotate(-Math.PI / 2)
                    ctx.fillText(bedD.toFixed(0) + "mm", 0, 0)
                    ctx.restore()
                } else {
                    // Custom placeholder
                    ctx.setLineDash([4, 3])
                    ctx.strokeStyle = Theme.textDisabled
                    ctx.fillStyle = "rgba(86, 96, 112, 0.06)"
                    ctx.beginPath()
                    ctx.roundRect(pad + 10, pad + 10, drawW - 20, drawH - 20, 4)
                    ctx.fill()
                    ctx.stroke()
                    ctx.setLineDash([])

                    ctx.fillStyle = Theme.textDisabled
                    ctx.font = "11px sans-serif"
                    ctx.textAlign = "center"
                    ctx.fillText(qsTr("从 STL 导入"), ox, oy)
                }
            }
        }
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
                text: qsTr("取消")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.reject()
            }

            CxButton {
                text: qsTr("确定")
                cxStyle: CxButton.Style.Primary
                onClicked: root.accept()
            }
        }
    }

    onOpened: {
        // Sync text fields with current editorVm values
        if (editorVm) {
            widthField.text = editorVm.bedShapeType === 1
                ? editorVm.bedDiameter.toFixed(1)
                : editorVm.bedWidth.toFixed(1)
            depthField.text = editorVm.bedDepth.toFixed(1)
            heightField.text = editorVm.bedMaxHeight.toFixed(1)
            originXField.text = editorVm.bedOriginX.toFixed(1)
            originYField.text = editorVm.bedOriginY.toFixed(1)
            previewCanvas.draw()
        }
    }
}
