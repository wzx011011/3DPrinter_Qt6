import QtQuick
pragma ComponentBehavior: Bound
import QtQuick.Controls
import OWzx

Button {
    id: root

    property int cxStyle: CxButton.Style.Primary
    property bool compact: false
    // Phase 161 (DS-02): ToolTip support for parity with CxIconButton.
    property string toolTipText: ""
    // X2 (audit 2026-10-10 ④): selected state for the Segment style -- the
    // former per-page PixelSegment pseudo-segments (LeftSidebar scope pills,
    // MonitorPage resolution pills, gizmo-panel tool selectors) all carry an
    // on/off pill look, so the state lives on the shared control instead of
    // being re-drawn per page.
    property bool selected: false
    // X2: optional leading icon (16px, same slot as CxIconButton). Lets
    // label+icon buttons (PrintHostDialog browse/test row, upstream
    // ButtonType::Parameter) come from the system instead of a private
    // Rectangle+MouseArea copy.
    property url iconSource: ""

    // X2 variant additions (体系缺变体先给 CxButton 补变体，而非页内自绘):
    //   Segment     - on/off pill (selected = accent capsule); replaces the
    //                 per-page PixelSegment/伪分段.
    //   Chip        - dense borderless-min-width chip for in-panel/toast
    //                 action rows (清除/重置/取消/复制详情 ...); no 64px
    //                 minimum, content-width.
    //   Warning     - solid amber (MonitorPage 暂停/停止延时).
    //   Info        - solid blue (MonitorPage 连接/延时摄影).
    //   GhostDanger - outline destructive (CaliHistoryDialog 清空).
    enum Style { Primary, Secondary, Danger, Ghost, Segment, Chip, Warning, Info, GhostDanger }

    readonly property bool _isChip: cxStyle === CxButton.Style.Chip
    // X2 fix (X4 visual gate catch): a `url` property compared with the STRICT
    // `!==` against "" is true even when empty (no string coercion across the
    // url/value-type boundary), which reserved a phantom 16px icon slot on
    // every iconless CxButton (+20px width, label pushed off-center). Coerce
    // through String() so an unset iconSource really is "no icon".
    readonly property bool _hasIcon: String(iconSource).length > 0

    implicitHeight: compact ? Theme.controlHeightSM : Theme.controlHeightMD
    // Chip drops the 64px text-button minimum: dense chip rows size to
    // content (the former hand-drawn chips were 20-28px wide).
    implicitWidth: _isChip
                   ? contentItem.implicitWidth + leftPadding + rightPadding
                   : Math.max(64, contentItem.implicitWidth + leftPadding + rightPadding)
    leftPadding: _isChip ? Theme.spacingMD : Theme.spacingLG
    rightPadding: _isChip ? Theme.spacingMD : Theme.spacingLG

    // Phase 161 (DS-02): press-scale for parity with CxIconButton's 0.92.
    scale: root.pressed ? 0.96 : 1.0
    Behavior on scale { NumberAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }

    background: Rectangle {
        // ctl-6: upstream corner radius is per style tier (Button.cpp:191-221,
        // FromDIP): Choice/Parameter/Icon/Expanded = 4, Compact = 8,
        // Window = 12. The Qt6 style enum has no Window-tier equivalent;
        // `compact` maps to the upstream Compact tier, all others use the
        // standard 4px tier.
        radius: root.compact ? Theme.radiusLG : Theme.radiusSM
        color: {
            const d = !root.enabled
            const h = root.hovered
            const p = root.pressed
            if (root.cxStyle === CxButton.Style.Primary) {
                // R14 (restoration-map.md:44): disabled is a neutral capsule
                // (controlDisabledBg), replacing the old accentSubtle +
                // 0.45-opacity overlay (the overlay was removed with U01).
                if (d) return Theme.controlDisabledBg
                if (p) return Theme.accentDark
                if (h) return Theme.accentLight
                return Theme.accent
            }
            if (root.cxStyle === CxButton.Style.Danger) {
                // Phase 161 (DS-02): replaced the previous runtime color
                // manipulation with explicit Phase 160 tokens (statusErrorDark /
                // statusErrorPressed). No runtime darker/lighter calls — those
                // bypass the Theme token system.
                if (d) return Theme.statusErrorDark
                if (p) return Theme.statusErrorPressed
                return Theme.statusError
            }
            if (root.cxStyle === CxButton.Style.Ghost) {
                if (p) return Theme.bgPressed
                if (h) return Theme.bgHover
                return "transparent"
            }
            if (root.cxStyle === CxButton.Style.Segment) {
                if (d) return Theme.bgPanel
                if (root.selected) return p ? Theme.accentDark : Theme.accent
                if (p) return Theme.bgPressed
                if (h) return Theme.bgHover
                return Theme.bgPanel
            }
            if (root.cxStyle === CxButton.Style.Chip) {
                if (p) return Theme.bgPressed
                if (h) return Theme.bgHover
                return Theme.bgElevated
            }
            if (root.cxStyle === CxButton.Style.GhostDanger) {
                if (p || h) return Theme.bgErrorSubtle
                return "transparent"
            }
            // Warning / Info: the solid status fills have no Phase-160 style
            // pressed/hover tokens in the theme yet (Theme.qml is outside the
            // X2 batch whitelist, so no tokens were invented here); the
            // branches stay in place for when those tokens land, and the
            // press-scale above + the focus ring carry the feedback. The
            // hand-drawn buttons these variants replace had the same flat
            // behavior (dead hover ternaries on statusWarning/statusInfo).
            if (root.cxStyle === CxButton.Style.Warning) return Theme.statusWarning
            if (root.cxStyle === CxButton.Style.Info) return Theme.statusInfo
            // Secondary (default)
            if (d) return Theme.bgPanel
            if (p) return Theme.bgPressed
            if (h) return Theme.bgHover
            return Theme.bgElevated
        }
        Behavior on color { ColorAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }

        border.color: {
            // Phase 161 (DS-02): focus border for accessibility (keyboard nav).
            if (root.activeFocus) return Theme.borderFocus
            if (root.cxStyle === CxButton.Style.Primary) return "transparent"
            if (root.cxStyle === CxButton.Style.Danger) return "transparent"
            if (root.cxStyle === CxButton.Style.Ghost) return root.hovered ? Theme.borderDefault : "transparent"
            if (root.cxStyle === CxButton.Style.Segment)
                return root.selected ? Theme.accent
                                     : (root.hovered ? Theme.borderStrong : Theme.borderDefault)
            if (root.cxStyle === CxButton.Style.Chip)
                return root.hovered ? Theme.borderStrong : Theme.borderDefault
            if (root.cxStyle === CxButton.Style.GhostDanger) return Theme.statusError
            if (root.cxStyle === CxButton.Style.Warning) return "transparent"
            if (root.cxStyle === CxButton.Style.Info) return "transparent"
            return Theme.borderStrong
        }
        border.width: root.activeFocus ? 2 : 1
        Behavior on border.color { ColorAnimation { duration: Theme.motionFast } }
    }

    contentItem: Item {
        implicitWidth: contentLabel.implicitWidth
                       + (root._hasIcon ? 16 + Theme.spacingXS : 0)
        implicitHeight: contentLabel.implicitHeight

        // Optional leading icon (16px, same slot as CxIconButton). Plain-Item
        // manual layout: anchors are ignored on positioner children, so a Row
        // would top-left both children and shift every label off-center
        // (caught by the X4 visual gate as ghosted button labels).
        Image {
            visible: root._hasIcon
            source: root.iconSource
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: 16
            height: 16
            fillMode: Image.PreserveAspectFit
            smooth: true
            opacity: root.enabled ? 1.0 : 0.5
        }

        Text {
            id: contentLabel
            anchors.centerIn: parent
            text: root.text
            color: {
                if (!root.enabled) {
                    // R14: Primary's neutral disabled capsule pairs with its own
                    // disabled text tone; other styles keep the generic disabled
                    // text token.
                    return root.cxStyle === CxButton.Style.Primary
                            ? Theme.controlDisabledText : Theme.textDisabled
                }
                if (root.cxStyle === CxButton.Style.Primary) return Theme.textOnAccent
                if (root.cxStyle === CxButton.Style.Danger) return Theme.textOnAccent
                if (root.cxStyle === CxButton.Style.Segment)
                    return root.selected ? Theme.textOnAccent : Theme.textSecondary
                if (root.cxStyle === CxButton.Style.Chip) return Theme.textPrimary
                if (root.cxStyle === CxButton.Style.GhostDanger) return Theme.statusError
                if (root.cxStyle === CxButton.Style.Warning) return Theme.textOnAccent
                if (root.cxStyle === CxButton.Style.Info) return Theme.textOnAccent
                return Theme.textPrimary
            }
            // ctl-5: upstream buttons default to Body_14 (Button.cpp:54, and the
            // Choice/Parameter/Expanded tiers at :204/210/222); only the Compact
            // tier steps down (Body_10). Align to the 14px tier, compact to 12px.
            font.pixelSize: root.compact ? Theme.fontSizeMD : Theme.fontSizeLG
            font.bold: root.cxStyle === CxButton.Style.Segment && root.selected
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    // Phase 161 (DS-02): ToolTip mirror CxIconButton's contract.
    ToolTip.visible: root.hovered && root.toolTipText.length > 0
    ToolTip.text: root.toolTipText
    ToolTip.delay: 400
}
