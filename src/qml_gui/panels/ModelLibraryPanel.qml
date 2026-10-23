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

    color: Theme.bgPanel
    border.width: 1
    border.color: Theme.borderSubtle

    Component.onCompleted: {
        if (Backend) {
            Backend.attachModelLibraryChannel(libChannel)
            // One shell identity for all Creality web surfaces (idempotent).
            WebEngine.defaultProfile.httpUserAgent = Backend.crealityWebViewUserAgent()
        }
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

    // Close affordance (docked panel, no dialog chrome).
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 6
        width: 26; height: 26; radius: 13
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
