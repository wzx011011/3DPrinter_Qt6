import dataclasses
import inspect
import claude_agent_sdk as s
from claude_agent_sdk import ClaudeAgentOptions, ClaudeSDKClient
import claude_agent_sdk.types as t

def show(cls):
    if dataclasses.is_dataclass(cls):
        return {f.name: str(f.type) for f in dataclasses.fields(cls)}
    if hasattr(cls, 'model_fields'):
        return {k: str(v.annotation) for k, v in cls.model_fields.items()}
    return str(cls)

print('== ClaudeAgentOptions fields ==')
for k, v in show(ClaudeAgentOptions).items():
    print(' ', k, '::', v)
print()
print('== ClaudeSDKClient methods ==')
print(sorted(m for m in dir(ClaudeSDKClient) if not m.startswith('_')))
print()
print('== ClaudeSDKClient.__init__ ==')
print(inspect.signature(ClaudeSDKClient.__init__))
print()
print('== query signature ==')
print(inspect.signature(s.query))
print()
print('== McpServerConfig ==', show(t.McpServerConfig))
for n in dir(t):
    if 'Http' in n or 'Stdio' in n:
        try:
            print(n, '::', show(getattr(t, n)))
        except Exception as e:
            print(n, 'err', e)
print()
print('== CanUseTool ==', inspect.signature(s.CanUseTool))
print('PermissionResultAllow ::', show(t.PermissionResultAllow))
print('PermissionResultDeny ::', show(t.PermissionResultDeny))
print('PermissionUpdate ::', show(t.PermissionUpdate))
print()
print('== ResultMessage ==', show(t.ResultMessage))
print('TextBlock ==', show(t.TextBlock))
print('ToolUseBlock ==', show(t.ToolUseBlock))
print('ToolResultBlock ==', show(t.ToolResultBlock))
print('Message ==', show(t.Message))
