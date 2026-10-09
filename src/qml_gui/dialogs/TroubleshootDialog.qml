import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// TroubleshootDialog.qml — UI-04 Troubleshoot Center (upstream 1:1 port).
//
// Upstream truth: third_party/OrcaSlicer/src/slic3r/GUI/TroubleshootDialog.cpp
//   - Title _L("Troubleshoot Center") (:118), white body (:122),
//     SetSizerAndFit content-fitted window (:358).
//   - Left column: header logo ScalableBitmap("OrcaSlicer_horizontal", 64)
//     (:131), 2px brand underline (wxSize(-1, FromDIP(2)), #009687 -> OWzx
//     accent) (:133-134/:322), version label font x1.65 (:135-138/:323),
//     build-commit button opening the commit URL (:141-147/:324), centered
//     multi-line system-info panel (:150-165/:325) with Hide/Show + Copy
//     (:167-188/:327) and the Wiki Guide link (:190/:328).
//   - Right column: "Information" (3 wrapped descriptions + Report issue
//     hyperlink + Pack... button) (:229-265/:338-340), "Profiles" (clean
//     system profiles cache, loaded profiles overview + export, 6-column
//     Active/System/User grid) (:267-283/:342-345), "More" (configurations
//     folder, log level combo, stored logs pack/clear) (:285-311/:347-351).
//   - Margins FromDIP(15), 20px column gap (:353-356). No in-body close
//     button: upstream closes via title bar / Esc (wxDEFAULT_DIALOG_STYLE
//     only, :118-119).
// Backend: system RAM/GPU strings come from backend.systemInfo()
// (BackendContext::systemInfo); OS/package/CPU, log storage, ZIP packing,
// cache clean and the log-severity key live on MonitorViewModel (diagnostic*).
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root

    modal: true
    dialogTitle: qsTr("设备排错")
    // Upstream fits the two-column body (~880 DIP wide, :353-358).
    width: 880
    height: 640
    padding: 0

    required property var monitorVm

    // ── Data (refreshed on open, upstream ctor runs once per dialog) ──────
    property var sysInfo: ({})
    property var sysDiag: ({})
    // m_sys_panel_mode initial true (TroubleshootDialog.hpp:41).
    property bool sysPanelMode: true
    property string logSeverity: "info"
    property var logFiles: []
    property var profileCounts: null
    // Pending file list for the FolderDialog pack flows.
    property var _packPaths: []
    property string _packBaseName: ""

    readonly property var logLevels: ["fatal", "error", "warning", "info", "debug", "trace"]
    readonly property var logLevelsDisplay: logLevels.map(function(l) { return qsTr(l) })

    readonly property string appVersion: (sysInfo.appVersion || "")
    readonly property string buildCommit: (sysInfo.buildCommit || "")
    // Upstream opens the commit page of the running build (:143-147); the
    // OWzx commit hash belongs to this repository's GitHub remote.
    readonly property string buildCommitUrl: buildCommit !== ""
        ? "https://github.com/wzx011011/3DPrinter_Qt6/commit/" + buildCommit
        : ""

    // GetRAMinfo + " RAM" (TroubleshootDialog.cpp:157/:802-806).
    readonly property string ramLine: {
        var mb = sysInfo.totalRamMb
        if (mb === undefined || mb === null || mb <= 0)
            return qsTr("不可用")
        var tenths = Math.round((mb * 1048576) / 107374182.4)
        return Math.floor(tenths / 10) + "." + (tenths % 10) + " GB RAM"
    }

    // GetGPUinfo (TroubleshootDialog.cpp:808-816): renderer + GLSL + profile.
    readonly property string gpuLine: {
        var renderer = sysInfo.glRenderer || "n/a"
        var glsl = sysInfo.glslVersion || "n/a"
        var sf = sysInfo.surfaceFormat || ""
        var profile = sf.indexOf("Core") >= 0 ? "  Core" : "  Compatibility"
        return renderer + "  GLSL:" + glsl + profile
    }

    // GetMONinfo (TroubleshootDialog.cpp:818-903): "WxH-scale%" per monitor,
    // Windows appends TextScaling-N% (:892-900).
    readonly property string monitorsLine: {
        var screens = Qt.application.screens
        if (!screens || screens.length === 0)
            return "Unknown"
        var parts = []
        for (var i = 0; i < screens.length; ++i) {
            var s = screens[i]
            parts.push(s.width + "x" + s.height + "-"
                       + Math.round(s.devicePixelRatio * 100) + "%")
        }
        var line = parts.join("  ")
        if ((sysDiag.osType || "") === "Windows")
            line += "  TextScaling-" + Math.round(screens[0].devicePixelRatio * 100) + "%"
        return line
    }

    // sys_info_lines (TroubleshootDialog.cpp:150-161): collapsed shows only
    // GetOStype().
    readonly property var sysPanelLines: {
        if (!sysPanelMode)
            return [sysDiag.osType || ""]
        return [
            sysDiag.osInfo || qsTr("不可用"),
            sysDiag.packageType || qsTr("不可用"),
            sysDiag.cpuInfo || qsTr("不可用"),
            ramLine,
            gpuLine,
            monitorsLine
        ]
    }

    // UpdateLogsStorage label (TroubleshootDialog.cpp:1107-1125).
    readonly property string logsStorageText: {
        var files = logFiles
        if (!files || files.length === 0)
            return ""
        var total = 0
        for (var i = 0; i < files.length; ++i)
            total += files[i].bytes
        var isMb = total >= 1024 * 1024
        var sizeStr = (total / 1024 / (isMb ? 1024 : 1)).toFixed(2) + (isMb ? " MB" : " KB")
        return qsTr("%1 个日志 (%2)").arg(files.length).arg(sizeStr)
    }

    onAboutToShow: {
        sysInfo = (typeof backend !== "undefined" && backend && backend.systemInfo)
            ? backend.systemInfo() : {}
        sysDiag = monitorVm ? monitorVm.diagnosticSystemInfo() : {}
        logFiles = monitorVm ? monitorVm.diagnosticLogFiles() : []
        var stored = monitorVm ? monitorVm.logSeverityLevel() : ""
        logSeverity = stored !== "" ? stored : "info"
        logLevelCombo.currentIndex = Math.max(0, logLevels.indexOf(logSeverity))
        refreshProfileCounts()
    }

    // ── Profiles overview counts (upstream create_item_loaded_profiles,
    //    TroubleshootDialog.cpp:55-95) ─────────────────────────────────────
    function refreshProfileCounts() {
        var psvc = (typeof backend !== "undefined" && backend && backend.presetServiceMock)
            ? backend.presetServiceMock : null
        if (!psvc) {
            profileCounts = null
            return
        }
        // Active = enabled vendor printer models / filaments and processes
        // compatible with the selected printer (upstream m_*__act).
        var vendors = psvc.vendors()
        var printersActive = 0
        for (var i = 0; i < vendors.length; ++i)
            printersActive += psvc.printerModelsForVendor(vendors[i]).length
        var printerModel = psvc.selectedPrinterModel()
        function row(category, active) {
            var total = psvc.presetNamesForCategory(category).length
            var user = psvc.userPresetNamesForCategory(category).length
            return { active: active, system: total - user, user: user }
        }
        profileCounts = {
            printers: row(2, printersActive),
            filaments: row(1, psvc.compatiblePresetNamesForCategory(1, printerModel).length),
            processes: row(0, psvc.compatiblePresetNamesForCategory(0, printerModel).length)
        }
    }

    // GetProfilesOverview JSON (TroubleshootDialog.cpp:408-546), condensed to
    // the overview counts plus the per-collection enabled/user name lists
    // available from PresetServiceMock.
    function buildOverviewJson() {
        var psvc = (typeof backend !== "undefined" && backend && backend.presetServiceMock)
            ? backend.presetServiceMock : null
        if (!psvc || !profileCounts)
            return "{}"
        var printerModel = psvc.selectedPrinterModel()
        var vendors = psvc.vendors()
        var printerModels = []
        for (var i = 0; i < vendors.length; ++i)
            printerModels = printerModels.concat(psvc.printerModelsForVendor(vendors[i]))
        var overview = {
            printers__act: profileCounts.printers.active,
            printers__usr: profileCounts.printers.user,
            filaments_act: profileCounts.filaments.active,
            filaments_usr: profileCounts.filaments.user,
            processes_act: profileCounts.processes.active,
            processes_usr: profileCounts.processes.user
        }
        var doc = {
            "Overview": overview,
            "printers_enabled": printerModels,
            "printers_user": psvc.userPresetNamesForCategory(2),
            "filaments_enabled": psvc.compatiblePresetNamesForCategory(1, printerModel),
            "filaments_user": psvc.userPresetNamesForCategory(1),
            "processes_enabled": psvc.compatiblePresetNamesForCategory(0, printerModel),
            "processes_user": psvc.userPresetNamesForCategory(0)
        }
        return JSON.stringify(doc, null, 4)
    }

    // Report issue URL (TroubleshootDialog.cpp:237-259).
    function reportIssueUrl() {
        var url = "https://github.com/OrcaSlicer/OrcaSlicer/issues/new?template=bug_report.yml"
        var os = sysDiag.osType || ""
        if (os !== "")
            url += "&os_type=%22" + os + "%22"
        url += "&version=" + encodeURIComponent(appVersion)
        url += "&os_version=" + encodeURIComponent(sysDiag.osInfo || "")
        return url
    }

    function exportTimestamp() {
        // GetTimestamp format %Y%m%d_%H%M (TroubleshootDialog.cpp:365-369).
        return Qt.formatDateTime(new Date(), "yyyyMMdd_hhmm")
    }

    function notify(message, title, severity) {
        if (typeof backend !== "undefined" && backend && backend.postNotification)
            backend.postNotification(message, title, severity)
    }

    function startPackAll() {
        // PackAll (TroubleshootDialog.cpp:905-951): session logs + project
        // file; a dirty project is offered for saving first (:926-940).
        var proj = (typeof backend !== "undefined" && backend && backend.projectViewModel)
            ? backend.projectViewModel : null
        var projectPath = proj ? (proj.currentProjectPath || "") : ""
        if (projectPath !== "" && proj.isDirty) {
            saveDirtyDialog.open()
            return
        }
        if (projectPath === "")
            notify(qsTr("当前会话没有项目文件，压缩包将仅包含日志。"), qsTr("打包"), 1)
        _packPaths = collectPackPaths(projectPath)
        _packBaseName = "OWzx_PackedDebugInfo_" + exportTimestamp()
        packDestDialog.open()
    }

    function collectPackPaths(projectPath) {
        var paths = []
        var files = monitorVm ? monitorVm.diagnosticLogFiles() : []
        for (var i = 0; i < files.length; ++i)
            paths.push(files[i].path)
        if (projectPath !== "")
            paths.push(projectPath)
        return paths
    }

    function startLogsPack() {
        var paths = []
        var files = monitorVm ? monitorVm.diagnosticLogFiles() : []
        for (var i = 0; i < files.length; ++i)
            paths.push(files[i].path)
        _packPaths = paths
        _packBaseName = "OWzx_Logs_" + exportTimestamp()
        logsDestDialog.open()
    }

    function packTo(destDir) {
        var zipPath = monitorVm
            ? monitorVm.diagnosticPackZip(_packPaths, destDir, _packBaseName) : ""
        notify(zipPath !== ""
                   ? qsTr("已导出到 %1").arg(zipPath)
                   : qsTr("导出失败\n请检查写入权限或文件是否被占用"),
               qsTr("导出"), zipPath !== "" ? 0 : 2)
    }

    function clearLogs() {
        // ClearLogs (TroubleshootDialog.cpp:305-309/:1070-1105) keeps the
        // newest log, then refreshes the storage label.
        monitorVm.diagnosticClearLogs()
        logFiles = monitorVm ? monitorVm.diagnosticLogFiles() : []
    }

    // Clipboard helper (hidden TextInput selection copy, same idiom as
    // SysInfoDialog.copyToClipboard).
    function copyToClipboard(text) {
        var temp = Qt.createQmlObject(
            'import QtQuick; TextInput { visible: false }', root, "clipboardHelper")
        temp.text = text
        temp.selectAll()
        temp.copy()
        temp.destroy()
    }

    // GetSysInfoAll 8-line dump (TroubleshootDialog.cpp:371-383).
    function sysInfoAllText() {
        return "Version   :  " + appVersion + "\n"
             + "Build     :  " + buildCommit + "\n"
             + "Package   :  " + (sysDiag.packageType || "") + "\n"
             + "Platform  :  " + (sysDiag.osInfo || "") + "\n"
             + "Processor :  " + (sysDiag.cpuInfo || "") + "\n"
             + "Memory    :  " + ramLine + "\n"
             + "Renderer  :  " + gpuLine + "\n"
             + "Monitors  :  " + monitorsLine
    }

    // ── Inline components ─────────────────────────────────────────────────

    // Upstream create_title (StaticLine + Head_16, :194-199): bold section
    // text with a rule filling the rest of the row.
    component SectionTitle: RowLayout {
        id: sectionTitleRoot
        property string title: ""
        spacing: Theme.spacingXS
        Text {
            text: sectionTitleRoot.title
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeXL
            font.bold: true
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.borderSubtle
        }
    }

    // Upstream create_label (:201-211): 275 DIP label (source comment
    // "400 - 120 - 5") + 15 gap + the control.
    component LabelRow: RowLayout {
        id: labelRowRoot
        default property alias rowContent: rowHost.data
        property string rowLabel: ""
        property string rowTip: ""
        spacing: Theme.spacingXL
        Text {
            Layout.preferredWidth: 275
            Layout.alignment: Qt.AlignVCenter
            text: labelRowRoot.rowLabel
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeMD
            wrapMode: Text.WordWrap
            HoverHandler { id: labelHover }
            ToolTip.visible: labelHover.hovered && labelRowRoot.rowTip !== ""
            ToolTip.text: labelRowRoot.rowTip
            ToolTip.delay: 400
        }
        RowLayout {
            id: rowHost
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 8
        }
    }

    // Upstream HyperLink (:190/:237): accent text, underlined on hover.
    component HyperLinkText: Text {
        id: linkRoot
        property string linkUrl: ""
        color: Theme.accent
        font.pixelSize: Theme.fontSizeMD
        font.underline: linkHover.hovered
        HoverHandler { id: linkHover; cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: Qt.openUrlExternally(linkRoot.linkUrl) }
    }

    // Grid cell texts (wxFlexGridSizer(1, 6, FromDIP(3), FromDIP(15)),
    // :63): header Body_12, values Body_14.
    component GridText: Text {
        property string cell: ""
        property int cellWidth: 60
        Layout.preferredWidth: cellWidth
        horizontalAlignment: Text.AlignHCenter
        text: cell
        color: Theme.textPrimary
        font.pixelSize: Theme.fontSizeMD
        elide: Text.ElideRight
    }

    contentItem: Rectangle {
        color: Theme.bgPanel
        anchors.fill: parent

        RowLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingXL
            spacing: Theme.spacingXL

            // ── LEFT SIZER (TroubleshootDialog.cpp:314-329) ──────────────
            ColumnLayout {
                Layout.preferredWidth: 420
                Layout.fillHeight: true
                spacing: 0

                // Header logo (:131/:321, 64px height, centered).
                Image {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredHeight: 64
                    Layout.preferredWidth: 171
                    fillMode: Image.PreserveAspectFit
                    source: "qrc:/qml/assets/icons/owzx_horizontal_logo.svg"
                }

                // logo_line: 2px full-width brand underline (:133-134/:322).
                Rectangle {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    Layout.preferredHeight: 2
                    color: Theme.accent
                }

                // Version, font x1.65 (≈18pt) (:135-139/:323).
                Text {
                    Layout.topMargin: 6
                    Layout.alignment: Qt.AlignHCenter
                    text: root.appVersion
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeXXL
                }

                // build_commit_label button (:141-147/:324).
                CxButton {
                    visible: root.buildCommit !== ""
                    Layout.topMargin: 4
                    Layout.alignment: Qt.AlignHCenter
                    text: root.buildCommit
                    cxStyle: CxButton.Style.Secondary
                    toolTipText: root.buildCommitUrl
                    onClicked: Qt.openUrlExternally(root.buildCommitUrl)
                }

                // System info panel (CenteredMultiLinePanel :150-165/:325),
                // centered word-wrapped lines.
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 15
                    spacing: Theme.spacingMD
                    Repeater {
                        model: root.sysPanelLines
                        Text {
                            required property var modelData
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            text: modelData
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                            wrapMode: Text.WordWrap
                            lineHeight: 1.15
                        }
                    }
                }

                // AddStretchSpacer (:326).
                Item { Layout.fillHeight: true }

                // sys_btn_sizer: Hide ... Copy (:316-319/:327).
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 15
                    CxButton {
                        Layout.leftMargin: 5
                        text: root.sysPanelMode ? qsTr("隐藏") : qsTr("显示")
                        toolTipText: qsTr("显示/隐藏系统信息")
                        onClicked: root.sysPanelMode = !root.sysPanelMode
                    }
                    Item { Layout.fillWidth: true }
                    CxButton {
                        Layout.rightMargin: 5
                        text: qsTr("复制")
                        toolTipText: qsTr("复制系统信息到剪贴板")
                        onClicked: root.copyToClipboard(root.sysInfoAllText())
                    }
                }

                // Wiki Guide link (:190/:328).
                HyperLinkText {
                    Layout.topMargin: 15
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Wiki 指南")
                    linkUrl: "https://www.orcaslicer.com/wiki/troubleshoot_center"
                }

                // AddSpacer(FromDIP(5)) (:329).
                Item { Layout.preferredHeight: 5 }
            }

            // ── RIGHT SIZER (TroubleshootDialog.cpp:331-351) ──────────────
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // INFORMATION (:338-340).
                SectionTitle {
                    Layout.fillWidth: true
                    title: qsTr("信息")
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 5
                    spacing: 8
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("我们需要相关信息来诊断问题根源。详细指南请查看 Wiki 页面。")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMD
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("“打包”按钮会将项目文件与当前会话日志收集到一个 ZIP 压缩包中。")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMD
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("报告问题时，图片或屏幕录制等额外的可视化示例可能会有所帮助。")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMD
                        wrapMode: Text.WordWrap
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    HyperLinkText {
                        text: qsTr("报告问题")
                        linkUrl: root.reportIssueUrl()
                    }
                    Item { Layout.fillWidth: true }
                    CxButton {
                        text: qsTr("打包...")
                        cxStyle: CxButton.Style.Secondary
                        onClicked: root.startPackAll()
                    }
                }

                // PROFILES (:342-345).
                SectionTitle {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    title: qsTr("预设")
                }
                LabelRow {
                    Layout.topMargin: 8
                    rowLabel: qsTr("清理系统预设缓存")
                    rowTip: qsTr("下次启动时清理并重建系统预设缓存。")
                    CxButton {
                        text: qsTr("清理")
                        cxStyle: CxButton.Style.Secondary
                        toolTipText: qsTr("下次启动时清理并重建系统预设缓存。")
                        onClicked: cleanConfirmDialog.open()
                    }
                }
                LabelRow {
                    Layout.topMargin: 5
                    rowLabel: qsTr("已加载预设概览")
                    rowTip: qsTr("此区域显示已加载预设的信息。")
                    CxButton {
                        text: qsTr("导出...")
                        cxStyle: CxButton.Style.Secondary
                        toolTipText: qsTr("以 JSON 格式导出已加载预设的详细概览。")
                        enabled: root.profileCounts !== null
                        onClicked: exportJsonDialog.open()
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 5
                    spacing: Theme.spacingXS
                    RowLayout {
                        spacing: Theme.spacingXL
                        Item { Layout.preferredWidth: 100 }
                        GridText { cell: qsTr("使用中"); cellWidth: 60; font.pixelSize: Theme.fontSizeSM }
                        Item { Layout.preferredWidth: 20 }
                        GridText { cell: qsTr("系统"); cellWidth: 60; font.pixelSize: Theme.fontSizeSM }
                        Item { Layout.preferredWidth: 20 }
                        GridText { cell: qsTr("用户"); cellWidth: 60; font.pixelSize: Theme.fontSizeSM }
                    }
                    Repeater {
                        model: [
                            { label: qsTr("打印机"), key: "printers" },
                            { label: qsTr("耗材"), key: "filaments" },
                            { label: qsTr("工艺"), key: "processes" }
                        ]
                        delegate: RowLayout {
                            id: countRow
                            required property var modelData
                            spacing: Theme.spacingXL
                            readonly property var counts: (root.profileCounts && root.profileCounts[countRow.modelData.key])
                                                          ? root.profileCounts[countRow.modelData.key]
                                                          : { active: 0, system: 0, user: 0 }
                            Text {
                                Layout.preferredWidth: 100
                                text: countRow.modelData.label
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeMD
                                elide: Text.ElideRight
                            }
                            GridText { cell: countRow.counts.active }
                            GridText { cell: "/"; cellWidth: 20; font.pixelSize: Theme.fontSizeSM }
                            GridText { cell: countRow.counts.system }
                            GridText { cell: "+"; cellWidth: 20; font.pixelSize: Theme.fontSizeSM }
                            GridText { cell: countRow.counts.user }
                        }
                    }
                }

                // MORE (:347-351).
                SectionTitle {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    title: qsTr("更多")
                }
                LabelRow {
                    Layout.topMargin: 8
                    rowLabel: qsTr("配置目录")
                    CxButton {
                        text: qsTr("浏览...")
                        cxStyle: CxButton.Style.Secondary
                        toolTipText: qsTr("打开配置目录。")
                        onClicked: {
                            if (root.monitorVm)
                                root.monitorVm.diagnosticOpenFolder(root.monitorVm.appDataDir())
                        }
                    }
                }
                LabelRow {
                    Layout.topMargin: 5
                    rowLabel: qsTr("日志级别")
                    CxComboBox {
                        id: logLevelCombo
                        Layout.preferredWidth: 120
                        model: root.logLevelsDisplay
                        // Upstream persists app_config "log_severity_level"
                        // immediately on selection (:104-111); the OWzx Qt
                        // logger consumes the persisted value on next launch.
                        onActivated: function(index) {
                            root.logSeverity = root.logLevels[index]
                            if (root.monitorVm)
                                root.monitorVm.setLogSeverityLevel(root.logSeverity)
                        }
                    }
                }
                LabelRow {
                    Layout.topMargin: 5
                    rowLabel: qsTr("存储的日志")
                    CxButton {
                        text: qsTr("打包...")
                        cxStyle: CxButton.Style.Secondary
                        toolTipText: qsTr("将所有存储的日志打包为 ZIP 压缩包。")
                        onClicked: root.startLogsPack()
                    }
                }
                LabelRow {
                    Layout.topMargin: 5
                    rowLabel: root.logsStorageText
                    CxButton {
                        text: qsTr("清除")
                        cxStyle: CxButton.Style.Secondary
                        onClicked: root.clearLogs()
                    }
                }
            }
        }
    }

    // ── Helper dialogs ─────────────────────────────────────────────────────

    // Dirty-project prompt before Pack (upstream MessageDialog
    // wxYES_NO|wxCANCEL, TroubleshootDialog.cpp:926-940; “不保存” aborts the
    // pack, upstream closes the dialog for NO/CANCEL).
    CxDialog {
        id: saveDirtyDialog
        modal: true
        dialogTitle: qsTr("保存")
        width: 420
        height: 200
        padding: 0

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingXL
            spacing: Theme.spacingLG

            Text {
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: qsTr("当前项目有未保存的改动。是否在继续之前保存？")
                      + "\n\n" + qsTr("选择“不保存”将关闭对话框并返回检查项目。")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                wrapMode: Text.WordWrap
                verticalAlignment: Text.AlignVCenter
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight
                spacing: Theme.spacingMD

                CxButton {
                    text: qsTr("不保存")
                    cxStyle: CxButton.Style.Secondary
                    onClicked: saveDirtyDialog.reject()
                }
                CxButton {
                    text: qsTr("保存")
                    cxStyle: CxButton.Style.Primary
                    onClicked: {
                        var saved = (typeof backend !== "undefined" && backend)
                            ? backend.topbarSaveProject() : false
                        saveDirtyDialog.accept()
                        if (!saved) {
                            root.notify(qsTr("项目保存失败，已取消打包。"), qsTr("打包"), 2)
                            return
                        }
                        var proj = (typeof backend !== "undefined" && backend && backend.projectViewModel)
                            ? backend.projectViewModel : null
                        root._packPaths = root.collectPackPaths(proj ? (proj.currentProjectPath || "") : "")
                        root._packBaseName = "OWzx_PackedDebugInfo_" + root.exportTimestamp()
                        packDestDialog.open()
                    }
                }
            }
        }
    }

    // "Restart Required" confirm before the system cache clean
    // (TroubleshootDialog.cpp:971-976). The cache clean itself reports
    // honestly via a notification (no app restart: OWzx system presets are
    // read-only resources, nothing needs reloading).
    CxDialog {
        id: cleanConfirmDialog
        modal: true
        dialogTitle: qsTr("需要重启")
        width: 420
        height: 210
        padding: 0

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingXL
            spacing: Theme.spacingLG

            Text {
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: qsTr("请确保没有其他 OWzx 实例正在运行") + "\n" + qsTr("是否继续？")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                wrapMode: Text.WordWrap
                verticalAlignment: Text.AlignVCenter
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight
                spacing: Theme.spacingMD

                CxButton {
                    text: qsTr("取消")
                    cxStyle: CxButton.Style.Secondary
                    onClicked: cleanConfirmDialog.reject()
                }
                CxButton {
                    text: qsTr("确定")
                    cxStyle: CxButton.Style.Primary
                    onClicked: {
                        cleanConfirmDialog.accept()
                        var res = root.monitorVm ? root.monitorVm.diagnosticCleanSystemProfilesCache()
                                                 : { existed: false, removed: false }
                        if (!res.existed)
                            root.notify(qsTr("系统预设缓存目录不存在（OWzx 系统预设为只读内置资源），无需清理。"),
                                        qsTr("清理系统预设缓存"), 1)
                        else if (res.removed)
                            root.notify(qsTr("系统预设缓存已删除，重启应用后生效。"),
                                        qsTr("清理系统预设缓存"), 0)
                        else
                            root.notify(qsTr("系统预设缓存删除失败，请检查文件是否被占用。"),
                                        qsTr("清理系统预设缓存"), 2)
                    }
                }
            }
        }
    }

    // Destination picker for Pack (upstream wxDirDialog in ExportAsZip,
    // TroubleshootDialog.cpp:1244-1249).
    FolderDialog {
        id: packDestDialog
        title: qsTr("选择 ZIP 导出位置")
        onAccepted: {
            var dest = selectedFolder.toString().replace(/^file:\/\/\//, "")
            if (dest !== "")
                root.packTo(dest)
        }
    }

    FolderDialog {
        id: logsDestDialog
        title: qsTr("选择 ZIP 导出位置")
        onAccepted: {
            var dest = selectedFolder.toString().replace(/^file:\/\/\//, "")
            if (dest !== "")
                root.packTo(dest)
        }
    }

    // Profiles overview JSON export (upstream wxFileDialog in ExportAsJson,
    // TroubleshootDialog.cpp:1201-1233).
    FileDialog {
        id: exportJsonDialog
        title: qsTr("选择 JSON 导出位置")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("JSON 文件 (*.json)")]
        onAccepted: {
            var path = selectedFile.toString().replace(/^file:\/\/\//, "")
            var ok = path !== "" && root.monitorVm
                     && root.monitorVm.writeTextFile(path, root.buildOverviewJson())
            root.notify(ok ? qsTr("导出成功")
                           : qsTr("导出失败\n请检查写入权限或文件是否被占用"),
                        qsTr("导出"), ok ? 0 : 2)
        }
    }
}
