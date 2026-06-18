# Roadmap: Milestone v2.3 — UI Completion Polish

## Overview

把 UI 完整度收尾到"无明显缺失"——补齐缺失对话框、挂载孤儿页面、完善 Gizmo UI、建立 i18n 基础。

**工作方式：** Phase 模式（回归传统 Phase 线性推进，每个 Phase 有明确 deliverable + 验收）

---

## v2.0-v2.2 已完成（地基）

- v2.0: UI 架构对齐（Plater/Sidebar/GLToolbars/八大区块）
- v2.1: 切片/预览深化（着色/TickCode/Preset Dialog/Search）+ 切片崩溃修复
- v2.2: 页面补全（ProjectPage/DeviceList/HomePage/ExportBundle）+ 审计 80%

---

## Phase 结构（v2.3）

### Phase 1: 缺失对话框 + 孤儿页面挂载

**目标：** 补齐信号已发但无接收的 Dialog + 挂载孤儿页面

| Task | 内容 | 验收 |
|---|---|---|
| UI-01 | KBShortcutsDialog（快捷键总览）| 打开显示快捷键列表 |
| UI-02 | AuxiliaryPage 挂载 main.qml | tab 切换能看到 Auxiliary 页 |
| UI-03 | NetworkTestDialog | 对话框能打开 |
| UI-04 | TroubleshootDialog | 对话框能打开 |

**Phase 1 出口：** 所有已有信号都有 dialog 接收 + 无孤儿页面

---

### Phase 2: 剩余 Gizmo UI 交互面板

**目标：** 完善 Gizmo 的交互面板（按钮已有，交互 UI 缺）

| Task | 内容 | 验收 |
|---|---|---|
| UI-05 | MmuSegmentation 多色分区 UI | 多色绘制面板可交互 |
| UI-06 | Hollow/BrimEars/FuzzySkin 占位 | Gizmo 按钮有基础面板 |
| UI-07 | Emboss UI 完善 | 文字输入 + 3D 预览 |

**Phase 2 出口：** 所有 GLToolbars 的 Gizmo 按钮都有对应交互面板

---

### Phase 3: i18n 基础 + 视觉打磨

**目标：** 建立 i18n 翻译基础 + 视觉打磨收尾

| Task | 内容 | 验收 |
|---|---|---|
| UI-08 | zh_CN 翻译填充（80%+ 常用串）| lrelease 后中文显示 |
| UI-09 | i18n 流程文档 | docs 有 lupdate/lrelease 工作流 |
| UI-10 | 视觉打磨（图标/配色微调）| 待本机验收 |

**Phase 3 出口：** i18n 基础建立 + 视觉无明显缺失

---

## 工作流程（每个 Phase）

```
1. 按 Phase 顺序执行 Task
2. 每个 Task: 实现 → 构建 → 冒烟
3. Phase 所有 Task 完成 → Phase 验收（构建 + 冒烟 + 用户确认）
4. Phase 验收通过 → 进入下一 Phase
```

---

## Out of Scope (v2.3)

- Device/Cloud/Network/Calibration 真实化（v2.4+）
- PartPlate/AssembleView（v2.4+）
- ModelMall/Home WebView（v2.4+）
- CreatePresetsDialog 264KB（v2.4+）

---
*Last updated: 2026-06-18 — v2.3 (UI Completion Polish) 启动；Phase 模式；10 Tasks 分 3 Phase*
