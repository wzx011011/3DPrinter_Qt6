import QtQuick
import QtWebEngine
import QtWebChannel
import ".."

// ChatSidebar.qml - OWzx-only AI assistant chat panel (decision record:
// docs/ai-control.md step 2). The chat UI itself is a local web page
// (qrc:/web/chat/index.html, marked+DOMPurify card rendering) hosted in a
// WebEngineView and talking to the host over QWebChannel (AiChatBridge,
// exposed as backend.aiChatBridge). This QML file is only the panel host:
// it carries the frame styling and the channel registration; every visible
// message/permission/input control lives in the web page, and every action
// routes through the bridge back into AiViewModel (no logic here).

Rectangle {
    id: root

    property var bridge: null
    signal closed()

    color: Theme.bgPanel
    border.width: 1
    border.color: Theme.borderSubtle

    WebEngineView {
        id: webView
        anchors.fill: parent
        url: "qrc:/web/chat/index.html"
        webChannel: chatChannel
        backgroundColor: "transparent"
        settings.showScrollBars: false
        settings.focusOnNavigationEnabled: true
    }

    WebChannel {
        id: chatChannel
        registeredObjects: root.bridge ? [root.bridge] : []
    }

    Connections {
        target: root.bridge
        function onCloseRequested() { root.closed() }
    }
}
