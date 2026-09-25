# PRESET-DETAIL — Preset 管理 Dialog 套件详细规格

**Milestone:** v2.1
**需求域:** PRESET（Preset 管理 Dialog）
**创建日期:** 2026-06-17
**状态:** 待实现

---

## 上游真值（详细对照）

### 上游文件清单

| 文件 | 大小 | 作用 |
|---|---|---|
| `SavePresetDialog.cpp/.hpp` | 18KB | 保存当前参数为预设（模态对话框，含 Item 列表）|
| `UnsavedChangesDialog.cpp/.hpp` | 95KB | 切换 preset/project 时的 diff 守卫（树形对比）|
| `ExportPresetBundleDialog.cpp/.hpp` | 23KB | 导出预设包（多选 + 压缩）|
| `CreatePresetsDialog.cpp/.hpp` | 264KB | 批量创建/导入预设（巨型，多 tab）|

### SavePresetDialog 结构（上游）

- 继承 `DPIDialog`（模态）
- `ActionType { ChangePreset, AddPreset, Switch, UndefAction }`
- 含 `Item` 列表（每个 Preset::Type 一个 Item）：
  - Item 含：preset 名称 ComboBox + 输入框 + 校验图标 + "Save to project" RadioGroup + Detach checkbox
- 构造支持单 type 或多 type（`vector<Preset::Type>`）
- 公开接口：`get_name()`, `get_name(type)`, `get_save_to_project_selection()`, `get_detach_value()`
- 确认后调用 `accept()` → 写入 PresetCollection

### UnsavedChangesDialog 结构（上游）

- 用 `DiffModel`（DataView 树形）展示 preset A vs B 的参数差异
- `ModelNode` 含：参数名 + 旧值/新值 + 颜色 + 子节点
- 事件 `EVT_DIFF_DIALOG_TRANSFER`（Transfer 按钮）
- 按钮组：Save（保存当前）/ Transfer（迁移到新）/ Discard（丢弃）/ Cancel

---

## 当前实现状态（v2.0 之后）

`src/qml_gui/dialogs/` 14 个 Dialog **都没有** SavePreset/UnsavedChanges/ExportBundle/CreatePresets。

SettingsPage 有"保存预设"动作入口，但无 dialog 接收。

---

## PRESET-01: SavePresetDialog [P0]

### 上游对照
- 上游：SavePresetDialog.hpp（Item 列表 + 校验 + save-to-project + detach）
- 我们：无

### 实现方案

**1. 新建 `SavePresetDialog.qml`**（`src/qml_gui/dialogs/`）
- 模态对话框（基于 CxDialog）
- 属性：`int presetType`（Printer/Filament/Print）, `string suggestedName`
- 内容：
  - 标题："Save Preset"
  - Item 区（单 type 简化版）：
    - 名称输入框（ComboBox 可选已有 + 输入新名）
    - 校验图标（重名警告/合法）
    - "Save to project" 复选框
  - 按钮：[Save] [Cancel]

**2. PresetServiceMock 扩展**
- `Q_INVOKABLE savePreset(int type, QString name, bool saveToProject)` → 写入 PresetCollection
- `Q_INVOKABLE presetExists(int type, QString name)` → 校验重名

**3. SettingsPage 接入**
- "保存预设"按钮 → 弹 SavePresetDialog
- Dialog 确认 → `presetService.savePreset(...)`

### UI 规格
- 尺寸：440×280（DPI 自适应）
- 名称输入：ComboBox（已有预设列表）+ 可编辑
- 校验：重名时红色警告图标 + 提示
- 按钮风格对齐 CxDialog

### 数据流
```
SettingsPage 点 [保存预设]
  → 弹 SavePresetDialog { presetType, suggestedName: "MyPreset (modified)" }
  → 用户输入名 "MyCustom"
  → Dialog 校验：presetService.presetExists(Print, "MyCustom")? 否 → 合法
  → 点 [Save]
  → presetService.savePreset(Print, "MyCustom", false)
  → PresetCollection 写入 + emit presetListChanged
  → SettingsPage 预设列表更新
```

