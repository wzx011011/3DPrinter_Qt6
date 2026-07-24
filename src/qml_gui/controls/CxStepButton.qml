import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// CxStepButton.qml - mono-font step/jump button used by the Preview move slider
// and the layer rail. Unifies the former inline MoveStepButton (MoveSlider.qml)
// and RailButton (PreviewLayerRail.qml) into one theme-aware control
// (Phase 194, UI-01).
//
// Presentation only: callers own the delta and the triggered signal handler.
Rectangle {
    id: root

    property string label: ""
    property string tooltip: ""
    property bool controlEnabled: true
    property int preferredWidth: 28
    property int preferredHeight: 26
    property bool smallFont: false
    // Generic step value for callers that need to read it back from the
    // instance (e.g. MoveSlider passes -10/-1/+1/+10). Not used internally.
    property int stepValue: 0
    signal triggered()

    Layout.preferredWidth: root.preferredWidth
    Layout.preferredHeight: root.preferredHeight
    Layout.alignment: Qt.AlignHCenter
    radius: 4
    color: stepMouse.containsMouse && root.controlEnabled ? Theme.bgHover : Theme.bgElevated
    border.width: 1
    border.color: stepMouse.containsMouse && root.controlEnabled ? Theme.accentDark : Theme.borderSubtle
    opacity: root.controlEnabled ? 1.0 : 0.45

    Text {
        anchors.centerIn: parent
        text: root.label
        color: Theme.textPrimary
        font.pixelSize: root.smallFont ? Theme.fontSizeXS : Theme.fontSizeSM
        font.family: Theme.fontMono
    }

    MouseArea {
        id: stepMouse
        anchors.fill: parent
        enabled: root.controlEnabled
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.triggered()
    }

    ToolTip.visible: stepMouse.containsMouse && root.tooltip.length > 0
    ToolTip.text: root.tooltip
    ToolTip.delay: 450
}
