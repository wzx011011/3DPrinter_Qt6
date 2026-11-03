---
name: migrating-source-truth
description: "Migrating source-truth tasks from OrcaSlicer upstream to Qt6/QML. Triggers when the user asks to continue migration, pick up the next task, or run migration in batch mode."
---

# 源码真值迁移

推进本仓库从 OrcaSlicer 到 Qt6/QML 的源码真值迁移。

遵循 `.codex/rules/source-truth-migration.md` 中的规范，不在此重复全局规则。

## 目标

从 `docs/源码对照迁移任务追踪.md` 中选取当前最高优先级且未完成的任务，以 `third_party/OrcaSlicer` 为工作基础推进有边界的迁移工作单元。

## 参数

- `$ARGUMENTS` 为空或未提供：**单任务模式** — 推进 1 个任务后停止并等待用户再次调用
- `$ARGUMENTS` 包含 `all`：**批量模式** — 连续推进直到遇到停止条件

## 工作流

1. 读取 `docs/源码对照迁移任务追踪.md`，显示任务队列总览（格式见 `reference/output-format.md`）
2. **选任务**：按追踪文档第 7 节"建议顺序"依次扫描第 4 节缺陷（C*/P* 编号）和第 5 节功能缺口；跳过第 5.3 节外部阻塞项和第 6 节待决策项，选择第一个可推进的条目。某一类剩余条目全部被阻塞时，**必须自动跳到下一类**，不得停住
3. 报告任务 ID、上游模块、Qt6 目标模块、当前状态
4. 在 `third_party/OrcaSlicer` 中定位上游源码，在 `src/core` 或 `src/qml_gui` 中定位 Qt6 目标
5. 实现最小、可验证的根因迁移
6. 运行 `scripts/auto_verify_with_vcvars.ps1` 验证，报告结果
7. 更新追踪文档：条目修复并验证后**直接删除该条**（历史在 git 里）；领域状态确实变化时更新第 3 节（状态语义见 rules）
8. **单任务模式**：停止并报告完成。**批量模式**：按 `reference/stop-conditions.md` 决定是否继续

## 跨节扫描规则（批量模式关键）

批量模式下，**停止条件只在整个追踪文档中无任何可推进任务时才触发**。以下情况**不是停止理由**，而是跳到下一节的理由：

- 当前节的剩余任务都被外部依赖阻塞（如 CGAL/SLA/harfbuzz）
- 当前节已无 `[-]` 或 `[ ]` 任务

**禁止行为**：完成第 4 节部分缺陷后，发现其余缺陷被阻塞就停止。必须继续扫描第 5 节的功能缺口。

## 上下文预算管理（批量模式关键）

批量模式长时间运行会耗尽上下文窗口，导致 `/compact` 后批量状态丢失。为避免此问题：

- **每完成 2 个任务**，必须将恢复点写入 MEMORY.md（路径: `C:\Users\67376\.Codex\projects\E--ai-3DPrinter-Qt6\memory\MEMORY.md`）。MEMORY.md 是唯一 compact 后保证可用的持久化机制
- 恢复点格式（写入 MEMORY.md）：

```
## 批量迁移恢复点
**Last update:** YYYY-MM-DD
**Tasks completed this session:** <ID>, <ID>
**Last verification:** 编译数/smoke test数, QML warnings数
**Next task:** <ID> - 任务简述
**Blocked:** 被阻塞的任务及原因
```

- **恢复流程**：compact 后用户重新调用 `/migrating-source-truth all`，Skill 启动时从 MEMORY.md 读取恢复点，直接跳到 Next task
- **积极使用子 agent**：上游分析、代码实现、构建验证都应委托子 agent，主 agent 只做任务选择和文档更新。详见 `reference/subagent-strategy.md`
- **单个任务的子 agent 委托链**：上游分析 → 代码实现 → 构建验证 → 文档更新，应拆为独立子 agent 串行执行，而非在主上下文中展开
- **上下文压力信号**：当主上下文已积累大量工具输出时，应更激进地使用子 agent 并避免在主上下文中读取大文件

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
