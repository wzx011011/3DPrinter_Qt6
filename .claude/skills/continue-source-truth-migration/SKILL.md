---
name: continue-source-truth-migration
description: Continue the next highest-priority source-truth migration task in this repository.
disable-model-invocation: true
---

# 继续源码真值迁移

继续推进本仓库中下一个已记录的源码真值迁移任务。

遵循 `.claude/rules/source-truth-migration.md` 中的规范，不在此重复全局规则。

## 目标

从 `docs/源码对照迁移任务追踪.md` 中选取当前最高优先级且未完成的任务，以 `third_party/CrealityPrint` 为工作基础推进有边界的迁移工作单元。

## 输出格式（必须严格遵守）

### 1. 任务队列总览（每次开始时必须显示）

```
## 任务队列

| 优先级 | 任务ID | 简述 | 状态 | 剩余差距 |
|--------|--------|------|------|----------|
| P1 | P1.1 | 顶栏导航... | [-] | 缺通知管理器UI |
| P2 | P2.1 | Prepare平板... | [-] | 缺真实切片引擎 |
| ... | ... | ... | ... | ... |

**当前选中:** Px.x - 任务简述
```

### 2. 当前任务进度（任务执行中持续更新）

```
## 当前进度: Px.x - 任务简述

**Upstream:** `path/to/upstream/file.cpp:line`
**Target:** `path/to/qt6/file.qml`
**Status:** `[-]` → 目标 `[x]`

### 进度
- [x] 分析上游行为
- [x] 定位 Qt6 目标模块
- [-] 实现迁移代码
- [ ] 构建验证
- [ ] 更新文档

### 剩余差距
- 具体差距描述
```

### 3. 任务完成报告

```
## 完成: Px.x

**Change:** 具体修改内容
**Gap remaining:** 剩余差距（如无则写"无"）
**Doc updated:** 是/否
**Verification:** 构建命令已运行，结果: 通过/失败
```

## 工作流（严格执行）

1. **读取任务追踪**
   - 读取 `docs/源码对照迁移任务追踪.md`
   - 显示任务队列总览表格
   - 选择最高优先级 `[-]` 或 `[ ]` 任务

2. **显示当前任务进度**
   - 显示 Upstream/Target/Status
   - 列出进度检查项（分析/定位/实现/验证/文档）

3. **执行迁移**
   - 在 `third_party/CrealityPrint` 中定位上游源码
   - 在 `src/core` 或 `src/qml_gui` 中定位 Qt6 目标
   - 实现最小、可验证的根因迁移

4. **构建验证（每个任务完成后必须执行）**
   - 运行 `scripts/auto_verify_with_vcvars.ps1`
   - 等待构建完成
   - 报告验证结果

5. **更新文档**
   - 仅当任务状态确实变化时更新追踪文档

6. **循环或停止**
   - 如果验证通过且有下一个任务：继续执行
   - 如果验证失败：停止并报告
   - 如果无更多任务：停止并报告完成

## 停止条件

- 任务队列为空（无 `[-]` 或 `[ ]` 任务）
- 当前任务缺少上游映射，需用户决策
- 构建验证失败且无法立即修复
- 用户显式要求停止

## 任务粒度

- 每次只推进 **1 个** 有边界的任务单元
- 任务完成后 **必须** 运行构建验证
- 验证通过后才算任务完成
- 用户再次调用本 skill 继续下一个任务

## 子 Agent 策略（避免上下文膨胀）

积极使用 Agent 工具启动子 agent 执行独立任务，保持主上下文精简：

| 任务类型 | Agent 类型 | 说明 |
|----------|------------|------|
| 上游源码分析 | `Explore` | 分析 `third_party/CrealityPrint` 中的行为，返回关键发现 |
| 代码搜索 | `Explore` | 搜索 QML/C++ 文件定位目标模块 |
| 代码实现 | `general-purpose` | 独立实现迁移代码，完成后返回 diff 摘要 |
| 构建验证 | `general-purpose` | 运行 `auto_verify_with_vcvars.ps1`，返回通过/失败 |

**执行模式：**
```
主 Agent (精简上下文)
├── Agent 1: 分析上游 Search.cpp 行为 → 返回 jump_to_option 关键逻辑
├── Agent 2: 定位 Qt6 目标文件 → 返回 PrintSettings.qml:85
├── Agent 3: 实现滚动代码 → 返回 "已修改 PrintSettings.qml 第 918-940 行"
└── Agent 4: 构建验证 → 返回 "通过，0 错误"
```

**规则：**
- 主 agent 只负责：读取任务队列、选择任务、汇总结果、更新文档
- 凡是涉及文件搜索、代码阅读、代码修改、构建执行的操作，都委托给子 agent
- 子 agent 返回精简摘要，不返回完整代码或长日志
