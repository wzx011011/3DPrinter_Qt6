# Roadmap: Milestone v2.4 — Project & Preset Real IO

## Overview

补全项目/Preset 的真实文件 IO——.3mf 读写 + PresetBundle 导入导出，让文件操作不再是占位。

**工作方式：** Phase 模式（沿用 v2.3）

---

## Phase 结构

### Phase 1: ProjectService 真实 .3mf 导出

| Task | 内容 | 验收 |
|---|---|---|
| IO-01 | saveProjectAs(path) 调 libslic3r 3mf.cpp | 保存后文件存在 + 可重新 loadFile |
| IO-02 | exportModel(path, format) STL/3MF/OBJ | 导出文件可被其他切片软件打开 |
| IO-03 | ProjectPage 保存/另存接 saveProjectAs | 点击保存→选路径→文件生成 |

### Phase 2: PresetBundle 导入导出

| Task | 内容 | 验收 |
|---|---|---|
| IO-04 | exportBundle(path) 导出预设包 | 生成 .zip 含 print/filament/printer |
| IO-05 | importBundle(path) 导入预设包 | 导入后预设列表更新 |
| IO-06 | ExportPresetBundleDialog 接 exportBundle | 对话框确认→文件生成 |

### Phase 3: 集成 + 自回归

| Task | 内容 | 验收 |
|---|---|---|
| IO-07 | 自回归扩展（saveProject + exportBundle） | 脚本跑通 0=PASS |
| IO-08 | ProjectPage 导出按钮接 exportModel | 区分 G-code/3mf 导出 |
| IO-09 | ConfigViewModel 接通真实 PresetBundle | preset 增删改链路完整 |

---

## 上游参考

- **.3mf 导出**：`libslic3r/3mf.cpp` (147KB) — `store_3mf()` / `export_3mf()`
- **项目保存**：`Plater::save_project()` → `export_3mf()` (Plater.cpp:12165)
- **PresetBundle**：`PresetBundle::export_config_bundle()` / `load_config_bundle()`
- **导出模型**：`Plater::export_model()` — STL/3MF/OBJ/AMF

---

## Out of Scope (v2.4)

- Device/Cloud/Network 真实化（v2.5+）
- Calibration 真实化（v2.5+）
- PartPlate/AssembleView（v2.5+）
- Gizmo UI 完善（v2.5+）

---
*Last updated: 2026-06-18 — v2.4 (Project & Preset Real IO) 启动；Phase 模式；9 Tasks 分 3 Phase*
