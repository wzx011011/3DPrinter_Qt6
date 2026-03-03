import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "pages"
import "components"

ApplicationWindow {
    id: root
    width: 1828
    height: 1000
    visible: true
    title: "Creality Print 7.0 - QML"
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint
    minimumWidth: 1100
    minimumHeight: 700

    readonly property int resizeMargin: 6
    readonly property int frameMargin: (backend.visualCompareMode || root.visibility === Window.Maximized) ? 0 : 10
    readonly property int frameRadius: (backend.visualCompareMode || root.visibility === Window.Maximized) ? 0 : 18

    // 导航标签列表，语言切换后重新赋値以触发重绘
    property var navTabs: buildNavTabs()
    function buildNavTabs() {
        return [
            { label: qsTr("主页"),   page: 0 },
            { label: qsTr("准备"),   page: 1 },
            { label: qsTr("预览"),   page: 2 },
            { label: qsTr("设备"),   page: 3 },
            { label: qsTr("项目"),   page: 4 },
            { label: qsTr("校准"),   page: 5 },
            { label: qsTr("辅助"),   page: 6 },
            { label: qsTr("设备列表"), page: 7 },
            { label: qsTr("偏好"),   page: 8 },
            { label: qsTr("模型商城"), page: 9 },
            { label: qsTr("多机"),   page: 10 },
            { label: qsTr("参数设置"), page: 11 }
        ]
    }
    Connections {
        target: backend
        function onLanguageChanged() { root.navTabs = root.buildNavTabs() }
    }
        readonly property string compareReferenceSource: backend.currentPage === 1
                                                                                                    ? "qrc:/qml/assets/prepare_ref.png"
                                                                                                    : backend.currentPage === 2
                                                                                                        ? "qrc:/qml/assets/preview_ref.png"
                                                                                                        : backend.currentPage === 3
                                                                                                            ? "qrc:/qml/assets/monitor_ref.png"
                                                                                                            : ""

    Rectangle {
        id: shell
        anchors.fill: parent
        anchors.margins: root.frameMargin
        radius: root.frameRadius
        color: backend.bgColor
        border.color: backend.borderColor
        border.width: root.visibility === Window.Maximized ? 0 : 1
        clip: true

        transform: Scale {
            xScale: backend.uiScale
            yScale: backend.uiScale
            origin.x: 0
            origin.y: 0
        }

        Image {
            anchors.fill: parent
            visible: backend.visualCompareMode && root.compareReferenceSource !== ""
            source: root.compareReferenceSource
            fillMode: Image.Stretch
        }

        ColumnLayout {
            visible: !backend.visualCompareMode || root.compareReferenceSource === ""
            anchors.fill: parent
            spacing: 0

            Rectangle {
                id: titleBar
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                color: backend.surfaceColor

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    propagateComposedEvents: true
                    onPressed: (mouse) => {
                        if (mouse.button === Qt.LeftButton && root.visibility !== Window.Maximized)
                            root.startSystemMove()
                    }
                    onDoubleClicked: {
                        if (root.visibility === Window.Maximized) {
                            root.showNormal()
                        } else {
                            root.showMaximized()
                        }
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 6
                    spacing: 4

                    // Logo/branding
                    Text {
                        text: "✦ Creality Print"
                        color: "#18c75e"
                        font.bold: true
                        font.pixelSize: 13
                    }

                    Rectangle { width: 1; height: 20; color: "#2e3444" }

                    // 11-tab navigation
                    Repeater {
                        model: root.navTabs
                        delegate: Rectangle {
                            required property var modelData
                            Layout.preferredHeight: 30
                            Layout.preferredWidth: implicitLbl.width + 16
                            radius: 4
                            color: backend.currentPage === modelData.page
                                   ? "#1c3828"
                                   : (tabHov.containsMouse ? "#1c2130" : "transparent")
                            border.color: backend.currentPage === modelData.page ? "#18c75e" : "transparent"
                            border.width: 1

                            Text {
                                id: implicitLbl
                                anchors.centerIn: parent
                                text: parent.modelData.label
                                color: backend.currentPage === parent.modelData.page ? "#18c75e" : "#a0abbe"
                                font.pixelSize: 11
                                font.bold: backend.currentPage === parent.modelData.page
                            }

                            HoverHandler { id: tabHov }
                            TapHandler { onTapped: backend.setCurrentPage(parent.modelData.page) }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Window controls
                    ToolButton {
                        text: "－"
                        implicitWidth: 28; implicitHeight: 28
                        onClicked: root.showMinimized()
                    }
                    ToolButton {
                        text: root.visibility === Window.Maximized ? "❐" : "□"
                        implicitWidth: 28; implicitHeight: 28
                        onClicked: root.visibility === Window.Maximized ? root.showNormal() : root.showMaximized()
                    }
                    ToolButton {
                        text: "✕"
                        implicitWidth: 28; implicitHeight: 28
                        onClicked: Qt.quit()
                    }
                }
            }

            // Warning-level error banner (severity=1)
            ErrorBanner { }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: backend.currentPage

                // Page 0 — Home (Loader: lazy to avoid Qt6 QVariantList/V4 crash during static init)
                Loader {
                    active: backend.currentPage === 0
                    sourceComponent: Component {
                        HomePage { homeVm: backend.homeViewModel }
                    }
                }
                // Page 1 — Prepare (3D editor)
                PreparePage {
                    editorVm: backend.editorViewModel
                    configVm: backend.configViewModel
                }
                // Page 2 — Preview (G-code)
                PreviewPage {
                    previewVm: backend.previewViewModel
                }
                // Page 3 — Monitor
                MonitorPage {
                    monitorVm: backend.monitorViewModel
                }
                // Page 4 — Project (Loader: fileTree is QVariantList)
                Loader {
                    active: backend.currentPage === 4
                    sourceComponent: Component {
                        ProjectPage { projectVm: backend.projectViewModel }
                    }
                }
                // Page 5 — Calibration (Loader: lazy)
                Loader {
                    active: backend.currentPage === 5
                    sourceComponent: Component {
                        CalibrationPage { calibrationVm: backend.calibrationViewModel }
                    }
                }
                // Page 6 — Auxiliary
                AuxiliaryPage {
                    projectVm: backend.projectViewModel
                }
                // Page 7 — Device List (Loader: lazy)
                Loader {
                    active: backend.currentPage === 7
                    sourceComponent: Component {
                        DeviceListPage { monitorVm: backend.monitorViewModel }
                    }
                }
                // Page 8 — Preferences
                PreferencesPage {
                    settingsVm: backend.settingsViewModel
                }
                // Page 9 — Model Mall (Loader: lazy)
                Loader {
                    active: backend.currentPage === 9
                    sourceComponent: Component {
                        ModelMallPage { modelMallVm: backend.modelMallViewModel }
                    }
                }
                // Page 10 — Multi-machine (Loader: lazy)
                Loader {
                    active: backend.currentPage === 10
                    sourceComponent: Component {
                        MultiMachinePage { multiMachineVm: backend.multiMachineViewModel }
                    }
                }
                // Page 11 — Settings (full-screen parameter editor)
                Loader {
                    active: backend.currentPage === 11
                    sourceComponent: Component {
                        SettingsPage { configVm: backend.configViewModel }
                    }
                }
            }

            // Status bar
            StatusBar {
                Layout.fillWidth: true
                statusText: "就绪  |  Qt 6.10  |  页面 " + (backend.currentPage + 1)
            }
        }

        // Floating Info toast (severity=0), z-stacked over shell content
        ErrorToast { }
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.width: 0
        z: 999
        visible: root.visibility !== Window.Maximized && !backend.visualCompareMode

        MouseArea {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: root.resizeMargin
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton)
                    root.startSystemResize(Qt.LeftEdge)
            }
        }

        MouseArea {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: root.resizeMargin
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton)
                    root.startSystemResize(Qt.RightEdge)
            }
        }

        MouseArea {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: root.resizeMargin
            hoverEnabled: true
            cursorShape: Qt.SizeVerCursor
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton)
                    root.startSystemResize(Qt.TopEdge)
            }
        }

        MouseArea {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: root.resizeMargin
            hoverEnabled: true
            cursorShape: Qt.SizeVerCursor
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton)
                    root.startSystemResize(Qt.BottomEdge)
            }
        }

        MouseArea {
            anchors.left: parent.left
            anchors.top: parent.top
            width: root.resizeMargin + 4
            height: root.resizeMargin + 4
            hoverEnabled: true
            cursorShape: Qt.SizeFDiagCursor
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton)
                    root.startSystemResize(Qt.TopEdge | Qt.LeftEdge)
            }
        }

        MouseArea {
            anchors.right: parent.right
            anchors.top: parent.top
            width: root.resizeMargin + 4
            height: root.resizeMargin + 4
            hoverEnabled: true
            cursorShape: Qt.SizeBDiagCursor
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton)
                    root.startSystemResize(Qt.TopEdge | Qt.RightEdge)
            }
        }

        MouseArea {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            width: root.resizeMargin + 4
            height: root.resizeMargin + 4
            hoverEnabled: true
            cursorShape: Qt.SizeBDiagCursor
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton)
                    root.startSystemResize(Qt.BottomEdge | Qt.LeftEdge)
            }
        }

        MouseArea {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: root.resizeMargin + 4
            height: root.resizeMargin + 4
            hoverEnabled: true
            cursorShape: Qt.SizeFDiagCursor
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton)
                    root.startSystemResize(Qt.BottomEdge | Qt.RightEdge)
            }
        }
    }

}
