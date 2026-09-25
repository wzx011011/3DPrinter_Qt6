---
quick_id: 260702-1bn
slug: prepare-roadmap-audit-downgrade
date: 2026-07-02
status: complete
---

# Quick Task 260702-1bn — Summary

## What
按交互级审计标准（可见 + 可点 + 后端非 no-op + 非 stub）重审 `docs/准备页收敛路线图.md` 初版基线表，纠正视觉级过度乐观判定。

## Changes（单文件：`docs/准备页收敛路线图.md`）
- **第 2 节基线表**：10 行加"审计判定"列；4 行从 ✅ 降级为 ⚠️（#2 preset 体系 / #5 对象列表 / #9 gizmo 面板；#4 搜索栏保留 ✅ 记小瑕疵）；#8 EditorViewModel 标 INFO
- **新增第 3 节**：17 处误导性假绿清单，按根因分类——
  - A（`CollapsibleSection.qml:84-88` MouseArea 吞点击，3 症状）
  - B（CGAL 5.4 < 5.6：MeshBoolean/Drill/简化右键）
  - C（死信号死路径："在参数表中编辑"4 入口 / 导出STL / 修复网格 / 重载 / 工艺☰）
  - D（上游未迁移：Measure / Flatten / 分组缺失 / 添加图元文字SVG / 导入按钮）
  每处带 file:line 证据
- **新增第 4 节**：修复优先级 P0/P1/P2/P3（横向，跨 phase），编号对齐第 3 节
- **第 5-8 节**：原 Phase 56-59 路线图/依赖/锚点/验证方式顺延编号并保留；锚点表补充审计新发现文件

## Key Findings（17 处假绿 TOP 5）
1. `CollapsibleSection.qml:84-88` MouseArea 吞点击 → 齿轮⚙ / ↻ / 红点 hover 全死（**1 改修 3 个**）
2. "在参数表中编辑"4 入口 DEAD（`selectionSettingsRequested` 信号全仓库 0 连接）
3. MeshBoolean/Drill 面板 STUB（`ProjectServiceMock.cpp:2980/3167` 硬编码 return false，CGAL 版本阻塞）
4. Measure STUB（`EditorViewModel.cpp:90` 只读 bbox，无点/边/面拾取）—— 最隐蔽假绿
5. 分组功能 MISSING（基线表初版误标"分组"，实际只有只读 groupHeader）

## Audit Evidence
4 组并行审计 agent（general-purpose），逐入口核查到 `.cpp` 实现层（不只读 `.h`），结果存于会话上下文。判定标准记于 `memory/alignment-verification-standard.md`。

## Scope
- **只改文档**，未改 src 代码
- P0 修复（CollapsibleSection / 耗材按钮 / "在参数表中编辑"）**未做**，留作后续独立 quick task（用户已选"先 A 再 B"，B 即 P0 修复）

## Verify（done criteria 全满足）
- [x] 文档 8 节结构完整
- [x] 基线表 4 降级行判定准确，每行有 file:line
- [x] 17 处假绿每处有 file:line + 根因归类
- [x] 修复优先级 P0/P1/P2/P3 与第 3 节编号对应
- [x] 第 5-8 节原 Phase 56-59 内容保留无丢失
