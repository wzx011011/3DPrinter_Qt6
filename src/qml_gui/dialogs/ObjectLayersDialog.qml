import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// ObjectLayersDialog.qml — Phase 175 (FEAT-02) Object Layer-Range Editor
//
// Upstream: third_party/OrcaSlicer/src/slic3r/GUI/GUI_ObjectLayers.cpp
//   - Editable per-layer-height ranges (e.g. bottom 0-5mm at 0.1mm layer height,
//     above 5mm at 0.2mm) for tapering quality.
//
// OWzx implementation (Phase 175 minimal source-truth port):
//   - Non-modal CxDialog
//   - ListView of existing ranges (min/max Z bounds + layer_height override)
//   - Add range row (minZ + maxZ + layer_height inputs + Add button)
//   - Each existing range: editable layer_height + remove button
//
// Backend: ProjectServiceMock already exposes addObjectLayerRange/
// removeObjectLayerRange/setLayerRangeValue/layerRangeValue/objectLayerRanges.
// Phase 175 adds QML proxies on EditorViewModel + this dialog.
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: false
    dialogTitle: qsTr("层高范围")
    width: 480
    height: 420

    property var editorVm: null
    property int objectIndex: -1

    onOpened: refreshModel()

    function refreshModel() {
        rangeModel.clear()
        if (!root.editorVm || root.objectIndex < 0) return
        var n = root.editorVm.objectLayerRangeCount(root.objectIndex)
        for (var i = 0; i < n; ++i) {
            var minZ = root.editorVm.layerRangeMinZ(root.objectIndex, i)
            var maxZ = root.editorVm.layerRangeMaxZ(root.objectIndex, i)
            var lh = root.editorVm.layerRangeValue(root.objectIndex, i, "layer_height", "")
            rangeModel.append({ rangeIndex: i, minZ: minZ, maxZ: maxZ, layerHeight: lh })
        }
    }

    ListModel { id: rangeModel }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingXL
        spacing: Theme.spacingMD

        Text {
            Layout.fillWidth: true
            text: qsTr("为对象的不同高度设置不同层高（例如底部精细、上部加速）。")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
            wrapMode: Text.WordWrap
        }

        // Existing ranges list
        ListView {
            id: rangeList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: rangeModel
            interactive: true

            Text {
                anchors.centerIn: parent
                visible: rangeList.count === 0
                text: qsTr("暂无层高范围（使用全局层高）")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSizeSM
            }

            delegate: Rectangle {
                required property var modelData
                required property int index

                width: rangeList.width
                height: 44
                color: index % 2 === 0 ? Theme.bgElevated : Theme.bgBase

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingLG
                    anchors.rightMargin: Theme.spacingLG
                    spacing: Theme.spacingMD

                    Text {
                        Layout.preferredWidth: 140
                        text: qsTr("%1 – %2 mm").arg(modelData.minZ).arg(modelData.maxZ)
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                    }
                    Text {
                        text: qsTr("层高")
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSizeXS
                    }
                    CxTextField {
                        Layout.preferredWidth: 70
                        text: modelData.layerHeight
                        font.pixelSize: Theme.fontSizeSM
                        placeholderText: qsTr("mm")
                        onEditingFinished: {
                            if (root.editorVm && text.length > 0) {
                                root.editorVm.setLayerRangeValue(root.objectIndex, modelData.rangeIndex, "layer_height", text)
                                root.refreshModel()
                            }
                        }
                    }
                    Item { Layout.fillWidth: true }
                    CxIconButton {
                        buttonSize: 24
                        iconSize: 12
                        cxStyle: CxIconButton.Style.Ghost
                        toolTipText: qsTr("删除该范围")
                        Text {
                            anchors.centerIn: parent
                            text: "×"
                            color: Theme.statusError
                            font.pixelSize: Theme.fontSizeMD
                        }
                        onClicked: {
                            if (root.editorVm) {
                                root.editorVm.removeObjectLayerRange(root.objectIndex, modelData.rangeIndex)
                                root.refreshModel()
                            }
                        }
                    }
                }
            }
        }

        // Add new range row
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            color: Theme.bgSurface
            radius: Theme.radiusSM

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingLG
                anchors.rightMargin: Theme.spacingLG
                spacing: Theme.spacingMD

                Text {
                    text: qsTr("新增")
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSizeXS
                }
                CxTextField {
                    id: newMinZ
                    Layout.preferredWidth: 60
                    font.pixelSize: Theme.fontSizeSM
                    placeholderText: qsTr("起 mm")
                }
                CxTextField {
                    id: newMaxZ
                    Layout.preferredWidth: 60
                    font.pixelSize: Theme.fontSizeSM
                    placeholderText: qsTr("止 mm")
                }
                CxTextField {
                    id: newLH
                    Layout.preferredWidth: 70
                    font.pixelSize: Theme.fontSizeSM
                    placeholderText: qsTr("层高 mm")
                }
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("添加")
                    compact: true
                    cxStyle: CxButton.Style.Primary
                    enabled: newMinZ.text.length > 0 && newMaxZ.text.length > 0 && newLH.text.length > 0
                    onClicked: {
                        if (root.editorVm) {
                            var ok = root.editorVm.addObjectLayerRange(
                                root.objectIndex,
                                parseFloat(newMinZ.text),
                                parseFloat(newMaxZ.text))
                            if (ok) {
                                root.editorVm.setLayerRangeValue(
                                    root.objectIndex,
                                    root.editorVm.objectLayerRangeCount(root.objectIndex) - 1,
                                    "layer_height", newLH.text)
                                newMinZ.text = ""
                                newMaxZ.text = ""
                                newLH.text = ""
                                root.refreshModel()
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
                text: qsTr("关闭")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.reject()
            }
        }
    }
}
