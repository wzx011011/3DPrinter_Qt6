---
phase: 08-v26-camera-stream
plan: 01
subsystem: Core Services (Camera) + QML UI
tags: [v2.6, camera, ffmpeg, rtsp, qquickimageprovider]
dependency_graph:
  requires: [06-v26-ssdp-discovery]
  provides: [camera-stream-service, monitor-live-video, ffmpeg-runtime-dlls]
  affects: [09-v26-integration-regression]
tech-stack:
  added:
    - src/core/services/CameraStream.h
    - src/core/services/CameraStream.cpp
    - src/qml_gui/CameraImageProvider.h
    - src/qml_gui/CameraImageProvider.cpp
  patterns: [ffmpeg-rtsp-decode-thread, qquickimageprovider-frame-token]
key-files:
  created:
    - src/core/services/CameraStream.h
    - src/core/services/CameraStream.cpp
    - src/qml_gui/CameraImageProvider.h
    - src/qml_gui/CameraImageProvider.cpp
  modified:
    - src/core/services/CameraServiceMock.h
    - src/core/services/CameraServiceMock.cpp
    - src/core/viewmodels/MonitorViewModel.h
    - src/core/viewmodels/MonitorViewModel.cpp
    - src/qml_gui/BackendContext.h
    - src/qml_gui/BackendContext.cpp
    - src/qml_gui/main_qml.cpp
    - src/qml_gui/pages/MonitorPage.qml
    - CMakeLists.txt
decisions:
  - "CameraStream 用 FFmpeg 解码 RTSP→YUV420P→RGB24→QImage，低优先级 QThread"
  - "FFmpeg 头文件必须先于 Qt 头文件（宏冲突），static_cast<QImage::Format>(4) 规避 RGB24 污染"
  - "CameraImageProvider (QQuickImageProvider) 注册 image://camera/live，frameToken 做 cache-buster"
  - "CameraServiceMock 持有 CameraStream，进入 Streaming 时自动构造 Bambu RTSP URL 启动解码"
  - "MonitorPage 视频 tab：streaming+frameToken>0 显示真实 Image，否则保留 mock 占位"
  - "CRITICAL 运行时修复：FFmpeg 版本号 DLL（avcodec-59 等）从未复制到 build 目录，"
  - "  导致 STATUS_DLL_NOT_FOUND (0xC0000135) 启动崩溃。新增 POST_BUILD 复制步骤（镜像 OCCT DLL 模式）"
metrics:
  duration: ~90 minutes
  completed_date: 2026-06-19
  commits: [49db939, e67d116]
---

# Phase 3 (v2.6): 摄像头视频流 (CAM-01~03)

One-line: FFmpeg RTSP 解码线程 + QQuickImageProvider 帧桥接 + MonitorPage 实时视频；修 FFmpeg DLL 运行时崩溃。

## 已实现 (CAM-01/02/03)

- **CAM-01/02**: `CameraStream`（FFmpeg RTSP 解码线程）— avformat→avcodec→swscale→QImage，
  运行在低优先级 QThread，`frameReady` 信号投递 QImage 到 UI 线程。
- **CAM-03**: `CameraImageProvider`（QQuickImageProvider）注册 `image://camera/live`，
  `CameraServiceMock.frameToken` Q_PROPERTY 做 cache-buster 驱动 QML Image 重新拉取。
  MonitorPage 视频 tab 显示真实帧（带加载旋转图标），非流式状态保留 mock 占位。

## 关键运行时修复
FFmpeg 通过 import lib 链接，但版本号 DLL（avcodec-59/avformat-59/avutil-57/swscale-6/
dav1d/swresample/avfilter）从未复制到 build 目录，导致 `OWzxSlicer.exe` 启动即
`STATUS_DLL_NOT_FOUND (0xC0000135)`。新增 POST_BUILD `copy_if_different` 步骤。

## 延后
- CAM-04 延时摄影：延后 v2.7（依赖切片完成事件 + 摄像头触发框架）
