import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
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

    readonly property int pagePrepare: 1
    readonly property int pagePreview: 2
    readonly property int pageDevice: 7
    readonly property int pageOnline: 9
    readonly property color accentColor: "#18c75e"
    readonly property color topbarHover: "#1b2230"
    readonly property color topbarPressed: "#242c3a"
    readonly property color topbarText: "#cdd7e6"

    property var workflowTabs: buildWorkflowTabs()
    function buildWorkflowTabs() {
        return [
            { label: qsTr("在线模型"), page: root.pageOnline },
            { label: qsTr("准备"), page: root.pagePrepare },
            { label: qsTr("预览"), page: root.pagePreview },
            { label: qsTr("设备"), page: root.pageDevice }
        ]
    }

    function currentProjectTitle() {
        if (backend.editorViewModel && backend.editorViewModel.projectName && backend.editorViewModel.projectName.length > 0)
            return backend.editorViewModel.projectName
        if (backend.projectViewModel && backend.projectViewModel.currentProjectPath && backend.projectViewModel.currentProjectPath.length > 0) {
            var p = backend.projectViewModel.currentProjectPath.toString()
            var parts = p.split(/[\\/]/)
            return parts.length > 0 ? parts[parts.length - 1] : p
        }
        return qsTr("未命名")
    }

    Connections {
        target: backend
        function onLanguageChanged() { root.workflowTabs = root.buildWorkflowTabs() }
    }

    FileDialog {
        id: openModelDialog
        title: qsTr("打开模型文件")
        nameFilters: [
            qsTr("3MF 文件 (*.3mf)"),
            qsTr("STL 文件 (*.stl)"),
            qsTr("OBJ 文件 (*.obj)"),
            qsTr("AMF 文件 (*.amf)"),
            qsTr("所有文件 (*)")
        ]
        onAccepted: {
            backend.topbarImportModel(selectedFile.toString())
        }
    }

    FileDialog {
        id: openProjectDialog
        title: qsTr("打开项目")
        nameFilters: [qsTr("项目文件 (*.3mf *.cxprj)"), qsTr("所有文件 (*)")]
        onAccepted: {
            backend.topbarOpenProject(selectedFile.toString())
        }
    }

    FileDialog {
        id: saveProjectAsDialog
        title: qsTr("项目另存为")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("项目文件 (*.3mf *.cxprj)")]
        onAccepted: {
            backend.topbarSaveProjectAs(selectedFile.toString())
        }
    }

    Menu {
        id: fileMenu
        MenuItem { text: qsTr("新建项目"); onTriggered: backend.topbarNewProject() }
        MenuItem { text: qsTr("打开项目..."); onTriggered: openProjectDialog.open() }
        MenuItem { text: qsTr("导入模型..."); onTriggered: openModelDialog.open() }
        MenuSeparator {}
        MenuItem {
            text: qsTr("保存项目")
            onTriggered: {
                if (!backend.topbarSaveProject())
                    saveProjectAsDialog.open()
            }
        }
        MenuItem { text: qsTr("项目另存为..."); onTriggered: saveProjectAsDialog.open() }
        MenuSeparator {}
        Menu {
            id: fileRecentMenu
            title: qsTr("最近文件")
            Instantiator {
                model: backend.projectViewModel ? backend.projectViewModel.recentProjects : []
                delegate: MenuItem {
                    required property string modelData
                    text: modelData
                    onTriggered: {
                        backend.topbarOpenProject(modelData)
                    }
                }
                onObjectAdded: (index, object) => fileRecentMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => fileRecentMenu.removeItem(object)
            }
            MenuSeparator {}
            MenuItem {
                text: qsTr("清空最近文件")
                enabled: backend.projectViewModel && backend.projectViewModel.recentProjects.length > 0
                onTriggered: backend.projectViewModel && backend.projectViewModel.clearRecentProjects()
            }
        }
        MenuSeparator {}
        MenuItem { text: qsTr("退出"); onTriggered: Qt.quit() }
    }

    Menu {
        id: dropdownMenu
        MenuItem { text: qsTr("偏好设置"); onTriggered: backend.setCurrentPage(8) }
        MenuItem { text: qsTr("参数设置"); onTriggered: backend.openSettings() }
        MenuSeparator {}
        MenuItem { text: qsTr("主页"); onTriggered: backend.setCurrentPage(0) }
    }

    Shortcut {
        sequence: StandardKey.Undo
        enabled: backend.currentPage === root.pagePrepare
        onActivated: preparePage.undoFromTopbar()
    }
    Shortcut {
        sequence: StandardKey.Save
        onActivated: {
            if (!backend.topbarSaveProject())
                saveProjectAsDialog.open()
        }
    }
    Shortcut {
        sequence: StandardKey.Redo
        enabled: backend.currentPage === root.pagePrepare
        onActivated: preparePage.redoFromTopbar()
    }
    Shortcut {
        sequence: "Ctrl+Shift+Z"
        enabled: backend.currentPage === root.pagePrepare
        onActivated: preparePage.redoFromTopbar()
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
                Layout.preferredHeight: 44
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
                    anchors.rightMargin: 8
                    spacing: 6

                    Rectangle {
                        Layout.preferredWidth: 22
                        Layout.preferredHeight: 22
                        radius: 4
                        color: "#121821"
                        border.color: "#273040"
                        Text {
                            anchors.centerIn: parent
                            text: "△"
                            color: root.accentColor
                            font.bold: true
                            font.pixelSize: 14
                        }
                    }

                    Rectangle {
                        id: fileEntry
                        Layout.preferredHeight: 28
                        Layout.preferredWidth: 92
                        radius: 7
                        border.width: 1
                        border.color: fileMouse.containsMouse ? "#314058" : "#253043"
                        color: fileMouse.pressed ? root.topbarPressed : (fileMouse.containsMouse ? root.topbarHover : "#151b25")
                        Row {
                            anchors.centerIn: parent
                            spacing: 6
                            Text {
                                text: qsTr("三文件")
                                color: "#dce5f1"
                                font.pixelSize: 12
                                font.bold: true
                            }
                            Text {
                                text: "▾"
                                color: "#9fb0c7"
                                font.pixelSize: 11
                            }
                        }
                        MouseArea {
                            id: fileMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                const p = fileEntry.mapToItem(root.contentItem, 0, fileEntry.height + 4)
                                fileMenu.popup(p.x, p.y)
                            }
                        }
                    }

                    Rectangle { width: 1; height: 18; color: "#2e3444" }

                    ToolButton {
                        text: qsTr("保")
                        implicitWidth: 28
                        implicitHeight: 28
                        flat: true
                        hoverEnabled: true
                        onClicked: {
                            if (!backend.topbarSaveProject())
                                saveProjectAsDialog.open()
                        }
                        background: Rectangle {
                            radius: 6
                            color: parent.down ? root.topbarPressed : (parent.hovered ? root.topbarHover : "transparent")
                            border.color: parent.hovered ? "#2f3d54" : "transparent"
                        }
                        contentItem: Text {
                            text: parent.text
                            color: root.topbarText
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }

                    ToolButton {
                        text: "↶"
                        implicitWidth: 28
                        implicitHeight: 28
                        enabled: backend.currentPage === root.pagePrepare
                        flat: true
                        hoverEnabled: true
                        onClicked: if (backend.currentPage === root.pagePrepare) preparePage.undoFromTopbar()
                        background: Rectangle {
                            radius: 6
                            color: parent.down ? root.topbarPressed : (parent.hovered ? root.topbarHover : "transparent")
                            border.color: parent.hovered ? "#2f3d54" : "transparent"
                            opacity: parent.enabled ? 1.0 : 0.45
                        }
                        contentItem: Text {
                            text: parent.text
                            color: parent.enabled ? root.topbarText : "#78879c"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 13
                        }
                    }

                    ToolButton {
                        text: "↷"
                        implicitWidth: 28
                        implicitHeight: 28
                        enabled: backend.currentPage === root.pagePrepare
                        flat: true
                        hoverEnabled: true
                        onClicked: if (backend.currentPage === root.pagePrepare) preparePage.redoFromTopbar()
                        background: Rectangle {
                            radius: 6
                            color: parent.down ? root.topbarPressed : (parent.hovered ? root.topbarHover : "transparent")
                            border.color: parent.hovered ? "#2f3d54" : "transparent"
                            opacity: parent.enabled ? 1.0 : 0.45
                        }
                        contentItem: Text {
                            text: parent.text
                            color: parent.enabled ? root.topbarText : "#78879c"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 13
                        }
                    }

                    Rectangle { width: 1; height: 18; color: "#2e3444" }

                    Repeater {
                        model: root.workflowTabs
                        delegate: Rectangle {
                            required property var modelData
                            Layout.preferredHeight: 30
                            Layout.preferredWidth: Math.max(76, implicitLbl.implicitWidth + 22)
                            radius: 6
                            color: backend.currentPage === modelData.page
                                ? "#1c3828"
                                : (tabHov.containsMouse ? root.topbarHover : "transparent")
                            border.color: backend.currentPage === modelData.page ? root.accentColor : "transparent"
                            border.width: 1

                            Text {
                                id: implicitLbl
                                anchors.centerIn: parent
                                text: parent.modelData.label
                                color: backend.currentPage === parent.modelData.page ? root.accentColor : "#a8b5c8"
                                font.pixelSize: 12
                                font.bold: backend.currentPage === parent.modelData.page
                            }

                            HoverHandler { id: tabHov }
                            TapHandler { onTapped: backend.setCurrentPage(parent.modelData.page) }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Label {
                        text: root.currentProjectTitle()
                        color: "#c9d4e4"
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignRight
                        Layout.preferredWidth: 220
                        Layout.maximumWidth: 260
                        ToolTip.visible: titleMouse.containsMouse && text.length > 0
                        ToolTip.text: text
                        MouseArea {
                            id: titleMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }
                    }

                    ToolButton {
                        text: "⋯"
                        implicitWidth: 28
                        implicitHeight: 28
                        flat: true
                        hoverEnabled: true
                        onClicked: {
                            const p = this.mapToItem(root.contentItem, 0, this.height + 4)
                            dropdownMenu.popup(p.x, p.y)
                        }
                        background: Rectangle {
                            radius: 6
                            color: parent.down ? root.topbarPressed : (parent.hovered ? root.topbarHover : "transparent")
                            border.color: parent.hovered ? "#2f3d54" : "transparent"
                        }
                        contentItem: Text {
                            text: parent.text
                            color: root.topbarText
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    ToolButton {
                        text: "－"
                        implicitWidth: 28
                        implicitHeight: 28
                        flat: true
                        hoverEnabled: true
                        onClicked: root.showMinimized()
                        background: Rectangle {
                            radius: 6
                            color: parent.down ? root.topbarPressed : (parent.hovered ? root.topbarHover : "transparent")
                        }
                        contentItem: Text {
                            text: parent.text
                            color: root.topbarText
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    ToolButton {
                        text: root.visibility === Window.Maximized ? "❐" : "□"
                        implicitWidth: 28
                        implicitHeight: 28
                        flat: true
                        hoverEnabled: true
                        onClicked: root.visibility === Window.Maximized ? root.showNormal() : root.showMaximized()
                        background: Rectangle {
                            radius: 6
                            color: parent.down ? root.topbarPressed : (parent.hovered ? root.topbarHover : "transparent")
                        }
                        contentItem: Text {
                            text: parent.text
                            color: root.topbarText
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    ToolButton {
                        text: "✕"
                        implicitWidth: 28
                        implicitHeight: 28
                        flat: true
                        hoverEnabled: true
                        onClicked: Qt.quit()
                        background: Rectangle {
                            radius: 6
                            color: parent.down ? "#aa1f2d" : (parent.hovered ? "#d33241" : "transparent")
                        }
                        contentItem: Text {
                            text: parent.text
                            color: parent.hovered ? "#ffffff" : "#d9e2ef"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: "#202838"
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
                    id: preparePage
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
