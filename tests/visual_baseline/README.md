# 视觉回归基线（tests/visual_baseline/）

本目录存放 **VisualRegressionTests**（X4，审计 2026-10-10 ④ X4）的截图基线 PNG。
套件在固定视口（1440×900）+ offscreen QPA + Qt Quick **software** 后端下渲染各关键页面，
与这里的 PNG 做逐像素容差比对（单通道容差 12，超差像素占比预算 0.2%）。

**门禁永不写基线**：verify 里跑的套件只读本目录；基线缺失/超差/尺寸不符一律红。
写基线只能由开发者显式触发（见下），且产出以 PNG diff 形式进 git review——
"更新基线即通过"在机制上不可能：基线变更在 diff 里可见，必须随代码一起被评审。

## 何时更新基线

只有一种正当理由：**本次提交有意改变了页面视觉**（QML 布局/样式/行为改动，
或 Qt 底座/字体管线变更——后者是全量重建，须在提交说明里写明前后环境）。
"先跑红→不知道为什么→顺手更新基线"不是理由；先按失败信息里的
`visual_failures/<页面>__diff.png` 归因，确认是预期改动再更新。

## 如何更新

```powershell
# 全量重建（Qt 底座/字体/视口变更）
$env:OWZX_VISREG_UPDATE_BASELINE = "1"
build\VisualRegressionTests.exe
Remove-Item Env:OWZX_VISREG_UPDATE_BASELINE

# 单页重建（日常 QML 改动；OWZX_VISREG_PAGE 只捕获该页）
$env:OWZX_VISREG_UPDATE_BASELINE = "1"; $env:OWZX_VISREG_PAGE = "PreparePage"
build\VisualRegressionTests.exe
Remove-Item Env:OWZX_VISREG_UPDATE_BASELINE, Env:OWZX_VISREG_PAGE
```

套件在更新模式会打印 **old-vs-new 差异指标**（超差像素数/占比、maxChannelDiff、
meanAbsDiff、aHash 距离）——**原样引用进提交说明**，这是评审判断改动幅度的依据。

## 提交规则（评审清单）

1. 基线 PNG 必须**与它祝福的 QML/C++ 改动同一提交**；基线-only 提交需在说明里
   给出独立正当性（环境变更归因 + 全量重建证据），否则打回。
2. 提交说明引用更新模式打印的前后指标，并附一句话改动摘要（"按钮 3450 从 hollow
   开关换 CxSwitch，PreparePage 超差 0.8%，布局无漂移"级别）。
3. 评审人看 PNG diff（GitHub/吉提上可直接看图）：确认视觉变化 == 代码意图，
   没有夹带无关漂移。发现夹带即打回。
4. 页面改名/删除时，同名 PNG 在**同一提交**里删除（套件会把无行基线判红）。
5. 禁止调整套件容差（`kPixelChannelTolerance` / `kMaxMismatchFraction`）来"消化"
   一次回归——那是把门禁拆掉，等价于删测试。

## 环境一致性

基线由本机 canonical 环境（Qt 6.12 LTS + Windows + 内置 HarmonyOS Sans 字体）生成。
换 Qt 版本、换字体、改视口尺寸（套件常量）都会全量失配——走全量重建并按上面第 1/2 条
记录前后环境。软件后端抹掉了 GPU/驱动差异；余下的机器间差异（字体 hinting 微差）
落在容差预算内，不构成更新理由。
