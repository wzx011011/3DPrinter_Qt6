// ModeSwitch.qml - settings visibility mode switch.
//
// Presentation of upstream ModeSwitchButton (Tab.cpp:428-444,
// ParamsPanel.cpp:281-284): a three-position pill (Simple / Advanced /
// Expert). Clicking cycles the app-wide mode through
// ConfigViewModel.cycleUserMode(); developer mode locks it on Expert.

import QtQuick
import QtQuick.Controls
import OWzx

Item {
    id: root

    property var configVm: null
    property color accentColor: Theme.accent

    readonly property bool locked: !!root.configVm && root.configVm.developerMode
    readonly property int position: !root.configVm ? 0
                                    : (root.locked ? 2 : root.configVm.userMode)
    readonly property var modeNames: [qsTr("简单"), qsTr("高级"), qsTr("专家")]

    implicitWidth: 38
    implicitHeight: 18
    opacity: root.locked ? 0.5 : 1.0

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: root.accentColor

        Row {
            anchors.centerIn: parent
            spacing: 4

            Repeater {
                model: 3
                delegate: Rectangle {
                    required property int index
                    readonly property bool current: index === root.position
                    width: current ? 10 : 4
                    height: current ? 10 : 4
                    radius: width / 2
                    anchors.verticalCenter: parent.verticalCenter
                    color: "white"
                    opacity: current ? 1.0 : 0.7
                }
            }
        }
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        enabled: !root.locked
        cursorShape: Qt.PointingHandCursor
        onClicked: if (root.configVm) root.configVm.cycleUserMode()
    }

    ToolTip.visible: area.containsMouse
    ToolTip.delay: 500
    ToolTip.text: qsTr("参数显示模式：") + root.modeNames[root.position]
}
