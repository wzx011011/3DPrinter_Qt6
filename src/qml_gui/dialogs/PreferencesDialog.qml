import QtQuick
pragma ComponentBehavior: Bound
import QtQuick.Controls
import QtQuick.Layouts
import OWzx

// PreferencesDialog.qml — upstream hosts Preferences as a modal dialog opened
// from the menu (Preferences.cpp / MainFrame), NOT as a workflow tab (the
// upstream TAB_ID set has 7 tabs). Converted from the former PreferencesPage
// (2026-10-19): same body, CxDialog chrome. Settings apply immediately via
// settingsVm (no upstream OK/Cancel deferred-apply plumbing — noted deviation,
// consistent with the other Cx* tool dialogs).
CxDialog {
    id: root
    dialogTitle: qsTr("偏好设置")
    width: 920
    height: 640
    required property var settingsVm

    AboutDialog {
        id: aboutDlg
    }

    // Q11-⑤: single source of truth for shortcut documentation. The
    // KBShortcutsDialog data tables (aligned with main.qml's real Shortcut
    // registrations and upstream KBShortcutsDialog.cpp pages) are rendered
    // by the dialog itself and by the shortcuts preference page below — no
    // second hand-copied table exists. This invisible instance is that data
    // source; it is never opened from here.
    KBShortcutsDialog {
        id: shortcutsCatalog
        visible: false
    }

    // 切换到《关于》分类时自动弹出
    Connections {
        target: root.settingsVm
        function onPrefCategoryChanged() {
            if (root.settingsVm.prefCategory === 10)
                aboutDlg.open()
        }
    }

    // Advanced-category diagnostics entries (upstream MainFrame hosts these
    // launchers in Preferences). NetworkTestDialog drives the real Backend
    // DNS+HTTPS probe; TroubleshootDialog is the device diagnostics walkthrough.
    NetworkTestDialog {
        id: networkTestDialog
        // Required properties must be wired or Qt 6 aborts the whole page
        // creation (blank page); same wiring as main.qml:1219.
        networkVm: Backend.monitorViewModel
    }

    TroubleshootDialog {
        id: troubleshootDialog
        monitorVm: Backend.monitorViewModel
    }

    // Dead-control elimination: the update check drives the real
    // Backend.checkForUpdates release query (upstream Help > Check for
    // Update, MainFrame.cpp:2160); the result lands here.
    property string updateCheckMessage: qsTr("点击“检查更新”获取最新版本信息。")
    property bool updateCheckOk: false

    Connections {
        target: root.Backend
        function onUpdateCheckFinished(ok, available, message) {
            root.updateCheckOk = ok
            root.updateCheckMessage = message
        }
    }


    Rectangle { anchors.fill: parent; color: Theme.bgBase }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Sidebar categories
        Rectangle {
            Layout.preferredWidth: 200; Layout.fillHeight: true; color: Theme.bgInset

            Column {
                anchors.fill: parent; anchors.topMargin: 12; spacing: 2

                Repeater {
                    // Upstream TabCtrl entries are plain text, no icons
                    // (Preferences.cpp:1487-1489 AppendItem(_L("General")) ...).
                    model: [
                        qsTr("通用"),
                        qsTr("外观"),
                        qsTr("语言"),
                        qsTr("快捷键"),
                        qsTr("打印机"),
                        qsTr("账号与隐私"),
                        qsTr("更新"),
                        qsTr("高级"),
                        qsTr("开发者"),
                        qsTr("AI 助手"),
                        qsTr("关于")
                    ]
                    delegate: Rectangle {
                        required property var modelData
                        required property int index
                        width: parent.width - 12; x: 6; height: 36; radius: 4
                        // Phase 241 (PAGE-04): developerMode gates the
                        // Developer category — the toggle now has a real
                        // consumer (upstream hides debug surfaces the same
                        // way behind the debug build flag).
                        visible: index !== 8 || root.settingsVm.developerMode
                        // Upstream selection is text-only: SetItemBold plus
                        // StateColor #6B6B6C (unselected) / #363636 (selected),
                        // font Body_14 (Preferences.cpp:1477, 1487-1501). No row
                        // fill or border. Theme mapping for the neutralized
                        // palette: chromeTextMuted = dim unselected role,
                        // textPrimary + bold = selected role; fontSizeLG = 14.
                        color: "transparent"

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left; anchors.leftMargin: 12
                            text: modelData
                            color: root.settingsVm.prefCategory === index ? Theme.textPrimary : Theme.chromeTextMuted
                            font.pixelSize: Theme.fontSizeLG
                            font.bold: root.settingsVm.prefCategory === index
                        }
                        TapHandler { onTapped: root.settingsVm.setPrefCategory(index) }
                    }
                }
            }
        }

        // Content area
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true; color: Theme.bgBase

            ColumnLayout {
                anchors.fill: parent; anchors.margins: 24; spacing: 16

                // Pages scroll; the footer stays pinned below (the General
                // page alone is ~700px against a 548px content area, which
                // used to push 自动消失时间 onto the frame line and the
                // 恢复默认/取消/应用 row outside the sheet entirely).
                Flickable {
                    id: prefPagesScroll
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentHeight: pagesColumn.implicitHeight
                    boundsBehavior: Flickable.StopAtBounds

                    ColumnLayout {
                        id: pagesColumn
                        width: prefPagesScroll.availableWidth
                        spacing: 16

                // No page-level title or separator: upstream scrolls straight
                // into the group title rows (create_item_title,
                // Preferences.cpp:244-256).

                // General settings (对齐上游 PreferencesDialog create_general_page, index=0)
                ColumnLayout {
                    visible: root.settingsVm.prefCategory === 0
                    Layout.fillWidth: true; spacing: 16

                    // Show home page on startup
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("启动时显示主页"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            checked: root.settingsVm.showHomePage
                            onToggled: root.settingsVm.setShowHomePage(checked)
                        }
                    }

                    // Multi-device tab (upstream enable_multi_machine, default
                    // off: MainFrame.cpp:1330 gates TAB_ID_MULTI_DEVICE the
                    // same way; default experience shows the Device tab only).
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("启用多设备页签"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            function isEnabled() {
                                const v = AppSettings.value("enable_multi_machine", false)
                                return v === true || v === "true"
                            }
                            checked: isEnabled()
                            onToggled: AppSettings.setValue("enable_multi_machine", checked)
                        }
                    }

                    // Default page
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("默认页面"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        Row {
                            spacing: 4
                            Repeater {
                                model: [qsTr("主页"), qsTr("准备")]
                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: 70; height: 28; radius: 4
                                    color: root.settingsVm.defaultPage === index ? Theme.chromePressed : Theme.bgFloating
                                    border.color: root.settingsVm.defaultPage === index ? Theme.accent : Theme.bgHover
                                    border.width: 1
                                    Text { anchors.centerIn: parent; text: modelData; color: root.settingsVm.defaultPage === index ? Theme.accent : Theme.textMuted; font.pixelSize: Theme.fontSizeSM }
                                    MouseArea {
                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: root.settingsVm.setDefaultPage(index)
                                    }
                                }
                            }
                        }
                    }

                    // Units
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("单位"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        Row {
                            spacing: 4
                            Repeater {
                                model: [qsTr("公制 (mm)"), qsTr("英制 (inch)")]
                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: 90; height: 28; radius: 4
                                    color: root.settingsVm.units === index ? Theme.chromePressed : Theme.bgFloating
                                    border.color: root.settingsVm.units === index ? Theme.accent : Theme.bgHover
                                    border.width: 1
                                    Text { anchors.centerIn: parent; text: modelData; color: root.settingsVm.units === index ? Theme.accent : Theme.textMuted; font.pixelSize: Theme.fontSizeSM }
                                    MouseArea {
                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: root.settingsVm.setUnits(index)
                                    }
                                }
                            }
                        }
                    }

                    // User role row removed (Phase 241 PAGE-04): no upstream
                    // Preferences mapping and no consumer — dead UI must go.

                    // Auto-save / periodic backup（对齐上游 backup_switch，
                    // Preferences.cpp:1179 "Backup your project periodically
                    // for restoring from the occasional crash"）。
                    // Phase 241 (PAGE-04) honest semantics: OWzx writes a
                    // project snapshot to the app-data backup directory every
                    // N minutes while there are unsaved changes (delta from
                    // the upstream seconds-level crash backup timer — the
                    // label says exactly what this build does).
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("定期备份"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            checked: root.settingsVm.autoSave
                            onToggled: root.settingsVm.setAutoSave(checked)
                        }
                        Text { text: qsTr("每"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeSM }
                        CxComboBox {
                            model: ["5", "10", "15", "30"]
                            currentIndex: {
                                var intervals = [5, 10, 15, 30]
                                return intervals.indexOf(root.settingsVm.autoSaveInterval)
                            }
                            enabled: root.settingsVm.autoSave
                            onActivated: root.settingsVm.setAutoSaveInterval(parseInt(model[currentIndex]))
                        }
                        Text { text: qsTr("分钟"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeSM }
                    }

                    Text {
                        // notYetEffectiveHint: this setting IS effective (a
                        // real backup file lands in AppData/backup); the hint
                        // documents the upstream interval-semantics delta.
                        text: qsTr("开启后，每 N 分钟将未保存的项目快照写入应用数据 backup 目录（上游为秒级崩溃备份；本版本为分钟级定期备份）。")
                        color: Theme.textDisabled
                        font.pixelSize: Theme.fontSizeXS
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: 500
                    }

                    // notYetEffectiveHint: persisted only — no startup update
                    // service reads the flag yet; manual checks live on the
                    // 更新 page and drive the real release query.
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("启动时检查更新"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            checked: root.settingsVm.checkUpdates
                            onToggled: root.settingsVm.setCheckUpdates(checked)
                        }
                    }

                    // Notification preferences（对齐上游 notification_manager preferences）
                    Text {
                        text: qsTr("通知设置")
                        color: Theme.chromeText; font.pixelSize: Theme.fontSize13; font.bold: true
                        Layout.topMargin: 8
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("配置通知的显示方式和自动消失行为。通知将在切片完成、导出等操作时弹出。")
                        color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS; wrapMode: Text.Wrap
                        Layout.preferredWidth: 500
                        Layout.bottomMargin: 4
                    }

                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("启用通知"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            checked: root.settingsVm.notificationsEnabled
                            onToggled: root.settingsVm.setNotificationsEnabled(checked)
                        }
                    }

                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("显示提示"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            checked: root.settingsVm.hintsEnabled
                            enabled: root.settingsVm.notificationsEnabled
                            onToggled: root.settingsVm.setHintsEnabled(checked)
                        }
                    }

                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("显示进度通知"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            checked: root.settingsVm.showProgressNotifications
                            enabled: root.settingsVm.notificationsEnabled
                            onToggled: root.settingsVm.setShowProgressNotifications(checked)
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.leftMargin: 196
                        text: qsTr("关闭后将不再显示切片进度弹窗，切片完成后仍会通知。")
                        color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS; wrapMode: Text.Wrap
                        Layout.preferredWidth: 320
                    }

                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("自动消失时间"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxComboBox {
                            model: ["3s", "5s", "8s", "10s", "15s"]
                            currentIndex: {
                                var secs = root.settingsVm.autoDismissSec
                                if (secs <= 3) return 0
                                if (secs <= 5) return 1
                                if (secs <= 8) return 2
                                if (secs <= 10) return 3
                                return 4
                            }
                            onActivated: (index) => {
                                var values = [3, 5, 8, 10, 15]
                                root.settingsVm.setAutoDismissSec(values[index])
                            }
                        }
                    }

                    Text {
                        text: qsTr("区域、云同步和设备上传依赖未迁移的网络服务，当前不可用。")
                        color: Theme.textDisabled
                        font.pixelSize: Theme.fontSizeXS
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: 500
                    }
                }

                // Appearance settings (shown when index=1)
                ColumnLayout {
                    visible: root.settingsVm.prefCategory === 1
                    Layout.fillWidth: true; spacing: 16

                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("界面主题"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 140 }
                        // S6: the theme axis is two-valued (0=dark, 1=light)
                        // since the merged single chain retired Backend's
                        // kThemes 3-tier palette (深蓝/极暗 were dark-shade
                        // palettes with no light-mode story). themeIndex
                        // highlight/set below is unchanged.
                        Repeater {
                            model: [qsTr("深色 (默认)"), qsTr("浅色")]
                            delegate: Rectangle {
                                required property var modelData
                                required property int index
                                width: 100; height: 32; radius: 4
                                color: root.settingsVm.themeIndex === index ? Theme.chromePressed : Theme.bgFloating
                                border.color: root.settingsVm.themeIndex === index ? Theme.accent : Theme.bgHover; border.width: 1
                                Text { anchors.centerIn: parent; text: modelData; color: root.settingsVm.themeIndex === index ? Theme.accent : Theme.textMuted; font.pixelSize: Theme.fontSizeSM }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.settingsVm.setThemeIndex(index)
                                }
                            }
                        }
                    }

                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("字体大小"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 140 }
                        CxSlider {
                            from: 10; to: 16; stepSize: 1; value: root.settingsVm.fontSize
                            Layout.preferredWidth: 200
                            onMoved: root.settingsVm.setFontSize(Math.round(value))
                        }
                        Text { text: root.settingsVm.fontSize + "px"; color: Theme.chromeText; font.pixelSize: Theme.fontSizeSM }
                    }

                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("界面缩放"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 140 }
                        CxComboBox {
                            model: ["100%","125%","150%","175%","200%"]
                            currentIndex: root.settingsVm.uiScaleIndex
                            onActivated: root.settingsVm.setUiScaleIndex(currentIndex)
                        }
                    }

                    // v5.16 (P17.12): G-code preview prefs — upstream hosts the
                    // "G-code Preview" group in the Graphics pane
                    // (Preferences.cpp:1907-1915).
                    Text {
                        text: qsTr("G-code 预览")
                        color: Theme.textPrimary; font.pixelSize: Theme.fontSizeMD; font.bold: true
                    }
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("变暗下层"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 140 }
                        CxSwitch { checked: root.settingsVm.previewDimPreviousLayers; onToggled: root.settingsVm.setPreviewDimPreviousLayers(checked) }
                    }
                    Text {
                        text: qsTr("在切片预览中拖动层滑块时，将当前层之下的各层渲染为变暗状态，仅当前查看的层保持全亮。")
                        color: Theme.textDisabled
                        font.pixelSize: Theme.fontSizeXS
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: 400
                    }
                }

                // Language settings (index=1)
                ColumnLayout {
                    visible: root.settingsVm.prefCategory === 2
                    Layout.fillWidth: true; spacing: 8
                    Repeater {
                        model: [qsTr("简体中文"),"English","日本語","한국어","Deutsch","Français"]
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true; height: 40; radius: 4
                            color: root.settingsVm.languageIndex === index ? Theme.chromePressed : Theme.bgSurface
                            border.color: root.settingsVm.languageIndex === index ? Theme.accent : Theme.chromePressed
                            border.width: 1

                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left; anchors.leftMargin: 16; spacing: 12
                                Text {
                                    text: root.settingsVm.languageIndex === index ? "✓" : " "
                                    color: Theme.accent; font.pixelSize: Theme.fontSize13; font.bold: true
                                }
                                Text {
                                    text: modelData
                                    color: root.settingsVm.languageIndex === index ? Theme.accent : Theme.chromeText
                                    font.pixelSize: Theme.fontSize13
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.settingsVm.setLanguageIndex(index)
                            }
                        }
                    }
                }

                // Shortcuts settings (对齐上游 PreferencesDialog create_key_shortcuts_page, index=3)
                ColumnLayout {
                    visible: root.settingsVm.prefCategory === 3
                    Layout.fillWidth: true; spacing: 12

                    Text {
                        text: qsTr("快捷键绑定")
                        color: Theme.chromeText; font.pixelSize: Theme.fontSize13; font.bold: true
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("快捷键列表与《键盘快捷键》总览对话框同源渲染；部分快捷键仅在特定页面生效。")
                        color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS; wrapMode: Text.Wrap
                        Layout.preferredWidth: 500
                        Layout.bottomMargin: 8
                    }

                    // Q11-⑤: rendered from the KBShortcutsDialog data source
                    // (shortcutsCatalog above) — the single truth that also
                    // feeds the Help-menu dialog and mirrors main.qml's real
                    // Shortcut registrations. The previous hand-copied table
                    // contradicted the actual keymap in several rows (W/E/R
                    // gizmo keys, Ctrl+Shift+X cut, Ctrl+D "clone" vs the real
                    // Delete all) and drifted every time a binding changed.
                    Repeater {
                        model: shortcutsCatalog.groups
                        delegate: ColumnLayout {
                            id: groupSection
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true; spacing: 4

                            Text {
                                text: qsTr("%1 快捷键").arg(groupSection.modelData.name)
                                color: Theme.textMuted; font.pixelSize: Theme.fontSizeSM; font.bold: true
                                Layout.topMargin: index === 0 ? 0 : 8
                            }

                            Repeater {
                                model: shortcutsCatalog.shortcutsForGroup(groupSection.modelData.id)
                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    Layout.fillWidth: true; height: 26; radius: Theme.radiusSM
                                    color: index % 2 === 0 ? Theme.bgInset : "transparent"

                                    Row {
                                        anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
                                        spacing: 16
                                        Text {
                                            width: 180; elide: Text.ElideRight
                                            text: modelData.key
                                            color: Theme.chromeTextMuted; font.pixelSize: Theme.fontSizeXS; font.family: "monospace"
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        Text {
                                            text: modelData.desc
                                            color: Theme.chromeText; font.pixelSize: Theme.fontSizeSM
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Printer settings (对齐上游 PreferencesDialog 打印机设置, index=4)
                ColumnLayout {
                    visible: root.settingsVm.prefCategory === 4
                    Layout.fillWidth: true; spacing: 16

                    Text {
                        text: qsTr("默认打印机设置")
                        color: Theme.chromeText; font.pixelSize: Theme.fontSize13; font.bold: true
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("配置打印机默认参数。此处设置将作为新项目的初始值。")
                        color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS; wrapMode: Text.Wrap
                        Layout.preferredWidth: 500
                        Layout.bottomMargin: 8
                    }

                    // Default nozzle diameter
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("默认喷嘴直径"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        Row {
                            spacing: 4
                            Repeater {
                                model: ["0.2", "0.4", "0.6", "0.8"]
                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: 56; height: 28; radius: 4
                                    color: root.settingsVm.defaultNozzleIndex === index ? Theme.chromePressed : Theme.bgFloating
                                    border.color: root.settingsVm.defaultNozzleIndex === index ? Theme.accent : Theme.bgHover
                                    border.width: 1
                                    Text { anchors.centerIn: parent; text: modelData; color: root.settingsVm.defaultNozzleIndex === index ? Theme.accent : Theme.textMuted; font.pixelSize: Theme.fontSizeSM }
                                    MouseArea {
                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: root.settingsVm.setDefaultNozzleIndex(index)
                                    }
                                }
                            }
                        }
                    }

                    // Default bed shape
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("默认热床形状"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        Row {
                            spacing: 4
                            Repeater {
                                model: [qsTr("矩形"), qsTr("圆形")]
                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: 70; height: 28; radius: 4
                                    color: root.settingsVm.defaultBedShape === index ? Theme.chromePressed : Theme.bgFloating
                                    border.color: root.settingsVm.defaultBedShape === index ? Theme.accent : Theme.bgHover
                                    border.width: 1
                                    Text { anchors.centerIn: parent; text: modelData; color: root.settingsVm.defaultBedShape === index ? Theme.accent : Theme.textMuted; font.pixelSize: Theme.fontSizeSM }
                                    MouseArea {
                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: root.settingsVm.setDefaultBedShape(index)
                                    }
                                }
                            }
                        }
                    }

                    // v5.12 gap-closure: camera settings (对齐上游 Preferences
                    // General tab, Preferences.cpp:1123-1128).
                    Text {
                        text: qsTr("相机设置")
                        color: Theme.textPrimary; font.pixelSize: Theme.fontSizeMD; font.bold: true
                    }
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("相机风格"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 200 }
                        CxComboBox {
                            model: [qsTr("默认"), qsTr("触摸板")]
                            currentIndex: root.settingsVm.cameraNavStyle
                            onActivated: root.settingsVm.setCameraNavStyle(index)
                        }
                    }
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("缩放到鼠标位置"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 200 }
                        CxSwitch { checked: root.settingsVm.zoomToMouse; onToggled: root.settingsVm.setZoomToMouse(checked) }
                    }
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("自由相机"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 200 }
                        CxSwitch { checked: root.settingsVm.freeCamera; onToggled: root.settingsVm.setFreeCamera(checked) }
                    }
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("反转滚轮缩放"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 200 }
                        CxSwitch { checked: root.settingsVm.reverseZoom; onToggled: root.settingsVm.setReverseZoom(checked) }
                    }

                    Text {
                        text: qsTr("自动上传需要设备网络通道，当前不可用。")
                        color: Theme.textDisabled
                        font.pixelSize: Theme.fontSizeXS
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: 400
                    }
                }

                // Updates settings (对齐上游 PreferencesDialog 更新, index=6)
                ColumnLayout {
                    visible: root.settingsVm.prefCategory === 6
                    Layout.fillWidth: true; spacing: 16

                    Text {
                        text: qsTr("软件更新")
                        color: Theme.chromeText; font.pixelSize: Theme.fontSize13; font.bold: true
                    }

                    // Current version info
                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 60; radius: 6
                        color: Theme.bgPanel; border.color: Theme.chromePressed; border.width: 1

                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 12
                            spacing: 4
                            Text {
                                text: qsTr("当前版本：2.4.0-dev (Qt6 Edition)")
                                color: Theme.chromeText; font.pixelSize: Theme.fontSizeMD
                            }
                            Text {
                                text: qsTr("上游基线：OrcaSlicer main branch")
                                color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("更新通道为本地偏好，当前构建未接入自动下载通道，仅记录选择。")
                        color: Theme.textDisabled
                        font.pixelSize: Theme.fontSizeXS; wrapMode: Text.Wrap
                        Layout.preferredWidth: 400
                    }

                    // notYetEffectiveHint: persisted only — no update feed
                    // consumer reads the channel selection yet (upstream feeds
                    // it from the update service channel).
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("更新通道"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 200 }
                        Row {
                            spacing: 4
                            Repeater {
                                model: [qsTr("稳定版"), qsTr("测试版"), qsTr("开发版")]
                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: 80; height: 28; radius: 4
                                    color: root.settingsVm.updateChannel === index ? Theme.chromePressed : Theme.bgFloating
                                    border.color: root.settingsVm.updateChannel === index ? Theme.accent : Theme.bgHover
                                    border.width: 1
                                    Text { anchors.centerIn: parent; text: modelData; color: root.settingsVm.updateChannel === index ? Theme.accent : Theme.textMuted; font.pixelSize: Theme.fontSizeSM }
                                    MouseArea {
                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: root.settingsVm.setUpdateChannel(index)
                                    }
                                }
                            }
                        }
                    }

                    // Check now button (dead-control elimination: drives the
                    // real Backend.checkForUpdates release query).
                    CxButton {
                        Layout.preferredWidth: 140
                        cxStyle: CxButton.Style.Primary
                        enabled: !Backend.updateCheckRunning
                        text: Backend.updateCheckRunning ? qsTr("检查中…") : qsTr("检查更新")
                        onClicked: Backend.checkForUpdates()
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.updateCheckMessage
                        color: root.updateCheckOk ? Theme.statusSuccess : Theme.textDisabled
                        font.pixelSize: Theme.fontSizeXS; wrapMode: Text.Wrap
                        Layout.preferredWidth: 400
                    }
                }

                // Account & Privacy settings (对齐上游 PreferencesDialog 账号与隐私, index=5)
                ColumnLayout {
                    visible: root.settingsVm.prefCategory === 5
                    Layout.fillWidth: true; spacing: 16

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("账号、隐私和云端备份依赖云服务，当前不可用。")
                        color: Theme.textDisabled
                        font.pixelSize: Theme.fontSizeXS
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: 400
                    }
                }

                // Advanced settings (对齐上游 PreferencesDialog 高级, index=7)
                ColumnLayout {
                    visible: root.settingsVm.prefCategory === 7
                    Layout.fillWidth: true; spacing: 16

                    // Phase 236 (DLG-01): dialog launchers that upstream hosts
                    // in Preferences (AMS / Firmware / Plugin Manager / Lite
                    // Mode). Each opens the already-instantiated main.qml or
                    // page-local dialog via the BackendContext request
                    // signals. NetworkTestDialog drives the real Backend
                    // probe; FirmwareDialog discloses that OTA is not
                    // integrated instead of simulating an update. (Q11-③:
                    // the SpeedLimit launcher is gone with the pure-mock
                    // SpeedLimitDialog.)
                    Text {
                        text: qsTr("工具与设备对话框")
                        color: Theme.chromeText; font.pixelSize: Theme.fontSize13; font.bold: true
                    }

                    RowLayout {
                        spacing: Theme.spacingSM
                        CxButton {
                            text: qsTr("AMS 设置…")
                            onClicked: Backend.showAMSSettingsDialog()
                        }
                        CxButton {
                            text: qsTr("固件…")
                            onClicked: Backend.showFirmwareDialog()
                        }
                        CxButton {
                            text: qsTr("插件管理…")
                            onClicked: Backend.showPluginManagerDialog()
                        }
                        CxButton {
                            text: qsTr("精简模式…")
                            onClicked: Backend.showEnableLiteModeDialog()
                        }
                        CxButton {
                            text: qsTr("擦料塔…")
                            onClicked: Backend.showWipeTowerDialog()
                        }
                    }

                    RowLayout {
                        spacing: Theme.spacingSM
                        CxButton {
                            text: qsTr("网络测试…")
                            onClicked: networkTestDialog.open()
                        }
                        CxButton {
                            text: qsTr("设备排错…")
                            onClicked: troubleshootDialog.open()
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("文件关联和单实例运行依赖未迁移的平台服务，当前不可用。")
                        color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS; wrapMode: Text.Wrap
                        Layout.preferredWidth: 500
                    }

                    // Low-detail mode row removed (Phase 241 PAGE-04): no
                    // upstream Preferences mapping (enable_reduce_detail does
                    // not exist upstream) and no renderer consumer — dead UI.

                    // Undo stack limit（对齐上游 undo/redo 历史限制）
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("撤销栈上限"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        Row {
                            spacing: 4
                            Repeater {
                                model: [20, 50, 100, 200]
                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: 56; height: 28; radius: 4
                                    color: root.settingsVm.undoLimit === modelData ? Theme.chromePressed : Theme.bgFloating
                                    border.color: root.settingsVm.undoLimit === modelData ? Theme.accent : Theme.bgHover
                                    border.width: 1
                                    Text { anchors.centerIn: parent; text: modelData; color: root.settingsVm.undoLimit === modelData ? Theme.accent : Theme.textMuted; font.pixelSize: Theme.fontSizeSM }
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.settingsVm.setUndoLimit(modelData)
                                    }
                                }
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("设置撤销/重做的历史记录上限。值越大可回退的操作越多，但占用更多内存。")
                        color: Theme.textDisabled
                        font.pixelSize: Theme.fontSizeXS
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: 400
                    }
                }

                // Developer settings (对齐上游 PreferencesDialog::create_debug_page, index=8)
                ColumnLayout {
                    // Phase 241 (PAGE-04): content hidden with its sidebar
                    // entry when developerMode is off.
                    visible: root.settingsVm.prefCategory === 8
                             && root.settingsVm.developerMode
                    Layout.fillWidth: true; spacing: 16

                    Text {
                        text: qsTr("开发者选项")
                        color: Theme.chromeText; font.pixelSize: Theme.fontSize13; font.bold: true
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("这些选项面向开发者调试使用，普通用户无需更改。")
                        color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS; wrapMode: Text.Wrap
                        Layout.preferredWidth: 500
                        Layout.bottomMargin: 8
                    }

                    // Developer Mode toggle
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("开发者模式"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            checked: root.settingsVm.developerMode
                            onToggled: root.settingsVm.setDeveloperMode(checked)
                        }
                    }

                    Text {
                        // notYetEffectiveHint: developerMode IS effective (it
                        // shows/hides this whole category); the knobs below
                        // persist only.
                        text: qsTr("开发者模式控制本分类的显隐；下方各项当前仅持久化，尚未接入运行时。")
                        color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS; wrapMode: Text.Wrap
                        Layout.preferredWidth: 500
                    }

                    // Debug Overlay toggle
                    // notYetEffectiveHint: persisted only — no renderer
                    // consumer reads the overlay flag yet.
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("调试覆盖层"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            checked: root.settingsVm.showDebugOverlay
                            onToggled: root.settingsVm.setShowDebugOverlay(checked)
                        }
                    }

                    // Log Level selector
                    // notYetEffectiveHint: persisted only — logging rules are
                    // compiled in; the level selection has no runtime reader.
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("日志级别"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxComboBox {
                            model: [qsTr("Error"), qsTr("Warning"), qsTr("Info"), qsTr("Debug"), qsTr("Trace")]
                            currentIndex: root.settingsVm.logLevel
                            onActivated: root.settingsVm.setLogLevel(currentIndex)
                        }
                    }

                    // Verbose G-code toggle
                    // notYetEffectiveHint: persisted only — no G-code export
                    // consumer reads the verbosity flag yet.
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("详细 G-code"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            checked: root.settingsVm.verboseGcode
                            onToggled: root.settingsVm.setVerboseGcode(checked)
                        }
                    }

                    // OpenGL Debug toggle
                    // notYetEffectiveHint: persisted only — the GL context is
                    // created before preferences load; takes effect in a test
                    // harness, not the running window.
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("OpenGL 调试上下文"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            checked: root.settingsVm.glDebugContext
                            onToggled: root.settingsVm.setGlDebugContext(checked)
                        }
                    }

                    // Max Log Size selector
                    // notYetEffectiveHint: persisted only — log rotation is
                    // not implemented, so the size cap has no runtime reader.
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("最大日志大小 (MB)"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxComboBox {
                            model: ["10", "25", "50", "100", "200"]
                            currentIndex: {
                                var sizes = [10, 25, 50, 100, 200]
                                return sizes.indexOf(root.settingsVm.maxLogSizeMb)
                            }
                            onActivated: root.settingsVm.setMaxLogSizeMb(parseInt(model[currentIndex]))
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("日志文件达到指定大小后将自动轮转。增大此值可保留更多历史日志，但占用更多磁盘空间。")
                        color: Theme.textDisabled
                        font.pixelSize: Theme.fontSizeXS
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: 400
                    }
                }

                // ── AI 助手设置（OWzx-only 决策，docs/ai-control.md, index=9）──
                ColumnLayout {
                    visible: root.settingsVm.prefCategory === 9
                    Layout.fillWidth: true; spacing: 16

                    Text {
                        text: qsTr("AI 助手")
                        color: Theme.chromeText; font.pixelSize: Theme.fontSize13; font.bold: true
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("内嵌 Agent harness（Claude Agent SDK sidecar）+ GLM，可通过聊天控制整个软件（可见与不可见功能）。破坏性操作会在聊天中弹出确认卡。")
                        color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS; wrapMode: Text.Wrap
                        Layout.preferredWidth: 500
                        Layout.bottomMargin: 8
                    }

                    // Enable toggle
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("启用 AI 助手"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            checked: root.settingsVm.aiEnabled
                            onToggled: root.settingsVm.setAiEnabled(checked)
                        }
                    }

                    // API Key
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("API Key"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxTextField {
                            Layout.preferredWidth: 320
                            placeholderText: qsTr("智谱开放平台 API Key")
                            echoMode: TextInput.Password
                            text: root.settingsVm.aiApiKey
                            onEditingFinished: root.settingsVm.setAiApiKey(text)
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("仅在本地保存（QSettings），用于直连智谱 Anthropic 兼容端点；软件本身不中转、不上传。")
                        color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS; wrapMode: Text.Wrap
                        Layout.preferredWidth: 500
                    }

                    // Model
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("模型"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxTextField {
                            Layout.preferredWidth: 320
                            placeholderText: "glm-5.3-flash"
                            text: root.settingsVm.aiModel
                            onEditingFinished: root.settingsVm.setAiModel(text)
                        }
                    }

                    // Base URL
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("API 端点"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxTextField {
                            Layout.preferredWidth: 320
                            placeholderText: "https://open.bigmodel.cn/api/anthropic"
                            text: root.settingsVm.aiBaseUrl
                            onEditingFinished: root.settingsVm.setAiBaseUrl(text)
                        }
                    }

                    // MCP port
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("本地控制端口"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSpinBox {
                            from: 1024; to: 65535
                            value: root.settingsVm.aiPort
                            onValueModified: root.settingsVm.setAiPort(value)
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("AI 通过 127.0.0.1 上的 MCP 服务器控制软件，仅监听本机并使用随机 Token 鉴权。修改端口后需重新启用。")
                        color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS; wrapMode: Text.Wrap
                        Layout.preferredWidth: 500
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("还需要安装 AI 组件（ai_sidecar 目录，约 350MB 可选包）：运行 scripts/package_ai_sidecar.ps1 或从发布页下载。")
                        color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS; wrapMode: Text.Wrap
                        Layout.preferredWidth: 500
                    }
                }

                    }
                }

                // Save/Restore buttons
                RowLayout {
                    spacing: 12
                    Item { Layout.fillWidth: true }

                    // 实时生效提示
                    Text {
                        id: appliedHint
                        text: qsTr("✓ 已实时生效")
                        color: Theme.accent
                        font.pixelSize: Theme.fontSizeSM
                        opacity: 0
                        Behavior on opacity { NumberAnimation { duration: 300 } }
                    }

                    CxButton {
                        text: qsTr("恢复默认")
                        onClicked: root.settingsVm.resetPreferences()
                    }
                    CxButton {
                        text: qsTr("取消")
                        onClicked: root.settingsVm.cancelPreferences()
                    }
                    CxButton {
                        text: qsTr("应用")
                        cxStyle: CxButton.Style.Primary
                        onClicked: {
                            root.settingsVm.applyPreferences()
                            appliedHint.opacity = 1
                            appliedHintTimer.restart()
                        }
                    }

                    Timer {
                        id: appliedHintTimer
                        interval: 2000
                        onTriggered: appliedHint.opacity = 0
                    }
                }
            }
        }
    }
}
