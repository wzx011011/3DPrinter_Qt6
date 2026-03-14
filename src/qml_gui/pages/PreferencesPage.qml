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
            if (root.settingsVm.prefCategory === 8)
                aboutDlg.open()
        }
    }

    Rectangle { anchors.fill: parent; color: "#0d0f12" }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Sidebar categories
        Rectangle {
            Layout.preferredWidth: 200; Layout.fillHeight: true; color: "#0f1218"

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
                        { icon: "❓",  name: qsTr("关于") }
                    ]
                    delegate: Rectangle {
                        required property var modelData
                        required property int index
                        width: parent.width - 12; x: 6; height: 36; radius: 4
                        color: root.settingsVm.prefCategory === index ? "#1c2a3e"
                             : (catHov.containsMouse ? "#151c28" : "transparent")
                        border.color: root.settingsVm.prefCategory === index ? "#18c75e" : "transparent"
                        border.width: 1

                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left; anchors.leftMargin: 12; spacing: 10
                            Text { text: modelData.icon; font.pixelSize: 14 }
                            Text { text: modelData.name; color: "#c8d4e0"; font.pixelSize: 12 }
                        }
                        HoverHandler { id: catHov }
                        TapHandler { onTapped: root.settingsVm.setPrefCategory(index) }
                    }
                }
            }
        }

        // Content area
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true; color: "#0d0f12"

            ColumnLayout {
                anchors.fill: parent; anchors.margins: 24; spacing: 16

                Text {
                    text: root.settingsVm.prefCategoryTitle; color: "#e8edf6"
                    font.pixelSize: 16; font.bold: true
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#1e2430" }

                // General settings (对齐上游 PreferencesDialog create_general_page, index=0)
                ColumnLayout {
                    visible: root.settingsVm.prefCategory === 0
                    Layout.fillWidth: true; spacing: 16

                    // Show home page on startup
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("启动时显示主页"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 180 }
                        Switch {
                            checked: root.settingsVm.showHomePage
                            onToggled: root.settingsVm.setShowHomePage(checked)
                        }
                    }

                    // Default page
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("默认页面"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 180 }
                        Row {
                            spacing: 4
                            Repeater {
                                model: [qsTr("主页"), qsTr("准备")]
                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: 70; height: 28; radius: 4
                                    color: root.settingsVm.defaultPage === index ? "#1c2a3e" : "#1a1e28"
                                    border.color: root.settingsVm.defaultPage === index ? "#18c75e" : "#2e3444"
                                    border.width: 1
                                    Text { anchors.centerIn: parent; text: modelData; color: root.settingsVm.defaultPage === index ? "#18c75e" : "#8a96a8"; font.pixelSize: 11 }
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
                        Text { text: qsTr("单位"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 180 }
                        Row {
                            spacing: 4
                            Repeater {
                                model: [qsTr("公制 (mm)"), qsTr("英制 (inch)")]
                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: 90; height: 28; radius: 4
                                    color: root.settingsVm.units === index ? "#1c2a3e" : "#1a1e28"
                                    border.color: root.settingsVm.units === index ? "#18c75e" : "#2e3444"
                                    border.width: 1
                                    Text { anchors.centerIn: parent; text: modelData; color: root.settingsVm.units === index ? "#18c75e" : "#8a96a8"; font.pixelSize: 11 }
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
                        Text { text: qsTr("用户角色"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 180 }
                        Row {
                            spacing: 4
                            Repeater {
                                model: [qsTr("基础"), qsTr("专业")]
                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: 70; height: 28; radius: 4
                                    color: root.settingsVm.userRole === index ? "#1c2a3e" : "#1a1e28"
                                    border.color: root.settingsVm.userRole === index ? "#18c75e" : "#2e3444"
                                    border.width: 1
                                    Text { anchors.centerIn: parent; text: modelData; color: root.settingsVm.userRole === index ? "#18c75e" : "#8a96a8"; font.pixelSize: 11 }
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
                        Text { text: qsTr("自动保存"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 180 }
                        Switch {
                            checked: root.settingsVm.autoSave
                            onToggled: root.settingsVm.setAutoSave(checked)
                        }
                        Text { text: qsTr("每"); color: "#6b7d94"; font.pixelSize: 11 }
                        ComboBox {
                            model: ["5", "10", "15", "30"]
                            currentIndex: {
                                var intervals = [5, 10, 15, 30]
                                return intervals.indexOf(root.settingsVm.autoSaveInterval)
                            }
                            implicitWidth: 60
                            implicitHeight: 28
                            enabled: root.settingsVm.autoSave
                            onActivated: root.settingsVm.setAutoSaveInterval(parseInt(model[currentIndex]))
                        }
                        Text { text: qsTr("分钟"); color: "#6b7d94"; font.pixelSize: 11 }
                    }

                    // Check for updates（对齐上游 preset_update/版本检查）
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("启动时检查更新"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 180 }
                        Switch {
                            checked: root.settingsVm.checkUpdates
                            onToggled: root.settingsVm.setCheckUpdates(checked)
                        }
                    }

                    // Reduced motion（对齐上游 enable_reduce_motion）
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("减少动画效果"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 180 }
                        Switch {
                            checked: root.settingsVm.reducedMotion
                            onToggled: root.settingsVm.setReducedMotion(checked)
                        }
                    }

                    // Notification preferences（对齐上游 notification_manager preferences）
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("启用通知"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 180 }
                        Switch {
                            checked: backend.notificationsEnabled
                            onToggled: backend.setNotificationsEnabled(checked)
                        }
                    }

                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("显示提示"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 180 }
                        Switch {
                            checked: backend.hintsEnabled
                            enabled: backend.notificationsEnabled
                            onToggled: backend.setHintsEnabled(checked)
                        }
                    }

                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("自动消失时间"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 180 }
                        CxComboBox {
                            model: ["3s", "5s", "8s", "10s", "15s"]
                            currentIndex: {
                                var secs = backend.autoDismissSec
                                if (secs <= 3) return 0
                                if (secs <= 5) return 1
                                if (secs <= 8) return 2
                                if (secs <= 10) return 3
                                return 4
                            }
                            onActivated: (index) => {
                                var values = [3, 5, 8, 10, 15]
                                backend.setAutoDismissSec(values[index])
                            }
                        }
                    }

                    // Region selection（对齐上游 PreferencesDialog region combo）
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("区域设置"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 180 }
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
                        Text { text: qsTr("界面主题"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 140 }
                        Repeater {
                            model: [qsTr("深色 (默认)"), qsTr("深蓝"), qsTr("极暗")]
                            delegate: Rectangle {
                                required property var modelData
                                required property int index
                                width: 100; height: 32; radius: 4
                                color: root.settingsVm.themeIndex === index ? "#1c2a3e" : "#1a1e28"
                                border.color: root.settingsVm.themeIndex === index ? "#18c75e" : "#2e3444"; border.width: 1
                                Text { anchors.centerIn: parent; text: modelData; color: root.settingsVm.themeIndex === index ? "#18c75e" : "#8a96a8"; font.pixelSize: 11 }
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
                        Text { text: qsTr("字体大小"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 140 }
                        Slider {
                            from: 10; to: 16; stepSize: 1; value: root.settingsVm.fontSize
                            Layout.preferredWidth: 200
                            onMoved: root.settingsVm.setFontSize(Math.round(value))
                        }
                        Text { text: root.settingsVm.fontSize + "px"; color: "#c8d4e0"; font.pixelSize: 11 }
                    }

                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("界面缩放"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 140 }
                        ComboBox {
                            model: ["100%","125%","150%","175%","200%"]
                            currentIndex: root.settingsVm.uiScaleIndex
                            implicitWidth: 120
                            implicitHeight: 28
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
                            color: root.settingsVm.languageIndex === index ? "#1c2a3e" : "#13181f"
                            border.color: root.settingsVm.languageIndex === index ? "#18c75e" : "#252c38"
                            border.width: 1

                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left; anchors.leftMargin: 16; spacing: 12
                                Text {
                                    text: root.settingsVm.languageIndex === index ? "✓" : " "
                                    color: "#18c75e"; font.pixelSize: 13; font.bold: true
                                }
                                Text {
                                    text: modelData
                                    color: root.settingsVm.languageIndex === index ? "#18c75e" : "#c8d4e0"
                                    font.pixelSize: 13
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
                        color: "#c8d4e0"; font.pixelSize: 13; font.bold: true
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("以下为当前版本支持的快捷键列表。部分快捷键仅在特定页面生效。")
                        color: "#566070"; font.pixelSize: 10; wrapMode: Text.Wrap
                        Layout.preferredWidth: 500
                        Layout.bottomMargin: 8
                    }

                    // Shortcut table header
                    Rectangle {
                        Layout.fillWidth: true; height: 28; radius: 4; color: "#151c28"
                        Row {
                            anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
                            spacing: 8
                            Text { text: qsTr("功能"); color: "#8a96a8"; font.pixelSize: 10; font.bold: true; width: 160 }
                            Text { text: qsTr("快捷键"); color: "#8a96a8"; font.pixelSize: 10; font.bold: true; width: 120 }
                            Text { text: qsTr("页面"); color: "#8a96a8"; font.pixelSize: 10; font.bold: true }
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
                            color: index % 2 === 0 ? "#0f1418" : "transparent"

                            Row {
                                anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
                                spacing: 8
                                Text { text: modelData.action; color: "#c8d4e0"; font.pixelSize: 11; width: 160 }
                                Rectangle {
                                    width: modelData.key.length * 8 + 12; height: 20; radius: 3
                                    color: "#1e2430"; border.color: "#2e3540"; border.width: 1
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text { anchors.centerIn: parent; text: modelData.key; color: "#80cbc4"; font.pixelSize: 10; font.family: "monospace" }
                                }
                                Text { text: modelData.scope; color: "#6b7d94"; font.pixelSize: 10 }
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
                        color: "#c8d4e0"; font.pixelSize: 13; font.bold: true
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("配置打印机默认参数。此处设置将作为新项目的初始值。")
                        color: "#566070"; font.pixelSize: 10; wrapMode: Text.Wrap
                        Layout.preferredWidth: 500
                        Layout.bottomMargin: 8
                    }

                    // Default nozzle diameter
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("默认喷嘴直径"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 180 }
                        Row {
                            spacing: 4
                            Repeater {
                                model: ["0.2", "0.4", "0.6", "0.8"]
                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: 56; height: 28; radius: 4
                                    color: root.settingsVm.defaultNozzleIndex === index ? "#1c2a3e" : "#1a1e28"
                                    border.color: root.settingsVm.defaultNozzleIndex === index ? "#18c75e" : "#2e3444"
                                    border.width: 1
                                    Text { anchors.centerIn: parent; text: modelData; color: root.settingsVm.defaultNozzleIndex === index ? "#18c75e" : "#8a96a8"; font.pixelSize: 11 }
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
                        Text { text: qsTr("默认热床形状"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 180 }
                        Row {
                            spacing: 4
                            Repeater {
                                model: [qsTr("矩形"), qsTr("圆形")]
                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: 70; height: 28; radius: 4
                                    color: root.settingsVm.defaultBedShape === index ? "#1c2a3e" : "#1a1e28"
                                    border.color: root.settingsVm.defaultBedShape === index ? "#18c75e" : "#2e3444"
                                    border.width: 1
                                    Text { anchors.centerIn: parent; text: modelData; color: root.settingsVm.defaultBedShape === index ? "#18c75e" : "#8a96a8"; font.pixelSize: 11 }
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
                        Text { text: qsTr("切片完成后自动上传"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 200 }
                        Switch {
                            checked: root.settingsVm.autoUpload
                            onToggled: root.settingsVm.setAutoUpload(checked)
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("启用后，切片完成后将自动上传 G-code 到连接的打印机（需先在设备页面连接打印机）。")
                        color: "#566070"; font.pixelSize: 10; wrapMode: Text.Wrap
                        Layout.preferredWidth: 400
                    }
                }

                // Updates settings (对齐上游 PreferencesDialog 更新, index=6)
                ColumnLayout {
                    visible: root.settingsVm.prefCategory === 6
                    Layout.fillWidth: true; spacing: 16

                    Text {
                        text: qsTr("软件更新")
                        color: "#c8d4e0"; font.pixelSize: 13; font.bold: true
                    }

                    // Current version info
                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 60; radius: 6
                        color: "#151c28"; border.color: "#252c38"; border.width: 1

                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 12
                            spacing: 4
                            Text {
                                text: qsTr("当前版本：v7.0.1 (Qt6 Edition)")
                                color: "#c8d4e0"; font.pixelSize: 12
                            }
                            Text {
                                text: qsTr("上游基线：CrealityPrint v7.0.1 (0d4ac73)")
                                color: "#6b7d94"; font.pixelSize: 10
                            }
                        }
                    }

                    // Check for updates button
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("自动检查更新"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 180 }
                        Switch {
                            checked: root.settingsVm.checkUpdates
                            onToggled: root.settingsVm.setCheckUpdates(checked)
                        }
                    }

                    // Update channel
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("更新通道"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 180 }
                        Row {
                            spacing: 4
                            Repeater {
                                model: [qsTr("稳定版"), qsTr("测试版"), qsTr("开发版")]
                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: 80; height: 28; radius: 4
                                    color: root.settingsVm.updateChannel === index ? "#1c2a3e" : "#1a1e28"
                                    border.color: root.settingsVm.updateChannel === index ? "#18c75e" : "#2e3444"
                                    border.width: 1
                                    Text { anchors.centerIn: parent; text: modelData; color: root.settingsVm.updateChannel === index ? "#18c75e" : "#8a96a8"; font.pixelSize: 11 }
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
                        color: updateBtnMA.containsMouse ? "#19a84e" : "#157a39"
                        Text { anchors.centerIn: parent; text: qsTr("检查更新"); color: "white"; font.pixelSize: 12; font.bold: true }
                        MouseArea {
                            id: updateBtnMA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: {} // Mock: no real update check
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("当前为 Mock 模式，更新检查功能需要连接 Creality 更新服务器后启用。")
                        color: "#566070"; font.pixelSize: 10; wrapMode: Text.Wrap
                        Layout.preferredWidth: 400
                    }
                }

                // Account & Privacy settings (对齐上游 PreferencesDialog 账号与隐私, index=5)
                ColumnLayout {
                    visible: root.settingsVm.prefCategory === 5
                    Layout.fillWidth: true; spacing: 16

                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("自动备份项目到云端"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 200 }
                        Switch {
                            checked: root.settingsVm.autoBackup
                            onToggled: root.settingsVm.setAutoBackup(checked)
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("启用后，项目文件将自动备份到您的云端账户。需要先登录云端账号。")
                        color: "#566070"
                        font.pixelSize: 10
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
                        Text { text: qsTr("低细节模式"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 180 }
                        Switch {
                            checked: root.settingsVm.compactMode
                            onToggled: root.settingsVm.setCompactMode(checked)
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("启用后，3D 视口将降低渲染细节以提升性能。适合模型较多或硬件性能不足时使用。")
                        color: "#566070"
                        font.pixelSize: 10
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: 400
                        visible: root.settingsVm.compactMode
                    }

                    // Undo stack limit（对齐上游 undo/redo 历史限制）
                    RowLayout {
                        spacing: 16
                        Text { text: qsTr("撤销栈上限"); color: "#a0abbe"; font.pixelSize: 12; Layout.preferredWidth: 180 }
                        Row {
                            spacing: 4
                            Repeater {
                                model: [20, 50, 100, 200]
                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: 56; height: 28; radius: 4
                                    color: root.settingsVm.undoLimit === modelData ? "#1c2a3e" : "#1a1e28"
                                    border.color: root.settingsVm.undoLimit === modelData ? "#18c75e" : "#2e3444"
                                    border.width: 1
                                    Text { anchors.centerIn: parent; text: modelData; color: root.settingsVm.undoLimit === modelData ? "#18c75e" : "#8a96a8"; font.pixelSize: 11 }
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
                        color: "#566070"
                        font.pixelSize: 10
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
                        color: "#18c75e"
                        font.pixelSize: 11
                        opacity: 0
                        Behavior on opacity { NumberAnimation { duration: 300 } }
                    }

                    Button {
                        text: qsTr("恢复默认")
                        implicitHeight: 30; implicitWidth: 90
                        background: Rectangle { radius: 4; color: "#252b38"; border.color: "#363d4e"; border.width: 1 }
                        contentItem: Text { text: parent.text; color: "#c8d4e0"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: root.settingsVm.resetPreferences()
                    }
                    Button {
                        text: qsTr("应用")
                        implicitHeight: 30; implicitWidth: 80
                        background: Rectangle { radius: 4; color: "#18c75e" }
                        contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
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