### 验收
- [ ] 弹 dialog，显示当前 type + 建议名
- [ ] 重名校验（红色警告）
- [ ] 保存后预设列表出现新预设
- [ ] Save to project 选项生效

### 风险
- **R1**：PresetServiceMock 的 savePreset 需真实写入（对齐上游 PresetCollection::save_current_preset）
- **R2**：多 type 批量保存（上游支持 vector），v2.1 先做单 type

---

## PRESET-02: UnsavedChangesDialog [P1]

### 上游对照
- 上游：UnsavedChangesDialog（95KB，DiffModel 树形 diff）
- 我们：无

### 实现方案

**1. 新建 `UnsavedChangesDialog.qml`**
- 模态对话框
- 属性：`string oldPresetName`, `string newPresetName`, `var diffEntries`（参数 diff 数组）
- 内容：
  - 标题："Unsaved Changes — {oldName} → {newName}"
  - 树形 diff 列表（参数名 + 旧值 → 新值）
  - 按钮：[Save][Transfer][Discard][Cancel]

**2. PresetServiceMock 扩展**
- `Q_INVOKABLE diffPresets(int type, QString oldName, QString newName)` → 返回 diff 数组
- `Q_INVOKABLE transferPreset(...)` / `discardChanges()`

**3. 切换守卫接入**
- SettingsPage/任何 preset 切换前：检查当前 preset 是否 dirty
- dirty → 弹 UnsavedChangesDialog
- 用户选择后继续切换

### UI 规格
- 尺寸：600×400（diff 较多）
- diff 树：参数名 + 旧值（红）→ 新值（绿）
- 按钮：Save（保存当前）/ Discard（丢弃修改）/ Cancel（不切换）

### 验收
- [ ] dirty preset 切换时弹 dialog
- [ ] diff 列表显示参数差异
- [ ] Save 保存后切换 / Discard 丢弃后切换 / Cancel 中止

### 风险
- **R1**：diff 计算需对比 ConfigOption 值，PresetServiceMock 需支持
- **R2**：95KB 的上游实现复杂（树形 + 颜色 + Transfer），v2.1 简化版（平铺列表 diff）

---

## PRESET-03: ExportPresetBundleDialog [P2]

### 上游对照
- 上游：ExportPresetBundleDialog（23KB，多选 + 压缩导出）

### 实现方案（v2.1 简化）
- 新建 dialog：勾选要导出的预设 → FileDialog 选路径 → 压缩导出
- v2.1 可简化为"导出全部预设"（不做多选）

### 验收
- [ ] 选路径 → 导出 .zip/.bbscfg

---

## PRESET-04: CreatePresetsDialog [P2]

### 上游对照
- 上游：CreatePresetsDialog（264KB，巨型，多 tab 批量创建）

### 实现方案（v2.1 评估）
- **评估结论**：264KB 文件 + 多 tab + 批量导入，工作量巨大
- **v2.1 决策**：**延后到 v2.2**（标记 [ ] 不做）
- 理由：性价比低，单预设保存（PRESET-01）已覆盖大部分用户需求

---

## 依赖关系图

```
PRESET-01 (SavePreset) ← 独立（P0 优先）
   ↓
PRESET-02 (UnsavedChanges) ← 依赖 PRESET-01（切换守卫在保存基础上）

PRESET-03 (ExportBundle) ← 独立（P2）
PRESET-04 (CreatePresets) ← v2.2 延后
```

---

## 实现顺序

1. **PRESET-01** SavePresetDialog（独立，P0）
2. **PRESET-02** UnsavedChangesDialog（依赖 01）
3. **PRESET-03** ExportBundle（独立，P2）
4. PRESET-04 延后

---

## 整体验收

完成后 SettingsPage 具备：
- ✅ 保存当前参数为预设（SavePresetDialog）
- ✅ dirty preset 切换守卫（UnsavedChangesDialog）
- ✅ 导出预设包（ExportPresetBundleDialog）

**对标上游预设管理主流程。**
