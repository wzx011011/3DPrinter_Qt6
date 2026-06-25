# 差距分析: PartPlate and AssembleView (v3.0 候选)

**日期:** 2026-06-25
**方法:** 只读源真值分析(OrcaSlicer 上游 vs Qt6 实现),遵循 `.codex/rules/source-truth-migration.md`
**输出依据:** 两个并行 Explore 子代理(差距分析 + OCCT 可用性专项核查)

## 结论速览

| 维度 | 评级 | 一句话 |
|---|---|---|
| Qt6 plate 数据模型 | **Mock** | `int plateCount_` + 一组并行 `QList` 向量,无 `PartPlate` 类 |
| Plate 生命周期 | **Mock + Missing** | 增删改名锁有(壳),clone/duplicate/reorder/per-plate-printable 缺失 |
| Per-plate 切片调度 | **Hybrid** | 真 clone→slice 循环存在,但非上游架构(无 per-plate Print/Config 合并) |
| Per-plate 配置 override | **Mock** | `setPlateScopedOptionValue` 在 HAS_LIBSLIC3R 下 `return false`(stub) |
| 3MF 多盘 round-trip | **Mock / broken** | 真保存路径只序列化 `model_`,**不写任何 plate 状态** → 存盘即丢失 |
| AssembleView | **Placeholder** | QML 静态 `Text { "装配视图暂不可用" }`,无实现 |
| OCCT 依赖(专项核查) | **可用 + 非阻断** | PartPlate 不需要 OCCT;且 v2.9 文档里"OCCT 被 DelayLoadHook 抑制"是**过时描述** |

**总规模:** 大型(LARGE)。上游 PartPlate 共 ~7700 行,其中需迁移的数据/生命周期/IO 层约 ~2700 行(~35%),其余是 GL 渲染(36%,Qt6 用自己的渲染器)/wxWidgets(17%,无关)/cereal(1%,Qt6 自持久化)。

---

## 1. 数据模型差距

### 上游 `PartPlate`(完整对象,`PartPlate.hpp:77-557`)

每个 plate 拥有(序列化字段):`m_plate_index`、`m_name`、`m_origin`、`m_width/depth/height`、`m_locked`、`m_printable`、`m_ready_for_slice`、`m_slice_result_valid`、`m_config`(`DynamicPrintConfig`,per-plate 配置覆盖)、`obj_to_instance_set`(object+instance 成员集合,`set<pair<int,int>>`)、多种缩略图(top/pick/no_light/obj_preview/cali)、`m_shape/m_exclude_area/m_extruder_areas`、包围盒、`m_gcode_result`(GCodeProcessorResult*)、`filament_maps`、层序打印序列等。`PartPlateList`(`:559-937`)持有 `m_plate_list`、`m_print_list`(per-plate Print 映射)、`MAX_PLATE_COUNT=36` 等。

### Qt6 plate 数据模型(`ProjectServiceMock.h:336-361`)

**没有 `PartPlate` 类。** "plate" 是一组并行 `QList` 向量:`plateCount_`(int)、`currentPlateIndex_`(int)、`plateNames_`(QStringList)、`plateObjectIndices_`(`QList<QList<int>>`,仅 object 索引,**无 instance 对**)、`plateLockedStates_`、`plateBedTypes_/platePrintSequences_/...`、`m_mockPlateOverrides`(`QHash<int,QHash<QString,QVariant>>`)。

### 上游有、Qt6 完全缺失的字段
- **真实几何**:`m_origin/width/depth/height`、`m_shape`、`m_exclude_area`、`m_extruder_areas`、包围盒 — Qt6 plate 无位置概念
- **instance 级成员**(`set<pair<int,int>>`):Qt6 只存 per-object,**无法表达多实例 plate 和"instance 在 plate 外"跟踪**
- per-plate `Print` / `GCodeProcessorResult*`(Qt6 把切片结果放 `SliceService::plateResults_`,不在 plate 上)
- 多种缩略图(Qt6 只有 `generatePlateThumbnail()` 合成纯色 PNG,`ProjectServiceMock.cpp:3820`)
- `filament_maps` / `filament_map_mode`
- 切片状态机:`is_printable()/can_slice()/is_slice_result_ready_for_print()/for_export()`

---

## 2. 生命周期差距

