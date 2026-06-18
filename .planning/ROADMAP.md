# Roadmap: Milestone v2.5 — Real Device Integration

## Overview

三大底层集成——真机打印闭环 + Calibration 真实化 + PartPlate 多板系统。

**工作方式：** Phase 模式（沿用）

---

## Phase 结构

### Phase 1: MQTT 真机通信基础
- DEV-01 MQTT 客户端封装（paho-mqtt）
- DEV-02 DeviceService 真实化（MQTT 订阅设备状态）
- DEV-03 SSDP 局域网设备发现

### Phase 2: 打印发送链路
- DEV-04 SelectMachineDialog（选目标打印机 + 发送）
- DEV-05 PrintDialog 接通 SelectMachine
- DEV-06 打印进度实时更新

### Phase 3: Calibration 真实化
- CAL-01 CalibrationService 接 libslic3r calib.cpp
- CAL-02 9 种 CalibMode 实现
- CAL-03 结果回写 preset
- CAL-04 CalibrationPage 菜单激活

### Phase 4: PartPlate 多板系统
- PLATE-01 PartPlate 数据结构
- PLATE-02 多板编辑 UI
- PLATE-03 独立板切片配置
- PLATE-04 AssembleView（占位）

### Phase 5: 集成 + 自回归
- INT-01 设备自回归
- INT-02 校准自回归
- INT-03 多板自回归

---

## 依赖与外部库

| 模块 | 依赖 | 状态 |
|---|---|---|
| MQTT | paho-mqtt | ✅ deps 目录已有 |
| Calibration | libslic3r calib.cpp (40KB) | ✅ 上游已有 |
| PartPlate | PartPlate.cpp (258KB) | ✅ 上游已有 |

---

## Out of Scope (v2.5)

- ModelMall/Home WebView（v2.6+）
- i18n 外语翻译（v2.6+）
- CreatePresetsDialog（v2.6+）
- 摄像头视频流（v2.6+）

---
*Last updated: 2026-06-19 — v2.5 (Real Device Integration) 启动；5 Phase, 16 Tasks*
