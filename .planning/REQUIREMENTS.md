# Requirements: CrealityPrint Qt6/QML Migration

## Active Requirements

### Prepare Workspace

- [ ] **PREP-01**: Prepare 工作区完整对齐 -- 平板管理真实耗材分配、对象树/侧栏细节、工具栏/侧栏视觉打磨
- [ ] **PREP-02**: 右侧面板端到端验证 -- 切片结果摘要、参数面板三段式、排列设置全部在有真实数据时正确展示
- [ ] **PREP-03**: 后台切片状态机对照 -- BackgroundSlicingProcess 状态机与上游对齐

### Preview Workspace

- [ ] **PREV-01**: Preview 工作区完整对照 -- 13 种颜色映射端到端验证、StatsPanel 真实 G-code 数据、LayerSlider/MoveSlider 交互验证

### Settings / Preset

- [ ] **SETT-01**: Settings/Preset 完整继承链 -- 上游 Tab 真实数据加载、PresetBundle 完整继承、ConfigOptionDef 完整 schema

### Project Workflow

- [ ] **PROJ-01**: 项目工作流完善 -- 项目保存/加载完整工作流、文件导入格式支持对齐

### Gizmo Rendering

- [ ] **GIZM-01**: Gizmo GL 渲染层补全 -- Text/Emboss/SVG/Simplify/Drill/MeshBoolean/AdvancedCut 的 GL 交互渲染
- [ ] **GIZM-02**: Support Paint / Seam Paint GL 渲染和数据管理层 -- 非 TriangleSelector 方案
- [ ] **GIZM-03**: Hollow Gizmo GL 渲染和空洞几何生成 -- 非 OpenVDB 方案
- [ ] **GIZM-04**: MmuSegmentation per-triangle 绘制 -- 非 TriangleSelectorPatch 方案

### Device / Monitor

- [ ] **DEVC-01**: 设备交互层对齐 -- DeviceManager/MachineObject 交互层状态机

### Calibration

- [ ] **CALI-01**: 校准工作流深度功能 -- 设备连接/预设选择/真实参数编辑/历史记录

### Model Mall

- [ ] **MALL-01**: 商城 WebView 集成 -- QtWebEngine 替代 wxWebView

### Multi-Machine

- [ ] **MULT-01**: 多机管理真实连接 -- MQTT/SSDP 协议层

### Release Readiness

- [ ] **REL-01**: 发布就绪度 -- 上游行为对照回归基线、全页面回归测试自动化、I18N 完整验证

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| PREP-01 | Phase 1 | Pending |
| PREP-02 | Phase 1 | Pending |
| PREP-03 | Phase 1 | Pending |
| PREV-01 | Phase 3 | Pending |
| SETT-01 | Phase 2 | Pending |
| PROJ-01 | Phase 4 | Pending |
| GIZM-01 | Phase 5 | Pending |
| GIZM-02 | Phase 5 | Pending |
| GIZM-03 | Phase 5 | Pending |
| GIZM-04 | Phase 5 | Pending |
| DEVC-01 | Phase 6 | Pending |
| CALI-01 | Phase 6 | Pending |
| MALL-01 | Phase 7 | Pending |
| MULT-01 | Phase 7 | Pending |
| REL-01 | Phase 8 | Pending |

---
*Last updated: 2026-05-31 after roadmap creation*
