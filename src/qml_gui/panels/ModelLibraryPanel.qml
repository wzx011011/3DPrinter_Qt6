import QtQuick
pragma ComponentBehavior: Bound
import QtWebEngine
import QtWebChannel
import OWzx

// ModelLibraryPanel.qml (feature/cloud-model-library, M3) — the Creality
// Cloud model library dock, ported from the official open-source
// CrealityPrint WebModelLibraryView: an embedded WebEngineView loading
// cp.crealitycloud.cn|com with a QWebChannel bridge re-exported to the page
// as `window.CXSWGroupInterface` (the exact protocol name the remote
// frontend calls); native→web replies go through `window.handleStudioCmd`.
//
// Channel wiring (measured, same constraints as ChatSidebar.qml):
// - webChannel MUST be typed QQmlWebChannel* (the QML element) and assigned
//   before `url`; Backend.attachModelLibraryChannel registers the C++
//   ModelLibraryBridge as channel.objects.cxbridge through the public
//   QWebChannel base API.
// - The remote page does not include qwebchannel.js itself, so two
//   DocumentCreation user scripts inject it and then the shim that
//   re-exports the bridge under the protocol name, queueing early
//   postMessage calls until the handshake settles.
// - The profile user agent marks the shell to the frontend
//   (creality_print_slice/<ver>, upstream WebModelLibraryView parity) and is
//   set once on the shared default profile so the login dialog and this
//   panel present one identity; the shared profile also owns the persistent
//   login cookies.
//
// OWZX_DISABLE_WEBENGINE=1 never reaches this file (the loader in main.qml
// uses a source URL and stays inactive; the topbar entry checks the same
// env via Backend.webEngineAvailable).
Rectangle {
    id: root

    readonly property QtObject bridge: Backend ? Backend.modelLibraryBridge : null
    /// qwebchannel.js source, read in C++ (sync XHR to qrc is unreliable
    /// from QML in Qt 6 — it returned empty here).
    readonly property string qwebchannelSource:
        Backend ? Backend.qwebchannelScript() : ""
    signal closed()
    /// The mall hides its own header when embedded in a wide shell view, so
    /// login needs an explicit entry: opens the narrow login dialog (whose
    /// page shows the site's 登录/注册 header) sharing this profile.
    signal loginRequested()

    /// Reload after a successful login elsewhere so the session cookie is
    /// picked up by the page.
    function reloadPage() {
        webView.reload()
    }

    color: Theme.bgPanel
    border.width: 1
    border.color: Theme.borderSubtle

    Component.onCompleted: {
        if (Backend)
            Backend.attachModelLibraryChannel(libChannel)
        // NOTE: deliberately NO shell user-agent here. The mall frontend
        // detects `creality_print_slice` in the UA and then REMOVES its own
        // 登录/注册 header (it assumes the shell provides the account UI —
        // verified 2026-10-23 via DOM probe: nav-bar absent under the spoofed
        // UA). With a plain browser UA the page renders its full header +
        // login modal, the session cookie lands in the shared profile, and
        // downloads arrive as native WebEngineDownloadRequests (below),
        // which we route into the plate — same end state as the official
        // bridge flow without reimplementing the account UI.
    }

    WebEngineView {
        id: webView
        anchors.fill: parent
        // Channel assigned before url (see ChatSidebar.qml note): Chromium
        // injects the qt.webChannelTransport bootstrap only for navigations
        // that start after the channel is attached.
        webChannel: libChannel
        profile: WebEngine.defaultProfile
        url: root.bridge
             ? (Backend ? Backend.crealityAuthService.loginUrl() : "")
             : ""
        backgroundColor: Theme.bgSurface
        settings.focusOnNavigationEnabled: true

        onLoadingChanged: function (loadRequest) {
            if (loadRequest.status === WebEngineView.LoadStartedStatus
                || loadRequest.status === WebEngineView.LoadSucceededStatus)
                webView.injectBridgeScripts()
            if (loadRequest.status === WebEngineView.LoadSucceededStatus) {
                webView.runJavaScript("window.scrollTo(0, 0); "
                                      + "document.documentElement.scrollTop = 0;")
                // TEMP probe (remove after login-entry investigation)
                webView.runJavaScript(
                    "JSON.stringify({"
                    + "navBar: !!document.querySelector('.nav-bar'),"
                    + "navLogin: (function(){var e=document.querySelector('.nav-bar .login');"
                    + "  if(!e) return 'absent'; var r=e.getBoundingClientRect();"
                    + "  return Math.round(r.width)+'x'+Math.round(r.height)+'@y'+Math.round(r.top);})(),"
                    + "loginWord: (document.body.innerText.match(/登录\\/注册|登录/g)||[]).slice(0,3).join(','),"
                    + "scrollY: window.scrollY,"
                    + "docH: document.documentElement.scrollHeight,"
                    + "vpW: window.innerWidth})",
                    function (result) { console.log("[ModelLibraryPanel][probe]", result) })
            }
        }

        // qwebchannel.js + CXSWGroupInterface shim injection. The natural
        // mechanism is WebEngineView.userScripts, but Qt 6.12's qmlcachegen
        // rejects the lowercase `webEngineScript` value type inside array
        // literals ("Expected type name") while the capitalized form is
        // isCreatable:false at runtime — so the two scripts are injected at
        // runtime instead: best effort at LoadStarted, guaranteed at
        // LoadSucceeded; the shim is idempotent and queues postMessage calls
        // until the QWebChannel handshake settles. If a future qmlcachegen
        // accepts the value type, move these back into userScripts at
        // DocumentCreation (removes the startup race noted in the shim).
        readonly property string cxShimSource: "(function() {
            if (window.__cxShimInstalled) return;
            window.__cxShimInstalled = true;
            var pending = [];
            window.CXSWGroupInterface = {
                postMessage: function (s) {
                    if (window.__cxReady) window.__cxBridge.postMessage(String(s));
                    else pending.push(String(s));
                }
            };
            new QWebChannel(qt.webChannelTransport, function (channel) {
                window.__cxBridge = channel.objects.cxbridge;
                window.__cxReady = true;
                for (var i = 0; i < pending.length; ++i)
                    window.__cxBridge.postMessage(pending[i]);
                pending = [];
            });
        })();"

        function injectBridgeScripts() {
            webView.runJavaScript(qwebchannelSource + cxShimSource)
        }

        onNavigationRequested: function (request) {
            const s = String(request.url)
            const afterScheme = s.substring(s.indexOf("://") + 3)
            const host = afterScheme.split("/")[0].split(":")[0].toLowerCase()
            const family = host.endsWith("creality.com")
                           || host.endsWith("crealitycloud.cn")
                           || host.endsWith("crealitycloud.com")
            const local = (host === "localhost" || host === "127.0.0.1")
            if (family || local) {
                request.action = WebEngineView.AcceptRequest
            } else {
                // Everything else (external help links, trackers) stays out
                // of the dock; genuine user-facing pages open in the system
                // browser via the bridge's linkType-403 path.
                request.action = WebEngineView.IgnoreRequest
            }
        }
    }

    WebChannel {
        id: libChannel
    }

    // 浏览器式下载路径（Qt 6.8+：下载信号在 Profile 上，不在 View 上）：
    // 页面（普通 UA 模式）的下载按钮触发原生下载，重定向到配置的下载目录；
    // 完成后按扩展名自动入板（3mf → 打开项目，stl/obj 等 → 导入模型）。
    Connections {
        target: WebEngine.defaultProfile
        function onDownloadRequested(download) {
            if (Backend)
                download.downloadDirectory = Backend.modelDownloadDirectory()
            download.accept()
            download.stateChanged.connect(function () {
                if (download.state === WebEngineDownloadRequest.DownloadCompleted
                    && Backend)
                    Backend.importDownloadedFile(
                        download.downloadDirectory + "/"
                        + download.downloadFileName)
            })
        }
    }

    // Native→web replies (bridge → handleStudioCmd).
    Connections {
        target: root.bridge
        function onSendToWeb(json) {
            webView.runJavaScript(
                "window.handleStudioCmd && window.handleStudioCmd(" + json + ");")
        }
        function onOpenInSystemBrowser(url) {
            Qt.openUrlExternally(url)
        }
        function onTokenExpired() {
            // The frontend shows its own 登录/注册 after this; nothing to do
            // beyond surfacing the event (M5 status line).
        }
    }

    // Panel chrome: close affordance only. 登录 lives in the site's own
    // header (plain-UA mode renders it); the former chrome 登录 button
    // overlapped the site header and duplicated it. The loginRequested
    // signal / login dialog stay available for the HomePage entry.
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 6
        width: 26; height: 26; radius: width / 2
        color: closeMA.containsMouse ? Theme.bgHover : "transparent"
        Text { anchors.centerIn: parent; text: "✕"; color: Theme.textSecondary }
        MouseArea {
            id: closeMA
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.closed()
        }
    }
}