| 操作 | 上游方法(文件:行) | Qt6 状态 | Qt6 方法 |
|---|---|---|---|
| 创建 plate | `PartPlateList::create_plate` `PartPlate.cpp:4407` | Mock(向量 append,无几何/布局) | `addPlate` `ProjectServiceMock.cpp:884` |
| 删除 plate | `delete_plate` `:4554`(重索引、移 wipe_tower 数组、重算列、重定位 instance) | Mock(仅向量 removeAt) | `deletePlate` `:904` |
| 重命名 | `set_plate_name` `PartPlate.hpp:301` | Mock | `renamePlate` `:936` |
| **克隆/复制 plate** | `duplicate_plate` `PartPlate.cpp:4484`(复制+移对象到新 plate 相对位置+重建 instance 集) | **缺失(无 API)** | — |
| 锁定 | `lock_plate` `:4938` | Mock(仅 bool) | `setPlateLocked` `:974` |
| **移动/重排 plate** | `move_plate_to_index` `:4895` | **缺失** | — |
| 对象跨 plate 移动 | `add_to_plate`/`notify_instance_update` `:812/806` | Mock(object 索引 shuffle,无 instance 拆分) | `setObjectPlateForIndex` `:1022` |
| **per-plate printable** | `m_printable`;`is_printable()` `:430` | **缺失**(Qt6 只有 per-object printable) | — |
| **shared/unprintable plate** | `unprintable_plate` + "shared" 语义 | **缺失** | — |
| 选择 plate | `select_plate` `Plater.cpp:10018`(切换当前 plate + 切片上下文) | Mock(仅设 `currentPlateIndex_`) | `setCurrentPlateIndex` `:873` |

---

## 3. 切片调度差距(PLATE-01 关键)

**上游:** plate-by-plate 状态机。`m_cur_slice_plate`、`on_action_slice_all`(`Plater.cpp:10107`)重置后逐 plate 调 `select_plate` + `reslice()`,每 plate 一个独立 `Print`(来自 `PartPlateList::m_print_list`),切片上下文经 `update_slice_context_to_current_plate` 加载(Plater.cpp 中约 15 处调用)。完成时 `m_cur_slice_plate++`(`:10015`)。

**Qt6:** `EditorViewModel::requestSliceAll()`(`EditorViewModel.cpp:3656`)建 `m_sliceAllQueue`,`continueSliceAllQueue()`(`:3672`)逐 plate 调 `SliceService::startSlicePlate(plate)`(`SliceService.cpp:760`)。实际切片调 `projectService_->cloneCurrentPlateModel()`(`SliceService.cpp:261-262`)——**构建只含当前 plate 对象的 `model_` 过滤副本**,然后切片这一个副本为一个 `Slic3r::Print`。**无 per-plate Print 列表、无 per-plate GCodeProcessorResult、无 per-plate wipe-tower 逻辑。**

**per-plate 配置注入:** 仅手工 patch 3 个 key(`curr_bed_type`/`print_sequence`/`spiral_mode`,`SliceService.cpp:379-403`),**非** `PartPlate::config()` 合并 → 任意 per-plate override(`filament_maps`、`print_compatible_*`、层序)**被静默丢弃**。

**状态:Hybrid。** 真的能产出每 plate 的 G-code,但非上游架构,多盘/多实例/wipe-tower 场景 G-code 可能不正确。

---

## 4. Per-Plate 配置 Override 差距

- 上游:`PartPlate::m_config`(`DynamicPrintConfig`,`PartPlate.hpp:159,244`),3MF 持久化 `PartPlate.cpp:6160`
- Qt6 API:`plateScopedOptionValue/setPlateScopedOptionValue`(`ProjectServiceMock.h:120-121`)
- **真模式:stub。** `setPlateScopedOptionValue`(`ProjectServiceMock.cpp:1330-1342`)在 `#ifdef HAS_LIBSLIC3R` 下 `return false`,注释 `// TODO: upstream PartPlate config access`。mock 分支只写 `m_mockPlateOverrides`。切片时从不查询 override map,只读 3 个硬编码 key。
- 3MF 保存时真路径不写 `m_mockPlateOverrides` 及任何 plate 向量。

---

## 5. AssembleView 差距

上游有两个 "Assemble" 概念,Qt6 **都没实现**:

1. **`CanvasAssembleView`**:鸟瞰多盘布局画布(`Gizmos/GLGizmoAssembly.cpp:34,60` 引用 `get_canvas_type()==CanvasAssembleView`)
2. **`GLGizmoAssembly`**(`Gizmos/GLGizmoAssembly.cpp`,193 行):volume 级面/点对齐装配 gizmo(`GLGizmoMeasure` 子类),非多盘布局工具

**Qt6:** `ViewMode::AssembleView = 2`(`Plater.qml:99-112`,`BackendContext.h:141-185`)是**显式占位**——只渲染 `Text { qsTr("装配视图暂不可用") }`。`BackendContext.h:180` 注释:"AssembleView 在 v2.0 为 Out of Scope,仅留占位。" 无对应 GLCanvas,Qt6 有自己的渲染器且无多画布 Assemble 视图。

---

## 6. 3MF Round-Trip 差距(PLATE-02 关键)

