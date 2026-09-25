# Summary 139-02: de/fr/ja/ko Baseline Advance (I18N-05)

**Phase:** 139 (WS3, I18N-05)
**Plan:** 139-02
**Status:** Complete
**Date:** 2026-07-16

## What Shipped
- Advanced de/fr/ja/ko from 0% baseline (1629 unfinished each) by translating 723 high-frequency core UI messages per language (564 unique sources → 723 message blocks).
- Re-runnable parameterized Python script (`scripts/translate_core_i18n.py`), using the proven regex pattern from `translate_en_ts.py`.
- Domain-accurate (Filament/Stützstruktur/Support/サポート/서포트, etc.).

## Verification
- Before: 1629 unfinished per language (0%). After: 906 unfinished per language (44.4% complete, 723 filled).
- XML valid for all 4 files. Canonical build exit 0; .qm files produced for de/fr/ja/ko (de.qm=59KB). 5/5 ctest PASS. E2E PASS. APP_RUNNING_PID=2780.

## Remaining-work estimate
~906 messages per language remaining (~55.6%). The untranslated remainder is the long tail: tooltips, process-option descriptions, one-off dialog copy. Requires extended CJK→target dictionary or per-language translator review.
