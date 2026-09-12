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
| ObjectPicking::pick 32-ray sweep | p50 / p95 / sweep / hits | 6.58 / 12.43 / 239 / 32/32 |
| slice（libslic3r 全切片） | slice_ms | 1692 |
| preview parse（GUI 线程 gcode 重解析） | preview_parse_ms | **850**（2026-09-13 修复后；修复前 72147） |
| 峰值内存 | after expand / after preview | 100 / 383 MiB |

### 2m 三角形球体

| Stage | 指标 | 基线 |
|---|---|---|
| load | load_ms | 6924 |
| meshData 序列化 | meshdata_ms / bytes | 121 / 71.9 MB |
| scene expand | scene_expand_ms / verts | 174 / 5,989,704 |
| ObjectPicking::pick 32-ray sweep | p50 / p95 / sweep / hits | 33.55 / 41.43 / 1111 / 32/32 |
| 峰值内存 | after expand | 495 MiB |

## 已知热点与结论

- **preview parse 已修复**（2026-09-13，72s → 0.85s，~85x）。根因：
  `parseAxis`/`parseSValue`/`parseFValue` 每次调用都重新构造并编译
  QRegularExpression，每个 G1 move 固定 5 次调用，37.6 万 move 的基准
  gcode 产生 ~190 万次模式编译（~38µs/次 ≈ 72s）。修复为手写扫描 +
  常量模式缓存 + role 标签/映射哈希一次性构建，语义保持不变
  （PreviewParserTests 门禁覆盖）。
- pick 已走生产路径 `ObjectPicking::pick`（AABB 预过滤 + Moller-Trumbore），
  400k p50 6.6ms 可接受；2m 模型 p50 33.6ms 提示后续可引入空间加速结构。
- 场景展开与 meshData 序列化随三角形数线性扩展，量级正常。

## 噪声特性（2026-09-13 三轮复测结论）

同一二进制连续三轮复测（基线轮 + 门禁后两轮）：

- **稳定锚点**（轮间波动 <5%，作为回归判定依据）：`meshdata_ms`、
  `scene_expand_ms`、`pick_ray_p50/p95_ms`、`scene_vertex_count`、
  `meshdata_bytes`。三轮 pick p50 = 6.58 / 6.53 / 6.65。
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