### 保存(真模式,`ProjectServiceMock.cpp:4491-4654`)
`#ifdef HAS_LIBSLIC3R` 且 `model_` 非空时(`:4500-4538`):建 `StoreParams{path, model_, SaveStrategy::Zip64}` 调 `store_bbs_3mf`(`:4516`)。**只序列化 `model_`**。无 `PartPlateList::store_to_3mf_structure` 调用,无 `PlateData` 列表构建,**所有 Qt6 plate 向量(名/锁/床类型/序列/spiral/层序/override)都不写**。JSON mock fallback(`:4540-4654`)会序列化,但只在 `model_` 为空 / HAS_LIBSLIC3R 关闭时跑。

### 加载(真模式,`loadProject` `:4656-4803`)
调 `Model::read_from_archive(... &plateDataList ...)`(`:4731`),**确实**读 `plateDataList` 并从 `plate->plate_name` + `plate->objects_and_instances` 重建 `loadedPlateNames` + `loadedPlateObjectIndices`(`:4776-4799`)。所以 **盘名 + 对象→盘成员关系在加载时能恢复**。

### 净 round-trip
- **加载恢复:** plate 数、plate 名、对象→盘成员关系(via 3MF `objects_and_instances`)
- **保存丢失(真模式):** 超出 model 原生承载的盘名、锁状态、床类型、打印序列、spiral、filament maps、层序、per-plate config override。**从不写** → 真路径存→加载会**把所有 per-plate 设置塌缩为默认值,可能塌缩为单盘**。

**状态:Mock / 多盘 broken。PLATE-02 当前不可达**,除非实现 Qt6 侧 `store_to_3mf_structure` 写路径(在 `store_bbs_3mf` 前把 plate 向量填进 `PlateData`)。

---

## 7. 复杂度评估

上游 `PartPlate.cpp` 6781 行 + `.hpp` 946 行 ≈ **7727 行**。

| 类别 | 约行数 | 占比 | 迁移相关性 |
|---|---|---|---|
| **数据模型 + 生命周期**(字段、create/duplicate/delete、store/load_3mf、instance-set、update_slice_context、config、filament-map、print-sequence) | ~1800 | ~23% | **必须迁移**(真实盘语义) |
| GL 渲染(render*、calc_*、PickingModel、GLModel) | ~2800 | ~36% | 无关(Qt6 自有 QML/QtQuick 渲染器) |
| wxWidgets/对话框/纹理/图标/DPI | ~1300 | ~17% | 无关 |
| 排列集成(preprocess_arrange_polygon*、compute_plate_index、rebuild_plates_after_arrangement) | ~600 | ~8% | 多盘排列需要时迁移 |
| Wipe-tower 估算 + 包围检测 + 挤出器区几何 | ~500 | ~6% | per-machine 盘几何需要时迁移 |
| GCode/缩略图文件 IO | ~400 | ~5% | 部分迁移(仅加载) |
| Cereal 序列化 | ~80 | ~1% | 跳过(Qt6 自持久化) |
| 切片状态机(can_slice、is_slice_result_*、update_states) | ~250 | ~3% | 切片调度正确性需迁移 |

**Qt6 需吸收的真实"数据+生命周期+状态机+排列+3MF IO"约 ~2700 行(~35%)**,其余 ~65% 是 GL/wx。

`PlateSettingsDialog.cpp`(718 行)几乎全是 wx 对话布局,Qt6 等价物是 QML 设置面板;只需移植字段/值映射(床类型/打印序列/spiral/层序),而 Qt6 **已有 API 面**(`EditorViewModel.h:488-509`),问题是**背后是 mock**(§4)。

---

## 8. OCCT 可用性专项核查(用户特别要求)

### 结论:OCCT **可用**,且 PartPlate **不依赖 OCCT → 非阻断**

### 关键修正:v2.9 文档过时

v2.9 baseline 称"OCCT 是 delay-loaded 且被 DelayLoadHook 抑制"——**这是过时描述**。当前 Qt6 build(FromSource 配置,默认开启)里:

- **OCCT 是静态导入,非 delay-load**(`cmake/BuildLibslic3rFromSource.cmake:599-679`)。注释(`:612-615`)解释:delay-load 曾导致 `TDataStd_GenericEmpty::Paste` 递归崩溃(因 `__pfnDliNotifyHook2` 未链入 exe),故禁用 delay-load,TK*.dll 正常加载。
- **`DelayLoadHook.cpp` 是死代码**:仅文档引用(`AGENTS.md`、`CLAUDE.md` 等),**不在 `owzx_app_core` 源列表**(`CMakeLists.txt:69-137`),`cmake/*.cmake` 也无引用。抑制机制未生效。
- **build/ 下有 38 个 TK*.dll**,由 `auto_verify_with_vcvars.ps1:95-104` 和 `CMakeLists.txt:251-258` POST_BUILD 复制部署。
- 二进制确认:`OWzxSlicer.exe`(6/25 09:42 构建)静态引用 `TKernel/TKBRep/TKXDESTEP/TKXCAF/TKTopAlgo/TKPrim/TKMesh/TKMath/TKLCAF.dll`(9 个,各 1 次,load-time import)。

