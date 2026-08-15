import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// ObjColorDialog.qml — Phase 236 (DLG-03) OBJ mtl color -> extruder mapping.
//
// Upstream: when an OBJ with a multi-color .mtl is imported, the
// ObjImportColorDialog lists the material colors and lets the user assign an
// extruder; the chosen extruder is applied to the new object.
//
// OWzx data flow:
//   ProjectServiceMock::objMtlColors parses the sibling .mtl (Kd lines) ->
//   EditorViewModel::pendingObjColors + objColorMappingRequested ->
//   this dialog -> applyPendingObjColors(extruderId) sets every volume of the
//   pending object to the chosen extruder.
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    closePolicy: Popup.NoAutoClose
    dialogTitle: qsTr("OBJ 颜色分配")
    width: 420
    height: 360
    padding: 0

    required property var editorVm

    property string targetObjectName: ""
    property int extruderCount: 4
    property int selectedExtruder: 0

    signal applyRequested(int extruderId)

    readonly property var colors: root.editorVm ? root.editorVm.pendingObjColors : []

    function extruderLabel(i) {
        return qsTr("挤出机 %1").arg(i + 1)
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacingMD
        anchors.margins: Theme.spacingXL

        Text {
            Layout.fillWidth: true
            text: root.targetObjectName !== ""
                ? qsTr("模型“%1”的材质文件包含 %2 种颜色。请选择用于打印的挤出机：")
                    .arg(root.targetObjectName).arg(root.colors.length)
                : qsTr("模型的材质文件包含 %1 种颜色。请选择用于打印的挤出机：").arg(root.colors.length)
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeMD
            wrapMode: Text.WordWrap
        }

        // Color swatches (read-only; the whole object maps onto one extruder —
        // per-color extruder assignment arrives with the per-extruder model)
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSM

            Repeater {
                model: Math.min(root.colors.length, 8)

                Rectangle {
                    required property int index
                    width: 28
                    height: 28
                    radius: 4
                    color: root.colors[index] !== undefined ? root.colors[index] : Theme.bgCard
                    border.color: Theme.borderDefault
                    border.width: 1
                    ToolTip.visible: swatchHover.containsMouse
                    ToolTip.text: root.colors[index] !== undefined ? root.colors[index] : ""
                    HoverHandler { id: swatchHover }
                }
            }

            Text {
                visible: root.colors.length > 8
                text: qsTr("+%1").arg(root.colors.length - 8)
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeXS
            }

            Item { Layout.fillWidth: true }
        }

        // Extruder picker
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.bgInset
            radius: 4
            border.color: Theme.borderSubtle
            border.width: 1
            clip: true

            ListView {
                anchors.fill: parent
                anchors.margins: Theme.spacingXS
                clip: true
                model: root.extruderCount
                spacing: 2
                currentIndex: root.selectedExtruder

                delegate: Rectangle {
                    required property int index
                    width: parent.width
                    height: 32
                    radius: 3
                    color: extruderList.currentIndex === index ? Theme.accentSubtle
                           : (extruderHover.containsMouse ? Theme.bgHover : "transparent")

                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: Theme.spacingSM
                        spacing: Theme.spacingSM

                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 16
                            height: 16
                            radius: 8
                            color: extruderList.currentIndex === index ? Theme.accent : Theme.bgCard
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: root.extruderLabel(index)
                            color: extruderList.currentIndex === index ? Theme.textPrimary : Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSM
                        }
                    }

                    HoverHandler { id: extruderHover }
                    TapHandler {
                        onTapped: {
                            root.selectedExtruder = index
                            extruderList.currentIndex = index
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight
            spacing: Theme.spacingMD

            CxButton {
                text: qsTr("取消")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.reject()
            }
            CxButton {
                text: qsTr("确定")
                cxStyle: CxButton.Style.Primary
                onClicked: {
                    root.applyRequested(root.selectedExtruder)
                    root.accept()
                }
            }
        }
    }
}
