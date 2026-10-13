# 效率自测基线（PerfBench stage benchmark）

Established 2026-09-13 on branch `review/v5.16-gap-closure`, commit context of
the HOTKEY/CANVAS-FOCUS + PERF-STAGE-BENCH round. Machine: local Windows 11
dev box, Release build, Ninja/MSVC.

## 运行方式

```powershell
powershell -ExecutionPolicy Bypass -File scripts/perf/run_stage_bench.ps1
```

The harness generates the benchmark STLs on first run
(`scripts/perf/gen_benchmark_models.py`, binary STL UV-spheres; default tiers
400k + 2m triangles, optional 5m and a 20x100k grid-shell scene via
`PERF_BENCH_TIERS` / `GRID_SHELLS`), builds `PerfBench` through the canonical
verify script target list, resolves the Qt bin dir from CMakeCache, runs
`build/PerfBench.exe` with `OWZX_PERF_LOG=1`, and prints
`build/perf_out/stage_report.json` as a table.

## 基线数值（stage_report.json，2026-09-13；**已被 §「Qt 6.12 底座锚点重定」取代为现行判定锚**，本节保留为历史叙述）

### 400k 三角形球体

| Stage | 指标 | 基线 |
|---|---|---|
| load (STL import + ProjectService) | load_ms | 1306 |
| meshData 序列化 | meshdata_ms / bytes | 22 / 14.3 MB |
| scene expand（PrepareSceneData 顶点展开） | scene_expand_ms / verts | 28 / 1,194,480 |
| picking BVH 构建（一次性，懒建） | pick_build_ms / nodes | 406 / 133,597 |
| BVH pick 32-ray sweep | p50 / p95 / hits / parity | **0.0077** / 0.0111 / 32/32 / ok |
| slice（libslic3r 全切片） | slice_ms | 1692 |
| preview parse（GUI 线程 gcode 重解析） | preview_parse_ms | **850**（2026-09-13 修复后；修复前 72147） |
| 峰值内存 | after expand / after preview | 100 / 383 MiB |

### 2m 三角形球体

| Stage | 指标 | 基线 |
|---|---|---|
| load | load_ms | 6924 |
| meshData 序列化 | meshdata_ms / bytes | 121 / 71.9 MB |
| scene expand | scene_expand_ms / verts | 174 / 5,989,704 |
| picking BVH 构建（一次性，懒建） | pick_build_ms / nodes | 2316 / 692,919 |
| BVH pick 32-ray sweep | p50 / p95 / hits / parity | **0.0098** / 0.0139 / 32/32 / ok |
| 峰值内存 | after expand | 495 MiB（BVH 另占 ~50 MiB，首次 pick 后） |

注：`pick_sweep_ms` 自 BVH 轮起包含 32 条 brute-force 对照射线
（ObjectPicking::pick 逐条 parity 校验），不再等于纯加速后的扫描耗时，
故回归判定只看 `pick_ray_p50/p95_ms` 与 `pick_hits`/`pick_parity`。

## 已知热点与结论

- **preview parse 已修复**（2026-09-13，72s → 0.85s，~85x）。根因：
  `parseAxis`/`parseSValue`/`parseFValue` 每次调用都重新构造并编译
  QRegularExpression，每个 G1 move 固定 5 次调用，37.6 万 move 的基准
  gcode 产生 ~190 万次模式编译（~38µs/次 ≈ 72s）。修复为手写扫描 +
  常量模式缓存 + role 标签/映射哈希一次性构建，语义保持不变
  （PreviewParserTests 门禁覆盖）。