### PartPlate 是否需要 OCCT?
**否。** `PartPlate.hpp` + `PartPlate.cpp` 共 7727 行 **零** OCCT 符号(`BRep/TopoDS/gp_Pnt/STEPControl/OCCT/OpenCASCADE/TKBRep/CGAL/cgal` 全 0 匹配)。PartPlate 操作 `BoundingBox`/`Polygon`/`ModelInstance`——纯 libslic3r 类型,无 CAD 内核。它是 `slic3r/GUI/` 层(GUI 类),连 libslic3r core 都不是。

### 其他 v3.0+ 候选特性的依赖矩阵

| 特性 | 引擎 | 需 OCCT? | Qt6 状态 |
|---|---|---|---|
| **PartPlate** | 纯 libslic3r 几何 | 否 | N/A |
| Mesh boolean | CGAL(非 OCCT) | 否 | **构建排除**(`BuildLibslic3rFromSource.cmake:318`),需 CGAL 5.6+(当前 5.4),已 stub |
| Cut surface | CGAL | 否 | **构建排除**(`:316`),同 CGAL 版本问题 |
| STEP 导入 | OCCT | 是 | 构建并链接(已部署) |
| SVG 导入 | OCCT | 是 | 构建并链接 |
| Text/emboss 几何 | OCCT | 是 | 构建并链接 |
| Support paint / hollow | OpenVDB | 否(OCCT) | **BLOCKED**(OpenVDB 链接失败) |
| TPMS infill | cr_tpms 闭源 DLL | 否(OCCT) | delay-load 后部署时删除,事实不可用 |

### 建议
- **PartPlate 里程碑:无 OCCT/CGAL 顾虑,直接推进。**
- v2.9 文档(AGENTS.md:130,147,372 / CLAUDE.md / STATE.md)关于 OCCT 的描述应更正("delay-loaded + 抑制" → "load-time imported, available")。可作 v3.0 启动前 5 分钟清理,或留 v3.0 顺手。
- 后续若做 mesh boolean / cut surface,真正阻断是 **CGAL 升级到 5.6+**(与 OCCT 无关)。

---

## Top 5 迁移风险

1. **3MF 多盘状态保存丢失(PLATE-02 阻断)。** `saveProject` 真路径只写 `model_`,从不填 `PlateData` → 多盘项目存盘即丢名/锁/override/序列。需建 Qt6 侧 `store_to_3mf_structure` 写路径。
2. **plate 数据模型是并行向量,非领域对象。** 无 `PartPlate` 类 → per-plate GCodeProcessorResult/Print/instance 对集/几何/状态机无法表达。重构 `ProjectServiceMock` plate 存储为真 `PartPlateList` 等价物会触及 ~30 个 Q_INVOKABLE 方法 + 加载路径(`:4768-4799`)。
3. **per-plate config override 在 HAS_LIBSLIC3R 下 stub。** `setPlateScopedOptionValue` `return false`,切片只注入 3 个硬编码 key → 任意 override 被丢(PLATE-01 正确性风险)。
4. **slice-all 循环重切过滤副本,非 per-plate Print。** `cloneCurrentPlateModel` 删非本盘对象再切;对锁定/空盘/多实例/wipe-tower-per-plate 与上游 `update_slice_context` 背离 → 多盘/多实例 G-code 可能不正确。
5. **缺失生命周期:clone/duplicate plate、move-plate reorder、per-plate printable、shared-plate 语义。** 上游 `duplicate_plate`(`:4484`)、`move_plate_to_index`(`:4895`)无 Qt6 等价(已验证无 `clonePlate/duplicatePlate/movePlate` 符号)。

---

## 验证需求

- 本报告所有结论为**文本/证据级**(只读分析)。
- **PLATE-02 完成前**需运行时验证 3MF round-trip(存多盘 → 重载 → 断言盘数、名、锁、override 保留)。
- 遵循 `.codex/rules/source-truth-migration.md` "Required Verification":用 `scripts/auto_verify_with_vcvars.ps1` 做全量验证;built-only 测试须显式说明;需硬件/不可用依赖的工作流单独记录 manual verification。

---

*分析于 2026-06-25,作为 v3.0 PartPlate and AssembleView 里程碑启动前的源真值差距评估。*
