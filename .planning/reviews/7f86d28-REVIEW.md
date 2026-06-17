# Code Review — commit 7f86d28 (G1–G5 Prepare 页差距消除)

**审查日期：** 2026-06-17
**审查范围：** GLToolbars.qml / CollapsibleSection.qml / PreparePage.qml / LeftSidebar.qml
**审查方式：** Explore agent 深度审查 + 交叉验证（GLViewport.h / EditorViewModel.h / qml.qrc / main_qml.cpp）

---

## 概览

| 严重程度 | 数量 | 阻塞？ |
|---------|------|--------|
| Critical | 1 (C1) | 强烈建议修复 |
| Warning | 8 (W1-W8) | W2/W4/W6 建议本次修 |
| Info | 10 | 不阻塞 |

**用户关心的验证点全部通过：** 前向引用、required property 注入、枚举解析、v1.x 悬空引用——均无 Critical。

---

## Critical

### C1 — LeftSidebar.qml:19 PowerShell 转义符 `` `r`n `` 污染源码
三行属性（color/radius/border.width）被 `` `r`n `` 吞进同一行注释，`radius:0` 和 `border.width:0` 从未被解析。运行时碰巧无副作用（Rectangle 默认值就是 0），但源码污染必须清理，且需全仓排查。

## Warning

- **W1** GLToolbars overlay 覆盖区是"死区"（右键菜单不触发）——设计取舍，加注释说明
- **W2** GlyphButton/GizmoButton `enabled` 遮蔽 Item.enabled —— 改名 isEnabled
- **W3** GLToolbars.qml:80 Row 内 anchors.verticalCenter 无效 —— 删掉
- **W4** LeftSidebar.qml printerExpanded 双向写环隐患 —— 单一真源
- **W5** CollapsibleSection 折叠动画时序 —— contentContainer 去掉自带 Behavior
- **W6** Scale 和 Fit 按钮都用 "⤢" —— Fit 改用 "⊡"
- **W7** GLToolbars 硬编码颜色/尺寸未走 Theme —— 抽 Theme 令牌
- **W8** 内联 component 固定像素不响应 DPI —— 走 Theme fontSize

## Info

- **I1-I3** 前向引用/枚举/API 验证通过 ✅
- **I4** PreparePage v1.x 清理无悬空引用 ✅；ToolStripDivider 是死代码，建议删
- **I5** GlyphButton/GizmoButton 可合并为独立组件
- **I6** `tip` vs `toolTipText` 命名不统一
- **I7-I10** qsTr 覆盖良好；键盘可达性可接受（有快捷键替代）；可用 Repeater 减少 gizmo 代码

---

## 最优先修复（本次）

1. **C1** LeftSidebar.qml:19 拆三行 + 全仓排查 `` `r`n ``
2. **W2** GlyphButton/GizmoButton enabled 改名
3. **W4** printerExpanded 写环 → 单一真源