- **大模型拾取已加速**（2026-09-13，2m pick p50 33.55ms → 0.0098ms，
  ~3400x）。`PrepareSceneData::pickingRaycaster()` 懒建 binned-SAH BVH
  （PickingRaycaster，上游逐 volume `GUI::MeshRaycaster` AABB 树的对位实现，
  MeshUtils.hpp:159），RhiViewport/SoftwareViewport 全部 4 个生产 pick
  调用点已切换；`ObjectPicking::pick` 保留为 brute-force 参照，测试与
  PerfBench 逐射线断言最近命中完全一致。BVH 构建是每次 mesh 修订后
  首次 pick 的一次性成本（与上游懒建 raycaster 成本结构一致），
  摊销：2m 模型构建 2.3s ≈ 70 次 33ms 的 brute-force pick，悬停
  每次鼠标移动都会 pick，交互开始后即纯收益。
- 场景展开与 meshData 序列化随三角形数线性扩展，量级正常。

## 噪声特性（2026-09-13 三轮复测结论）

同一二进制连续三轮复测（基线轮 + 门禁后两轮）：

- **稳定锚点**（轮间波动 <5%，作为回归判定依据）：`meshdata_ms`、
  `scene_expand_ms`、`pick_ray_p50/p95_ms`、`pick_build_ms`、
  `pick_hits`、`pick_parity`、`scene_vertex_count`、
  `meshdata_bytes`。三轮 pick p50 = 6.58 / 6.53 / 6.65（brute-force 轮）；
  BVH 轮起 pick p50 进入 ~0.01ms 量级，构建耗时为确定性 CPU 工作，
  同样按稳定锚点对待。
- **易变墙钟指标**（随系统负载波动 1.5–2.5 倍，不作单次回归判定）：
  `load_ms`（1306→2568→3508）、`slice_ms`（1692→1872→3567）。
  受磁盘缓存/杀软扫描/后台进程影响明显，复测时应等待系统安静，
  必要时多次取最小值。`preview_parse_ms` 修复前在 72s 量级时同样
  易变（72/79/80s）；修复后（0.85s）为纯 CPU 解析，按稳定锚点对待。
- 内存指标轮间一致。

## 回归规则（常备）

AGENTS.md「Self-Test Standing Rules」：任何影响加载、网格序列化、场景展开、
拾取、切片或 preview 解析的改动，合并前必须重跑本基准；**稳定锚点指标**
相对本文件基线回退超过 ~10% 视为回归，需定位或显式记录原因；易变墙钟
指标只有在与稳定锚点同向恶化且复测复现时才判回归。

## 切片回归门禁（自动门禁，2026-10-10 起）

切片效率与正确性已升级为**自动门禁**（`scripts/perf/run_slice_gate.ps1`，
内嵌于 canonical verify 末尾，触碰切片链路面时自动触发，规则详见
`.codex/rules/build-rules.md`）：

- **正确性**：`owzx-cli` 钉死配置（layer 0.2 / auto 线宽）切
  `bench_sphere_400k.stl` + `Block20XY.stl`，gcode 经
  `tools/gcode_compare.py --summarize` 归一化（去时间戳/缩略图/路径），
  与 `tests/data/slice_gate_baseline.json` 黄金基线逐字段硬比对。
- **效率**：本文件的 `slice_ms` / `preview_parse_ms` 行对应的机器可读
  真值迁移至 `tests/data/slice_gate_baseline.json` 的 `timing` 节
  （min-of-N + 阈值 25% / 10% + 超阈自动复测一轮）；本文件表格保留为
  历史叙述与全阶段基线，两处不一致时以门禁基线 JSON 为准。
- **步骤画像**（P4，2026-10-13 起，门禁第三层）：4 个代表模型各跑一次
  PerfBench 画像轮，逐步骤墙钟真值存于基线 JSON `step_profiles` 节
  （`steps_ms` / `steps_counts`，键=§6d 步骤键），单步骤退化 >+30% 且
  基线 ≥20ms 才记失败，超阈自动复测一轮、两轮都超才 exit 2；步骤键
  （`model_prep…gcode_export` + `unknown_*` 兜底）是引擎 `set_status`
  文本的归一化映射——引擎换版本改文案会让键集变化即门禁红，属设计内
  的重建金触发器。逐步骤毫秒来源：`SliceService` worker 内
  `SliceStepTiming{key,text,percent,firstAtMs,count}` 时间线（首现时间戳
  + 同键计数去重），PerfBench 以相邻首现差算各步耗时，尾部 `done`
  哨兵只作 gcode_export 时长锚、不入表。一致性口径：各步之和 ≈
  `slice_ms`（差值=worker 调度与完成投递开销，本机实测 1–33ms）。
