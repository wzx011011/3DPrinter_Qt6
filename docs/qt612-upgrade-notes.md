# Qt 6.10.2 → 6.12.0 LTS 底座升级纪要（M2.5，2026-10-14 定稿）

> 范围：Windows 主线（win64_msvc2022_64）Qt **6.10.2 → 6.12.0 LTS** 底座切换，
> 对应开发计划 §6e M2.5（G0 调查 / G1 底座就位 / G2 编译清障 / G3 行为回归 /
> G4 门禁与基线重定 / G5 清理与纪要）。执行记录与提交哈希的权威出处是
> `docs/开发计划_2026-10-10.md` §7 M2.5 各条与 §8 修订记录；本文是面向后续
> 升级/回滚的工程纪要。WSL 侧 Qt 6.10.2 linux_gcc_64（~/owzx）随 Linux 线
> 另行升级，不在本纪要范围（计划 §6e 已登记待办）。

## 1. 改动清单（按阶段）

### G0 决策门（2026-10-12，纯调查无提交）

- 6.12.0 开源可用性确认：`download.qt.io/official_releases/qt/6.12/6.12.0/`
  自 2026-09-30 落位 → 分支 B（挂起）不触发，走分支 A；采纳 **6.12.0**
  （aqt 无 6.12.1，G1 实装复查后仍仅 .0 → 按 §6e 口径执行 .0 深度回归）。

### G1 底座就位（提交 2555640）

- Qt 6.12.0 装入 `.deps/Qt6.12/6.12.0/msvc2022_64`（2.6G，旧 6.10 目录不动，
  回滚=改回锚点）。
- **锚点三处原子切换**（禁止双版本混链）：
  1. `scripts/auto_verify_with_vcvars.ps1` — `$qtRootCandidates` 候选列表
     （`auto_verify_with_vcvars.ps1:237-247`，单版本纪律注释随 G5 收敛）；
  2. `CMakeLists.txt:109-116` — `third_party/Qt6_12` 回退分支 +
     `QT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT=3.21`；
  3. `.github/workflows/tag-build.yml:57-66` — install-qt-action
     `version: "6.12.0"` + `aqtsource` 钉 git master。
- 工具链事实（读安装目录实测，记录于 `.codex/rules/build-rules.md` Expected
  Environment）：① PyPI aqtinstall 3.3.0 装不了 6.11+（Qt 在线仓库目录布局
  变更，修复在 git master PR #1000，本地 3.3.1.dev166 实测可装；CI 钉
  `aqtsource` 待 PyPI 出修复版后摘除）；② Qt 6.12 supported min CMake
  **3.25**，本机 3.21 靠 force-var 覆盖（configure 有 Qt 官方非致命
  WARNING，符合预期；CMake≥3.25 升级时复估）；③ **无 C++20 要求**，全树
  `CMAKE_CXX_STANDARD 17` 编译通过。

### G2 编译清障（提交 e42a61c）

- 全目标编译通过（清 `build/` 的 CMakeCache+CMakeFiles 防 33 条 6.10 缓存
  混链后重配；495 ninja 步、0 error）。
- 弃用告警：src/ 内唯一命中 = ProjectServiceMock.cpp 四处
  `QtConcurrent::run` 丢弃 QFuture（6.12 起 `[[nodiscard]]`，MSVC 记
  C4858）——**保留显式 `(void)`**，不采 Qt 建议换
  `QThreadPool::start(Callable&&)`（四处 worker lambda 均非整体 try/catch，
  换语义会把"弃异常吞进被丢弃的 future"变成 std::terminate）；其余
  C4996×43/C5051/C4133/C4709 全部出自 third_party（OrcaSlicer
  mcut/miniz/CGAL），不动。
