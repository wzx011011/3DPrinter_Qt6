# SLICE-DETAIL — Preview TickCode/IMSlider 系统详细规格

**Milestone:** v2.1
**需求域:** SLICE（Preview TickCode/IMSlider）
**创建日期:** 2026-06-17
**状态:** 待实现

---

## 上游真值（详细对照）

### 上游文件清单

| 文件 | 行数 | 作用 |
|---|---|---|
| `third_party/OrcaSlicer/src/slic3r/GUI/GCodeViewer.cpp` | 4723 | G-code 解析 + 着色渲染 + 视角控制 |
| `GCodeViewer.hpp` | 371 | EViewType 枚举 + SequentialView + 公开接口 |
| `IMSlider.cpp` | 1828 | 层滑块（刻度/拖拽/TickCode 渲染/键盘） |
| `IMSlider.hpp` | 246 | IMSlider 类 + TickCode 管理 |
| `TickCode.cpp` | 204 | TickCode 业务逻辑（增删改查/颜色计算） |
| `TickCode.hpp` | 62 | TickCode 结构 + TickCodeInfo 类 |

### EViewType（着色模式枚举）

上游 `GCodeViewer.hpp` 定义 6 种着色模式（v2.1 全部实现）：

```
enum class EViewType : unsigned char {
    Feature,         // 按 Feature 类型（OuterWall/InnerWall/Infill/Skirt/Support 等，~15 种颜色）
    Extruder,        // 按挤出机编号（每个 extruder 一种颜色）
    Speed,           // 按打印速度（mm/s，渐变色蓝→绿→黄→红）
    LayerHeight,     // 按层高（mm，渐变色）
    Pressure,        // 按 MSNP 压力数据（渐变色）
    Pixel            // 按像素（特殊渲染模式）
};
```

切换时 `GCodeViewer::set_view_type(type)` → 重新着色所有 G-code 段 → GPU buffer 更新。

### TickCode 结构（上游 TickCode.hpp）

```cpp
struct TickCode {
    int         tick = 0;        // 在 slider 上的刻度位置（层序号）
    Type        type = ColorChange;  // CustomGCode::Type
    int         extruder = 0;    // 挤出机编号（ColorChange 用）
    std::string color;           // 颜色（ColorChange 用，如 "#FF0000"）
    std::string extra;           // 自定义 G-code 文本（Custom 用）
};
```

`CustomGCode::Type` 枚举（来自 `libslic3r/CustomGCode.hpp`）：
- `ColorChange` — 换色
- `PausePrint` — 暂停打印
- `ToolChange` — 换挤出机
- `Custom` — 自定义 G-code
- `Template` — 模板（不实际写入）

### IMSlider 关键交互（上游 IMSlider.cpp）

- **层切换**：拖拽 slider 滑块 / 点击刻度 / 键盘 ←→
- **TickCode 渲染**：在 slider 刻度位置画彩色小方块（ColorChange=挤出机色，Pause=黄，Custom=灰）
- **插入 TickCode**：右键 slider → 菜单（Add ColorChange/Pause/Custom G-code）
- **编辑 TickCode**：左键点击已有 TickCode → 弹编辑对话框
- **删除 TickCode**：右键已有 TickCode → Delete
- **键盘**：←/→ 层切换；Home/End 首/末层；PgUp/PgDn 大步进

---

## 当前实现状态（v2.0 之后）

`src/qml_gui/pages/PreviewPage.qml` (302 行) 当前实现：
- ✅ GLViewport(CanvasPreview) + LayerSlider + MoveSlider
- ✅ StatsPanel + Legend（静态）+ ToolPositionTooltip
- ✅ 键盘导航（Left/Right/Home/End/PageUp/Down）

**缺失（本域要做的）**：
- ❌ 着色模式切换 UI + 渲染（SLICE-01）
- ❌ IMSlider 刻度标记 + 拖拽手感（SLICE-02）
- ❌ TickCode 系统（SLICE-03/04）
- ❌ Legend 动态（SLICE-06）
- ❌ 冲突热区（SLICE-05）

---

## SLICE-01: G-code 着色模式切换 [P0]

### 上游对照
- 上游：GCodeViewer.hpp EViewType + `set_view_type()` + 重新着色
- 我们：GCodeRenderer 有 `renderMode` 属性，但无 UI 切换入口

### 实现方案

**1. GCodeRenderer 扩展**（`src/core/rendering/GCodeRenderer.h/.cpp`）
- 新增 `Q_ENUM GCodeViewMode { Feature=0, Extruder=1, Speed=2, LayerHeight=3, Pressure=4, Pixel=5 }`
- 新增 `Q_PROPERTY(int viewMode READ viewMode NOTIFY viewModeChanged)`
- 新增 `Q_INVOKABLE void setViewMode(int mode)` → 内部重新着色 G-code 段
- 着色逻辑：根据 mode 重新计算每段 G-code 的颜色（Feature→固定 15 色，Speed→渐变映射，等）

