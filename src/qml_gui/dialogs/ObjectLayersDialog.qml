import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// ObjectLayersDialog.qml — Phase 175 (FEAT-02) Object Layer-Range Editor
//
// Upstream: third_party/OrcaSlicer/src/slic3r/GUI/GUI_ObjectLayers.cpp
//   - Per-layer-height ranges with editable bounds. Each row mirrors the
//     upstream grid row built by ObjectLayers::create_layer
//     (GUI_ObjectLayers.cpp:63-168): "Height Range | [Min Z] to [Max Z] mm"
//     followed by the -/+ buttons appended in create_layers_list (:172-203).
//     BBS upstream has the per-range layer_height editor commented out
//     (GUI_ObjectLayers.cpp:135-160), so the row contract here is bounds
//     editing + row actions only; a layer_height override for a new range is
//     still accepted by the add row through setLayerRangeValue.
//
// U12 alignment:
//   ① Min/Max Z are inline CxTextField editors committing with upstream
//      validation (GUI_ObjectLayers.cpp:88-135): |Δ|<EPSILON rejected, a min
//      pushed past max clamps max = min + 0.5, max pulled below min rejected.
//      Parse failure / negative value reverts the field to the previous valid
//      value like LayerRangeEditor::get_value + m_valid_value (:347-352,
//      :369-379, error text _L("Invalid numeric.") :453). A bounds edit
//      removes + re-adds the range through the existing VM channels — the QML
//      composition of ObjectList::edit_layer_range erase+insert
//      (GUI_ObjectList.cpp:4717-4745) — carrying the layer_height override
//      across; the backend clips overlaps on add (ProjectServiceMock.cpp:
//      6989-7031, call-only per U12 — U07 mutex).
//   ② Per-row '+' inserts after the row's maxZ via addObjectLayerRange
//      (upstream add_layer_range_after_current, GUI_ObjectList.cpp:4552-4644:
//      new range {second, second + 2.0}); disabled + tooltip when no object is
//      selected (BBS can_add_new_range_after_current :4647-4668 only fails on
//      the missing-object assert).
//   ③ Add row parseFloat guarded by Number.isFinite; invalid input is kept and
//      reported, mirroring the m_valid_value rollback.
//   ④/⑤ R11 row rhythm: row height 30 / pitch 30, numeric inputs 119×25
//      (dialog width widened 480→520 so the 119-wide fields fit).
//   ⑥ Object selection change rebuilds the whole list via onObjectIndexChanged
//      (upstream update_layers_list, GUI_ObjectLayers.cpp:226-257).
//   ⑦ Focused-range 3D highlight: deferred (layers gizmo not landed in the Qt
//      port).
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: false
    dialogTitle: qsTr("层高范围")
    width: 520
    height: 420

    property var editorVm: null
    property int objectIndex: -1

    // Slic3r::EPSILON (libslic3r.h:52) — the EPSILON the upstream row editors
    // compare against (GUI_ObjectLayers.cpp:90/:120).
    readonly property real layerEpsilon: 1e-4

    onOpened: refreshModel()
    // ⑥ upstream update_layers_list rebuilds the panel for every selection
    // change (GUI_ObjectLayers.cpp:226-257); objectIndex tracks
    // editorVm.selectedObjectIndex in PreparePage.qml:164.
    onObjectIndexChanged: refreshModel()

    // Upstream Field.cpp:47 double_to_string(4) with trailing zeroes stripped.
    function formatZ(v) {
        var s = v.toFixed(4)
        if (s.indexOf(".") >= 0)
            s = s.replace(/0+$/, "").replace(/\.$/, "")
        return s
    }

    function refreshModel() {
        rangeModel.clear()
        clearHint()
        if (!root.editorVm || root.objectIndex < 0) return
        var rows = []
        var n = root.editorVm.objectLayerRangeCount(root.objectIndex)
        for (var i = 0; i < n; ++i) {
            rows.push({
                rangeIndex: i,
                minZ: root.editorVm.layerRangeMinZ(root.objectIndex, i),
                maxZ: root.editorVm.layerRangeMaxZ(root.objectIndex, i)
            })
        }
        // Upstream shows layer_config_ranges (std::map → sorted by range);
        // the mock keeps list order (appends at the end), so sort for display
        // and carry the backend index used by remove/set calls.
        rows.sort(function(a, b) {
            return a.minZ !== b.minZ ? a.minZ - b.minZ : a.maxZ - b.maxZ
        })
        for (var j = 0; j < rows.length; ++j)
            rangeModel.append(rows[j])
    }

    // Inline error hint (upstream shows _L("Invalid numeric.") as a modal
    // error, GUI_ObjectLayers.cpp:453; rendered inline here per the fork's
    // dialog conventions).
    function showHint(msg) {
        formHint.text = msg
        hintTimer.restart()
    }
    function clearHint() {
        formHint.text = ""
        hintTimer.stop()
    }
    Timer { id: hintTimer; interval: 4000; onTriggered: formHint.text = "" }

    // Upstream LayerRangeEditor::get_value (GUI_ObjectLayers.cpp:347-352):
    // unparseable or negative input is invalid. Returns NaN on rejection.
    function parseEditorValue(text) {
        var v = parseFloat(text)
        if (!Number.isFinite(v) || v < 0.0) {
            showHint(qsTr("Invalid numeric."))
            return NaN
        }
        return v
    }

    // Upstream min-Z editor commit (GUI_ObjectLayers.cpp:88-103): same-value
    // rejected; a min at/above the old max clamps the new max to min + 0.5.
    function commitMinZ(rangeIndex, oldMin, oldMax, editor) {
        var v = parseEditorValue(editor.text)
        if (Number.isNaN(v)) { editor.text = formatZ(oldMin); return }
        if (Math.abs(v - oldMin) < layerEpsilon) { editor.text = formatZ(oldMin); return }
        var newMax = v < oldMax ? oldMax : v + 0.5
        applyRangeEdit(rangeIndex, v, newMax, editor, oldMin)
    }

    // Upstream max-Z editor commit (GUI_ObjectLayers.cpp:118-130): same-value
    // and min > max rejected.
    function commitMaxZ(rangeIndex, oldMin, oldMax, editor) {
        var v = parseEditorValue(editor.text)
        if (Number.isNaN(v)) { editor.text = formatZ(oldMax); return }
        if (Math.abs(v - oldMax) < layerEpsilon || oldMin > v) { editor.text = formatZ(oldMax); return }
        applyRangeEdit(rangeIndex, oldMin, v, editor, oldMax)
    }

    // QML composition of upstream ObjectList::edit_layer_range(range,new_range)
    // (GUI_ObjectList.cpp:4717-4745: erase the old key, insert the new key,
    // move the per-range config): remove + re-add through the existing VM
    // proxies and carry the layer_height override over via setLayerRangeValue.
    // The backend trims overlaps of neighbouring ranges on add
    // (ProjectServiceMock.cpp:6989-7031 — call-only per U12).
    function applyRangeEdit(rangeIndex, newMin, newMax, editor, revertValue) {
        if (!root.editorVm || root.objectIndex < 0) return
        var carryLH = root.editorVm.layerRangeValue(root.objectIndex, rangeIndex, "layer_height", "")
        var oldMin = root.editorVm.layerRangeMinZ(root.objectIndex, rangeIndex)
        var oldMax = root.editorVm.layerRangeMaxZ(root.objectIndex, rangeIndex)
        if (!root.editorVm.removeObjectLayerRange(root.objectIndex, rangeIndex))
            return
        if (!root.editorVm.addObjectLayerRange(root.objectIndex, newMin, newMax)) {
            // Rejected by the backend (e.g. minZ >= maxZ) — roll the range
            // back so it is not lost and revert the field (m_valid_value).
            root.editorVm.addObjectLayerRange(root.objectIndex, oldMin, oldMax)
            if (carryLH !== "")
                root.editorVm.setLayerRangeValue(root.objectIndex,
                    root.editorVm.objectLayerRangeCount(root.objectIndex) - 1,
                    "layer_height", carryLH)
            editor.text = formatZ(revertValue)
            // The remove+re-add round-trip moved the restored range to the
            // end of the backend list — rebuild so delegate rangeIndex values
            // match the backend indices again before any further row action.
            refreshModel()
            showHint(qsTr("修改失败：范围无效"))
            return
        }
        if (carryLH !== "")
            root.editorVm.setLayerRangeValue(root.objectIndex,
                root.editorVm.objectLayerRangeCount(root.objectIndex) - 1,
                "layer_height", carryLH)
        refreshModel()
    }

    // ③ Add row with the upstream numeric guard: parse failure or negative
    // keeps the inputs and reports instead of silently clearing
    // (LayerRangeEditor::get_value rollback, GUI_ObjectLayers.cpp:347-352/:369-379).
    function addRangeFromInputs() {
        clearHint()
        if (!root.editorVm || root.objectIndex < 0) return
        var minZ = parseFloat(newMinZ.text)
        var maxZ = parseFloat(newMaxZ.text)
        var lhText = newLH.text
        var lh = lhText.length > 0 ? parseFloat(lhText) : NaN
        if (!Number.isFinite(minZ) || !Number.isFinite(maxZ) || minZ < 0.0 || maxZ < 0.0 ||
                (lhText.length > 0 && (!Number.isFinite(lh) || lh < 0.0))) {
            showHint(qsTr("Invalid numeric."))
            return // inputs intentionally kept on failure
        }
        if (!root.editorVm.addObjectLayerRange(root.objectIndex, minZ, maxZ)) {
            showHint(qsTr("添加失败：最小值需小于最大值"))
            return
        }
        if (lhText.length > 0)
            root.editorVm.setLayerRangeValue(root.objectIndex,
                root.editorVm.objectLayerRangeCount(root.objectIndex) - 1,
                "layer_height", lhText)
        newMinZ.text = ""
        newMaxZ.text = ""
        newLH.text = ""
        refreshModel()
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

        Text {
            id: formHint
            Layout.fillWidth: true
            visible: text.length > 0
            color: Theme.statusError
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
            spacing: 0 // ④ R11: row 30, pitch 30

            Text {
                anchors.centerIn: parent
                visible: rangeList.count === 0
                text: qsTr("暂无层高范围（使用全局层高）")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSizeSM
            }

            delegate: Rectangle {
                required property int index
                required property int rangeIndex
                required property double minZ
                required property double maxZ

                width: rangeList.width
                height: 30 // ④ R11 row height
                color: index % 2 === 0 ? Theme.bgElevated : Theme.bgBase

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingLG
                    anchors.rightMargin: Theme.spacingLG
                    spacing: Theme.spacingSM

                    Text {
                        text: qsTr("Height Range") // upstream _L("Height Range") GUI_ObjectLayers.cpp:81
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                    }
                    CxTextField {
                        id: minZField
                        Layout.preferredWidth: 119 // ⑤ R11 input 119×25
                        implicitHeight: 25
                        text: formatZ(minZ)
                        font.pixelSize: Theme.fontSizeSM
                        onEditingFinished: root.commitMinZ(rangeIndex, minZ, maxZ, minZField)
                    }
                    Text {
                        text: qsTr("to") // upstream _L("to") GUI_ObjectLayers.cpp:112
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                    }
                    CxTextField {
                        id: maxZField
                        Layout.preferredWidth: 119
                        implicitHeight: 25
                        text: formatZ(maxZ)
                        font.pixelSize: Theme.fontSizeSM
                        onEditingFinished: root.commitMaxZ(rangeIndex, minZ, maxZ, maxZField)
                    }
                    Text {
                        text: qsTr("mm") // upstream _L("mm") GUI_ObjectLayers.cpp:121
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                    }
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
                                root.editorVm.removeObjectLayerRange(root.objectIndex, rangeIndex)
                                root.refreshModel()
                            }
                        }
                    }
                    CxIconButton {
                        buttonSize: 24
                        iconSize: 12
                        cxStyle: CxIconButton.Style.Ghost
                        // BBS can_add_new_range_after_current only fails on the
                        // missing-object assert (GUI_ObjectList.cpp:4647-4668),
                        // so the insert is disabled only without an object.
                        enabled: root.editorVm !== null && root.objectIndex >= 0
                        toolTipText: enabled ? qsTr("Add height range") // upstream _L("Add height range") GUI_ObjectLayers.cpp:186
                                             : qsTr("未选中对象")
                        Text {
                            anchors.centerIn: parent
                            text: "+"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                        }
                        onClicked: {
                            if (root.editorVm && root.objectIndex >= 0) {
                                // upstream add_layer_range_after_current
                                // (GUI_ObjectList.cpp:4552-4644): the new range
                                // starts at the row's maxZ; the backend clips
                                // overlaps with the following ranges
                                // (ProjectServiceMock.cpp:6989-7031).
                                root.editorVm.addObjectLayerRange(root.objectIndex, maxZ, maxZ + 2.0)
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
                spacing: Theme.spacingSM

                CxTextField {
                    id: newMinZ
                    Layout.preferredWidth: 119 // ⑤ R11 input 119×25
                    implicitHeight: 25
                    font.pixelSize: Theme.fontSizeSM
                    placeholderText: qsTr("起 mm")
                }
                CxTextField {
                    id: newMaxZ
                    Layout.preferredWidth: 119
                    implicitHeight: 25
                    font.pixelSize: Theme.fontSizeSM
                    placeholderText: qsTr("止 mm")
                }
                CxTextField {
                    id: newLH
                    Layout.preferredWidth: 119
                    implicitHeight: 25
                    font.pixelSize: Theme.fontSizeSM
                    placeholderText: qsTr("层高 mm")
                }
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("添加")
                    compact: true
                    cxStyle: CxButton.Style.Primary
                    enabled: newMinZ.text.length > 0 && newMaxZ.text.length > 0
                    onClicked: root.addRangeFromInputs()
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
