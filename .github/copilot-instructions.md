# CrealityPrint Qt6 工作区指令

> 本文件与 `.cdx2cache/rules/source-truth-migration.md` 保持同步，修改时请同步更新两处。

本仓库是一个源码真值迁移项目。

## 核心规则

- 将 `third_party/CrealityPrint` 视为用户可见行为、工作流和功能范围的上游真值源。
- 将 `src/` 下的 Qt6/QML 代码视为迁移与呈现层，必须继承上游行为。
- 当上游已有实现时，不得自由设计新的产品行为。

## 必须遵循的工作流程

在修改代码前：

1. 在 [docs/TASKS.md](../docs/TASKS.md) 中识别匹配的任务。
2. 在 `third_party/CrealityPrint` 中识别上游源码文件或模块。
3. 确认应承载该行为的 Qt6 目标模块。

如果没有匹配的任务：

- 先更新 [docs/TASKS.md](../docs/TASKS.md)。
- 不要针对未记录的目标开始实现。

## 进度记账

- 不要仅因为页面存在或基础链路可运行就标记工作为完成。
- 只有在与上游行为对照并验证后才算任务完成。
- 使用 [docs/TASKS.md](../docs/TASKS.md) 中定义的任务状态语义：
  - `[x]` 已与上游完全对齐并通过验证
  - `[-]` 部分迁移或仅完成基础闭环
  - `[ ]` 尚未开始或仅有占位

## 架构边界

- 保持架构与 [docs/CrealityPrint_Qt_GUI_Rewrite_Architecture.md](../docs/CrealityPrint_Qt_GUI_Rewrite_Architecture.md) 对齐。
- 保持模块边界与 [docs/PROJECT_STRUCTURE.md](../docs/PROJECT_STRUCTURE.md) 对齐。
- 优先将上游行为适配到 `src/core/services`、`src/core/viewmodels` 和 `src/core/rendering`，而不是在 QML 中嵌入业务逻辑。
- QML 应专注于呈现、组合和交互接线。

## 建议的执行顺序

除非用户明确覆盖，按以下顺序优先推进：

1. P0 源码真值版本管理与功能矩阵
2. P2 Prepare 工作区
3. P3 Preview 工作区
4. P4 Settings / Preset / 覆盖作用域
5. P5 Device / Monitor / Network
6. P6 Calibration / Model Mall / MultiMachine
7. P7 质量与发布收尾

## 验证预期

- 当代码变更影响运行时行为时，使用 `scripts/auto_verify_with_vcvars.ps1` 作为权威的完整验证路径。
- 优先根因修复而非仅做视觉层面的修补。
- 报告进度时，同时提及任务 ID 和正在迁移的上游模块。

## 反模式

- 不要在没有上游映射目标的情况下继续逐页美化。
- 不要将旧的阶段完成记录视为功能完成的证明。
- 不要在上游真实路径已存在或应该存在时添加新的仅 Mock 行为。