**2. ViewModel 层**（`PreviewViewModel`）
- 加 `Q_PROPERTY(int gcodeViewMode ...)` 透传 GCodeRenderer
- 加 `Q_INVOKABLE setGcodeViewMode(int)`

**3. QML UI**（PreviewPage.qml）
- 在 slider 上方加 **ColorModeSelector** 横排按钮组：
  ```
  [Feature] [Extruder] [Speed] [LayerHeight] [Pressure] [Pixel]
  ```
- 点击 → `previewVm.setGcodeViewMode(idx)`
- 当前选中高亮

### UI 规格
- 位置：Preview 页 3D 视口底部、LayerSlider 上方
- 样式：6 个紧凑文字按钮（CxFloatingButton），选中态绿色高亮
- 图标：可选（每种模式一个小图标），先用文字

### 数据流
```
用户点 [Speed] 按钮
  → QML: previewVm.setGcodeViewMode(2)
  → PreviewViewModel: emit gcodeViewModeChanged(2)
  → GCodeRenderer: setViewMode(2) → 重新着色所有段 → GPU buffer 更新
  → GLViewport 触发重绘 → 用户看到速度渐变色
```

### 验收
- [ ] 切换 6 种模式，每种渲染颜色不同（截图对比）
- [ ] Feature 模式：~15 种 Feature 类型颜色区分
- [ ] Speed 模式：速度渐变色（蓝→绿→黄→红）
- [ ] 切换不卡顿（< 200ms）

### 风险
- **R1**：Speed/Pressure 的渐变色计算依赖 G-code 解析数据，如果 GCodeRenderer 没解析速度数据，需补解析
- **R2**：Pressure/Pixel 模式可能依赖特殊数据（MSNP），如果上游数据不可得，这两个模式降级为占位

---

## SLICE-02: IMSlider 层滑块增强 [P0]

### 上游对照
- 上游：IMSlider.cpp 1828 行，自绘 slider + 刻度 + TickCode
- 我们：PreviewPage 的 LayerSlider 是基础 CxSlider，无刻度

### 实现方案

**1. 新建 `GCodeSlider.qml`**（`src/qml_gui/components/`）
- 替代当前 LayerSlider，自绘刻度 + TickCode 标记
- 属性：`int layerCount`, `int currentLayer`, `var tickCodes` (TickCode 数组)
- 信号：`layerChanged(int)`, `tickClicked(int tick)`, `tickRightClicked(int tick)`
- 渲染：
  - 水平底栏（高度 ~24px）
  - 刻度：层分隔线（每 N 层一条，对齐上游 tick spacing）
  - TickCode 标记：在对应层位置画彩色小方块
  - 滑块：可拖拽，显示当前层号 tooltip

**2. PreviewPage 接入**
- 替换 LayerSlider 为 GCodeSlider
- 绑定 `layerCount: previewVm.layerCount`
- 绑定 `currentLayer: previewVm.currentLayer`
- `onLayerChanged: previewVm.setCurrentLayer(layer)`

### UI 规格
- 位置：Preview 页底部全宽
- 高度：24px（刻度） + 20px（TickCode 标记）= 44px
- 刻度：灰色细线，每 10 层一条加粗
- 滑块：绿色圆角，宽 8px，拖拽时显示层号
- TickCode：6px 方块，ColorChange=挤出机色，Pause=黄色，Custom=灰色

### 数据流
```
用户拖拽滑块
  → GCodeSlider: emit layerChanged(newLayer)
  → PreviewPage: previewVm.setCurrentLayer(newLayer)
  → PreviewViewModel: emit currentLayerChanged
  → GLViewport: 渲染到 newLayer 截止
  → 用户看到该层及以下的 G-code
```

### 验收
- [ ] 拖拽流畅，实时更新 3D 视口
- [ ] 刻度清晰，每 10 层加粗
- [ ] 拖拽时显示层号 tooltip
- [ ] 键盘 ←→ 步进，Home/End 跳首尾，PgUp/PgDn 大步进

### 风险
- **R1**：自绘 slider 性能（大量 G-code 段时），需优化重绘范围
- **R2**：拖拽手感要接近原生（惯性/吸附），需调试

---

## SLICE-03: TickCode 自定义刻度插入 [P1]

### 上游对照
- 上游：TickCode.hpp `TickCodeInfo::add_tick()` + IMSlider 右键菜单
- 我们：无

### 实现方案

**1. PreviewViewModel 扩展**
- `Q_PROPERTY(QVariantList tickCodes READ tickCodes NOTIFY tickCodesChanged)`
- `Q_INVOKABLE addTick(int layer, int type, int extruder, QString color, QString extra)`
- `Q_INVOKABLE editTick(int layer, ...)`
- `Q_INVOKABLE removeTick(int layer)`
- 内部维护 `std::vector<TickCode>`，写入 libslic3r Model 的 custom_gcodes（对齐上游持久化）

