---
name: analyzing-source-truth-gap
description: "Analyzing an upstream-to-Qt migration gap without making code changes. Triggers when the user asks for a read-only comparison between CrealityPrint upstream behavior and current Qt6 implementation."
---

# 分析源码真值差距

对本仓库执行只读的源码真值分析。

遵循 `.claude/rules/source-truth-migration.md` 中的规范，不在此重复全局规则。

## 目标

将当前的 Qt6/QML 实现与上游 `third_party/CrealityPrint` 的行为进行对比，识别缺失、不匹配或仅部分迁移的内容。

## 范围

- 不修改文件
- 不更新任务状态
- 当上游行为存在时，不提出自由设计的行为方案

## 工作流

1. 阅读 `docs/源码对照迁移任务追踪.md`、`docs/项目结构.md` 和 `docs/CrealityPrint_Qt_GUI重写架构.md`
2. 根据参数确定目标任务（参数格式见下方）
3. 识别 `third_party/CrealityPrint` 中对应的上游源码
4. 识别 `src/core`、`src/pages` 或 `src/qml_gui` 中当前的 Qt6 模块
5. 对比上游行为和当前 Qt6 行为，以具体条目报告差距

## 参数

`$ARGUMENTS` 视为要分析的任务 ID、模块、页面或功能名称。未提供时分析下一个最高优先级未完成任务。

## 输出格式

参见 `reference/output-format.md`。
