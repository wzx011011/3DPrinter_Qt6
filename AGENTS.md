# 3DPrinter_Qt6 Codex Instructions

This repository is a source-truth migration project from OrcaSlicer to Qt6/QML (brand: OWzx).

See @.codex/rules/source-truth-migration.md for the canonical project migration rules.
See @.codex/rules/build-rules.md for the canonical build rules.

Screenshot-driven UI milestones use screenshots as visual/layout truth and OrcaSlicer source as behavior truth. If an existing page is materially off-design, replace it and remove the old files, routes, registrations, tests, imports, and resources instead of keeping deprecated UI code.

## Project Skills

- Use `/migrating-source-truth` to continue the next recorded migration task under the project rules. Append `all` for continuous batch mode.
- Use `/analyzing-source-truth-gap <task-or-feature>` to perform a read-only upstream-to-Qt gap analysis before implementation.

## Build

**唯一构建命令：** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`

**唯一构建目录：** `build/`

不得创建其他构建目录，不得使用其他构建脚本。详见 `.codex/rules/build-rules.md`。

## Self-Test Standing Rules（自测常备规则）

每次做了与下列测试相关的改动后，必须重新运行对应自测，且效果不得倒退：

- **主流程自测**（canonical verify gate）：`powershell -NoProfile -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`。凡改动 C++/QML 源码、测试或构建脚本后都要跑；12 套件必须 0 failed。
- **效率自测**（stage benchmark）：`powershell -NoProfile -ExecutionPolicy Bypass -File scripts/perf/run_stage_bench.ps1`。凡改动模型加载、meshData、场景展开、拾取、切片、预览解析或渲染提交路径后都要跑；各阶段耗时与基线（docs/perf-baseline.md）相比不得出现超出噪声（~10%）的回退，基线由 `build/perf_out/stage_report.json` 更新。
