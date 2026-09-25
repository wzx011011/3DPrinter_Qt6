# Phase 184: i18n Long Tail — de/fr to 85%

**Status:** Planned
**Workstream:** I18N
**Requirement:** I18N-07
**Dependencies:** none (Wave A parallel, can run alongside Phase 185)

## Goal

Push i18n translation coverage for German (de) and French (fr) from current ~69% to ≥85%.

## Baseline (measured 2026-07-20)

- `i18n/de.ts`: 1682 messages, 508 unfinished → **69% covered**
- `i18n/fr.ts`: 1682 messages, 508 unfinished → **69% covered**
- Target: ≥85% (≤252 unfinished per language → translate ~256 messages per language)

## Scope

### Step 1: Refresh .ts files

- Run `lupdate` to refresh `i18n/de.ts` and `i18n/fr.ts` against current source. This picks up new strings added since v5.3 Phase 177 (mostly v5.3 ship + v5.4 new code from Phases 180-183).
- Review the diff: how many new messages, how many obsoleted.

### Step 2: LLM-assisted translation passes

- Batch the unfinished messages (group by source file / context for consistency).
- LLM-assisted translation with curated glossary (carry forward v5.3 Phase 177 glossary for de/fr).
- Apply translations in batches, verify UTF-8 encoding and XML well-formedness after each batch.

### Step 3: Verify coverage

- Re-measure coverage. Target: ≥85% for both de and fr.
- If a particular batch has many technical/ambiguous strings that LLM handles poorly, leave them unfinished and document in SUMMARY.md (don't force bad translations).

## Out of Scope

- Translating the remaining ~15% to 100% — that needs human review for nuance.
- Changes to source strings (i18n is read-only on the source side).
- Adding new languages.

## Verification

- `i18n/de.ts` and `i18n/fr.ts` coverage ≥85% each.
- `lrelease` builds both .qm files without errors.
- OWzxSlicer runs with `LANG=de_DE` / `LANG=fr_FR` and shows translated UI (no encoding glitches).
- QmlUiAuditTests: add I18N-07 anchor in Phase 187 (verify a key de/fr string like "Drucken" / "Imprimer" exists).

## Risk Notes

- LLM translation quality varies by domain. Technical slicing terms (e.g. "infill density", "pressure advance") need consistent glossary entries to avoid term drift across the file.
- If lupdate surfaces a large number of new strings (post-v5.3 surfaces), the 85% target may need more batches than estimated.