- 私有 API 三处复核：
  1. `CxComboBox.qml` 的 `import QtQuick.Controls.impl` = 死 import，**摘除**；
  2. RhiViewportRenderer/RhiBackendSelector 的 GuiPrivate **不可摘**——
     6.12.0 的 `rhi/qrhi.h`、`qrhi_platform.h` 仍只在
     `include/QtGui/6.12.0/QtGui/rhi/` 版本化私有树下；
  3. `main_qml.cpp` WebEngineQuick 修饰名动态解析**保留**（直链会把"DLL
     缺失优雅降级"变硬启动依赖；dumpbin 实测
     `?initialize@QtWebEngineQuick@@YAXXZ` 在 6.10.2 与 6.12.0 完全一致，
     ordinal 252），补 resolve 失败 qWarning 使未来改名响亮失败。
- **GL 链清理（Qt6::OpenGL 摘除 + BackendContext GL sysinfo 换
  QSGRendererInterface/DXGI）未随 G2 执行**（Q6 复核改挂项，仍待办，
  见计划 §8 2026-10-13 行）。

### G3 行为回归（提交 9c970df + 002dd3d，执行记录见计划 §7 G3 条）

- VBLANK workaround 退役试验 = **退役落地**（见 §3 workaround 清单）。
- D3D12 探测路径复测 = **崩溃复现**（`OWZX_RHI_RENDERER=d3d12` 显式
  opt-in：probe 成功、QQuickWindow swapchain 路径 0xC0000005；本轮在
  **独显 RX 7800 XT** 上也复现，崩因不限于核显）→ D3D11-first 保留，
  d3d12 降为纯 triage opt-in（RhiBackendSelector.cpp 两文件注释已改写）。
- RHI 视口逐页目验（build/g3_*.png 在案）：home / prepare+test_model.stl /
  prepare+multi_material_fixture.3mf（导航立方体、move gizmo、床网格、
  贴盘图标列、双对象多材质）/ preview / monitor / 偏好设置，逐项过；
  assemble 视图不可达与软件回退断裂为**升级前既有债务**（计划 §7 G3 条
  债务 ②③，非 6.12 回归）。
- WebEngine（Chromium 大版本跳变）：AI 聊天面板 Loader 报
  `qtwebenginequickplugin.dll` 加载 err 127（Qt6WebEngineCore 自身在系统
  import snap 层 STATUS_ENTRYPOINT_NOT_FOUND，build/ 部署与安装逐字节
  一致、48 个 Qt6*.dll 交叉校验 0 缺失）→ Loader 进 Error 态、应用继续跑
  =**优雅降级成立**；WebChannel 桥未到达（面板未挂载），重测窗口
  =6.12 point release / 换机。
- verify 红转绿修复（002dd3d）：6.12 起 QML 对"函数对象赋给非 var 属性"
  与"undefined 赋给 bool 属性"由静默容忍改报错——
  `MultiMachineViewModel.h` 补 `pagedCloudTaskCount`/`onlineMachineCount`
  两个 Q_PROPERTY（连带修复 MultiMachinePage 云任务 delegate 首次真正
  实例化暴露的 width 越界引用与 Row 锚定）、`LeftSidebar.qml` 三处
  `!!root.configVm && …` 强制布尔。遗留 allowlist 条目与处方见计划
  §7 ⑤ 记录。

### G4 门禁与基线重定（提交 23c6ada + fd0cd42）

- 切片门禁效率层+步骤画像层基线重建（23c6ada，`-UpdateBaseline`）：
  `slice_ms_min 1670→2050 (+22.8%)`、`preview_parse_ms_min 791→1012
  (+27.9%)`；4 模型（sphere_400k / overhang_tower / grid_20x100k /
  block_20xy）步骤画像重建、**13 步骤键集不变**（引擎 `set_status` 文本
  未动）= Qt 侧系统性漂移而非单点热点，亦非切片输出变化；正确性金样
  重祝福（归一化等价成立）。归因证据链见计划 §7 G4 条。
