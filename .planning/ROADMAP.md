# Roadmap: Milestone v2.1 — Slice & Preview Deep Dive

## Overview

深化切片预览体验 + 完善预设管理。集中在 Prepare/Preview/Settings 已有页面深化，不碰 Mock 服务。

**工作方式**（沿用 v2.0 后期确立）：
- **差距清单驱动**（不按 Phase 线性推进）
- **视觉类必须用户截图验收**（agent 不自行标记完成）
- **目标"功能等价 + 视觉不违和"**（不追求像素级 1:1）

---

## v2.0 已沉淀的架构基础（v2.1 地基）

v2.0 完成了 UI 架构对齐，作为 v2.1 深化的地基：

| 已就绪 | 说明 |
|---|---|
| 9-Page Notebook + BBLTopbar | 顶部 tab 框架 ✅ |
| Plater 共享实例 | Prepare/Preview 共享，viewMode 联动 ✅ |
| Sidebar Dockable | 折叠/宽度/dockArea + 持久化 ✅ |
| 八大区块 + 卡片化 | Printer/Filament/Process/Search/Objects/Settings/Layers/Params ✅ |
| GLToolbars overlay | MainToolbar/Gizmos竖条/ViewToolbar ✅ |
| v1.x 清理 | RightParamsPanel/topTools/viewPresets/processBar 删除 ✅ |

---

## v2.1 差距清单（真值驱动，按需求域组织）

### SLICE — Preview TickCode/IMSlider 系统（核心）

**上游真值：** `third_party/OrcaSlicer/src/slic3r/GUI/GCodeViewer.cpp` (4723行) + `IMSlider.cpp` (1828行) + `TickCode.cpp` (204行)

| ID | 差距 | 优先级 | 状态 | 验收 |
|---|---|---|---|---|
| **SLICE-01** | G-code 着色模式切换（6种：Feature/Speed/Extruder/LayerHeight/Pressure/Pixel） | P0 | 🔴 待做 | 截图确认 6 模式切换 |
| **SLICE-02** | IMSlider 层滑块增强（刻度标记+拖拽手感） | P0 | 🔴 待做 | 截图确认刻度+操作确认拖拽 |
| **SLICE-03** | TickCode 自定义刻度插入（change filament/pause/custom gcode） | P1 | 🔴 待做 | 操作确认插入流程 |
| **SLICE-04** | CustomGcodeDialog 联动 slider（dialog 已存在） | P1 | 🔴 待做 | 操作确认联动 |
| **SLICE-05** | 切片冲突热区点击 | P2 | 🔴 待做（评估） | — |
| **SLICE-06** | Legend 动态更新（跟随着色模式） | P2 | 🔴 待做 | 截图确认 |
| **SLICE-07** | ToolPositionTooltip 增强 | P2 | 🔴 待做 | 截图确认 |
| **SLICE-08** | 键盘导航对齐上游 IMSlider | P2 | 🔴 待做 | 操作确认 |

### PRESET — Preset 管理 Dialog 套件

**上游真值：** SavePresetDialog (18KB) / UnsavedChangesDialog (95KB) / ExportPresetBundleDialog (23KB) / CreatePresetsDialog (264KB)

| ID | 差距 | 优先级 | 状态 | 验收 |
|---|---|---|---|---|
| **PRESET-01** | SavePresetDialog（保存当前参数为预设） | P0 | 🔴 待做 | 操作确认保存流程 |
| **PRESET-02** | UnsavedChangesDialog（切换 preset/project 守卫） | P1 | 🔴 待做 | 操作确认守卫触发 |
| **PRESET-03** | ExportPresetBundleDialog（导出预设包） | P2 | 🔴 待做 | 操作确认导出 |
| **PRESET-04** | CreatePresetsDialog（批量创建，264KB 巨型，评估后可能延后） | P2 | 🔴 待评估 | — |

### PREPARE — Prepare 页打磨收尾（v2.0 遗留 G6/G8）

| ID | 差距 | 优先级 | 状态 | 验收 |
|---|---|---|---|---|
| **PREPARE-01** | BBLTopbar 样式打磨（G6） | P1 | 🔴 待做 | 截图确认 |
| **PREPARE-02** | 配色对比度（G8，Theme 调整） | P2 | 🔴 待做 | 截图确认 |

### SEARCH — Settings Search 集成

| ID | 差距 | 优先级 | 状态 | 验收 |
|---|---|---|---|---|
| **SEARCH-01** | SearchDialog 接入 SettingsPage tier | P1 | 🔴 待做 | 操作确认跳转 |

---

## 优先级排序（v2.1 推进顺序）

**第一波（Preview 核心，SLICE-01/02）：**
1. SLICE-01 — G-code 着色模式切换（preview 最有价值的差异化）
2. SLICE-02 — IMSlider 增强（刻度+拖拽）

**第二波（刻度系统，SLICE-03/04）：**
3. SLICE-03 — TickCode 刻度插入
4. SLICE-04 — CustomGcodeDialog 联动

**第三波（Preset 管理，PRESET-01/02）：**
5. PRESET-01 — SavePresetDialog
6. PRESET-02 — UnsavedChangesDialog

**第四波（打磨 + 集成）：**
7. PREPARE-01 — BBLTopbar 打磨
8. SEARCH-01 — Settings Search 集成
9. SLICE-05~08 / PRESET-03/04 / PREPARE-02 — 按优先级补

---

## 工作流程（每个差距）

```
1. 选一个差距
2. 对照上游源码（GCodeViewer.cpp/IMSlider.cpp/TickCode.cpp/SavePresetDialog 等）
3. 实现/修复（用 Edit 工具，不用 PowerShell 行操作）
4. 构建 + 冒烟（编译通过 + 不崩 + 零 QML warning）
5. 启动程序 → 用户截图（视觉类）/ 用户操作确认（功能类）
6. 达标 → 标记 ✅；未达标 → 回到步骤 3
```

**agent 自律**：视觉/功能类差距**绝不自行标记完成**，必须用户确认。

---

## Out of Scope (v2.1 明确不做)

- Device/Cloud/Network/Calibration 真实化（v2.2+）
- AssembleView / PartPlate 完整（v2.2+）
- ModelMall WebView / Home WebView（v2.2+，需 QtWebEngine）
- i18n 翻译内容填充（v2.2+）
- 90+ Dialog 补全的剩余部分（分散到各 milestone）

---

## v2.0 历史归档（仅供追溯）

v2.0 的 Phase 1-5 plan 文档在 `.planning/phases/01-05`，记录架构改动来龙去脉。v2.0 完成的差距清单（G1-G8）在 `.planning/差距清单_Prepare页.md`。v2.0 的全局差距盘点在 `.planning/差距盘点_全局.md`。

---
*Last updated: 2026-06-17 — v2.1 (Slice & Preview Deep Dive) 启动；差距清单 SLICE/PRESET/PREPARE/SEARCH 共 15 项*
