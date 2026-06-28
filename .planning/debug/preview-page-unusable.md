---
status: resolved
trigger: "Preview 页主流程不可用；用户反馈预览页根本用不了"
created: 2026-06-28
updated: 2026-06-28
---

# Debug Session: Preview Page Unusable

## Symptoms

- Expected behavior: Preview 页的渲染、层/步进滑块、视角交互、显示开关应作为主流程可用。
- Actual behavior: 用户进入 Preview 后反馈页面不可用；此前已观察到拖层/旋转后画面消失。
- Error messages: 启动日志中出现 `StatsPanel.qml` 调用 `setStealthMode`、`setShowBed`、`setShowTravelMoves`、`setShowMarker` 失败，错误为 `Property ... is not a function`。
- Timeline: v3.3 QRhi/D3D11 Preview 接入后暴露。
- Reproduction: 进入 Preview，点击右侧统计面板里的 Normal/Stealth、显示空驶、显示热床、显示位置标记开关。

## Current Focus

- hypothesis: PreviewViewModel 的开关 setter 只作为 Q_PROPERTY WRITE 存在，但没有 `Q_INVOKABLE`/slot 元数据；QML 使用函数调用形式，因此控件点击直接 TypeError。
- test: 添加 QML/UI 审计回归，要求 StatsPanel 使用到的 PreviewViewModel setter 都以 QML 可调用形式暴露。
- expecting: 当前 `PreviewViewModel.h` 中这些 setter 未带 Q_INVOKABLE，测试应先失败。
- next_action: 写 RED 测试并验证失败。
- reasoning_checkpoint: 日志中已有具体 TypeError，代码中 StatsPanel.qml 明确调用 `root.previewVm.setShowTravelMoves(checked)` 等方法。
- tdd_checkpoint: RED verified by canonical verify failing at QML UI audit; GREEN verified by canonical verify passing.

## Evidence

- timestamp: 2026-06-28
  observation: `build/startup_diagnostics.log` 记录 `StatsPanel.qml:31 TypeError: Property 'setStealthMode' ... is not a function`。
- timestamp: 2026-06-28
  observation: 同一日志记录 `setShowBed`、`setShowTravelMoves`、`setShowMarker` 不是函数。
- timestamp: 2026-06-28
  observation: `PreviewViewModel.h` 声明了这些 setter，但没有 `Q_INVOKABLE`。
- timestamp: 2026-06-28
  observation: RED verification failed at QML UI audit because the setters were not QML-invokable.
- timestamp: 2026-06-28
  observation: GREEN verification passed after marking the four existing setters `Q_INVOKABLE`.

## Eliminated

- hypothesis: 只是用户误操作或单个渲染相机问题。
  reason: 日志显示多个 Preview 控件存在稳定的 QML 方法绑定错误。

## Resolution

- root_cause: StatsPanel 用函数调用方式触发 PreviewViewModel 的显示开关 setter，但这些 setter 只作为 Q_PROPERTY WRITE 存在，没有注册为 QML 可调用方法。
- fix: 将 `setStealthMode`、`setShowTravelMoves`、`setShowBed`、`setShowMarker` 标记为 `Q_INVOKABLE`，保持原业务逻辑不变。
- verification: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` 通过；QML UI audit、启动冒烟、E2E pipeline 通过。
- files_changed: tests/QmlUiAuditTests.cpp, src/core/viewmodels/PreviewViewModel.h
