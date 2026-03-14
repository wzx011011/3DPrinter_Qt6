---
name: continue-source-truth-migration
description: Continue the next highest-priority source-truth migration task in this repository.
disable-model-invocation: true
---

# 继续源码真值迁移

继续推进本仓库中下一个已记录的源码真值迁移任务。

遵循 `.claude/rules/source-truth-migration.md` 中的规范，不在此重复全局规则。

## 目标

从 `docs/源码对照迁移任务追踪.md` 中持续选取当前最高优先级且未完成的任务，以 `third_party/CrealityPrint` 为工作基础反复推进有边界的迁移工作单元，直到达到停止条件。

## 工作流

1. 阅读 `docs/源码对照迁移任务追踪.md`，选择最高优先级未完成任务，除非用户明确指定其他任务。
2. 在 `third_party/CrealityPrint` 中定位对应的上游源码文件或模块。
3. 在 `src/core` 或 `src/qml_gui` 中定位对应的 Qt6 目标模块。
4. 如果任务边界不清晰，先更新 `docs/源码对照迁移任务追踪.md`，再开始实现。
5. 实现最小、可验证的根因迁移步骤，避免表层美化式修改。
6. 如果运行时行为发生变化，运行 `scripts/auto_verify_with_vcvars.ps1`。
7. 仅当任务状态确实变化时更新 `docs/源码对照迁移任务追踪.md`。
8. 一个任务完成后，立即重新读取 `docs/源码对照迁移任务追踪.md`，继续选择下一个最高优先级未完成任务，不等待用户再次确认。
9. 默认每完成 2 个任务运行一次 `scripts/auto_verify_with_vcvars.ps1`；如果单个任务已经影响运行时行为，则该任务完成后立即验证。

## 默认范围

- 单次调用默认连续推进多个有边界的工作单元，而不是只推进 1 个任务。
- 只要仍存在明确的未完成任务、没有遇到阻塞、验证没有失败，就继续向下执行。
- 优先按任务粒度推进；仅在多个任务天然耦合时，将其作为一个小批次连续完成。

## 停止条件

满足任一条件时停止连续执行并汇报当前状态：

- `docs/源码对照迁移任务追踪.md` 中不存在可继续推进的未完成任务。
- 当前任务缺少清晰的上游映射，且需要先补任务文档或等待用户决策。
- 连续执行过程中验证失败，且在当前调用中无法安全修复。
- 出现外部依赖、环境、权限或构建问题，导致无法继续可靠推进。
- 用户显式要求停止，或显式限制只执行一个任务。

## 连续执行约束

- 目标是“持续循环直到收敛或遇到停止条件”，不是无条件死循环。
- 不要为了追求连续执行而跳过验证、跳过文档更新、或跨越不清晰的任务边界。
- 每完成一个任务都要重新评估优先级，不能机械地沿用上一个任务的上下文。
- 当连续执行的工作量已经很大时，应在阶段性输出中明确报告已完成任务、当前验证状态和下一个候选任务。

## 输出格式

开始时报告：

- Task
- Upstream
- Target
- Current status

连续执行过程中每完成一个任务后报告：

- Change made
- Remaining gap to reach `[x]`
- Whether `docs/源码对照迁移任务追踪.md` status changed
- Verification run or skipped
- Next selected task

最终停止时报告：

- Tasks completed in this run
- Final verification status
- Why execution stopped
- Next recommended task
