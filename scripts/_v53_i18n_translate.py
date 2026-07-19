#!/usr/bin/env python
# Phase 177/178 (I18N-06): apply glossary translations to .ts files.
# For each <translation type="unfinished"></translation>, look up the source
# string in the glossary. If a translation is found, fill it in + remove the
# type="unfinished" attribute (becomes a finished translation).

import sys, re, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from _v53_i18n_glossary import translate

def process_ts(ts_path, lang):
    with open(ts_path, encoding="utf-8") as f:
        src = f.read()
    # Match: <message>...<source>ZH</source>...<translation type="unfinished"></translation>...</message>
    # Use a non-greedy pattern; capture the source and the translation line.
    pattern = re.compile(
        r'(<source>([^<]+)</source>\s*<translation\s+type="unfinished">)(</translation>)',
        re.MULTILINE
    )
    translated_count = [0]
    skipped_count = [0]
    def repl(m):
        prefix = m.group(1)
        zh_source = m.group(2)
        suffix = m.group(3)
        tr = translate(zh_source, lang)
        if tr is None:
            skipped_count[0] += 1
            return m.group(0)
        # Replace: drop type="unfinished", fill translation
        new_prefix = prefix.replace('<translation\s+type="unfinished">', '<translation>')
        # The prefix contains '<source>...</source>\n        <translation type="unfinished">'
        # We need to rewrite to '<source>...</source>\n        <translation>'
        # Just reconstruct cleanly:
        new_prefix = re.sub(r'<translation\s+type="unfinished">$',
                            '<translation>', prefix)
        translated_count[0] += 1
        return new_prefix + tr + suffix
    new_src = pattern.sub(repl, src)
    # Cleanup: <translation>X</translation> (no type attr) is now finished
    if translated_count[0] > 0:
        with open(ts_path, "w", encoding="utf-8") as f:
            f.write(new_src)
    return translated_count[0], skipped_count[0]

if __name__ == "__main__":
    lang = sys.argv[1] if len(sys.argv) > 1 else "de"
    targets = sys.argv[2:] or [f"i18n/{lang}.ts"]
    for t in targets:
        if not os.path.exists(t):
            print(f"  SKIP {t} (not found)")
            continue
        tr, sk = process_ts(t, lang)
        print(f"  {t}: {tr} translated, {sk} no-match (kept unfinished)")
