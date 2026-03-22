# 输出格式

## 1. 任务队列总览（每次开始时必须显示）

```
## 任务队列

| 优先级 | 任务ID | 简述 | 状态 | 剩余差距 |
|--------|--------|------|------|----------|
| P1 | P1.1 | 顶栏导航... | [-] | 缺通知管理器UI |
| P2 | P2.1 | Prepare平板... | [-] | 缺真实切片引擎 |
| ... | ... | ... | ... | ... |

**当前选中:** Px.x - 任务简述
```

## 2. 当前任务进度（任务执行中持续更新）

```
## 当前进度: Px.x - 任务简述

**Upstream:** `path/to/upstream/file.cpp:line`
**Target:** `path/to/qt6/file.qml`
**Status:** `[-]` -> 目标 `[x]`

### 进度
- [x] 分析上游行为
- [x] 定位 Qt6 目标模块
- [-] 实现迁移代码
- [ ] 构建验证
- [ ] 更新文档

### 剩余差距
- 具体差距描述
```

## 3. 任务完成报告（每个任务完成后）

```
## 完成: Px.x

**Change:** 具体修改内容
**Gap remaining:** 剩余差距（如无则写"无"）
**Doc updated:** 是/否
**Verification:** 构建命令已运行，结果: 通过/失败
```

## 4. 批量模式最终停止报告

```
## 迁移会话结束

**Tasks completed:** Px.x, Py.y, Pz.z
**Final verification status:** 通过/失败
**Stop reason:** 为什么停止
**Next recommended task:** Px.x - 任务简述
```

## 5. 恢复点摘要（批量模式每完成 2 个任务必须输出）

当 `/compact` 或上下文压力导致会话中断时，此摘要用于恢复批量模式状态：

```
## 恢复点

**Session:** 2026-03-18
**Tasks completed this session:** Px.x, Py.y
**Last verification:** 271/271 编译, 61/61 smoke test, 0 QML warnings
**Resume point:** 下一个应执行的任务是 Pz.z - 任务简述
**Blocked tasks:** P2.7 GLGizmoDrill (需 CGAL), P2.7 GLGizmoFaceDetector (需 SLA)
**Next scan order:** Pz.z → Pa.a → Pb.b (按优先级)
```
