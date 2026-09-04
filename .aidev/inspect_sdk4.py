from claude_agent_sdk.types import AssistantMessage, UserMessage, SystemMessage, StreamEvent
import claude_agent_sdk
for c in (AssistantMessage, UserMessage, SystemMessage):
    print(c.__name__, '::', getattr(c, '__annotations__', None))
print('version attr:', getattr(claude_agent_sdk, '__version__', None))
import importlib.metadata
print('dist version:', importlib.metadata.version('claude-agent-sdk'))
from claude_agent_sdk import ClaudeSDKClient
import inspect
print('query:', inspect.signature(ClaudeSDKClient.query))
print('interrupt:', inspect.signature(ClaudeSDKClient.interrupt))
print('receive_response:', inspect.signature(ClaudeSDKClient.receive_response))
