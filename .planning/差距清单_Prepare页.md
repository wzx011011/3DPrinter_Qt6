# Prepare 页差距清单 — OWzxSlicer vs OrcaSlicer 上游

**生成时间：** 2026-06-17
**方法：** 截图对比（用户截图当前 OWzxSlicer + 之前 OrcaSlicer 上游图 + analyze_image 分析）
**用途：** 后续 UI 工作的真值驱动，替代"凭感觉标记完成"

---

## 🔴 P0 — 功能性 Bug（必须修）

### G1. Filament 耗材区块默认折叠，看起来"消失"
- **现象：** 截图里 Printer 之后直接是 Process，Filament 区块折叠了（`expanded: false`）
- **根因：** `LeftSidebar.qml:204` `filamentSection.expanded: false`
- **上游：** Filament 区块默认展开显示耗材列表
- **修复：** `expanded: false` → `expanded: true`

---

## 🟠 P1 — 布局/层次问题（视觉差很多的主因）

### G2. 左侧栏区块没有统一卡片背景，元素"揉在一起"
- **现象：** 截图里左侧栏各区块（Printer/Filament/Process/Search/Objects...）没有清晰的卡片分隔，文字和按钮挤在一起
- **上游：** 每个区块是独立的圆角卡片（深灰背景 + 圆角 + 微妙边框 + 区块间明显间距）
- **修复方向：** 给每个 CollapsibleSection / 区块统一卡片背景（bgElevated + radius 8 + 内边距），区块间 spacing 增大到 12-16

### G3. Printer 区块内部多个裸 Rectangle 堆叠，无统一容器
- **现象：** Printer section 是 `ColumnLayout { Rectangle{} Rectangle{} ... }`，内部标题栏/内容/喷嘴选择各自为政
- **上游：** Printer 是一个完整卡片，内部有结构但外层统一
- **修复方向：** 把 Printer section 包进一个卡片 Rectangle

### G4. 3D 视口工具栏 overlay ✅ 已消除（2026-06-17 用户确认"都有"）
- **状态：** ✅ 已达标
- **实现：** GLToolbars.qml（MainToolbar + Gizmos 竖条 + ViewToolbar 三合一 overlay）
- **验收：** 用户截图确认三个工具栏都显示
- **遗留：** G4-d PartPlateList（底部多板列表）未做

### G5. 右侧 Gizmo 竖条（processBar）样式老旧 ✅ 已消除
- **状态：** ✅ 已达标（processBar 已删，由 GLToolbars 的 Gizmos 竖条取代）
- **实现：** 删 processBar + GLToolbars Gizmos 竖条（13 个 gizmo + active 高亮）

---

## 🟡 P2 — 细节/打磨问题

### G6. 顶部标题栏 tab 样式和上游有差距
- **现象：** 我们的 tab 是"首页/准备/预览/设备..."文字按钮，绿色高亮当前
- **上游：** BBLTopbar 风格更精致（图标+文字、间距、配色不同）
- **状态：** Phase 2 做了基础，需视觉打磨

### G7. 左侧栏滚动条/边距
- **现象：** CxScrollView 的 margins 可能不够，内容贴边
- **修复方向：** 调整 anchors.margins

### G8. 整体配色对比度
- **现象：** 我们的配色偏暗，区块层次不明显
- **上游：** 区块背景和主背景有更明显的对比

---

## ✅ 已完成（确认 OK 的部分）

- 顶部 tab 导航存在（首页/准备/预览/设备/多设备/项目/校准）
- 品牌名是 OWzxSlicer（Phase 1 品牌清理生效）
- Prepare tab 当前激活（绿色高亮）
- 3D 视口能渲染热床网格
- 左侧栏八大区块代码层面都存在（但 Filament 折叠了 + 视觉层次差）

---

## 📋 修复优先级建议

**第一波（立即修，见效快）：**
1. G1 — Filament 默认展开（1 行改动）
2. G2 — 左侧栏统一卡片背景 + 间距（视觉层次核心）
3. G3 — Printer 区块包卡片容器

**第二波（Phase 6 核心，工作量大）：**
4. G4 — 3D 视口工具栏 overlay（MainToolbar/Gizmos/ViewToolbar/PartPlateList）
5. G5 — Gizmo 竖条重做

**第三波（打磨）：**
6. G6/G7/G8 — 标题栏/滚动条/配色细节

---

## 验收方式变更（重要）

**旧方式（导致虚报完成）：** 编译通过 + 冒烟不崩 + 零 warning → 标记完成
**新方式：** 实现后 → 用户截图 → analyze_image 对比本清单 → 逐项确认消除 → 才标记完成

后续所有视觉类工作必须走新方式。
