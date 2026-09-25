---
status: resolved
trigger: "Preview 模式下用鼠标拖拽旋转之后模型/G-code 渲染消失"
created: 2026-06-28
updated: 2026-06-28
---

# Debug Session: Preview Orbit Disappears

## Symptoms

- Expected behavior: Preview 模式下鼠标拖拽旋转视角时，已渲染的 G-code 预览应继续可见。
- Actual behavior: 用户拖拽旋转后，中间渲染内容消失。
- Error messages: 未报告错误弹窗或日志错误。
- Timeline: v3.3 QRhi/D3D11 Preview 主流程接入后暴露；上一轮已修复层滑块拖动造成的伪层消失问题。
- Reproduction: 进入 Preview 模式，加载已切片预览，鼠标拖拽旋转视图。

## Current Focus

- hypothesis: Preview 的 RHI 视口没有根据 G-code 包围盒初始化相机，拖拽 orbit 时围绕 Prepare 默认中心/距离旋转，导致路径被旋出视锥或跑到屏幕外。
- test: 添加 QML/UI 审计回归，要求 RhiViewport 在 Preview 数据进入时解析 GCV1 包围盒，并调用 CameraController::fitView 让相机先对准预览路径。
- expecting: 当前代码没有 Preview 数据驱动的 fit 逻辑，测试应先失败。
- next_action: 写 RED 测试并验证失败。
- reasoning_checkpoint: RhiViewport::mouseMoveEvent 左键无选中对象时执行 orbit；PreviewPage 没有 requestFitView 绑定；旧 GCodeRenderer::parsePreviewSegments 会按段包围盒 camera_.fitView。
- tdd_checkpoint: RED verified by canonical verify failing at QML UI audit; GREEN verified by canonical verify passing.

## Evidence

- timestamp: 2026-06-28
  observation: PreparePage 通过 fitHint 调用 requestFitView，PreviewPage 没有等价绑定。
- timestamp: 2026-06-28
  observation: RhiViewportRenderer Preview 分支只上传段数据和相机 uniform，不主动计算/回写相机 fit。
- timestamp: 2026-06-28
  observation: 旧 OpenGL GCodeRenderer::parsePreviewSegments 会计算预览线段包围盒并调用 camera_.fitView。
- timestamp: 2026-06-28
  observation: RED verification failed at QML UI audit because RhiViewport had no Preview-data camera fit hook.
- timestamp: 2026-06-28
  observation: GREEN verification passed after RhiViewport parsed GCV1 preview bounds and called CameraController::fitView.

## Eliminated

- hypothesis: 层高拖动选中了空伪层。
  reason: 已由 z-hop/travel 伪层回归测试修复；当前新症状由鼠标旋转触发。

## Resolution

- root_cause: RHI Preview 视口没有继承旧 GCodeRenderer 的预览线段包围盒 fit 行为；鼠标 orbit 围绕默认 Prepare 相机中心旋转，导致 G-code 路径容易转出视锥。
- fix: RhiViewport 在 CanvasPreview 收到 GCV1 previewData 或切入 Preview 时解析线段包围盒，按渲染坐标映射计算中心/半径，调用 CameraController::fitView 并标记 camera uniform dirty。
- verification: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` 通过；QML UI audit、启动冒烟、E2E pipeline 通过。
- files_changed: tests/QmlUiAuditTests.cpp, src/qml_gui/Renderer/RhiViewport.h, src/qml_gui/Renderer/RhiViewport.cpp
