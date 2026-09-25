# PREPARE-DETAIL & SEARCH-DETAIL — Prepare 打磨 + Settings 搜索详细规格

**Milestone:** v2.1
**需求域:** PREPARE（v2.0 遗留 G6/G8）+ SEARCH（SearchDialog 集成）
**创建日期:** 2026-06-17
**状态:** 待实现

---

## PREPARE 域

### PREPARE-01: BBLTopbar 样式打磨（G6）[P1]

#### 上游对照
- 上游：MainFrame + BBLTopbar（wxWidgets）
- 我们：`BBLTopbar.qml`（702 行，Phase 2 基础框架，样式粗糙）

#### 当前问题
- tab 按钮：纯文字（"首页/准备/预览..."），绿色高亮当前
- 上游：图标+文字，间距精致，配色层次更丰富

#### 实现方案

**1. BBLTopbar tab 重设计**
- 每个 tab 加图标（首页🏠/准备⚙/预览👁/设备📡/项目📁/校准🎯）
- 文字+图标水平排列，间距 8px
- 选中态：绿色背景 + 白色图标文字；未选：灰色
- hover 态：微亮背景

**2. 配色对齐**
- tab 栏背景：bgPanel
- 分隔线：borderSubtle
- 选中高亮：accent (绿色)

#### UI 规格
- tab 高度：40px
- 图标尺寸：16px
- 文字 fontSize：12px
- tab 间距：4px（紧凑）

#### 验收
- [ ] 截图对比上游，tab 风格接近
- [ ] hover/选中态正确
- [ ] 图标清晰可辨

#### 风险
- **R1**：图标资源（需找/制作 6 个 SVG，对齐上游 icon）

---

### PREPARE-02: 配色对比度（G8）[P2]

#### 上游对照
- 上游区块背景 vs 主背景对比度更明显
- 我们：bgPanel/bgElevated 差异不够

#### 实现方案
- 调整 Theme.qml 令牌：
  - bgElevated：从 `#252b38` 调到 `#2a3140`（更亮，增强层次）
  - borderSubtle：从 `#2a3040` 调到 `#333b4e`（更明显）
- 影响全局，需谨慎测试

#### 验收
- [ ] 区块层次比之前清晰（截图对比）
- [ ] 不破坏其他页面（全局回归检查）

#### 风险
- **R1**：改 Theme 影响所有页面，可能引入新违和，需逐页检查

---

## SEARCH 域

### SEARCH-01: SearchDialog 接入 SettingsPage [P1]

#### 上游对照
- 上游：SearchCtrl 在 ParamsPanel 顶部，输入跳转参数
- 我们：`SearchDialog.qml` 已存在，但未集成入 SettingsPage tier

#### 当前问题
- SearchDialog 独立存在，SettingsPage 无搜索入口
- 上游在 Process 顶部条右侧有 SearchCtrl

#### 实现方案

**1. SettingsPage 加搜索入口**
- 在 tier 标题栏右侧加搜索图标按钮
- 点击 → 弹 SearchDialog（或内联展开搜索框）

**2. SearchDialog 接入**
- 数据源：ConfigViewModel 的所有参数（printOptions/filamentOptions/machineOptions 合并）
- 输入 → 模糊匹配参数名/标签
- 选中结果 → 跳转到对应 tier + 滚动到该参数 + 高亮

**3. ConfigViewModel 扩展**
- `Q_INVOKABLE searchOption(QString keyword)` → 返回匹配列表 [{tier, optionKey, label}]

#### UI 规格
- 搜索入口：SettingsPage 标题栏右侧 🔍 按钮
- SearchDialog：450×400，输入框 + 结果列表
- 结果项：参数名 + 所属 tier 标签
- 点击结果 → 关闭 dialog + 跳转

#### 数据流
```
SettingsPage 点 🔍
  → 弹 SearchDialog
  → 用户输入 "layer height"
  → configVm.searchOption("layer height") → 返回 [{Print, layer_height, "层高"}]
  → 用户点 "层高"
  → SearchDialog 关闭 + emit optionSelected(Print, "layer_height")
  → SettingsPage 切到 Print tier + 滚动到 layer_height + 高亮 1s
```

#### 验收
- [ ] 搜索入口可点
- [ ] 输入关键词返回匹配结果
- [ ] 点击结果跳转到对应参数
- [ ] 跳转后参数高亮

#### 风险
- **R1**：参数搜索的模糊匹配质量（需对齐标签而非 key）
- **R2**：滚动定位 + 高亮需要 ParamsPage 支持

---

## 依赖关系

```
PREPARE-01 (BBLTopbar) ← 独立（需图标资源）
PREPARE-02 (配色) ← 独立（但全局影响，最后做）
SEARCH-01 (Search) ← 独立
```

---

## 实现顺序（在 SLICE/PRESET 之后）

1. **SEARCH-01**（独立，SettingsPage 深化）
2. **PREPARE-01**（BBLTopbar 打磨，需图标）
3. **PREPARE-02**（配色，最后做，全局影响）
