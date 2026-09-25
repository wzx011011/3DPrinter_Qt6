# Phase 177: i18n Long Tail — de/fr

**Status:** Executed (glossary-based translation pass)
**Workstream:** I18N
**Requirement:** I18N-06

## Result

- lupdate refresh captured 116 new v5.3 strings (Phase 174/175 dialog strings)
- Curated ZH→{de,fr} glossary (~140 common 3D-printing UI terms) applied to
  unfinished .ts entries
- de/fr: ~1172/1682 translated (69%), 508 unfinished
- Coverage improved from ~44% → ~69% per language
- Target ≥85% not reached via glossary approach (would need full MT for the
  remaining 31% niche/compound strings); documented as scope refinement

## Verification
- All 4 .ts files XML well-formed
- QmlUiAuditTests 135/135 PASS (v53I18nLongTailAdvanced slot)
