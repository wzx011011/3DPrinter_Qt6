# 3DPrinter_Qt6 (OWzx Slicer)

OrcaSlicer 的 Qt6/QML 重写版桌面切片软件。引擎直接编译上游 libslic3r，界面用 Qt Quick 重新实现，行为以上游 OrcaSlicer 为准。

## 构建与运行（Windows）

唯一构建命令（configure + 编译 + 全部测试，输出到 `build/`）：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

运行：

```powershell
./build/OWzxSlicer.exe
```

- 不要使用其他构建目录或自行拼 cmake 命令，详见 `.claude/rules/build-rules.md`。
- `BUILD_LIBSLIC3R` 必须为 `ON`；关闭时 configure 直接失败，避免生成能启动但不能导入、切片的程序。
- 环境：Visual Studio 的 MSVC 与 Ninja（脚本经 vcvars 自动定位）、Qt 6.12.0（`.deps/Qt6.12`）；OrcaSlicer 依赖包与 vcpkg 依赖的路径由构建脚本解析。
- 启动失败时先看 `build/startup_diagnostics.log`。

## 开发方式

本仓库按"上游源码真值迁移"推进：先在 `third_party/OrcaSlicer` 找到对应的上游实现，再在 `src/` 中承接，不自行设计新的产品行为。

- 规则：`CLAUDE.md`、`.claude/rules/`
- 继续下一个迁移任务：`/migrating-source-truth`（追加 `all` 为连续批量模式）
- 只读差距分析：`/analyzing-source-truth-gap <task-or-feature>`

## 文档

| 文档 | 内容 |
|---|---|
| [docs/源码对照迁移任务追踪.md](docs/源码对照迁移任务追踪.md) | 各领域现状、未关闭缺陷、功能缺口、待决策事项 |
| [docs/架构说明.md](docs/架构说明.md) | 分层、目录、服务、线程、测试 |
| [docs/上游基线与功能矩阵.md](docs/上游基线与功能矩阵.md) | 上游 pin、引擎补丁、上游模块与 Qt 模块对照 |
| [docs/services-status.md](docs/services-status.md) | 各服务是真实实现还是 Mock，以及替换条件 |
| [docs/依赖与协议边界审计.md](docs/依赖与协议边界审计.md) | 外部依赖与协议边界、阻塞登记、解除决策 |
| [docs/LAN打印机接入.md](docs/LAN打印机接入.md)、[docs/创想云模型库接入.md](docs/创想云模型库接入.md)、[docs/ai-control.md](docs/ai-control.md) | 项目自有功能的设计与实现 |
| [docs/qt612-upgrade-notes.md](docs/qt612-upgrade-notes.md)、[docs/perf-baseline.md](docs/perf-baseline.md) | Qt 6.12 升级记录；性能基线 |
| [i18n/workflow.md](i18n/workflow.md) | 翻译流程 |

## CI / 发布

`.github/workflows/tag-build.yml`：推送任意 tag（或在 Actions 页面手动触发）时，在 `windows-2022` 上构建 OrcaSlicer 依赖和 Paho MQTT（带缓存），以 `BUILD_LIBSLIC3R=ON` 编译 `OWzxSlicer`，用 `windeployqt` 打包为 `OWzxSlicer-<tag>-windows.zip`，并创建 GitHub Release。

```powershell
git tag v0.1.1
git push origin v0.1.1
```
