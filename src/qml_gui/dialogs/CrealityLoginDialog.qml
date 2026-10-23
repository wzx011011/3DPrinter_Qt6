import QtQuick
pragma ComponentBehavior: Bound
import QtQuick.Controls
import QtQuick.Layouts
import QtWebEngine
import OWzx

// CrealityLoginDialog (feature/cloud-model-library, M2) — account login for
// the Creality Cloud, mirroring the official open-source CrealityPrint
// WebModelLibraryView design: login happens IN-PAGE on the model-library
// site (the mall header carries its own 登录/注册 entry); the resulting
// session cookie lives in the shared QWebEngineProfile, so the M3 model
// library panel reuses it directly. The loopback OAuth capture
// (OAuthCallbackServer via Backend.crealityAuthService.beginLogin) stays
// armed for the documented oauth-redirect chain used by the account face.
//
// Route policy (upstream LoginDialog.cpp:255-324 parity):
// - creality.com / crealitycloud.cn / crealitycloud.com (+ subdomains) and
//   the localhost callback (/login?code=…) stay inside the embedded view.
// - Third-party OAuth provider domains open in the system browser — but only
//   for genuine link clicks; tracker/subframe traffic is silently ignored.
// Loaded via a source-URL Loader in main.qml so a missing WebEngine
// (OWZX_DISABLE_WEBENGINE=1 or module load failure) only breaks this dialog.
CxDialog {
    id: root

    /// Emitted after the dialog closes (renamed: Popup already owns closed()).
    signal dismissed()

    dialogTitle: qsTr("登录创想账号")
    width: 460
    height: 640
    closePolicy: Popup.CloseOnEscape

    readonly property QtObject auth: Backend ? Backend.crealityAuthService : null

    onOpened: {
        if (auth) auth.beginLogin()
        // (Re)start from the library home every time the dialog opens; the
        // user completes login through the site's own 登录/注册 entry.
        webView.url = auth ? auth.loginUrl() : ""
    }
    onClosed: {
        if (auth && auth.loginInProgress) auth.cancelLogin()
        // Stop page activity (media, websockets) when hidden.
        webView.triggerWebAction(WebEngineView.Stop)
        root.dismissed()
    }

    Connections {
        target: root.auth
        function onLoginSucceeded() { root.close() }
        function onLoginFailed(message) {
            errorLabel.text = message
            errorLabel.visible = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Label {
            id: errorLabel
            Layout.fillWidth: true
            Layout.margins: 8
            visible: false
            color: Theme.statusError
            font.pixelSize: Theme.fontSizeSM
            wrapMode: Text.Wrap
        }

        WebEngineView {
            id: webView
            Layout.fillWidth: true
            Layout.fillHeight: true
            backgroundColor: Theme.bgSurface
            settings.focusOnNavigationEnabled: true

            onNavigationRequested: function (request) {
                // QML `url` is string-typed (no .host() method) — parse
                // "scheme://host[:port]/path" by hand.
                const s = String(request.url)
                const afterScheme = s.substring(s.indexOf("://") + 3)
                const host = afterScheme.split("/")[0].split(":")[0].toLowerCase()
                const path = afterScheme.substring(afterScheme.indexOf("/"))
                const isCrealityFamily =
                    host.endsWith("creality.com") || host.endsWith("crealitycloud.cn")
                    || host.endsWith("crealitycloud.com")
                const isLocalCallback =
                    (host === "localhost" || host === "127.0.0.1")
                    && path.startsWith("/login")
                const isThirdPartyLoginDomain =
                    host.indexOf("weixin") !== -1 || host.indexOf("qq.com") !== -1
                    || host.indexOf("facebook.com") !== -1 || host.indexOf("google.com") !== -1
                    || host.indexOf("apple.com") !== -1
                if (isCrealityFamily || isLocalCallback) {
                    request.action = WebEngineView.AcceptRequest
                } else if (isThirdPartyLoginDomain
                           && request.navigationType === WebEngineView.LinkClickedNavigation) {
                    // Upstream LoginDialog::OnWebViewNavigating: third-party
                    // OAuth providers run in the system browser. Tracker
                    // subframes (analytics beacons etc.) are NOT link clicks
                    // and must not pop a browser window.
                    Qt.openUrlExternally(request.url)
                    request.action = WebEngineView.IgnoreRequest
                } else {
                    request.action = WebEngineView.IgnoreRequest
                }
            }
        }

        // Honest fallback when the embedded page cannot render (WebEngine
        // degraded or network blocked): the mall site in a system browser.
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 8
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: qsTr("网页无法显示时，可在系统浏览器打开模型库")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeXS
                elide: Text.ElideRight
            }
            CxButton {
                text: qsTr("系统浏览器打开")
                onClicked: {
                    if (root.auth) Qt.openUrlExternally(root.auth.loginUrl())
                }
            }
        }
    }
}
