import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
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
            if (root.settingsVm.prefCategory === 9)
                aboutDlg.open()
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
                        { icon: "🖨",  name: qsTr("打印机") },
                        { icon: "🔒",  name: qsTr("账号与隐私") },
                        { icon: "📦",  name: qsTr("更新") },
                        { icon: "🛠",  name: qsTr("高级") },
                        { icon: "🐛",  name: qsTr("开发者") },
                        { icon: "❓",  name: qsTr("关于") }
                    ]
                    delegate: Rectangle {
                        required property var modelData
                        required property int index
                        width: parent.width - 12; x: 6; height: 36; radius: 4
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

                    // User role
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("用户角色"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        Row {
                            spacing: 4
                            Repeater {
                                model: [qsTr("基础"), qsTr("专业")]
                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: 70; height: 28; radius: 4
                                    color: root.settingsVm.userRole === index ? Theme.chromePressed : Theme.bgFloating
                                    border.color: root.settingsVm.userRole === index ? Theme.accent : Theme.bgHover
                                    border.width: 1
                                    Text { anchors.centerIn: parent; text: modelData; color: root.settingsVm.userRole === index ? Theme.accent : Theme.textMuted; font.pixelSize: Theme.fontSizeSM }
                                    MouseArea {
                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: root.settingsVm.setUserRole(index)
                                    }
                                }
                            }
                        }
                    }

                    // Auto-save（对齐上游 auto_save 选项）
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("自动保存"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
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

                    // Check for updates（对齐上游 preset_update/版本检查）
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("启动时检查更新"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            checked: root.settingsVm.checkUpdates
                            onToggled: root.settingsVm.setCheckUpdates(checked)
                        }
                    }

                    // Reduced motion（对齐上游 enable_reduce_motion）
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("减少动画效果"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            checked: root.settingsVm.reducedMotion
                            onToggled: root.settingsVm.setReducedMotion(checked)
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

                    // Region selection（对齐上游 PreferencesDialog region combo）
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("区域设置"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxComboBox {
                            model: [qsTr("跟随系统"), qsTr("中国"), qsTr("美国"), qsTr("欧洲"), qsTr("日本")]
                            currentIndex: root.settingsVm.region
                            onActivated: root.settingsVm.setRegion(currentIndex)
                        }
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

                    // Print host upload
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("切片完成后自动上传"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 200 }
                        CxSwitch {
                            checked: root.settingsVm.autoUpload
                            onToggled: root.settingsVm.setAutoUpload(checked)
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("启用后，切片完成后将自动上传 G-code 到连接的打印机（需先在设备页面连接打印机）。")
                        color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS; wrapMode: Text.Wrap
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

                    // Check for updates button
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("自动检查更新"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            checked: root.settingsVm.checkUpdates
                            onToggled: root.settingsVm.setCheckUpdates(checked)
                        }
                    }

                    // Update channel
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("更新通道"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
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

                    // Check now button
                    Rectangle {
                        Layout.preferredWidth: 140; Layout.preferredHeight: 32; radius: 6
                        enabled: false
                        color: updateBtnMA.containsMouse ? Theme.accentDark : Theme.accentSubtle
                        Text { anchors.centerIn: parent; text: qsTr("检查更新"); color: "white"; font.pixelSize: Theme.fontSizeMD; font.bold: true }
                        MouseArea {
                            id: updateBtnMA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.NoButton
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("当前为 Mock 模式，更新检查功能需要连接更新服务器后启用。")
                        color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS; wrapMode: Text.Wrap
                        Layout.preferredWidth: 400
                    }
                }

                // Account & Privacy settings (对齐上游 PreferencesDialog 账号与隐私, index=5)
                ColumnLayout {
                    visible: root.settingsVm.prefCategory === 5
                    Layout.fillWidth: true; spacing: 16

                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("自动备份项目到云端"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 200 }
                        CxSwitch {
                            checked: root.settingsVm.autoBackup
                            onToggled: root.settingsVm.setAutoBackup(checked)
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("启用后，项目文件将自动备份到您的云端账户。需要先登录云端账号。")
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

                    // Compact/LOD mode（对齐上游 3D view LOD / enable_reduce_detail）
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("低细节模式"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            checked: root.settingsVm.compactMode
                            onToggled: root.settingsVm.setCompactMode(checked)
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("启用后，3D 视口将降低渲染细节以提升性能。适合模型较多或硬件性能不足时使用。")
                        color: Theme.textDisabled
                        font.pixelSize: Theme.fontSizeXS
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: 400
                        visible: root.settingsVm.compactMode
                    }

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
                    visible: root.settingsVm.prefCategory === 8
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

                    // Debug Overlay toggle
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("调试覆盖层"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            checked: root.settingsVm.showDebugOverlay
                            onToggled: root.settingsVm.setShowDebugOverlay(checked)
                        }
                    }

                    // Log Level selector
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
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("详细 G-code"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            checked: root.settingsVm.verboseGcode
                            onToggled: root.settingsVm.setVerboseGcode(checked)
                        }
                    }

                    // OpenGL Debug toggle
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("OpenGL 调试上下文"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; Layout.preferredWidth: 180 }
                        CxSwitch {
                            checked: root.settingsVm.glDebugContext
                            onToggled: root.settingsVm.setGlDebugContext(checked)
                        }
                    }

                    // Max Log Size selector
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
                        text: qsTr("应用")
                        cxStyle: CxButton.Style.Primary
                        onClicked: {
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
