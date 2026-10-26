import QtQuick
pragma ComponentBehavior: Bound
import QtQuick.Controls
import QtQuick.Layouts
import OWzx

// CxNumericEdit.qml - right-aligned numeric text editor for option rows.
// Extracted from the inline NumericEdit in OptionRow.qml (Phase 194, UI-01) so
// double/percent/range editors share one theme-aware rendering and validator.
//
// Presentation only: the caller validates and commits via the `commit` signal.
// The int/percent path still uses CxSpinBox (step arrows); this control covers
// the free-form double and range-editor cases.
//
// X2 (audit 2026-10-10 ④, §4 并入项): unified input error state. The caller
// sets minimumValue/maximumValue; on editingFinished the commit is only
// emitted for values inside the bounds -- an out-of-range or non-numeric
// entry sets `error` + `errorText` (red border + hover tooltip) and the text
// STAYS so the user sees what they typed, instead of the caller silently
// clamping the value back ("非法输入可见反馈而非静默回弹"). `error` is a
// writable property so callers can surface their own validation through the
// same visual contract; editing the text again clears it.
Rectangle {
    id: editor

    property alias text: edit.text
    property int decimals: 3
    property bool monospace: true
    // Unit side text rendered INSIDE the field, right-aligned. Upstream merges
    // the sidetext into the single-line TextInput instead of a sibling label
    // (Field.cpp:923 m_combine_side_text = !m_opt.multiline), same approach as
    // CxSpinBox's suffix.
    property string suffix: ""
    // X2: optional bounds; the sentinels mean "unbounded" (the pre-X2
    // behavior -- every parseable value commits). Both are inclusive.
    property real minimumValue: -Number.MAX_VALUE
    property real maximumValue: Number.MAX_VALUE
    // X2: visible error state (writable for caller-driven validation).
    property bool error: false
    property string errorText: ""
    signal commit(string valueText)

    Layout.preferredWidth: 120
    Layout.preferredHeight: Theme.controlHeightSM
    radius: Theme.radiusSM
    // ctl-12: flat panel base, same color as the hosting panel (matches
    // CxTextField/CxSpinBox; was Theme.bgInset).
    color: Theme.bgPanel
    border.width: 1
    // X2: the error border wins over focus/hover so invalid state is never
    // visually silent.
    border.color: editor.error ? Theme.statusError
                 : edit.activeFocus ? Theme.borderFocus
                 : enabled && editHover.hovered ? Theme.accentDark
                 : enabled ? Theme.borderInput
                 : Theme.borderSubtle
    opacity: enabled ? 1.0 : 0.55
    Behavior on border.color { ColorAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }

    // ctl-10: hover border turns the adjudicated accent dark tier
    // (Theme.accentDark, shared with CxTextField; upstream
    // TextInput.cpp:33-35 Normal #DBDBDB -> Hovered teal).
    HoverHandler { id: editHover }

    // X2: the error feedback also speaks when hovered -- the allowed range.
    ToolTip.visible: editor.error && editHover.hovered && editor.errorText.length > 0
    ToolTip.text: editor.errorText
    ToolTip.delay: 200

    // X2: single commit path shared by editingFinished and tests. Commit only
    // fires for in-range numeric text; anything else shows the error state
    // and keeps the typed text on screen.
    function tryCommit() {
        const parsed = parseFloat(edit.text)
        if (isNaN(parsed)) {
            editor.error = true
            editor.errorText = qsTr("请输入有效数字")
            return
        }
        if (parsed < editor.minimumValue || parsed > editor.maximumValue) {
            editor.error = true
            editor.errorText = qsTr("超出范围（%1 ~ %2）")
                    .arg(editor.minimumValue).arg(editor.maximumValue)
            return
        }
        editor.error = false
        editor.errorText = ""
        editor.commit(edit.text)
    }

    TextInput {
        id: edit
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: 6
        anchors.right: suffixText.visible ? suffixText.left : parent.right
        anchors.rightMargin: suffixText.visible ? 5 : 6
        verticalAlignment: Text.AlignVCenter
        // X2 regression restore (X4 visual gate catch): the batch-1 edit
        // dropped this line, silently re-aligning every option-row value
        // left (upstream Field renders the value right-aligned against the
        // sidetext, Field.cpp combine_side_text).
        horizontalAlignment: Text.AlignRight
        // X2: invalid text turns red in-place as well, so the feedback
        // survives even when the border is covered by a neighbor.
        color: editor.error ? Theme.statusError : Theme.textPrimary
        font.pixelSize: Theme.fontSizeSM
        font.bold: true
        font.family: editor.monospace ? Theme.fontMono : ""
        selectByMouse: true
        readOnly: !editor.enabled
        validator: DoubleValidator {
            decimals: editor.decimals
            notation: DoubleValidator.StandardNotation
        }
        // X2: editing again clears the error before the next commit check.
        onTextEdited: {
            editor.error = false
            editor.errorText = ""
        }
        onEditingFinished: editor.tryCommit()
    }

    Text {
        id: suffixText
        visible: editor.suffix !== ""
        anchors.right: parent.right
        anchors.rightMargin: Theme.insetSM
        anchors.verticalCenter: parent.verticalCenter
        text: editor.suffix
        color: Theme.textTertiary
        font.pixelSize: Theme.fontSizeXS
        font.family: editor.monospace ? Theme.fontMono : ""
    }
}
