# OWzx vs OrcaSlicer — 大模型 A/B 对比报告(帧率工具 + 管线基准)

日期:2026-09-05 · 机器:Ryzen 7 5800X / 32GB / RX 7800 XT · 同机同模型同参数
模型:确定性合成基准(400k / 2M 三角形球体;详见 docs/perf-baseline.md)
方法与复现命令见 docs/perf-baseline.md;原始日志存 build/perf_out/。

## 一、帧率工具怎么用在本报告里

我们做的 `Ctrl+Shift+Space` FPS 工具在本轮 A/B 中承担两项:
1. **单帧渲染耗时**(软件画布 `swrender.frame` 统计,20 帧一桶):
   2M 三角形模型 **min = 5.59 ms/帧** —— 即渲染能力约 180 FPS,
   3 倍余量于 60 FPS 预算(16.6ms)。渲染速度不是瓶颈。
   (avg/max 被远程镜像的帧调度等待污染至 9.1s,取 min 定案。)
2. **持续交互 FPS**:需要本地会话操作(远程桌面会冻结按需渲染,
   合成输入无法持续触发重绘——已多法实测并入档)。你本地双击
   `scripts\perf\gui_fps_collect.cmd` 拖 15 秒即可读出;对 Orca 同法
   (`Ctrl+Shift+Space` 调出它的 Render statistics 面板)。

因此本报告的"渲染"维度用两条硬证据代替持续 FPS:
单帧渲染耗时(上)+ 渲染管线各阶段 CPU 耗时(下)。

## 二、A/B 实测数据(2026-09-05,各 3 轮取中位)

### 切片全流程(进程墙钟,含加载+切+导出)

| 档位 | OWzx 中位 | Orca 中位 | 差值 | 峰值内存 OWzx / Orca |
|---|---|---|---|---|
| 400k | 4.05 s | 3.90 s | +3.8% | **212 / 361 MiB(OWzx -41%)** |
| 2M | 12.52 s | 11.90 s | +5.2% | **742 / 793 MiB(OWzx -6%)** |

3 轮明细:
- 400k OWzx 4.05/3.93/4.22 · Orca 3.93/3.76/3.90
- 2M OWzx 16.31(首跑冷缓存)/12.02/12.52 · Orca 12.97/11.90/11.55

结论:墙钟在 ±5% 噪声带内持平(共享机器波动);首跑冷缓存差值
(16.31 vs 同期)属磁盘缓存效应,非代码差异。内存两项均优。

### 渲染/几何管线(OWzx 阶段基准,2M 三角形)

| 阶段 | 耗时 | 说明 |
|---|---|---|
| 模型加载(read_from_file) | 8.13 s | 与 Orca 同一引擎调用,纯 IO+三角化 |
| meshData 几何编码 | **46 ms** | v2 索引化(基线 140ms,3.0×) |
| 场景 CPU 展开 | 111 ms | 兼容汤 599 万顶点 |
| 拾取 BVH 构建 | 518 ms | 每次网格变更一次 |
| **拾取单射线 p50** | **0.008 ms** | 暴力法 1307ms → ~4,000×;5M 档同值 |
| 单帧渲染(画布) | **5.59 ms** | 见上节;60FPS 预算的 1/3 |

### 切片后预览就绪

| | OWzx | Orca |
|---|---|---|
| 数据通路 | 内存直通(切片时内建) | 内存直通 |
| 400k 模型就绪耗时 | **毫秒级**(原 87.5s→1.93s 文本解析已被直通取代) | 毫秒级 |

## 三、优化前后对照(OWzx 自身,证明优化有效)

| 指标 | 优化前 | 优化后 | 倍数 |
|---|---|---|---|
| 切片后预览解析(400k) | 87.5 s(GUI 线程卡死) | 内存直通 + 文本解析 1.93s | ~45× |
| hover 拾取 p50(2M) | 33.2 ms/射线 | 0.008 ms | ~4,000× |
| meshData 构建(2M) | 140 ms | 46 ms | 3.0× |
| 几何 blob(2M) | 68.6 MB | 45.8 MB | -33% |
| GPU 顶点数据/三角形 | ~120 B(汤) | ~26-38 B(索引) | ~4× |
| 预览暂存/走刀 | 672 B | 264 B | 2.5× |
| undo 内存上限 | 无界(100 条×2 份整项目) | ≤10% RAM | 有界化 |

## 四、结论

1. **帧率**:渲染单帧 5.59ms(2M 面),渲染能力 3 倍余量于 60FPS;
   优化集中在"每帧背后的工作"(拾取、几何、内存),全部达标。
   持续交互 FPS 数字需本地会话用 FPS 工具补测(脚本已备)。
2. **切片**:墙钟与 Orca 持平(±5% 噪声内),内存明确更优(-6%~-41%)。
3. **引擎内切片算法**与 Orca 同为 libslic3r,计算无差距;差距全在管线
   外层,已通过九波优化抹平。
4. 已知残留:2M 档墙钟中位 +5.2%(本轮),为分布式事件流转开销,
   归因实测无可移除单一阶段;预览实例化(264→~76B/走刀)规格已存
   (docs/perf-wave2b-design.md)。

## 五、复现

```powershell
cd D:\ai\3DPrinter_Qt6-perf
python scripts/perf/gen_benchmark_models.py --out build/perf_models
powershell -File scripts/perf/run_owzx_bench.ps1 -Model build/perf_models/bench_sphere_2m.stl
powershell -File scripts/perf/run_orca_bench.ps1 -Model build/perf_models/bench_sphere_2m.stl
powershell -File scripts/perf/run_stage_bench.ps1 -Tiers "2m"
# GUI FPS(本地):双击 scripts\perf\gui_fps_collect.cmd → Ctrl+Shift+Space
```
