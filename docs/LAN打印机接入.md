# OWzx 局域网打印机接入

> 状态：一期完成（L0–L4 代码 + L5 部分），分支 `feature/lan-printers`
> 协议参考：`CP/creality-ref/调研报告-LAN协议.md`（CrealityPrint 官方开源实现，AGPL-3.0）
> 审计记录：[依赖与协议边界审计.md](依赖与协议边界审计.md) — "设备基础发现与局域网连接"行（2026-10-23 部分解除）

## 1. 范围

**只做局域网，不做创想云设备通道**（云 MQTT/绑定/任务维持外部阻塞结论）。

一期机型：现代创想 LAN 机型（K1/K1C/K1 Max/K3 系，4408 固件 + Moonraker 两类）。
二期候选：老 FTP 机型（CR-10 Smart/D3）、WebRTC 视频、CFS 耗材向导、对象排除、
设备分组、设备文件管理、下载/上传断点续传。

## 2. 架构

```
Device/Backend 装配开关（QSettings，默认 mock）
  ├─ mock      → DeviceServiceMock（演示设备 + 诚实披露，现状不变）
  └─ moonraker → MoonrakerDeviceService : IDeviceDataPlane（本计划新增）
        ├─ 发现：MdnsDiscovery（mDNS _creality<类型码><Moonraker端口>）
        │        + LanDeviceProbe（/info HTTPS→HTTP，字段 model/mac/wssPort/videoPort）
        ├─ 状态：4408 → ws://IP:9999 推送；Moonraker → /printer/objects/query 3s 轮询
        ├─ 控制：{pause:1|0}/{stop:1}（ws）或 /printer/print/pause|resume|cancel
        └─ 发送：multipart 上传（/upload/<name> 或 /server/files/upload）→ 启动
                 （{opGcodeFile} 或 /printer/print/start）→ sendPrint* 信号族
```

- **UI 零结构改动**：MonitorPage/MultiMachinePage/PrintDialog 绑定的是
  MonitorViewModel（已放宽为 `IDeviceDataPlane*`），数据源切换对 UI 全透明
- **披露切换**：`Backend.deviceBackendKind` — moonraker 后端下监控页演示横幅
  不显示；mock 后端保持演示披露
- **持久化**：`Devices/Lan` QSettings 组（JSON 设备列表）+ `Devices/LanLastIp`
  （重启恢复选中）

## 3. 诚实语义（审计纪律）

- LAN 后端**无演示设备**（`isDemoDevice` 恒 false，`demo` 键恒 false）
- 未连接时控制命令无副作用；发送失败必发 `sendPrintFailed`，绝不假翻 printing
- 不支持的面（HMS/AMS/云任务）返回诚实空值

## 3.5 一期收尾包（2026-10-23 第二批）

| # | 功能 | 实现 |
|---|------|------|
| 1 | 错误码呈现 | `LanErrorCodes`（115=断料可确证；未知码显示"错误码 N"不编造——完整表在机型固件侧，开源材料不含）；ws `err.errcode` → `selectedDeviceErrorText` → 监控页错误横幅 |
| 2 | 打印前校准 | PrintDialog 发送页勾选（moonraker 后端）→ `enableSelfTest` 随启动命令 |
| 3 | 上传取消 | reply 句柄持久 + abort → `sendPrintCancelled`（mock 同语义） |
| 4 | 设备文件管理 | ws `pFileList` / Moonraker `/server/files/list` → 监控页文件 tab（列表/打印/删除）；**mDNS Moonraker 标记合并修复**（typeCode "00"+端口原在探测时丢失） |
| 5 | 详细控制 | 速度四档（speedMode）/舱灯（lightSw）/喷嘴温度（setNozzleTemp，0-300 守卫）——监控页 LAN 控制条 |
| 7 | 设备管理 | 重命名（持久化，空名守卫）/移除（含状态通道清理与选中复位）——设备详情头按钮 |
| 8 | 轴/风扇 | 归零（gcodeCmd G28）/轴偏移（setPosition）/模型风扇（modelFanPct 0-100 守卫）——控制条按钮 |

## 4. 已知限制 / 待办

1. **真机验收未完成**：开发环境 LAN 暂无在线的创想打印机（mDNS 浏览 +
   /24 端口扫描确认，2026-10-23）——真打印机上电后，运行
   `LanDiscoveryTests::realLanDiscoverySmoke` 即可完成发现/探测真机验证；
   发送/监控 E2E 待真机人工验收
2. ~~上传取消为 no-op~~ → 已实现（收尾#3）
3. 老机型 FTP / WebRTC 视频 / CFS 向导 / 对象排除 / 设备分组 / 文件管理——二期
4. 测试二进制的 OpenSSL 符号冲突（paho-mqtt 静态 vs 系统 libcrypto）：
   TLS 相关测试钩子（httpsDisabled）规避，生产路径不受影响

## 5. 测试

- `LanDiscoveryTests`（9）：合成 mDNS 报文解析（PTR RDATA 终止符规则）、
  /info 探测（回环 stub：现代/老机型/非法）、真局域网冒烟钩子
  （`realLanDiscoverySmoke`，打印机上电后即验）
- `MoonrakerDeviceServiceTests`（5）：持久化往返 + 重启存活、未连接诚实语义
  （控制无副作用/发送失败必报）、Moonraker 上传→启动全链（回环 stub）
- canonical 13 + 新增 5 套件全绿

## 6. 提交账

| 里程碑 | 提交 |
|---|---|
| L0 审计解锁 | 2ccb8fa |
| L1 发现+探测 | e0b722d |
| L2 设备服务+装配 | 70a2cb4 |
| L3-L6 UI/打磨 | 见后续提交 |

## 7. UI 对照（CP 设备页）

样式与交互对照 CP 官方设备页（bundle 文案全量提取，1095 条，深色主题
#363638/#CBCBCC/#17CC5F 与 OWzx Theme 同系）。OWzx 监控页现有布局已覆盖
CP 的核心信息结构（设备列表/打印信息/温度/控制/摄像头）。

一期复刻：扫描添加/手动添加按钮（现成）、在线筛选、打印信息卡、温度卡、
打印控制、设备信息。二期记账：设备文件管理、延时摄影、CFS 进退料向导
（完整状态机文案已提取备用）、对象排除、设备分组、腔体温度、错误码映射表
（CP bundle 的 errcode→中文文案全集可直搬）。
