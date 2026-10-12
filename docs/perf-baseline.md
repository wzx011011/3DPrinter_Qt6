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

## 基线数值（stage_report.json，2026-09-13）

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

### 逐步骤基线数值（2026-10-13 建金，PerfBench layer 0.2 钉配置，单轮）

| 步骤键 | sphere_400k | overhang_tower | grid_20x100k | block_20xy |
|---|---:|---:|---:|---:|
| model_prep | 0 | 0 | 0 | 0 |
| config_prep | 1 | 1 | 1 | 1 |
| param_apply | 57 | 34 | 352 | 2 |
| validate | 18 | 2 | 6 | 2 |
| mesh_slicing | 149 | 67 | 753 | 1 |
| perimeters | 274 | 12 | 1274 | 3 |
| infill_regions | 130 | 9 | 1068 | 2 |
| infill_toolpath | 228 | 35 | 971 | 1 |
| z_contouring | — | — | — | — |
| support | 18 | **739** | 205 | 0 |
| overhang_lift | 5 | 5 | 32 | 1 |
| skirt_brim | 11 | 31 | 229 | 0 |
| path_optimization | 12 | 112 | 27 | 2 |
| gcode_export | 773 | 3864 | 2888 | 73 |
| **合计（≈slice_ms）** | **1676 / 1684** | **4911 / 4917** | **7806 / 7839** | **88 / 89** |

（`z_contouring` 在四个基准画像中未触发，引擎静默步骤连同
`posEstimateCurledExtrusions`/wipe-tower 时长一并并入相邻步骤间隙。）
（单位 ms；`合计/slice_ms` 为逐步骤之和与同轮 slice_ms 对照。）

画像形态结论（同轮目验）：sphere 以 export/墙线/填充为主（与规格预期
一致）；tower 的 support 是第一非 export 步骤（739ms，≈球体的 40 倍）
——本引擎上 `gcode_export` 对所有模型都是第一大步（47–80%），因为支撑
计算产出的支撑挤出物其 G-code 生成成本约为支撑计算本身的 4 倍，字面
"support 占大头"在引擎现状下不可达，已按可达成口径建金并在此记录。
tower 生成器为 perf/waves 移植版（`overhang_tower(levels=200, side=100,
shrink=0.5, layer_h=0.5, sub=16)`，230,400 三角/100mm 高/50 次外扩桥），
tier 在 PerfBench 内开 `enable_support`（引擎默认关，PrintConfig.cpp）。
