# Phase 3 Plan — Plater Shared Instance

**Goal:** Prepare 与 Preview 共享单一 `Plater.qml`，通过 `viewMode` 切换内部视图，状态完整保留。

---

## Wave 1 — 03-01: ViewMode 基础设施 + Plater.qml 骨架

**Requirements:** ARCH-06 (viewMode 三选一), ARCH-07 准备 (状态保留验证机制)

### Tasks

1. **BackendContext ViewMode 枚举**
   - 加 `enum class ViewMode { View3D = 0, Preview = 1, AssembleView = 2 };` + `Q_ENUM(ViewMode)`
   - 加 `Q_PROPERTY(int currentViewMode READ currentViewMode NOTIFY currentViewModeChanged)`
   - 加 vm 常量 Q_PROPERTY（同 TabPosition 模式）：`vmView3D` / `vmPreview` / `vmAssembleView`
   - 加 `Q_INVOKABLE void requestChangeViewMode(int mode);` + signal `currentViewModeChanged()` + `viewModeChangeRequested(int)`
   - 联动：`requestSelectTab(tp3DEditor)` → 自动设 View3D；`requestSelectTab(tpPreview)` → 自动设 Preview
   - 默认 `currentViewMode_ = View3D`

2. **Plater.qml 骨架**
   - 新建 `src/qml_gui/pages/Plater.qml`，Item 根，含两个 GLViewport 占位（visible 绑定 viewMode）
   - required property: `editorVm`, `previewVm`, `configVm`
   - required property: `int viewMode`（绑定 `backend.currentViewMode`）
   - 内部两个子 Item：`view3DSlot`（visible: viewMode == vmView3D）、`previewSlot`（visible: viewMode == vmPreview）
   - 各 slot 内先放空 Item 占位（Wave 2 才填内容）
   - 注册到 qml.qrc

3. **单元测试（ViewModelSmokeTests.cpp）**
   - `testViewModeEnumValues()` — 验证 View3D=0/Preview=1/AssembleView=2 + Q_ENUM 注册
   - `testCurrentViewModeDefault()` — 默认 View3D
   - `testRequestChangeViewModeSignal()` — QSignalSpy currentViewModeChanged + viewModeChangeRequested
   - `testTabSelectDrivesViewMode()` — requestSelectTab(tp3DEditor)→View3D；requestSelectTab(tpPreview)→Preview；其他 tab 不改 viewMode

4. **Wave 1 验证**
   - 构建通过
   - ViewModelSmokeTests 全部通过
   - QML 启动无 warning

### Wave 1 出口

- BackendContext 暴露 ViewMode 给 QML
- Plater.qml 骨架存在且 viewMode 驱动 visible（空内容）
- tab → viewMode 联动验证通过

---

## Wave 2 — 03-02: 内容整合 + main.qml 重构 + 状态保留验证

**Requirements:** ARCH-05 (单实例共享), ARCH-07 (状态保留)

**Depends on:** Wave 1 完成

### 务实方案（采用）：Plater 作为 wrapper 容器

**背景**：PreparePage.qml 3583 行（含 LeftSidebar + GLViewport + 15+ gizmo 浮层 + 5 个 Dialog/Component），PreviewPage.qml 也独立持有 GLViewport。**完全重写进 Plater.qml 风险极高、回归面大**。

**决策**：Plater.qml 不重写内容，而是作为**容器/协调器**：
- 内部同时持有 PreparePage + PreviewPage 两个实例（都常驻，靠 visible 切换）
- viewMode 驱动哪个 Page visible（View3D→PreparePage, Preview→PreviewPage）
- 两个 Page 实例都不销毁 → 满足 ARCH-07 状态保留
- 单一 Plater 容器被 Prepare/Preview tab 共享 → 满足 ARCH-05
- viewMode 三选一切内部视图 → 满足 ARCH-06

