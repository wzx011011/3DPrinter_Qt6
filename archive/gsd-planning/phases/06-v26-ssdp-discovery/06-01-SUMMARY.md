---
phase: 06-v26-ssdp-discovery
plan: 01
subsystem: Core Services (Networking)
tags: [v2.6, ssdp, device-discovery, udp-multicast]
dependency_graph:
  requires: []
  provides: [ssdp-discovery-service, network-service-real-discover]
  affects: [08-v26-camera-stream, 09-v26-integration-regression]
tech-stack:
  added:
    - src/core/services/SsdpDiscovery.h
    - src/core/services/SsdpDiscovery.cpp
  patterns: [QUdpSocket-M-SEARCH, multicast-239.255.255.250]
key-files:
  created:
    - src/core/services/SsdpDiscovery.h
    - src/core/services/SsdpDiscovery.cpp
  modified:
    - src/core/services/NetworkServiceMock.h
    - src/core/services/NetworkServiceMock.cpp
    - src/qml_gui/pages/DeviceListPage.qml
decisions:
  - "SSDP M-SEARCH 发到 239.255.255.250:1900，QUdpSocket 绑定任意 IPv4"
  - "Bambu 设备识别：ST 含 'bambu' → port=8883；否则 port=1883"
  - "USN uuid 提取 serial，Location header 提取 IP"
  - "DeviceListPage 启动时自动 discoverDevices()（修复强制为空 bug）"
  - "NetworkServiceMock.discoverDevices 转发到 SsdpDiscovery"
metrics:
  duration: ~25 minutes
  completed_date: 2026-06-19
  commits: [1aef967, a9d691c]
---

# Phase 1 (v2.6): SSDP/mDNS 设备发现

One-line: 实现 SSDP M-SEARCH 多播设备发现 + 响应解析，NetworkService 真实化，DeviceListPage 自动发现。

## 已实现 (SSDP-01~03)

- **SSDP-01**: `SsdpDiscovery` 类 — QUdpSocket 发 M-SEARCH 到 239.255.255.250:1900，
  监听响应，解析 HTTP 风格头（LOCATION/ST/USN/SERVER）。`parseResponse` 提取
  ip/serial/name/isBambu/port。
- **SSDP-02**: `NetworkServiceMock.discoverDevices()` 转发到 `SsdpDiscovery`，
  真实多播发现替代 mock 占位。
- **SSDP-03**: `DeviceListPage` Component.onCompleted 自动调用 discoverDevices()，
  修复之前"设备列表强制为空"的 bug。

## 验证
- INT-01 自回归测试（见 09 phase）在真实 LAN 发现 Synology NAS，解析字段正确。
- 构建 6/6 通过，应用启动无 SSDP 相关警告。
