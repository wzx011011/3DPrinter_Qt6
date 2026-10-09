import QtQuick
import QtQuick.Controls
import ".."

// ctl-7: upstream CheckBox renders three 18px bitmaps (CheckBox.cpp:11-13):
// check_on = solid rounded box (rx1.5) + white round-cap polyline check,
// check_off = hollow 1px stroke box, check_half = solid box + white round-cap
// bar. The shapes are replicated with QML primitives (not the SVG bitmaps) so
// the fill follows Theme.accent instead of the upstream hardcoded teal;
// geometry and stroke layout match check_on/off/half.svg.
CheckBox {
    id: root

    font.pixelSize: Theme.fontSizeMD

    // Half-selected state renders the check_half bar. Tristate cycling stays
    // off (upstream boxes are binary on click); consumers may still set
    // checkState: Qt.PartiallyChecked programmatically.
    readonly property bool partial: root.checkState === Qt.PartiallyChecked

    onCheckedChanged: checkMark.requestPaint()
    onPartialChanged: checkMark.requestPaint()

    indicator: Item {
        implicitWidth: 18
        implicitHeight: 18
        x: root.leftPadding
        y: (root.height - height) / 2
        opacity: root.enabled ? 1.0 : 0.45

        // check_on/check_half rect 15x15 rx1.5; check_off rect 14x14 rx1 with
        // 1px stroke (box-local coordinates in the 18px bitmaps).
        Rectangle {
            id: box
            anchors.centerIn: parent
            width: 15
            height: 15
            radius: root.checked || root.partial ? 1.5 : 1
            color: root.checked || root.partial ? Theme.accent : "transparent"
            // Phase 170 (P0-2): keyboard focus ring replaces the default
            // stroke while focused.
            border.color: root.activeFocus ? Theme.borderFocus : Theme.borderDefault
            border.width: root.activeFocus ? 2 : 1
            Behavior on color { ColorAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }
            Behavior on border.color { ColorAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }
        }

        // White round-cap strokes from check_on.svg / check_half.svg, drawn in
        // the 15x15 box-local space (bitmap coords minus the box origin).
        Canvas {
            id: checkMark
            anchors.fill: box
            antialiasing: true
            visible: root.checked || root.partial
            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.strokeStyle = Theme.textOnAccent
                ctx.lineWidth = 1.5
                ctx.lineCap = "round"
                ctx.lineJoin = "round"
                ctx.beginPath()
                if (root.partial) {
                    // check_half.svg: line 2.5,8.5 -> 12.5,8.5
                    ctx.moveTo(2.5, 7.5)
                    ctx.lineTo(12.5, 7.5)
                } else {
                    // check_on.svg: polyline 12.5,6.5 5.5,11.5 2.5,7.5
                    ctx.moveTo(12.5, 5.5)
                    ctx.lineTo(5.5, 10.5)
                    ctx.lineTo(2.5, 6.5)
                }
                ctx.stroke()
            }
        }
    }

    contentItem: Text {
        leftPadding: root.indicator.width + root.spacing
        text: root.text
        color: root.enabled ? Theme.textPrimary : Theme.textDisabled
        font: root.font
        verticalAlignment: Text.AlignVCenter
    }
}