- **G4 enforce 重跑记录（2026-10-14，Qt 6.12 底座，HEAD 002dd3d）**：
  三层全绿 exit 0，**未重建任何金样**。正确性层两模型逐字段 PASS
  （normalized_sha256 / line_count / layer_changes / total_e / command
  counts 全等，gcode size delta 0%）。效率层首跑（默认 repeats=3）
  preview_parse_ms min 1127 对基线 1012 +11.4% 双轮超阈 exit 2——解析
  路径自建金（6cd0cba）以来零代码改动且 slice_ms 钉在基线（2069 vs
  2050），判 min-of-3 边界噪声；以门禁自带 `OWZX_SLICE_GATE_REPEATS=5`
  复跑，slice 2221 (+8.3%)、preview 1081 (+6.8%) 均入噪声内，噪声假设
  证实。步骤画像层第 1 轮 1 项超阈、第 2 轮全入阈（两轮规则）PASS，
  13 键集不变。启示：repeats=3 的 min 在 1s 级纯 CPU 指标上贴近 10% 阈
  时会抖出假阳性，复核先用加大 repeats 再谈重建。

### 逐步骤基线数值（Qt 6.12 重建，2026-10-13T14:32Z 建金 commit 6cd0cba；下表即门禁基线 JSON `step_profiles` 节的展开）

2026-10-13 随 Qt 6.10→6.12 底座切换按 §6e G4 处方 `-UpdateBaseline` 重建
（提交 23c6ada，效率层 slice_ms_min 1670→2050 (+22.8%)、preview_parse_ms_min
791→1012 (+27.9%)）。下表括号内为 6.10 时代建金值（2026-10-13 上午，
5d12d97）作前后对照：全步骤 +20-30% 系统性抬升，13 个步骤键集不变（引擎
`set_status` 文本未动，`z_contouring` 四画像仍未触发）= Qt 侧系统性漂移
而非单点热点，亦非切片输出变化（归因证据链见 开发计划 §7 G4 条目）。

| 步骤键 | sphere_400k | overhang_tower | grid_20x100k | block_20xy |
|---|---:|---:|---:|---:|
| model_prep | 0 (0) | 0 (0) | 0 (0) | 0 (0) |
| config_prep | 2 (1) | 1 (1) | 1 (1) | 1 (1) |
| param_apply | 79 (57) | 39 (34) | 456 (352) | 2 (2) |
| validate | 20 (18) | 2 (2) | 7 (6) | 2 (2) |
| mesh_slicing | 181 (149) | 69 (67) | 1030 (753) | 2 (1) |
| perimeters | 314 (274) | 14 (12) | 1529 (1274) | 3 (3) |
| infill_regions | 155 (130) | 9 (9) | 1487 (1068) | 1 (2) |
| infill_toolpath | 275 (228) | 42 (35) | 1289 (971) | 2 (1) |
| z_contouring | — | — | — | — |
| support | 20 (18) | **686** (**739**) | 284 (205) | 0 (0) |
| overhang_lift | 6 (5) | 5 (5) | 50 (32) | 1 (1) |
| skirt_brim | 15 (11) | 26 (31) | 305 (229) | 0 (0) |
| path_optimization | 13 (12) | 120 (112) | 28 (27) | 2 (2) |
| gcode_export | 1012 (773) | 4325 (3864) | 4088 (2888) | 80 (73) |
| **合计（≈slice_ms）** | **2092 / 2101**（1676/1684） | **5338 / 5344**（4911/4917） | **10554 / 10589**（7806/7839） | **96 / 98**（88/89） |

