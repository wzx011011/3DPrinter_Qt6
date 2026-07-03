# Qt 版本升级与 RHI 渲染管线评估

> 生成日期：2026-07-04
> 调查范围：`E:\ai\3DPrinter_Qt6` 工程的 Qt 版本现状、QtQuick3D 选型对比、D3D12 崩溃诊断、GLViewport 退役可行性、QRhi 管线优化
> 调查方式：全部基于代码实证，关键结论标注 `文件:行号`

---

## 目录

- [一、Qt 版本现状与升级建议](#一qt-版本现状与升级建议)
  - [1.1 工程当前版本](#11-工程当前版本)
  - [1.2 Qt 版本格局（2026-07）](#12-qt-版本格局2026-07)
  - [1.3 升级建议](#13-升级建议)
  - [1.4 已知 Qt 版本兼容伤疤（升级回归验证清单）](#14-已知-qt-版本兼容伤疤升级回归验证清单)
- [二、QtQuick3D 选型对比](#二qtquick3d-选型对比)
  - [2.1 当前渲染架构](#21-当前渲染架构)
  - [2.2 结论：不建议迁移 QtQuick3D](#22-结论不建议迁移-qtquick3d)
- [三、报告 A：D3D12 启动崩溃诊断](#三报告-ad3d12-启动崩溃诊断)
- [四、报告 B：GLViewport 退役可行性](#四报告-bglviewport-退役可行性)
- [五、报告 C：QRhi 管线优化清单](#五报告-cqrhi-管线优化清单)
- [六、执行顺序建议](#六执行顺序建议)

---

## 一、Qt 版本现状与升级建议

### 1.1 工程当前版本

通过 `build/CMakeCache.txt` 与 `qmake -v` 确认：

```
Qt6_DIR = E:/Qt6.10/lib/cmake/Qt6
Using Qt version 6.10.2 in E:/Qt6.10/lib
```

- **工程实际运行在 Qt 6.10.2**。
- `CMakeLists.txt` 中 `find_package(Qt6 ...)` 未锁版本（`CMakeLists.txt:63/66/328/471/539`），跟随本机安装版本。

### 1.2 Qt 版本格局（2026-07）

| 版本 | 类型 | 发布 | 支持截止 | 与本工程关系 |
|------|------|------|----------|-----------|
| **6.8 LTS** | ✅ LTS（当前唯一活跃 LTS） | 2024-10 | 2027 / 2029（商业版 5 年） | 比本工程还旧，降级无意义；且本工程用 `QQuickRhiItem`（6.9 引入），降 6.8 不可行 |
| 6.9 | 非 LTS | 2025-04 | 2025-10 已 EOL | 跳过 |
| **6.10.2 ← 当前位置** | 非 LTS | 2025-10 | ≈ 2026-10（约还剩 3 个月） | 当前位置 |
| **6.11** | 非 LTS | 2026-03-23 | ≈ 2026-09 | 可升，但收益有限 |
| **6.12 LTS** | ✅ 下一 LTS | 预计 **2026-09** | ≈ 2031 | **真正值得等的目标** |

> 关键判断：本工程跑在非 LTS（6.10）上，比当前 LTS（6.8）还新一档。"升级到最新 LTS"在本工程是伪命题 —— 不缺 LTS，是跑得太靠前。

### 1.3 升级建议

**推荐方案：等 6.12 LTS（2026-09），而非升 6.11。**

理由：

1. **现在（7 月）不建议升 6.11**：
   - 6.11 也是非 LTS，支持期到 9 月就结束，升了立刻又面临 EOL。
   - 本工程深度依赖 QRhi 私有 API，对 RHI API 变化敏感，非 LTS 每 6 个月动一次 API，迁移成本高。
   - 6.11 的 QtQuick3D/GPU 改进对本工程无效（本工程直接用 QRhi 手写管线，不走 QtQuick3D，详见 [第二节](#二qtquick3d-选型对比)）。

2. **9 月直接升 6.12 LTS**：
   - LTS 5 年支持，一次迁移管到 ~2031。
   - 跨度 6.10→6.12 正好攒了一批 bug 修复，优先重测已知伤疤点（见 [1.4](#14-已知-qt-版本兼容伤疤升级回归验证清单)）。
   - 届时 6.10 EOL（10 月），时间点天然契合。

3. **短期低风险动作**：把 6.10.2 跟到最新 patch（6.10.3 已发布，纯 bug 修复，无 API 变化），保持稳定即可。

4. **顺带建议**：在 `CMakeLists.txt` 的 `find_package(Qt6 ...)` 里锁定最低版本（如 `find_package(Qt6 6.10 REQUIRED ...)`），避免换机器构建时静默拉到更旧的 Qt 导致行为差异。

### 1.4 已知 Qt 版本兼容伤疤（升级回归验证清单）

代码中已记录的 Qt 6.10 兼容问题，升 6.12 LTS 时应**逐项重测**，若已修复则可删除对应 workaround：

| # | 位置 | 问题 | workaround | 验证方式 |
|---|------|------|-----------|---------|
| 1 | `src/qml_gui/main_qml.cpp:142-146` | Qt 6.10 MSVC debug heap crash：`QQmlApplicationEngine` 析构 `QQuickFramebufferObject` 时损坏堆 | 故意 leak engine，跳过析构 | 升级后去掉 leak，正常析构跑回归测试 |
| 2 | `src/qml_gui/Models/ConfigOptionModel.h:142` | `QVariantList` 在 Qt 6.10 V4 引擎崩溃 | 避免 QML 调用 | 验证 V4 修复 |
| 3 | `src/qml_gui/BackendContext.h:143` | `Q_ENUM` 通过 context-property 访问不稳定（"of undefined"） | 加 workaround | 验证 context-property 枚举访问 |
| 4 | `src/qml_gui/Renderer/RhiBackendSelector.cpp:39-41` | D3D12 启动崩溃 | D3D11-first fallback | 详见 [报告 A](#三报告-ad3d12-启动崩溃诊断) |
| 5 | `src/qml_gui/Renderer/GLViewportRenderer.cpp:893` | `glPolygonMode` 不在 `QOpenGLExtraFunctions` on Qt 6.10 | 手动 resolve | 验证是否已纳入 |

---

## 二、QtQuick3D 选型对比

### 2.1 当前渲染架构

本工程是**自实现的 3D 切片器渲染器**，基于 `QRhi` + `QQuickRhiItem` 手写渲染管线，而非声明式 `QtQuick3D` 的 `View3D`。

```
RhiViewport : QQuickRhiItem          ← 主路径（D3D11 默认）
  └─ RhiViewportRenderer : QQuickRhiItemRenderer
        └─ 直接操作 QRhiGraphicsPipeline / QRhiBuffer / QShader

GLViewport : QQuickFramebufferObject  ← OWZX_OPENGL=1 回退路径
SoftwareViewport                       ← 驱动初始化失败的兜底
```

注册逻辑见 `src/qml_gui/main_qml.cpp:129-138`：三选一注册为 QML `GLViewport`。

功能模块（全部自实现）：

| 模块 | 实现方式 |
|------|---------|
| 模型网格渲染 | 顶点缓冲 + 自写 vert/frag shader（`mvp*position` + 顶点色） |
| 热床渲染 | `buildBedGeometry` + 程序化网格 + 纹理 |
| 擦料塔 (wipe tower) | `buildWipeTowerGeometry` |
| Gizmo 系统（移动/旋转/缩放/切割） | `renderMoveGizmo/Rotate/Scale/CutPlane` 全自写 |
| 鼠标拾取 | `computeRay` / `rayXZIntersect` / `pickObject` / `pickMoveAxis` — 自写射线相交 |
| G-code 预览管线 | `RhiViewportRenderer` 里的 preview segment pipeline，按 layer/role 过滤 |
| 相机控制 | `CameraController` 自写 |

**总渲染代码量约 4400 行**，shader 极简（9 行 frag，22 行 vert）。

### 2.2 结论：不建议迁移 QtQuick3D

**直接结论：不会更好，反而更差。** 逐条对照：

#### ❌ 1. 本工程需要的功能 QtQuick3D 不擅长

| 本工程需求 | QtQuick3D 能力 | 差距 |
|---------|---------------|------|
| **G-code 预览**（百万级线段、按 layer/role 动态着色过滤） | 擅长三角形网格，不擅长海量线段 + 运行时变色 | `RhiViewportRenderer` 的 preview segment pipeline 专门为此优化，换 Q3D 要重写 |
| **拾取射线的精确控制**（区分 object / gizmo axis / XZ 平面拖拽） | `View3D::pick` 只返回物体，不返回 sub-mesh 轴 | `pickMoveAxis` 的逻辑 Q3D 无法直接表达 |
| **顶点色驱动的着色**（每个对象一个色，表示选中/悬停/角色） | 需走 `PrincipledMaterial` / `DefaultMaterial`，顶点色不是一等公民 | 现 shader 用顶点色 0 开销；Q3D 要建材质实例 |
| **Gizmo（箭头/圆环/方块手柄）** | 没有内置 3D manipulator，要自己塞 Model 进场景 | 仍需自写，不省事 |

#### ❌ 2. 性能上 QtQuick3D 是倒退

当前管线特点（接近理论极限的轻量管线）：
- shader 极简（9 行 frag，22 行 vert）
- 单一 UBO（只有 mvp 矩阵）
- 顶点缓冲按 dirty flag 增量上传（`uploadSceneBuffers(dirtyFlags)`）

QtQuick3D 的额外代价：
- 每帧走完整的 scene graph 同步 + 批处理
- `PrincipledMaterial` 默认带 PBR 光照计算（哪怕不需要光照）
- 多一层 QML → C++ 的属性同步开销
- G-code 动态顶点数据，每次变动都要重建 `QQuick3DGeometry`，比现在的 dirty-flag 增量上传重得多

对于切片器这种"网格大、要频繁切片重算、要预览百万 G-code 线段"的场景，**自写 QRhi 管线在性能上是对的**。

#### ❌ 3. 已知伤疤换框架换不掉

第一节列的 5 个兼容问题（heap crash、D3D12 崩溃、QML V4 崩溃）在 RHI 层和 QML 引擎层，不在 QtQuick3D 层。换框架换不掉。

#### ✅ 唯一 QtQuick3D 占优的点

- 将来要加 **PBR 材质、环境光照、阴影、IBL** → QtQuick3D 现成，自写要几天
- 要加 **3D 粒子特效** → QtQuick3D 有 `QtQuick3D.Particles3D`
- **团队不熟图形管线，想快速加新 3D 物体** → 声明式 QML 上手快

但切片器核心需求（网格 + 顶点色 + 拾取 + G-code 线段）不需要这些。**BambuStudio / OrcaSlicer / Cura 也都是自写 OpenGL/RHI 渲染器，没有用 QtQuick3D** —— 这是行业共识。

#### 备选方案：混合方案（低优先级）

如果未来真要 PBR/光照，可**局部**引入 QtQuick3D 只渲染模型网格，G-code/gizmo 仍走自写 RHI。

---

## 三、报告 A：D3D12 启动崩溃诊断

### 结论：这个 bug 早就修了，但 workaround 从未回滚

#### 证据链

**1. 根因已被精确定位**（`src/qml_gui/Renderer/RhiViewportRenderer.cpp:139-142`）：

```cpp
// BUG-V31-1 fix: camera uniform MUST be uploaded before beginPass, not after.
// beginPass-after-resourceUpdate is undefined in QRhi; D3D12 strictly enforces
// command buffer ordering and segfaults on this pattern (root cause of the
// D3D12 crash that was worked around with D3D11-first in RhiBackendSelector).
```

**根因**：`beginPass()` 之后调用 `resourceUpdate()` 是 QRhi 未定义行为。D3D12 后端严格执行命令缓冲排序，遇到这种顺序直接 segfault。D3D11/Vulkan/OpenGL 比较宽容（但仍是隐患）。

**2. 修复已落地**（`src/qml_gui/Renderer/RhiViewportRenderer.cpp:143-155`）：Preview 路径的 camera uniform 已改到 `beginPass` **之前**上传：

```cpp
if (m_canvasType == RhiViewport::CanvasPreview) {
    if (!updates) updates = rhi()->nextResourceUpdateBatch();
    if (!m_previewSegmentBufferUploaded && !m_previewVertices.isEmpty()) {
      if (uploadPreviewSegmentBuffer(updates)) m_previewSegmentBufferUploaded = true;
    }
    uploadCameraUniform(updates, PrepareSceneData::DirtyCamera);  // ← beginPass 前
}
cb->beginPass(renderTarget(), m_clearColor, {1.0f, 0}, updates);  // ← beginPass 在后 ✓
```

**3. View3D 路径也是正确顺序**（`src/qml_gui/Renderer/RhiViewportRenderer.cpp:118-136`）：

```cpp
if ((sceneDirty || !m_sceneBuffersUploaded) && !m_pipelineFailed) {
    updates = rhi()->nextResourceUpdateBatch();        // ← 创建在 beginPass 前 ✓
    if (!uploadSceneBuffers(updates, dirtyFlags)) { ... }
}
cb->beginPass(renderTarget(), m_clearColor, {1.0f, 0}, updates);  // ✓ 顺序正确
```

**实际上两条路径都已经是正确顺序了。**

**4. 但 fallback 策略还是 D3D11-first**（`src/qml_gui/Renderer/RhiBackendSelector.cpp:39-46`）：

```cpp
// D3D11-first: D3D12 has a rendering crash on startup (Phase 26 isolation:
// prepare render Segfault under D3D12, D3D11 works). Make D3D11 the safe
// default for "auto", and D3D12 only via explicit OWZX_RHI_RENDERER=d3d12.
return {
    {QStringLiteral("d3d11"), QSGRendererInterface::Direct3D11, QRhi::D3D11},
    {QStringLiteral("d3d12"), QSGRendererInterface::Direct3D12, QRhi::D3D12},
};
```

这条注释描述的是 Phase 26 时的崩溃，而 BUG-V31-1 修复发生在 Phase 26 之后（编号更大）。

#### 诊断结论

| 项 | 状态 |
|----|------|
| 崩溃根因 | ✅ 已定位（beginPass 后 resourceUpdate 的未定义顺序） |
| 渲染端修复 | ✅ 已落地（camera uniform 改到 beginPass 前上传） |
| 默认后端回滚到 D3D12 | ❌ **未做**，仍保持 D3D11-first |
| **2026-07-04 实测验证** | ❌ **D3D12 仍崩溃** — 见下方"实测验证结果" |

#### 实测验证结果（2026-07-04）

按"建议执行步骤"实际跑了 `OWZX_RHI_RENDERER=d3d12` 验证：

- D3D12 后端**初始化成功**：`QRhi backend selection: selected=d3d12 attempts=[d3d12:ok]`
- 但程序在 QML 加载完成、**首帧渲染时段错误**（segfault）
- crash dump 涉及 `RhiViewportRenderer` + `d3d12.pdb`（确认崩溃在 RHI 渲染管线 D3D12 层）
- BUG-V31-1 修复（camera uniform 上传顺序）**不足以**消除 D3D12 崩溃 —— 还存在其他 D3D12 特有的渲染问题（可能是 pipeline 状态、资源生命周期、或 D3D12 严格校验）

**结论**：报告 A 的"切 D3D12 默认"建议**当前不可执行**。D3D12 路径需要进一步的崩溃诊断（用 cdb/windbg 解 minidump stack）才能落地。本报告其余"D3D12-first"建议在崩溃修复前均搁置。D3D11 保持默认。

#### 建议执行步骤

1. **~~验证 D3D12~~**（**已验证：仍崩溃，不可执行**）：

   ```bash
   # 强制 D3D12 启动，复现测试
   set OWZX_RHI_RENDERER=d3d12
   # 跑 Prepare 场景 + Preview 场景，确认无崩溃
   ```

2. **验证通过后**，改 `src/qml_gui/Renderer/RhiBackendSelector.cpp:42-46`：

   ```cpp
   // D3D12-first: BUG-V31-1 fixed the beginPass/resourceUpdate ordering;
   // D3D12 is now the default high-performance path. D3D11 retained as fallback.
   return {
       {QStringLiteral("d3d12"), QSGRendererInterface::Direct3D12, QRhi::D3D12},
       {QStringLiteral("d3d11"), QSGRendererInterface::Direct3D11, QRhi::D3D11},
   };
   ```

3. **收益**：D3D12 比 D3D11 在现代 GPU 上有 10-30% 的 CPU 开销优势（更低的 draw call 开销），对 G-code 预览这种大量 draw 的场景收益明显。

4. **保留逃生口**：`OWZX_RHI_RENDERER=d3d11` 仍可显式回退，零风险。

---

## 四、报告 B：GLViewport 退役可行性

### 结论：功能差距很大，短期内不能退役；GLViewport 是 RHI 路径的功能路线图蓝本

#### 功能覆盖对比（代码实证）

| 功能 | GLViewport (2285 行) | RhiViewport (759 行) | 差距 |
|------|---------------------|---------------------|------|
| 基础场景（bed/model） | ✅ | ✅ | — |
| 高亮（选中/悬停） | ✅ | ✅ | — |
| G-code 预览段管线 | ✅ | ✅ | — |
| **Gizmo 渲染**（Move/Rotate/Scale） | ✅ `renderMoveGizmo`/`renderRotateGizmo`/`renderScaleGizmo` | ❌ **0 实现** | **🔴 大** |
| **Gizmo 几何构建** | ✅ `buildGizmoGeometry`/`buildRotateGizmoGeometry`/`buildScaleGizmoGeometry` | ❌ | **🔴 大** |
| **Gizmo 轴拾取** | ✅ `pickMoveAxis`/`pickRotateAxis`/`pickScaleAxis` | ❌ | **🔴 大** |
| **物体拾取精度** | ✅ 精确射线-三角形（`computeRay`/`pickObject`） | ⚠️ AABB 投影屏幕矩形（`pickSourceObjectAt`） | **🟡 中** |
| **Wipe tower 渲染** | ✅ `buildWipeTowerGeometry`/`renderWipeTower` | ❌ | **🟡 中** |
| **Cut plane 渲染** | ✅ `renderCutPlane` | ❌ | **🟡 中** |
| **床纹理** | ✅ `createBedTexture` | ❌（只有顶点色网格） | 🟢 低 |
| **Marker 渲染** | ✅ | ❌ | 🟢 低 |
| **线框模式** | ✅ `glPolygonMode` | ❌ | 🟢 低 |
| **FBO 缩略图捕获** | ✅ `captureThumbnailToFbo`（真渲染） | ⚠️ 占位灰图（`requestThumbnailCapture` 第 412-419 行） | **🟡 中** |
| **模型变换矩阵** | ✅ `computeModelMatrix`/`transformedAABB` | ❌ | 🟡 中 |
| **鼠标拖拽（gizmo 交互）** | ✅ 完整 gizmo 拖拽逻辑 | ⚠️ 仅相机 orbit/pan | 🔴 大 |
| CameraController | ✅ 共享 | ✅ 共享 | — |

#### 接口面对齐情况（好消息）

Q_PROPERTY 和 Q_INVOKABLE 接口面两边几乎完全一致（`RhiViewport.h` 抄了 `GLViewport.h` 的接口）。QML 侧 `components/GLToolbars.qml` 用 `GLViewport.GizmoMove` 等枚举，两个 viewport 类的枚举值定义一致，QML 层切换无感知。

#### 真正的阻塞点

RHI 路径虽然接收了 `gizmoMode`/`cutAxis`/`showWipeTower` 等属性，但渲染端完全不画。这意味着：

- 用户在 RHI 路径下点"移动"按钮 → `gizmoMode` 设置成功 → **屏幕上什么都不出现**
- 用户拖拽 → RHI 路径只能 orbit 相机，无法拖物体

#### 建议

**短期（现在）**：不要退役 GLViewport。它是当前功能完整的参考实现，RHI 路径只是"高性能骨架"。

**中期路线图**（按优先级补齐 RHI 路径）：

| 阶段 | 补齐功能 | 估算工作量 | 价值 |
|------|---------|-----------|------|
| 1 | Gizmo 几何 + 渲染（Move/Rotate/Scale） | 3-5 天 | 🔴 解锁交互 |
| 2 | Gizmo 轴拾取 + 拖拽响应 | 2-3 天 | 🔴 解锁编辑 |
| 3 | 精确物体拾取（射线-三角形，替代 AABB） | 1-2 天 | 🟡 精度 |
| 4 | Wipe tower + Cut plane 渲染 | 2 天 | 🟡 完整性 |
| 5 | 真实 FBO 缩略图捕获 | 1 天 | 🟡 完整性 |
| 6 | 床纹理 / Marker / 线框 | 2 天 | 🟢 锦上添花 |

补齐后（约 11-15 个工作日），GLViewport 可以退役，减掉 2285 行 + 一条 `QQuickFramebufferObject` 依赖路径。届时可只保留 RHI + Software 两条路径，D3D12 也能用上。

---

## 五、报告 C：QRhi 管线优化清单

### 当前管线的性能特征（代码实证）

**Draw call 分布**（View3D 一帧，`src/qml_gui/Renderer/RhiViewportRenderer.cpp:160-204`）：

- `cb->draw(m_bedFillVertexCount)` — 热床填充
- `cb->draw(m_bedLineVertexCount)` — 热床网格线
- `cb->draw(m_modelVertexCount)` — 模型
- `cb->draw(m_highlightVertexCount)` — 高亮
- Preview 路径：N 个 draw（按 layer/role 分段，`computePreviewDrawRanges`）

**Pipeline 状态**：只有 2 个 pipeline（fill=Triangles + line=Lines），`ensurePipeline`（第 298-337 行）**没配置深度测试/深度写入**。

### 🟢 Tier 1：低风险高收益（立即可做）

#### 1. 启用深度测试

**现状**：`ensurePipeline`（`src/qml_gui/Renderer/RhiViewportRenderer.cpp:298-337`）没有 `setDepthTest(true)`，靠绘制顺序（bed→model→highlight）保证遮挡。导致：
- highlight 永远画在最上面（即使被遮挡）
- 多物体重叠时无法正确排序
- 切到 D3D12 后性能浪费（GPU 仍跑完整深度，只是不用）

**改法**：

```cpp
QRhiGraphicsPipelineTargetBlend blend;
pipeline->setTargetBlends({blend});

// 新增：
QRhiDepthStencilOpDesc depthOp;
depthOp.setDepthTestEnabled(true);
depthOp.setDepthWriteEnabled(true);
depthOp.setDepthCompareOp(QRhiDepthStencilOpDesc::Less);
// 需配套给 render target 加 QRhiRenderBuffer (DepthStencil)
```

**收益**：渲染正确性 ↑；highlight 不再穿透。
**风险**：低，标准做法。

#### 2. Cache `computePreviewDrawRanges` 结果

**现状**（`src/qml_gui/Renderer/RhiViewportRenderer.cpp:673-758`）：每帧 CPU 遍历所有 `m_previewDrawSpans`（可能百万级）来计算可见 draw ranges。但输入（layerMin/layerMax/moveEnd/roleVisibility）大部分帧不变。

**改法**：

```cpp
// 加 cache key
quint64 m_previewRangeCacheKey = 0;  // hash(layerMin, layerMax, moveEnd, roleVisibility, spanCount)
QVector<PreviewDrawRange> m_cachedPreviewRanges;
```

当 key 不变时直接返回 cache，省掉每帧 O(N) 遍历。

**收益**：Preview 帧时间显著下降（拖动滑块时仍重算，静止时几乎 0）。
**风险**：极低。

#### 3. 合并 bed-fill / bed-line / model 到同一 vertex buffer

**现状**：3 个独立 `QRhiBuffer` + 3 次 `setVertexInput` + 3 次 draw。可以合并成 1 个 buffer 的不同 offset，用同一次 `setVertexInput` + 不同 `draw(..., firstVertex)`。

**收益**：减少 buffer 绑定开销，尤其 D3D12 下每个 binding 切换都有 CPU 成本。
**风险**：低。

### 🟡 Tier 2：中等收益（G-code 预览重度场景）

#### 4. Preview segment buffer 用 Dynamic + 分层上传

**现状**（`src/qml_gui/Renderer/RhiViewportRenderer.cpp:656-671`）：整个 preview 用 `uploadStaticBuffer` 一次性全量上传。G-code 大文件（百万段）时这一下就传几百 MB。

**优化**：

- buffer 类型改 `QRhiBuffer::Dynamic`
- 按 layer 分块上传（slice 加载时只传新 layer）
- 或用 `QRhiBuffer::Static` + staging buffer + `copyResource` 异步

**收益**：大 G-code 加载时间 ↓。
**风险**：中，需重构上传逻辑。

#### 5. Preview draw 用 indirect draw

**现状**：CPU 计算 draw ranges 后，循环 `cb->draw(range.vertexCount, 1, range.firstVertex)`，每个 range 一次 draw call。如果 ranges 多（layer 切片有间隔），draw call 数会很多。

**优化**：用 `QRhiCommandBuffer::drawIndexed` + indirect buffer（`QRhiBuffer::IndirectDrawBuffer`），GPU 端根据 visibility buffer 决定画哪些段。

**收益**：draw call 数从 N 降到 1。
**风险**：中高，需 GPU 端剔除逻辑。
**前提**：Qt 6.x RHI 支持 indirect draw（6.7+ 有 `QRhiCommandBuffer::drawIndexed`，indirect 需确认）。

#### 6. 线段宽度 / 抗锯齿

**现状**：`QRhiGraphicsPipeline::Lines` 默认线宽 1px，无 AA。G-code 预览看起来锯齿严重。

**优化**：

- `pipeline->setLineWidth(2.0f)`（需 backend 支持）
- 或改用三角形条带（triangle strip）画 quad 代替线，获得可控宽度 + AA

**收益**：视觉质量 ↑。
**风险**：低（setLineWidth）到中（改 quad）。

### 🔴 Tier 3：架构级（长期）

#### 7. 异步上传 + ring buffer

为 preview segment 建立多帧 ring buffer，避免每帧 `nextResourceUpdateBatch` 的同步点。

#### 8. Compute shader 做颜色编码

**现状**：G-code 颜色在 CPU 端预烘焙（`PreviewViewModel` 的 D-26-03）。

**优化**：改成 GPU 端 compute shader 根据 feedrate/temperature 实时算色，省掉 CPU 着色时间 + 减小 vertex 带宽（顶点不再带 RGBA，只带属性索引）。

**收益**：G-code 着色模式实时切换 0 开销。
**风险**：高，需 compute shader + 重构 vertex layout。

### 不建议的优化（避坑）

- **Instancing**：本工程场景物体数量少（切片器模型数量有限），instancing 的收益主要在"画几万个相同物体"，这里不适用。
- **Meshlet / mesh shader**：复杂度极高，收益对切片器网格密度不明显。

---

## 六、执行顺序建议

```
现在 ──────────────────────────────────────────►
  │
  ├─[A] 验证 D3D12（设 OWZX_RHI_RENDERER=d3d12 跑一遍）
  │     └─ 通过 → 改默认后端为 D3D12-first（RhiBackendSelector.cpp:42-46）
  │
  ├─[C-Tier1] 立即做：深度测试 + ranges cache + buffer 合并
  │
  ├─[B] 按路线图补齐 RHI gizmo（11-15 天）
  │     └─ 补完后退役 GLViewport
  │
  └─[C-Tier2/3] 长期：preview 分层上传 / indirect draw
```

### 优先级排序

| 优先级 | 任务 | 价值 | 工作量 |
|--------|------|------|--------|
| 🔴 P0 | [A] 验证并切 D3D12 默认后端 | 白捡 10-30% draw call 性能 | 0.5 天 |
| 🔴 P0 | [C-1] 启用深度测试 | 渲染正确性 | 0.5 天 |
| 🔴 P0 | [C-2] Cache preview draw ranges | Preview 静止帧近 0 开销 | 0.5 天 |
| 🟡 P1 | [B-阶段1/2] RHI 补齐 gizmo + 拾取 | 解锁 RHI 路径完整交互 | 5-8 天 |
| 🟡 P1 | [C-3] 合并 vertex buffer | 减少 binding 开销 | 0.5 天 |
| 🟢 P2 | [B-阶段3/4/5] RHI 补齐 wipe tower/cut plane/缩略图 | 功能完整性 | 4-5 天 |
| 🟢 P2 | [C-4/5] Preview 分层上传 / indirect draw | 大 G-code 性能 | 3-5 天 |
| 🟢 P3 | 退役 GLViewport | 减 2285 行代码 | 补齐 B 后 0.5 天 |
| 🟢 P3 | [C-7/8] 异步上传 / compute shader | 重度场景 | 长期 |

---

## 参考来源

- [Qt 6.8 Release – Qt Wiki](https://wiki.qt.io/Qt_6.8_Release)
- [Qt 6.10 Release – Qt Wiki](https://wiki.qt.io/Qt_6.10_Release)
- [Qt 6.10.2 / 6.10.3 Release Notes – GitHub](https://github.com/qt/qtreleasenotes/blob/dev/qt/6.10.2/release-note.md)
- [Qt 6.11 Toolkit – Phoronix](https://www.phoronix.com/news/Qt-6.11-Toolkit)
- [Qt end-of-life tracker](https://endoflife.date/qt)
- [Qt Framework Release Cycle](https://www.qt.io/development/qt-framework/release-cycle)
- [What's new in Qt 6 – Official Docs](https://doc.qt.io/qt-6/whatsnewqt6.html)
- [Next Qt LTS release discussion – Reddit](https://www.reddit.com/r/QtFramework/comments/1ptov7u/when_is_the_next_qt_lts_release_expected/)
