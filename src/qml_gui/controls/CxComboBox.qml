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
        color: {
            if (!root.enabled) return Theme.bgPanel
            if (root.pressed) return Theme.bgPressed
            if (root.hovered) return Theme.bgHover
            return Theme.bgElevated
        }
        border.color: root.activeFocus ? Theme.borderFocus : Theme.borderStrong
        border.width: 1
        Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
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
            color: Theme.bgElevated
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
            color: highlighted && comboItem.enabled ? Theme.accentSubtle : "transparent"
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
