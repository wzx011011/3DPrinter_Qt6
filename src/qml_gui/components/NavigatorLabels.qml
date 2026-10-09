import QtQuick
import ".."

// v5.16 (NAVIGATOR): label overlay for the bottom-left 3D navigator cube.
// Upstream ImGuizmo renders uppercase axis labels ("Y"/"Z"/"X", matching the
// transform widgets, GLCanvas3D.cpp:6144-6146) at the projection of
// origin + unit dirAxis * 1.2f (1.2 axis lengths, not half-axes), centered on
// that point (ImGuizmo.cpp:3052-3054), plus a solid-color label on every
// visible face (face pass 2955-2965, text warped onto the face quad at
// 2995-2998; visibility test 3013-3028; GLCanvas3D.cpp:5688-5691). Pending
// viewport-side gaps this component cannot fix alone: the C++ anchor still
// projects at 1.3f, axis labels carry no dim state (upstream dims the
// back-facing line+label pair to 35% alpha, ImGuizmo.cpp:3031-3033), and
// face text renders flat instead of warped onto the face quad. Upstream
// draws these labels as single solid AddText calls with no outline/shadow.
// The viewport owns the geometry (navigatorLabels property, item-pixel
// anchors); this component only renders text.
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
            // face labels render on every visible face (ImGuizmo.cpp:2955).
            // No Text.Outline: upstream draws these labels as single
            // solid-color AddText calls (face ImGuizmo.cpp:2965, axis 3054)
            // with no stroke or shadow.
            color: Theme.textPrimary
            font.pixelSize: 13
        }
    }
}
