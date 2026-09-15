# 对照报告：通知中心（SHELL-NOTIFY）与 gizmo 浮动面板（PREP-GIZMOPANEL）

> 日期：2026-09-15 ｜ 性质：**代码级对照**（双侧截图采集被 open P0
> AI-SIDECAR-STARTUP-RACE 阻塞——应用在窗口可交互前即堆损坏死亡，
> 退出码 0xC0000374；待沙箱外提权 PageHeap 定位元凶并修复后补采截图）
> 真值：`third_party/OrcaSlicer/src/slic3r/GUI/notification_manager.*`（行为）、
> `Gizmos/GLGizmo*::on_render_input_window`（布局/行为）

## 1. 通知中心（SHELL-NOTIFY）

### 结论：Real（历史列表 UI 完整存在，P1.1 旧结论过时）

| 上游 notification_manager | OWzx | 对照 |
| --- | --- | --- |
| 滚动通知列表（severity 着色 + 时间戳） | `NotificationCenter.qml` ListView：severity 图标/颜色 + 标题 + 消息（3 行截断换行）+ 等宽时间戳 | ✅ 一致 |
| 历史保留 | `m_notificationHistory` QVector 上限 100 条、dismiss 即入史、FIFO 淘汰 | ✅ 一致（上限 100 为 OWzx 取整） |
| 未读管理 | `unreadHistoryCount` 徽标（99+ 封顶）+ `markHistoryRead()` | ✅ 一致 |
| 清理 | `clearHistory()` | ✅ 一致 |
| 入口 | 顶栏铃铛（bell.svg + 未读角标）→ Popup（Esc/点击外部关闭） | ✅ 一致 |
| 通知偏好设置 | PreferencesPage：`notificationsEnabled` / `hintsEnabled` 开关（级联禁用） | ✅ 一致 |
| 持久通知 + 进度（切片进度条通知） | ErrorToast：进度条 + 确认/取消按钮行（P1.1） | ✅ 已承接 |

### 审计锁定
`QmlUiAuditTests` 通知相关断言（历史访问器 `historyMessage/Title/Severity/Time`、
`markHistoryRead/clearHistory`、`unreadHistoryCount` 徽标）随既有套件覆盖；
本轮无代码改动（纯真值同步）。

## 2. gizmo 浮动面板（PREP-GIZMOPANEL）

### 结论：Hybrid（面板已实现并接真实参数；逐面板 parity 复核保留在波次 4）

| 上游 on_render_input_window | OWzx | 对照 |
| --- | --- | --- |
| Advanced Cut：连接器尺寸/深度/等宽等参数 | `connectorSize` 滑条 + 数值显示（PreparePage.qml:3073-3078）+ groove 派生默认 | ✅ 参数真实（EditorViewModel 属性） |
| Emboss：文字高度/深度/字体 | `embossHeight` SpinBox（:4286）+ text/emboss 真实 API（P2.7 已核实） | ✅ 参数真实 |
| Support/Seam 画笔：半径 | `brushRadius` 绑定 `_activeBrushRadius()`（:1819）+ paintAtFacet 全参数链（:1945-1947） | ✅ 参数真实 |
| MMU Segmentation：16 色挤出机选择器 | P2.7 已核实的 16 色色块选择器 + 当前耗材提示 + 清除分段 | ✅ 已承接 |
| 面板形态：浮于画布右侧输入窗 | PreparePage 内嵌面板（按 gizmoMode 条件显示，57 处引用） | ⚠️ 形态差异：内嵌 vs 浮窗——与"内嵌化"已登记决策一致（GizmoObjectManipulation 同模式） |

### 残留（波次 4）
- 逐面板参数与上游 `on_render_input_window` 的字段级 parity 复核
- 浮窗 vs 内嵌的形态差异维持已登记偏离

## 3. 截图债

- OWzx 侧：`docs/visual-compare/owzx_notification_center.png`、`owzx_gizmo_panels.png`（待 P0 修复后采集）
- 上游侧：notification manager 弹出态 + 各 gizmo 输入窗展开态
- 采集前置条件：AI-SIDECAR-STARTUP-RACE 修复（沙箱外提权 PageHeap + PDB 定位元凶）
