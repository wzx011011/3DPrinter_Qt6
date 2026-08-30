import QtQuick
import QtWebEngine
import QtWebChannel
import ".."
// ChatSidebar.qml - OWzx-only AI assistant chat panel (decision record:
// docs/ai-control.md step 2). The chat UI itself is a local web page
// (qrc:/web/chat/index.html, marked+DOMPurify card rendering) hosted in a
// WebEngineView and talking to the host over QWebChannel (AiChatBridge).
// This QML file is only the panel host: it carries the frame styling and
// the channel wiring; every visible message/permission/input control lives
// in the web page, and every action routes through the bridge back into
// AiViewModel (no logic here).
//
// Channel wiring (Qt 6.10 constraints, both measured not assumed):
// - WebEngineView.webChannel is typed QQmlWebChannel* — only the QML
//   WebChannel element satisfies it; a plain C++ QWebChannel silently
//   fails to attach and the page never sees `qt`.
// - The element's registeredObjects cannot be used for a C++-owned object:
//   in Qt 6 registration requires the attached WebChannel.id property on
//   the registered object, which only exists on objects declared in QML.
// So the element is handed to BackendContext (attachAiChatChannel), which
// registers the bridge through the public QWebChannel base API.

Rectangle {
    id: root

    readonly property QtObject bridge: backend ? backend.aiChatBridge : null
    signal closed()

    color: Theme.bgPanel
    border.width: 1
    border.color: Theme.borderSubtle

    Component.onCompleted: {
        if (backend)
            backend.attachAiChatChannel(chatChannel)
    }

    WebEngineView {
        id: webView
        anchors.fill: parent
        // webChannel MUST be assigned before url: the page navigation starts
        // on the url assignment, and Chromium only injects the
        // qt.webChannelTransport bootstrap for navigations that begin after
        // the channel is attached (document-order property assignment).
        webChannel: chatChannel
        url: root.bridge ? "qrc:/web/chat/index.html" : ""
        backgroundColor: "transparent"
        settings.showScrollBars: false
        settings.focusOnNavigationEnabled: true
    }

    WebChannel {
        id: chatChannel
    }

    Connections {
        target: root.bridge
        function onCloseRequested() { root.closed() }
    }
}