- G4 enforce 重跑（2026-10-14，HEAD 002dd3d）：**三层全绿 exit 0、零
  重建金样**；效率层 repeats=3 时 preview_parse min 1127 (+11.4%) 双轮
  超阈属 min-of-3 边界噪声，`OWZX_SLICE_GATE_REPEATS=5` 复跑 slice
  2221 (+8.3%) / preview 1081 (+6.8%) 入噪——启示：1s 级纯 CPU 指标贴
  阈时先加大 repeats 再谈重建（详见 perf-baseline.md 门禁节）。
- stage bench 锚点重定（fd0cd42，对照两次官方安静时段整轮跑：
  6.10=2026-10-11 21:41 `m0_bench_v5.log` vs 6.12=2026-10-13 12:26）：
  **唯一超噪稳定锚 = 2m meshdata_ms 55→72 (+30.9%)**（QByteArray TLV
  直写打包热循环，bytes 完全一致 71,876,484 = 纯耗时漂移），400k 全部
  持平/改善，内存持平/改善；前后对照全表见
  `docs/perf-baseline.md` §「Qt 6.12 底座锚点重定」。
- 启动耗时/内存参考点（fd0cd42）：6.10 侧数据不可回溯（切换前未留基线，
  G4 在切换段内运行——已在该节声明）；6.12.0 首个参考点 =
  `--open-page home` 真实桌面启动 5004/5036/5065 ms（min/avg/max，极差
  1.2%）、峰值工作集 558.9–561.0 MiB、峰值虚拟内存 ≈5742 MiB
  （busy box 口径与方法学见 perf-baseline.md 末节）。

### G5 清理与纪要（本批）

- 仓库内 **6.10 现行表述清扫**（见 §5）+ 本纪要 + 计划 §7/§8 记账。

## 2. 门禁/基线前后对照（摘要）

| 层 | 指标 | 6.10（切换前） | 6.12（重定后） | 处置 |
|---|---|---|---|---|
| 切片门禁·效率 | slice_ms_min | 1670 | 2050 (+22.8%) | 23c6ada 建金 |
| 切片门禁·效率 | preview_parse_ms_min | 791 | 1012 (+27.9%) | 23c6ada 建金 |
| 切片门禁·画像 | step_profiles | 6.10 建金（5d12d97） | 全步骤 +20-30% 系统性抬升，13 键集不变 | 23c6ada 建金，前后值留存 perf-baseline 表 |
| 切片门禁·正确性 | 黄金基线 | 6.10 建金 | 重祝福（归一化等价），enforce 逐字段 PASS、gcode size delta 0% | 23c6ada + fd0cd42 enforce |
| stage bench | 2m meshdata_ms | 55 | **72 (+30.9%)** | fd0cd42 重建为 72（唯一超噪锚） |
| stage bench | 400k 全锚 / 2m expand·pick_build·内存 | — | 持平或改善（µs 级 pick 例外，见 perf-baseline） | 无处置 |
| 启动/内存 | 参考点 | 无可回溯数据（缺失已声明） | 5004/5036/5065 ms；WS 558.9-561.0 MiB | fd0cd42 建立 6.12 首参考点 |

权威数值与测量条件（官方安静跑 vs busy box 口径）以
`docs/perf-baseline.md` 与 `tests/data/slice_gate_baseline.json` 为准。

## 3. workaround 清单（摘除/保留及理由）

