---
name: migrating-source-truth
description: "Migrating source-truth tasks from CrealityPrint upstream to Qt6/QML. Triggers when the user asks to continue migration, pick up the next task, or run migration in batch mode."
---

# 源码真值迁移

推进本仓库从 CrealityPrint 到 Qt6/QML 的源码真值迁移。

遵循 `.claude/rules/source-truth-migration.md` 中的规范，不在此重复全局规则。

## 目标

从 `docs/源码对照迁移任务追踪.md` 中选取当前最高优先级且未完成的任务，以 `third_party/CrealityPrint` 为工作基础推进有边界的迁移工作单元。

## 参数

- `$ARGUMENTS` 为空或未提供：**单任务模式** — 推进 1 个任务后停止并等待用户再次调用
- `$ARGUMENTS` 包含 `all`：**批量模式** — 连续推进直到遇到停止条件

## 工作流

1. 读取 `docs/源码对照迁移任务追踪.md`，显示任务队列总览（格式见 `reference/output-format.md`）
2. 选择最高优先级 `[-]` 或 `[ ]` 任务，报告任务 ID、上游模块、Qt6 目标模块、当前状态
3. 在 `third_party/CrealityPrint` 中定位上游源码，在 `src/core` 或 `src/qml_gui` 中定位 Qt6 目标
4. 实现最小、可验证的根因迁移
5. 运行 `scripts/auto_verify_with_vcvars.ps1` 验证，报告结果
6. 仅当任务状态确实变化时更新追踪文档（状态语义见 rules）
7. **单任务模式**：停止并报告完成。**批量模式**：按 `reference/stop-conditions.md` 决定是否继续

## 子 Agent 策略

积极使用子 agent 委托独立任务以保持主上下文精简，详见 `reference/subagent-strategy.md`。

## 优先级顺序

1. P0 源码真值版本管理与功能矩阵
2. P2 Prepare 工作区
3. P3 Preview 工作区
4. P4 Settings / Preset / 覆盖作用域
5. P5 Device / Monitor / Network
6. P6 Calibration / Model Mall / MultiMachine
7. P7 质量与发布收尾
