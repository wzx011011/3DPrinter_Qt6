import QtQuick
pragma ComponentBehavior: Bound
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick.Dialogs
import OWzx

// P8.2 -- BedShapeDialog: bed shape configuration (aligns with upstream
// BedShapeDialog / BedShapeDialog.cpp).
//
// U02 alignment (upstream BedShapePanel::build_panel, BedShapeDialog.cpp:
// 194-282):
//   - Read-only shape combo with exactly 3 pages: Rectangle / Circle / Custom
//     (m_shape_combo, :199-205 — 3*em ≈ 30px height).
//   - "Settings" group with Size(X,Y) + Origin(X,Y) point fields (mm side
//     text + upstream tooltips, :49-75; the height row is upstream-absent
//     and removed). Defaults surface as 200 / 0 placeholders.
//   - Texture / Model groups (:260-375): Load.../Remove + filename label;
//     missing file renders red (Theme.statusError — upstream #E14747) with a
//     "Not found:" tooltip, Remove is disabled while unset ("None").
//   - Custom page "Load shape from STL..." (:238-248) -> file dialog ->
//     horizontal projection outline (geometry lives in
//     EditorViewModel::bedShapeProjectionFromStl, porting :549-593).
//   - ~64em wide, height fits content (:176-179 SetMinSize/SetSize + Fit).
// Texture/model paths stay dialog-local (preset-config write-back needs a
// ConfigViewModel option channel — registered defer); the committed shape
// goes through EditorViewModel::commitBedShape (upstream Tab.cpp:7856-7862).
// Usage: BedShapeDialog { id: dlg }  ->  dlg.open()

