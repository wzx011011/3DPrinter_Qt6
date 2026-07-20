# Phase 185: i18n Long Tail — ja/ko to 85%

**Status:** Planned
**Workstream:** I18N
**Requirement:** I18N-07
**Dependencies:** none (Wave A parallel, can run alongside Phase 184)

## Goal

Push i18n translation coverage for Japanese (ja) and Korean (ko) from current ~68-70% to ≥85%.

## Baseline (measured 2026-07-20)

- `i18n/ja.ts`: 1682 messages, 529 unfinished → **68% covered**
- `i18n/ko.ts`: 1682 messages, 502 unfinished → **70% covered**
- Target: ≥85% (ja: translate ~276 messages; ko: translate ~250 messages)

## Scope

### Step 1: Refresh .ts files

- Run `lupdate` to refresh `i18n/ja.ts` and `i18n/ko.ts` against current source. Picks up new strings since v5.3 Phase 178.

### Step 2: LLM-assisted translation passes

- Batch unfinished messages by source file / context.
- LLM-assisted translation with curated glossary (carry forward v5.3 Phase 178 ja/ko glossary).
- CJK-specific care: ensure proper half-width/full-width character usage, no mojibake, consistent technical term handling (Katakana for tech loanwords in ja; standard Sino-Korean for tech in ko).

### Step 3: Verify coverage

- Re-measure coverage. Target: ≥85% for both ja and ko.

## Out of Scope

- 100% coverage — needs human native-speaker review.
- Source string changes.
- New languages.

## Verification

- `i18n/ja.ts` and `i18n/ko.ts` coverage ≥85% each.
- `lrelease` builds both .qm files without errors.
- OWzxSlicer runs with `LANG=ja_JP` / `LANG=ko_KR` and shows translated UI (no encoding glitches, no mojibake).
- QmlUiAuditTests: extend I18N-07 anchor in Phase 187 to cover ja/ko.

## Risk Notes

- CJK LLM translation quality is generally strong, but katakana-vs-native-japanese choices for new tech terms (e.g. "pressure advance") may be inconsistent across the file — needs a glossary lock.
- Japanese technical writing has multiple politeness levels — pick one (typically です/ます for UI) and stick to it.