（`z_contouring` 在四个基准画像中未触发，引擎静默步骤连同
`posEstimateCurledExtrusions`/wipe-tower 时长一并并入相邻步骤间隙。）
（单位 ms；`合计/slice_ms` 为逐步骤之和与同轮 slice_ms 对照。）

画像形态结论（两轮建金同形态）：sphere 以 export/墙线/填充为主（与规格预期
一致）；tower 的 support 是第一非 export 步骤（6.12 建金 686ms / 6.10 建金
739ms，≈球体的 34-40 倍）
——本引擎上 `gcode_export` 对所有模型都是第一大步（47–80%），因为支撑
计算产出的支撑挤出物其 G-code 生成成本约为支撑计算本身的 4 倍，字面
"support 占大头"在引擎现状下不可达，已按可达成口径建金并在此记录。
tower 生成器为 perf/waves 移植版（`overhang_tower(levels=200, side=100,
shrink=0.5, layer_h=0.5, sub=16)`，230,400 三角/100mm 高/50 次外扩桥），
tier 在 PerfBench 内开 `enable_support`（引擎默认关，PrintConfig.cpp）。

## Qt 6.12 底座锚点重定（G4，2026-10-14 记录；现行回归判定锚）

Qt 6.10.2 → 6.12.0 底座切换（M2.5）后的 stage bench 锚点重定。**对照源为
两次官方安静时段整轮跑**（同机同 tiers）：6.10 侧 = 2026-10-11 21:41
（`build/m0_bench_v5.log`，Qt bin E:\Qt6.10，M0 轮）；6.12 侧 = 2026-10-13
12:26（`build/perf_out/stage_report.json`，切换后首轮）。本节表格取代
§「基线数值（2026-09-13）」成为现行判定锚；切片/preview 两项仍以
`tests/data/slice_gate_baseline.json` 为准（见门禁节）。

### 400k 三角形球体（6.10 → 6.12）

| Stage | 指标 | 6.10（10-11） | 6.12 新锚（10-13） | Δ |
|---|---|---:|---:|---|
| load（易变墙钟） | load_ms | 1261 | 1411 | +12%（易变，不判定） |
| meshData 序列化（稳定锚） | meshdata_ms / bytes | 10 / 14,333,796 | 10 / 14,333,796 | 0% |
| scene expand（稳定锚） | scene_expand_ms / verts | 16 / 1,194,480 | 18 / 1,194,480 | +2ms（量级噪声） |
| picking BVH 构建（稳定锚） | pick_build_ms / nodes | 389 / 133,539 | 409 / 133,539 | +5.1% |
| BVH pick 32-ray sweep（稳定锚） | p50 / p95 / hits / parity | 0.0050 / 0.0071 / 32/32 / ok | 0.0053 / 0.0071 / 32/32 / ok | +6% / 0% |
| slice（门禁 JSON 管） | slice_ms | 1539 | 1764 | 易变；基线 2050（+25% 阈）内 |
| preview parse（门禁 JSON 管） | preview_parse_ms | 835 | 816 | −2.3% |
| 峰值内存 | after expand / preview / slice | 100.6 / 465.4 / 346.8 | 100.6 / 449.5 / 329.8 | 持平/改善 |

### 2m 三角形球体（6.10 → 6.12）

| Stage | 指标 | 6.10（10-11） | 6.12 新锚（10-13） | Δ |
|---|---|---:|---:|---|
| load（易变墙钟） | load_ms | 7893 | 8106 | +2.7%（易变，不判定） |
| meshData 序列化（稳定锚） | meshdata_ms / bytes | **55** / 71,876,484 | **72** / 71,876,484 | **+30.9%（超噪，见下）** |
| scene expand（稳定锚） | scene_expand_ms / verts | 88 / 5,989,704 | 88 / 5,989,704 | 0% |
| picking BVH 构建（稳定锚） | pick_build_ms / nodes | 2189 / 692,363 | 2242 / 692,363 | +2.4% |
| BVH pick 32-ray sweep（稳定锚） | p50 / p95 / hits / parity | 0.0076 / 0.0095 / 32/32 / ok | 0.0084 / 0.0106 / 32/32 / ok | +10.5% / +11.6%（µs 级绝对量） |
| 峰值内存 | after expand | 498.6 | 485.6 | −2.6% |

