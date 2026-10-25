# 3DPrinter_Qt6 Claude Code Instructions

This repository is a source-truth migration project from OrcaSlicer to Qt6/QML (brand: OWzx).

See @.claude/rules/source-truth-migration.md for the canonical project migration rules.
See @.claude/rules/build-rules.md for the canonical build rules.

## Project Skills

- Use `/migrating-source-truth` to continue the next recorded migration task under the project rules. Append `all` for continuous batch mode.
- Use `/analyzing-source-truth-gap <task-or-feature>` to perform a read-only upstream-to-Qt gap analysis before implementation.

## Build

**唯一构建命令：** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`

**唯一构建目录：** `build/`

不得创建其他构建目录，不得使用其他构建脚本。详见 `.claude/rules/build-rules.md`。

## 上游对齐纪律（2026-10-25，来自模型池修复实战）

两层对齐，边界不可混：

1. **引擎核心（切片/路径/G-code 生成）一律对齐上游 Orca**——回归以 Orca 为锚，禁止 CP 语义进入。
2. **输入/配置方言层允许 CP 兼容超集**（我们的输入是 CP 生成的 3MF）。规则：
   - 每处兼容注释标注 `CP-DIALECT` 并写明上游对照行为；
   - 缺省值**派生自上游行为**（如 multicolor_method：上游无守卫=恒输出 → 耗材>1 即 true），不猜测 CP 行为硬编码；
   - 新配置键需同时加 PrintConfig defs **和** FullPrintConfig 静态成员（PrintConfig.hpp 宏列表）——defs map 不进静态类实例，只加 defs 不生效（0009 教训）。

完整说明与实例见 wzxBaseline 仓库 wzx_harness/README.md「对齐纪律」。