**2. GCodeSlider 右键菜单**
- 右键空白刻度 → 菜单："Add Color Change" / "Add Pause" / "Add Custom G-code"
- 右键已有 TickCode → 菜单："Edit" / "Delete"

### UI 规格
- 右键菜单用 CxMenu（已有）
- Color Change：选择后弹颜色选择器（挤出机色或自定义）
- Pause：直接插入（黄色标记）
- Custom G-code：弹 CustomGcodeDialog（SLICE-04）输入文本

### 数据流
```
用户右键 slider 第 50 层 → 选 "Add Color Change"
  → 弹颜色选择器 → 用户选红色
  → GCodeSlider: 触发 addTick(50, ColorChange, extruder=1, "#FF0000", "")
  → PreviewViewModel: addTick(...) → 写入 Model custom_gcodes
  → emit tickCodesChanged → GCodeSlider 在第 50 层画红色方块
```

### 验收
- [ ] 右键弹菜单，3 个选项
- [ ] Color Change 插入红色方块，切片后该层换色
- [ ] Pause 插入黄色方块
- [ ] Custom G-code 弹 dialog 输入文本后插入灰色方块
- [ ] 编辑/删除已有 TickCode

### 风险
- **R1**：TickCode 写入 libslic3r Model custom_gcodes 后，切片时要重新应用——需确认 SliceService 支持
- **R2**：多挤出机场景的 extruder 编号映射

---

## SLICE-04: CustomGcodeDialog 联动 [P1]

### 上游对照
- 上游：CustomGcodeDialog 在 IMSlider 内部，输入文本后写入 TickCode.extra
- 我们：`CustomGcodeDialog.qml` 已存在，但未与 slider 联动

### 实现方案
- GCodeSlider 插入 "Custom G-code" 时，弹 `CustomGcodeDialog`
- Dialog 确认 → 返回文本 → `previewVm.addTick(layer, Custom, 0, "", text)`
- Dialog 取消 → 不插入

### 验收
- [ ] 选 "Add Custom G-code" → 弹 dialog
- [ ] 输入文本（如 "M300 S440 P100" beep）→ 确认 → 插入
- [ ] 切片后该层执行该 G-code

---

## SLICE-05: 切片冲突热区点击 [P2]

### 上游对照
- 上游 GCodeViewer 支持点击碰撞区域跳转/高亮

### 实现方案（v2.1 占位）
- **评估**：完整实现需 GCodeViewer 的碰撞检测数据，复杂
- **v2.1**：只做 UI 占位（如果有冲突数据，点击高亮；没有则不显示）
- 完整实现延后 v2.2

---

## SLICE-06: Legend 动态更新 [P2]

### 上游对照
- 上游 Legend 根据 EViewType 显示不同颜色映射

### 实现方案
- PreviewPage 的 Legend 组件改为绑定 `previewVm.gcodeViewMode`
- Feature 模式：显示 15 种 Feature 颜色图例
- Speed 模式：显示速度渐变色阶（0→max mm/s）
- 其他模式类似

### 验收
- [ ] 切换模式时 Legend 内容更新

---

## SLICE-07: ToolPositionTooltip 增强 [P2]

### 实现方案
- 当前有基础 tooltip，对齐上游增加：当前坐标（X/Y/Z）+ 层高 + 速度
- 绑定 GCodeRenderer 的鼠标悬停数据

---

## SLICE-08: 键盘导航对齐 [P2]

### 实现方案
- 当前有 ←→/Home/End/PgUp/PgDn
- 确认对齐上游 IMSlider 语义（步进量、大步进量、循环行为）

---

## 依赖关系图

```
SLICE-01 (着色切换) ← 独立
SLICE-02 (IMSlider)  ← 独立
   ↓
SLICE-03 (TickCode) ← 依赖 SLICE-02（slider 才能插刻度）
   ↓
SLICE-04 (Dialog 联动) ← 依赖 SLICE-03

SLICE-06 (Legend 动态) ← 依赖 SLICE-01
SLICE-05/07/08 ← 独立（P2 收尾）
```

---

## 实现顺序（推荐）

1. **SLICE-01** 着色切换（独立，preview 最有价值）
2. **SLICE-02** IMSlider（独立，后续 TickCode 基础）
3. **SLICE-03** TickCode（依赖 02）
4. **SLICE-04** Dialog 联动（依赖 03）
5. **SLICE-06** Legend 动态（依赖 01）
6. **SLICE-05/07/08** 收尾

---

## 整体验收

完成后 Preview 页具备：
- ✅ 6 种 G-code 着色模式切换
- ✅ 刻度化层滑块（含 TickCode 标记）
- ✅ 右键插入 ColorChange/Pause/CustomGcode
- ✅ 动态 Legend
- ✅ 键盘导航

**对标上游 IMSlider + GCodeViewer 的核心 preview 体验。**
