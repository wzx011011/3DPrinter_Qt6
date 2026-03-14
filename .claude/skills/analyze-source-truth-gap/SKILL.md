---
name: analyze-source-truth-gap
description: Analyze an upstream-to-Qt migration gap for this repository without making code changes.
disable-model-invocation: true
---

# 分析源码真值差距

对本仓库执行只读的源码真值分析。

遵循 `.claude/rules/source-truth-migration.md` 中的规范，不在此重复全局规则。

## 目标

将当前的 Qt6/QML 实现与上游 `third_party/CrealityPrint` 的行为进行对比，识别缺失、不匹配或仅部分迁移的内容。

## 范围

- 不修改文件。
- 不更新任务状态。
- 当上游行为存在时，不提出自由设计的行为方案。

## 工作流

1. 阅读 `docs/源码对照迁移任务追踪.md`、`docs/项目结构.md` 和 `docs/CrealityPrint_Qt_GUI重写架构.md`。
2. 根据提供的上下文或参数确定目标任务。
3. 识别 `third_party/CrealityPrint` 中对应的上游源码文件或模块。
4. 识别 `src/core`、`src/pages` 或 `src/qml_gui` 中当前的 Qt6 目标模块。
5. 对比上游行为和当前 Qt6 行为。
6. 以具体条目报告差距，而非模糊印象。

## 输出格式

始终报告：

- 任务 ID
- 上游模块
- Qt6 目标模块
- `docs/源码对照迁移任务追踪.md` 中的当前任务状态
- 已实现的现有行为
- 缺失或不匹配的行为
- 建议的下一步根因迁移操作
- 实现后是否可能需要运行时验证

## 参数

如果提供了参数，将其视为要分析的任务、模块、页面或功能：

$ARGUMENTS
