# 对照报告：SettingsDialog 七区域 vs 上游 Tab.cpp（代码级）

> 日期：2026-09-15 ｜ 性质：**代码级对照**（双侧截图待 P0 修复后补采）
> 真值：`third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp`（Tab::Tab 对话框骨架、
> m_tabpanel 页签、OptionsGroup、search、底部按钮行）、`Tabbook.cpp`、
> `ConfigManipulation.cpp`
> 对象：`src/qml_gui/dialogs/SettingsDialog.qml`（875 行，三实例共享，PSET2-03）

## 逐区域状态（v3.6 旧结论 → 现行核实）

| 区域 | v3.6 旧结论 | 现行核实（2026-09-15） | 证据 |
| --- | --- | --- | --- |
| SETPRINT-SHELL 对话框外壳 | Missing（嵌入页） | **Real**：独立 SettingsDialog 已落地（v3.6 SETTINGS-01 决策执行），875 行、三实例（print/filament/printer）共享单模态门 | :19/:184/:247 |
| SETPRINT-PRESETBAR 预设栏 | Hybrid | **Hybrid（维持）**：预设下拉 + 脏状态悬停指示（:412）+ 兼容徽标（:431）+ 创建预设入口（:384，Phase 236 DLG-01）+ 对比预设（:470，CLOS-01）+ 保存按钮（:457） | :384-470 |
| SETPRINT-TABS 类别页签 | Hybrid | **Real**：processTabStrip（工艺三级分组页签 :528）+ genericTabStrip（打印机/耗材 :628）双形态页签，仅列有真实选项源的页签（PSET2-08 :98） | :528/:628 |
| SETPRINT-GROUPNAV 左侧组导航 | Placeholder | **Hybrid**：工艺层组导航已实现（processGroupDelegate :716，分组列 + 组内有序选项 :761）；打印机/耗材层为搜索+平铺列表（上游亦以搜索为主要导航，分组导航深度差异保留波次 4） | :716-790 |
| SETPRINT-OPTIONS 参数编辑区 | Hybrid | **Hybrid（维持）**：processOptionListComponent（:685）+ genericOptionListComponent（:808），经 `configVm.filterOptionIndices(presetTier, searchText, advancedMode)`（:165）统一过滤 | :685/:808/:165 |
| SETPRINT-SEARCH 搜索+进阶 | Placeholder | **Real**：compactSearchField（:501）+ advancedMode 开关（:516-521）接入过滤管线（:165/:195） | :501/:516 |
| SETPRINT-FOOTER 底部操作栏 | Placeholder | **仍有缺口（本轮确认）**：上游 Tab.cpp 对话框底部有 Save/放弃/取消 按钮行；OWzx 以"预设栏保存按钮 + 关闭时 UnsavedChangesDialog 守卫（:183-246 openUnsavedChangesGuard + :246 实例）"替代——功能等价但形态不同。**波次 2 剩余任务：补底部按钮行（保存=saveCurrentPreset、放弃=discard、关闭=守卫路径）** | :183-246/:457 |
| SETPRINT-DIRTY 脏状态指示 | Hybrid | **Real**：isPresetDirty 标记 + dirtyHover 悬停详情 + 兼容徽标 + UnsavedChangesDialog 联动（PSET2-03 pending keys 机制 :261） | :412/:247-269 |

## 结论与下一步

- 七区域中 5 项已达 Real/Hybrid 且 v3.6 三项 Placeholder 结论（SHELL/SEARCH/FOOTER）中两项已被后续 Phase 落地，矩阵同步为现行真值
- **唯一形态级缺口 = SETPRINT-FOOTER 底部操作栏**（波次 2 剩余）：补 Save/放弃/关闭 三按钮行，复用既有 `configVm.saveCurrentPreset()` 与 openUnsavedChangesGuard 路径
- **截图债**：双侧截图（上游 Tab 对话框 vs SettingsDialog 三实例）待 P0 修复后补采
- **审计锁定**：既有 SettingsDialog 91 处审计引用 + PSET2 系列断言覆盖本轮核实区域；FOOTER 落地时补页脚三按钮断言
