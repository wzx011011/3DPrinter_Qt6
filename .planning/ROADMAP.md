# Roadmap: Milestone v2.2 — Page Completion & Cleanup

## Overview

把低完成度页面提到可用状态 + 清理冗余 + 收尾 v2.1 延后项。集中在 UI 补全，不引入新的复杂集成。

**工作方式**（沿用）：差距清单驱动 + 视觉类用户截图验收 + 功能类构建+冒烟通过。

---

## v2.1 已完成（地基）

v2.1 完成了切片/预览深化 + 预设管理 + 搜索 + 配色。详见 commit 346c450 + 85e2b5c。

---

## v2.2 差距清单

### PAGE — 页面补全（核心）

| ID | 差距 | 优先级 | 状态 | 验收 |
|---|---|---|---|---|
| **PAGE-01** | ProjectPage 6 按钮接线（新建/打开/保存/另存/导入/导出） | P0 | 🔴 待做 | 操作确认每个按钮有效 |
| **PAGE-02** | DeviceListPage 修复强制写空 bug + 完整设备展示 | P0 | 🔴 待做 | 设备列表非空 |
| **PAGE-03** | AuxiliaryPage 9 卡片接线（模型分析/支撑预览/重量估算） | P1 | 🔴 待做 | 操作确认卡片可点 |
| **PAGE-04** | HomePage 完善（DailyTips + 最近项目 + 登录入口） | P1 | 🔴 待做 | 截图确认 |
| **PAGE-05** | ConfigPage 清理（冗余死代码） | P2 | 🔴 待做 | 删除后无引用 |

### V21DEFER — v2.1 延后项收尾

| ID | 差距 | 优先级 | 状态 |
|---|---|---|---|
| **V21-01** | SLICE-05 切片冲突热区 | P2 | 🔴 待评估 |
| **V21-02** | PRESET-03 ExportPresetBundleDialog | P2 | 🔴 待做 |
| **V21-03** | PRESET-04 CreatePresetsDialog | P2 | 🔴 待评估 |

### TECH — 技术债

| ID | 差距 | 优先级 | 状态 |
|---|---|---|---|
| **TECH-01** | cgal/libslic3r 每次全量重编（节省 10+ 分钟/次） | P1 | 🔴 待做 |
| **TECH-02** | review 遗留 Warning | P2 | 🔴 待做 |

---

## 优先级排序

**第一波（P0 页面补全）：**
1. PAGE-01 ProjectPage 按钮接线（影响项目工作流）
2. PAGE-02 DeviceListPage bug 修复（已知 bug）

**第二波（P1）：**
3. PAGE-03 AuxiliaryPage 卡片接线
4. PAGE-04 HomePage 完善
5. TECH-01 cgal 重编问题（工程效率）

**第三波（P2 收尾）：**
6. PAGE-05 ConfigPage 清理
7. V21-01/02/03 延后项
8. TECH-02 Warning

---

## Out of Scope (v2.2)

- Device/Cloud/Network 真实化（v2.3+）
- Calibration 真实化（v2.3+）
- AssembleView / ModelMall WebView / i18n（v2.3+）

---
*Last updated: 2026-06-18 — v2.2 (Page Completion & Cleanup) 启动；10 项差距*
