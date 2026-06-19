---
gsd_state_version: 1.0
milestone: v2.6
milestone_name: v2.5 Remaining Completion
status: complete
last_updated: 2026-06-20
progress:
  total_phases: 4
  completed_phases: 4
  total_tasks: 14
  completed_tasks: 13
  deferred_tasks: 1
stopped_at: null
---

# Project State

**Milestone:** v2.6 — v2.5 Remaining Completion
**Status:** ✅ Complete (4/4 phases)
**下一步:** v2.7 新里程碑规划（待 gsd-audit-milestone 收尾后启动）

## v2.6 Phase 完成状态

| Phase | 名称 | 状态 | 关键 commit |
|---|---|---|---|
| 1 | SSDP/mDNS 设备发现 | ✅ Complete | a9d691c |
| 2 | Calibration 完整实现 | ⏭ Deferred → v2.7 | (骨架保留) |
| 3 | 摄像头视频流 (CAM-01~03) | ✅ Complete | 49db939, e67d116 |
| 4 | 集成 + 自回归 (INT-01/03) | ✅ Complete | 03c6078, ade78f3, 4f0018e |

## v2.6 关键交付
- **SSDP 设备发现**：SsdpDiscovery（QUdpSocket M-SEARCH + 响应解析）→ NetworkService 真实化 → DeviceListPage 自动发现
- **摄像头视频流**：CameraStream（FFmpeg RTSP 解码线程）+ CameraImageProvider（QQuickImageProvider）+ MonitorPage 实时视频
- **FFmpeg 运行时修复**（关键）：版本号 DLL 从未复制到 build 目录导致 0xC0000135 启动崩溃 → POST_BUILD 复制步骤
- **自回归**：INT-01 SSDP（PASS）、INT-03 Camera（PASS）、INT-02 延后 v2.7
- **arrange bed_idx==-1 生产 bug 修复**：arrange_objects 默认 throwing vfn → 容错 no-op vfn + loadFile 默认 220x220 热床

## v2.0-v2.5 已完成
- v2.0 UI 架构 ✅ / v2.1 切片深化+崩溃修复 ✅ / v2.2 页面补全 ✅
- v2.3 UI 收尾+i18n ✅ / v2.4 项目IO ✅ / v2.5 MQTT+打印发送 ✅

## 延后到 v2.7
- Calibration 完整实现（CAL-01~04，CalibPressureAdvanceLine + GCode 上下文）
- CAM-04 延时摄影
- INT-02 Calibration 自回归
- PartPlate 多板编辑（258KB 上游）
- ModelMall/Home WebView（Qt WebEngine）
- CreatePresetsDialog（264KB 上游）
- i18n 外语（ja/ko/de/fr）
- AssembleView 多板装配
- 完整 PresetBundle 加载（让 slice-path E2E 真正跑通）
