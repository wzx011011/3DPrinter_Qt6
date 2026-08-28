# AI 助手（AI Control）— OWzx-only 决策与实现说明

> 决策性质：**OWzx-only 功能**。上游 OrcaSlicer 没有等价行为，本文即
> `.codex/rules/source-truth-migration.md` 要求的"显式记录为 OWzx-only 决策"。
> 用户决策（2026-08-28）：内嵌完整 harness（Claude Agent SDK sidecar）+
> GLM 5.3-flash（智谱 Anthropic 兼容端点），聊天侧栏为主界面，软件中
> 可见与不可见的功能全部可被 AI 控制。

## 架构

```
ChatSidebar.qml（右侧聊天侧栏，main.qml 挂载）
    ↓ 绑定
AiViewModel（src/core/viewmodels/）— 消息列表/工具卡片/确认卡
    ↓ Qt signals
AiAgentService（src/core/ai/）— QProcess 托管 sidecar，NDJSON over stdio
    ↓
agent.py（tools/ai_sidecar/，打包到 build/ai_sidecar/）
    — Claude Agent SDK 0.2.x（内嵌 claude.exe）为 harness：
      agent 循环、上下文管理、工具调用、权限回调
    ↓ Anthropic 兼容端点
GLM（https://open.bigmodel.cn/api/anthropic，模型 glm-5.3-flash）
    ↓ tool_use（SDK 原生 MCP 客户端，HTTP）
McpHttpServer（src/core/ai/）— 127.0.0.1，MCP Streamable HTTP 子集
    ↓ tools/list / tools/call
AppToolRegistry（src/core/ai/）— 25 个 JSON-Schema 工具
    ↓ 全部为现有 API 薄包装（GUI 线程执行）
EditorViewModel / ConfigViewModel / SliceService / ProjectServiceMock
```

- 破坏性工具确认：SDK `can_use_tool` → sidecar `permission_request` NDJSON
  事件 → AiViewModel 确认卡（允许/拒绝）→ `answer_permission` 命令回传。
- 上下文获取是 harness 惯用法：模型自己按需调用 `get_app_state` /
  `list_config_keys` 等查询工具，不做手工 system-prompt 注入。
- 工具定义使用 JSON Schema（与 MCP 同源）。未来若需接入外部 harness
  （Claude Code / Cursor / 任意 MCP 客户端），同一 McpHttpServer 直接可用，
  工具层零改动。

## 工具面（AppToolRegistry，v1 = 25 个）

- 查询：`get_app_state` `get_scene` `get_plate_info` `get_slice_status`
  `get_object_info` `list_config_keys`（含当前值/类型/枚举/范围）
- 场景：`load_model` `delete_object`(⚠) `duplicate_object`
  `set_object_transform` `arrange_objects` `orient_objects`
- 配置：`set_config_value`（走 ConfigViewModel::setValue 同一管线）
  `select_preset`
- 切片（异步，轮询 `get_slice_status`）：`slice_plate` `slice_all_plates`
  `cancel_slice` `export_gcode`
- UI 可见控制：`switch_page` `select_object` `toggle_sidebar`
- 全局：`undo` `redo` `save_project` `clear_project`(⚠)

(⚠) = destructive，经 can_use_tool 走用户确认卡。

## 安全

- MCP 服务器仅绑定 127.0.0.1，Bearer Token 鉴权（QSettings
  `aiControl/token` 随机 UUID，独立分组，重置偏好不清除），Host/Origin
  校验防 DNS rebinding，4MiB 请求上限。
- 默认**关闭**：偏好设置 → AI 助手 → 启用开关 + API Key 才会拉起
  MCP 服务器与 sidecar。
- API Key 仅存本地 QSettings，通过环境变量注入 sidecar，日志不落 key。
- sidecar 不读取机器上的 Claude 用户/项目配置（`setting_sources=[]`）。
- 单轮工具循环上限 24 turns（harness 保险丝）。

## 打包（可选组件，约 350MB）

`cmake --build build --target ai_sidecar`（或直接运行
`scripts/package_ai_sidecar.ps1`）产出 `build/ai_sidecar/`：
嵌入式 CPython 3.12 + claude-agent-sdk（内含 ~230MB claude.exe）+
agent.py。首次运行需网络（python.org + PyPI），之后离线缓存。
`agent.py --selftest` 无网络自检（SDK 导入 + 协议回环），可进 CI。

随主程序分发时把 `ai_sidecar/` 目录放到 `OWzxSlicer.exe` 同级目录。

## 启用与验收

1. 安装组件（上节）；
2. 偏好设置 → AI 助手：填 API Key、打开启用开关（模型/端点/端口有默认值）；
3. Prepare 页右缘出现 AI 窄条 → 打开聊天侧栏；
4. 手动验收（需真实 key）：
   - 排障：制造一个切片失败，让 AI 读取状态并解释；
   - 自然语言调参："把壁数调成 4，填充 20%" → 工具卡展示
     `set_config_value` 新旧值，侧栏设置面板同步变化；
   - 破坏性确认："清空工作区" → 确认卡出现，拒绝后 AI 停止。

