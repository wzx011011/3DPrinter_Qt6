# 对象列表面板 · 上游行为真值（X6，2026-10-15）

对象：`third_party/OrcaSlicer/src/slic3r/GUI/GUI_ObjectList.cpp`（6977 行）+ 其数据模型
`ObjectDataViewModel.{hpp,cpp}`。本文是 OWzx Qt6 对象列表面板（`panels/ObjectListPanel.qml`
+ `src/core/model/SceneTreeModel`）的行为真值清单与偏差登记；行号以本仓库锁定的
OrcaSlicer 快照为准，全部条目均在实施时逐行核对。

## 1. 位置与结构（Plater 同位）

| 上游 | 证据 | OWzx 落点 |
|---|---|---|
| ObjectList 挂在 Sidebar 内（不是独立右栏）：`p->sizer_params->Add(p->m_object_list, 1, wxEXPAND)`，位于搜索条之下、对象设置/工艺参数之上，权重 1（与 params 的 3 分摊剩余高度） | Plater.cpp:3234, 3243-3245 | `panels/ObjectListPanel.qml` 挂进 `LeftSidebar.qml` 现有 ColumnLayout 尾段（切片信息卡与工艺参数面板之间），随 DockableSidebar 左右停靠，与上游"sidebar 内一节"同位 |
| 数据树：根 = 板（itPlate），板 → 对象（itObject），对象 → 体（itVolume）/ 实例根（itInstanceRoot，名 "Instances"）/ 实例（itInstance，名 "Instance N"）/ 设置（itSettings）/ 层根+层（itLayerRoot/itLayer）/ 信息（itInfo） | ObjectDataViewModel.hpp:25-36；ObjectDataViewModel.cpp:112-130（"Instances"/"Instance %d" 命名） | SceneTreeModel 同构：板 → 对象 → 体 + 实例根 → 实例。设置/层/信息三种子树 Qt6 侧由既有 dialog/画布路径承载（见 §7 偏差） |
| 列：colName / colHeight（变层高）/ colPrint（可打印）/ colFilament（耗材）/ colSupportPaint / colColorPaint / colSinking / colEditing | ObjectDataViewModel.hpp:38-50；GUI_ObjectList.cpp:424-472 | 面板列 = Name（图标+文本，可内联重命名）+ Printable（锁形指示，点击切换）+ 耗材号（只读数字）。其余列依赖的 gizmo/画布设施由工具栏/右键菜单承载（见 §7） |

## 2. 拖拽重排（GUI_ObjectList.cpp:1845-2054）

- 只允许单选拖拽：`mult_sel → Veto`（:1847-1851）。
- 只允许 itObject 与 itVolume 两类行发起拖拽，其余类型 Veto（:1855）。
- **对象**：只能在同一板内重排——`from_plate != to_plate` 即拒绝（can_drop :1920-1939）。
- 落点生效 = 逐对 `std::swap(m_objects[id], m_objects[id+delta])` 从源走到目标（OnDrop :2028-2029）+ `ReorganizeObjects`（模型侧同步重排），快照名 **"Object order changed"**（:2005），随后整板 reload + `changed_object`。
- **体**：仅限同一对象内部（:1941-1943）；不可跨类型重排，SUPPORT_BLOCKER 及以上类型不可移动（:1952-1953）；对象只有一个 MODEL_PART 时该 part 不可移动、NEGATIVE/PARAMETER 不可插入首位（:1956-1975）。
- Qt6 对应：对象拖拽 = `ProjectService::moveObject(from,to)`（ProjectService.cpp:9248 起，逐对 swap 的等价实现 + 平行数组/板成员同步）+ `MoveObjectCommand` 入 undo 栈（UndoCommands.h:212-226，undo 走 `moveObject(to,from)` 原路换回）。体拖拽暂缺 `moveVolume` 服务面（见 §7 偏差登记），面板侧对体行不发起拖拽。

## 3. 列表内重命名（:1176-1213, 6080-6098, 6399-6409, 6564-6597）

