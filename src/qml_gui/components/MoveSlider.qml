import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// 对齐上游 IMSlider — 播放/暂停、时间轴滑块、当前时间显示、彩色色带
Item {
    id: root
    required property var previewVm

    property int totalMoves: root.previewVm ? root.previewVm.moveCount : 0
    property int toolChangeCount: root.previewVm ? root.previewVm.toolChangePositionCount() : 0

    RowLayout {
        anchors.fill: parent
        spacing: Theme.spacingMD

        // Play / Pause button (对齐上游 draw_circle_frame + set_animating)
        CxButton {
            text: root.previewVm && root.previewVm.isPlaying ? "⏸" : "▶"
            font.pixelSize: 14
            implicitWidth: 32
            implicitHeight: 28
            onClicked: if (root.previewVm) root.previewVm.togglePlayPause()
        }

        // Time axis slider with colored band (对齐上游 IMSlider groove + draw_colored_band)
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 36

            // Colored bands for tool changes (对齐上游 IMSlider draw_colored_band)
            Item {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 6
                anchors.topMargin: 15

                Rectangle {
                    anchors.fill: parent
                    radius: 3
                    color: "#2a3040"
                }

                Repeater {
                    model: root.toolChangeCount
                    delegate: Rectangle {
                        visible: root.totalMoves > 0
                        x: root.totalMoves > 0
                           ? (root.previewVm.toolChangePositionAt(index) / root.totalMoves) * parent.width
                           : 0
                        width: {
                            if (!root.previewVm || root.totalMoves <= 0) return 0
                            var nextPos = (index + 1 < root.toolChangeCount)
                                ? root.previewVm.toolChangePositionAt(index + 1)
                                : root.totalMoves
                            return ((nextPos - root.previewVm.toolChangePositionAt(index)) / root.totalMoves) * parent.width
                        }
                        height: parent.height
                        radius: 3
                        color: root.previewVm
                            ? root.previewVm.extruderColor(root.previewVm.toolChangeExtruderIdAt(index))
                            : "#009688"
                        opacity: 0.6
                    }
                }
            }

            // Slider overlay
            CxSlider {
                id: moveSlider
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                from: 0
                to: Math.max(0, root.totalMoves)
                stepSize: 1
                value: root.previewVm ? root.previewVm.currentMove : 0
                onMoved: if (root.previewVm) root.previewVm.setCurrentMove(Math.round(value))
            }

            // Hover time tooltip (对齐上游 IMSlider hover 时间提示)
            Rectangle {
                id: hoverTooltip
                anchors.bottom: moveSlider.top
                anchors.bottomMargin: 6
                x: {
                    if (!sliderHoverArea.containsMouse) return -100
                    return Math.max(0, Math.min(sliderHoverArea.mouseX - width / 2, parent.width - width))
                }
                width: hoverTimeText.implicitWidth + 14
                height: 22
                radius: 6
                color: "#1a2332"
                border.width: 1
                border.color: Theme.borderSubtle
                visible: sliderHoverArea.containsMouse && root.totalMoves > 0

                Text {
                    id: hoverTimeText
                    anchors.centerIn: parent
                    text: {
                        if (!sliderHoverArea.containsMouse || !root.previewVm || root.totalMoves <= 0)
                            return ""
                        var hoverMove = (sliderHoverArea.mouseX / moveSlider.width) * root.totalMoves
                        hoverMove = Math.max(0, Math.min(Math.round(hoverMove), root.totalMoves))
                        return root.previewVm.timeAtMove(hoverMove)
                    }
                    color: Theme.textPrimary
                    font.pixelSize: 10
                    font.family: "monospace"
                }

                // Arrow
                Rectangle {
                    anchors.top: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 6
                    height: 6
                    rotation: 45
                    color: "#1a2332"
                }
            }

            // Invisible hover area over the slider track
            MouseArea {
                id: sliderHoverArea
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                hoverEnabled: true
                // Don't intercept clicks — pass through to CxSlider
                // Using acceptedButtons: Qt.NoButton to be a pure hover detector
                acceptedButtons: Qt.NoButton
            }
        }

        // Current elapsed time (对齐上游 IMSlider::get_label + short_and_splitted_time)
        Label {
            text: root.previewVm ? root.previewVm.currentTime : "0s"
            color: "#80cbc4"
            font.pixelSize: 12
            font.family: "monospace"
            Layout.minimumWidth: 50
            Layout.rightMargin: 4
        }

        // Total time
        Label {
            text: root.previewVm ? ("/ " + root.previewVm.totalTime) : "/ --:--:--"
            color: Theme.textSecondary
            font.pixelSize: 11
        }

        // Move counter
        Label {
            text: root.previewVm
                ? (root.previewVm.currentMove + " / " + root.previewVm.moveCount)
                : "0 / 0"
            color: Theme.textSecondary
            font.pixelSize: 10
            Layout.leftMargin: 4
        }
    }
}
