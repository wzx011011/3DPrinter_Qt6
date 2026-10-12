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

- **主流程自测**（canonical verify gate）：`powershell -NoProfile -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`。凡改动 C++/QML 源码、测试或构建脚本后都要跑。**真实通过口径（2026-10-12 Q14 QML 冒烟套件入门禁后）**：**全部 13 套件 0 failed 且脚本 exit 0**——2026-08-31 起 AppTool / ViewportContextMenu / E2E 三套件曾因 import 期 heap-corruption 家族而黄字非阻塞（9 阻塞+3 非阻塞口径，Q13a 成文），Q13 已归因为陈旧 moc/混代增量构建类问题并以 autogen timestamp 强制重扫根治（见 `.codex/rules/build-rules.md` 处置记录），三套件于 2026-10-12 恢复红字 exit，E2E 随 Q12 环境探测语义（缺前置才 SKIP 并计数、有前置失败必须 FAIL）一并转阻塞；Q14 同日把 QML 行为冒烟套件 QmlSmokeTests 加入门禁（minimal 平台插件离屏加载 main.qml+10 页面，断言无 QML 错误、context property/关键属性可读、页面可实例化；S1/X2 的行为安全网）。三套测试的 `initTestCase` 已安装 CrashHandlerWin，未来任何崩溃都会在 `build/crash_dumps/` 留下栈与 minidump，不再静默。切片回归门禁（触发条件下自动加跑，见下）必须绿。
- **切片回归门禁**（slice regression gate，内嵌于 canonical verify，自动触发）：当未提交改动或自上次绿门禁以来的提交触碰到切片链路面（SliceService / PreviewViewModel / EditorViewModel / `src/cli/*` / ProjectServiceMock / PresetServiceMock / OrcaSlicer 子模块指针 / PerfBench.cpp / 门禁自身文件）时，verify 末尾自动运行 `scripts/perf/run_slice_gate.ps1`。正确性层 = `owzx-cli` 钉死配置切基准模型后经 `tools/gcode_compare.py` 归一化，与 `tests/data/slice_gate_baseline.json` 黄金基线逐字段硬比对（退化即 verify 失败）；效率层 = PerfBench 切片档 min-of-N 对比基线阈值（slice_ms +25% / preview_parse_ms +10%，超阈自动复测一轮）；步骤画像层（P4，2026-10-13 起）= 4 个代表模型（sphere_400k / overhang_tower / grid_20x100k / block_20xy）各跑一次逐步骤计时画像（`SliceService::sliceStepTimeline()` → `slice_steps_ms`），单步骤退化 >+30% 且基线步骤 ≥20ms 才记失败、超阈自动复测一轮画像、两轮都超才 exit 2，步骤键集变化（引擎 set_status 文本变更）即红。`OWZX_SLICE_GATE=0` 跳过，`=force` 强跑；有意变更切片输出或引擎步骤文本后用 `-UpdateBaseline` 重新建金并连同改动一起提交。详见 `.codex/rules/build-rules.md`。
- **效率自测**（stage benchmark）：`powershell -NoProfile -ExecutionPolicy Bypass -File scripts/perf/run_stage_bench.ps1`。凡改动模型加载、meshData、场景展开、拾取、切片、预览解析或渲染提交路径后都要跑；各阶段耗时与基线（docs/perf-baseline.md）相比不得出现超出噪声（~10%）的回退，基线由 `build/perf_out/stage_report.json` 更新。
