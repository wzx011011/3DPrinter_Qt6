pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// UnsavedChangesDialog.qml — unsaved-changes guard for preset switches.
//
// Upstream truth: third_party/OrcaSlicer/src/slic3r/GUI/UnsavedChangesDialog.cpp
// (preset-switch ctor :813-831, build() :845-1090, update_list() :1294-1460).
// Structure ported 1:1:
//   - action line: two preset-name-aware sentences wrapping at 490DIP
//     (:1263-1280; Body_13 / GREY900 at :858-860)
//   - table header band "Settings" / "Old Value" / "New Value" with 1px
//     separators above the list (:876-941); columns 190/150/150 DIP
//     (:768-770)
//   - category -> group -> option rows, 24DIP high, indents 23/37/51
//     (:1332-1448); values ellipsize at column end, embedded newlines
//     flattened to spaces (:1420, :1434)
//   - button row right-aligned Transfer -> Discard -> Save with tooltips and
//     the Help hyperlink on the left (:964-1051); no explicit cancel button —
//     upstream closes via the title-bar close box / Esc (wxID_CANCEL,
//     :818, :1053-1062), which keeps action "cancel" here
// OWzx deltas (test-locked, do not remove):
//   - per-row CxCheckBox column (tests/QmlUiAuditTests.cpp:3372-3377 pin
//     CxCheckBox + checkedKeys). The checkbox sits inside the name column's
//     51px indent gutter so the three value columns still align with the
//     header band 1:1.
//   - SettingsDialog routes action === "transfer" to
//     ConfigViewModel::transferPendingChanges(checkedKeys).
// Surfaces/colors stay on the current OWzx neutral tokens (Theme.bg* family);
// the upstream teal link color maps to the OWzx brand green (Theme.accent).
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
  id: root
  modal: true
  // Upstream preset-switch constructor title (:816).
  dialogTitle: qsTr("转移或丢弃修改")
  // Content 490DIP wide + 20px side margins (:767, :954). Height follows the
  // content like upstream SetSizerAndFit (:1086-1088); the diff list itself
  // is fixed at 374DIP and scrolls internally (:767, :944).
  width: 530
  height: Theme.dialogHeaderHeight + 20 + mainColumn.implicitHeight + 18
  padding: 0

  required property var configVm
  /// 当前 preset tier（用于显示）
  property string presetTier: ""
  /// 用户选择的动作: "save" / "transfer" / "discard" / "cancel"
  property string action: "cancel"
  /// v5.16 (PSET2-03): per-item selection — the checked keys are the ones
  /// Save/Transfer act on (upstream checkbox column, default all checked).
  property var checkedKeys: []
  /// Sectioned diff rows rebuilt on open(): [{rowType, text, key}] with
  /// rowType "category" | "group" | "option" (upstream update_list()).
  property var diffRows: []
  /// Transfer only applies when the pending action is a preset switch
  /// (upstream shows Transfer for preset switches, Keep/Discard otherwise).
  readonly property bool transferAvailable: root.configVm
    && root.configVm.pendingUnsavedAction
    && root.configVm.pendingUnsavedAction.indexOf("switch-") === 0

  // Upstream geometry constants (UnsavedChangesDialog.cpp:765-771).
  readonly property int colNameWidth: 190   // UNSAVE_CHANGE_DIALOG_FIRST_VALUE_WIDTH
  readonly property int colValueWidth: 150  // UNSAVE_CHANGE_DIALOG_VALUE_WIDTH
  readonly property int rowHeight: 24       // UNSAVE_CHANGE_DIALOG_ITEM_HEIGHT
  readonly property int listHeight: 374     // UNSAVE_CHANGE_DIALOG_SCROLL_WINDOW_SIZE height

  // The "%2%" of the upstream tooltip/action-line format: the preset being
  // edited before the switch (dependent collection's edited preset, :1028).
  readonly property string previousPresetName: {
    if (!root.configVm)
      return ""
    const tier = root.configVm.activePresetTier
    if (tier === "printer")
      return root.configVm.currentPrinterPreset
    if (tier === "filament")
      return root.configVm.currentFilamentPreset
    return root.configVm.currentPrintPreset
  }

  // The "%1%" target name; upstream falls back to "the new profile"
  // (:1028-1029).
  readonly property string newPresetName: root.configVm
    && root.configVm.pendingUnsavedTarget.length > 0
    ? root.configVm.pendingUnsavedTarget : qsTr("新预设")

  function openDialog() {
    root.action = "cancel"
    root.rebuildDiffRows()
    var keys = []
    var count = root.configVm ? root.configVm.globalModifiedCount : 0
    for (var i = 0; i < count; ++i) {
      var key = root.configVm.globalModifiedKey(i)
      if (key !== "")
        keys.push(key)
    }
    root.checkedKeys = keys
    root.open()
  }

  function toggleKey(key, checked) {
    var keys = root.checkedKeys.slice()
    var idx = keys.indexOf(key)
    if (checked && idx < 0)
      keys.push(key)
    else if (!checked && idx >= 0)
      keys.splice(idx, 1)
    root.checkedKeys = keys
  }

  // Flatten embedded newlines for the single-line value cells (upstream
  // subreplace calls at UnsavedChangesDialog.cpp:1420 and :1434).
  function flatValue(value) {
    return String(value).replace(/\n/g, " ")
  }

  // Rebuild the sectioned diff rows: first-appearance category order, one
  // band per category, one header row per group, then the option rows
  // (upstream update_list(), UnsavedChangesDialog.cpp:1294-1460).
  function rebuildDiffRows() {
    var rows = []
    var vm = root.configVm
    if (!vm) {
      root.diffRows = rows
      return
    }
    var categoryOrder = []
    var categoryMap = {}
    var count = vm.globalModifiedCount
    for (var i = 0; i < count; ++i) {
      var key = vm.globalModifiedKey(i)
      if (!key)
        continue
      var category = vm.globalModifiedCategory(key)
      if (category === "")
        category = qsTr("其他")
      var group = vm.globalModifiedGroup(key)
      var bucket = categoryMap[category]
      if (bucket === undefined) {
        bucket = categoryMap[category] = { groupOrder: [], groupMap: {} }
        categoryOrder.push(category)
      }
      if (bucket.groupMap[group] === undefined) {
        bucket.groupMap[group] = []
        bucket.groupOrder.push(group)
      }
      bucket.groupMap[group].push(key)
    }
    for (var c = 0; c < categoryOrder.length; ++c) {
      var categoryName = categoryOrder[c]
      var categoryBucket = categoryMap[categoryName]
      rows.push({ rowType: "category", text: categoryName, key: "" })
      for (var g = 0; g < categoryBucket.groupOrder.length; ++g) {
        var groupName = categoryBucket.groupOrder[g]
        if (groupName !== "")
          rows.push({ rowType: "group", text: groupName, key: "" })
        var groupKeys = categoryBucket.groupMap[groupName]
        for (var k = 0; k < groupKeys.length; ++k)
          rows.push({ rowType: "option", text: vm.globalModifiedLabel(groupKeys[k]), key: groupKeys[k] })
      }
    }
    root.diffRows = rows
  }

  contentItem: Rectangle {
    id: contentRoot
    color: Theme.bgPanel

    ColumnLayout {
      id: mainColumn
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.top: parent.top
      anchors.topMargin: 20
      anchors.leftMargin: 20
      anchors.rightMargin: 20
      spacing: 0

      // m_action_line (:1263-1280): two sentences, preset-name aware, split
      // by branch on the Transfer availability exactly like upstream; wraps
      // at the 490DIP content width (:1280).
      Text {
        Layout.fillWidth: true
        text: root.previousPresetName.length > 0
          ? qsTr("你已修改预设 \"%1\" 的一些设置。").arg(root.previousPresetName)
            + (root.transferAvailable
              ? qsTr("\n你可以保存或丢弃已修改的预设值，或选择将修改的值转移到新预设。")
              : qsTr("\n你可以保存或丢弃已修改的预设值。"))
          : qsTr("你之前修改过自己的设置。")
            + (root.transferAvailable
              ? qsTr("\n你可以丢弃已修改的预设值，或将修改的值转移到新项目")
              : qsTr("\n你可以保存或丢弃已修改的预设值。"))
        color: Theme.textPrimary
        font.pixelSize: Theme.fontSize13
        wrapMode: Text.WordWrap
      }

      // m_panel_tab (:871-955): header band + scroll list, 12px below the
      // action line (wxTOP 12 spacer at :869). Row surface stays flat like
      // upstream GREY200 (:873).
      Rectangle {
        id: panelTab
        Layout.fillWidth: true
        Layout.topMargin: 12
        color: Theme.bgInset
        implicitHeight: tableTop.implicitHeight + listFlickable.height

        // m_table_top header band (:876-941): dark band with white Body_13
        // titles, 190/150/150 columns split by 1px separators inset 2px
        // top/bottom (:915); titles carry 5px vertical margins
        // (:890, :909, :930).
        Rectangle {
          id: tableTop
          anchors.top: parent.top
          anchors.left: parent.left
          anchors.right: parent.right
          color: Theme.bgSurface
          implicitHeight: tableTopRow.implicitHeight

          RowLayout {
            id: tableTopRow
            anchors.fill: parent
            spacing: 0

            Text {
              Layout.fillWidth: true
              Layout.topMargin: 5
              Layout.bottomMargin: 5
              text: qsTr("设置")
              color: Theme.textPrimary
              font.pixelSize: Theme.fontSize13
            }
            Rectangle {
              Layout.preferredWidth: 1
              Layout.fillHeight: true
              Layout.topMargin: 2
              Layout.bottomMargin: 2
              color: Theme.borderStrong
            }
            Text {
              Layout.preferredWidth: root.colValueWidth
              Layout.topMargin: 5
              Layout.bottomMargin: 5
              text: qsTr("旧值")
              color: Theme.textPrimary
              font.pixelSize: Theme.fontSize13
            }
            Rectangle {
              Layout.preferredWidth: 1
              Layout.fillHeight: true
              Layout.topMargin: 2
              Layout.bottomMargin: 2
              color: Theme.borderStrong
            }
            Text {
              Layout.preferredWidth: root.colValueWidth
              Layout.topMargin: 5
              Layout.bottomMargin: 5
              text: qsTr("新值")
              color: Theme.textPrimary
              font.pixelSize: Theme.fontSize13
            }
          }
        }

        // m_scrolledWindow (:943-948): fixed 374DIP tall, vertical scrolling
        // only, row-colored background (m_panel_tab GREY200).
        Flickable {
          id: listFlickable
          anchors.top: tableTop.bottom
          anchors.left: parent.left
          anchors.right: parent.right
          height: root.listHeight
          contentWidth: width
          contentHeight: diffColumn.height
          clip: true
          boundsBehavior: Flickable.StopAtBounds

          ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

          Column {
            id: diffColumn
            width: listFlickable.width
            spacing: 0

            Repeater {
              model: root.diffRows
              delegate: Rectangle {
                id: diffRow
                required property var modelData
                readonly property bool isCategory: modelData.rowType === "category"
                readonly property bool isGroup: modelData.rowType === "group"
                readonly property bool isOption: modelData.rowType === "option"

                width: diffColumn.width
                height: root.rowHeight
                // Category band (GREY300, :1337) over the flat row surface;
                // group/option rows stay transparent like upstream
                // GREY200-on-GREY200.
                color: diffRow.isCategory ? Theme.bgSurface : "transparent"

                // Category text: Head_13 bold, GREY900, left indent 23
                // (:1343-1347).
                Text {
                  visible: diffRow.isCategory
                  anchors.verticalCenter: parent.verticalCenter
                  anchors.left: parent.left
                  anchors.leftMargin: 23
                  anchors.right: parent.right
                  anchors.rightMargin: 23
                  text: diffRow.modelData.text
                  color: Theme.textPrimary
                  font.pixelSize: Theme.fontSize13
                  font.bold: true
                  elide: Text.ElideRight
                }

                // Group header: Head_13 bold, GREY700, left indent 37
                // (:1382-1386).
                Text {
                  visible: diffRow.isGroup
                  anchors.verticalCenter: parent.verticalCenter
                  anchors.left: parent.left
                  anchors.leftMargin: 37
                  anchors.right: parent.right
                  anchors.rightMargin: 23
                  text: diffRow.modelData.text
                  color: Theme.textSecondary
                  font.pixelSize: Theme.fontSize13
                  font.bold: true
                  elide: Text.ElideRight
                }

                // Option row: three fixed columns 190/150/150 aligned with
                // the header band (:1395-1448), Body_13 regular, values
                // ellipsized at the column end with 5px side margins
                // (:1416-1444).
                Item {
                  visible: diffRow.isOption
                  anchors.fill: parent

                  // Test-locked per-item checkbox
                  // (QmlUiAuditTests.cpp:3372-3377); it lives in the name
                  // column's indent gutter so the 51px text indent and the
                  // three columns stay upstream-aligned.
                  CxCheckBox {
                    anchors.left: parent.left
                    anchors.leftMargin: 23
                    anchors.verticalCenter: parent.verticalCenter
                    checked: diffRow.modelData.key !== ""
                      && root.checkedKeys.indexOf(diffRow.modelData.key) >= 0
                    onToggled: root.toggleKey(diffRow.modelData.key, checked)
                  }
                  Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 51
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.colNameWidth - 51
                    text: diffRow.modelData.text
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSize13
                    elide: Text.ElideRight
                  }
                  Text {
                    anchors.left: parent.left
                    anchors.leftMargin: root.colNameWidth + 5
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.colValueWidth - 10
                    text: root.flatValue(root.configVm ? root.configVm.globalModifiedDefaultValue(diffRow.modelData.key) : "")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSize13
                    elide: Text.ElideRight
                  }
                  Text {
                    anchors.left: parent.left
                    anchors.leftMargin: root.colNameWidth + root.colValueWidth + 5
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.colValueWidth - 10
                    text: root.flatValue(root.configVm ? root.configVm.globalModifiedCurrentValue(diffRow.modelData.key) : "")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSize13
                    elide: Text.ElideRight
                  }
                }
              }
            }
          }
        }
      }

      // Button row: 15px below the list (wxTOP 9 spacer at :957 + the row's
      // own wxTOP 6 border at :1064), right-aligned Transfer -> Discard ->
      // Save with 10DIP gaps (ButtonProps::ChoiceButtonGap,
      // Widgets/Button.hpp:12). Upstream deleted the explicit cancel button
      // (:1053-1062) — the header close box / Esc path keeps action
      // "cancel" (openDialog default + CxDialog reject).
      RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 15
        spacing: Theme.spacingMD

        // Help hyperlink, shown exactly for preset switches
        // (dependent_presets != nullptr, :977-980); upstream teal link color
        // maps to the OWzx brand green.
        Text {
          visible: root.transferAvailable
          text: qsTr("帮助")
          color: helpLinkMouse.containsMouse ? Theme.accentLight : Theme.accent
          font.pixelSize: Theme.fontSizeLG
          font.bold: true
          font.underline: true
          Layout.leftMargin: 2  // upstream wxLEFT 22 minus the 20 column margin
          MouseArea {
            id: helpLinkMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: Qt.openUrlExternally("https://www.orcaslicer.com/wiki/transfer_discard_changes")
          }
          // Upstream HyperLink shows its URL as the tooltip
          // (Widgets/HyperLink.cpp:19-21).
          ToolTip.visible: helpLinkMouse.containsMouse
          ToolTip.text: "https://www.orcaslicer.com/wiki/transfer_discard_changes"
          ToolTip.delay: 400
        }

        Item { Layout.fillWidth: true }

        // Action::Transfer (:1007-1015, ButtonStyle::Confirm).
        CxButton {
          text: qsTr("转移到新预设")
          visible: root.transferAvailable
          enabled: root.checkedKeys.length > 0
          cxStyle: CxButton.Style.Primary
          toolTipText: qsTr("\"%1\" 中修改的所有\"新值\"设置将被转移到 \"%2\"。")
            .arg(root.previousPresetName).arg(root.newPresetName)
          onClicked: { root.action = "transfer"; root.accept() }
        }
        // Action::Discard (:1018-1022).
        CxButton {
          text: qsTr("丢弃修改")
          toolTipText: qsTr("切换到\n\"%1\"\n并丢弃在\n\"%2\"\n中所做的修改。")
            .arg(root.newPresetName).arg(root.previousPresetName)
          onClicked: { root.action = "discard"; root.accept() }
        }
        // Action::Save (:1025).
        CxButton {
          text: qsTr("保存为预设...")
          toolTipText: qsTr("所有\"新值\"设置都保存在 \"%1\" 中，\"%2\" 将不做任何更改直接打开。")
            .arg(root.previousPresetName).arg(root.newPresetName)
          onClicked: { root.action = "save"; root.accept() }
        }
      }
    }
  }
}
