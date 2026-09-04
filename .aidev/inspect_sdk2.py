import dataclasses
from claude_agent_sdk.types import McpHttpServerConfig, McpStdioServerConfig
print('McpHttpServerConfig:', dataclasses.fields(McpHttpServerConfig) if dataclasses.is_dataclass(McpHttpServerConfig) else McpHttpServerConfig.model_fields)
print('McpStdioServerConfig:', dataclasses.fields(McpStdioServerConfig) if dataclasses.is_dataclass(McpStdioServerConfig) else McpStdioServerConfig.model_fields)
try:
    c = McpHttpServerConfig(type='http', url='http://127.0.0.1:1/mcp', headers={'Authorization': 'Bearer x'})
    print('construct ok:', c)
except Exception as e:
    print('ctor err:', e)
