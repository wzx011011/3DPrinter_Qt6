---
paths:
  - "src/**/*"
  - "scripts/**/*"
---

# Debugging Rules

## Program Entry

- 程序入口是 `src/qml_gui/main_qml.cpp`（不是 `src/main.cpp`）

## Diagnostic Logs

- 启动诊断日志在 `build/startup_diagnostics.log`，QML 加载失败、rootObjects 为空等信息都记录在里面
- 程序无法启动时，**第一步检查 `startup_diagnostics.log`**，不要先查 DLL 依赖或加调试打印
- `CrashHandlerWin` 安装日志在 `build/crash_dumps/crash_stack.log`

## QML Pitfalls

- Qt6 `SpinBox.stepSize` 只接受 `int`，浮点步进需用整数缩放 + `textFromValue`/`valueFromText`
- `TapHandler`/`HoverHandler` 等非 Item 没有 `z` 属性