CxDialog {
    id: root

    // U02: upstream runs the dialog modally and EndModal(CANCEL)s on Esc —
    // Esc = cancel (with the onRejected rollback below).
    closePolicy: Popup.CloseOnEscape

    dialogTitle: qsTr("热床形状设置")

    width: 640

    required property var editorVm
    // R-P1.J3 + U02: VM state captured on open; fields write through on every
    // edit, so Cancel must restore this snapshot (upstream BedShapeDialog
    // applies only on OK). rollback() is the single reject path.
    property var _snapshot: null
    // Texture / Model group working state (upstream m_custom_texture /
    // m_custom_model). "" mirrors the upstream "None" sentinel.
    property string customTexture: ""
    property string customModel: ""
    // Custom-page loaded outline: flat [x0,y0,x1,y1,...] mm (upstream
    // m_loaded_shape, filled by load_stl).
    property var customOutline: []
    property string customOutlineName: ""

    function textureName(p) {
        if (p === "") return qsTr("无")
        const parts = ("" + p).split("/")
        return parts[parts.length - 1]
    }
    function textureMissing(p) {
        return p !== "" && root.editorVm && !root.editorVm.fileExists(p)
    }
    function textureTip(p) {
        if (p === "") return ""
        return (textureMissing(p) ? qsTr("未找到：") + " " : "") + p
    }

    function rollback() {
        // U02: extracted from the Cancel button so onRejected shares it.
        if (!root._snapshot || !root.editorVm)
            return
        var s = root._snapshot
        if (root.editorVm.bedShapeType !== s.type)
            root.editorVm.bedShapeType = s.type
        root.editorVm.bedWidth = s.w
        root.editorVm.bedDepth = s.d
        root.editorVm.bedDiameter = s.diam
        root.editorVm.bedOriginX = s.ox
        root.editorVm.bedOriginY = s.oy
        root.customTexture = s.texture
        root.customModel = s.model
        root.customOutline = s.outline
        root.customOutlineName = s.outlineName
    }

    onRejected: root.rollback()

    contentItem: RowLayout {
        spacing: Theme.spacingXL
        anchors.fill: parent
        anchors.margins: Theme.spacingXL
        // -- Left: shape options + dimension inputs + texture/model groups --
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD

            // Shape type (upstream m_shape_combo read-only combo, 3 entries)
            Text {
                text: qsTr("热床形状")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
            }
            CxComboBox {
                id: shapeCombo
                Layout.fillWidth: true
                implicitHeight: 30
                textRole: "label"
                model: [
                    { label: qsTr("矩形") },
                    { label: qsTr("圆形") },
                    { label: qsTr("自定义") }
                ]
                currentIndex: root.editorVm
                    ? Math.min(Math.max(root.editorVm.bedShapeType, 0), 2) : 0
                onActivated: (idx) => {
                    if (root.editorVm)
                        root.editorVm.bedShapeType = idx
                }
            }

            // Settings group header (upstream "Settings" static box,
            // BedShapeDialog.cpp:289)
            Text {
                text: qsTr("设置")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
                topPadding: 4
            }

            // Rectangle page: Size(X,Y) (upstream RectSize coPoints line,
            // :49-57 — two mm fields + tooltip, label column ~100px)
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSM
                visible: root.editorVm ? root.editorVm.bedShapeType === 0 : true
                Text {
                    Layout.preferredWidth: 100
                    text: qsTr("尺寸")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeSM
                }
                Text { text: "X"; color: Theme.textTertiary; font.pixelSize: Theme.fontSizeSM }
                CxTextField {
                    id: sizeXField
                    Layout.fillWidth: true
                    placeholderText: "200"
                    ToolTip.visible: sizeTipHover.hovered
                    ToolTip.text: qsTr("矩形板的 X 和 Y 方向尺寸。")
                    text: {
                        if (!root.editorVm) return "200"
                        return root.editorVm.bedShapeType === 0
                            ? root.editorVm.bedWidth : ""
                    }
                    onEditingFinished: {
                        if (!root.editorVm) return
                        var v = parseFloat(text)
                        if (isFinite(v)) root.editorVm.bedWidth = v
                    }
                }
                Text { text: qsTr("mm"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeSM }
                Text { text: "Y"; color: Theme.textTertiary; font.pixelSize: Theme.fontSizeSM }
                CxTextField {
                    id: sizeYField
                    Layout.fillWidth: true
                    placeholderText: "200"
                    ToolTip.visible: sizeTipHover.hovered
                    ToolTip.text: qsTr("矩形板的 X 和 Y 方向尺寸。")
                    text: {
                        if (!root.editorVm) return "200"
                        return root.editorVm.bedShapeType === 0
                            ? root.editorVm.bedDepth : ""
                    }
                    onEditingFinished: {
                        if (!root.editorVm) return
                        var v = parseFloat(text)
                        if (isFinite(v)) root.editorVm.bedDepth = v
                    }
                }
                Text { text: qsTr("mm"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeSM }
                HoverHandler { id: sizeTipHover }
            }

            // Rectangle page: Origin(X,Y) (upstream RectOrigin coPoints line,
            // :58-75)
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSM
                visible: root.editorVm ? root.editorVm.bedShapeType === 0 : true
                Text {
                    Layout.preferredWidth: 100
                    text: qsTr("原点")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeSM
                }
                Text { text: "X"; color: Theme.textTertiary; font.pixelSize: Theme.fontSizeSM }
                CxTextField {
                    id: originXField
                    Layout.fillWidth: true
                    placeholderText: "0"
                    ToolTip.visible: originTipHover.hovered
                    ToolTip.text: qsTr("G-code 0,0 坐标距矩形左前角的距离。")
                    text: root.editorVm ? root.editorVm.bedOriginX : "0"
                    onEditingFinished: {
                        if (!root.editorVm) return
                        var v = parseFloat(text)
                        if (isFinite(v)) root.editorVm.bedOriginX = v
                    }
                }
                Text { text: qsTr("mm"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeSM }
                Text { text: "Y"; color: Theme.textTertiary; font.pixelSize: Theme.fontSizeSM }
                CxTextField {
                    id: originYField
                    Layout.fillWidth: true
                    placeholderText: "0"
                    ToolTip.visible: originTipHover.hovered
                    ToolTip.text: qsTr("G-code 0,0 坐标距矩形左前角的距离。")
                    text: root.editorVm ? root.editorVm.bedOriginY : "0"
                    onEditingFinished: {
                        if (!root.editorVm) return
                        var v = parseFloat(text)
                        if (isFinite(v)) root.editorVm.bedOriginY = v
                    }
                }
                Text { text: qsTr("mm"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeSM }
                HoverHandler { id: originTipHover }
            }

            // Circle page: Diameter (upstream Parameter::Diameter line,
            // :76-90, default 200)
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSM
                visible: root.editorVm ? root.editorVm.bedShapeType === 1 : false
                Text {
                    Layout.preferredWidth: 100
                    text: qsTr("直径")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeSM
                }
                CxTextField {
                    id: diameterField
                    Layout.fillWidth: true
                    placeholderText: "200"
                    ToolTip.visible: diamTipHover.hovered
                    ToolTip.text: qsTr("打印床的直径。原点 (0,0) 假定位于圆心。")
                    text: root.editorVm ? root.editorVm.bedDiameter : "220"
                    onEditingFinished: {
                        if (!root.editorVm) return
                        var v = parseFloat(text)
                        if (isFinite(v)) root.editorVm.bedDiameter = v
                    }
                }
                Text { text: qsTr("mm"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeSM }
                HoverHandler { id: diamTipHover }
            }

            // Custom page: Load shape from STL... (upstream :238-248)
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSM
                visible: root.editorVm ? root.editorVm.bedShapeType === 2 : false
                CxButton {
                    text: qsTr("从 STL 导入…")
                    cxStyle: CxButton.Style.Secondary
                    onClicked: stlDialog.open()
                }
                Text {
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    text: root.customOutline.length > 0
                        ? root.customOutlineName
                        : qsTr("（未加载轮廓）")
                    color: root.customOutline.length > 0
                        ? Theme.textSecondary : Theme.textDisabled
                    font.pixelSize: Theme.fontSizeSM
                }
            }

            // Texture group (upstream init_texture_panel, :260-320)
            Text {
                text: qsTr("贴图")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
                topPadding: 4
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSM
                Text {
                    id: textureLabel
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    text: root.textureName(root.customTexture)
                    color: root.textureMissing(root.customTexture)
                        ? Theme.statusError : Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    ToolTip.visible: root.customTexture !== "" && texTipHover.hovered
                    ToolTip.text: root.textureTip(root.customTexture)
                    HoverHandler { id: texTipHover }
                }
                CxButton {
                    text: qsTr("加载…")
                    cxStyle: CxButton.Style.Secondary
                    onClicked: textureDialog.open()
                }
                CxButton {
                    text: qsTr("移除")
                    cxStyle: CxButton.Style.Secondary
                    // Upstream: remove disabled while unset ("None" sentinel,
                    // BedShapeDialog.cpp:343).
                    enabled: root.customTexture !== ""
                    onClicked: root.customTexture = ""
                }
            }

            // Model group (upstream init_model_panel, :376-436)
            Text {
                text: qsTr("模型")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
                topPadding: 4
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSM
                Text {
                    id: modelLabel
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    text: root.textureName(root.customModel)
                    color: root.textureMissing(root.customModel)
                        ? Theme.statusError : Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    ToolTip.visible: root.customModel !== "" && mdlTipHover.hovered
                    ToolTip.text: root.textureTip(root.customModel)
                    HoverHandler { id: mdlTipHover }
                }
                CxButton {
                    text: qsTr("加载…")
                    cxStyle: CxButton.Style.Secondary
                    onClicked: modelDialog.open()
                }
                CxButton {
                    text: qsTr("移除")
                    cxStyle: CxButton.Style.Secondary
                    enabled: root.customModel !== ""
                    onClicked: root.customModel = ""
                }
            }
        }

        // -- Right: bed shape preview (upstream Bed_2D pane) --
        // QtQuick.Shapes port (2026-10-23, FB-R7-08): the old Canvas called
        // ctx.roundRect(), which the Qt 6.12 Canvas 2D context does not
        // implement -- every repaint threw a TypeError and skipped the
        // outline. Vector shapes also react to property changes through
        // plain bindings, so the requestPaint() plumbing is gone entirely.
        Item {
            id: previewPane
            Layout.preferredWidth: 170
            Layout.preferredHeight: 330

            readonly property real bedW: root.editorVm ? root.editorVm.bedWidth : 220
            readonly property real bedD: root.editorVm ? root.editorVm.bedDepth : 220
            readonly property real bedDiam: root.editorVm ? root.editorVm.bedDiameter : 220
            readonly property int shapeType: root.editorVm ? root.editorVm.bedShapeType : 0
            readonly property var outline: root.customOutline

            readonly property real pad: 10
            readonly property real drawW: width - pad * 2
            readonly property real drawH: height - pad * 2
            readonly property real ox: pad + drawW / 2
            readonly property real oy: pad + drawH / 2

            // Rectangle outline (shapeType 0), same fit as the old Canvas
            // (300mm reference scale).
            readonly property real rectScale: Math.min(drawW / 300, drawH / 300)
            readonly property real rw: Math.max(1, bedW * rectScale)
            readonly property real rh: Math.max(1, bedD * rectScale)
            readonly property real rx: ox - rw / 2
            readonly property real ry: oy - rh / 2

            // Circle radius (shapeType 1): the old Canvas clamped the
            // 120mm-reference radius into half the shorter draw side.
            readonly property real maxR: Math.min(drawW, drawH) / 2
            readonly property real circleR: Math.max(1, Math.min(maxR, (bedDiam / 2) * (maxR / 120)))

            // Custom projection (shapeType 2) fitted into the inset area,
            // exactly the old drawOutline() math (inset 10, aspect fit,
            // centered on the origin crosshair).
            readonly property bool hasOutline: {
                var pts = outline
                return pts !== undefined && pts !== null && pts.length >= 6
            }
            function outlinePath() {
                var pts = outline
                if (!pts || pts.length < 6)
                    return ""
                var minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity
                for (var i = 0; i + 1 < pts.length; i += 2) {
                    minX = Math.min(minX, pts[i]);   maxX = Math.max(maxX, pts[i])
                    minY = Math.min(minY, pts[i+1]); maxY = Math.max(maxY, pts[i+1])
                }
                var spanX = Math.max(1e-6, maxX - minX)
                var spanY = Math.max(1e-6, maxY - minY)
                var scale = Math.min((drawW - 20) / spanX, (drawH - 20) / spanY)
                var s = ""
                for (var j = 0; j + 1 < pts.length; j += 2) {
                    var px = ox + (pts[j] - (minX + spanX / 2)) * scale
                    var py = oy + (pts[j+1] - (minY + spanY / 2)) * scale
                    s += (j === 0 ? "M" : "L") + px + " " + py
                }
                return s + "Z"
            }

            // 15mm grid lines across the padded area.
            function gridPath() {
                var s = ""
                for (var gx = pad; gx <= pad + drawW + 0.01; gx += 15)
                    s += "M" + gx + " " + pad
                       + "L" + gx + " " + (pad + drawH)
                for (var gy = pad; gy <= pad + drawH + 0.01; gy += 15)
                    s += "M" + pad + " " + gy
                       + "L" + (pad + drawW) + " " + gy
                return s
            }

            // Axis crosshair through the print origin.
            function crosshairPath() {
                return "M" + ox + " " + pad
                     + "L" + ox + " " + (pad + drawH)
                     + "M" + pad + " " + oy
                     + "L" + (pad + drawW) + " " + oy
            }

            // Rounded rectangle as an SVG path (the Shapes replacement for
            // the never-implemented Canvas roundRect).
            function roundedRectPath(x, y, w, h, r) {
                r = Math.min(r, w / 2, h / 2)
                return "M" + (x + r) + " " + y
                     + "H" + (x + w - r)
                     + "A" + r + " " + r + " 0 0 1 " + (x + w) + " " + (y + r)
                     + "V" + (y + h - r)
                     + "A" + r + " " + r + " 0 0 1 " + (x + w - r) + " " + (y + h)
                     + "H" + (x + r)
                     + "A" + r + " " + r + " 0 0 1 " + x + " " + (y + h - r)
                     + "V" + (y + r)
                     + "A" + r + " " + r + " 0 0 1 " + (x + r) + " " + y + "Z"
            }

            // Grid + origin crosshair
            Shape {
                anchors.fill: parent
                antialiasing: true
                ShapePath {
                    fillColor: "transparent"
                    strokeColor: Theme.scrollBarTrackColor
                    strokeWidth: 0.5
                    PathSvg { path: previewPane.gridPath() }
                }
                ShapePath {
                    fillColor: "transparent"
                    strokeColor: Theme.bgPressed
                    strokeWidth: 0.5
                    PathSvg { path: previewPane.crosshairPath() }
                }
            }

            // Bed shape outline
            Shape {
                anchors.fill: parent
                antialiasing: true
                // Circle
                ShapePath {
                    fillColor: "#1418C75E"
                    strokeColor: Theme.accent
                    strokeWidth: 2
                    PathSvg {
                        path: previewPane.shapeType === 1
                              ? "M" + (previewPane.ox - previewPane.circleR) + " " + previewPane.oy
                                + "A" + previewPane.circleR + " " + previewPane.circleR
                                + " 0 1 0 " + (previewPane.ox + previewPane.circleR) + " " + previewPane.oy
                                + "A" + previewPane.circleR + " " + previewPane.circleR
                                + " 0 1 0 " + (previewPane.ox - previewPane.circleR) + " " + previewPane.oy + "Z"
                              : ""
                    }
                }
                // Rounded rectangle
                ShapePath {
                    fillColor: "#1418C75E"
                    strokeColor: Theme.accent
                    strokeWidth: 2
                    PathSvg {
                        path: previewPane.shapeType === 0
                              ? previewPane.roundedRectPath(previewPane.rx, previewPane.ry,
                                                            previewPane.rw, previewPane.rh, 3)
                              : ""
                    }
                }
                // Custom STL projection: dashed outline while a shape is
                // loaded, dashed placeholder rect while empty (upstream
                // m_loaded_shape semantics).
                ShapePath {
                    fillColor: previewPane.hasOutline ? "#1418C75E" : "#0F566070"
                    strokeColor: previewPane.hasOutline ? Theme.accent : Theme.textDisabled
                    strokeWidth: 2
                    dashPattern: [4, 3]
                    PathSvg {
                        path: previewPane.shapeType === 2
                              ? (previewPane.hasOutline
                                   ? previewPane.outlinePath()
                                   : previewPane.roundedRectPath(previewPane.pad + 10, previewPane.pad + 10,
                                                                 previewPane.drawW - 20, previewPane.drawH - 20, 4))
                              : ""
                    }
                }
            }

            // Dimension labels (rect bed only), kept as text items.
            Text {
                visible: previewPane.shapeType === 0
                text: previewPane.bedW.toFixed(0) + "mm"
                color: Theme.textTertiary
                font.pixelSize: 9
                anchors.horizontalCenter: parent.horizontalCenter
                y: previewPane.ry - height - 2
            }
            Text {
                visible: previewPane.shapeType === 0
                text: previewPane.bedD.toFixed(0) + "mm"
                color: Theme.textTertiary
                font.pixelSize: 9
                x: previewPane.rx - width - 2
                y: previewPane.oy
                transform: Rotation { origin.x: 0; origin.y: 0; angle: -90 }
            }
            Text {
                visible: previewPane.shapeType === 2 && !previewPane.hasOutline
                text: qsTr("从 STL 导入")
                color: Theme.textDisabled
                font.pixelSize: 11
                anchors.centerIn: parent
            }
        }
    }

    // Custom-page STL picker (upstream load_stl file dialog,
    // BedShapeDialog.cpp:552). The projection itself runs in the VM.
    FileDialog {
        id: stlDialog
        nameFilters: [qsTr("STL 文件 (*.stl)"), qsTr("所有文件 (*)")]
        onAccepted: {
            if (!root.editorVm) return
            const path = selectedFile.toString().replace(/^file:\/\/\//, "")
            const outline = root.editorVm.bedShapeProjectionFromStl(path)
            if (outline.length === 0) {
                // Upstream error branch (BedShapeDialog.cpp:577-585): empty /
                // unreadable geometry never replaces the loaded outline.
                return
            }
            root.customOutline = outline
            root.customOutlineName = selectedFile.toString().split("/").pop()
        }
    }

    // Texture picker (PNG/SVG, upstream load_texture :598-614)
    FileDialog {
        id: textureDialog
        nameFilters: [qsTr("图片文件 (*.png *.svg)"), qsTr("所有文件 (*)")]
        onAccepted: {
            root.customTexture = selectedFile.toString().replace(/^file:\/\/\//, "")
        }
    }

    // Model picker (STL, upstream load_model :616-633)
    FileDialog {
        id: modelDialog
        nameFilters: [qsTr("STL 文件 (*.stl)"), qsTr("所有文件 (*)")]
        onAccepted: {
            root.customModel = selectedFile.toString().replace(/^file:\/\/\//, "")
        }
    }

    footer: Rectangle {
        width: parent.width
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
                text: qsTr("取消")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.reject()
            }

            CxButton {
                text: qsTr("确定")
                cxStyle: CxButton.Style.Primary
                onClicked: {
                    // U02: upstream Tab.cpp:7856-7862 — OK commits the shape
                    // (printable_area/bed_shape write-back lives in the VM).
                    if (root.editorVm) {
                        const sx = parseFloat(sizeXField.text)
                        const sy = parseFloat(sizeYField.text)
                        const dia = parseFloat(diameterField.text)
                        root.editorVm.commitBedShape(
                            isFinite(sx) ? sx : root.editorVm.bedWidth,
                            isFinite(sy) ? sy : root.editorVm.bedDepth,
                            root.editorVm.bedOriginX,
                            root.editorVm.bedOriginY,
                            isFinite(dia) ? dia : root.editorVm.bedDiameter,
                            root.editorVm.bedShapeType,
                            root.editorVm.bedMaxHeight)
                    }
                    root.accept()
                }
            }
        }
    }

    onOpened: {
        // Sync text fields with current editorVm values
        if (editorVm) {
            // R-P1.J3: capture the opened state for the Cancel rollback.
            root._snapshot = {
                type: editorVm.bedShapeType,
                w: editorVm.bedWidth,
                d: editorVm.bedDepth,
                diam: editorVm.bedDiameter,
                ox: editorVm.bedOriginX,
                oy: editorVm.bedOriginY,
                texture: root.customTexture,
                model: root.customModel,
                outline: root.customOutline,
                outlineName: root.customOutlineName
            }
            sizeXField.text = editorVm.bedWidth
            sizeYField.text = editorVm.bedDepth
            originXField.text = editorVm.bedOriginX
            originYField.text = editorVm.bedOriginY
            diameterField.text = editorVm.bedDiameter
            root.customTexture = editorVm.bedTextureUrl !== undefined
                ? (editorVm.bedTextureUrl + "").replace(/^file:\/\/\//, "") : ""
            root.customModel = editorVm.bedModelUrl !== undefined
                ? (editorVm.bedModelUrl + "").replace(/^file:\/\/\//, "") : ""
        }
    }
}
