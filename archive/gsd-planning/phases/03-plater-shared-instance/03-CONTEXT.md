# Phase 3 Context — Plater Shared Instance

**Phase:** 3
**Goal:** Prepare 与 Preview 共享单一 `Plater.qml` 组件实例，通过 `viewMode` 切换内部 View3D↔Preview 视图，保留全部状态。
**Depends on:** Phase 2（TabPosition / requestSelectTab 基础设施）
**Requirements:** ARCH-05, ARCH-06, ARCH-07
**Mode:** Standard / auto (yolo)

---

## Upstream Truth

- `third_party/OrcaSlicer/src/slic3r/GUI/Plater.hpp:275` — `class Plater: public wxPanel`（单一实例）
- `Plater.cpp:4332` — `AssembleView* assemble_view`（三 canvas 设计：view3D / preview / assemble_view）
- `Plater.cpp:4959` — `assemble_view = new AssembleView(panel_3d, ...)`
- `Plater.hpp:428-436` — `is_preview_shown()` / `is_view3D_shown()` / `show_view3D_labels()` / `show_view3D_overhang()`
- `Plater.hpp:607-609` — `get_current_canvas3D()` / `get_view3D_canvas3D()` / `get_preview_canvas3D()`
- `Plater.cpp:11101` — `update_preview_bottom_toolbar()` / `reset_gcode_toolpaths()`（preview canvas 在切片完成后重载）

**上游核心事实：**
- Plater 是**单一 wxPanel 实例**，被 Prepare tab 和 Preview tab 共享
- 内部持有**两个独立 GLCanvas3D**（view3D + preview），不是单 canvas 切模式
- 通过 `show_view3D()` / `show_preview()` 切换**可见性**（非销毁重建）
- 两份 canvas 共享同一份 model / config / undo stack / selection / gizmo manager
- Preview canvas 在切片完成后通过 `reset_gcode_toolpaths` + `load_gcode` 加载

---

## Current State（v1.x / Phase 02 之后）

- `main.qml` 把 `PreparePage` 和 `PreviewPage` 作为**两个独立 QML 组件**放在 StackLayout 不同 slot（第 1/2 slot）
- 各自持有**独立的 `GLViewport`（QQuickFramebufferObject）实例**
- 共享了 ViewModel（editorViewModel / previewViewModel），但 GLViewport 状态独立
- 切 tab 时 StackLayout 隐藏组件但**不销毁**——这是好消息，说明状态本来就不会丢，但**架构不符合上游"单一 Plater 实例"契约**

---

## Design Decision

**方案 C（采用）**：单 `Plater.qml` 实例 + 两个 GLViewport + viewMode 切换可见性

```
StackLayout slot 1 (tp3DEditor) ──┐
                                  ├──► Plater.qml (single instance)
StackLayout slot 2 (tpPreview) ──┘        │
                                          ├── view3D: GLViewport (visible when viewMode==View3D)
                                          ├── preview: GLViewport (visible when viewMode==Preview)
                                          └── assemble: GLViewport (visible when viewMode==AssembleView, v2.0 占位)
```

**QML 实现关键：** 在 StackLayout 里把同一 Plater 实例放两次会失败（QML 同 id 不能多父）。所以实际方案：

- StackLayout 仍保持 9 个 slot，但 slot 1 和 slot 2 **不再各放一个 Page 组件**
- 改为：slot 1 放 `Plater.qml`，slot 2 放一个轻量 `PlaterPreviewSlot.qml`（仅持有 Plater 引用 + 监听 currentPage 联动 viewMode）
- **或更简单**：slot 2 留空 Item 占位，监听 `backend.currentPage` 变化时把 Plater 的 parent 切换 / viewMode 改

**最终采用：Plater 作为 currentPage 驱动的单实例**，由 `BackendContext::currentViewMode` 联动。

## Pitfalls

1. **QML StackLayout 同实例多父问题** —— 用 viewMode 属性切换 + 单 slot 承载，不重复实例化
2. **GLViewport visible:false 是否停渲染** —— QQuickFramebufferObject 在 hidden 时不渲染，切回时自动恢复（Wave 1 单测验证）
3. **EditorVM + PreviewVM 协作** —— Phase 3 不动 VM 结构，两个 VM 并存
4. **AssembleView 在 v2.0 是 Out of Scope** —— viewMode 三选一里 AssembleView 留占位
5. **tabSelectRequested 已存在** —— viewMode 切换挂接到现有 `requestSelectTab`，不重复造信号

## Acceptance（来自 ROADMAP Success Criteria）

1. `Plater.qml` 单一组件实例，被 Prepare tab 和 Preview tab 共享
2. viewMode 属性（View3D/Preview/AssembleView）切换内部三选一显示
3. 切换 Prepare→Preview→Prepare 后：对象列表、选择状态、切片结果、gizmo 状态全部保留
4. 切换不触发 Plater 重建（属性切换，非 Loader source 切换）
5. 切片完成后切到 Preview 自动加载 G-code 数据，无需手动刷新
6. 单元测试：QSignalSpy 验证 viewMode 切换不触发 modelChanged