- 可重命名类型 = itVolume | itObject | itPlate（渲染器编辑门 :418-421）。
- 空名拒绝；`Plater::has_illegal_filename_characters` 命中即警告并取消（:6089-6095，OnEditingDone :6585 弹警告）。
- 快照名：对象 = **"Rename Object"**、体 = **"Rename Part"**（update_name_in_model :1196）。
- 对象改名若只有一个体，该体同步改名（:1203-1205）；板名走 `plate->set_plate_name`（:1178-1189）。
- Qt6 对应：`EditorViewModel::renameObject(sourceIndex, newName)` 入 `RenameCommand`（EditorViewModel.cpp:10065-10083 已有 undo；X6 起接 source-index 语义 + 空名/非法字符拒绝对齐上游，非法字符集与 ProjectService 载入侧一致）。

## 4. 显隐/锁定（同一引擎旗标）

- 本快照引擎里对象级只有一个旗标 `ModelInstance::printable`；对象列表的"眼睛"就是可打印指示（ProjectService.cpp:2569-2581 的 R-P0.5 注记；上游 toggle_printable_state GUI_ObjectList.cpp:6812-6867 直接写 `inst->printable`）。colPrint 渲染为 check_on/check_off 位图（ObjectDataViewModel.cpp:203-209）。
- 点击 colPrint（对象行/实例行均可）：以**第一个选中行**的取反值为目标，应用到全部选中行；itObject 行应用到其全部实例（:6826-6852）；快照名空串（BBS 去名，:6831）；随后 canvas `update_instance_printable_state_for_objects` + 整景 update（:6862-6865）。
- 快捷键 `p` 同 toggle（key_event :1824-1825）；`d` = auto_drop（Qt6 未移植 auto_drop，不登记为面板缺失——引擎面无此旗标的 UI）。
- Qt6 对应：行点击 → `EditorViewModel::setSelectedObjectsPrintable` 既有宏路径（EditorViewModel.cpp:7907-7947，`PrintableCommand` 每对象一条，P16.8）+ X6 新增单行直达 `toggleObjectPrintableAt(sourceIndex)`（同样走 PrintableCommand）。显隐与锁定在面板上是**同一列同一动作**，与引擎单旗标一致——不做假的第二开关。

## 5. 实例（instances）

- 对象行下挂 "Instances" 容器与 "Instance N" 行；实例行可单独切 printable（:6846-6849），实例级 toggle 在 Qt6 已有 `instancePrintable/toggleInstancePrintable`（EditorViewModel.h:980-981，对齐 ObjectList::toggle_printable_state itInstance 分支）。
- +/-/上下文菜单增删实例（key_event :1820-1823；Qt6 既有 addSelectedInstance/removeSelectedInstance + instance 菜单）。
- Qt6 对应：SceneTreeModel 实例行展示 per-instance printable；面板实例行复用 instance 菜单族。

## 6. 选中同步与右键菜单

- 列表选中 ↔ 画布选中双向同步（selection_changed :1229-1266：`update_selections_on_canvas`；反向 update_selections :4814）。
- 右键菜单**与画布共用同一族**：多选同对象 → instance_menu，多选其它 → multi_selection_menu；单选按行类型 part_menu/text_part_menu/svg_part_menu/plate_menu/instance_menu/object_menu；无选中 → default_menu（show_context_menu :1621-1674）。
- 列点击分流（list_manipulation :1543-1614）：colHeight→层编辑；colPrint→切 printable；colSupportPaint/colColorPaint→开对应 gizmo；colSinking→ensure_on_bed；colEditing→打开对象/层设置页；colName→警告图标点击 = fix_through_cgal，其余 = 菜单。
- 双击对象/体/实例行 = `zoom_to_selection`（:495-503）。
- 键盘（key_event :1797-1842）：Del 删除、Ctrl+A 全选、Ctrl+C/V/X 复制/粘贴/剪切、Ctrl+Z/Y 撤销重做、Ctrl+K 克隆、+/- 增减实例、数字键切耗材。
- Qt6 对应：面板行右键 → `synchronizeViewportContext(Object,…)` → PreparePage 既有 `prepareContextMenus.openResolved(family, x, y)`（PreparePage.qml:1848-1856），与画布右键同一菜单实例（= 上游"共用 plater 菜单"的同构实现）。行单击选中走 `selectSourceObject`；Del/复制等键由 Plater 级既有快捷键承载，不在面板内重建第二套。

