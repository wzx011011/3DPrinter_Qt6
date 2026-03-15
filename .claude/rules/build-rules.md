# Build Rules

## 权威构建脚本

**唯一构建脚本：** `scripts/auto_verify_with_vcvars.ps1`

该脚本执行完整的 cmake configure + ninja build + smoke test 验证。
所有编译场景都必须使用此脚本，不得使用其他脚本或手动 cmake 命令。

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

## 构建目录

**唯一构建目录：** `build/`

不得创建或使用其他构建目录（如 `build_src/`、`cmake-build-*/` 等）。

## 禁止事项

- **禁止** 使用 `BUILD_LIBSLIC3R=OFF` — 必须保持 `ON` 以从源码编译 libslic3r
- **禁止** 删除 TK*.dll — 这些是 OpenCASCADE 运行时依赖
- **禁止** 创建新的构建脚本 — 所有构建需求通过修改 `auto_verify_with_vcvars.ps1` 实现

## 仅增量编译

如果只需要增量编译（跳过 cmake configure），在已有的正确构建环境下直接：

```powershell
cmake --build build --config Release
```

但前提是 `build/` 已由 `auto_verify_with_vcvars.ps1` 正确 configure 过。

## 其他脚本（保留）

| 脚本 | 用途 |
|------|------|
| `scripts/smoke_test.ps1` | 独立 smoke test（不编译） |
| `scripts/capture_qml_warnings.ps1` | QML 警告捕获 |
| `scripts/quick_run.ps1` | 快速启动应用 |