### 重定结论

- **唯一超噪稳定锚 = 2m 档 meshdata_ms 55→72（+30.9%）**。该阶段是
  QByteArray TLV 直写打包热循环（`ProjectServiceMock::meshData()`，
  src/core/services/ProjectServiceMock.cpp:12596；PerfBench.cpp:113 计时），
  与 G4 预期的"导入/解析路径 Qt 容器/字符串实现漂移"吻合；bytes 完全
  一致（71,876,484）= 输出不变、纯耗时漂移。400k 档同代码 10→10 持平
  （14MB 数据上 1ms 粒度不可分辨），pick/expand/build 全体持平在噪声内
  ——漂移限于该打包路径自身，非全局性。按 G4 处方**重建为 72 并在此
  记录前后数值**；挂账观察项：若后续 point release 收敛可回看。
- 2m pick p50/p95 +10.5%/+11.6% 恰在噪声线上的 µs 级绝对量
  （+0.0008/+0.0011ms），按锚点重建值记录、不单独立项。
- 内存峰值全部持平或改善（400k after-preview 465.4→449.5 MiB、
  after-slice 346.8→329.8 MiB；2m after-expand 498.6→485.6 MiB）。
- **测量条件声明**：2026-10-14 的三轮复核直跑（busy box，ToDesk/ZCode
  常驻 ~26% 负载，`build/perf_out/stage_report_g4_qt612_r1..3.json`）
  全指标系统性高于 10-13 官方跑 10–30%（含 6.10→6.12 间曾持平的
  2m expand/pick_build）——证实忙机直跑不可作锚点对照源，锚点 Δ 只从
  两次安静官方跑计算；busy 轮仅用于同版数量级旁证。

## 启动耗时与内存峰值参考点（G4，2026-10-14 建立）

- **6.10 侧基线缺失声明**：§6e G4 原文要求"升级前先留一份 6.10 基线"，
  实际未执行（G4 于切换后的集中切换段内运行）；`.deps/` 下 6.10 目录已
  移除（残留 E:\Qt6.10 待 G5 清）、唯一构建目录已重配 6.12、追加式
  `build/startup_diagnostics.log` 最早条目为 2026-10-14 00:21——6.10 侧
  启动耗时/内存峰值**无任何可回溯数据**，前后对照不可补测。本节因此
  建立的是 **6.12 首个参考点**，供下一次底座升级做前后对照。
- **方法**（可复现）：`build/OWzxSlicer.exe --open-page home
  --skip-first-run` 真实桌面启动；外部计时 = 进程创建 →
  `startup_diagnostics.log` 出现 "Startup open-page handled" 行（该行经
  `QQuickWindow::frameSwapped` 一次性门 + QueuedConnection 落盘，
  main_qml.cpp FIXTURE-02 = 首帧已上屏代理点，只匹配本轮新增字节防旧
  行误配）；峰值内存 = 到点后 2.5s 读 `PeakWorkingSet64` /
  `PeakVirtualMemorySize64` 后杀进程。1 预热 + 5 测量。
- **6.12.0 参考值**（2026-10-14 凌晨，busy box ~26% 负载，6 轮全成）：
  启动到首帧 min/avg/max = **5004 / 5036 / 5065 ms**（极差 1.2%，极稳；
  构成 ≈0.4s 进程+DLL+RHI 前段 + ≈4.5s `engine->load(main.qml)` QML 树
  编译创建 + ≈65ms 首帧投递，in-app 里程碑见 startup_diagnostics.log）；
  峰值工作集 min/avg/max = **558.9 / 559.9 / 561.0 MiB**；峰值虚拟内存
  ≈5742 MiB。探针脚本为一次性工具未入库（build/g4_startup_probe.ps1，
  本次运行产物 build/perf_out/g4_startup_612.json）。