**对上游契约的偏离说明**：上游 Plater 是 view3D/preview 两个独立 canvas 共享 model；本方案是两个 Page（各自 canvas）常驻。功能等价（状态保留 + 视图切换），实现路径不同，记入 CONTEXT 作为已知差异。未来若要真正共享单 canvas，需 Phase 6+ 把 GLViewport 提到 Plater 级。

### Tasks

1. **Plater.qml 改造为 wrapper**
   - 移除 Wave 1 占位 Text
   - view3DSlot 内放 `PreparePage { editorVm; configVm; visible: viewMode==vmView3D }`
   - previewSlot 内放 `PreviewPage { previewVm; visible: viewMode==vmPreview }`
   - assembleSlot 保留占位
   - 暴露 alias 给 main.qml：`preparePageRef` / `previewPageRef`（保持 BBLTopbar 接线）

2. **main.qml StackLayout 重构**
   - slot 1 (tp3DEditor)：放 `Plater { viewMode: backend.currentViewMode; ... }`
   - slot 2 (tpPreview)：放空 `Item {}` 占位
   - **关键**：修改 `currentIndex` 绑定——当 currentPage ∈ {tp3DEditor, tpPreview} 时都映射到 slot 1（Plater），viewMode 由 currentPage 联动（已在 Wave 1 的 BackendContext 实现）
   - 用一个 helper 函数 `platerSlotIndex(page)` 在 QML 内做映射

3. **BackendContext 联动调整**
   - `requestSelectTab(tpPreview)` 仍设 currentPage=tpPreview（让 UI 上 tab 高亮正确）
   - 但 main.qml 的 StackLayout currentIndex 用映射函数，把 tpPreview 也指向 slot 1
   - viewMode 联动已在 Wave 1 完成

4. **切片完成自动加载 G-code**
   - PreviewViewModel 已有 loadFromSliceResult
   - 在 Plater.qml 监听：viewMode 切到 Preview 时，若 editorViewModel 有切片结果，触发 previewVm.loadFromSliceResult()
   - 或 PreviewPage 内部已有此逻辑（检查后决定）

5. **状态保留验证**
   - 切换 Prepare→Preview→Prepare：对象列表、选择、gizmo 状态全保留（两 Page 实例不销毁）
   - QSignalSpy：viewMode 切换期间 editorViewModel 不发 modelChanged（Wave 1 测试已覆盖 tab 联动，此处补 viewMode 直接切换的验证）

6. **Wave 2 验证**
   - 完整构建通过
   - E2EWorkflowTests + ViewModelSmokeTests 通过
   - QML 启动无 warning
   - 手动 UAT：Prepare/Preview 切换状态保留（记录到 VERIFICATION.md）

### Wave 2 出口

- Plater.qml 作为单实例容器，内部常驻 PreparePage + PreviewPage
- main.qml slot 1 承载 Plater，tp3DEditor/tpPreview 都映射到 slot 1
- 状态保留验证通过（两 Page 不销毁）
- Phase 3 Success Criteria 1/2/3/4/5 达成（6 由单测覆盖）

---

## Risks

- **R1（中）**：QML StackLayout + 单实例多 tab 的视觉处理复杂 → 用 currentPage 联动 viewMode、StackLayout 留在 slot 1 的方案规避
- **R2（低）**：GLViewport visible:false 行为 → Wave 1 单测验证
- **R3（低）**：main.qml BBLTopbar 对 preparePage ref 的依赖 → 保留 Plater 的 id 和对外接口

## Verification

- `scripts/auto_verify_with_vcvars.ps1` 完整构建
- ViewModelSmokeTests（新增 4 个 viewMode 用例）
- E2EWorkflowTests 不回归
- QML 启动无 warning（`FramelessDialogDemo.exe` 启动观察）

## Out of Scope（Phase 3 不做）

- Sidebar Dockable（Phase 4）
- Prepare Sidebar 八大区块（Phase 5）
- GLCanvas 工具栏系统（Phase 6）
- AssembleView 真实功能（v2.0 占位）
