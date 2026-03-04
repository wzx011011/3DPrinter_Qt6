// Ultimate proxy setup - intercept setGlobalDispatcher
const mergedNoProxy = 'localhost,127.0.0.1,::1,172.17.0.1,host.docker.internal,172.26.0.1,ollama';
process.env.NO_PROXY = mergedNoProxy;
process.env.no_proxy = mergedNoProxy;
process.env.HTTP_PROXY = '';
process.env.http_proxy = '';
process.env.HTTPS_PROXY = '';
process.env.https_proxy = '';
const HTTP_PROXY = null;

if (HTTP_PROXY) {
  try {
    const undici = require('/app/node_modules/undici');
    const ProxyAgent = undici.ProxyAgent;

    const proxyAgent = new ProxyAgent({ uri: HTTP_PROXY });
    const originalSetGlobalDispatcher = undici.setGlobalDispatcher;
    let forceProxy = true;

    undici.setGlobalDispatcher = function(dispatcher) {
      if (forceProxy) {
        console.log('[proxy-setup] Intercepted setGlobalDispatcher, forcing proxy agent');
        return originalSetGlobalDispatcher.call(this, proxyAgent);
      }
      return originalSetGlobalDispatcher.call(this, dispatcher);
    };

    originalSetGlobalDispatcher.call(undici, proxyAgent);
    console.log('[proxy-setup] Global fetch proxy enabled:', HTTP_PROXY);

    setTimeout(() => {
      forceProxy = false;
      console.log('[proxy-setup] No longer forcing proxy, but keeping interception');
    }, 120000);

    const originalFetch = globalThis.fetch;
    globalThis.fetch = async function(input, init = {}) {
      const url = typeof input === 'string' ? input : input.url;
      if (url && (url.includes('githubcopilot.com') || url.includes('api.github.com/copilot'))) {
        init.headers = init.headers || {};
        const headers = new Headers(init.headers);
        const IDE_HEADERS = {
          'Editor-Version': 'vscode/1.85.0',
          'Editor-Plugin-Version': 'copilot/1.155.0',
          'User-Agent': 'GithubCopilot/1.155.0'
        };
        for (const [key, value] of Object.entries(IDE_HEADERS)) {
          if (!headers.has(key)) headers.set(key, value);
        }
        init.headers = headers;
      }
      return originalFetch.call(this, input, init);
    };
    console.log('[proxy-setup] IDE headers injection enabled');
  } catch (e) {
    console.error('[proxy-setup] Failed:', e.message);
  }
}
