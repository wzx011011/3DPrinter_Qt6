import json
import time
import urllib.request

URL = 'http://127.0.0.1:11434/api/generate'
MODEL = 'qwen3.5:9b'
PROMPT = 'Introduce yourself in one short sentence.'


def run_once(tag: str):
    payload = {
        'model': MODEL,
        'prompt': PROMPT,
        'stream': True,
        'options': {
            'temperature': 0,
            'num_predict': 80
        }
    }
    req = urllib.request.Request(
        URL,
        data=json.dumps(payload, ensure_ascii=False).encode('utf-8'),
        headers={'Content-Type': 'application/json'}
    )

    start = time.perf_counter()
    first_token_at = None
    done_obj = None

    with urllib.request.urlopen(req, timeout=300) as resp:
        for raw in resp:
            if not raw:
                continue
            line = raw.decode('utf-8', errors='ignore').strip()
            if not line:
                continue
            obj = json.loads(line)

            chunk = obj.get('response') or ''
            if first_token_at is None and chunk:
                first_token_at = time.perf_counter()

            if obj.get('done'):
                done_obj = obj
                break

    end = time.perf_counter()
    if done_obj is None:
        raise RuntimeError('No done object from Ollama stream')

    ttft_ms = None if first_token_at is None else (first_token_at - start) * 1000.0
    eval_count = int(done_obj.get('eval_count') or 0)
    eval_duration_ns = int(done_obj.get('eval_duration') or 0)
    tok_s = (eval_count / (eval_duration_ns / 1e9)) if eval_duration_ns > 0 else 0.0

    return {
        'tag': tag,
        'ttft_ms': ttft_ms,
        'tok_s': tok_s,
        'eval_count': eval_count,
        'total_s': end - start,
        'prompt_eval_ms': int(done_obj.get('prompt_eval_duration') or 0) / 1e6,
        'load_ms': int(done_obj.get('load_duration') or 0) / 1e6,
    }


if __name__ == '__main__':
    warmup = run_once('warmup')
    runs = [run_once(f'run{i}') for i in range(1, 4)]
    avg_ttft = sum(r['ttft_ms'] for r in runs if r['ttft_ms'] is not None) / len(runs)
    avg_tok_s = sum(r['tok_s'] for r in runs) / len(runs)

    out = {
        'model': MODEL,
        'warmup': warmup,
        'runs': runs,
        'avg_ttft_ms': avg_ttft,
        'avg_tok_s': avg_tok_s,
    }
    print(json.dumps(out, ensure_ascii=False))
