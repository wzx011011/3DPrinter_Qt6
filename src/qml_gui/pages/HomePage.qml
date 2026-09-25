import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../controls"
import "../dialogs"

Item {
    id: root
    required property var homeVm
    signal newProjectRequested()
    signal openProjectRequested(string filePath)
    // Pure-JS copy - avoids Qt6 V4 VariantAssociationObject lifetime crash
    // Initialized to [] so Repeater componentComplete() sees empty model.
    // Phase 241 (PAGE-01): mirrors homeVm's real recent list, which itself
    // mirrors the persisted ProjectViewModel recentProjects (upstream
    // app_config "recent_projects"). Empty until a project is opened.
    property var _recentProjects: []

    Component.onCompleted: {
        // Use Q_INVOKABLE accessors - never touch QVariantList to avoid Qt6 V4 VariantAssociationObject crash
        root.reloadRecentProjects()
        // Phase 241 (PAGE-01): Daily Tips rotate the BackendContext hint
        // database (hints.json, upstream DailyTips/MarkdownTip) instead of a
        // single static string.
        if (typeof backend !== "undefined")
            backend.showDailyTip()
    }

    function reloadRecentProjects() {
        var arr = []
        var n = homeVm.recentProjectCount()
        for (var i = 0; i < n; ++i)
            arr.push({ name: homeVm.recentProjectName(i), date: homeVm.recentProjectDate(i), path: homeVm.recentProjectPath(i) })
        _recentProjects = arr
    }

    Connections {
        target: root.homeVm
        function onRecentProjectsChanged() { root.reloadRecentProjects() }
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
                    text: loginDialog.loginError
                    color: Theme.statusError
                    font.pixelSize: Theme.fontSizeSM
                    visible: loginDialog.loginError !== ""
                }

                // R-P1.E: user-visible disclosure required by
                // docs/依赖与协议边界审计.md -- cloud login/bind are local
                // mocks (real `bambu_networking` is externally blocked), so a
                // "success" here is a demo flow, not a real account login.
                Label {
                    text: qsTr("演示模式：登录/绑定为本地模拟，不会连接真实云服务（依赖 bambu_networking，外部阻塞）。")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeXS
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
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
                function onCloudLoginFailed(error) { loginDialog.loginError = error }
                function onCloudStateChanged() {
                    if (root.homeVm.cloudLoggedIn) {
                        loginDialog.close()
                        loginDialog.loginError = ""
                    }
                }
            }

            function doLogin() {
                root.homeVm.cloudLogin(loginUser.text, loginPass.text)
            }

            onOpened: { loginUser.text = ""; loginPass.text = ""; loginError = ""; loginUser.forceActiveFocus() }
        }

        // ── Bind device dialog (对齐上游 BindDialog / PingCodeBindDialog) ──
        // Upstream PingCodeBindDialog (BindDialog.cpp:60-172) is a 460x240
        // white two-page simplebook: page 1 = two-line guide text (Body_14,
        // #262E30) + "Can't find Pin Code?" wiki hyperlink + "Pin Code" title
        // + six 38x38 single-char green inputs + Confirm/Cancel; after submit
        // page 2 = "Binding..." + "Please confirm on the printer screen" +
        // Close. Dark-mode mapping: white panel -> Theme.bgPanel. The extra
        // "device name" input is a mock-flow addition kept per the page brief.
        Dialog {
            id: bindDialog
            anchors.centerIn: parent
            modal: true
            title: qsTr("通过 PIN 码绑定")
            padding: 20

            background: Rectangle {
                radius: 12
                color: Theme.bgPanel
                border.color: Theme.borderSubtle
                border.width: 1
            }

            header: Label {
                text: qsTr("通过 PIN 码绑定")
                color: Theme.textPrimary
                font.bold: true
                font.pixelSize: Theme.fontSizeXL
                padding: 12
            }

            // Upstream simplebook page index (BindDialog.cpp:163/303):
            // 0 = request page, 1 = binding page.
            property int bindState: 0
            property string bindError: ""
            // PIN cells registry, PING_CODE_LENGTH = 6 (BindDialog.hpp:42).
            property var pinCells: []
            // Upstream Confirm stays disabled until all 6 cells are filled
            // (BindDialog.cpp:226-245).
            property bool pinComplete: false

            function refreshPinComplete() {
                if (pinCells.length < 6) {
                    pinComplete = false
                    return
                }
                var filled = true
                for (var i = 0; i < 6; ++i) {
                    if (pinCells[i].text === "")
                        filled = false
                }
                pinComplete = filled
            }

            function pinCode() {
                var s = ""
                for (var i = 0; i < pinCells.length; ++i)
                    s += pinCells[i].text
                return s
            }

            ColumnLayout {
                spacing: 10
                // Upstream simplebook is 460x240 (BindDialog.cpp:70-72); 460
                // is the dialog width incl. 20 padding. Height flows with the
                // content (the mock device-name row adds one row upstream
                // does not have).
                width: 460

                // ── Page 1: request (upstream request_bind_panel, BindDialog.cpp:74-126) ──
                ColumnLayout {
                    visible: bindDialog.bindState === 0
                    Layout.fillWidth: true
                    spacing: 10

                    // Guide text, upstream m_status_text (BindDialog.cpp:88-91):
                    // Body_14, #262E30, two lines.
                    Label {
                        text: qsTr("请在打印机屏幕的『账号』页面找到 PIN 码，\n然后在下方输入。")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSize13
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }

                    // Upstream "Can't find Pin Code?" wiki hyperlink
                    // (BindDialog.cpp:94).
                    Label {
                        text: qsTr("<a href=\"https://wiki.bambulab.com/en/bambu-studio/manual/pin-code\">找不到 PIN 码？</a>")
                        color: Theme.accent
                        font.pixelSize: Theme.fontSize13
                        Layout.fillWidth: true
                        onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                    }

                    // Mock-flow addition kept per page brief (upstream has no
                    // device-name input on this dialog).
                    Label {
                        text: qsTr("设备名称")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeMD
                    }
                    CxTextField {
                        id: bindDeviceName
                        Layout.fillWidth: true
                        placeholderText: qsTr("例如：K1 Max")
                        Keys.onReturnPressed: if (bindDialog.pinComplete) bindDialog.doBind()
                        Keys.onEnterPressed: if (bindDialog.pinComplete) bindDialog.doBind()
                    }

                    // Upstream "Pin Code" input title (BindDialog.cpp:96).
                    Label {
                        text: qsTr("PIN 码")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeMD
                    }

                    // Six 38x38 single-char cells (BindDialog.cpp:111-123):
                    // centered, green #228B22, Body_16, auto-advance.
                    Row {
                        spacing: 10

                        Repeater {
                            model: 6

                            CxTextField {
                                id: pinCell
                                required property int index
                                width: 38
                                height: 38
                                font.pixelSize: Theme.fontSizeXL
                                // Upstream SetTextColour(wxColour(34,139,34))
                                // (BindDialog.cpp:115).
                                color: "#228b22"
                                horizontalAlignment: TextInput.AlignHCenter
                                verticalAlignment: TextInput.AlignVCenter
                                leftPadding: 4
                                rightPadding: 4
                                maximumLength: 1
                                // Upstream on_key_input whitelist: 0-9 a-z A-Z
                                // (BindDialog.cpp:203-216).
                                validator: RegularExpressionValidator { regularExpression: /[0-9a-zA-Z]/ }

                                Component.onCompleted: bindDialog.pinCells.push(pinCell)
                                onTextChanged: {
                                    // Auto-advance once one char is entered
                                    // (upstream on_text_changed,
                                    // BindDialog.cpp:226-230).
                                    if (text.length === 1 && pinCell.index < 5)
                                        bindDialog.pinCells[pinCell.index + 1].forceActiveFocus()
                                    bindDialog.refreshPinComplete()
                                }
                                // Qt 6.10 Keys has no backspacePressed signal;
                                // match Key_Backspace manually (upstream
                                // on_key_backspace, BindDialog.cpp:255-268).
                                Keys.onPressed: function(event) {
                                    if (event.key !== Qt.Key_Backspace)
                                        return
                                    event.accepted = true
                                    // Move focus to the previous cell.
                                    if (pinCell.index > 0)
                                        bindDialog.pinCells[pinCell.index - 1].forceActiveFocus()
                                }
                                Keys.onReturnPressed: if (bindDialog.pinComplete) bindDialog.doBind()
                                Keys.onEnterPressed: if (bindDialog.pinComplete) bindDialog.doBind()
                            }
                        }
                    }

                    Text {
                        text: bindDialog.bindError
                        color: Theme.statusError
                        font.pixelSize: Theme.fontSizeSM
                        visible: bindDialog.bindError !== ""
                        Layout.fillWidth: true
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
                            enabled: bindDialog.pinComplete
                            onClicked: bindDialog.doBind()
                        }
                    }

                    // R-P1.E: mock bind disclosure (dependency-audit registry).
                    Label {
                        text: qsTr("演示模式：绑定为本地模拟，不会连接真实设备（依赖 bambu_networking，外部阻塞）。")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }

                // ── Page 2: binding (upstream binding_panel, BindDialog.cpp:162-172) ──
                ColumnLayout {
                    visible: bindDialog.bindState === 1
                    Layout.fillWidth: true
                    spacing: 10

                    Item { Layout.fillHeight: true }
                    Label {
                        text: qsTr("绑定中…")
                        color: Theme.textPrimary
                        font.bold: true
                        font.pixelSize: Theme.fontSizeXL
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Label {
                        text: qsTr("请在打印机屏幕上确认")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeLG
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Item { Layout.fillHeight: true }
                    CxButton {
                        text: qsTr("关闭")
                        Layout.alignment: Qt.AlignRight
                        onClicked: bindDialog.close()
                    }
                }
            }

            Connections {
                target: root.homeVm
                function onCloudLoginFailed(error) {
                    // Upstream surfaces the failure and stays on the request
                    // page (BindDialog.cpp:288-293).
                    bindDialog.bindError = error
                    bindDialog.bindState = 0
                }
                function onCloudStateChanged() {
                    bindDialog.close()
                    bindDialog.bindError = ""
                    bindDialog.bindState = 0
                }
            }

            function doBind() {
                // Upstream on_bind_printer (BindDialog.cpp:280-301): requires
                // login + a full 6-char code; success flips the simplebook to
                // the binding page, failure surfaces an error and stays put.
                if (!pinComplete)
                    return
                bindError = ""
                bindState = 1
                root.homeVm.cloudBindDevice(bindDeviceName.text, pinCode())
            }

            onOpened: {
                bindState = 0
                bindError = ""
                bindDeviceName.text = ""
                for (var i = 0; i < pinCells.length; ++i)
                    pinCells[i].text = ""
                refreshPinComplete()
                if (pinCells.length > 0)
                    pinCells[0].forceActiveFocus()
            }
        }

        // PAGE-04: Daily Tips（对齐上游 DailyTips.cpp DailyTipsPanel）。
        // Upstream is a vertical collapsible card: a 16:9 image area renders
        // above the text when the hint carries an image (DailyTips.cpp:100-108),
        // below it the first text line is the highlighted title
        // (HintNotification.cpp:893-896, COL_ORANGE_LIGHT) followed by the
        // body (+ wiki hypertext line), and a 30px control bar
        // (DailyTips.cpp:253) carries the collapse label, the n/m page
        // counter and 38x38 prev/next icon buttons with hover accent
        // (DailyTips.cpp:408/:478-488/:497-525). Collapsed = control bar only
        // (DailyTips.cpp:293-299).
        // Phase 241 (PAGE-01): rotates the BackendContext hint database
        // (hints.json) with prev/next navigation and documentation-link
        // support where the hint carries one — no more static single string.
        Rectangle {
            id: dailyTipsCard
            Layout.fillWidth: true
            property bool expanded: true
            // Bumped by dailyTipChanged so the Q_INVOKABLE-derived bindings
            // below re-evaluate (BackendContext.h:567-568).
            property int tipRevision: 0
            // Upstream hint img_url (DailyTipsDataRenderer::has_image); the
            // Qt hint database (hints.json) carries no image URLs and
            // BackendContext exposes none yet, so the 16:9 slot stays hidden.
            readonly property string tipImageUrl: ""
            readonly property string tipFullText: {
                var _rev = tipRevision
                if (typeof backend === "undefined")
                    return ""
                return backend.currentHintText()
            }
            // First line (before \n) is the headline
            // (DailyTips.cpp:169-173 / HintNotification.cpp:893-896).
            readonly property string tipTitle: {
                var i = tipFullText.indexOf("\n")
                return i >= 0 ? tipFullText.substring(0, i) : tipFullText
            }
            readonly property string tipBody: {
                var i = tipFullText.indexOf("\n")
                var base = i >= 0 ? tipFullText.substring(i + 1) : ""
                var follow = (typeof backend !== "undefined" && tipRevision >= 0)
                             ? backend.currentHintFollowText() : ""
                var parts = []
                if (base !== "") parts.push(base)
                if (follow !== "") parts.push(follow)
                return parts.join(" ")
            }
            readonly property bool tipHasWiki: tipRevision >= 0
                && typeof backend !== "undefined"
                && backend.currentHintHasDocumentationLink()
                && backend.currentHintHypertext() !== ""
            readonly property string tipWikiUrl: tipRevision >= 0
                && typeof backend !== "undefined"
                ? backend.currentHintHypertext() : ""
            // Page counter "n/m" (upstream DailyTips.cpp:478-488).
            readonly property string tipPage: tipRevision >= 0
                && typeof backend !== "undefined"
                ? (backend.currentHintIndex() + 1) + "/" + backend.hintCount() : ""
            // Expanded = content + 30px footer; collapsed = footer only
            // (upstream DailyTips.cpp:293-299).
            Layout.preferredHeight: expanded ? tipsColumn.height + 28 : 30
            radius: 10
            color: Theme.bgElevated
            border.width: 1
            border.color: Theme.borderSubtle
            clip: true

            Connections {
                target: typeof backend !== "undefined" ? backend : null
                function onDailyTipChanged() { dailyTipsCard.tipRevision++ }
            }

            Column {
                id: tipsColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 14
                spacing: 6

                // 16:9 image area above the text when the hint carries an
                // image (upstream DailyTips.cpp:100-108); hidden otherwise.
                Rectangle {
                    visible: dailyTipsCard.tipImageUrl !== ""
                    width: parent.width
                    height: visible ? width * 9 / 16 : 0
                    radius: 6
                    color: Theme.bgBase
                    clip: true

                    Image {
                        anchors.fill: parent
                        source: dailyTipsCard.tipImageUrl
                        fillMode: Image.PreserveAspectCrop
                    }
                }

                // Headline: first line, highlighted bold orange (upstream
                // COL_ORANGE_LIGHT = ColorRGBA::ORANGE 0.923,0.504,0.264 ->
                // #EB8043, Color.hpp:136 via ImGuiWrapper.cpp:165).
                Text {
                    id: dailyTipTitle
                    width: parent.width
                    text: dailyTipsCard.tipTitle !== "" ? dailyTipsCard.tipTitle : qsTr("每日提示")
                    color: "#eb8043"
                    font.pixelSize: Theme.fontSizeLG
                    font.bold: true
                    elide: Text.ElideRight
                }

                Text {
                    id: dailyTipBody
                    width: parent.width
                    text: dailyTipsCard.tipBody !== "" ? dailyTipsCard.tipBody : qsTr("暂无提示")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    wrapMode: Text.WordWrap
                }

                // Wiki hypertext line (upstream hint documentation link).
                Text {
                    id: dailyTipWiki
                    width: parent.width
                    visible: dailyTipsCard.tipHasWiki
                    text: dailyTipsCard.tipWikiUrl
                    color: Theme.accent
                    font.pixelSize: Theme.fontSizeSM
                    font.underline: dailyTipWikiMA.containsMouse
                    elide: Text.ElideRight
                    // Upstream HintNotification hypertext_type=documentation.
                    MouseArea {
                        id: dailyTipWikiMA
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: backend.openHintDocumentation()
                    }
                }
            }

            // 30px control bar (upstream m_footer_height = 30*scale,
            // DailyTips.cpp:253).
            Rectangle {
                id: tipsFooter
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 30
                color: "transparent"

                // Collapse / expand control (upstream DailyTips.cpp:412-455):
                // expanded = "Collapse" + arrow; collapsed = bold "Daily Tips"
                // + arrow; hover draws an underline.
                Row {
                    id: tipsCollapseControl
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4

                    Text {
                        text: dailyTipsCard.expanded ? qsTr("折叠") : qsTr("每日提示")
                        color: dailyTipsCard.expanded ? Theme.textTertiary : Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                        font.bold: !dailyTipsCard.expanded
                        font.underline: tipsCollapseMA.containsMouse
                    }
                    Text {
                        text: dailyTipsCard.expanded ? "\u25be" : "\u25b8"
                        color: dailyTipsCard.expanded ? Theme.textTertiary : Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                        font.underline: tipsCollapseMA.containsMouse
                    }
                }
                MouseArea {
                    id: tipsCollapseMA
                    anchors.fill: tipsCollapseControl
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: dailyTipsCard.expanded = !dailyTipsCard.expanded
                }

                // Page counter "n/m" (upstream DailyTips.cpp:478-488).
                Text {
                    text: dailyTipsCard.tipPage
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    anchors.right: tipsPrevBtn.left
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                }

                // Prev/next 38x38 transparent-background icon buttons; hover
                // recolors to Theme.accent (upstream 38x38*scale buttons with
                // hover ImColor(0,150,136), DailyTips.cpp:408/:497-525; the
                // upstream glyphs are icon-font arrows, rendered here as
                // chevron glyphs).
                Rectangle {
                    id: tipsPrevBtn
                    width: 38; height: 38
                    color: "transparent"
                    anchors.right: tipsNextBtn.left
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        anchors.centerIn: parent
                        text: "\u2039"
                        color: tipsPrevMA.containsMouse ? Theme.accent : Theme.textSecondary
                        font.pixelSize: Theme.fontSizeXXL
                    }
                    MouseArea {
                        id: tipsPrevMA
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: backend.prevHint()
                    }
                }
                Rectangle {
                    id: tipsNextBtn
                    width: 38; height: 38
                    color: "transparent"
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        anchors.centerIn: parent
                        text: "\u203a"
                        color: tipsNextMA.containsMouse ? Theme.accent : Theme.textSecondary
                        font.pixelSize: Theme.fontSizeXXL
                    }
                    MouseArea {
                        id: tipsNextMA
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: backend.showDailyTip()
                    }
                }
            }
        }

        Text { text: qsTr("最近项目"); color: Theme.textSecondary; font.pixelSize: Theme.fontSize13; font.bold: true }

        ScrollView {
            Layout.fillWidth: true; Layout.preferredHeight: 188; clip: true

            // Phase 241 (PAGE-01): honest empty state — the persisted recent
            // list starts empty on a fresh install (no fabricated entries).
            Text {
                visible: root._recentProjects.length === 0
                text: qsTr("暂无最近项目，打开或保存一个项目后将显示在这里")
                color: Theme.textDisabled
                font.pixelSize: Theme.fontSizeSM
                anchors.centerIn: parent
            }
            Flow {
                width: parent.width; spacing: Theme.spacingMD
                visible: root._recentProjects.length > 0
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
                        // Phase 241 (PAGE-01): cards are clickable — opens the
                        // project through the same path as the topbar Recent
                        // submenu (upstream recent-files menu).
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.openProjectRequested(modelData.path || "")
                        }
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
                    // Phase 241 (PAGE-01): every quick action is wired to a
                    // real handler. The dead ModelMall entry (model mall out
                    // of scope per REQUIREMENTS) was removed.
                    { icon: "📂", title: qsTr("打开项目"),   sub: qsTr("打开已有 3MF/STL 文件"), action: "open" },
                    { icon: "➕", title: qsTr("新建项目"),   sub: qsTr("从空白开始创建"),        action: "new" },
                    { icon: "🔧", title: qsTr("校准"),       sub: qsTr("打印机校准向导"),        action: "calibration" }
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
                    // Phase 241 (PAGE-01): open = file dialog ->
                    // topbarOpenProject; new = topbarNewProject; calibration
                    // = Calibration tab route (upstream menu routing).
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            switch (parent.parent.modelData.action) {
                                case "open":
                                    root.openProjectRequested("")
                                    break
                                case "new":
                                    root.newProjectRequested()
                                    break
                                case "calibration":
                                    backend.requestSelectTab(backend.tpCalibration)
                                    break
                            }
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        Text { text: qsTr("版本 2.4.0-dev  |  Qt 6.10  |  OWzx"); color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS }
    }
}
