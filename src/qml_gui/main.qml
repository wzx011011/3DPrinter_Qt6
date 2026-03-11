import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Window
import "controls"
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
    property int pendingSwitchToken: -1
    property int pendingSwitchTargetPage: -1
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

    component TitleBarDivider: Rectangle {
        implicitWidth: 1
        implicitHeight: 18
        radius: 1
        color: "#263040"
    }

    Connections {
        target: backend
        function onLanguageChanged() { root.workflowTabs = root.buildWorkflowTabs() }
    }

    onFrameSwapped: {
        if (root.pendingSwitchToken >= 0 && backend.currentPage === root.pendingSwitchTargetPage) {
            backend.endLatency(root.pendingSwitchToken)
            root.pendingSwitchToken = -1
            root.pendingSwitchTargetPage = -1
        }
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
    Shortcut {
        sequence: "Delete"
        enabled: backend.currentPage === root.pagePrepare
                 && backend.editorViewModel
                 && backend.editorViewModel.hasSelection
        onActivated: backend.editorViewModel.deleteSelection()
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
                Layout.preferredHeight: 52
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
                    anchors.leftMargin: 14
                    anchors.rightMargin: 8
                    spacing: 8

                    Rectangle {
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 32
                        radius: 9
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#152230" }
                            GradientStop { position: 1.0; color: "#0f161f" }
                        }
                        border.color: "#2b394d"

                        Rectangle {
                            width: 16
                            height: 16
                            radius: 5
                            anchors.centerIn: parent
                            color: root.accentColor
                            opacity: 0.12
                        }

                        Image {
                            anchors.centerIn: parent
                            width: 18
                            height: 18
                            source: "qrc:/qml/assets/icons/printer.svg"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }
                    }

                    Column {
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 0

                        Text {
                            text: qsTr("Creality Print")
                            color: "#edf3fb"
                            font.pixelSize: 12
                            font.bold: true
                        }

                        Text {
                            text: qsTr("工作区")
                            color: "#7f90a6"
                            font.pixelSize: 10
                        }
                    }

                    Item { Layout.fillWidth: true }

                    RowLayout {
                        id: centeredGroups
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                        spacing: 8

                        Rectangle {
                            id: fileEntry
                            Layout.preferredHeight: 30
                            Layout.preferredWidth: 104
                            radius: 9
                            border.width: 1
                            border.color: fileMouse.containsMouse ? "#314058" : "#253043"
                            color: fileMouse.pressed ? root.topbarPressed : (fileMouse.containsMouse ? root.topbarHover : "#10161e")

                            RowLayout {
                                anchors.centerIn: parent
                                spacing: 6

                                Image {
                                    width: 14
                                    height: 14
                                    source: "qrc:/qml/assets/icons/folder-open.svg"
                                    fillMode: Image.PreserveAspectFit
                                    smooth: true
                                }

                                Text {
                                    text: qsTr("文件")
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

                        TitleBarDivider { }

                        Rectangle {
                            Layout.preferredHeight: 34
                            Layout.preferredWidth: actionTools.implicitWidth + 16
                            radius: 10
                            color: "#0f151d"
                            border.color: "#253043"

                            RowLayout {
                                id: actionTools
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                spacing: 4

                                Rectangle {
                                    Layout.preferredWidth: 28
                                    Layout.preferredHeight: 28
                                    radius: 7
                                    color: "#14202c"
                                    border.color: "#223041"

                                    Image {
                                        anchors.centerIn: parent
                                        width: 14
                                        height: 14
                                        source: "qrc:/qml/assets/icons/layout-grid.svg"
                                        fillMode: Image.PreserveAspectFit
                                        smooth: true
                                    }
                                }

                                CxIconButton {
                                    cxStyle: CxIconButton.Style.Chrome
                                    buttonSize: 34
                                    iconSize: 18
                                    iconSource: "qrc:/qml/assets/icons/device-floppy.svg"
                                    toolTipText: qsTr("保存项目")
                                    onClicked: {
                                        if (!backend.topbarSaveProject())
                                            saveProjectAsDialog.open()
                                    }
                                }

                                CxIconButton {
                                    cxStyle: CxIconButton.Style.Chrome
                                    buttonSize: 34
                                    iconSize: 18
                                    iconSource: "qrc:/qml/assets/icons/arrow-back-up.svg"
                                    toolTipText: qsTr("撤销")
                                    enabled: backend.currentPage === root.pagePrepare
                                    onClicked: if (backend.currentPage === root.pagePrepare) preparePage.undoFromTopbar()
                                }

                                CxIconButton {
                                    cxStyle: CxIconButton.Style.Chrome
                                    buttonSize: 34
                                    iconSize: 18
                                    iconSource: "qrc:/qml/assets/icons/arrow-forward-up.svg"
                                    toolTipText: qsTr("重做")
                                    enabled: backend.currentPage === root.pagePrepare
                                    onClicked: if (backend.currentPage === root.pagePrepare) preparePage.redoFromTopbar()
                                }
                            }
                        }

                        Rectangle {
                            Layout.preferredHeight: 34
                            Layout.preferredWidth: tabsRow.implicitWidth + 14
                            radius: 11
                            color: "#0f151d"
                            border.color: "#253043"

                            RowLayout {
                                id: tabsRow
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                spacing: 6

                                Repeater {
                                    model: root.workflowTabs
                                    delegate: Rectangle {
                                        required property var modelData
                                        Layout.preferredHeight: 26
                                        Layout.preferredWidth: Math.max(82, implicitLbl.implicitWidth + 28)
                                        radius: 8
                                        color: backend.currentPage === modelData.page
                                            ? "#183425"
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
                                        TapHandler {
                                            onTapped: {
                                                if (backend.currentPage === parent.modelData.page)
                                                    return
                                                root.pendingSwitchToken = backend.beginLatency("tab-switch", parent.modelData.label)
                                                root.pendingSwitchTargetPage = parent.modelData.page
                                                backend.setCurrentPage(parent.modelData.page)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    RowLayout {
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        spacing: 8

                        Rectangle {
                            Layout.preferredHeight: 30
                            Layout.preferredWidth: 224
                            Layout.maximumWidth: 280
                            radius: 9
                            color: "#10161e"
                            border.color: "#253043"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: 8

                                Rectangle {
                                    width: 6
                                    height: 6
                                    radius: 3
                                    color: root.accentColor
                                }

                                Label {
                                    id: projectTitleLabel
                                    Layout.fillWidth: true
                                    text: root.currentProjectTitle()
                                    color: "#c9d4e4"
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                    verticalAlignment: Text.AlignVCenter
                                    ToolTip.visible: titleMouse.containsMouse && text.length > 0
                                    ToolTip.text: text
                                }

                                MouseArea {
                                    id: titleMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.NoButton
                                }
                            }
                        }

                        CxIconButton {
                            id: moreActionsButton
                            cxStyle: CxIconButton.Style.Chrome
                            buttonSize: 34
                            iconSize: 18
                            iconSource: "qrc:/qml/assets/icons/dots.svg"
                            toolTipText: qsTr("更多")
                            onClicked: {
                                const p = moreActionsButton.mapToItem(root.contentItem, 0, moreActionsButton.height + 4)
                                dropdownMenu.popup(p.x, p.y)
                            }
                        }

                        Rectangle {
                            Layout.preferredHeight: 34
                            Layout.preferredWidth: windowControls.implicitWidth + 16
                            radius: 10
                            color: "#0f151d"
                            border.color: "#253043"

                            RowLayout {
                                id: windowControls
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                spacing: 4

                                CxIconButton {
                                    cxStyle: CxIconButton.Style.Chrome
                                    buttonSize: 34
                                    iconSize: 16
                                    iconSource: "qrc:/qml/assets/icons/minus.svg"
                                    toolTipText: qsTr("最小化")
                                    onClicked: root.showMinimized()
                                }

                                CxIconButton {
                                    cxStyle: CxIconButton.Style.Chrome
                                    buttonSize: 34
                                    iconSize: 16
                                    iconSource: root.visibility === Window.Maximized
                                        ? "qrc:/qml/assets/icons/restore.svg"
                                        : "qrc:/qml/assets/icons/maximize.svg"
                                    toolTipText: root.visibility === Window.Maximized ? qsTr("还原") : qsTr("最大化")
                                    onClicked: root.visibility === Window.Maximized ? root.showNormal() : root.showMaximized()
                                }

                                CxIconButton {
                                    cxStyle: CxIconButton.Style.ChromeDanger
                                    buttonSize: 34
                                    iconSize: 16
                                    iconSource: "qrc:/qml/assets/icons/x.svg"
                                    toolTipText: qsTr("关闭")
                                    onClicked: Qt.quit()
                                }
                            }
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
                statusText: "就绪  |  Qt 6.10  |  页面 " + (backend.currentPage + 1) + "  |  " + backend.latencyBrief
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
