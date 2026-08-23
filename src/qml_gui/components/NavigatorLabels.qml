import QtQuick
import ".."

// v5.16 (NAVIGATOR): label overlay for the bottom-left 3D navigator cube.
// Upstream ImGuizmo renders the axis labels ("x"/"y"/"z") at 1.3x the axis
// direction plus a label on the hovered face (ImGuizmo.cpp:2942/3037,
// GLCanvas3D.cpp:5688-5691). The viewport owns the geometry (navigatorLabels
// property, item-pixel anchors); this component only renders text.
Item {
    id: root

    required property var viewport

    anchors.fill: parent

    Repeater {
        model: root.viewport ? root.viewport.navigatorLabels : []

        delegate: Text {
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
