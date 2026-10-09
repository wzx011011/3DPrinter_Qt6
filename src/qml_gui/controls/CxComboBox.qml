import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import ".."

ComboBox {
    id: root

    implicitHeight: Theme.controlHeightSM
    font.pixelSize: Theme.fontSizeMD

    // v5.16 (PSET2-05): section headers + disabled entries in plain string
    // models (upstream PresetComboBoxes.cpp:1281-1317 — "User presets"/
    // "System presets" separators and LABEL_ITEM_DISABLED graying).
    // Entries starting with sectionPrefix render as non-selectable group
    // headers; entries ending with disabledSuffix render grayed and cannot
    // be activated.
    property string sectionPrefix: "—"
    property string disabledSuffix: " (不兼容)"

    background: Rectangle {
        radius: Theme.radiusSM
        // ctl-12: flat base, same color as the hosting panel. Upstream reacts
        // to hover via the BORDER only (ComboBox.cpp:54-59), not the fill.
        color: {
            if (!root.enabled) return Theme.bgPanel
            if (root.activeFocus) {
                // ctl-10: focused combo picks up a ~10% accent tint over the
                // panel base (upstream readonly combo focused bg #E5F0EE,
                // dark #283232 -- adapted to the OWzx accent).
                const a = Theme.accent
                const b = Theme.bgPanel
                return Qt.rgba(b.r + (a.r - b.r) * 0.1,
                               b.g + (a.g - b.g) * 0.1,
                               b.b + (a.b - b.b) * 0.1, 1)
            }
            return Theme.bgPanel
        }
        border.color: {
            if (!root.enabled) return Theme.borderSubtle
            // ctl-10 + Phase 170 (P0-2): focus uses the shared borderFocus
            // ring (same as CxTextField/CxButton); hover steps to the accent
            // dark tier (was a merged hover||focus hard-coded darker green).
            if (root.activeFocus) return Theme.borderFocus
            if (root.hovered) return Theme.accentDark
            return Theme.borderDefault
        }
        border.width: 1
        Behavior on color { ColorAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }
        Behavior on border.color { ColorAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }
        opacity: root.enabled ? 1.0 : 0.45
    }

    contentItem: Text {
        leftPadding: Theme.spacingMD
        rightPadding: root.indicator.width + Theme.spacingXS
        text: root.displayText
        color: root.enabled ? Theme.textPrimary : Theme.textDisabled
        font: root.font
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    indicator: Text {
        x: root.width - width - Theme.spacingMD
        y: (root.height - height) / 2
        text: "▾"
        color: Theme.textMuted
        font.pixelSize: Theme.fontSizeXS
    }

    popup: Popup {
        y: root.height + 2
        width: root.width
        implicitHeight: contentItem.implicitHeight
        padding: 0

        background: Rectangle {
            // ctl-11: popup base follows the (ctl-1) flat panel gray instead
            // of the raised bgElevated step.
            color: Theme.bgPanel
            border.color: Theme.borderDefault
            border.width: 1
            radius: Theme.radiusSM
        }

        contentItem: ListView {
            clip: true
            implicitHeight: Math.min(contentHeight, 240)
            model: root.popup.visible ? root.delegateModel : null
            ScrollIndicator.vertical: ScrollIndicator {}
        }
    }

    delegate: ItemDelegate {
        id: comboItem
        readonly property string entryText: root.textRole
            ? (Array.isArray(root.model) ? modelData[root.textRole] : model[root.textRole])
            : modelData
        // v5.16 (PSET2-05): section headers ("— … —") and incompatible
        // entries ("… (不兼容)") are disabled so they never activate.
        readonly property bool isSection: typeof entryText === "string"
            && entryText.length > 0
            && entryText.startsWith(root.sectionPrefix)
        readonly property bool isDisabledEntry: typeof entryText === "string"
            && entryText.length > root.disabledSuffix.length
            && entryText.endsWith(root.disabledSuffix)

        width: root.width
        height: Theme.controlHeightSM - 2
        enabled: !isSection && !isDisabledEntry
        highlighted: root.highlightedIndex === index
        opacity: enabled ? 1.0 : (isSection ? 0.9 : 0.45)
        background: Rectangle {
            // ctl-11: upstream dropdown highlight is translucent accent, not a
            // solid fill (StateColor.cpp:50-51: checked item #BFE1DE = 25%
            // accent, hovered item #E5F0EE = 10%).
            color: !comboItem.enabled ? "transparent"
                 : highlighted ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.25)
                 : comboItem.hovered ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.1)
                 : "transparent"
        }
        contentItem: Text {
            leftPadding: Theme.spacingLG
            text: comboItem.entryText
            color: comboItem.isSection ? Theme.textMuted
                 : comboItem.enabled ? Theme.textPrimary
                 : Theme.textDisabled
            font.pixelSize: comboItem.isSection ? Theme.fontSizeXS : Theme.fontSizeMD
            font.bold: comboItem.isSection
            horizontalAlignment: comboItem.isSection ? Text.AlignHCenter : Text.AlignLeft
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }
}
