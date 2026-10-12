# UI 还原映射 · 设计基准网页 → QML 细节还原

> 建立日期：2026-09-24。配套工具：[index.html](index.html)（上游视觉真值的网页化重建）。
> 口径不变：**上游运行截图 = 视觉真值**（`docs/visual-compare/upstream_*.png`），
> `third_party/OrcaSlicer` 源码 = 行为真值（[ui-full-restoration-inventory.md](../ui-full-restoration-inventory.md)）。

## 1. 设计基准网页是什么

`docs/ui-reference/index.html` 是上游截图的**可检查重建**：1366×721 固定艺术板，
全部颜色/几何逐像素采样自上游截图（非自由设计），并内置还原工具：

| 工具 | 快捷键 | 用途 |
|---|---|---|
| 叠加截图 | `O` + 透明度滑杆 | 上游截图 50% 叠在网页上直接看偏差 |
| 差值模式 | `D` | mix-blend difference，像素级对齐 |
| 检查器 | `I` | 悬停任意区域：尺寸/字号/色值 + 上游源码 + QML 目标文件 |
| Token 对照表 | `T` | 上游值 ↔ `Theme.qml` 现值逐条列出 |
| 页面状态 | `1`/`2` | 准备页 / 预览页（按钮可用性按截图规律切换） |

截图/比对脚本（无头 Chrome，窗口高按本机 DPI 需补偿，实测 ×0.8556）：

```powershell
& "C:\Program Files\Google\Chrome\Application\chrome.exe" --headless=new --disable-gpu `
  --force-device-scale-factor=1 --window-size=1420,860 --hide-scrollbars `
  --screenshot="out.png" "file:///E:/ai/3DPrinter_Qt6/docs/ui-reference/index.html?shot=1&page=prepare"
# ?shot=1 锁定 1:1 原点；裁剪 (0,0,1366,721) 后与 docs/visual-compare/upstream_*.png 做逐像素差
```

当前保真度：与上游两图逐像素差（阈值>40）**3.5%**，残余为字体光栅化差异、
打印机照片占位、手绘 SVG 图标——不影响作为几何/色值基准使用。

## 2. 量化真值表（自上游截图采样，坐标即证据）

| Token | 值 | 采样位置 (upstream_prepare.png) |
|---|---|---|
| 标题栏底 | `#262e30` | (700,16) |
| 导航行底 | `#2d2d30` | (700,45) |
| 主体底（侧栏/视口/卡片/输入框） | `#2d2d31` | (200,430) |
| 区头横带（打印机/耗材丝/工艺） | `#36363b` | (200,103)，**高 30px**（y=89..118） |
| 主强调（激活页签填充/胶囊按钮/箭头盒） | `#00675b`（Material Teal 900） | 页签 (100,40) |
| 亮强调（页签下划线/侧栏停靠手柄） | `#009688`（Teal 500） | 下划线 y=379..381 |
| 耗材丝序号方块 | `#26a69a`（Teal 400），24×24 | (13..38, 243..262) |
| 滚动条槽 / 滑块 | `#171717` / `#959595` | x=374..391 |
| 禁用胶囊底 / 文字 | `#8e8e83` / `#56564f` | prepare 导出胶囊 (1200,58) |
| 控件描边 | `≈#505054` 1px | 输入框边 (246..365, 435) |
| 分隔线 | `#000000` 1px | y=33（标题栏下）、x=391（侧栏右） |

几何真值：标题栏高 32；导航行高 38（激活页签填充 36 顶部对齐，x=40..175）；
侧栏总宽 **392**（内容 374 + 滚动条 18）；参数行高 30、输入框 25×119、行距 30；
工艺预设输入 284×25 @ (34,320)；耗材丝输入 29 高 @ y238..267；
质量..其他 页签间距 ~25px；分体按钮：箭头盒 25×24 恒 teal + 胶囊 24 高/14px 字，
切片胶囊宽 ~98、导出 ~147、两按钮间距 15、右缘距窗口 30；
标题居中于左工具簇末端(x≈210)→窗口右缘之间；窗口控制键宽 ~50。

## 3. 区域 → QTL 目标映射与当前差距

