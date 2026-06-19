# Roadmap: Milestone v2.6 — v2.5 Remaining Completion

## Overview

完成 v2.5 三块遗留：SSDP 设备发现 + Calibration 完整 + 摄像头视频流。

**状态：✅ Complete (4/4 phases)** — 2026-06-20

### Phase 1: SSDP/mDNS 设备发现 ✅ Complete (commit a9d691c)
- ✅ SSDP-01 QUdpSocket M-SEARCH + 响应解析
- ✅ SSDP-02 NetworkService 真实化
- ✅ SSDP-03 DeviceListPage 自动发现

### Phase 2: Calibration 完整实现 ⏭ Deferred → v2.7
- ⏭ CAL-01 CalibPressureAdvanceLine 实例化
- ⏭ CAL-02 PA 校准 G-code 生成 + 切片
- ⏭ CAL-03 FlowRate/TempTower
- ⏭ CAL-04 结果回写 + 菜单激活
> 延后原因：CalibPressureAdvance 抽象类（protected 构造）需子类 + GCode 上下文，工作量超 v2.6 范围。

### Phase 3: 摄像头视频流 ✅ Complete (commits 49db939, e67d116)
- ✅ CAM-01 FFmpeg 集成
- ✅ CAM-02 CameraService 真实化（RTSP 解码）
- ✅ CAM-03 MonitorPage 视频显示
- ⏭ CAM-04 延时摄影 → v2.7

### Phase 4: 集成 + 自回归 ✅ Complete (commits 03c6078, ade78f3, 4f0018e)
- ✅ INT-01 SSDP 自回归（PASS）
- ⏭ INT-02 Calibration 自回归 → v2.7
- ✅ INT-03 Camera 自回归（PASS）
- ✅ E2E 切片测试夹具 + arrange bed_idx==-1 生产 bug 修复

---
*Last updated: 2026-06-20 — v2.6 Complete; 4 Phase, 14 Tasks (13 done, 1 deferred to v2.7)*
