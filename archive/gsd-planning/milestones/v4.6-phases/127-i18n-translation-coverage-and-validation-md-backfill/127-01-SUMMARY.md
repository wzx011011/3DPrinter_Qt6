# Phase 127 Summary: i18n Translation Coverage And VALIDATION.md Backfill

**Phase:** 127 (WS4, I18N-01 + PROC-01)
**Status:** Complete
**Date:** 2026-07-15

## What Shipped
- i18n pipeline documented (.planning/i18n-workflow.md: lupdate→translate→lrelease).
- lupdate refreshed all 6 .ts files (captured v4.6 strings, removed obsolete).
- zh_CN: 10 v4.6 TickCode strings translated (finished) — proof-of-pipeline.
- ja/ko/de/fr: refreshed, documented baseline (unfinished, not false 100%).
- VALIDATION.md backfilled for phases 117-126 (Nyquist per-task maps).
- translationPipelineDocumented source-audit slot.

## Verification
Canonical build exit 0; all 5 ctest groups PASS; APP_RUNNING_PID=35572.
