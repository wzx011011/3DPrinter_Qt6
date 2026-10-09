pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P10.1 -- PluginManagerDialog (Phase 202: real PluginService backend).
//
// Aligns 1:1 with the upstream OrcaSlicer PluginsDialog (NOT
// WebDownPluginDlg -- an earlier comment here pointed at the wrong
// upstream; WebDownPluginDlg is the 410x200 guided-web frame, while the
// plugin manager is PluginsDialog):
//   - top-level resizable window 900x820, min 760x715
//     (PluginsDialog.cpp:452 create_webview wxSize(900,820)/wxSize(760,715)),
//     opened non-modal via Show()+Raise (GUI_App.cpp:8294-8295);
//   - toolbar: 300x26 search box (magnifier + placeholder + 10x10 round
//     clear + Aa/ab 22x22 match-case/whole-word toggles,
//     plugin-search.css) and the right cluster gap 8: 26px Refresh +
//     split install button (22px arrow segment + 140px main segment, 32px
//     high, dropdown with "Install plugin" / "Install local plugin",
//     styles.css:29-98);
//   - content: list pane (62% default, min 180px) + 9px drag splitter
//     (2px divider, double-click resets, index.js:37-39) + details pane
//     (min 160px) with 5 equal tabs: Plugin Info / Description / Config /
//     Changelog / Diagnostics (index.html:69-204);
//   - list: 5-column sortable table 70px | 2.4fr | 0.85fr | 0.7fr | 0.95fr
//     (min 110/96/130px, styles.css:233-236), rows min-height 34px with
//     1px separators, hover/selected states, 14px activate checkbox with
//     indeterminate mixed-capability state and expandable capability
//     sub-rows (index.js:485-496);
//   - version cell: plain text + 18x18 update-available badge
//     (styles.css:466-492), source cell with the Local/Mine/Subscribed/
//     Orphaned colored variants (index.js:707-719);
//   - bottom status bar: 8px status dot + message, success/error/warn/info
//     levels (index.html:207-210, styles.css:711-771); operation results
//     arrive through PluginService::statusMessage;
//   - row context menu (min-width 190px, radius 6, danger items,
//     PluginsDialog.cpp:302-314 action policy).
//
// Colors stay on the OWzx tokens: Theme.accent brand green replaces the
// upstream teal, surfaces use the current neutral gray tokens. The data
// source is PluginService (C++) with QSettings persistence under
// plugins/*. installPlugin is a mock state flip (no real HTTP download /
// no Python) -- see PLAN.md.
//
// Usage: PluginManagerDialog { id: dlg; pluginService: backend.pluginService }
//   -> dlg.open()

