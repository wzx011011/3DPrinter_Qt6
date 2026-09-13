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
