---
gsd_state_version: 1.0
milestone: v2.1
milestone_name: Slice & Preview Deep Dive
status: gap_driven
last_updated: 2026-06-17T12:00:00.000Z
last_activity: 2026-06-17 -- v2.1 milestone 启动（切片/预览深化）
progress:
  total_gaps: 15       # v2.1 差距总数（SLICE 8 + PRESET 4 + PREPARE 2 + SEARCH 1）
  resolved_gaps: 0
  pending_gaps: 15
stopped_at: v2.1 milestone 启动 — 待开始 SLICE-01 (G-code 着色模式切换)
---

# Project State

## Current Position

**Milestone:** v2.1 Slice & Preview Deep Dive
**工作模式：** 差距清单驱动（沿用 v2.0 后期）
**当前焦点：** SLICE-01 G-code 着色模式切换（preview 核心差异化）
**下一步：** SLICE-01 → SLICE-02 (IMSlider) → SLICE-03/04 (TickCode) → PRESET-01/02

## v2.0 已完成（架构层，作为 v2.1 地基）

- ✅ 9-Page Notebook + BBLTopbar 框架
- ✅ Plater 共享实例（Prepare/Preview 共享，viewMode 联动）
- ✅ Sidebar Dockable（折叠/宽度/dockArea + 持久化）
- ✅ 八大区块 + 卡片化
- ✅ GLToolbars overlay（MainToolbar/Gizmos竖条/ViewToolbar）
- ✅ v1.x 残留清理
- ✅ Prepare 页 G1-G5 差距消除（用户确认 + code review 通过）

详见 `.planning/差距清单_Prepare页.md`（v2.0 Prepare 页差距）和 `.planning/差距盘点_全局.md`（全局盘点）。

## v2.1 差距清单进度

| 需求域 | 差距数 | 已消除 | 说明 |
|---|---|---|---|
| SLICE（Preview TickCode/IMSlider）| 8 | 0 | preview 核心深化 |
| PRESET（Preset 管理 Dialog）| 4 | 0 | 预设管理套件 |
| PREPARE（G6/G8 打磨）| 2 | 0 | v2.0 遗留 |
| SEARCH（Settings 搜索）| 1 | 0 | SearchDialog 集成 |

## 验收方式（沿用）

**视觉类：** 实现 → 构建 → 用户截图 → 确认 → ✅
**功能类：** 实现 → 构建 → 用户操作确认 → ✅
**agent 不自行标记视觉/功能类完成**

## 关键文件索引（v2.1 工作区）

- Preview：`src/qml_gui/pages/PreviewPage.qml` (302行) + `src/core/rendering/GCodeRenderer.*`
- 上游参考：`third_party/OrcaSlicer/src/slic3r/GUI/GCodeViewer.cpp` / `IMSlider.cpp` / `TickCode.cpp`
- Preset Dialog：`src/qml_gui/dialogs/` + 上游 `SavePresetDialog.cpp` / `UnsavedChangesDialog.cpp`
