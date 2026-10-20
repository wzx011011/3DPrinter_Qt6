import QtQuick
pragma ComponentBehavior: Bound
import QtQuick.Controls
import OWzx

// MP9 (dev plan 多盘 2026-10-20): the Select Plate rail — upstream
// _render_imgui_select_plate_toolbar (GLCanvas3D.cpp:8909-9330): a vertical
// rail on the canvas' left edge with a "Slice All" header and one thumbnail
// button per plate. Layout follows upstream (left edge, 100px-class
// thumbnails, state badges, empty plates dimmed); colors are brand Theme
// tokens per the 2026-10-07 ruling. MP9-followup completes the badge set:
// the slicing badge shows the live percent (editorVm.sliceProgress property,
// NOT the plateRailModel — per-percent model rebuilds would re-decode every
// thumbnail) and a persistent 切片失败 badge rides the VM's failed-plate set
// (latched at sliceFailed, cleared on that plate's next success).
Rectangle {
    id: root

    property var editorVm: null
    /// (plateIndex) => void — PreparePage owns the activation semantics
    /// (sliced -> jump to preview; unsliced non-empty -> switch + slice).
    property var plateActivated: null

    readonly property var railModel: editorVm ? editorVm.plateRailModel : []
    readonly property int slicedCount: {
        let n = 0
        for (let i = 0; i < railModel.length; ++i)
            if (railModel[i].sliced) ++n
        return n
    }

    width: 116
    radius: 8
    color: Theme.bgCard
    border.color: Theme.borderSubtle
    border.width: 1
    opacity: railModel.length > 0 ? 1.0 : 0.0
    visible: railModel.length > 1  // single plate: the rail adds nothing

    Column {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        // Header: Slice All (upstream EVT_GLTOOLBAR_SLICE_ALL stats button).
        Rectangle {
            width: parent.width
            height: 30
            radius: 5
            readonly property bool hovered: sliceAllMouse.containsMouse
            readonly property bool pressed: sliceAllMouse.pressed
            color: pressed ? Theme.accentDark
                 : hovered ? Theme.accentLight : Theme.accent
            Text {
                anchors.centerIn: parent
                text: qsTr("全部切片")
                color: "white"
                font.pixelSize: Theme.fontSizeSM
                font.bold: true
            }
            MouseArea {
                id: sliceAllMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: if (root.editorVm) root.editorVm.requestSliceAll()
            }
        }

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            // Upstream stats item: sliced/total across the plates.
            text: qsTr("%1/%2 已切片").arg(root.slicedCount).arg(root.railModel.length)
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeXS
        }

        Repeater {
            model: root.railModel

            delegate: Rectangle {
                id: plateButton
                required property var modelData
                width: parent.width
                height: 104
                radius: 6
                // Upstream dims empty plates (no printable instances).
                opacity: plateButton.modelData.objectCount > 0 ? 1.0 : 0.55
                color: plateMouse.pressed ? Theme.bgHover
                     : plateMouse.containsMouse ? Theme.bgElevated : "transparent"
                border.width: plateButton.modelData.current ? 2 : 1
                border.color: plateButton.modelData.current
                              ? Theme.accent : Theme.borderSubtle

                Image {
                    id: thumb
                    anchors.fill: parent
                    anchors.margins: 6
                    fillMode: Image.PreserveAspectFit
                    source: plateButton.modelData.thumb
                            ? "data:image/png;base64," + plateButton.modelData.thumb : ""
                    visible: plateButton.modelData.thumb !== ""
                }

                Text {
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    anchors.margins: 4
                    width: parent.width - 8
                    text: plateButton.modelData.name
                    elide: Text.ElideRight
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeXS
                }

                // State badge (upstream SLICED / SLICING overlays,
                // GLCanvas3D.cpp:9281-9310). The slicing variant carries the
                // live percent; upstream renders the percentage inside the
                // spinner badge (:9296-9304).
                Rectangle {
                    visible: plateButton.modelData.sliced || plateButton.modelData.slicing
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: 4
                    width: badgeText.implicitWidth + 10
                    height: 16
                    radius: 8
                    color: plateButton.modelData.sliced ? Theme.accent : "#f2754e"
                    Text {
                        id: badgeText
                        anchors.centerIn: parent
                        text: plateButton.modelData.sliced
                              ? qsTr("已切片")
                              : (root.editorVm && root.editorVm.isSlicing()
                                     ? qsTr("%1%").arg(root.editorVm.sliceProgress)
                                     : qsTr("切片中"))
                        color: "white"
                        font.pixelSize: Theme.fontSizeXS
                    }
                }

                // Persistent failure badge (upstream SLICE_FAILED, latched
                // until the plate re-slices successfully). Hidden while a
                // new run is in flight or a valid result exists.
                Rectangle {
                    visible: plateButton.modelData.failed
                             && !plateButton.modelData.slicing
                             && !plateButton.modelData.sliced
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: 4
                    width: failedText.implicitWidth + 10
                    height: 16
                    radius: 8
                    color: "#d94040"
                    Text {
                        id: failedText
                        anchors.centerIn: parent
                        text: qsTr("切片失败")
                        color: "white"
                        font.pixelSize: Theme.fontSizeXS
                    }
                }

                MouseArea {
                    id: plateMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (root.plateActivated)
                            root.plateActivated(plateButton.modelData.index)
                    }
                }
            }
        }
    }
}