| # | 区域 | 上游源 | QML 目标 | 上游值 | 当前值 | 差距 |
|---|---|---|---|---|---|---|
| R1 | **全局色板基调** | 截图 | `Theme.qml` | 中性灰 `#2d2d31` 系 + Material Teal | 蓝黑 `#161a23` 系 + 绿 `#18c75e` | **需决策**（见 §4） |
| R2 | 标题栏高度/底色 | BBLTopbar.cpp | `BBLTopbar.qml` titleToolRow | 32 / `#262e30` | 36 / `chromeSurface #10161e` | 高度 −4、色未登记 |
| R3 | 导航行高度 | MainFrame.cpp notebook | `BBLTopbar.qml` workflowBar | 38 | 34 | −4 |
| R4 | 激活页签填充色 | 同上 | 同上 background | `#00675b` | `Theme.accent #18c75e` | 品牌色（关联已登记偏离 #1） |
| R5 | 切片/导出分体按钮 | Plater.cpp reslice | `BBLTopbar.qml` prepareSliceButton/prepareExportGcodeButton | [箭头盒][胶囊] 结构，箭头盒恒 teal | **已改**（2026-09-24）：25×24 箭头盒 + 24 高/14px 字胶囊，间距 15、右缘 30；胶囊禁用色仍走 OWzx token（随 R1 定）；箭头盒下拉菜单为上游行为未移植，暂仅视觉 |
| R6 | 项目标题居中算法 | BBLTopbar.cpp CenteredTitle | `BBLTopbar.qml` projectTitleLabel | 于左簇末(210)→右缘居中 | `width/2-500` 近似 | 公式不同，窄窗口时偏差大 |
| R7 | 窗口控制键宽 | — | `BBLTopbar.qml` tb-win | ~50 | 30 | −20（热区偏小） |
| R8 | 侧栏宽度 | Sidebar.cpp | `PreparePage.qml:39` sidebarWidth | 392 | **已改**（2026-09-24）：`kSidebarDefaultWidth = 392` + 设置版本 bump 5（旧存档一次性迁移）；Prepare/Plater/Preview/Assemble 四页回退值与 `LeftSidebar.targetSidebarWidth`、`Theme.sidebarWidth*` 同步为 392；可调区间保持 300/520 |
| R9 | 区头横带 | Sidebar.cpp | `LeftSidebar.qml` | `#36363b` 高 30 | `bgPanel #161a23` | 色高均不同 |
| R10 | 耗材丝序号方块 | Sidebar.cpp | `LeftSidebar.qml` | 24×24 `#26a69a` | Theme.accent | 色/尺寸 |
| R11 | 参数行节奏 | ParamsPanel.cpp/Tab.cpp | `LeftSidebar.qml` 参数树 | 行 30/输入 25×119/行距 30 | 待逐项核（popup 宽 380） | 核对后收口 |
| R12 | 参数页签激活态 | ParamsPanel.cpp tabs | `LeftSidebar.qml` | 白字+2px `#009688` 下划线 | 待核 | 同上 |
| R13 | 滚动条 | — | CxScrollView | 槽 `#171717`/滑块 `#959595` | `#1c2230`/`#3a4258` | 过暗 |
| R14 | 禁用胶囊配色 | — | `BBLTopbar.qml` | `#8e8e83`/`#56564f` | `borderInput` 系 | 过暗 |

## 4. 还原前必须先定的一个决策（R1）

`inventory` 冻结口径是"上游截图 = 视觉真值"，但 `Theme.qml`（Phase 160 DS-01）
已把 OWzx 蓝黑+绿确立为 token 体系，且 §5 已登记品牌偏离（绿色 pill 等）。
两者冲突：**细节还原应把 token 值改成上游采样值（本文档 §2），还是保留 OWzx
token 只对齐几何/结构？** 建议：几何/结构/密度全量对齐上游（R2..R14 无争议），
色板走"上游中性灰为底 + OWzx 绿仅替换 Teal 900 语义位"的最小偏离方案，
并在 inventory §5 追加登记。定案后按表逐行执行。

## 5. 执行循环（沿用 inventory §7）

真机差分工具（2026-09-24 起）：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/ui_diff_loop.ps1 -Page prepare
# 定窗 1366x721 启动 → 抓屏 → scripts/ui_diff_compare.py 像素差分
# 产出 build/ui_diff/{owzx_prepare.png, diffmask.png, sbs.png, metrics.json}
# -DevQmlDir（默认开）走 main_qml.cpp 的 OWZX_DEV_QML_DIR 拦截器：
#   改 QML 只需重跑本脚本（应用重启级），无需 rcc+链接重编译
```

每区域：网页叠加定位差距 → 改 QML（只动呈现）→ `ui_diff_loop.ps1` 与
`upstream_*.png` 逐像素差（阈值 >40 占比向 ~3.5% 字体噪声地板收敛）→
QmlUiAuditTests 断言 → 主流程 gate 12 套件 0 failed → 双侧截图 + 对照报告。

已执行记录：
- R5/R8（2026-09-24）：分体按钮结构 + 侧栏 392 默认宽落地，含审计锁值同步
  （`kSidebarSettingsVersion` 4→5）。gate 12 套件 0 failed；真机抓屏复核：
  侧栏右缘落在 x=392（像素级命中），分体按钮渲染正确。首轮差分
  `overall=51.3%`（sidebar_band 12.6% / content_band 66.9%）——大头是 R1
  色板决策未落地的视口/全局底色差异，属预期。
- 已知坑两则（均已根治，处置见下）：(1) 改 C++ 头文件后 ninja 复用陈旧 obj
  （inventory §7 旧坑）——已由 Q13 根治（77fbcc2，autogen timestamp 强制重扫，
  见 .codex/rules/build-rules.md 处置记录），不再需要手动删 obj；
  (2) `--open-page` 深链曾触发 AI-SIDECAR-RACE 崩溃（`...27a2` AV 签名，
  2026-09-15 起存在），ui_diff_loop 因此一度改走 HKCU 注册表启动页偏好绕行
  ——根因已被补丁 0007（emboss font registry wstring overread）根治，
  ui_diff_loop 已于 2026-10-13（Q6）恢复 `--open-page` 深链直测并拆除注册表
  绕行，实测 prepare 深链启动过渲染稳定窗无复现。
