# Tag 触发编译与发布（GitHub Actions）

本仓库已配置工作流：`.github/workflows/tag-build.yml`

- CI：自动构建（验证代码可编译）
- CD：自动发布（创建 GitHub Release 并上传发布包）

## 触发方式

- 推送任意 tag（如 `v0.1.0`）会自动触发 Windows 编译。
- 也支持在 GitHub Actions 页面手动触发（`workflow_dispatch`）。

## 本地发布步骤（建议）

1. 先确保当前分支可构建（本地已通过 smoke / auto verify）。
2. 提交代码：
   - `git add .`
   - `git commit -m "chore: prepare release"`
3. 打 tag：
   - `git tag v0.1.0`
4. 推送分支和 tag：
   - `git push`
   - `git push origin v0.1.0`

## 产物位置

- 编译完成后可在对应 workflow run 的 `Artifacts` 中下载原始产物。
- 同时会自动创建对应 tag 的 GitHub Release，并上传：
   - `FramelessDialogDemo-<tag>-windows.zip`

## 说明

- 当前 CI 以“编译成功”为主，默认关闭 `libslic3r`（`-DBUILD_LIBSLIC3R=OFF`），用于稳定完成 tag 构建验证。