| workaround | 处置 | 依据 |
|---|---|---|
| `QT_D3D_NO_VBLANK_THREAD=1` qputenv 钉死（main_qml.cpp） | **摘除**（9c970df） | qtbase `qdxgivsyncservice.cpp` run() 循环 v6.10.0/v6.11.0/v6.12.0 **字节一致**（Qt 未修）；死锁消失系驱动环境（32.0.12033.1030 → 32.0.31036.15，WaitForVBlank 恢复真阻塞）；摘除后 3+ 启动深链页面上屏 + cdb 实证 QDxgiVSyncThread 真等垂直同步。回滚预案以注释钉在 main_qml.cpp:390-422 原位；环境变量 6.12 仍被尊重，`QT_D3D_NO_VBLANK_THREAD=1 OWzxSlicer.exe` 是用户侧逃生门。**每个 6.12 point release 复测一次（常态保留）** |
| D3D12 降级（D3D11-first 默认） | **保留，不可简化** | 6.12 下 0xC0000005 复现且独显也崩（见 §1 G3）；d3d12 仅 triage opt-in |
| WebEngineQuick 修饰名动态解析 | **保留** + resolve 失败 qWarning | 修饰名跨 6.10.2/6.12.0 一致（dumpbin）；直链会把优雅降级变硬依赖（e42a61c） |
| `QtConcurrent::run` 丢弃 QFuture 的显式 `(void)` | **保留** | 6.12 起 `[[nodiscard]]`；换 `QThreadPool::start` 会改变异常语义（e42a61c） |
| `QT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT=3.21`（CMakeLists.txt:115） | **保留** | 6.12 supported min CMake=3.25，本机 3.21；CMake≥3.25 升级时复估 |
| CI `aqtsource` 钉 aqtinstall git master | **保留** | PyPI 3.3.0 装不了 6.11+（PR #1000）；出修复版后摘除 |
| `import QtQuick.Controls.impl`（CxComboBox.qml） | **摘除** | 死 import，全文件仅用公开类型（e42a61c） |

钉版本 workaround 的"每个 point release 复测一次"常态登记在计划 §5 挂账。

## 4. qmllint 收集结论（6.12 工具链，G2 口径=只收集不门禁，S8 才门禁化）

- 覆盖 **84 个 QML** 文件（计划口径 87，Q6 孤儿清理后漂移）：**0 error**；
  31 文件干净、53 文件有 findings。
- findings 分布：unqualified **1890** / Quick.layout-positioning 79 /
  missing-property 56 / unused-imports 16 / incompatible-type 8 / import 7 /
  index 5 / unresolved-type 2 / id-shadows-member 2 /
  block-scope-var-declaration 2 / duplicated-name 1。
- `pragma ComponentBehavior` 在 6.12 仍为 **opt-in**（qmllint 仅 Info 提示），
  全量补齐随 S8（`pragma ComponentBehavior: Bound` 现为 5/87）。
- 6.12 QML 引擎严格化已在 G3 实证一处行为面（函数对象/undefined 赋值报
  错，002dd3d 修复）——S8 迁移与 qmllint 门禁化时按此口径预期更多存量
  潜伏缺陷显形。

## 5. G5 清理清单与残留登记

### 本批已清（仓库内现行表述）

- `scripts/auto_verify_with_vcvars.ps1` — 候选列表注释中的 6.10 回滚路径
  改为"历史记录，勿复加，见本文"（候选列表本身 G1 起即仅 6.12）。
- `.codex/rules/build-rules.md` — "Rollback = restore the 6.10 anchors" 改为
  指向本文的回滚史；stale-DLL 教训泛化为"上一底座的陈旧 DLL"（保留
  6.10→6.12 事件为实例）。
