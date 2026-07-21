import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"
import "../dialogs"

Item {
    id: root
    required property var homeVm
    // Pure-JS copy - avoids Qt6 V4 VariantAssociationObject lifetime crash
    // Initialized to [] so Repeater componentComplete() sees empty model
    property var _recentProjects: []

    Component.onCompleted: {
        // Use Q_INVOKABLE accessors - never touch QVariantList to avoid Qt6 V4 VariantAssociationObject crash
        var arr = []
        var n = homeVm.recentProjectCount()
        for (var i = 0; i < n; ++i)
            arr.push({ name: homeVm.recentProjectName(i), date: homeVm.recentProjectDate(i), path: homeVm.recentProjectPath(i) })
        _recentProjects = arr
    }

    Rectangle { anchors.fill: parent; color: Theme.bgBase }

    // Phase 171 (CL-01): destructive-action confirm for cloud device unbind.
    property int _pendingUnbindIndex: -1
    ConfirmDialog {
        id: unbindConfirm
        dialogTitle: qsTr("解绑云设备")
        message: qsTr("确定要解绑该云设备吗？解绑后将不再显示该设备的状态。")
        confirmText: qsTr("解绑")
        cancelText: qsTr("取消")
        destructive: true
        onAccepted: {
            if (root._pendingUnbindIndex >= 0 && root.homeVm)
                root.homeVm.cloudUnbindDevice(root._pendingUnbindIndex)
            root._pendingUnbindIndex = -1
        }
        onRejected: root._pendingUnbindIndex = -1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingXXL
        spacing: Theme.spacingXL

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 88
            radius: 20
            color: Theme.bgPanel
            border.width: 1
            border.color: Theme.borderSubtle

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                spacing: Theme.spacingXL

                Rectangle {
                    width: 48
                    height: 48
                    radius: 14
                    color: Theme.accentSubtle
                    border.width: 1
                    border.color: Theme.accentDark

                    Text {
                        anchors.centerIn: parent
                        text: "△"
                        color: Theme.accentLight
                        font.pixelSize: 22
                        font.bold: true
                    }
                }

                Column {
                    spacing: 4
                    Text { text: "OWzx Slicer"; color: Theme.textPrimary; font.pixelSize: 24; font.bold: true }
                    Text { text: qsTr("专业级 3D 打印切片软件"); color: Theme.textSecondary; font.pixelSize: Theme.fontSize13 }
                }

                Item { Layout.fillWidth: true }

                // ── Cloud account section (对齐 upstream WebUserLoginDialog / NetworkAgent) ──
                RowLayout {
                    spacing: Theme.spacingSM
                    visible: !root.homeVm.cloudLoggedIn

                    CxButton {
                        text: qsTr("登录账号")
                        cxStyle: CxButton.Style.Primary
                        onClicked: loginDialog.open()
                    }
                }

                RowLayout {
                    spacing: Theme.spacingSM
                    visible: root.homeVm.cloudLoggedIn

                    // User avatar placeholder
                    Rectangle {
                        width: 32; height: 32; radius: 16
                        color: Theme.accentSubtle
                        border.width: 1; border.color: Theme.accent

                        Text {
                            anchors.centerIn: parent
                            text: root.homeVm.cloudUserName.charAt(0).toUpperCase()
                            color: Theme.accent
                            font.pixelSize: Theme.fontSizeLG
                            font.bold: true
                        }
                    }

                    Column {
                        spacing: 2
                        Text {
                            text: root.homeVm.cloudUserName
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSize13
                            font.bold: true
                        }
                        Text {
                            text: root.homeVm.cloudBoundDeviceCount > 0
                                  ? qsTr("%1 台设备").arg(root.homeVm.cloudBoundDeviceCount)
                                  : qsTr("无绑定设备")
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeSM
                        }
                    }

                    // Sync button
                    Rectangle {
                        width: 28; height: 28; radius: Theme.radiusLG
                        color: syncMA.containsMouse ? Theme.bgHover : "transparent"
                        visible: !root.homeVm.cloudSyncing

                        Text {
                            anchors.centerIn: parent
                            text: "\u21BB"
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeLG
                        }
                        HoverHandler { id: syncHover }
                        MouseArea {
                            id: syncMA
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.homeVm.cloudSyncPresets()
                        }
                    }

                    // Syncing indicator
                    Text {
                        text: qsTr("同步中...")
                        color: Theme.accent
                        font.pixelSize: Theme.fontSizeSM
                        visible: root.homeVm.cloudSyncing
                    }

                    // Logout button
                    Rectangle {
                        width: 28; height: 28; radius: Theme.radiusLG
                        color: logoutMA.containsMouse ? Theme.textOnAccent : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "\u23FB"
                            color: logoutMA.containsMouse ? Theme.statusError : Theme.textTertiary
                            font.pixelSize: Theme.fontSizeLG
                        }
                        MouseArea {
                            id: logoutMA
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.homeVm.cloudLogout()
                        }
                    }
                }
            }
        }

        // ── Cloud bound devices section (对齐 upstream BindDialog / AccountDeviceMgr) ──
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            visible: root.homeVm.cloudLoggedIn && root.homeVm.cloudBoundDeviceCount > 0

            Text {
                text: qsTr("云端设备")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSize13
                font.bold: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD

                Repeater {
                    model: root.homeVm.cloudBoundDeviceCount
                    delegate: Rectangle {
                        width: 180; height: 64; radius: Theme.radiusXL
                        color: Theme.bgPanel
                        border.width: 1; border.color: Theme.borderSubtle

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: Theme.spacingMD
                            spacing: Theme.spacingSM

                            // Online indicator
                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: {
                                    var d = root.homeVm.cloudBoundDeviceAt(index)
                                    return d.online ? Theme.accent : Theme.borderActive
                                }
                            }

                            Column {
                                spacing: 2
                                Text {
                                    text: {
                                        var d = root.homeVm.cloudBoundDeviceAt(index)
                                        return d.name || ""
                                    }
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeMD
                                    font.bold: true
                                }
                                Text {
                                    text: {
                                        var d = root.homeVm.cloudBoundDeviceAt(index)
                                        return d.sn || ""
                                    }
                                    color: Theme.textTertiary
                                    font.pixelSize: Theme.fontSizeXS
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: qsTr("解绑")
                                color: unbindMA.containsMouse ? Theme.statusError : Theme.textTertiary
                                font.pixelSize: Theme.fontSizeSM
                            }
                            MouseArea {
                                id: unbindMA
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    // Phase 171 (CL-01): confirm before unbinding (was firing immediately).
                                    root._pendingUnbindIndex = index
                                    unbindConfirm.open()
                                }
                            }
                        }
                    }
                }

                // Add device button
                Rectangle {
                    width: 180; height: 64; radius: Theme.radiusXL
                    color: addDevMA.containsMouse ? Theme.bgHover : Theme.bgPanel
                    border.width: 1
                    border.color: addDevMA.containsMouse ? Theme.accent : Theme.borderSubtle

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 4
                        Text { text: "+"; color: Theme.accent; font.pixelSize: Theme.fontSizeXL; font.bold: true }
                        Text { text: qsTr("绑定设备"); color: Theme.accent; font.pixelSize: Theme.fontSizeMD }
                    }
                    MouseArea {
                        id: addDevMA
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: bindDialog.open()
                    }
                }
            }
        }

        // ── Login dialog (对齐上游 WebUserLoginDialog) ──
        Dialog {
            id: loginDialog
            anchors.centerIn: parent
            modal: true
            title: qsTr("登录 OWzx 账号")
            padding: 20

            background: Rectangle {
                radius: 12
                color: Theme.bgElevated
                border.color: Theme.borderSubtle
                border.width: 1
            }

            header: Label {
                text: qsTr("登录 OWzx 账号")
                color: Theme.textPrimary
                font.bold: true
                font.pixelSize: Theme.fontSizeXL
                padding: 12
            }

            ColumnLayout {
                spacing: 12
                width: 280

                Label {
                    text: qsTr("用户名")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                }
                CxTextField {
                    id: loginUser
                    Layout.fillWidth: true
                    placeholderText: qsTr("输入用户名")
                }

                Label {
                    text: qsTr("密码")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                }
                CxTextField {
                    id: loginPass
                    Layout.fillWidth: true
                    echoMode: TextInput.Password
                    placeholderText: qsTr("输入密码")
                    Keys.onReturnPressed: doLogin()
                    Keys.onEnterPressed: doLogin()
                }

                // Error message
                Label {
                    text: loginError
                    color: Theme.statusError
                    font.pixelSize: Theme.fontSizeSM
                    visible: loginError !== ""
                }

                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: 8

                    CxButton {
                        text: qsTr("取消")
                        onClicked: loginDialog.close()
                    }
                    CxButton {
                        text: qsTr("登录")
                        highlighted: true
                        onClicked: doLogin()
                    }
                }
            }

            property string loginError: ""

            Connections {
                target: root.homeVm
                function onCloudLoginFailed(error) { loginError = error }
                function onCloudStateChanged() {
                    if (root.homeVm.cloudLoggedIn) {
                        loginDialog.close()
                        loginError = ""
                    }
                }
            }

            function doLogin() {
                root.homeVm.cloudLogin(loginUser.text, loginPass.text)
            }

            onOpened: { loginUser.text = ""; loginPass.text = ""; loginError = ""; loginUser.forceActiveFocus() }
        }

        // ── Bind device dialog (对齐上游 BindDialog / PingCodeBindDialog) ──
        Dialog {
            id: bindDialog
            anchors.centerIn: parent
            modal: true
            title: qsTr("绑定设备")
            padding: 20

            background: Rectangle {
                radius: 12
                color: Theme.bgElevated
                border.color: Theme.borderSubtle
                border.width: 1
            }

            header: Label {
                text: qsTr("绑定设备")
                color: Theme.textPrimary
                font.bold: true
                font.pixelSize: Theme.fontSizeXL
                padding: 12
            }

            ColumnLayout {
                spacing: 12
                width: 280

                Label {
                    text: qsTr("设备名称")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                }
                CxTextField {
                    id: bindDeviceName
                    Layout.fillWidth: true
                    placeholderText: qsTr("例如：K1 Max")
                }

                Label {
                    text: qsTr("PIN 码")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                }
                CxTextField {
                    id: bindPinCode
                    Layout.fillWidth: true
                    placeholderText: qsTr("设备屏幕上显示的 PIN 码")
                    Keys.onReturnPressed: doBind()
                    Keys.onEnterPressed: doBind()
                }

                Text {
                    text: bindError
                    color: Theme.statusError
                    font.pixelSize: Theme.fontSizeSM
                    visible: bindError !== ""
                }

                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: 8

                    CxButton {
                        text: qsTr("取消")
                        onClicked: bindDialog.close()
                    }
                    CxButton {
                        text: qsTr("绑定")
                        highlighted: true
                        onClicked: doBind()
                    }
                }
            }

            property string bindError: ""

            Connections {
                target: root.homeVm
                function onCloudLoginFailed(error) { bindError = error }
                function onCloudStateChanged() { bindDialog.close(); bindError = "" }
            }

            function doBind() {
                root.homeVm.cloudBindDevice(bindDeviceName.text, bindPinCode.text)
            }

            onOpened: { bindDeviceName.text = ""; bindPinCode.text = ""; bindError = ""; bindDeviceName.forceActiveFocus() }
        }

        // PAGE-04: Daily Tips（对齐上游 MarkdownTip / DailyTips.cpp）
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            radius: 10
            color: Theme.bgElevated
            border.width: 1
            border.color: Theme.borderSubtle

            RowLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                Text {
                    text: "💡"
                    font.pixelSize: Theme.fontSizeXXL
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: qsTr("每日提示")
                        color: Theme.accent
                        font.pixelSize: Theme.fontSizeSM
                        font.bold: true
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("切片前确保模型已平放在热床上。使用 W/E/R 切换移动/旋转/缩放工具。")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                        wrapMode: Text.WordWrap
                        elide: Text.ElideRight
                    }
                }
            }
        }

        Text { text: qsTr("最近项目"); color: Theme.textSecondary; font.pixelSize: Theme.fontSize13; font.bold: true }

        ScrollView {
            Layout.fillWidth: true; Layout.preferredHeight: 188; clip: true
            Flow {
                width: parent.width; spacing: Theme.spacingMD
                Repeater {
                    model: root._recentProjects
                    delegate: Rectangle {
                        width: 196; height: 164; radius: 16; color: Theme.bgPanel; border.color: Theme.borderSubtle; border.width: 1
                        Column {
                            anchors.fill: parent; anchors.margins: 12; spacing: 8
                            Rectangle { width: parent.width; height: 102; radius: 12; color: Theme.bgElevated
                                Text { anchors.centerIn: parent; text: "🖨"; font.pixelSize: 34; color: Theme.textDisabled }
                            }
                            Text { text: modelData.name || (qsTr("项目 ") + (index + 1)); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeMD; font.bold: true; elide: Text.ElideRight; width: parent.width }
                            Text { text: modelData.date || "—"; color: Theme.textSecondary; font.pixelSize: Theme.fontSizeXS }
                            Text { text: modelData.path || ""; color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS; elide: Text.ElideRight; width: parent.width }
                        }
                        HoverHandler { id: recentHover }
                        Rectangle { anchors.fill: parent; radius: parent.radius; color: recentHover.hovered ? "#1018c75e" : "transparent" }
                    }
                }
            }
        }

        Text { text: qsTr("快速入口"); color: Theme.textSecondary; font.pixelSize: Theme.fontSize13; font.bold: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingLG

            Repeater {
                model: [
                    { icon: "📂", title: qsTr("打开项目"),   sub: qsTr("打开已有 3MF/STL 文件") },
                    { icon: "➕", title: qsTr("新建项目"),   sub: qsTr("从空白开始创建") },
                    { icon: "🔧", title: qsTr("校准"),       sub: qsTr("打印机校准向导") },
                    { icon: "🌐", title: qsTr("模型商城"),   sub: qsTr("在线下载模型") }
                ]
                delegate: Rectangle {
                    required property var modelData
                    Layout.fillWidth: true
                    height: 112
                    radius: 18
                    color: qaHover.hovered ? Theme.bgHover : Theme.bgPanel
                    border.color: Theme.borderSubtle
                    border.width: 1

                    Column {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 8
                        Text { text: parent.parent.modelData.icon; font.pixelSize: 24; horizontalAlignment: Text.AlignHCenter; width: parent.width }
                        Text { text: parent.parent.modelData.title; color: Theme.textPrimary; font.pixelSize: Theme.fontSize13; font.bold: true; horizontalAlignment: Text.AlignHCenter; width: parent.width }
                        Text { text: parent.parent.modelData.sub; color: Theme.textSecondary; font.pixelSize: Theme.fontSizeXS; horizontalAlignment: Text.AlignHCenter; width: parent.width; wrapMode: Text.WordWrap }
                    }

                    HoverHandler { id: qaHover }
                }
            }
        }

        Item { Layout.fillHeight: true }

        Text { text: qsTr("版本 2.4.0-dev  |  Qt 6.10  |  OWzx"); color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS }
    }
}
