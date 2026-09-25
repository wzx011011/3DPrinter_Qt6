import QtQuick
import QtQuick.Layouts
import ".."

// CxNumericEdit.qml - right-aligned numeric text editor for option rows.
// Extracted from the inline NumericEdit in OptionRow.qml (Phase 194, UI-01) so
// double/percent/range editors share one theme-aware rendering and validator.
//
// Presentation only: the caller validates and commits via the `commit` signal.
// The int/percent path still uses CxSpinBox (step arrows); this control covers
// the free-form double and range-editor cases.
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
    signal commit(string valueText)

    Layout.preferredWidth: 120
    Layout.preferredHeight: Theme.controlHeightSM
    radius: Theme.radiusSM
    // ctl-12: flat panel base, same color as the hosting panel (matches
    // CxTextField/CxSpinBox; was Theme.bgInset).
    color: Theme.bgPanel
    border.width: 1
    border.color: edit.activeFocus ? Theme.borderFocus
                 : enabled && editHover.hovered ? "#0e8c46"
                 : enabled ? Theme.borderInput
                 : Theme.borderSubtle
    opacity: enabled ? 1.0 : 0.55
    Behavior on border.color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }

    // ctl-10: hover border turns accent (same adjudicated accent dark tier
    // #0e8c46 as CxTextField.qml:41; upstream TextInput.cpp:33-35 Normal
    // #DBDBDB -> Hovered teal).
    HoverHandler { id: editHover }

    TextInput {
        id: edit
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: 6
        anchors.right: suffixText.visible ? suffixText.left : parent.right
        anchors.rightMargin: suffixText.visible ? 5 : 6
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignRight
        color: Theme.textPrimary
        font.pixelSize: Theme.fontSizeSM
        font.bold: true
        font.family: editor.monospace ? Theme.fontMono : ""
        selectByMouse: true
        readOnly: !editor.enabled
        validator: DoubleValidator {
            decimals: editor.decimals
            notation: DoubleValidator.StandardNotation
        }
        onEditingFinished: editor.commit(text)
    }

    Text {
        id: suffixText
        visible: editor.suffix !== ""
        anchors.right: parent.right
        anchors.rightMargin: 5
        anchors.verticalCenter: parent.verticalCenter
        text: editor.suffix
        color: Theme.textTertiary
        font.pixelSize: Theme.fontSizeXS
        font.family: editor.monospace ? Theme.fontMono : ""
    }
}
