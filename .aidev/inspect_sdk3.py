from claude_agent_sdk.types import McpHttpServerConfig
import typing
print('annotations:', getattr(McpHttpServerConfig, '__annotations__', None))
print('total=False TypedDict?', McpHttpServerConfig.__total__ if hasattr(McpHttpServerConfig, '__total__') else 'n/a')
c = {"type": "http", "url": "http://127.0.0.1:1/mcp", "headers": {"Authorization": "Bearer x"}}
print('dict-form accepted as config (TypedDict)')
import claude_agent_sdk, os, pathlib
pkg = pathlib.Path(claude_agent_sdk.__file__).parent
for p in sorted(pkg.rglob('*')):
    if p.is_file() and (p.suffix in ('.exe', '.js', '.json') or 'cli' in p.name.lower()) and p.stat().st_size > 1_000_000:
        print('BIG:', p.relative_to(pkg), round(p.stat().st_size / 1e6, 1), 'MB')
cli = [p for p in pkg.rglob('*') if 'cli' in p.name.lower()]
print('cli files:', [str(p.relative_to(pkg)) for p in cli][:10])
