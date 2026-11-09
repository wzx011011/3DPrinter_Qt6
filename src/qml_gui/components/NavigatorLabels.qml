import QtQuick
pragma ComponentBehavior: Bound
import OWzx

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
        // Key the delegate objects on the label COUNT (stable during orbit)
        // and read the per-index geometry live below. Binding this Repeater's
        // model to the full QVariantList re-assigned the delegate model on
        // every navigatorLabelsChanged emission (60 Hz mid-orbit), which
        // crashed QQuickRepeater::disconnectModel (null ConnectionData) on
        // Qt 6.12. Count-keyed delegates persist; only their geometry rebinds.
        model: root.viewport ? root.viewport.navigatorLabelCount : 0

        delegate: Text {
            // Qt6: explicit required injection avoids the QVariantMap
            // modelData scope ambiguity that silently yields NaN coordinates.
            required property int index
            property var md: {
                const v = root.viewport
                if (!v)
                    return null
                const d = v.navigatorLabels
                return index < d.length ? d[index] : null
            }
            property string faceText: {
                const map = {
                    front: qsTr("Front"),
                    back: qsTr("Back"),
                    top: qsTr("Top"),
                    bottom: qsTr("Bottom"),
                    left: qsTr("Left"),
                    right: qsTr("Right")
                }
                return md ? (map[md.text] || md.text) : ""
            }

            visible: md !== null
            x: md ? md.x - width / 2 : 0
            y: md ? md.y - height / 2 : 0
            text: md ? (md.kind === "axis" ? md.text : faceText) : ""
            // Upstream TEXT color dark theme 224/255 (GLCanvas3D.cpp:5685);
            // face labels render on every visible face (ImGuizmo.cpp:2955).
            // No Text.Outline: upstream draws these labels as single
            // solid-color AddText calls (face ImGuizmo.cpp:2965, axis 3054)
            // with no stroke or shadow.
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSize13
        }
    }
}