- `docs/i18n_工作流.md` — lupdate/linguist/lrelease 工具路径
  `E:\Qt6.10\bin\` → `.deps\Qt6.12\6.12.0\msvc2022_64\bin\`（三工具在
  6.12 kit 内实测存在；**注意**：该文档属 `docs/*` 忽略策略内的未跟踪
  文档，此修复落在工作树不入库，以本纪要为持久记录）。
- `docs/perf-baseline.md` — "残留 E:\Qt6.10 待 G5 清"状态注更新。
- 锚点三处与 `CMakeLists.txt` 复核：**G1 已切换，无 Qt6_10/6.10 残留**
  （git grep 复核 2026-10-14）。
- 保留为**带日期历史记录**不改写（改写即伪造历史）：docs/ 各扫描/审计/
  对照报告的计划时点表述、`docs/开发计划_2026-10-10.md` §6e 规划文本与
  §7 各阶段执行记录、`docs/ai-control.md` Vulkan 事件记录、
  `docs/CrealityPrint_Qt_GUI重写架构.md` R10 风险行（设计期风险登记，
  未对 6.12 重评前不改值）。

### 残留登记（不在 G5 提交白名单内，留后续开卡/人工）

- **辅助脚本硬编码 `E:\Qt6.10`**（G1 时不属锚点白名单，计划 §7 G5 条
  原拟一并清）：`scripts/build_tests_only.ps1:13-14`、
  `scripts/capture_qml_warnings.ps1:39`、`scripts/regression_project_io.ps1:9`、
  `scripts/regression_slice.ps1:7`、`scripts/run_unit_tests_vcvars.ps1:40-42`、
  `scripts/verify_qml_116.ps1:49-50`——均非 canonical 构建路径，误用会
  指向已退役底座；处置=改指 `.deps\Qt6.12\6.12.0\msvc2022_64` 或随
  一次性脚本清理批删除。
- `i18n/workflow.md:18,32` 工具路径、`i18n/*.ts` 六语言 ×2 条 "Qt 6.10"
  用户可见串（镜像 HomePage.qml 页脚，随 qVersion() 化后 lupdate 刷新）。
- `src/qml_gui/pages/HomePage.qml:1040` 版本脚注硬编码 "Qt 6.10"
  （G3 新发现债务 ④：改运行时 `qVersion()` 或删除；src/ 不在 G5 白名单）。
- **磁盘清理（人工，门禁全绿后执行）**：`E:\Qt6.10` 旧安装目录删除
  （仓库外）；`.deps\Qt6.10` 经实测已不存在（`.deps/` 现仅含 Qt6.12）。

## 6. 回滚预案（底座回退 6.12 → 6.10）

1. 锚点三处改回 6.10 候选（`auto_verify_with_vcvars.ps1` 候选列表 /
   `CMakeLists.txt:114` third_party 分支 / `tag-build.yml` version）——
   历史 6.10 候选路径 = `.deps\Qt6.10\6.10.0\msvc2022_64`、`.deps\Qt6.10`、
   `E:\Qt6.10`、`D:\Qt6.10`（现均已删/待人工删，需先重装 kit）。
2. kit 重装用 aqtinstall git master（PyPI 版装不了 6.10 也可用同一路径，
   见 §1 G1 工具链事实）。
3. 清 `build/` 的 CMakeCache.txt + CMakeFiles 后重配（防混代缓存）；
   首次 canonical verify 会以新 `$qtRoot` 重部署 windeployqt 自愈陈旧
   DLL（0xc0000139 教训，build-rules.md Expected Environment）。
4. 切片门禁/stage bench 基线随底座重定（同一套 `-UpdateBaseline` +
   perf-baseline 记录流程，参照本轮 G4）。
5. 单版本纪律：任何时刻锚点只允许一个底座候选集，缺失即构建响亮失败，
   禁止回退候选静默共存。

## 7. 验收对照（§6e G5）

- 锚点三处全部指向 6.12 ✅（G1 落地，本批 grep 复核）+ WSL 待办已在
  计划 §6e 登记。
- 13 套件 0 failed + 三层门禁绿 / stage bench：由编排脚本统一复测
  （本纪要引数值均出自已落库的 G4 官方跑记录）。
- RHI 目验全过 ✅（G3，build/g3_*.png 在案）。
- 基线重定有据可查 ✅（perf-baseline.md 前后对照 + slice_gate_baseline.json）。
- 无 6.10 双版本混链残留：构建链路（verify 候选/CMake/CI/build/ 部署）
  ✅；上节"残留登记"为非构建链路的已知尾巴，已逐条带路径开卡。
