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
    signal commit(string valueText)

    Layout.preferredWidth: 90
    Layout.preferredHeight: Theme.controlHeightSM
    radius: Theme.radiusSM
    color: Theme.bgInset
    border.width: 1
    border.color: edit.activeFocus ? Theme.borderFocus
                 : enabled ? Theme.borderInput
                 : Theme.borderSubtle
    opacity: enabled ? 1.0 : 0.55

    TextInput {
        id: edit
        anchors.fill: parent
        anchors.leftMargin: 6
        anchors.rightMargin: 6
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
}
