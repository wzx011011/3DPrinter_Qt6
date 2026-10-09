import QtQuick
import QtQuick.Controls
import ".."

// params-8 / ctl-9: upstream SpinInput structure (SpinInput.cpp:33,198-240):
// square corners (radius 0), value text LEFT-aligned, two 14px icon step
// buttons stacked on the LEFT edge (inc above the mid line, dec below) with a
// 1px horizontal separator between them, and the unit suffix right-aligned
// INSIDE the field (Field.cpp combine_side_text). Background is the flat
// panel base color (ctl-12); the border turns accent on hover/focus
// (SpinInput.cpp:35 Normal #DBDBDB -> Hovered teal).
SpinBox {
    id: root

    // Unit suffix rendered inside the field, right-aligned (upstream side text).
    property string suffix: ""

    implicitHeight: Theme.controlHeightSM
    implicitWidth: 90
    font.pixelSize: Theme.fontSizeMD

    // SpinInput.cpp:225 btnSize = {14, (size.y - 4) / 2}
    readonly property int btnWidth: 14
    readonly property int btnHeight: Math.max(6, Math.round((height - 4) / 2))
    // ctl-10: upstream hover border is teal (SpinInput.cpp:35); OWzx accent
    // dark tier per the adjudicated value (no matching Theme token yet).
    readonly property color hoverBorder: Theme.accentDark

    background: Rectangle {
        radius: 0  // SpinInput.cpp:33
        color: Theme.bgPanel  // ctl-12: flat, same base as the hosting panel
        // Phase 170 (P0-2): keyboard focus ring uses the shared borderFocus;
        // hover keeps the accentDark step.
        border.color: !root.enabled ? Theme.borderSubtle
                     : (root.activeFocus || spinInput.activeFocus) ? Theme.borderFocus
                     : root.hovered ? root.hoverBorder
                     : Theme.borderDefault
        border.width: 1
        opacity: root.enabled ? 1.0 : 0.45
        Behavior on border.color { ColorAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }

        // SpinInput.cpp:198-201: 1px horizontal separator across the step
        // buttons at mid height, starting at the button x, button width - 2.
        Rectangle {
            x: 3
            y: parent.height / 2
            width: root.btnWidth - 2
            height: 1
            color: Theme.borderDefault
        }
    }

    // SpinInput.cpp:232-235: buttons at x=3, inc above the mid line, dec below.
    up.indicator: Rectangle {
        x: 3
        y: root.height / 2 - root.btnHeight - 1
        width: root.btnWidth
        height: root.btnHeight
        color: root.up.pressed ? Theme.bgPressed : root.up.hovered ? Theme.bgHover : "transparent"

        Image {
            anchors.centerIn: parent
            source: "qrc:/qml/assets/icons/spin_inc.svg"
            sourceSize: Qt.size(12, 6)
            fillMode: Image.PreserveAspectFit
        }
    }

    down.indicator: Rectangle {
        x: 3
        y: root.height / 2 + 1
        width: root.btnWidth
        height: root.btnHeight
        color: root.down.pressed ? Theme.bgPressed : root.down.hovered ? Theme.bgHover : "transparent"

        Image {
            anchors.centerIn: parent
            source: "qrc:/qml/assets/icons/spin_dec.svg"
            sourceSize: Qt.size(12, 6)
            fillMode: Image.PreserveAspectFit
        }
    }

    contentItem: Item {
        implicitWidth: childrenRect.width
        implicitHeight: childrenRect.height

        TextInput {
            id: spinInput
            z: 2
            anchors.left: parent.left
            anchors.leftMargin: 6 + root.btnWidth  // SpinInput.cpp:231 text x = 6 + btnSize.x
            anchors.right: suffixText.visible ? suffixText.left : parent.right
            anchors.rightMargin: suffixText.visible ? 5 : 10
            anchors.verticalCenter: parent.verticalCenter
            text: root.textFromValue(root.value, root.locale)
            color: Theme.textPrimary
            font: root.font
            horizontalAlignment: Qt.AlignLeft  // value left-aligned (upstream text ctrl)
            verticalAlignment: Qt.AlignVCenter
            readOnly: !root.editable
            validator: root.validator
            selectionColor: Theme.selectionColor
            selectedTextColor: Theme.selectionText
        }

        Text {
            id: suffixText
            visible: root.suffix !== ""
            anchors.right: parent.right
            anchors.rightMargin: 5  // SpinInput.cpp:205 label drawn at size.x - labelSize.x - 5
            anchors.verticalCenter: parent.verticalCenter
            text: root.suffix
            color: Theme.textTertiary
            font.pixelSize: Theme.fontSizeXS
            font.family: root.font.family
        }
    }
}
