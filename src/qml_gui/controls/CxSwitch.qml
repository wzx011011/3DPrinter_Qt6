import QtQuick
import QtQuick.Controls
import ".."

Switch {
    id: root

    // ctl-8: compact capsule per the design reference and the upstream toggle
    // bitmaps (toggle_on/off.svg are 24x14, rendered ~27x16 DIP; track rx4,
    // knob 10x10 rx3). Off track is neutral gray (upstream toggle_off.svg
    // #949494; dark-theme target #6b6b6d per the ctl-8 decision -- the stale
    // Theme.switchTrackOff blue-gray token belongs to the theme package, so
    // the decided value is inlined here). On track follows Theme.accent.
    indicator: Rectangle {
        x: root.leftPadding
        y: (root.height - height) / 2
        width: 24
        height: 14
        radius: Theme.radiusLG

        color: {
            if (!root.enabled) return Theme.bgPanel
            if (root.checked) return root.hovered ? Theme.accentLight : Theme.accent
            return Theme.textDisabled
        }
        // Phase 170 (P0-2): keyboard focus ring, shared borderFocus contract.
        border.width: root.activeFocus ? 2 : 0
        border.color: Theme.borderFocus
        Behavior on color { ColorAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }
        opacity: root.enabled ? 1.0 : 0.45

        Rectangle {
            x: root.checked ? parent.width - width - 2 : 2
            y: (parent.height - height) / 2
            width: 10
            height: 10
            radius: Theme.radiusMD
            color: root.enabled ? Theme.switchKnob : Theme.textDisabled
            Behavior on x { NumberAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }
            Behavior on color { ColorAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }
            scale: root.pressed ? 0.9 : 1.0
            Behavior on scale { NumberAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }
        }
    }

    contentItem: Text {
        leftPadding: root.indicator.width + root.spacing
        text: root.text
        color: root.enabled ? Theme.textPrimary : Theme.textDisabled
        font.pixelSize: Theme.fontSizeMD
        verticalAlignment: Text.AlignVCenter
    }
}
