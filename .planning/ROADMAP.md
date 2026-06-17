# Roadmap: Milestone v2.0 — OrcaSlicer UI Full Restoration

## Overview

把 Qt6/QML 项目的 UI 从 CrealityPrint 时代全面迁移对齐到 OrcaSlicer 上游。

**⚠️ 工作方式变更（2026-06-17）：** 废弃 Phase 线性推进。改为**差距清单驱动**——以"页面 × 区域"的视觉差距为工作单元，每个差距有上游截图真值 + 用户截图验收，做完一项用户确认消除才算完成。

---

## 为什么废弃 Phase 推进

旧方式（Phase 1-7 线性推进）的三个致命问题：
1. **验收标准太弱**——编译通过 + 不崩就标记完成，导致 Phase 3/4/5 虚报完成（用户截图后才发现"差很多"）
2. **Phase 划分割裂视觉**——Prepare 页的差距横跨多个 Phase，按 Phase 切反而难验收
3. **缺少视觉真值**——Success Criteria 是文字，没有上游截图对照，agent 按文字理解做，做出来和上游观感差十万八千里

新方式（差距清单驱动）：
1. **工作单元 = 消除一个差距**（如"左侧栏卡片背景""3D 视口 MainToolbar"）
2. **每个差距有上游截图真值**（`docs/upstream-reference/`）
3. **验收 = 用户截图 + analyze_image 对比**，不再凭"编译通过"

---

## 已沉淀的架构基础（Phase 1-5，作为后续地基）

这些架构改动保留，作为视觉对齐的地基：

| 已就绪 | 说明 | 视觉状态 |
|---|---|---|
| 品牌清理（Phase 1） | Creality→OWzx 全替换 | ✅ 视觉生效 |
| 9-Page Notebook + BBLTopbar（Phase 2） | 顶部 tab 框架 | 🟡 框架在，样式待打磨 |
| Plater 共享实例（Phase 3） | Prepare/Preview 共享单 Plater | ✅ 架构就绪 |
| Sidebar Dockable（Phase 4） | 折叠/宽度/dockArea + 持久化 | 🟡 功能在，视觉待对齐 |
| 八大区块骨架（Phase 5） | Printer/Filament/Process/Search/Objects/Settings/Layers/Params | 🟡 骨架在，视觉层次差 |

**这些不算"白做"**——架构是对的，只是视觉层没对齐上游。后续差距消除建立在这些地基上。

---

## 差距清单（真值驱动，按页面组织）

### Prepare 页（优先级最高）

**上游真值截图：** `docs/upstream-reference/prepare_page.png`（待用户提供）
**差距清单详情：** `docs/差距清单_Prepare页.md`

| ID | 差距 | 优先级 | 状态 | 验收方式 |
|---|---|---|---|---|
| **G1** | Filament 耗材区块默认折叠（应展开） | P0 | 🔴 待修 | 截图确认 Filament 可见 |
| **G2** | 左侧栏区块无统一卡片背景，元素揉一起 | P1 | 🔴 待修 | 截图确认卡片式层次 |
| **G3** | Printer 区块内部裸 Rectangle 堆叠，无统一容器 | P1 | 🔴 待修 | 截图确认 Printer 卡片 |
| **G4** | 3D 视口完全空旷，无 MainToolbar/Gizmos/ViewToolbar/PartPlateList overlay | P1 | 🔴 待修（大工作量） | 截图确认工具栏 overlay |
| **G5** | 右侧 Gizmo 竖条（processBar）样式老旧 | P2 | 🔴 待修 | 截图确认样式对齐 |
| **G6** | 顶部 BBLTopbar tab 样式与上游有差距 | P2 | 🔴 待修 | 截图确认 |
| **G7** | 左侧栏滚动条/边距问题 | P2 | 🔴 待修 | 截图确认 |
| **G8** | 整体配色对比度（区块层次不明显） | P2 | 🔴 待修 | 截图确认 |

### Preview 页

**上游真值截图：** `docs/upstream-reference/preview_page.png`（待用户提供）
**差距清单：** 待生成（用户截图后对比）

### Device/Monitor 页