CxDialog {
    id: root

    closePolicy: Popup.NoAutoClose
    // Upstream opens the plugin manager as a NON-modal top-level window
    // (m_plugins_dlg->Show(); Raise(); GUI_App.cpp:8294-8295) -- the app
    // stays usable while it is open.
    modal: false

    dialogTitle: qsTr("插件管理")

    // Phase 202: backend binding. Caller (main.qml) sets
    // pluginService: backend.pluginService. Falls back to a null-safe render
    // when unset so the dialog stays usable in isolation.
    property var pluginService: null

    // ── Window geometry (upstream PluginsDialog.cpp:452 / hpp:46) ──────
    // wxSize(900, 820) default, wxSize(760, 715) min. QML Popup has no
    // native resize affordance, so edge strips + corner grip reproduce the
    // wxRESIZE_BORDER | wxMAXIMIZE_BOX frame (EditGCodeDialog precedent).
    readonly property int minDialogWidth: 760
    readonly property int minDialogHeight: 715
    anchors.centerIn: undefined
    width: 900
    height: 820

    // ── Live registry view ─────────────────────────────────────────────
    // Re-evaluated on every pluginService stateChanged (the single batch
    // NOTIFY signal).
    readonly property var _registry: pluginService ? pluginService.plugins : []

    // ── Table state ────────────────────────────────────────────────────
    property var _view: []                    // filtered + sorted rows (each tagged with registry idx)
    property string _selectedName: ""         // upstream selectedPluginId (plugin name is the mock key)
    property string _sortKey: "none"          // none|name|version|source|status (plugin-sort.js)
    property string _sortOrder: "asc"
    property string _search: ""
    property bool _matchCase: false           // "Aa" toggle
    property bool _wholeWord: false           // "ab" toggle
    property var _expanded: ({})              // plugin name -> expanded (base state)
    property var _searchExpand: ({})          // transient per-search expand override
    property var _colW: []                    // computed 5-column pixel widths
    readonly property int colGap: 10

    // ── Detail pane state ──────────────────────────────────────────────
    property string _detailTab: "plugin-info"
    property real _splitRatio: 0.62           // index.js:36 SPLIT_DEFAULT_RATIO

    // ── Config tab state (JSON editor over one capability) ─────────────
    property string _cfgCapName: ""
    property string _cfgText: ""
    property bool _cfgValid: true
    property string _cfgValidation: ""        // JSON parse message
    property string _cfgError: ""             // service-side error strip

    // ── Install dropdown + context menu + status bar ───────────────────
    property int _installAction: 0            // 0 = "Install plugin", 1 = "Install local plugin"
    property var _ctxActions: []
    property int _ctxIdx: -1
    property string _statusText: ""
    property string _statusLevel: "info"      // success|error|warn|info

    // The selected registry row (upstream pluginsById.get(selectedPluginId)).
    readonly property var _selected: {
        for (let i = 0; i < _registry.length; ++i) {
            if (_registry[i].name === _selectedName)
                return _registry[i]
        }
        return null
    }

    // Configurable capabilities of the selected plugin (upstream
    // GetConfigurableCapabilities / config_capabilities rows).
    readonly property var _cfgCaps: {
        const caps = _selected ? (_selected.capabilities || []) : []
        return caps.filter(c => c.hasConfig)
    }
    readonly property var _cfgCap: {
        for (const c of _cfgCaps) {
            if (c.name === _cfgCapName)
                return c
        }
        return null
    }

    readonly property var _installActionLabels: [qsTr("安装插件"), qsTr("安装本地插件")]

    on_SearchChanged: {
        // Leaving the search resets the transient expand override (the base
        // expand state is never written while searching).
        if (_search.length === 0)
            _searchExpand = {}
        rebuildView()
    }
    on_MatchCaseChanged: rebuildView()
    on_WholeWordChanged: rebuildView()
    on_SortKeyChanged: rebuildView()
    on_SortOrderChanged: rebuildView()
    on_RegistryChanged: rebuildView()

    Connections {
        target: root.pluginService
        function onStatusMessage(message, level) {
            root._statusText = message
            root._statusLevel = level
        }
    }

    // ── View building (filter + sort) ──────────────────────────────────

    function searchActive() {
        return _search.length > 0
    }

    function _escapeRegExp(text) {
        return String(text).replace(/[.*+?^${}()|[\]\\]/g, "\\$&")
    }

    // Case / whole-word aware substring test (plugin-search.js semantics,
    // ASCII + CJK word bounds).
    function textMatches(hay) {
        const needle = _matchCase ? _search : _search.toLowerCase()
        const stack = _matchCase ? hay : String(hay).toLowerCase()
        if (!needle)
            return true
        if (_wholeWord) {
            if (stack === needle)
                return true
            const re = new RegExp(
                        "(?:^|[^a-z0-9\\u4e00-\\u9fff])" + _escapeRegExp(needle)
                        + "(?:$|[^a-z0-9\\u4e00-\\u9fff])")
            return re.test(stack)
        }
        return stack.indexOf(needle) !== -1
    }

    function matchInfo(row) {
        if (!searchActive())
            return { matched: true, capMatch: false }
        const nameMatch = textMatches(row.name)
        let capMatch = false
        const caps = row.capabilities || []
        for (const c of caps) {
            if (textMatches(c.name)) {
                capMatch = true
                break
            }
        }
        return { matched: nameMatch || capMatch, capMatch: capMatch }
    }

    // Display version (index.js GetDisplayVersion): installed version wins,
    // latest otherwise, N/A when the catalog carries nothing.
    function displayVersion(row) {
        if (row.isInstalled && row.installedVersion)
            return row.installedVersion
        return row.latestVersion || qsTr("N/A")
    }

    function _compareRows(a, b) {
        let r = 0
        if (_sortKey === "name")
            r = String(a.name).localeCompare(String(b.name))
        else if (_sortKey === "version")
            r = String(displayVersion(a)).localeCompare(String(displayVersion(b)))
        else if (_sortKey === "source")
            r = sourceLabel(a.source).localeCompare(sourceLabel(b.source))
        else if (_sortKey === "status")
            r = String(a.status).localeCompare(String(b.status))
        return _sortOrder === "desc" ? -r : r
    }

    function rebuildView() {
        const rows = []
        for (let i = 0; i < _registry.length; ++i) {
            const row = _registry[i]
            const info = matchInfo(row)
            if (!info.matched)
                continue
            // Copy the row and tag it with the registry index: every
            // PluginService Q_INVOKABLE addresses plugins by registry index,
            // not by the sorted/filtered view position.
            const copy = Object.assign({}, row)
            copy.idx = i
            copy.capMatch = info.capMatch
            rows.push(copy)
        }
        if (_sortKey !== "none")
            rows.sort(_compareRows)
        _view = rows
        // Default selection to the first visible row (index.js ApplyPlugins).
        if (!_selected && rows.length > 0)
            _selectedName = rows[0].name
    }

    function cycleSort(key) {
        // one click per column cycles asc -> desc -> clear (plugin-sort.js)
        if (_sortKey !== key) {
            _sortKey = key
            _sortOrder = "asc"
        } else if (_sortOrder === "asc") {
            _sortOrder = "desc"
        } else {
            _sortKey = "none"
            _sortOrder = "asc"
        }
    }

    // ── 5-column grid geometry (styles.css:233-236) ────────────────────
    // 70px | minmax(0,2.4fr) | minmax(110px,0.85fr) | minmax(96px,0.7fr)
    //      | minmax(130px,0.95fr), gap 10px, row padding 0 10px.
    function columnWidths(availWidth) {
        const flex = Math.max(0, availWidth - 20 - 4 * colGap - 70)
        const weights = [2.4, 0.85, 0.7, 0.95]
        const mins = [0, 110, 96, 130]
        const widths = [70]
        for (let i = 0; i < 4; ++i)
            widths.push(Math.max(mins[i], Math.floor(flex * weights[i] / 4.9)))
        return widths
    }

    function colX(widths, col) {
        let x = 0
        for (let i = 0; i < col && i < widths.length; ++i)
            x += widths[i] + colGap
        return x
    }

    // ── Row helpers ────────────────────────────────────────────────────

    function sourceLabel(source) {
        switch (String(source)) {
        case "mine": return qsTr("我的")
        case "subscribed": return qsTr("已订阅")
        case "orphaned": return qsTr("已失效")
        default: return qsTr("本地")
        }
    }

    function sourceColor(source) {
        switch (String(source)) {
        case "mine": return Theme.accent
        case "subscribed": return Theme.statusInfo
        case "orphaned": return Theme.statusWarning
        default: return Theme.textTertiary
        }
    }

    function sourceBold(source) {
        const s = String(source)
        return s === "mine" || s === "subscribed" || s === "orphaned"
    }

    function statusColor(statusKey) {
        switch (String(statusKey)) {
        case "activated": return Theme.statusSuccess
        case "loading": return Theme.statusWarning
        case "error": return Theme.statusError
        default: return Theme.textTertiary
        }
    }

    function statusBold(statusKey) {
        const s = String(statusKey)
        return s === "activated" || s === "loading" || s === "error"
    }

    function statusLevelColor(level) {
        switch (String(level)) {
        case "success": return Theme.statusSuccess
        case "error": return Theme.statusError
        case "warn": return Theme.statusWarning
        default: return Theme.textMuted
        }
    }

    // Mixed capability state -> indeterminate checkbox (index.js:624-650).
    function rowMixed(row) {
        if (!row.isInstalled || !row.isEnabled)
            return false
        const caps = (row.capabilities || []).filter(
                    c => c.canToggle && c.name && c.typeKey)
        return caps.length > 0 && caps.some(c => !c.enabled)
    }

    function rowExpanded(row) {
        const caps = row.capabilities || []
        if (!caps.length)
            return false
        if (searchActive()) {
            // transient override wins; else auto-expand capability matches
            if (_searchExpand.hasOwnProperty(row.name))
                return _searchExpand[row.name]
            return row.capMatch === true
        }
        return !!_expanded[row.name]
    }

    function toggleExpanded(row) {
        const next = !rowExpanded(row)
        const base = searchActive() ? _searchExpand : _expanded
        const copy = {}
        for (const k in base)
            copy[k] = base[k]
        if (next)
            copy[row.name] = true
        else
            delete copy[row.name]
        if (searchActive())
            _searchExpand = copy
        else
            _expanded = copy
    }

    // Row activate checkbox: a not-installed plugin installs on activate
    // (upstream toggle_plugin -> install for packages without a local
    // copy), an installed one enables/disables.
    function toggleRowActivation(row) {
        if (!pluginService)
            return
        if (!row.isInstalled)
            pluginService.installPlugin(row.idx)
        else
            pluginService.setPluginEnabled(row.idx, !row.isEnabled)
    }

    // Context-menu action policy (PluginsDialog.cpp:295-316
    // evaluate_action_policy). Upstream registers the six labels as raw
    // English literals without _L(), so every locale renders them
    // untranslated (PluginsDialog.cpp:302/304/307/311/313/314, verbatim in
    // the web frontend index.js:1545 button.textContent). The labels here
    // wrap the same upstream source strings.
    function contextActionsFor(row) {
        const acts = []
        const cloud = row.source === "mine" || row.source === "subscribed"
                || row.source === "orphaned"
        if (row.source === "subscribed") {
            acts.push({ id: "unsubscribe_plugin", label: qsTr("Unsubscribe"),
                        danger: true, enabled: true })
        } else if (row.isInstalled) {
            acts.push({ id: "delete_plugin", label: qsTr("Delete"),
                        danger: true, enabled: true })
        }
        acts.push({ id: "open_folder", label: qsTr("Show in folder"),
                    danger: false, enabled: row.isInstalled })
        if (row.source !== "orphaned") {
            if (cloud) {
                acts.push({ id: "reinstall_plugin", label: qsTr("Reinstall"),
                            danger: false, enabled: true })
            } else {
                acts.push({ id: "reload_plugin", label: qsTr("Reload"),
                            danger: false, enabled: true })
                acts.push({ id: "clear_cache_reload_plugin",
                            label: qsTr("Delete cache and reload"),
                            danger: false, enabled: true })
            }
        }
        return acts
    }

    function openContextMenu(row, fromItem, mx, my) {
        _selectedName = row.name
        const acts = contextActionsFor(row)
        if (acts.length === 0)
            return
        _ctxActions = acts
        _ctxIdx = row.idx
        const p = fromItem.mapToItem(root.contentItem, mx, my)
        ctxMenu.x = Math.max(4, p.x)
        ctxMenu.y = Math.max(4, p.y)
        ctxMenu.open()
    }

    function runContextAction(actionId) {
        ctxMenu.close()
        if (!pluginService || _ctxIdx < 0)
            return
        // Destructive actions gate on a YES/NO confirm first (upstream
        // delete_local_plugin / unsubscribe_cloud_plugin wxMessageBox
        // wxYES_NO|wxNO_DEFAULT|wxICON_WARNING, PluginsDialog.cpp:1109-1118
        // and 1146-1152); every other action runs straight through
        // plugin_menu_action.
        if (actionId === "delete_plugin" || actionId === "unsubscribe_plugin") {
            openPluginActionConfirm(actionId)
            return
        }
        pluginService.runPluginAction(_ctxIdx, actionId)
    }

    // Delete / unsubscribe confirmation texts (upstream wxMessageBox
    // message formats, PluginsDialog.cpp:1109-1115 and 1146-1152; owned
    // cloud rows delete local files only and stay reinstallable in the
    // cloud, PluginsDialog.cpp:1110-1112).
    function openPluginActionConfirm(actionId) {
        const row = pluginService.pluginAt(_ctxIdx)
        const name = String(row.name || "")
        if (actionId === "unsubscribe_plugin") {
            pluginActionConfirm.dialogTitle = qsTr("Unsubscribe")
            pluginActionConfirm.message =
                qsTr("Unsubscribe plugin \"%1\"?\n\nThis will stop tracking the plugin and delete any local plugin files.").arg(name)
        } else {
            pluginActionConfirm.dialogTitle = qsTr("Delete Plugin")
            if (row.source === "mine" || row.source === "subscribed"
                    || row.source === "orphaned")
                pluginActionConfirm.message =
                    qsTr("Delete plugin \"%1\"?\n\nThis removes the local plugin files. The plugin stays in the cloud and can be reinstalled.").arg(name)
            else
                pluginActionConfirm.message =
                    qsTr("Delete plugin \"%1\"?\n\nThis permanently removes the plugin folder.").arg(name)
        }
        pluginActionConfirm.confirmText = qsTr("Yes")
        pluginActionConfirm.cancelText = qsTr("No")
        pluginActionConfirm.openWithAction(function() {
            pluginService.runPluginAction(_ctxIdx, actionId)
        })
    }

    // ── Splitter (index.js InitPaneSplitter) ───────────────────────────

    function applySplitRatio(ratio) {
        const available = contentPane.avail
        if (available <= 0)
            return
        const sash = 9
        let next = isFinite(ratio) ? ratio : 0.62
        const min = 180 / available          // SPLIT_MIN_LIST_PX
        const max = (available - sash - 160) / available  // SPLIT_MIN_DETAILS_PX
        next = min > max ? 0.5 : Math.min(Math.max(next, min), max)
        _splitRatio = next
    }

    // ── Config tab helpers ─────────────────────────────────────────────

    function resetConfigSelection() {
        const caps = _cfgCaps
        _cfgCapName = caps.length > 0 ? String(caps[0].name) : ""
        loadConfigText()
    }

    function loadConfigText() {
        if (!pluginService || !_selected || !_cfgCapName) {
            _cfgText = ""
            _cfgError = ""
            _cfgValidation = ""
            _cfgValid = true
            configEditor.text = ""
            return
        }
        const res = pluginService.capabilityConfig(_selected.idx, _cfgCapName)
        _cfgText = String(res.config || "")
        _cfgError = String(res.error || "")
        _cfgValidation = ""
        _cfgValid = true
        configEditor.text = _cfgText
    }

    function validateCfg() {
        try {
            JSON.parse(_cfgText)
            _cfgValidation = ""
            _cfgValid = true
        } catch (e) {
            _cfgValidation = String(e.message || e)
            _cfgValid = false
        }
    }

    function saveCapabilityConfig() {
        if (!pluginService || !_selected || !_cfgCap || !_cfgValid)
            return
        const res = pluginService.saveCapabilityConfig(
                    _selected.idx, _cfgCapName, _cfgText)
        _cfgError = res.ok === true ? "" : String(res.error || qsTr("保存失败"))
    }

    function restoreCapabilityConfig() {
        if (!pluginService || !_selected || !_cfgCap)
            return
        const res = pluginService.restoreCapabilityConfig(_selected.idx, _cfgCapName)
        if (res.ok === true) {
            _cfgText = String(res.config || "")
            configEditor.text = _cfgText
            _cfgError = ""
            _cfgValidation = ""
            _cfgValid = true
        } else {
            _cfgError = String(res.error || qsTr("恢复失败"))
        }
    }

    function runCapability(row, cap) {
        if (!pluginService)
            return
        pluginService.runScriptCapability(row.idx, cap.name)
    }

    function emptyConfigMessage() {
        if (!_selected)
            return qsTr("选择一个插件以配置其能力")
        if (_selected.statusKey === "activated")
            return qsTr("该插件未提供可配置能力")
        return qsTr("激活该插件以配置其能力")
    }

    function statusDescription(statusKey) {
        switch (String(statusKey)) {
        case "activated": return qsTr("该插件已激活并可用。")
        case "loading": return qsTr("该插件仍在加载中。")
        case "error": return qsTr("该插件在错误解决前处于停用状态。")
        default: return qsTr("该插件未激活。激活以安装或加载它。")
        }
    }

    // Index of the active detail tab within the fixed 5-tab order.
    function _detailTabIndex() {
        const tabs = ["plugin-info", "description", "config",
                      "changelog", "diagnostics"]
        const i = tabs.indexOf(_detailTab)
        return i >= 0 ? i : 0
    }

    on_SelectedNameChanged: resetConfigSelection()

    // Upstream centers the dialog on its parent; a resize drag keeps the
    // top-left corner fixed, so centering is manual (EditGCodeDialog
    // precedent).
    function centerToOverlay() {
        const ov = Overlay.overlay
        if (!ov)
            return
        x = Math.max(0, Math.round((ov.width - width) / 2))
        y = Math.max(0, Math.round((ov.height - height) / 2))
    }

    onAboutToShow: {
        centerToOverlay()
        applySplitRatio(_splitRatio)
        _colW = columnWidths(listPane.width - 2)
    }

    // ── Shared cell frame: positions one column of the 5-column grid ───
    component CellFrame: Item {
        id: cellFrame
        property int col: 0
        x: root.colX(root._colW, col)
        width: col < root._colW.length ? root._colW[col] : 0
        default property alias content: inner.data
        Item {
            id: inner
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 8
            clip: true
        }
        // column separator (upstream .row > * border-right 1px --col-sep)
        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: 1
            visible: cellFrame.col < 4
            color: Theme.borderSubtle
        }
    }

    // 14px activate checkbox with 12x12 mark; mixed renders the bar
    // (styles.css:945-1012, teal rebranded to Theme.accent).
    component PluginCheck: Item {
        id: checkRoot
        property bool checked: false
        property bool mixed: false
        width: 14
        height: 14
        Rectangle {
            anchors.centerIn: parent
            width: 12
            height: 12
            radius: Theme.radiusXS
            color: checkRoot.checked || checkRoot.mixed ? Theme.accent : Theme.bgBase
            border.width: 1
            border.color: Theme.borderDefault
            opacity: checkRoot.enabled ? 1.0 : 0.6
        }
        Canvas {
            id: checkMark
            anchors.centerIn: parent
            width: 12
            height: 12
            antialiasing: true
            visible: checkRoot.checked || checkRoot.mixed
            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                if (!checkRoot.checked && !checkRoot.mixed)
                    return
                ctx.strokeStyle = Theme.textOnAccent
                ctx.lineWidth = 1.5
                ctx.lineCap = "round"
                ctx.lineJoin = "round"
                ctx.beginPath()
                if (checkRoot.mixed) {
                    // indeterminate: 6x1.5 bar (styles.css:1001-1010)
                    ctx.moveTo(3, 6)
                    ctx.lineTo(9, 6)
                } else {
                    // check: 3x6 polyline rotated 45deg (styles.css:983-993)
                    ctx.moveTo(3.2, 6.4)
                    ctx.lineTo(5.0, 8.2)
                    ctx.lineTo(9.0, 3.8)
                }
                ctx.stroke()
            }
        }
        onCheckedChanged: checkMark.requestPaint()
        onMixedChanged: checkMark.requestPaint()
    }

    contentItem: Item {
        width: root.availableWidth
        height: root.availableHeight

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            // ── Toolbar (index.html:26-67) ─────────────────────────────
            // Search pinned left (margin-right:auto), action cluster right.
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                Layout.topMargin: 8
                spacing: 8

                // Search box: 300x26, radius 6 (plugin-search.css:1-17).
                Rectangle {
                    Layout.preferredWidth: 300
                    Layout.preferredHeight: 26
                    Layout.alignment: Qt.AlignTop
                    radius: Theme.radiusMD
                    color: Theme.bgSurface
                    border.width: 1
                    border.color: searchInput.activeFocus ? Theme.borderFocus
                                                          : Theme.borderSubtle

                    // Magnifier (inline SVG, plugin-search.css:24-27), drawn
                    // at the 16-unit viewBox scaled into 14px.
                    Canvas {
                        x: 6
                        anchors.verticalCenter: parent.verticalCenter
                        width: 14
                        height: 14
                        antialiasing: true
                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            ctx.scale(width / 16, height / 16)
                            ctx.strokeStyle = Theme.textMuted
                            ctx.lineWidth = 1.4
                            ctx.beginPath()
                            ctx.arc(6.5, 6.5, 4.5, 0, Math.PI * 2)
                            ctx.moveTo(10, 10)
                            ctx.lineTo(14, 14)
                            ctx.stroke()
                        }
                    }

                    TextInput {
                        id: searchInput
                        anchors.verticalCenter: parent.verticalCenter
                        x: 24
                        width: parent.width - 99
                        height: parent.height
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMD
                        clip: true
                        selectByMouse: true
                        verticalAlignment: TextInput.AlignVCenter
                        onTextChanged: root._search = text
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        x: 24
                        visible: searchInput.text.length === 0
                        text: qsTr("搜索插件")
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSizeMD
                    }

                    // 10x10 round clear affordance; keeps its slot while
                    // hidden so Cc/W never reflow (plugin-search.css:63-97).
                    Item {
                        x: parent.width - 71
                        anchors.verticalCenter: parent.verticalCenter
                        width: 14   // 10px disc + 4px pre-toggle margin
                        height: 22
                        opacity: root._search.length > 0 ? 1.0 : 0.0
                        Rectangle {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            width: 10
                            height: 10
                            radius: Theme.radiusMD
                            color: clearMouse.containsMouse
                                   ? Qt.rgba(127 / 255, 127 / 255, 127 / 255, 0.45)
                                   : Qt.rgba(127 / 255, 127 / 255, 127 / 255, 0.20)
                            Canvas {
                                id: clearX
                                anchors.centerIn: parent
                                width: 12
                                height: 12
                                antialiasing: true
                                // repaint on hover so the glyph brightens
                                property bool hot: clearMouse.containsMouse
                                onHotChanged: requestPaint()
                                onPaint: {
                                    const ctx = getContext("2d")
                                    ctx.clearRect(0, 0, width, height)
                                    ctx.strokeStyle = clearX.hot
                                            ? Theme.textPrimary : Theme.textMuted
                                    ctx.lineWidth = 1.6
                                    ctx.lineCap = "round"
                                    ctx.beginPath()
                                    ctx.moveTo(5, 5)
                                    ctx.lineTo(11, 11)
                                    ctx.moveTo(11, 5)
                                    ctx.lineTo(5, 11)
                                    ctx.stroke()
                                }
                            }
                            MouseArea {
                                id: clearMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                enabled: root._search.length > 0
                                cursorShape: Qt.PointingHandCursor
                                onClicked: searchInput.clear()
                            }
                        }
                    }

                    // "Aa" match-case toggle
                    // (plugin-search.css:45-61,104-108).
                    Rectangle {
                        x: parent.width - 51
                        anchors.verticalCenter: parent.verticalCenter
                        width: 22
                        height: 22
                        radius: Theme.radiusSM
                        color: root._matchCase ? Theme.accent
                              : (caseMouse.containsMouse ? Theme.bgHover : "transparent")
                        border.width: 1
                        border.color: root._matchCase ? Theme.accentDark : "transparent"
                        Text {
                            anchors.centerIn: parent
                            text: "Aa"
                            font.pixelSize: Theme.fontSizeMD
                            font.weight: Font.DemiBold
                            color: root._matchCase ? Theme.textOnAccent
                                  : (caseMouse.containsMouse ? Theme.textPrimary
                                                             : Theme.textMuted)
                        }
                        MouseArea {
                            id: caseMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root._matchCase = !root._matchCase
                        }
                    }

                    // "ab" whole-word toggle with the bracket underline drawn
                    // by the CSS ::after box (plugin-search.css:110-130).
                    Rectangle {
                        x: parent.width - 28
                        anchors.verticalCenter: parent.verticalCenter
                        width: 22
                        height: 22
                        radius: Theme.radiusSM
                        color: root._wholeWord ? Theme.accent
                              : (wordMouse.containsMouse ? Theme.bgHover : "transparent")
                        border.width: 1
                        border.color: root._wholeWord ? Theme.accentDark : "transparent"
                        Item {
                            anchors.centerIn: parent
                            width: wordLabel.implicitWidth
                            height: wordLabel.implicitHeight + 2
                            Text {
                                id: wordLabel
                                anchors.top: parent.top
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "ab"
                                font.pixelSize: Theme.fontSizeMD
                                font.weight: Font.DemiBold
                                color: root._wholeWord ? Theme.textOnAccent
                                      : (wordMouse.containsMouse ? Theme.textPrimary
                                                                 : Theme.textMuted)
                            }
                            // bottom rule + side ticks, currentColor
                            Rectangle {
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.leftMargin: -1
                                anchors.rightMargin: -1
                                height: 1
                                color: wordLabel.color
                            }
                            Rectangle {
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                anchors.leftMargin: -1
                                width: 1
                                height: 2
                                color: wordLabel.color
                            }
                            Rectangle {
                                anchors.bottom: parent.bottom
                                anchors.right: parent.right
                                anchors.rightMargin: -1
                                width: 1
                                height: 2
                                color: wordLabel.color
                            }
                        }
                        MouseArea {
                            id: wordMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root._wholeWord = !root._wholeWord
                        }
                    }
                }

                Item { Layout.fillWidth: true; Layout.preferredHeight: 1 }

                // Refresh: compact 26px toolbar button (styles.css:36-46).
                Rectangle {
                    Layout.preferredWidth: refreshLabel.implicitWidth + 20
                    Layout.preferredHeight: 26
                    Layout.alignment: Qt.AlignTop
                    radius: Theme.radiusSM
                    color: refreshMouse.pressed ? Theme.bgPressed
                          : refreshMouse.containsMouse ? Theme.bgHover
                          : Theme.bgElevated
                    border.width: 1
                    border.color: Theme.borderStrong
                    Text {
                        id: refreshLabel
                        anchors.centerIn: parent
                        text: qsTr("刷新")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSize13
                    }
                    MouseArea {
                        id: refreshMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (root.pluginService)
                                root.pluginService.refreshPluginList()
                        }
                    }
                }

                // Split install button: 22px arrow segment + 140px main
                // segment, 32px high (styles.css:48-108).
                Item {
                    id: installGroup
                    Layout.preferredWidth: 162
                    Layout.preferredHeight: 32
                    Layout.alignment: Qt.AlignTop

                    Rectangle {
                        id: arrowSegment
                        width: 22
                        height: 32
                        radius: Theme.radiusSM
                        color: arrowMouse.pressed ? Theme.accentDark
                              : arrowMouse.containsMouse ? Theme.accentLight
                              : Theme.accent
                        // square the right corners (split border-radius)
                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            width: 4
                            height: parent.height
                            color: arrowSegment.color
                        }
                        // 1px separator toward the main segment (white 25%)
                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            width: 1
                            height: parent.height
                            color: Qt.rgba(1, 1, 1, 0.25)
                        }
                        Canvas {
                            anchors.centerIn: parent
                            width: 7
                            height: 7
                            antialiasing: true
                            onPaint: {
                                const ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)
                                ctx.strokeStyle = Theme.textOnAccent
                                ctx.lineWidth = 1.5
                                ctx.lineCap = "round"
                                ctx.lineJoin = "round"
                                ctx.beginPath()
                                ctx.moveTo(1, 2.2)
                                ctx.lineTo(3.5, 4.7)
                                ctx.lineTo(6, 2.2)
                                ctx.stroke()
                            }
                        }
                        MouseArea {
                            id: arrowMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                const p = installGroup.mapToItem(
                                            root.contentItem, 22, 34)
                                exploreMenu.x = p.x
                                exploreMenu.y = p.y
                                if (exploreMenu.opened)
                                    exploreMenu.close()
                                else
                                    exploreMenu.open()
                            }
                        }
                    }

                    Rectangle {
                        x: 22
                        width: 140
                        height: 32
                        radius: Theme.radiusSM
                        color: mainMouse.pressed ? Theme.accentDark
                              : mainMouse.containsMouse ? Theme.accentLight
                              : Theme.accent
                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            width: 4
                            height: parent.height
                            color: parent.color
                        }
                        Text {
                            anchors.centerIn: parent
                            text: root._installActionLabels[root._installAction]
                            color: Theme.textOnAccent
                            font.pixelSize: Theme.fontSize13
                        }
                        MouseArea {
                            id: mainMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                // runs the selected install action (upstream
                                // explore_btn -> RunSelectedInstallAction)
                                if (root.pluginService) {
                                    if (root._installAction === 0)
                                        root.pluginService.installFromHub()
                                    else
                                        root.pluginService.installLocalPlugin()
                                }
                            }
                        }
                    }
                }
            }

            // ── Content: list pane + splitter + details pane ───────────
            Item {
                id: contentPane
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: 8
                Layout.rightMargin: 8

                // sash height (styles.css:191-197 pane-splitter 9px)
                readonly property real sash: 9
                readonly property real avail: Math.max(0, height - sash)

                onAvailChanged: root.applySplitRatio(root._splitRatio)

                // ── List pane ──────────────────────────────────────────
                Rectangle {
                    id: listPane
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: Math.round(root._splitRatio * parent.avail)
                    radius: Theme.radiusLG
                    color: Theme.bgSurface
                    border.width: 1
                    border.color: Theme.borderSubtle
                    clip: true

                    onWidthChanged: root._colW = root.columnWidths(width - 2)

                    Column {
                        anchors.fill: parent

                        // Column header (.hdr, styles.css:225-236): 8px
                        // block padding, 1px bottom rule in border-strong.
                        Item {
                            id: headerRow
                            width: parent.width
                            height: 33

                            // Activate (centered over the checkbox column)
                            Item {
                                x: root.colX(root._colW, 0)
                                width: root._colW.length > 0 ? root._colW[0] : 70
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                Text {
                                    anchors.centerIn: parent
                                    text: qsTr("启用")
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeMD
                                    font.weight: Font.DemiBold
                                }
                            }

                            Repeater {
                                model: [
                                    { key: "name", title: qsTr("名称") },
                                    { key: "version", title: qsTr("插件版本") },
                                    { key: "source", title: qsTr("来源") },
                                    { key: "status", title: qsTr("状态") }
                                ]
                                delegate: Item {
                                    id: hdrCell
                                    required property var modelData
                                    required property int index
                                    x: root.colX(root._colW, index + 1)
                                    width: root._colW.length > index + 1
                                           ? root._colW[index + 1] : 0
                                    anchors.top: parent.top
                                    anchors.bottom: parent.bottom
                                    property bool activeSort:
                                        root._sortKey === modelData.key
                                    Row {
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        spacing: 6
                                        Text {
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: hdrCell.modelData.title
                                            color: Theme.textPrimary
                                            font.pixelSize: Theme.fontSizeMD
                                            font.weight: Font.DemiBold
                                        }
                                        // sort triangle: hidden until hover,
                                        // active column wins (plugin-sort.css)
                                        Item {
                                            width: 8
                                            height: 10
                                            anchors.verticalCenter: parent.verticalCenter
                                            Canvas {
                                                id: sortTri
                                                anchors.fill: parent
                                                antialiasing: true
                                                visible: hdrCell.activeSort
                                                         || headerSortMouse.containsMouse
                                                // reactive inputs: Canvas paints
                                                // once, so every state the paint
                                                // reads must trigger requestPaint
                                                property bool triUp:
                                                    hdrCell.activeSort
                                                    ? root._sortOrder === "asc"
                                                    : true
                                                property bool triActive:
                                                    hdrCell.activeSort
                                                onTriUpChanged: requestPaint()
                                                onTriActiveChanged: requestPaint()
                                                onVisibleChanged: requestPaint()
                                                onPaint: {
                                                    const ctx = getContext("2d")
                                                    ctx.clearRect(0, 0, width, height)
                                                    ctx.fillStyle = triActive
                                                            ? Theme.textPrimary
                                                            : Theme.textMuted
                                                    ctx.beginPath()
                                                    if (triUp) {
                                                        ctx.moveTo(4, 1)
                                                        ctx.lineTo(8, 6)
                                                        ctx.lineTo(0, 6)
                                                    } else {
                                                        ctx.moveTo(0, 1)
                                                        ctx.lineTo(8, 1)
                                                        ctx.lineTo(4, 6)
                                                    }
                                                    ctx.closePath()
                                                    ctx.fill()
                                                }
                                            }
                                        }
                                    }
                                    MouseArea {
                                        id: headerSortMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.cycleSort(hdrCell.modelData.key)
                                    }
                                    // header column separator
                                    Rectangle {
                                        anchors.top: parent.top
                                        anchors.bottom: parent.bottom
                                        anchors.right: parent.right
                                        width: 1
                                        visible: hdrCell.index < 3
                                        color: Theme.borderSubtle
                                    }
                                }
                            }

                            Rectangle {
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                height: 1
                                color: Theme.borderStrong
                            }
                        }

                        // Rows (index.js RenderPlugins)
                        ListView {
                            id: listView
                            width: parent.width
                            height: parent.height - headerRow.height
                            clip: true
                            boundsBehavior: Flickable.StopAtBounds
                            model: root._view
                            ScrollBar.vertical: ScrollBar {
                                policy: ScrollBar.AsNeeded
                                contentItem: Rectangle {
                                    implicitWidth: 6
                                    radius: Theme.radiusSM
                                    color: Theme.scrollBarColor
                                }
                            }
                            delegate: Item {
                                id: block
                                required property var modelData
                                required property int index
                                readonly property var row: modelData
                                readonly property bool selected:
                                    root._selectedName === row.name
                                readonly property bool expanded:
                                    root.rowExpanded(row)
                                readonly property var caps:
                                    row.capabilities || []
                                width: ListView.view ? ListView.view.width : 0
                                height: blockCol.implicitHeight

                                // selected backdrop + 1px inset outline
                                Rectangle {
                                    anchors.fill: parent
                                    visible: block.selected
                                    color: Qt.alpha(Theme.accent, 0.14)
                                    border.width: 1
                                    border.color: Theme.accent
                                }

                                Column {
                                    id: blockCol
                                    width: parent.width

                                    // ── main row (min-height 34px) ────
                                    Item {
                                        id: rowItem
                                        width: parent.width
                                        height: 34

                                        Rectangle {
                                            anchors.fill: parent
                                            color: Theme.bgHover
                                            visible: rowMouse.containsMouse
                                                     && !block.selected
                                        }

                                        // Activate cell
                                        Item {
                                            x: root.colX(root._colW, 0)
                                            width: root._colW.length > 0 ? root._colW[0] : 70
                                            anchors.top: parent.top
                                            anchors.bottom: parent.bottom
                                            PluginCheck {
                                                anchors.centerIn: parent
                                                checked: block.row.isInstalled
                                                         && block.row.isEnabled
                                                mixed: root.rowMixed(block.row)
                                            }
                                            MouseArea {
                                                anchors.centerIn: parent
                                                width: 20
                                                height: 20
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: root.toggleRowActivation(
                                                                block.row)
                                            }
                                        }

                                        // Name cell: expand triangle + name +
                                        // capability count badge
                                        CellFrame {
                                            col: 1
                                            anchors.top: parent.top
                                            anchors.bottom: parent.bottom
                                            Item {
                                                id: nameInner
                                                anchors.fill: parent
                                                // expand button 18x22 / spacer
                                                Item {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    width: 18
                                                    height: 22
                                                    visible: block.caps.length > 0
                                                    Canvas {
                                                        id: expandTri
                                                        anchors.centerIn: parent
                                                        width: 8
                                                        height: 10
                                                        antialiasing: true
                                                        rotation: block.expanded ? 90 : 0
                                                        // repaint on hover so the
                                                        // glyph brightens
                                                        property bool hot:
                                                            expandMouse.containsMouse
                                                        onHotChanged: requestPaint()
                                                        onPaint: {
                                                            const ctx = getContext("2d")
                                                            ctx.clearRect(0, 0, width, height)
                                                            ctx.fillStyle = hot
                                                                    ? Theme.textPrimary
                                                                    : Theme.textMuted
                                                            ctx.beginPath()
                                                            ctx.moveTo(2, 1)
                                                            ctx.lineTo(7, 5)
                                                            ctx.lineTo(2, 9)
                                                            ctx.closePath()
                                                            ctx.fill()
                                                        }
                                                    }
                                                    MouseArea {
                                                        id: expandMouse
                                                        anchors.fill: parent
                                                        hoverEnabled: true
                                                        cursorShape: Qt.PointingHandCursor
                                                        onClicked: {
                                                            root._selectedName =
                                                                    block.row.name
                                                            root.toggleExpanded(
                                                                        block.row)
                                                        }
                                                    }
                                                }
                                                Row {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    anchors.left: parent.left
                                                    anchors.leftMargin: 18
                                                    spacing: 6
                                                    Text {
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        width: Math.min(
                                                                    implicitWidth,
                                                                    nameInner.width - 18
                                                                    - (block.caps.length > 0 ? 24 : 0))
                                                        text: block.row.name
                                                        color: Theme.textPrimary
                                                        font.pixelSize: Theme.fontSizeMD
                                                        elide: Text.ElideRight
                                                    }
                                                    Rectangle {
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        visible: block.caps.length > 0
                                                        width: Math.max(
                                                                    18,
                                                                    capCount.implicitWidth + 10)
                                                        height: 16
                                                        radius: Theme.radiusLG
                                                        color: Qt.alpha(Theme.accent, 0.14)
                                                        Text {
                                                            id: capCount
                                                            anchors.centerIn: parent
                                                            text: String(block.caps.length)
                                                            color: Theme.accent
                                                            font.pixelSize: Theme.fontSizeSM
                                                            font.bold: true
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        // Version cell: plain text + update
                                        // badge
                                        CellFrame {
                                            col: 2
                                            anchors.top: parent.top
                                            anchors.bottom: parent.bottom
                                            Row {
                                                anchors.verticalCenter: parent.verticalCenter
                                                anchors.left: parent.left
                                                spacing: 8
                                                Text {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    text: root.displayVersion(block.row)
                                                    color: Theme.textPrimary
                                                    font.pixelSize: Theme.fontSizeMD
                                                    elide: Text.ElideRight
                                                }
                                                // 18x18 green update badge
                                                // (styles.css:466-492)
                                                Rectangle {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    visible: block.row.updateStatus
                                                             === "update_available"
                                                    width: 18
                                                    height: 18
                                                    radius: Theme.radiusMD
                                                    gradient: Gradient {
                                                        GradientStop {
                                                            position: 0.0
                                                            color: Theme.accentLight
                                                        }
                                                        GradientStop {
                                                            position: 1.0
                                                            color: Theme.accentDark
                                                        }
                                                    }
                                                    border.width: 1
                                                    border.color: Qt.rgba(0, 0, 0, 0.32)
                                                    Canvas {
                                                        anchors.centerIn: parent
                                                        width: 12
                                                        height: 12
                                                        antialiasing: true
                                                        onPaint: {
                                                            const ctx = getContext("2d")
                                                            ctx.clearRect(0, 0, width, height)
                                                            ctx.fillStyle = Theme.textOnAccent
                                                            ctx.beginPath()
                                                            ctx.moveTo(5, 11)
                                                            ctx.lineTo(5, 5)
                                                            ctx.lineTo(2.75, 5)
                                                            ctx.lineTo(6, 1)
                                                            ctx.lineTo(9.25, 5)
                                                            ctx.lineTo(7, 5)
                                                            ctx.lineTo(7, 11)
                                                            ctx.closePath()
                                                            ctx.fill()
                                                        }
                                                    }
                                                }
                                                // warning variant (unauthorized)
                                                Rectangle {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    visible: block.row.updateStatus
                                                             === "unauthorized"
                                                    width: 18
                                                    height: 18
                                                    radius: Theme.radiusMD
                                                    color: Theme.statusWarning
                                                    border.width: 1
                                                    border.color: Qt.rgba(0, 0, 0, 0.32)
                                                    Text {
                                                        anchors.centerIn: parent
                                                        text: "!"
                                                        color: Theme.textOnAccent
                                                        font.pixelSize: Theme.fontSizeMD
                                                        font.bold: true
                                                    }
                                                }
                                            }
                                        }

                                        // Source cell (colored text variants)
                                        CellFrame {
                                            col: 3
                                            anchors.top: parent.top
                                            anchors.bottom: parent.bottom
                                            Text {
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: root.sourceLabel(block.row.source)
                                                color: root.sourceColor(block.row.source)
                                                font.pixelSize: Theme.fontSizeMD
                                                font.weight: root.sourceBold(
                                                                 block.row.source)
                                                             ? Font.DemiBold : Font.Normal
                                                elide: Text.ElideRight
                                            }
                                        }

                                        // Status cell
                                        CellFrame {
                                            col: 4
                                            anchors.top: parent.top
                                            anchors.bottom: parent.bottom
                                            Text {
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: block.row.status
                                                color: root.statusColor(
                                                           block.row.statusKey)
                                                font.pixelSize: Theme.fontSizeMD
                                                font.weight: root.statusBold(
                                                                 block.row.statusKey)
                                                             ? Font.DemiBold : Font.Normal
                                                elide: Text.ElideRight
                                            }
                                        }

                                        MouseArea {
                                            id: rowMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            acceptedButtons: Qt.LeftButton
                                                             | Qt.RightButton
                                            onClicked: (mouse) => {
                                                if (mouse.button === Qt.LeftButton)
                                                    root._selectedName = block.row.name
                                            }
                                            onPressed: (mouse) => {
                                                if (mouse.button === Qt.RightButton)
                                                    root.openContextMenu(
                                                                block.row, rowItem,
                                                                mouse.x, mouse.y)
                                            }
                                        }
                                    }

                                    // ── capability sub-rows (index.js
                                    //    RenderCapabilityTree) ──────────
                                    Rectangle {
                                        width: parent.width
                                        height: visible
                                                ? block.caps.length * 30 : 0
                                        visible: block.expanded
                                        color: Qt.alpha(Theme.accent, 0.04)
                                        Rectangle {
                                            anchors.top: parent.top
                                            anchors.bottom: parent.bottom
                                            anchors.left: parent.left
                                            width: 2
                                            color: Theme.accent
                                        }
                                        Column {
                                            anchors.fill: parent
                                            Repeater {
                                                model: block.caps
                                                delegate: Item {
                                                    id: capRow
                                                    required property var modelData
                                                    required property int index
                                                    width: parent.width
                                                    height: 30
                                                    Rectangle {
                                                        anchors.fill: parent
                                                        color: Theme.bgHover
                                                        opacity: 0.35
                                                        visible: capHover.containsMouse
                                                    }
                                                    // row-level hover tracker;
                                                    // the cells above stay on top
                                                    MouseArea {
                                                        id: capHover
                                                        anchors.fill: parent
                                                        hoverEnabled: true
                                                        acceptedButtons: Qt.NoButton
                                                    }
                                                    Rectangle {
                                                        anchors.top: parent.top
                                                        anchors.left: parent.left
                                                        anchors.right: parent.right
                                                        height: 1
                                                        visible: capRow.index > 0
                                                        color: Theme.borderSubtle
                                                    }
                                                    // checkbox cell
                                                    Item {
                                                        x: root.colX(root._colW, 0)
                                                        width: root._colW.length > 0
                                                               ? root._colW[0] : 70
                                                        anchors.top: parent.top
                                                        anchors.bottom: parent.bottom
                                                        PluginCheck {
                                                            anchors.centerIn: parent
                                                            checked: capRow.modelData.enabled
                                                            visible: capRow.modelData.canToggle
                                                        }
                                                        MouseArea {
                                                            anchors.centerIn: parent
                                                            width: 20
                                                            height: 20
                                                            enabled: capRow.modelData.canToggle
                                                            cursorShape: Qt.PointingHandCursor
                                                            onClicked: {
                                                                if (root.pluginService)
                                                                    root.pluginService.setCapabilityEnabled(
                                                                                block.row.idx,
                                                                                capRow.modelData.name,
                                                                                !capRow.modelData.enabled)
                                                            }
                                                        }
                                                    }
                                                    // name cell with branch glyph
                                                    CellFrame {
                                                        col: 1
                                                        anchors.top: parent.top
                                                        anchors.bottom: parent.bottom
                                                        Item {
                                                            anchors.fill: parent
                                                            Row {
                                                                anchors.verticalCenter: parent.verticalCenter
                                                                anchors.left: parent.left
                                                                spacing: 8
                                                                Canvas {
                                                                    anchors.verticalCenter: parent.verticalCenter
                                                                    width: 18
                                                                    height: 18
                                                                    antialiasing: true
                                                                    onPaint: {
                                                                        // L branch glyph
                                                                        const ctx = getContext("2d")
                                                                        ctx.clearRect(0, 0, width, height)
                                                                        ctx.strokeStyle = Theme.textPrimary
                                                                        ctx.lineWidth = 1
                                                                        ctx.globalAlpha = 0.9
                                                                        ctx.beginPath()
                                                                        ctx.moveTo(2, 0)
                                                                        ctx.lineTo(2, 13)
                                                                        ctx.lineTo(17, 13)
                                                                        ctx.stroke()
                                                                    }
                                                                }
                                                                Text {
                                                                    anchors.verticalCenter: parent.verticalCenter
                                                                    width: Math.min(
                                                                                implicitWidth,
                                                                                parent.parent.width - 26)
                                                                    text: capRow.modelData.name
                                                                    color: Theme.textPrimary
                                                                    font.pixelSize: Theme.fontSizeMD
                                                                    elide: Text.ElideRight
                                                                }
                                                            }
                                                        }
                                                    }
                                                    // type cell
                                                    CellFrame {
                                                        col: 2
                                                        anchors.top: parent.top
                                                        anchors.bottom: parent.bottom
                                                        Text {
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            text: capRow.modelData.type
                                                            color: Theme.textMuted
                                                            font.pixelSize: Theme.fontSizeMD
                                                            elide: Text.ElideRight
                                                        }
                                                    }
                                                    // source cell (spacer,
                                                    // upstream
                                                    // capability-source-cell)
                                                    CellFrame {
                                                        col: 3
                                                        anchors.top: parent.top
                                                        anchors.bottom: parent.bottom
                                                    }
                                                    // actions cell: run button
                                                    // for runnable script caps
                                                    CellFrame {
                                                        col: 4
                                                        anchors.top: parent.top
                                                        anchors.bottom: parent.bottom
                                                        Item {
                                                            anchors.fill: parent
                                                            Rectangle {
                                                                anchors.centerIn: parent
                                                                width: 22
                                                                height: 22
                                                                radius: Theme.radiusSM
                                                                visible: capRow.modelData.canRun
                                                                         && capRow.modelData.enabled
                                                                color: runMouse.containsMouse
                                                                        ? Qt.alpha(Theme.accent, 0.12)
                                                                        : "transparent"
                                                                Canvas {
                                                                    anchors.centerIn: parent
                                                                    width: 16
                                                                    height: 16
                                                                    antialiasing: true
                                                                    onPaint: {
                                                                        // play triangle outline
                                                                        const ctx = getContext("2d")
                                                                        ctx.clearRect(0, 0, width, height)
                                                                        ctx.strokeStyle = Theme.accent
                                                                        ctx.lineWidth = 1.5
                                                                        ctx.lineJoin = "round"
                                                                        ctx.beginPath()
                                                                        ctx.moveTo(3.25, 2.25)
                                                                        ctx.lineTo(12.75, 8)
                                                                        ctx.lineTo(3.25, 13.75)
                                                                        ctx.closePath()
                                                                        ctx.stroke()
                                                                    }
                                                                }
                                                                MouseArea {
                                                                    id: runMouse
                                                                    anchors.fill: parent
                                                                    hoverEnabled: true
                                                                    cursorShape: Qt.PointingHandCursor
                                                                    onClicked: root.runCapability(
                                                                                    block.row,
                                                                                    capRow.modelData)
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Empty states (index.js RenderPlugins: "No plugins
                    // found" and the search no-match variant)
                    Text {
                        anchors.top: parent.top
                        anchors.topMargin: 18 + headerRow.height
                        anchors.left: parent.left
                        anchors.leftMargin: 18
                        visible: root._view.length === 0
                        text: root.searchActive()
                              ? qsTr("没有匹配 “%1” 的插件").arg(root._search)
                              : qsTr("未找到插件")
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSizeMD
                    }
                }

                // ── Splitter: 9px strip, 2px visible divider ───────────
                Item {
                    id: paneSplitter
                    anchors.top: listPane.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 9
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 2
                        radius: Theme.radiusXS
                        color: splitMouse.pressed ? Theme.accent
                              : splitMouse.containsMouse ? Theme.borderStrong
                              : "transparent"
                    }
                    MouseArea {
                        id: splitMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.SizeVerCursor
                        property real pressY
                        property real pressRatio
                        onPressed: (mouse) => {
                            pressY = mapToItem(contentPane, mouse.x, mouse.y).y
                            pressRatio = root._splitRatio
                        }
                        onPositionChanged: (mouse) => {
                            if (!pressed)
                                return
                            const y = mapToItem(contentPane, mouse.x, mouse.y).y
                            // grab-offset preserved so the strip does not jump
                            const delta = y - pressY
                            root.applySplitRatio(
                                        pressRatio + delta
                                        / Math.max(1, contentPane.avail))
                        }
                        onDoubleClicked: root.applySplitRatio(0.62)
                    }
                }

                // ── Details pane (index.html:90-204) ───────────────────
                Rectangle {
                    id: detailsPane
                    anchors.top: paneSplitter.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    radius: Theme.radiusLG
                    color: Theme.bgSurface
                    border.width: 1
                    border.color: Theme.borderSubtle
                    clip: true

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: Theme.spacingMD

                        // 5 equal tabs (styles.css:1037-1078)
                        Item {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 33
                            RowLayout {
                                anchors.fill: parent
                                spacing: 0
                                Repeater {
                                    model: [
                                        { id: "plugin-info", title: qsTr("插件信息") },
                                        { id: "description", title: qsTr("描述") },
                                        { id: "config", title: qsTr("配置") },
                                        { id: "changelog", title: qsTr("更新日志") },
                                        { id: "diagnostics", title: qsTr("诊断") }
                                    ]
                                    delegate: Item {
                                        id: detailTab
                                        required property var modelData
                                        required property int index
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        property bool active:
                                            root._detailTab === modelData.id
                                        Rectangle {
                                            anchors.fill: parent
                                            color: tabMouse.containsMouse
                                                    ? Theme.bgHover : "transparent"
                                        }
                                        Text {
                                            anchors.centerIn: parent
                                            text: detailTab.modelData.title
                                            color: detailTab.active
                                                   ? Theme.textPrimary : Theme.textMuted
                                            font.pixelSize: Theme.fontSizeMD
                                            font.weight: Font.DemiBold
                                        }
                                        Rectangle {
                                            anchors.top: parent.top
                                            anchors.bottom: parent.bottom
                                            anchors.right: parent.right
                                            width: 1
                                            visible: detailTab.index < 4
                                            color: Theme.borderStrong
                                        }
                                        MouseArea {
                                            id: tabMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root._detailTab =
                                                       detailTab.modelData.id
                                        }
                                    }
                                }
                            }
                            // 1px bottom rule; active tab draws its 2px
                            // accent underline on top (detail-tab.active)
                            Rectangle {
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                height: 1
                                color: Theme.borderStrong
                            }
                            Rectangle {
                                anchors.bottom: parent.bottom
                                x: parent.width / 5 * root._detailTabIndex()
                                width: parent.width / 5
                                height: 2
                                color: Theme.accent
                                visible: root._detailTabIndex() >= 0
                            }
                        }

                        // ── Tab panels ─────────────────────────────────
                        StackLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            currentIndex: root._detailTabIndex()

                            // ── Plugin Info ────────────────────────────
                            Item {
                                Row {
                                    anchors.fill: parent
                                    spacing: Theme.spacingXL

                                    // thumbnail 140x140 framed box
                                    // (styles.css:1106-1115); the mock
                                    // registry carries no thumbnail_url so
                                    // the frame stays empty (upstream hides
                                    // the img and keeps the frame)
                                    Rectangle {
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 140
                                        height: 140
                                        radius: Theme.radiusLG
                                        color: Theme.bgHover
                                        border.width: 1
                                        border.color: Theme.borderStrong
                                    }

                                    Column {
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: parent.width - 160

                                        Repeater {
                                            model: [
                                                { label: qsTr("来源"), kind: "source" },
                                                { label: qsTr("类型"), kind: "types" },
                                                { label: qsTr("作者"), kind: "author" },
                                                { label: qsTr("已装版本"), kind: "installed" },
                                                { label: qsTr("最新版本"), kind: "latest" }
                                            ]
                                            delegate: Item {
                                                id: infoField
                                                required property var modelData
                                                required property int index
                                                width: parent.width
                                                height: 33
                                                // top border (first row: none)
                                                Rectangle {
                                                    anchors.top: parent.top
                                                    anchors.left: parent.left
                                                    anchors.right: parent.right
                                                    height: 1
                                                    visible: infoField.index > 0
                                                    color: Theme.borderStrong
                                                }
                                                Text {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    x: 0
                                                    width: 110
                                                    text: infoField.modelData.label
                                                    color: Theme.textMuted
                                                    font.pixelSize: Theme.fontSizeMD
                                                    font.weight: Font.DemiBold
                                                    elide: Text.ElideRight
                                                }
                                                // value area
                                                Item {
                                                    anchors.top: parent.top
                                                    anchors.bottom: parent.bottom
                                                    anchors.left: parent.left
                                                    anchors.leftMargin: 122
                                                    anchors.right: parent.right
                                                    clip: true

                                                    // source: shared pill badge
                                                    Rectangle {
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        visible: infoField.modelData.kind === "source"
                                                                 && root._selected !== null
                                                        width: Math.max(
                                                                    17,
                                                                    infoSourceText.implicitWidth + 14)
                                                        height: 17
                                                        radius: 8.5
                                                        color: root._selected
                                                               ? Qt.alpha(
                                                                     root.sourceColor(
                                                                         root._selected.source),
                                                                     0.14)
                                                               : "transparent"
                                                        Text {
                                                            id: infoSourceText
                                                            anchors.centerIn: parent
                                                            text: root._selected
                                                                  ? root.sourceLabel(
                                                                        root._selected.source)
                                                                  : "-"
                                                            color: root._selected
                                                                   ? root.sourceColor(
                                                                         root._selected.source)
                                                                   : Theme.textTertiary
                                                            font.pixelSize: Theme.fontSizeSM
                                                            font.weight: Font.DemiBold
                                                        }
                                                    }
                                                    Text {
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        visible: infoField.modelData.kind === "types"
                                                        text: root._selected
                                                              ? String(
                                                                    root._selected.types)
                                                              : "-"
                                                        color: Theme.textPrimary
                                                        font.pixelSize: Theme.fontSizeMD
                                                        elide: Text.ElideRight
                                                        width: Math.min(
                                                                    implicitWidth,
                                                                    parent.width)
                                                    }
                                                    Text {
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        visible: infoField.modelData.kind === "author"
                                                        text: root._selected
                                                              ? (String(
                                                                     root._selected.author)
                                                                 || "-")
                                                              : "-"
                                                        color: Theme.textPrimary
                                                        font.pixelSize: Theme.fontSizeMD
                                                        elide: Text.ElideRight
                                                        width: Math.min(
                                                                    implicitWidth,
                                                                    parent.width)
                                                    }
                                                    Text {
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        visible: infoField.modelData.kind === "installed"
                                                        text: root._selected
                                                              ? (root._selected.isInstalled
                                                                 ? (String(
                                                                        root._selected.installedVersion)
                                                                    || "-")
                                                                 : qsTr("未安装"))
                                                              : "-"
                                                        color: Theme.textPrimary
                                                        font.pixelSize: Theme.fontSizeMD
                                                        elide: Text.ElideRight
                                                        width: Math.min(
                                                                    implicitWidth,
                                                                    parent.width)
                                                    }
                                                    // latest version row:
                                                    // value + warning badge +
                                                    // Update button
                                                    Row {
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        visible: infoField.modelData.kind === "latest"
                                                        spacing: 8
                                                        Text {
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            text: root._selected
                                                                  ? (String(
                                                                         root._selected.latestVersion)
                                                                     || "-")
                                                                  : "-"
                                                            color: Theme.textPrimary
                                                            font.pixelSize: Theme.fontSizeMD
                                                        }
                                                        // detail badge only warns
                                                        // about unauthorized
                                                        // (index.js:1299-1316)
                                                        Rectangle {
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            visible: root._selected
                                                                     && root._selected.updateStatus
                                                                     === "unauthorized"
                                                            width: 18
                                                            height: 18
                                                            radius: Theme.radiusMD
                                                            color: Theme.statusWarning
                                                            border.width: 1
                                                            border.color: Qt.rgba(0, 0, 0, 0.32)
                                                            Text {
                                                                anchors.centerIn: parent
                                                                text: "!"
                                                                color: Theme.textOnAccent
                                                                font.pixelSize: Theme.fontSizeMD
                                                                font.bold: true
                                                            }
                                                        }
                                                        Rectangle {
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            visible: root._selected
                                                                     && root._selected.updateStatus
                                                                     === "update_available"
                                                            width: updateLabel.implicitWidth + 24
                                                            height: 23
                                                            radius: Theme.radiusMD
                                                            gradient: Gradient {
                                                                GradientStop {
                                                                    position: 0.0
                                                                    color: Theme.accentLight
                                                                }
                                                                GradientStop {
                                                                    position: 1.0
                                                                    color: Theme.accentDark
                                                                }
                                                            }
                                                            border.width: 1
                                                            border.color: Qt.rgba(0, 0, 0, 0.32)
                                                            Text {
                                                                id: updateLabel
                                                                anchors.centerIn: parent
                                                                text: qsTr("更新")
                                                                color: Theme.textOnAccent
                                                                font.pixelSize: Theme.fontSizeMD
                                                                font.weight: Font.DemiBold
                                                            }
                                                            MouseArea {
                                                                anchors.fill: parent
                                                                cursorShape: Qt.PointingHandCursor
                                                                onClicked: {
                                                                    if (root.pluginService
                                                                            && root._selected)
                                                                        root.pluginService.updatePlugin(
                                                                                    root._selected.idx)
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // ── Description ────────────────────────────
                            Item {
                                Column {
                                    anchors.fill: parent
                                    Text {
                                        text: qsTr("描述")
                                        color: Theme.textPrimary
                                        font.pixelSize: Theme.fontSize13
                                        font.weight: Font.DemiBold
                                        bottomPadding: Theme.spacingMD
                                    }
                                    Text {
                                        width: parent.width
                                        text: root._selected
                                              ? (String(root._selected.description)
                                                 || qsTr("暂无描述"))
                                              : qsTr("暂无描述")
                                        color: Theme.textPrimary
                                        font.pixelSize: Theme.fontSize13
                                        wrapMode: Text.Wrap
                                        lineHeight: 1.45
                                    }
                                }
                            }

                            // ── Config ─────────────────────────────────
                            Item {
                                Text {
                                    anchors.top: parent.top
                                    anchors.topMargin: 18
                                    anchors.left: parent.left
                                    anchors.leftMargin: 8
                                    visible: root._cfgCaps.length === 0
                                    text: root.emptyConfigMessage()
                                    color: Theme.textMuted
                                    font.pixelSize: Theme.fontSizeMD
                                }

                                Row {
                                    anchors.fill: parent
                                    spacing: Theme.spacingMD
                                    visible: root._cfgCaps.length > 0

                                    // capability sidebar 180px
                                    // (styles.css:1147-1205)
                                    Item {
                                        width: 180
                                        anchors.top: parent.top
                                        anchors.bottom: parent.bottom
                                        Rectangle {
                                            anchors.top: parent.top
                                            anchors.bottom: parent.bottom
                                            anchors.right: parent.right
                                            width: 1
                                            color: Theme.borderSubtle
                                        }
                                        ListView {
                                            anchors.fill: parent
                                            anchors.rightMargin: 4
                                            clip: true
                                            boundsBehavior: Flickable.StopAtBounds
                                            model: root._cfgCaps
                                            delegate: Item {
                                                id: cfgCap
                                                required property var modelData
                                                required property int index
                                                width: ListView.view
                                                       ? ListView.view.width : 0
                                                height: 44
                                                Rectangle {
                                                    anchors.fill: parent
                                                    anchors.margins: Theme.spacingXXS
                                                    radius: Theme.radiusSM
                                                    color: cfgCapMouse.containsMouse
                                                           ? Theme.bgHover
                                                           : "transparent"
                                                    border.width: 1
                                                    border.color: root._cfgCapName
                                                                  === cfgCap.modelData.name
                                                                  ? Theme.accent
                                                                  : "transparent"
                                                    // selected fill on top of
                                                    // the hover tint
                                                    Rectangle {
                                                        anchors.fill: parent
                                                        radius: Theme.radiusSM
                                                        visible: root._cfgCapName
                                                                 === cfgCap.modelData.name
                                                        color: Qt.alpha(Theme.accent, 0.14)
                                                    }
                                                }
                                                Column {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    anchors.left: parent.left
                                                    anchors.leftMargin: 8
                                                    anchors.right: parent.right
                                                    anchors.rightMargin: 8
                                                    spacing: Theme.spacingXXS
                                                    Text {
                                                        width: parent.width
                                                        text: cfgCap.modelData.name
                                                        color: Theme.textPrimary
                                                        font.pixelSize: Theme.fontSizeMD
                                                        elide: Text.ElideRight
                                                    }
                                                    Text {
                                                        width: parent.width
                                                        text: cfgCap.modelData.type
                                                        color: Theme.textMuted
                                                        font.pixelSize: Theme.fontSizeSM
                                                        elide: Text.ElideRight
                                                    }
                                                }
                                                MouseArea {
                                                    id: cfgCapMouse
                                                    anchors.fill: parent
                                                    hoverEnabled: true
                                                    cursorShape: Qt.PointingHandCursor
                                                    onClicked: {
                                                        root._cfgCapName =
                                                                cfgCap.modelData.name
                                                        root.loadConfigText()
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    // config view
                                    ColumnLayout {
                                        anchors.top: parent.top
                                        anchors.bottom: parent.bottom
                                        width: parent.width - 190
                                        spacing: 8

                                        // service-side error strip
                                        Rectangle {
                                            Layout.fillWidth: true
                                            visible: root._cfgError.length > 0
                                            implicitHeight: cfgErrorText.implicitHeight + 12
                                            radius: Theme.radiusSM
                                            color: Qt.alpha(Theme.statusWarning, 0.14)
                                            Text {
                                                id: cfgErrorText
                                                anchors.fill: parent
                                                anchors.margins: 6
                                                text: root._cfgError
                                                color: Theme.statusWarning
                                                font.pixelSize: Theme.fontSizeMD
                                                wrapMode: Text.Wrap
                                            }
                                        }

                                        TextArea {
                                            id: configEditor
                                            Layout.fillWidth: true
                                            Layout.fillHeight: true
                                            color: Theme.textPrimary
                                            selectionColor: Theme.selectionColor
                                            selectedTextColor: Theme.selectionText
                                            font.family: Theme.fontMono
                                            font.pixelSize: Theme.fontSizeMD
                                            wrapMode: TextArea.NoWrap
                                            selectByMouse: true
                                            persistentSelection: false
                                            background: Rectangle {
                                                color: Theme.bgBase
                                                border.width: 1
                                                border.color: configEditor.activeFocus
                                                              ? Theme.borderFocus
                                                              : Theme.borderSubtle
                                                radius: Theme.radiusSM
                                            }
                                            onTextChanged: {
                                                root._cfgText = text
                                                root.validateCfg()
                                            }
                                        }

                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 8
                                            Text {
                                                Layout.fillWidth: true
                                                text: root._cfgValidation
                                                visible: root._cfgValidation.length > 0
                                                color: Theme.statusError
                                                font.pixelSize: Theme.fontSizeSM
                                                elide: Text.ElideRight
                                            }
                                            Item { Layout.fillWidth: true; Layout.preferredHeight: 1 }
                                            CxButton {
                                                text: qsTr("恢复默认")
                                                cxStyle: CxButton.Style.Secondary
                                                compact: true
                                                toolTipText: qsTr("放弃已保存的配置并恢复插件默认值")
                                                enabled: root._cfgCap !== null
                                                onClicked: root.restoreCapabilityConfig()
                                            }
                                            CxButton {
                                                text: qsTr("保存")
                                                cxStyle: CxButton.Style.Primary
                                                compact: true
                                                enabled: root._cfgCap !== null
                                                         && root._cfgValid
                                                onClicked: root.saveCapabilityConfig()
                                            }
                                        }
                                    }
                                }
                            }

                            // ── Changelog ──────────────────────────────
                            Item {
                                id: changelogPanel
                                readonly property var rows:
                                    root._selected
                                    ? (root._selected.changelog || []) : []

                                Column {
                                    anchors.fill: parent
                                    visible: changelogPanel.rows.length > 0

                                    // table head
                                    Item {
                                        width: parent.width
                                        height: 33
                                        Text {
                                            x: 10
                                            width: 110
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: qsTr("版本")
                                            color: Theme.textMuted
                                            font.pixelSize: Theme.fontSizeMD
                                            font.weight: Font.DemiBold
                                        }
                                        Text {
                                            x: 130
                                            width: 100
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: qsTr("日期")
                                            color: Theme.textMuted
                                            font.pixelSize: Theme.fontSizeMD
                                            font.weight: Font.DemiBold
                                        }
                                        Text {
                                            x: 240
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: qsTr("变更说明")
                                            color: Theme.textMuted
                                            font.pixelSize: Theme.fontSizeMD
                                            font.weight: Font.DemiBold
                                        }
                                        Rectangle {
                                            anchors.bottom: parent.bottom
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            height: 1
                                            color: Theme.borderStrong
                                        }
                                    }

                                    Repeater {
                                        model: changelogPanel.rows
                                        delegate: Item {
                                            id: changeRow
                                            required property var modelData
                                            width: parent.width
                                            height: Math.max(
                                                        33,
                                                        changeText.implicitHeight + 16)
                                            Rectangle {
                                                anchors.bottom: parent.bottom
                                                anchors.left: parent.left
                                                anchors.right: parent.right
                                                height: 1
                                                color: Theme.borderSubtle
                                            }
                                            Text {
                                                x: 10
                                                width: 110
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: changeRow.modelData.version
                                                color: Theme.textPrimary
                                                font.pixelSize: Theme.fontSizeMD
                                                elide: Text.ElideRight
                                            }
                                            Text {
                                                x: 130
                                                width: 100
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: changeRow.modelData.date
                                                color: Theme.textPrimary
                                                font.pixelSize: Theme.fontSizeMD
                                                elide: Text.ElideRight
                                            }
                                            Text {
                                                id: changeText
                                                x: 240
                                                anchors.verticalCenter: parent.verticalCenter
                                                width: parent.width - 250
                                                text: changeRow.modelData.changes
                                                color: Theme.textPrimary
                                                font.pixelSize: Theme.fontSizeMD
                                                wrapMode: Text.Wrap
                                            }
                                        }
                                    }
                                }

                                Text {
                                    anchors.top: parent.top
                                    anchors.topMargin: 18
                                    anchors.left: parent.left
                                    anchors.leftMargin: 8
                                    visible: changelogPanel.rows.length === 0
                                    text: qsTr("暂无更新日志")
                                    color: Theme.textMuted
                                    font.pixelSize: Theme.fontSizeMD
                                }
                            }

                            // ── Diagnostics ────────────────────────────
                            Item {
                                Column {
                                    anchors.fill: parent
                                    visible: root._selected !== null
                                    spacing: 8

                                    // status chip (styles.css:660-691)
                                    Rectangle {
                                        width: diagChipText.implicitWidth + 20
                                        height: 24
                                        radius: Theme.radiusXL
                                        color: root._selected
                                               ? Qt.alpha(
                                                     root.statusColor(
                                                         root._selected.statusKey),
                                                     0.14)
                                               : "transparent"
                                        Text {
                                            id: diagChipText
                                            anchors.centerIn: parent
                                            text: root._selected
                                                  ? String(root._selected.status)
                                                  : "-"
                                            color: root._selected
                                                   ? root.statusColor(
                                                         root._selected.statusKey)
                                                   : Theme.textTertiary
                                            font.pixelSize: Theme.fontSizeLG
                                            font.weight: Font.DemiBold
                                        }
                                    }

                                    Text {
                                        width: parent.width
                                        visible: root._selected !== null
                                        text: root._selected
                                              ? root.statusDescription(
                                                    root._selected.statusKey)
                                              : ""
                                        // upstream renders the error variant of
                                        // StatusDescription in the danger tone
                                        // (index.js:1369-1373)
                                        color: root._selected
                                               && root._selected.statusKey === "error"
                                               ? Theme.statusError : Theme.textPrimary
                                        font.pixelSize: Theme.fontSize13
                                        wrapMode: Text.Wrap
                                    }

                                    Text {
                                        width: parent.width
                                        visible: root._selected
                                                 && root._selected.updateStatus
                                                 === "update_available"
                                        text: qsTr("发现新版本。")
                                        color: Theme.textMuted
                                        font.pixelSize: Theme.fontSize13
                                        wrapMode: Text.Wrap
                                    }

                                    Text {
                                        width: parent.width
                                        visible: root._selected
                                                 && root._selected.updateStatus
                                                 === "unauthorized"
                                        text: qsTr("该插件未授权更新，无法通过云端更新。")
                                        color: Theme.statusWarning
                                        font.pixelSize: Theme.fontSize13
                                        wrapMode: Text.Wrap
                                    }
                                }

                                Text {
                                    anchors.top: parent.top
                                    anchors.topMargin: 18
                                    anchors.left: parent.left
                                    anchors.leftMargin: 8
                                    visible: root._selected === null
                                    text: qsTr("选择一个插件以查看其状态与详情。")
                                    color: Theme.textMuted
                                    font.pixelSize: Theme.fontSizeMD
                                }
                            }
                        }
                    }
                }
            }

            // ── Status bar (index.html:207-210, styles.css:711-771) ────
            Rectangle {
                id: statusBar
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(28, statusText.implicitHeight + 12)
                color: Theme.bgSurface
                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 1
                    color: Theme.borderSubtle
                }
                Text {
                    id: statusText
                    anchors.verticalCenter: parent.verticalCenter
                    x: root._statusText.length > 0 ? 30 : 14
                    width: parent.width - (root._statusText.length > 0 ? 44 : 28)
                    visible: root._statusText.length > 0
                    text: root._statusText
                    color: root._statusLevel === "info"
                           ? Theme.textPrimary
                           : root.statusLevelColor(root._statusLevel)
                    font.pixelSize: Theme.fontSize13
                    elide: Text.ElideRight
                }
                Rectangle {
                    id: statusDot
                    anchors.verticalCenter: parent.verticalCenter
                    x: 14
                    width: 8
                    height: 8
                    radius: Theme.radiusSM
                    visible: root._statusText.length > 0
                    color: root.statusLevelColor(root._statusLevel)
                }
            }
        }

        // Height resize strip along the bottom edge (wxRESIZE_BORDER).
        MouseArea {
            id: resizeHeightArea
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 6
            cursorShape: Qt.SizeVerCursor
            property real pressGlobalY
            property real pressHeight
            onPressed: (mouse) => {
                pressGlobalY = mapToItem(Overlay.overlay, mouse.x, mouse.y).y
                pressHeight = root.height
            }
            onPositionChanged: (mouse) => {
                if (!pressed)
                    return
                const gy = mapToItem(Overlay.overlay, mouse.x, mouse.y).y
                const ov = Overlay.overlay
                const maxH = ov ? ov.height - 24 : root.height
                root.height = Math.min(maxH,
                                       Math.max(root.minDialogHeight,
                                                Math.round(pressHeight + gy
                                                           - pressGlobalY)))
            }
        }

        // Bottom-right diagonal resize grip.
        MouseArea {
            id: resizeCornerArea
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: 24
            height: 12
            cursorShape: Qt.SizeFDiagCursor
            property real pressGlobalX
            property real pressGlobalY
            property real pressWidth
            property real pressHeight
            onPressed: (mouse) => {
                const gp = mapToItem(Overlay.overlay, mouse.x, mouse.y)
                pressGlobalX = gp.x
                pressGlobalY = gp.y
                pressWidth = root.width
                pressHeight = root.height
            }
            onPositionChanged: (mouse) => {
                if (!pressed)
                    return
                const gp = mapToItem(Overlay.overlay, mouse.x, mouse.y)
                const ov = Overlay.overlay
                const maxW = ov ? ov.width - 24 : root.width
                const maxH = ov ? ov.height - 24 : root.height
                root.width = Math.min(maxW,
                                      Math.max(root.minDialogWidth,
                                               Math.round(pressWidth + gp.x
                                                          - pressGlobalX)))
                root.height = Math.min(maxH,
                                       Math.max(root.minDialogHeight,
                                                Math.round(pressHeight + gp.y
                                                           - pressGlobalY)))
            }
        }
    }

    // Right-edge width resize strip; lives in the background layer so it
    // receives clicks along the frame edge (EditGCodeDialog precedent).
    background: Rectangle {
        color: Theme.bgElevated
        border.color: Theme.borderInput
        border.width: 1
        radius: Theme.radiusLG

        MouseArea {
            id: resizeWidthArea
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 6
            cursorShape: Qt.SizeHorCursor
            property real pressGlobalX
            property real pressWidth
            onPressed: (mouse) => {
                pressGlobalX = mapToItem(Overlay.overlay, mouse.x, mouse.y).x
                pressWidth = root.width
            }
            onPositionChanged: (mouse) => {
                if (!pressed)
                    return
                const gx = mapToItem(Overlay.overlay, mouse.x, mouse.y).x
                const ov = Overlay.overlay
                const maxW = ov ? ov.width - 24 : root.width
                root.width = Math.min(maxW,
                                      Math.max(root.minDialogWidth,
                                               Math.round(pressWidth + gx
                                                          - pressGlobalX)))
            }
        }
    }

    // Row context menu (styles.css:773-814: min-width 190px, padding 4px,
    // radius 6, danger items in the danger color).
    Popup {
        id: ctxMenu
        parent: root.contentItem
        // focus makes the popup take keyboard focus while open so
        // CloseOnEscape actually receives Esc (upstream hides the menu on
        // a document-level Escape keydown, index.js:101-106); on close the
        // focus returns to the dialog.
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        width: Math.max(190, ctxCol.implicitWidth + 8)
        height: ctxCol.implicitHeight + 8
        padding: 4
        background: Rectangle {
            color: Theme.bgSurface
            border.width: 1
            border.color: Theme.borderDefault
            radius: Theme.radiusMD
        }
        contentItem: Column {
            id: ctxCol
            spacing: Theme.spacingXXS
            Repeater {
                model: root._ctxActions
                delegate: Item {
                    id: ctxItem
                    required property var modelData
                    required property int index
                    width: ctxCol.width
                    height: 32
                    Rectangle {
                        anchors.fill: parent
                        radius: Theme.radiusSM
                        color: ctxItemMouse.containsMouse
                                ? Theme.bgHover : "transparent"
                        visible: ctxItem.modelData.enabled !== false
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        text: ctxItem.modelData.label
                        color: ctxItem.modelData.danger
                               ? Theme.statusError
                               : (ctxItem.modelData.enabled === false
                                  ? Theme.textDisabled : Theme.textPrimary)
                        font.pixelSize: Theme.fontSizeMD
                        opacity: ctxItem.modelData.enabled === false ? 0.45 : 1.0
                    }
                    MouseArea {
                        id: ctxItemMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        enabled: ctxItem.modelData.enabled !== false
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.runContextAction(ctxItem.modelData.id)
                    }
                }
            }
        }
    }

    // Destructive context-action confirm (upstream delete_local_plugin /
    // unsubscribe_cloud_plugin wxMessageBox wxYES_NO|wxNO_DEFAULT|
    // wxICON_WARNING, PluginsDialog.cpp:1106-1172). Title, message and the
    // Yes/No button pair are set per invocation by
    // openPluginActionConfirm().
    ConfirmDialog {
        id: pluginActionConfirm
        destructive: true
        width: 440
        height: 240
    }

    // Install dropdown menu: main-color panel, white items
    // (styles.css:110-148).
    Popup {
        id: exploreMenu
        parent: root.contentItem
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        width: 140
        height: exploreCol.implicitHeight + 8
        padding: 4
        background: Rectangle {
            color: Theme.accent
            radius: 0
        }
        contentItem: Column {
            id: exploreCol
            spacing: 0
            Repeater {
                model: root._installActionLabels
                delegate: Item {
                    id: exploreItem
                    required property string modelData
                    required property int index
                    width: exploreCol.width
                    height: 32
                    Rectangle {
                        anchors.fill: parent
                        color: root._installAction === exploreItem.index
                               || exploreItemMouse.containsMouse
                               ? Qt.rgba(1, 1, 1, 0.14) : "transparent"
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        text: exploreItem.modelData
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSize13
                    }
                    MouseArea {
                        id: exploreItemMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root._installAction = exploreItem.index
                            exploreMenu.close()
                        }
                    }
                }
            }
        }
    }
}