## 真机验收记录（2026-08-28，GLM Coding Plan key，glm-5.3-flash）

三场景全部通过（应用内嵌 sidecar + 桌面 UI 实测）：

1. 读状态：侧栏输入"当前应用在哪个页面？有几个模型？" →
   `get_app_state`/`get_scene` 工具卡（绿勾）→ 回复"首页，0 个模型"；
2. 自然语言调参："把壁厚循环数(wall_loops)设置为4" →
   `list_config_keys`(filter) → `set_config_value`（旧值 2 → 新值 4）→
   再读确认，回复带新旧值对照表；
3. 破坏性确认：`clear_project` → 确认卡（允许/拒绝）→ 允许后执行。

期间发现并修复的三个问题（均已加回归锁定）：

- **SDK 消息解析**：claude-agent-sdk 0.2.147 的消息是 dataclass 实例而非
  dict，`run_turn` 按 dict 取值在首个真实回合崩溃；解析提取为
  `_emit_message_events` 并入 `--selftest`。
- **智谱端点 schema 严格性**：`tools` 中出现联合类型
  `"type": "string|number|boolean"` 会被端点以错误码 1210 拒绝
  （Anthropic 原生端点则容忍）。全部工具 schema 收敛为单一具体类型，
  `AppToolTests` 递归校验属性类型合法性。
- **确认卡粒度**：最初所有工具调用都弹确认卡；按既定决策改为
  MCP annotations（`readOnlyHint`/`destructiveHint`）标注，
  sidecar 启动时拉取 tools/list，只读工具自动放行，
  仅 destructive 工具（delete_object/clear_project）走确认卡。

## 全量 UI 验收补充（2026-08-28 晚，重装 sidecar 包后）

重建 `build/ai_sidecar` 可选包后，经真实 ChatSidebar 逐项复核：
load_model / duplicate_object / orient_objects / set_object_transform /
select_object / switch_page(preview→currentPage=2) / arrange_objects /
select_preset / slice_plate / slice_all_plates / export_gcode /
save_project / undo / redo / get_app_state / get_scene 全部经 GLM 真实
回合执行成功；产物 `build/ai_ui_export.gcode`（约 480KB）与
`build/ai_ui_project.3mf` 非空。

期间发现并修复第四个问题：

- **`allowed_tools` 预授权绕过确认卡（安全回归）**：为修正工具路由而加入的
  `allowed_tools=["mcp__owzx__*"]` 在 SDK 语义中是"免提示预授权"，
  会使 `can_use_tool` 完全不被调用——实测 `delete_object` 在无任何
  确认卡交互的情况下直接执行（objectCount 1→0）。修复：`allowed_tools`
  恒为空，门控完全收敛到 `can_use_tool`（annotations 自动放行只读工具，
  destructive 工具等待确认卡）。修复后实测：delete_object 发起后
  objectCount 保持 1、聊天输入框禁用、确认卡（允许/拒绝）出现，
  点击"允许"后 objectCount 才变为 0。

`cancel_slice` 说明：测试模型切片约 2 秒完成，快于 GLM 回合延迟，
聊天内"切片中取消"竞态在真实 UI 下无法稳定复现；工具空闲时返回
`{"ok":true}`（幂等无操作），真实取消语义由 E2E
`test_cancelled_slice_clears_active_result_and_blocks_preview_export`
锁定（同一 SliceService::cancelSlice 路径）。

## 相关文件

| 层 | 文件 |
|---|---|
| 工具注册表 | `src/core/ai/AppToolRegistry.h/.cpp` |
| MCP 服务器 | `src/core/ai/McpHttpServer.h/.cpp` |
| sidecar 宿主 | `src/core/ai/AiAgentService.h/.cpp` |
| 聊天状态 | `src/core/viewmodels/AiViewModel.h/.cpp` |
| 组合根接线 | `src/qml_gui/BackendContext.h/.cpp`（实现 AppToolUiProvider） |
| 设置 | `src/core/viewmodels/SettingsViewModel.h/.cpp`（aiEnabled/aiApiKey/aiModel/aiBaseUrl/aiPort/aiControlToken） |
| UI | `src/qml_gui/panels/ChatSidebar.qml`、`main.qml`（右缘窄条+面板）、`PreferencesPage.qml`（AI 助手分类） |
| harness | `tools/ai_sidecar/agent.py`、`requirements.txt` |
| 打包 | `scripts/package_ai_sidecar.ps1`、`CMakeLists.txt`（OWZX_AI_SIDECAR 选项） |
| 回归 | `tests/AppToolTests.cpp`（10 用例：工具定义/状态/配置读写/场景操作/MCP 握手/鉴权/路由） |

## 已知边界

- 流式输出按 SDK 消息粒度（非逐字）；逐字流式留后续。
- `switch_page` 仅映射 home/prepare/preview/monitor/preferences 与 0..8 整数。
- sidecar 崩溃自动重启上限 3 次，之后报错停机。
- 外部 harness 直连（Claude Code 等）：服务器已具备，仅缺文档章节——
  属后续增量。