**上游真值截图：** `docs/upstream-reference/device_page.png`（待用户提供）
**差距清单：** 待生成

### 其他页面（Project/Calibration/MultiDevice/Home）

待用户截图后逐页生成差距清单。

---

## 工作流程（新）

```
1. 选一个差距（如 G1）
2. 实现/修复
3. 构建 + 冒烟（编译通过 + 不崩）
4. 启动程序 → 用户截图
5. analyze_image 对比上游真值 + 差距清单
6. 用户确认"消除" → 标记 ✅
7. 用户确认"未消除/有新问题" → 回到步骤 2
```

**关键约束（agent 自律）：**
- 我（agent）**不得自行标记视觉类差距为"完成"**
- 视觉类差距必须用户截图确认后才标记 ✅
- 只有纯 C++ 逻辑/架构类工作（无视觉影响）可由 agent 标记完成（仍需构建+单测通过）

---

## "对齐"的验收定义（重要，避免误判）

**目标不是"像素级 1:1 还原"**（上游 wxWidgets+ImGui 与我们的 Qt6/QML 是不同框架，像素级复制不现实）。
**目标是"功能等价 + 视觉不违和"**。具体验收标准：

### ✅ 算"对齐"的标准（达标）
- **结构对齐**：区块顺序、元素组成、按钮列表与上游一致（有什么元素、怎么排列）
- **功能等价**：点击有正确反应、该跳转的跳转、状态切换正确
- **配色与层次协调**：主色调接近、区块层次清晰、无明显拥挤/违和
- **用户指出的差距已消除**：截图指出的具体问题得到解决

### ❌ 不追求的标准（明确放弃，避免无谓消耗）
- 像素级完全一致（跨框架做不到）
- 完全相同的字体渲染/抗锯齿
- 完全相同的 3D 渲染质感（取决于 GL 渲染层，独立工作）
- 完全相同的动画/过渡手感
- ImGui 特有的逐像素绘制效果

### 验收流程
1. agent 实现差距修复 → 构建 + 冒烟通过
2. agent 启动程序 → 用户截图
3. agent 用 analyze_image 对比上游截图 + 差距清单
4. **用户判断**："结构对齐了、不违和" → 标记 ✅；"还是乱/缺东西" → 回到步骤 1

**决策权在用户**：agent 只负责实现和构建，"是否达标"由用户拍板。

---

## 优先级排序（当前）

**第一波（Prepare 页左侧栏，见效快）：**
1. G1 — Filament 默认展开（1 行改动）
2. G2 — 左侧栏统一卡片背景 + 间距
3. G3 — Printer 区块包卡片容器

**第二波（Prepare 页 3D 视口，大工作量）：**
4. G4 — 3D 视口工具栏 overlay（MainToolbar/Gizmos/ViewToolbar/PartPlateList）
5. G5 — Gizmo 竖条重做

**第三波（打磨 + 其他页面）：**
6. G6/G7/G8 — Prepare 页细节
7. Preview/Device/其他页 — 待截图生成清单

---

## Out of Scope（沿用 v2.1/v2.2 划分）

- **v2.1**：Home WebView / Device 双形态 / Multi-device / Project 5 子 Tab / ConfigWizard 完整版 / PreferencesDialog 完整版
- **v2.2**：AMS Dialog / 预设管理 Dialog / Print/PrintHost Dialog / 网络 Dialog / i18n 21 语言

---

## 历史归档说明

旧的 Phase 1-7 计划文档保留在 `.planning/phases/01-05` 作为历史参考（记录了架构改动的来龙去脉），但**不再作为工作驱动**。新的驱动是本文件的差距清单 + `docs/差距清单_*.md`。

旧 ROADMAP 的 Phase 6（GLCanvas Toolbar）和 Phase 7（Calibration Dialog）的 Success Criteria 仍可参考，但会转化为差距清单条目（如 G4 对应旧 Phase 6 的 GLUI-01~08）。

---
*Last updated: 2026-06-17 — 废弃 Phase 推进，改为差距清单驱动；Prepare 页 G1-G8 首批差距已生成；验收方式变更（视觉类必须用户截图确认）*
