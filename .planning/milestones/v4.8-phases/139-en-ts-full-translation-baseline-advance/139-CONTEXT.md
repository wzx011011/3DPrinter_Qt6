# Phase 139: en.ts Full Translation + Baseline Advance - Context

**Gathered:** 2026-07-16
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss) + enriched by i18n survey.

<domain>
## Phase Boundary

Fill the remaining ~1372 unfinished en.ts translations (I18N-04) so lrelease produces a complete en.qm, and advance de/fr/ja/ko meaningfully from the 0% baseline (I18N-05). This is primarily a CONTENT GENERATION task (Chinese→English translation of UI strings), not a code task.

**Survey findings (2026-07-16):**
- en.ts: 1629 messages total, **1372 unfinished** (target: 0).
- The 1372 unfinished occurrences dedupe to **1170 unique source strings**:
  - **243 non-CJK** (English source, e.g. "About", "Add", "Save", "3MF (*.3mf)", "%1 selected") — translation = source (mechanical, scripted).
  - **927 CJK** (Chinese source) — need actual Chinese→English translation. Many are standard slicer-UI terms (耗材/Filament, 打印/Print, 支撑/Support, 速度/Speed, 平台/Plate, etc.).
- zh_CN.ts is the reference (31 unfinished, ~98% complete).
- de/fr/ja/ko: 1629 messages, ALL unfinished (0% translated baseline).
- Source strings are written in Chinese (`qsTr("AMS 设置")`); en.ts `<translation>` provides the English.
- Phase 131 (v4.7) dictionary-batch-translated 121 high-frequency terms — the dictionary approach is proven; this phase extends it to the full set.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — discuss skipped. Use the survey findings and the proven Phase 131 dictionary approach.

### Confirmed approach (from survey)
- **243 non-CJK sources:** translation = source string (mechanical; a script can do this safely — these are already-English qsTr sources that lupdate marked unfinished).
- **927 CJK sources:** build a comprehensive Chinese→English dictionary covering standard 3D-printing/slicer UI terminology, then translate each source string. For compound sentences, decompose by dictionary terms and compose the English. For any residual uncovered strings, provide a reasonable translation based on context (the `<location filename=...>` attribute tells you which QML file/component the string belongs to).
- **Mark finished:** replace `<translation type="unfinished"></translation>` with `<translation>Filled English</translation>` (drop the `type="unfinished"` attribute to mark finished, matching how zh_CN.ts and the existing finished en.ts entries look).
- **Preserve XML structure:** do NOT reformat en.ts; edit `<translation>` elements in place. Preserve `<location>`, `<source>`, message ordering, and the `<context>` grouping.
- **de/fr/ja/ko advance:** translate a meaningful subset (e.g. the high-frequency core UI terms — the same set Phase 131 dictionary covered, ~120-200 terms) into each language, OR document a concrete remaining-work estimate with the coverage %. I18N-05 says "advance meaningfully + documented remaining-work estimate" — either real translations or an honest documented estimate satisfies it, but real translations are preferred.

### Tooling
- Use Python to parse the .ts XML (lxml or xml.etree), find unfinished messages, fill translations, write back. Python is available in the environment.
- Do NOT run lupdate (it would re-mark strings based on source and could undo manual fills); edit the .ts files directly. lrelease (via the CMake build) compiles .ts → .qm.

### Quality bar
- Translations must be real English (not machine-gibberish). Slicer-domain accuracy matters: 耗材=Filament (not "consumable"), 支撑=Support, 平台=Plate, 切片=Slice, 挤出机=Extruder, 热床=Hot bed/Print bed, etc.
- Placeholder tokens (`%1`, `%2`, `&amp;`, `\n`, HTML entities) MUST be preserved verbatim in translations.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `i18n/en.ts` — 1629 messages, 1372 unfinished. Source = Chinese; `<translation>` = English target.
- `i18n/zh_CN.ts` — reference (98% complete); shows the finished-translation format.
- `i18n/de.ts`, `fr.ts`, `ja.ts`, `ko.ts` — 0% baseline.
- `.planning/i18n-workflow.md` — documents the lupdate→translate→lrelease pipeline (lrelease is wired into CMake via `qt_add_translations`).
- Phase 131 (v4.7) dictionary approach (archived in `.planning/milestones/v4.7-phases/131-english-i18n-translation-fill-baseline-advance/`).

### Established Patterns
- Finished entries: `<translation>Filled English</translation>` (no `type` attribute).
- Unfinished entries: `<translation type="unfinished"></translation>` (empty) or `<translation type="unfinished">partial</translation>`.

### Integration Points
- lrelease runs in the canonical build → produces `build/*.qm`. The QML audit (QmlUiAuditTests) validates the .ts files parse.
- No source-code changes expected in this phase (pure i18n content).

</code_context>

<specifics>
## Specific Ideas

- Decompose into 2 plans: (01) fill all 1372 en.ts unfinished → 0 unfinished (I18N-04); (02) advance de/fr/ja/ko with a meaningful translated subset + documented estimate (I18N-05).
- A Python script that parses the .ts XML, looks up each unfinished source in a dictionary, and writes the translation is the lowest-risk, most-auditable approach (the script + dictionary can be committed and re-run).
- Verify with `grep -c 'type="unfinished"' i18n/en.ts` → must be 0 (or near-0 for any genuinely-ambiguous residuals, documented).

</specifics>

<deferred>
## Deferred Ideas

- Professional/localized review of the English translations (native speaker QA) — future polish.
- Full de/fr/ja/ko translation of all 1629 strings (this phase advances meaningfully; full coverage is future work).
- Migrating source strings from Chinese to English as the qsTr() source (architectural change, out of scope — the .ts translation layer is the current contract).

</deferred>
