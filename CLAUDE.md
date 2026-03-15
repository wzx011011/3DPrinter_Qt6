# 3DPrinter_Qt6 Claude Code Instructions

This repository is a source-truth migration project from CrealityPrint to Qt6/QML.

See @.claude/rules/source-truth-migration.md for the canonical project migration rules.
See @.claude/rules/build-rules.md for the canonical build rules.

## Project Skills

- Use `/continue-source-truth-migration` to continue the next recorded migration task under the project rules.
- Use `/analyze-source-truth-gap` to perform a read-only upstream-to-Qt gap analysis before implementation.

## Build

**唯一构建命令：** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`

**唯一构建目录：** `build/`

不得创建其他构建目录，不得使用其他构建脚本。详见 `.claude/rules/build-rules.md`。
