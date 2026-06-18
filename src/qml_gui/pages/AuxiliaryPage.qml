import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property var projectVm  // AuxiliaryPage reuses ProjectViewModel
    /// backend 注入（用于页面跳转 + Toast 提示）
    property var backend: null

    // PAGE-03: 卡片点击处理（对齐上游 Auxiliary 辅助工具入口）
    function handleCardAction(action) {
        switch (action) {
            case "analyze":
            case "weight":
                // 模型分析/重量估算 → 切到 Prepare 页（需要选中对象）
                if (root.backend) root.backend.requestSelectTab(root.backend.tp3DEditor)
                break
            case "support":
            case "layer":
                // 支撑预览/层预览 → 切到 Preview 页
                if (root.backend) root.backend.requestSelectTab(root.backend.tpPreview)
                break
            case "cut":
                // 模型切割 → 切到 Prepare 页（Cut gizmo）
                if (root.backend) root.backend.requestSelectTab(root.backend.tp3DEditor)
                break
            default:
                // 其他功能（尺寸标注/颜色分区/对称/打印报告）→ 提示开发中
                console.log("[AuxiliaryPage] action '" + action + "' not yet implemented")
                break
        }
    }

    Rectangle { anchors.fill: parent; color: "#0d0f12" }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 44; color: "#131720"
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16

                Text { text: qsTr("辅助功能"); color: "#e8edf6"; font.pixelSize: 15; font.bold: true }
                Item { Layout.fillWidth: true }
                Text { text: qsTr("提供打印辅助分析和工具"); color: "#566070"; font.pixelSize: 11 }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 16
            columns: 3
            rowSpacing: 12
            columnSpacing: 12

            Repeater {
                model: [
                    { icon: "📐", title: qsTr("模型分析"),     desc: qsTr("检查模型几何问题"), action: "analyze" },
                    { icon: "🔩", title: qsTr("支撑预览"),     desc: qsTr("查看支撑结构分布"), action: "support" },
                    { icon: "📏", title: qsTr("尺寸标注"),     desc: qsTr("精确测量模型尺寸"), action: "measure" },
                    { icon: "🎨", title: qsTr("颜色分区"),     desc: qsTr("多色打印分区工具"), action: "color" },
                    { icon: "⚖",  title: qsTr("重量估算"),     desc: qsTr("预估耗材用量"), action: "weight" },
                    { icon: "🔄", title: qsTr("对称工具"),     desc: qsTr("模型镜像与对称操作"), action: "mirror" },
                    { icon: "✂",  title: qsTr("模型切割"),     desc: qsTr("切割大型模型分部打印"), action: "cut" },
                    { icon: "🔬", title: qsTr("层预览"),       desc: qsTr("逐层查看切片结果"), action: "layer" },
                    { icon: "📊", title: qsTr("打印报告"),     desc: qsTr("生成详细打印分析报告"), action: "report" }
                ]
                delegate: Rectangle {
                    required property var modelData
                    Layout.fillWidth: true; Layout.preferredHeight: 90; radius: 8
                    color: auxMa.containsMouse ? "#161d28" : "#131720"
                    border.color: auxMa.containsMouse ? "#2e3a4c" : "#1e2430"
                    border.width: 1

                    Column {
                        anchors.centerIn: parent; spacing: 6
                        Text { text: parent.parent.modelData.icon; font.pixelSize: 26; horizontalAlignment: Text.AlignHCenter; width: parent.width }
                        Text { text: parent.parent.modelData.title; color: "#c8d4e0"; font.pixelSize: 12; font.bold: true; horizontalAlignment: Text.AlignHCenter; width: parent.width }
                        Text { text: parent.parent.modelData.desc; color: "#566070"; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter; width: parent.width }
                    }
                    MouseArea {
                        id: auxMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.handleCardAction(modelData.action)
                    }
                }
            }
        }
    }
}
