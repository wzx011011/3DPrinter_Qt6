import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

/// ModelMallPage - 3D model mall browsing interface
///
/// Upstream alignment: third_party/CrealityPrint/src/slic3r/GUI/ModelMall.cpp
///   - ModelMallDialog: DPIFrame with wxWebView loading a cloud-hosted mall URL
///   - Navigation controls: back, forward, refresh (mall_control_back/forward/refresh)
///   - Script message handling: "request_close_publish_window" command
///   - go_to_mall() / go_to_publish() for URL navigation
///
/// Current scope (P6.3): Native QML mock preview with grid layout, search,
/// category filtering, sort tabs, and model cards (thumbnail, name, author,
/// downloads, rating, price). WebView integration is a placeholder.
Item {
    id: root
    required property var modelMallVm

    Rectangle {
        anchors.fill: parent
        color: Theme.bgBase
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ══════════════════════════════════════════════════════════
        // TOP BAR — Search + sort tabs (aligns with upstream
        // ModelMallDialog m_web_control_panel navigation area)
        // ══════════════════════════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: Theme.bgSurface

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingXL
                anchors.rightMargin: Theme.spacingXL
                spacing: Theme.spacingLG

                // Page title
                Text {
                    text: qsTr("3D Model Mall")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeXL
                    font.bold: true
                }

                // Search bar (upstream: URL bar was commented out in ModelMallDialog)
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    radius: Theme.radiusLG
                    color: Theme.bgPanel
                    border.color: Theme.borderDefault
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: Theme.spacingSM

                        Text {
                            text: "\u{1F50D}"
                            font.pixelSize: Theme.fontSizeLG
                            color: Theme.textTertiary
                        }

                        TextField {
                            id: searchField
                            Layout.fillWidth: true
                            text: root.modelMallVm.searchQuery
                            placeholderText: qsTr("Search models...")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                            placeholderTextColor: Theme.textTertiary
                            background: Rectangle { color: "transparent" }
                            onTextChanged: root.modelMallVm.setSearchQuery(text)
                            onAccepted: root.modelMallVm.setSearchQuery(text)
                        }

                        // Clear button
                        Text {
                            visible: searchField.text.length > 0
                            text: "\u2715"
                            font.pixelSize: Theme.fontSizeMD
                            color: Theme.textTertiary
                            TapHandler {
                                onTapped: {
                                    searchField.text = ""
                                    searchField.forceActiveFocus()
                                }
                            }
                        }
                    }
                }

                // Sort tabs (aligns with upstream Recommended/Popular/Newest/Free)
                Repeater {
                    model: [qsTr("Recommended"), qsTr("Popular"), qsTr("Newest"), qsTr("Free")]
                    delegate: Rectangle {
                        required property int index
                        required property string modelData
                        height: 28
                        width: implicitText.width + 18
                        radius: 14
                        color: index === root.modelMallVm.sortMode ? Theme.accentSubtle : "transparent"
                        border.color: index === root.modelMallVm.sortMode ? Theme.accent : Theme.borderDefault
                        border.width: 1

                        Text {
                            id: implicitText
                            anchors.centerIn: parent
                            text: parent.modelData
                            color: index === root.modelMallVm.sortMode ? Theme.accent : Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSM
                        }

                        TapHandler {
                            onTapped: root.modelMallVm.setSortMode(index)
                        }
                    }
                }

                // Refresh button (aligns with upstream mall_control_refresh)
                Rectangle {
                    width: 30
                    height: 30
                    radius: Theme.radiusMD
                    color: refreshBtn.containsMouse ? Theme.bgHover : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: root.modelMallVm.isLoading ? "\u27F3" : "\u{1F504}"
                        font.pixelSize: 14
                        color: root.modelMallVm.isLoading ? Theme.accent : Theme.textSecondary
                        RotationAnimation on rotation {
                            running: root.modelMallVm.isLoading
                            loops: Animation.Infinite
                            from: 0; to: 360; duration: 1000
                        }
                    }

                    HoverHandler { id: refreshBtn }
                    TapHandler {
                        enabled: !root.modelMallVm.isLoading
                        onTapped: root.modelMallVm.refresh()
                    }
                }
            }
        }

        // ══════════════════════════════════════════════════════════
        // CATEGORY TABS — Horizontal category filter
        // ══════════════════════════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            color: Theme.bgInset

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingXL
                spacing: Theme.spacingSM

                Repeater {
                    model: root.modelMallVm.categories
                    delegate: Rectangle {
                        required property int index
                        required property string modelData
                        height: 26
                        width: catText.width + 20
                        radius: 13
                        color: index === root.modelMallVm.categoryIndex ? Theme.accentSubtle : "transparent"

                        Text {
                            id: catText
                            anchors.centerIn: parent
                            text: parent.modelData
                            color: index === root.modelMallVm.categoryIndex ? Theme.accent : Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSM
                        }

                        TapHandler {
                            onTapped: root.modelMallVm.setCategoryIndex(index)
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Results count
                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: Theme.spacingXL
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("%1 models").arg(root.modelMallVm.filteredCount)
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeXS
                }
            }
        }

        // ══════════════════════════════════════════════════════════
        // MODEL GRID — Card-based grid layout (Flow wrapping)
        // ══════════════════════════════════════════════════════════
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            Flow {
                id: gridFlow
                width: parent.width
                spacing: Theme.spacingLG
                topPadding: Theme.spacingLG
                leftPadding: Theme.spacingLG
                rightPadding: Theme.spacingLG
                bottomPadding: Theme.spacingLG

                Repeater {
                    model: root.modelMallVm.filteredCount

                    delegate: Rectangle {
                        id: card
                        required property int index
                        width: 192
                        height: 244
                        radius: Theme.radiusLG
                        color: Theme.bgSurface
                        border.color: cardMouse.containsMouse ? Theme.borderStrong : Theme.borderSubtle
                        border.width: 1
                        clip: true

                        Column {
                            anchors.fill: parent
                            spacing: 0

                            // ── Thumbnail area ──
                            Rectangle {
                                width: parent.width
                                height: 140
                                color: root.modelMallVm.modelThumbnailColor(card.index)

                                // Featured badge (aligns with upstream staffpick)
                                Rectangle {
                                    visible: root.modelMallVm.modelFeatured(card.index)
                                    anchors.left: parent.left
                                    anchors.top: parent.top
                                    anchors.margins: 8
                                    width: 52
                                    height: 20
                                    radius: 10
                                    color: Theme.accent

                                    Text {
                                        anchors.centerIn: parent
                                        text: qsTr("Featured")
                                        color: Theme.textOnAccent
                                        font.pixelSize: Theme.fontSizeXS
                                        font.bold: true
                                    }
                                }

                                // Free badge
                                Rectangle {
                                    visible: root.modelMallVm.modelFree(card.index)
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 8
                                    width: 36
                                    height: 20
                                    radius: 10
                                    color: "#0a3d1f"

                                    Text {
                                        anchors.centerIn: parent
                                        text: qsTr("Free")
                                        color: Theme.accent
                                        font.pixelSize: Theme.fontSizeXS
                                        font.bold: true
                                    }
                                }

                                // Centered thumbnail icon
                                Text {
                                    anchors.centerIn: parent
                                    text: root.modelMallVm.modelThumbnailIcon(card.index)
                                    font.pixelSize: 48
                                    color: "#ffffff40"
                                }

                                // Downloads count (bottom-right overlay)
                                Rectangle {
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    anchors.margins: 8
                                    width: dlText.width + 16
                                    height: 20
                                    radius: 10
                                    color: "#00000080"

                                    Text {
                                        id: dlText
                                        anchors.centerIn: parent
                                        text: "\u2B07 " + formatDownloads(root.modelMallVm.modelDownloads(card.index))
                                        color: Theme.textPrimary
                                        font.pixelSize: 9
                                    }
                                }
                            }

                            // ── Card info area ──
                            Column {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.margins: Theme.spacingMD
                                spacing: Theme.spacingSM
                                topPadding: Theme.spacingMD

                                // Model name
                                Text {
                                    text: root.modelMallVm.modelName(card.index)
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeMD
                                    font.bold: true
                                    elide: Text.ElideRight
                                    width: parent.width
                                }

                                // Author + Price row
                                RowLayout {
                                    width: parent.width
                                    spacing: Theme.spacingSM

                                    Text {
                                        text: root.modelMallVm.modelAuthor(card.index)
                                        color: Theme.textTertiary
                                        font.pixelSize: Theme.fontSizeSM
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    Text {
                                        text: root.modelMallVm.modelFree(card.index)
                                              ? qsTr("Free")
                                              : ("\u00A5" + root.modelMallVm.modelPrice(card.index).toFixed(1))
                                        color: root.modelMallVm.modelFree(card.index) ? Theme.accent : Theme.statusWarning
                                        font.pixelSize: Theme.fontSizeSM
                                        font.bold: true
                                    }
                                }

                                // Rating stars
                                Row {
                                    spacing: 2
                                    Repeater {
                                        model: 5
                                        delegate: Text {
                                            required property int index
                                            text: modelStarChar(card.index, index)
                                            font.pixelSize: 11
                                            color: Theme.statusWarning
                                        }
                                    }

                                    Text {
                                        anchors.left: parent.right
                                        anchors.leftMargin: 4
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: root.modelMallVm.modelRating(card.index).toFixed(1)
                                        color: Theme.textTertiary
                                        font.pixelSize: Theme.fontSizeXS
                                    }
                                }

                                // Download button
                                Rectangle {
                                    width: parent.width
                                    height: 28
                                    radius: Theme.radiusMD
                                    color: {
                                        if (root.modelMallVm.isDownloading(card.index))
                                            return Theme.bgPanel
                                        return dlBtnArea.containsMouse ? Theme.accentDark : Theme.accentSubtle
                                    }
                                    border.color: root.modelMallVm.isDownloading(card.index) ? Theme.borderDefault : Theme.accent
                                    border.width: 1

                                    Row {
                                        anchors.centerIn: parent
                                        spacing: 4
                                        visible: root.modelMallVm.isDownloading(card.index)
                                        Text {
                                            text: "\u27F3"
                                            font.pixelSize: 12
                                            color: Theme.textSecondary
                                            RotationAnimation on rotation {
                                                running: root.modelMallVm.isDownloading(card.index)
                                                loops: Animation.Infinite
                                                from: 0; to: 360; duration: 800
                                            }
                                        }
                                        Text {
                                            text: qsTr("Downloading...")
                                            color: Theme.textSecondary
                                            font.pixelSize: Theme.fontSizeSM
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }

                                    Text {
                                        anchors.centerIn: parent
                                        text: qsTr("Download")
                                        color: Theme.accent
                                        font.pixelSize: Theme.fontSizeSM
                                        font.bold: true
                                        visible: !root.modelMallVm.isDownloading(card.index)
                                    }

                                    HoverHandler { id: dlBtnArea }
                                    TapHandler {
                                        enabled: !root.modelMallVm.isDownloading(card.index)
                                        onTapped: root.modelMallVm.downloadModel(card.index)
                                    }
                                }
                            }
                        }

                        // Hover overlay
                        HoverHandler { id: cardMouse }
                        Rectangle {
                            anchors.fill: parent
                            radius: Theme.radiusLG
                            color: cardMouse.hovered ? "#08ffffff" : "transparent"
                        }

                        // Click to open detail (aligns with upstream OpenModelDetail -> browser)
                        TapHandler {
                            onTapped: root.modelMallVm.openModelDetail(card.index)
                        }
                    }
                }

                // Empty state
                Item {
                    visible: root.modelMallVm.filteredCount === 0
                    width: gridFlow.width
                    height: 200
                    Column {
                        anchors.centerIn: parent
                        spacing: Theme.spacingMD
                        Text {
                            text: "\u{1F50D}"
                            font.pixelSize: 40
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: qsTr("No models found")
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeLG
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: qsTr("Try adjusting your search or category filter")
                            color: Theme.textDisabled
                            font.pixelSize: Theme.fontSizeSM
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }
        }

        // ══════════════════════════════════════════════════════════
        // STATUS BAR — Bottom info bar
        // ══════════════════════════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.statusBarHeight
            color: Theme.chromeSurface

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingXL
                anchors.rightMargin: Theme.spacingXL
                spacing: Theme.spacingLG

                Text {
                    text: qsTr("Model Mall")
                    color: Theme.chromeTextMuted
                    font.pixelSize: Theme.fontSizeXS
                }

                Text {
                    text: qsTr("%1 models available").arg(root.modelMallVm.filteredCount)
                    color: Theme.chromeTextMuted
                    font.pixelSize: Theme.fontSizeXS
                }

                Item { Layout.fillWidth: true }

                // WebView status indicator
                Text {
                    visible: !root.modelMallVm.webViewAvailable
                    text: qsTr("Online mode unavailable \u2022 Showing offline preview")
                    color: Theme.statusWarning
                    font.pixelSize: Theme.fontSizeXS
                }

                Text {
                    text: qsTr("Powered by Creality Cloud")
                    color: Theme.chromeTextMuted
                    font.pixelSize: Theme.fontSizeXS
                }
            }
        }
    }

    // ── Helper functions ──

    function formatDownloads(count) {
        if (count >= 10000)
            return (count / 1000).toFixed(1) + "k";
        if (count >= 1000)
            return (count / 1000).toFixed(1) + "k";
        return count.toString();
    }

    function modelStarChar(cardIndex, starIndex) {
        var rating = root.modelMallVm.modelRating(cardIndex);
        if (starIndex < Math.floor(rating))
            return "\u2605"; // filled star
        if (starIndex < rating)
            return "\u2605"; // partial -> filled for simplicity
        return "\u2606";     // empty star
    }

    function formatFileSize(kb) {
        if (kb >= 1024)
            return (kb / 1024).toFixed(1) + " MB";
        return kb + " KB";
    }

    // ══════════════════════════════════════════════════════════
    // MODEL DETAIL DIALOG (aligns with upstream OpenModelDetail -> WebView)
    // ══════════════════════════════════════════════════════════
    Dialog {
        id: detailDialog
        anchors.centerIn: parent
        width: 560
        height: 440
        modal: true
        dim: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        visible: root.modelMallVm && root.modelMallVm.detailDialogOpen
        onVisibleChanged: {
            if (!visible && root.modelMallVm)
                root.modelMallVm.closeDetailDialog()
        }

        background: Rectangle {
            radius: Theme.radiusXL
            color: Theme.bgSurface
            border.color: Theme.borderDefault
            border.width: 1
        }

        contentItem: Item {
            anchors.fill: parent

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingXL
                spacing: Theme.spacingMD

                // Header: close button
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSM
                    Item { Layout.fillWidth: true }
                    Text {
                        text: qsTr("Model Details")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXL
                        font.bold: true
                    }
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        width: 28
                        height: 28
                        radius: 14
                        color: closeBtnArea.containsMouse ? Theme.bgHover : "transparent"
                        Text {
                            anchors.centerIn: parent
                            text: "\u2715"
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeLG
                        }
                        HoverHandler { id: closeBtnArea }
                        TapHandler { onTapped: detailDialog.close() }
                    }
                }

                // Main content: thumbnail + info
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: Theme.spacingXL

                    // Left: large thumbnail
                    Rectangle {
                        Layout.preferredWidth: 180
                        Layout.fillHeight: true
                        radius: Theme.radiusLG
                        color: root.modelMallVm ? root.modelMallVm.modelThumbnailColor(root.modelMallVm.selectedModelIndex) : "#333"

                        Rectangle {
                            visible: root.modelMallVm && root.modelMallVm.modelFeatured(root.modelMallVm.selectedModelIndex)
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.margins: 10
                            width: 56
                            height: 22
                            radius: 11
                            color: Theme.accent
                            Text {
                                anchors.centerIn: parent
                                text: qsTr("Featured")
                                color: Theme.textOnAccent
                                font.pixelSize: Theme.fontSizeXS
                                font.bold: true
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            text: root.modelMallVm ? root.modelMallVm.modelThumbnailIcon(root.modelMallVm.selectedModelIndex) : ""
                            font.pixelSize: 56
                            color: "#ffffff40"
                        }
                    }

                    // Right: model info
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: Theme.spacingSM

                        // Model name
                        Text {
                            text: root.modelMallVm ? root.modelMallVm.modelName(root.modelMallVm.selectedModelIndex) : ""
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeXL
                            font.bold: true
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        // Author
                        Text {
                            text: qsTr("by %1").arg(root.modelMallVm ? root.modelMallVm.modelAuthor(root.modelMallVm.selectedModelIndex) : "")
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeMD
                        }

                        // Rating stars + score
                        Row {
                            spacing: 2
                            Repeater {
                                model: 5
                                delegate: Text {
                                    required property int index
                                    text: {
                                        if (!root.modelMallVm) return "\u2606";
                                        var r = root.modelMallVm.modelRating(root.modelMallVm.selectedModelIndex);
                                        return index < Math.floor(r) ? "\u2605" : "\u2606";
                                    }
                                    font.pixelSize: 14
                                    color: Theme.statusWarning
                                }
                            }
                            Text {
                                anchors.leftMargin: 6
                                anchors.verticalCenter: parent.verticalCenter
                                text: root.modelMallVm ? root.modelMallVm.modelRating(root.modelMallVm.selectedModelIndex).toFixed(1) : "0.0"
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeMD
                            }
                            Text {
                                anchors.leftMargin: 8
                                anchors.verticalCenter: parent.verticalCenter
                                text: "(" + (root.modelMallVm ? root.modelMallVm.modelDownloads(root.modelMallVm.selectedModelIndex) : 0).toLocaleString() + " " + qsTr("downloads") + ")"
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSizeSM
                            }
                        }

                        // Price
                        Text {
                            text: root.modelMallVm
                                  ? (root.modelMallVm.modelFree(root.modelMallVm.selectedModelIndex)
                                     ? qsTr("Free")
                                     : ("\u00A5" + root.modelMallVm.modelPrice(root.modelMallVm.selectedModelIndex).toFixed(1)))
                                  : ""
                            color: root.modelMallVm && root.modelMallVm.modelFree(root.modelMallVm.selectedModelIndex) ? Theme.accent : Theme.statusWarning
                            font.pixelSize: Theme.fontSizeLG
                            font.bold: true
                        }

                        // Description
                        Text {
                            text: root.modelMallVm ? root.modelMallVm.modelDescription(root.modelMallVm.selectedModelIndex) : ""
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSM
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                            Layout.preferredHeight: 60
                        }

                        // Metadata grid
                        GridLayout {
                            columns: 2
                            columnSpacing: Theme.spacingLG
                            rowSpacing: Theme.spacingXS
                            Layout.fillWidth: true

                            Text { text: qsTr("File Format:"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS }
                            Text { text: root.modelMallVm ? root.modelMallVm.modelFileFormat(root.modelMallVm.selectedModelIndex) : ""; color: Theme.textPrimary; font.pixelSize: Theme.fontSizeXS }

                            Text { text: qsTr("File Size:"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS }
                            Text { text: formatFileSize(root.modelMallVm ? root.modelMallVm.modelFileSizeKB(root.modelMallVm.selectedModelIndex) : 0); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeXS }

                            Text { text: qsTr("Est. Print Time:"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS }
                            Text { text: root.modelMallVm ? root.modelMallVm.modelPrintTime(root.modelMallVm.selectedModelIndex) : ""; color: Theme.textPrimary; font.pixelSize: Theme.fontSizeXS }

                            Text { text: qsTr("Material:"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS }
                            Text { text: root.modelMallVm ? root.modelMallVm.modelMaterialUsage(root.modelMallVm.selectedModelIndex) : ""; color: Theme.textPrimary; font.pixelSize: Theme.fontSizeXS }
                        }

                        // Tags
                        Flow {
                            Layout.fillWidth: true
                            spacing: 4
                            Repeater {
                                model: {
                                    if (!root.modelMallVm) return 0;
                                    var tags = root.modelMallVm.modelTags(root.modelMallVm.selectedModelIndex);
                                    return tags ? tags.split(",").length : 0;
                                }
                                delegate: Rectangle {
                                    required property int index
                                    height: 20
                                    width: tagText.width + 14
                                    radius: 10
                                    color: Theme.bgPanel
                                    border.color: Theme.borderSubtle
                                    border.width: 1
                                    Text {
                                        id: tagText
                                        anchors.centerIn: parent
                                        text: {
                                            if (!root.modelMallVm) return "";
                                            var tags = root.modelMallVm.modelTags(root.modelMallVm.selectedModelIndex);
                                            return tags ? tags.split(",")[parent.index].trim() : "";
                                        }
                                        color: Theme.textSecondary
                                        font.pixelSize: 10
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        // Download button
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            radius: Theme.radiusMD
                            color: dlDetailBtn.containsMouse ? Theme.accent : Theme.accentSubtle
                            border.color: Theme.accent
                            border.width: 1

                            RowLayout {
                                anchors.centerIn: parent
                                spacing: 6
                                Text {
                                    text: root.modelMallVm && root.modelMallVm.modelFree(root.modelMallVm.selectedModelIndex)
                                          ? qsTr("\u2B07 Download Free")
                                          : qsTr("\u2B07 Purchase & Download")
                                    color: Theme.accent
                                    font.pixelSize: Theme.fontSizeMD
                                    font.bold: true
                                }
                                Text {
                                    text: root.modelMallVm && !root.modelMallVm.modelFree(root.modelMallVm.selectedModelIndex)
                                          ? ("\u00A5" + root.modelMallVm.modelPrice(root.modelMallVm.selectedModelIndex).toFixed(1))
                                          : ""
                                    color: Theme.accent
                                    font.pixelSize: Theme.fontSizeMD
                                    font.bold: true
                                }
                            }

                            HoverHandler { id: dlDetailBtn }
                            TapHandler {
                                onTapped: {
                                    if (root.modelMallVm) {
                                        root.modelMallVm.downloadModel(root.modelMallVm.selectedModelIndex)
                                        detailDialog.close()
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