## 7. 偏差登记（有意不做 / 另路径承载）

| 上游能力 | OWzx 处置 | 理由 |
|---|---|---|
| 层编辑挂列表（itLayerRoot/itLayer 行 + colHeight + add/edit/del_layer_range :4514-4760） | 不入树。层编辑由既有 `dialogs/ObjectLayersDialog.qml`（P16.3，ObjectList::edit_layer_range 的 QML 组合）+ 工具栏"变层高"按钮承载 | Qt6 层编辑交互已按 dialog 形态落地且带 undo；树内层行需要 set_status 级引擎回调面（layer_config_ranges 枚举/监听）做增量同步，成本/收益不匹配，登记为后续挂账而非本条目欠账 |
| 设置行（itSettings）/ 信息行（itInfo：Custom supports/MmSegmentation/FuzzySkin/CutConnectors） | 不入树 | 上游这两类行是设置入口的快捷方式；Qt6 走画布 gizmo 与对象设置菜单（ContextMenuFamily 已全量对齐 P16） |
| 体（volume）拖拽重排 | 面板发起面已按上游规则预留（单选+同对象+类型约束），但 `ProjectService::moveVolume` 服务面不存在；体行当前只读 | 服务面不在本条目 git 白名单（src/core/services/ProjectService.* 不可改）；登记为独立小条目挂账，避免绕白名单硬编 |
| 体（volume）列表内重命名 | 面板与 part 菜单均不提供 | `ProjectService` 无 renameVolume 服务面（同上白名单约束）；partMenu 的上游 Rename 首项随本缺口一并延后（上游 create_part_menu 第一项即 Rename，GUI_Factories.cpp:1556），服务面落位后两项一起补 |
| 耗材列内联编辑（BitmapChoiceRenderer + 数字键） | 面板只读显示 extruder 号；修改走既有 "Change Filament" 菜单（P16.4 setExtruderForSelectedItems） | 同一写路径、非缺失行为 |
| 搜索条（Search plate, object and part，Plater.cpp:3201-3232） | 不在本条目 | 独立功能面（搜索弹窗），不在 X6 规格（§6c）列明范围 |
| 板行重命名（itPlate 可编辑） | 板行显示名 + 当前板标记，重命名走画布 PLATEICONS-GL 既有 rename pencil（PreparePage plateRenameDialog） | 板名写路径已存在且带 plateChanged 刷新；树内第二入口会双写路径 |
| 双击行 = zoom_to_selection（:495-503 → GLCanvas3D::zoom_to_selection :1869） | 面板双击仅选中行，不缩放 | VM 无选区级 fit 通道（唯一 fit 面 = 全场景 `fitHint` QVector4D，NOTIFY sceneTopologyChanged；`requestFitView(cx,cy,cz,r)` 在视口侧）。选区 bbox center+radius 通道落位后补（挂账），不在本条目白名单内造第二套几何推导 |

## 8. Undo 链对照

| 动作 | 上游快照 | OWzx 命令 |
|---|---|---|
| 重排对象 | "Object order changed"（:2005） | `MoveObjectCommand`（逐对 swap 可逆） |
| 重命名对象/体 | "Rename Object"/"Rename Part"（:1196） | `RenameCommand` |
| 切 printable/显隐 | 空名快照（:6831） | `PrintableCommand`（宏 "Set Printable"，P16.8 既有） |
| 删除/克隆/剪切粘贴 | Plater::TakeSnapshot | `DeleteObjectsCommand`/`CloneCommand`（既有） |

全部命令最终由共享 `UndoRedoManager`（BackendContext 单例注入 EditorViewModel）承载，
与上游 Plater::TakeSnapshot → UndoRedo 单栈语义一致。
