import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

Item {
    id: root
    required property var previewVm

    property int totalMoves: root.previewVm ? root.previewVm.moveCount : 0
    property int toolChangeCount: root.previewVm ? root.previewVm.toolChangePositionCount() : 0

    RowLayout {
        anchors.fill: parent
        spacing: 6

        MoveStepButton {
            id: prevMajorMoveButton
            label: "<<"
            delta: -10
            controlEnabled: root.previewVm && root.totalMoves > 0
            onTriggered: root.previewVm.stepCurrentMove(prevMajorMoveButton.delta)
        }

        MoveStepButton {
            id: prevMoveButton
            label: "<"
            delta: -1
            controlEnabled: root.previewVm && root.totalMoves > 0
            onTriggered: root.previewVm.stepCurrentMove(prevMoveButton.delta)
        }

        CxButton {
            text: root.previewVm && root.previewVm.isPlaying ? qsTr("暂停") : qsTr("播放")
            compact: true
            implicitWidth: 52
            enabled: root.previewVm && root.totalMoves > 0
            onClicked: if (root.previewVm) root.previewVm.togglePlayPause()
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 36

            Item {
                id: toolBand
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 6
                anchors.topMargin: 15

                Rectangle {
                    anchors.fill: parent
                    radius: 3
                    color: Theme.borderSubtle
                }

                Repeater {
                    model: root.toolChangeCount
                    delegate: Rectangle {
                        visible: root.totalMoves > 0
                        x: root.totalMoves > 0
                           ? (root.previewVm.toolChangePositionAt(index) / root.totalMoves) * toolBand.width
                           : 0
                        width: {
                            if (!root.previewVm || root.totalMoves <= 0)
                                return 0
                            const nextPos = index + 1 < root.toolChangeCount
                                ? root.previewVm.toolChangePositionAt(index + 1)
                                : root.totalMoves
                            return ((nextPos - root.previewVm.toolChangePositionAt(index)) / root.totalMoves) * toolBand.width
                        }
                        height: toolBand.height
                        radius: 3
                        color: root.previewVm
                            ? root.previewVm.extruderColor(root.previewVm.toolChangeExtruderIdAt(index))
                            : Theme.accent
                        opacity: 0.65
                    }
                }
            }

            CxSlider {
                id: moveSlider
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                from: 0
                to: Math.max(0, root.totalMoves)
                stepSize: 1
                value: root.previewVm ? root.previewVm.currentMove : 0
                enabled: root.previewVm && root.totalMoves > 0
                onMoved: if (root.previewVm) root.previewVm.setCurrentMove(Math.round(value))
            }

            Rectangle {
                id: hoverTooltip
                anchors.bottom: moveSlider.top
                anchors.bottomMargin: 6
                x: {
                    if (!sliderHoverArea.containsMouse)
                        return -100
                    return Math.max(0, Math.min(sliderHoverArea.mouseX - width / 2, parent.width - width))
                }
                width: hoverTimeText.implicitWidth + 14
                height: 22
                radius: 4
                color: Theme.bgTooltip
                border.width: 1
                border.color: Theme.borderSubtle
                visible: sliderHoverArea.containsMouse && root.totalMoves > 0

                Text {
                    id: hoverTimeText
                    anchors.centerIn: parent
                    text: {
                        if (!sliderHoverArea.containsMouse || !root.previewVm || root.totalMoves <= 0)
                            return ""
                        let hoverMove = (sliderHoverArea.mouseX / Math.max(1, moveSlider.width)) * root.totalMoves
                        hoverMove = Math.max(0, Math.min(Math.round(hoverMove), root.totalMoves))
                        return root.previewVm.timeAtMove(hoverMove)
                    }
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeXS
                    font.family: "Consolas"
                }
            }

            MouseArea {
                id: sliderHoverArea
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
            }
        }

        Label {
            text: root.previewVm ? root.previewVm.currentTime : "0s"
            color: Theme.accentLight
            font.pixelSize: Theme.fontSizeSM
            font.family: "Consolas"
            Layout.minimumWidth: 54
            horizontalAlignment: Text.AlignRight
        }

        Label {
            text: root.previewVm ? "/ " + root.previewVm.totalTime : "/ --:--:--"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
            Layout.minimumWidth: 72
        }

        Label {
            text: root.previewVm ? root.previewVm.currentMoveLabel : "-- / --"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeXS
            font.family: "Consolas"
            Layout.minimumWidth: 86
            horizontalAlignment: Text.AlignRight
        }

        MoveStepButton {
            id: nextMoveButton
            label: ">"
            delta: 1
            controlEnabled: root.previewVm && root.totalMoves > 0
            onTriggered: root.previewVm.stepCurrentMove(nextMoveButton.delta)
        }

        MoveStepButton {
            id: nextMajorMoveButton
            label: ">>"
            delta: 10
            controlEnabled: root.previewVm && root.totalMoves > 0
            onTriggered: root.previewVm.stepCurrentMove(nextMajorMoveButton.delta)
        }
    }

    component MoveStepButton: Rectangle {
        id: moveStepButtonRoot
        property string label: ""
        property int delta: 0
        property bool controlEnabled: true
        signal triggered()

        Layout.preferredWidth: 28
        Layout.preferredHeight: 26
        radius: 4
        color: stepMouse.containsMouse && moveStepButtonRoot.controlEnabled ? Theme.bgHover : Theme.bgElevated
        border.width: 1
        border.color: stepMouse.containsMouse && moveStepButtonRoot.controlEnabled ? Theme.accentDark : Theme.borderSubtle
        opacity: moveStepButtonRoot.controlEnabled ? 1.0 : 0.45

        Text {
            anchors.centerIn: parent
            text: moveStepButtonRoot.label
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeSM
            font.family: "Consolas"
        }

        MouseArea {
            id: stepMouse
            anchors.fill: parent
            enabled: moveStepButtonRoot.controlEnabled
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: moveStepButtonRoot.triggered()
        }
    }
}
