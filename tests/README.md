# Tests

当前测试集：

- `ViewModelSmokeTests`
  - 覆盖 `EditorViewModel` 导入链路
  - 覆盖 `PreviewViewModel` 切片进度链路
  - 覆盖 `MonitorViewModel` 刷新链路
  - 覆盖 `ConfigViewModel` 预设切换链路
- `VisualRegressionTests`
  - 启动 QML 主界面并截图 4 个核心页面（Prepare/Preview/Monitor/Config）
  - 首次运行自动在 `tests/baseline/` 生成基线图
  - 后续运行按像素差异阈值比对

## 运行方式

在项目根目录执行：

```powershell
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH='E:/Qt6.10' '-DQT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT=3.21'
cmake --build build -j 8
ctest --test-dir build --output-on-failure
```

## 视觉回归基线

- 基线目录：`tests/baseline/`
- 输出目录：`tests/output/`
- 如果想重建基线：删除 `tests/baseline/*.png` 后重新执行 `ctest`

## 截图自校验（整套 UI）

支持用“整个软件各页面截图”作为参考图进行自校验：

1. 把参考图放到 `tests/reference/`，命名为：

- `prepare.png`
- `preview.png`
- `monitor.png`
- `config.png`

2. 运行：

```powershell
ctest --test-dir build --output-on-failure -R VisualRegressionTests
```

3. 查看结果：

- 运行截图：`tests/output/*.png`
- 差异报告：`tests/output/visual_report.txt`

可选：

- 仅抓图不比对（用于更新样本）：设置环境变量 `CREALITY_UI_CAPTURE_ONLY=1`
- 指定参考图目录：设置环境变量 `CREALITY_UI_REFERENCE_DIR=你的路径`

## 覆盖率（已接入）

`CMake` 提供：

- `-DENABLE_COVERAGE=ON`（GCC/Clang）
- `coverage_report` 目标（若系统安装 `gcovr`）

示例：

```powershell
cmake -S . -B build -G Ninja -DENABLE_COVERAGE=ON
cmake --build build --target coverage_report
```
