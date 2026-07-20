#!/usr/bin/env python3
"""
Translate zh_CN.ts unfinished entries using a curated dictionary.

This script operates on the .ts file as TEXT (not XML), to preserve the exact
formatting of all other entries. Only <translation type="unfinished">...</translation>
tags are modified:
  - type="unfinished" attribute removed (so the entry counts as "covered")
  - inner text replaced with the translation

For zh_CN specifically, most unfinished sources are already Chinese text (qsTr
wrote Chinese), so translation = source verbatim. A small number of English
sources need actual translation.
"""
import re
import sys
import os

# 翻译字典(英文/混合源 → 中文)
# 大部分中文源不需要列(脚本会自动复制源)
TRANSLATIONS_ZH = {
    # 纯英文源 → 中文
    '%1 difference(s)': '%1 个差异',
    'Filament Group': '耗材分组',
    'Format': '格式',
    'Hollow': '镂空',
    'Modified': '已修改',
    'No project': '无项目',
    'Path': '路径',
    'Size': '大小',
    # 这些是单字母/短标签,翻译就是原样(避免误改)
    # 'A', 'B', 'A: ', 'B: ', 'A：', 'B：', 'mm', '%1 – %2 mm' — keep as-is via default rule
}


def process_ts(ts_path):
    """Process .ts as text, replace only unfinished translation tags."""
    with open(ts_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Strategy: process line by line. When we see <source>, remember the text.
    # When we see <translation type="unfinished"> on a later line, replace it
    # using the remembered source text.

    stats = {'translated': 0, 'copied': 0, 'untouched': 0}

    lines = content.split('\n')
    out_lines = []
    current_source = None  # most recent <source> text (unescaped)

    source_re = re.compile(r'<source>(.*?)</source>')
    unfinished_re = re.compile(r'(\s*)<translation type="unfinished">(.*?)</translation>')

    for line in lines:
        # Track source
        sm = source_re.search(line)
        if sm:
            src_raw = sm.group(1)
            # Unescape XML entities to get the actual source string
            current_source = (src_raw.replace('&amp;', '&')
                                       .replace('&lt;', '<')
                                       .replace('&gt;', '>')
                                       .replace('&quot;', '"')
                                       .replace('&apos;', "'"))

        # Replace unfinished translation
        um = unfinished_re.search(line)
        if um and current_source is not None:
            indent = um.group(1)
            old_text = um.group(2)

            # Decide translation text (unescaped form)
            if current_source in TRANSLATIONS_ZH:
                translation_unesc = TRANSLATIONS_ZH[current_source]
                stats['translated'] += 1
            else:
                # Default: copy source verbatim (zh_CN convention for Chinese sources,
                # and acceptable fallback for English proper nouns / format strings)
                translation_unesc = current_source
                stats['copied'] += 1

            # Re-escape XML special chars
            translation = (translation_unesc.replace('&', '&amp;')
                                             .replace('<', '&lt;')
                                             .replace('>', '&gt;'))

            # Build new line: replace only the translation tag, keep indentation
            new_line = f'{indent}<translation>{translation}</translation>'
            # Preserve any trailing content on the line (shouldn't be any, but just in case)
            trailing = line[um.end():]
            out_lines.append(new_line + trailing)
            continue

        out_lines.append(line)

    new_content = '\n'.join(out_lines)
    if new_content != content:
        with open(ts_path, 'w', encoding='utf-8') as f:
            f.write(new_content)

    return stats


def count_unfinished(ts_path):
    with open(ts_path, 'r', encoding='utf-8') as f:
        content = f.read()
    total = content.count('<translation')
    unfinished = content.count('type="unfinished"')
    return total, unfinished


def main():
    os.chdir(r'E:\ai\3DPrinter_Qt6')
    ts_path = 'i18n/zh_CN.ts'

    total_before, unfinished_before = count_unfinished(ts_path)
    cov_before = (total_before - unfinished_before) * 100 // total_before if total_before else 0
    print(f'Before: {unfinished_before}/{total_before} unfinished ({cov_before}% covered)')

    stats = process_ts(ts_path)
    print(f'Translated: {stats["translated"]}')
    print(f'Copied source as translation: {stats["copied"]}')
    print(f'Untouched: {stats["untouched"]}')

    total_after, unfinished_after = count_unfinished(ts_path)
    cov_after = (total_after - unfinished_after) * 100 // total_after if total_after else 0
    print(f'After: {unfinished_after}/{total_after} unfinished ({cov_after}% covered)')
    print(f'Improvement: +{cov_after - cov_before}%')


if __name__ == '__main__':
    main()
