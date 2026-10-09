import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// D4 -- AboutDialog: version + build info + heritage narrative + license +
//       open-source components list (36 upstream entries) + website link.
// Usage: AboutDialog { id: aboutDlg }  ->  aboutDlg.open()
// PreferencesPage "About" category triggers
//
// Phase 236 (DLG-04): license corrected to AGPL-3.0. Upstream AboutDialog.cpp
// :148-156 states: "Orca Slicer is licensed under GNU Affero General Public
// License, version 3" + "Orca Slicer is based on PrusaSlicer and BambuStudio"
// + an open-source components list (m_entries). OWzx mirrors the same
// structure — the previous Lesser-GPL license text was factually wrong for
// an OrcaSlicer-derived work.
//
// Phase 252 (DLG-04b, U14): structure aligned to upstream AboutDialog.cpp —
//  - 36-entry m_entries list verbatim (AboutDialog.cpp:86-122, Admesh..zlib);
//    QML-only non-upstream items OpenCV/assimp removed. The upstream
//    "License Info" popup (CopyrightsDialog) is deferred to a later batch —
//    the entries render inline here until then.
//  - Website link https://www.orcaslicer.com (AboutDialog.cpp:334), opened
//    via Qt.openUrlExternally, link color uses the R1 accent token
//    (upstream #009789 semantic slot = R1 green).
//  - Three-paragraph heritage narrative (AboutDialog.cpp:270-272).
//  - Build line bound to backend.systemInfo().buildCommit (U04 contract),
//    hidden when empty — upstream "Build %s" (AboutDialog.cpp:249).
//  - Copyright(C) 2026 OWzx line (AboutDialog.cpp:315 format).
//  - The former 7-row static info table: fake values ("6.10.0", "Debug",
//    "2026-03-03", ...) replaced with systemInfo() truth (qtVersion /
//    platform / buildDate / appVersion) or removed — upstream has no such
//    table.
// Defer: brand logo asset (current emoji placeholder), License Info popup.
CxDialog {
    id: root

    readonly property var libEntries: [
        { name: "Admesh",                                   url: "https://admesh.readthedocs.io/" },
        { name: "Anti-Grain Geometry",                      url: "http://antigrain.com" },
        { name: "ArcWelderLib",                             url: "https://plugins.octoprint.org/plugins/arc_welder" },
        { name: "Boost",                                    url: "http://www.boost.org" },
        { name: "Cereal",                                   url: "http://uscilab.github.io/cereal" },
        { name: "CGAL",                                     url: "https://www.cgal.org" },
        { name: "Clipper",                                  url: "http://www.angusj.co" },
        { name: "libcurl",                                  url: "https://curl.se/libcurl" },
        { name: "Draco",                                    url: "https://google.github.io/draco/" },
        { name: "Eigen3",                                   url: "http://eigen.tuxfamily.org" },
        { name: "Expat",                                    url: "http://www.libexpat.org" },
        { name: "fast_float",                               url: "https://github.com/fastfloat/fast_float" },
        { name: "GLAD (Multi-Language GL Loader-Generator)", url: "https://github.com/Dav1dde/glad" },
        { name: "GLFW",                                     url: "https://www.glfw.org" },
        { name: "GNU gettext",                              url: "https://www.gnu.org/software/gettext" },
        { name: "ImGUI",                                    url: "https://github.com/ocornut/imgui" },
        { name: "ImGuizmo",                                 url: "https://github.com/CedricGuillemet/ImGuizmo" },
        { name: "Libigl",                                   url: "https://libigl.github.io" },
        { name: "libnest2d",                                url: "https://github.com/tamasmeszaros/libnest2d" },
        { name: "lib_fts",                                  url: "https://www.forrestthewoods.com" },
        { name: "Mesa 3D",                                  url: "https://mesa3d.org" },
        { name: "Miniz",                                    url: "https://github.com/richgel999/miniz" },
        { name: "Nanosvg",                                  url: "https://github.com/memononen/nanosvg" },
        { name: "nlohmann/json",                            url: "https://json.nlohmann.me" },
        { name: "Qhull",                                    url: "http://qhull.org" },
        { name: "Open Cascade",                             url: "https://www.opencascade.com" },
        { name: "OpenGL",                                   url: "https://www.opengl.org" },
        { name: "PoEdit",                                   url: "https://poedit.net" },
        { name: "PrusaSlicer",                              url: "https://www.prusa3d.com" },
        { name: "Real-Time DXT1/DXT5 C compression library", url: "https://github.com/Cyan4973/RygsDXTc" },
        { name: "SemVer",                                   url: "https://semver.org" },
        { name: "Shinyprofiler",                            url: "https://code.google.com/p/shinyprofiler" },
        { name: "SuperSlicer",                              url: "https://github.com/supermerill/SuperSlicer" },
        { name: "TBB",                                      url: "https://www.intel.cn/content/www/cn/zh/developer/tools/oneapi/onetbb.html" },
        { name: "wxWidgets",                                url: "https://www.wxwidgets.org" },
        { name: "zlib",                                     url: "http://zlib.net" }
    ]

    // systemInfo() truth (BackendContext::systemInfo, BackendContext.cpp:898-956);
    // buildCommit arrives via the U04 contract — absent key resolves to "" and
    // the Build row hides.
    property var sysInfo: ({})
    readonly property string buildCommit: (sysInfo.buildCommit || "") + ""

    onAboutToShow: {
        sysInfo = (typeof backend !== "undefined" && backend && backend.systemInfo)
            ? backend.systemInfo() : {}
    }

    dialogTitle: qsTr("关于 OWzx")
    titleIcon: "❓"
    showCloseButton: true

    anchors.centerIn: parent
    width:  560   // upstream panel FromDIP(560), AboutDialog.cpp:218
    height: contentCol.implicitHeight + 80

    contentItem: ColumnLayout {
        id: contentCol
        width: root.width - 32
        spacing: Theme.spacingLG
        // Logo + product name + real version/build strings
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.spacingSM
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 64; height: 64; radius: Theme.radiusXL
                color: Theme.chromeSurface
                border.color: Theme.accent; border.width: 2
                Text { anchors.centerIn: parent; text: "🖨"; font.pixelSize: Theme.fontSizeDisplay }
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "OWzx Slicer"
                color: Theme.textPrimary; font.pixelSize: Theme.fontSizeXXL; font.bold: true
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                visible: (root.sysInfo.appVersion || "") !== ""
                text: qsTr("版本") + " " + (root.sysInfo.appVersion || "")
                color: Theme.accent; font.pixelSize: Theme.fontSizeSM
            }

            // Build commit — upstream "Build %s" (AboutDialog.cpp:249); the
            // U04 contract injects the key, empty/absent hides the row.
            Text {
                Layout.alignment: Qt.AlignHCenter
                visible: root.buildCommit !== ""
                text: qsTr("Build %1").arg(root.buildCommit)
                color: Theme.textTertiary; font.pixelSize: Theme.fontSizeXS
            }
        }

        // Heritage narrative — three paragraphs, upstream AboutDialog.cpp:270-272
        // (Body_12, secondary gray, wrapped). Replaces the former single
        // condensed sentence.
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSM

            Text {
                Layout.fillWidth: true
                text: qsTr("开源切片器的今天建立在协作与署名的传统之上。Slic3r 由 Alessandro Ranellucci 与 RepRap 社区创建，奠定了基础；Prusa Research 的 PrusaSlicer 在其上继续构建，Bambu Studio 从 PrusaSlicer 分叉而来，SuperSlicer 又以社区驱动的增强对其加以扩展。每个项目都承接前人的成果，并将其发扬光大。")
                color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD
                wrapMode: Text.WordWrap
                lineHeight: 1.4
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("OrcaSlicer 秉承同样的精神起步，汲取自 PrusaSlicer、BambuStudio、SuperSlicer 与 CuraSlicer。但它早已远远超越起源——引入了先进的校准工具、精确的壁面与接缝控制，以及数百项其他特性。")
                color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD
                wrapMode: Text.WordWrap
                lineHeight: 1.4
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("如今，OrcaSlicer 已是 3D 打印社区中使用最广泛、开发最活跃的开源切片器，它的许多创新被其他切片器采纳，成为推动整个行业的力量。")
                color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD
                wrapMode: Text.WordWrap
                lineHeight: 1.4
            }
        }

        // Runtime info table — values bound to backend.systemInfo() truth
        // (upstream has no such table; the former static fake rows
        // "6.10.0"/"V4 / JavaScript"/"Debug"/"Windows x64 (MSVC)"/
        // "2026-03-03" were removed or rebound).
        Rectangle {
            Layout.fillWidth: true; radius: Theme.radiusMD; color: Theme.bgSurface; border.color: Theme.bgCard; height: infoCols.implicitHeight + 16

            ColumnLayout {
                id: infoCols
                anchors.fill: parent; anchors.margins: Theme.spacingMD; spacing: Theme.spacingSM
                component InfoRow: RowLayout {
                    required property string label
                    required property string value
                    Layout.fillWidth: true; spacing: Theme.spacingXS
                    Text { text: parent.label; color: Theme.textDisabled; font.pixelSize: Theme.fontSizeSM; Layout.preferredWidth: 120 }
                    Text { text: parent.value; color: Theme.chromeText; font.pixelSize: Theme.fontSizeSM; Layout.fillWidth: true; elide: Text.ElideRight }
                }

                InfoRow { label: qsTr("Qt 版本");     value: root.sysInfo.qtVersion || "" }
                InfoRow { label: qsTr("目标平台");    value: root.sysInfo.platform || "" }
                InfoRow { label: qsTr("构建日期");    value: root.sysInfo.buildDate || "" }
                InfoRow { label: qsTr("开源协议");    value: qsTr("GNU Affero General Public License v3 (AGPL-3.0)") }
            }
        }

        // Open-source components — upstream m_entries, AboutDialog.cpp:86-122
        // (rendered inline; the standalone "License Info" popup is deferred).
        Rectangle {
            Layout.fillWidth: true; radius: Theme.radiusMD; color: Theme.bgInset; height: libsCol.implicitHeight + 16

            ColumnLayout {
                id: libsCol
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: parent.top; anchors.margins: Theme.spacingMD
                spacing: Theme.spacingXS

                Text {
                    text: qsTr("开源组件")
                    color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM; font.bold: true
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("本软件使用的开源组件的版权及其他专有权利归其各自所有者：")
                    color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS
                    wrapMode: Text.WordWrap
                    lineHeight: 1.4
                }

                CxScrollView {
                    id: libsScroll
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(libsList.implicitHeight, 190)
                    clip: true

                    Column {
                        id: libsList
                        x: 0; y: 0
                        width: libsScroll.availableWidth
                        spacing: Theme.spacingXXS

                        Repeater {
                            model: root.libEntries
                            delegate: RowLayout {
                                width: libsList.width
                                spacing: Theme.spacingMD
                                Text {
                                    text: modelData.name
                                    color: Theme.textPrimary; font.pixelSize: Theme.fontSizeXS
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: modelData.url
                                    color: Theme.accent; font.pixelSize: Theme.fontSizeXS
                                    Layout.maximumWidth: libsList.width * 0.55
                                    elide: Text.ElideMiddle
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: Qt.openUrlExternally(modelData.url)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Divider
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.bgCard }

        // Footer — upstream bottom-left copyright + website link
        // (AboutDialog.cpp:311-341); confirm button kept on the right.
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingXS

                Text {
                    // untranslated upstream (AboutDialog.cpp:315 plain string)
                    text: "Copyright(C) 2026 OWzx All Rights Reserved"
                    color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS
                }

                Text {
                    text: "https://www.orcaslicer.com"
                    color: Theme.accent; font.pixelSize: Theme.fontSizeSM
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Qt.openUrlExternally("https://www.orcaslicer.com")
                    }
                }
            }

            // Confirm button
            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                width: 100; height: 30; radius: Theme.radiusSM
                color: okHov.containsMouse ? Theme.accentDark : Theme.accentSubtle
                Text { anchors.centerIn: parent; text: qsTr("确认"); color: "white"; font.pixelSize: Theme.fontSizeMD; font.bold: true }
                MouseArea { id: okHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.close() }
            }
        }
    }
}
