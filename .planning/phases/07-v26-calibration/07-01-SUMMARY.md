---
phase: 07-v26-calibration
plan: 01
subsystem: Core Services (Calibration)
tags: [v2.6, calibration, deferred]
dependency_graph:
  requires: [HAS_LIBSLIC3R]
  provides: [calibration-skeleton]
  affects: []
tech-stack:
  added:
    - src/core/services/CalibrationServiceMock.cpp (HAS_LIBSLIC3R 骨架)
  patterns: [calib-skeleton-logging]
key-files:
  created: []
  modified:
    - src/core/services/CalibrationServiceMock.cpp
decisions:
  - "CAL-01~04 整体延后到 v2.7：CalibPressureAdvance 是抽象类（protected 构造），"
  - "  需 CalibPressureAdvanceLine 子类 + GCode 上下文才能实例化，工作量超出 v2.6 范围"
  - "保留 HAS_LIBSLIC3R 校准骨架（PA/FlowRate/TempTower 日志），v2.7 完成"
metrics:
  duration: ~5 minutes (skeleton only)
  completed_date: 2026-06-19
  commits: []
  deferred_to: v2.7
---

# Phase 2 (v2.6): Calibration 完整实现 — 延后 v2.7

One-line: 调研发现 CalibPressureAdvance 抽象类约束，CAL-01~04 整体延后 v2.7；保留骨架。

## 延后原因（关键约束）
`Slic3r::CalibPressureAdvance` 构造函数是 protected，无法直接实例化。需要：
1. 实现 `CalibPressureAdvanceLine` 子类（override 虚函数）
2. 提供 GCode 上下文（`GCode` 实例 + `Print` + 完整 config）
3. 处理 PA/FlowRate/TempTower 三种校准模式的 G-code 生成

这些工作量超出 v2.6（已聚焦设备发现 + 摄像头 + 回归）的范围，故决策延后 v2.7。

## 保留
- `CalibrationServiceMock` 的 `HAS_LIBSLIC3R` 骨架（PA/FlowRate/TempTower 日志占位）
- INT-02 自回归测试延后 v2.7（已记录在 ROADMAP）
