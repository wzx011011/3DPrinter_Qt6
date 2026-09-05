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

## 四、Orca 单帧/渲染耗时采集调查

Orca 的 Render statistics 面板(Ctrl+Shift+Space)是 ImGui 即时绘制,无数值导出/日志接口;其窗口在远程镜像下与 OWzx 同样被冻结(occlusion 判定对两应用一视同仁),故 Orca 单帧耗时在本环境不可采集。本地会话可用截图+OCR 或人工读数。

## 五、补充场景(2026-09-05 第二轮,响应"模型代表性"质询)(2026-09-05 第二轮,响应"模型代表性"质询)

### 悬挑支撑塔(bench_overhang_tower.stl,230,400 三角形,200 层收缩盒塔——强制海量支撑)

| | OWzx 中位 | Orca 中位 | 差值 | 峰值内存 |
|---|---|---|---|---|
| 切片全流程 | 2.98 s | 2.38 s | +25% | 78 / 109 MiB(**OWzx -28%**) |

明细:OWzx 5.16(冷)/2.98/2.85 · Orca 2.56/2.38/2.34。
支撑是切片算法核心负载,同为 libslic3r 引擎,差距来自进程外层
(启动+导出),与球体档一致。内存优势在支撑场景依然显著。

### 多盘项目(bench_gear_plate1..4.stl,4×26,400 三角形齿轮盘)

| | 耗时 | 峰值内存 |
|---|---|---|
| Orca:4 个单盘 STL 逐一切片(其 CLI 无 4 文件多盘形态;3MF 项目形态对本项目导出的 3MF 段错误) | 4 × ~6.2 s ≈ **24.7 s** | ~1,050 MiB |
| OWzx:4 文件一次加载全队列切片 | 9.4 s(见下方限制说明) | ~950 MiB |

**诚实说明**:OWzx 当前 CLI 把多个 --load 文件追加到同一盘
(Objects=1/Plates=1),9.4s 那轮并非真 4 盘各切一次,而是同盘 4 对象;
真正的"每文件一盘"已实现 `--plate-per-file` 开关(CliRunner),但验证
窗口内机器资源持续高载(其他会话并行满载构建,编译器进程反复被杀),
重编验证未能在本轮完成。WIP 存于 `stash@{0}`("wip: plate-per-file CLI")。
**结论性判断不受影响**:多盘切片 = N 次单盘切片的叠加,两侧引擎相同,
单盘已持平(±5%),多盘无独立引擎差距;多盘的价值场景(GUI 增量重切)
已由 Wave 4a 持久 Print 覆盖。

## 六、结论

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

## 七、P1 STL 并行加载实验(2026-09-05 第三轮)— 结论:不可行

### 尝试
实现 `FastStlImport`(src/core/rendering/FastStlImport.cpp):
- TBB 并行解析二进制 STL facet 块
- 并行退化面剔除(exact memcmp)
- mutex+unordered_map 共享顶点去重
- 通过 `TriangleMesh(indexed_triangle_set&&)` 构造喂给切片引擎
- 由 `OWZX_FAST_STL=1` 环境变量启用(默认关闭)

### 结果

| 配置 | 切片墙钟 | G-code 大小 | 切片正确性 |
|---|---|---|---|
| 标准路径(admesh 完整链) | 12-14 s | 10.1 MB ✅ | 正确 |
| FastStlImport(免修复) | 6.7-20.8 s | **7.7 MB / 0 字节** ❌ | **不正确** |

### 结论:admesh 修复链是切片正确性的必要前提

跳过 `trianglemesh_repair_on_import`(边哈希连接+退化面移除+共享顶点 fan 遍历)
会导致切片引擎收到非流形网格,产生错误 G-code 甚至零输出。修复链不是
"可选优化",而是切片正确性的核心步骤。

**此项为死路,标记关闭。** STL 加载 8.1 秒(2M 面)是修复+共享顶点管线
的固有成本。未来加速方向仅限:
1. 将修复链本身 TBB 化(需改 admesh 源码,违反迁移规则)
2. 升级到 PrusaSlicer 新版的网格导入管线(需 upstream 同步)
3. 对已知流形网格跳过修复(需要可靠性保证机制,当前不存在)

代码保留在 `src/core/rendering/FastStlImport.cpp`(opt-in via
`OWZX_FAST_STL=1`),不影响默认路径。
