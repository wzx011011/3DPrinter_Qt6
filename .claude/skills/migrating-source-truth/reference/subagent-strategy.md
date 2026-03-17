# 子 Agent 委托策略

积极使用 Agent 工具启动子 agent 执行独立任务，保持主上下文精简。

## 委托表

| 任务类型 | Agent 类型 | 说明 |
|----------|------------|------|
| 上游源码分析 | `Explore` | 分析 `third_party/CrealityPrint` 中的行为，返回关键发现 |
| 代码搜索 | `Explore` | 搜索 QML/C++ 文件定位目标模块 |
| 代码实现 | `general-purpose` | 独立实现迁移代码，完成后返回 diff 摘要 |
| 构建验证 | `general-purpose` | 运行 `auto_verify_with_vcvars.ps1`，返回通过/失败 |

## 执行模式

```
主 Agent (精简上下文)
|-- Agent 1: 分析上游 Search.cpp 行为 -> 返回 jump_to_option 关键逻辑
|-- Agent 2: 定位 Qt6 目标文件 -> 返回 PrintSettings.qml:85
|-- Agent 3: 实现迁移代码 -> 返回 "已修改 PrintSettings.qml 第 918-940 行"
+-- Agent 4: 构建验证 -> 返回 "通过，0 错误"
```

## 规则

- 主 agent 只负责：读取任务队列、选择任务、汇总结果、更新文档
- 凡是涉及文件搜索、代码阅读、代码修改、构建执行的操作，都委托给子 agent
- 子 agent 返回精简摘要，不返回完整代码或长日志
