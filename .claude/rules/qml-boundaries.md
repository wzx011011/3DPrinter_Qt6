---
paths:
  - "src/qml_gui/**/*"
  - "src/pages/**/*"
---

# QML Boundary Rules

- QML 负责呈现、布局、组合和交互接线，不承载业务逻辑。
- 当页面需要非展示计算、状态合并或工作流判断时，优先迁移到 `src/core` 的 service、viewmodel 或 rendering adapter。
- 避免在 QML 中新增复杂内联脚本；简单展示辅助函数可以保留，业务判断不应保留在页面层。
- 页面行为应以上游 CrealityPrint 工作流为准，不自行发明新的用户路径。
