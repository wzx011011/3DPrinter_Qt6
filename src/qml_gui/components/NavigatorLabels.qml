import QtQuick
import ".."

// v5.16 (NAVIGATOR): label overlay for the bottom-left 3D navigator cube.
// Upstream ImGuizmo renders the axis labels ("x"/"y"/"z") at 1.3x the axis
// direction plus a label on every visible face (ImGuizmo.cpp:2942/3037,
// GLCanvas3D.cpp:5688-5691). The viewport owns the geometry (navigatorLabels
// property, item-pixel anchors); this component only renders text.
Item {
    id: root

    required property var viewport

    anchors.fill: parent

    Repeater {
        id: labelRepeater
        // Read the viewport size before the anchors so this binding also
        // depends on the QQuickItem width/height notify. The item is bound
        // before layout settles (height is still the pre-layout value), and
        // the C++ geometryChange re-emit path proved unreliable on
        // QQuickRhiItem; the size notify is the dependable refresh trigger.
        model: {
            if (!root.viewport)
                return []
            const w = root.viewport.width
            const h = root.viewport.height
            return root.viewport.navigatorLabels
        }

        delegate: Text {
            // Qt6: explicit required injection avoids the QVariantMap
            // modelData scope ambiguity that silently yields NaN coordinates.
            required property var modelData
            property string faceText: {
                const map = {
                    front: qsTr("Front"),
                    back: qsTr("Back"),
                    top: qsTr("Top"),
                    bottom: qsTr("Bottom"),
                    left: qsTr("Left"),
                    right: qsTr("Right")
                }
                return map[modelData.text] || modelData.text
            }

            x: modelData.x - width / 2
            y: modelData.y - height / 2
            text: modelData.kind === "axis" ? modelData.text : faceText
            // Upstream TEXT color dark theme 224/255 (GLCanvas3D.cpp:5685);
            // face labels render on every visible face (ImGuizmo.cpp:2941).
            color: "#E0E0E0"
            font.pixelSize: 13
            style: Text.Outline
            styleColor: "#101010"
        }
    }
}
