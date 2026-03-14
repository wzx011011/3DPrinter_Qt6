---
paths:
  - "src/core/**/*"
---

# Core Architecture Rules

- `src/core` 承载上游行为的 Qt 适配层，不承载纯展示逻辑。
- 优先在 `services`、`viewmodels`、`rendering` 中落业务行为和状态编排。
- 新增能力时优先复用或扩展现有服务和 ViewModel，而不是绕过它们直接从 QML 访问实现细节。
- 保持接口以迁移承接为目标，不为脱离上游的全新产品行为设计额外抽象。
