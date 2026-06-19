# Roadmap: Milestone v2.6 — v2.5 Remaining Completion

## Overview

完成 v2.5 三块遗留——SSDP 设备发现 + Calibration 完整 + 摄像头视频流。

### Phase 1: SSDP/mDNS 设备发现
- SSDP-01 QUdpSocket M-SEARCH + 响应解析
- SSDP-02 NetworkService 真实化
- SSDP-03 DeviceListPage 自动发现

### Phase 2: Calibration 完整实现
- CAL-01 CalibPressureAdvanceLine 实例化
- CAL-02 PA 校准 G-code 生成 + 切片
- CAL-03 FlowRate/TempTower
- CAL-04 结果回写 + 菜单激活

### Phase 3: 摄像头视频流
- CAM-01 FFmpeg 集成
- CAM-02 CameraService 真实化（RTSP 解码）
- CAM-03 MonitorPage 视频显示
- CAM-04 延时摄影

### Phase 4: 集成 + 自回归
- INT-01~03 SSDP/Calib/Camera 自回归

---
*Last updated: 2026-06-19 — v2.6 (v2.5 Remaining Completion); 4 Phase, 14 Tasks*
