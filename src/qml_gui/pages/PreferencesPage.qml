import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../dialogs"
import "../controls"

Item {
    id: root
    required property var settingsVm
    property var backend

    AboutDialog {
        id: aboutDlg
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
    // launchers in Preferences). NetworkTestDialog drives the real backend
    // DNS+HTTPS probe; TroubleshootDialog is the device diagnostics walkthrough.
    NetworkTestDialog {
        id: networkTestDialog
    }

    TroubleshootDialog {
        id: troubleshootDialog
    }

    // Dead-control elimination: the update check drives the real
    // backend.checkForUpdates release query (upstream Help > Check for
    // Update, MainFrame.cpp:2160); the result lands here.
    property string updateCheckMessage: qsTr("点击“检查更新”获取最新版本信息。")
    property bool updateCheckOk: false

    Connections {
        target: root.backend
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
                    model: [
                        { icon: "⚙",  name: qsTr("通用") },
                        { icon: "🎨", name: qsTr("外观") },
                        { icon: "🌍", name: qsTr("语言") },
                        { icon: "⌨",  name: qsTr("快捷键") },
                        { icon: "🖨", name: qsTr("打印机") },
                        { icon: "🔒", name: qsTr("账号与隐私") },
                        { icon: "📦", name: qsTr("更新") },
                        { icon: "🛠", name: qsTr("高级") },
                        { icon: "🐛", name: qsTr("开发者") },
                        { icon: "🤖", name: qsTr("AI 助手") },
                        { icon: "❓", name: qsTr("关于") }
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
                        color: root.settingsVm.prefCategory === index ? Theme.chromePressed
                             : (catHov.containsMouse ? Theme.bgPanel : "transparent")
                        border.color: root.settingsVm.prefCategory === index ? Theme.accent : "transparent"
                        border.width: 1

                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left; anchors.leftMargin: 12; spacing: 10
                            Text { text: modelData.icon; font.pixelSize: Theme.fontSizeLG }
                            Text { text: modelData.name; color: Theme.chromeText; font.pixelSize: Theme.fontSizeMD }
                        }
                        HoverHandler { id: catHov }
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

                Text {
                    text: root.settingsVm.prefCategoryTitle; color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeXL; font.bold: true
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.scrollBarTrackColor }

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
                        Repeater {
                            model: [qsTr("深色 (默认)"), qsTr("深蓝"), qsTr("极暗")]
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
                        text: qsTr("以下为当前版本支持的快捷键列表。部分快捷键仅在特定页面生效。")
                        color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS; wrapMode: Text.Wrap
                        Layout.preferredWidth: 500
                        Layout.bottomMargin: 8
                    }

                    // Shortcut table header
                    Rectangle {
                        Layout.fillWidth: true; height: 28; radius: 4; color: Theme.bgPanel
                        Row {
                            anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
                            spacing: 8
                            Text { text: qsTr("功能"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS; font.bold: true; width: 160 }
                            Text { text: qsTr("快捷键"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS; font.bold: true; width: 120 }
                            Text { text: qsTr("页面"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS; font.bold: true }
                        }
                    }

                    Repeater {
                        model: [
                            { action: qsTr("新建项目"), key: "Ctrl+N", scope: qsTr("全局") },
                            { action: qsTr("打开项目"), key: "Ctrl+O", scope: qsTr("全局") },
                            { action: qsTr("保存项目"), key: "Ctrl+S", scope: qsTr("全局") },
                            { action: qsTr("另存为"), key: "Ctrl+Shift+S", scope: qsTr("全局") },
                            { action: qsTr("撤销"), key: "Ctrl+Z", scope: qsTr("全局") },
                            { action: qsTr("重做"), key: "Ctrl+Y", scope: qsTr("全局") },
                            { action: qsTr("删除选中"), key: "Delete", scope: qsTr("准备") },
                            { action: qsTr("全选"), key: "Ctrl+A", scope: qsTr("准备") },
                            { action: qsTr("取消选择"), key: "Escape", scope: qsTr("准备") },
                            { action: qsTr("复制"), key: "Ctrl+C", scope: qsTr("准备") },
                            { action: qsTr("粘贴"), key: "Ctrl+V", scope: qsTr("准备") },
                            { action: qsTr("剪切"), key: "Ctrl+X", scope: qsTr("准备") },
                            { action: qsTr("克隆选中"), key: "Ctrl+D", scope: qsTr("准备") },
                            { action: qsTr("搜索设置"), key: "Ctrl+F", scope: qsTr("设置") },
                            { action: qsTr("偏好设置"), key: "Ctrl+P", scope: qsTr("全局") },
                            { action: qsTr("移动模式"), key: "W", scope: qsTr("准备") },
                            { action: qsTr("旋转模式"), key: "E", scope: qsTr("准备") },
                            { action: qsTr("缩放模式"), key: "R", scope: qsTr("准备") },
                            { action: qsTr("平放"), key: "G", scope: qsTr("准备") },
                            { action: qsTr("切割"), key: "Ctrl+Shift+X", scope: qsTr("准备") },
                            { action: qsTr("适应视图"), key: "F", scope: qsTr("准备") },
                            { action: qsTr("俯视"), key: "Ctrl+0", scope: qsTr("准备/预览") },
                            { action: qsTr("前视"), key: "Ctrl+1", scope: qsTr("准备/预览") },
                            { action: qsTr("右视"), key: "Ctrl+3", scope: qsTr("准备/预览") },
                            { action: qsTr("等轴视"), key: "Ctrl+6", scope: qsTr("准备/预览") },
                            { action: qsTr("测量"), key: "Ctrl+U", scope: qsTr("准备") },
                            { action: qsTr("播放/暂停"), key: "Space", scope: qsTr("预览") },
                            { action: qsTr("跳转前100步"), key: "←", scope: qsTr("预览") },
                            { action: qsTr("跳转后100步"), key: "→", scope: qsTr("预览") },
                            { action: qsTr("跳到开头"), key: "Home", scope: qsTr("预览") },
                            { action: qsTr("跳到结尾"), key: "End", scope: qsTr("预览") }
                        ]
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true; height: 30; radius: 3
                            color: index % 2 === 0 ? Theme.bgInset : "transparent"

                            Row {
                                anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
                                spacing: 8
                                Text { text: modelData.action; color: Theme.chromeText; font.pixelSize: Theme.fontSizeSM; width: 160 }
                                Rectangle {
                                    width: modelData.key.length * 8 + 12; height: 20; radius: 3
                                    color: Theme.scrollBarTrackColor; border.color: Theme.bgHover; border.width: 1
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text { anchors.centerIn: parent; text: modelData.key; color: Theme.chromeTextMuted; font.pixelSize: Theme.fontSizeXS; font.family: "monospace" }
                                }
                                Text { text: modelData.scope; color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS }
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
                    // real backend.checkForUpdates release query).
                    Rectangle {
                        Layout.preferredWidth: 140; Layout.preferredHeight: 32; radius: 6
                        opacity: backend.updateCheckRunning ? 0.6 : 1.0
                        color: updateBtnMA.containsMouse ? Theme.accentDark : Theme.accentSubtle
                        Text { anchors.centerIn: parent; text: backend.updateCheckRunning ? qsTr("检查中…") : qsTr("检查更新"); color: "white"; font.pixelSize: Theme.fontSizeMD; font.bold: true }
                        MouseArea {
                            id: updateBtnMA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: if (!backend.updateCheckRunning) backend.checkForUpdates()
                        }
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
                    // in Preferences (AMS / Firmware / SpeedLimit / Plugin
                    // Manager / Lite Mode). Each opens the already-instantiated
                    // main.qml or page-local dialog via the BackendContext
                    // request signals. NetworkTestDialog drives the real
                    // backend probe; FirmwareDialog discloses that OTA is not
                    // integrated instead of simulating an update.
                    Text {
                        text: qsTr("工具与设备对话框")
                        color: Theme.chromeText; font.pixelSize: Theme.fontSize13; font.bold: true
                    }

                    RowLayout {
                        spacing: Theme.spacingSM
                        CxButton {
                            text: qsTr("AMS 设置…")
                            onClicked: backend.showAMSSettingsDialog()
                        }
                        CxButton {
                            text: qsTr("固件…")
                            onClicked: backend.showFirmwareDialog()
                        }
                        CxButton {
                            text: qsTr("速度限制…")
                            onClicked: backend.showSpeedLimitDialog()
                        }
                        CxButton {
                            text: qsTr("插件管理…")
                            onClicked: backend.showPluginManagerDialog()
                        }
                        CxButton {
                            text: qsTr("精简模式…")
                            onClicked: backend.showEnableLiteModeDialog()
                        }
                        CxButton {
                            text: qsTr("擦料塔…")
                            onClicked: backend.showWipeTowerDialog()
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

                Item { Layout.fillHeight: true }

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
