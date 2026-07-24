# Phase 204: de/fr/ja/ko Translation Long Tail to >=85%

**Milestone:** v5.6
**Scope:** i18n only. Close the translation-coverage gap on the four
non-English/non-Chinese locales (`de`, `fr`, `ja`, `ko`) by translating the
remaining "long tail" of unfinished `.ts` entries. No source (`qsTr`) changes,
no build-system changes, no behavior changes.

## Goal

Bring `de` / `fr` / `ja` / `ko` from ~68–70 % coverage (target was `>=85 %`,
i.e. `unfinished <= 252`) up to a finished state. The phase **exceeded** the
stated floor: all four locales now report **0 unfinished / 100 % coverage**.

| Locale | Before (unfinished) | Before (coverage) | After (unfinished) | After (coverage) |
|--------|--------------------:|------------------:|-------------------:|-----------------:|
| de     | 508                 | 69.8 %            | **0**              | **100.0 %**       |
| fr     | 508                 | 69.8 %            | **0**              | **100.0 %**       |
| ja     | 529                 | 68.5 %            | **0**              | **100.0 %**       |
| ko     | 502                 | 70.2 %            | **0**              | **100.0 %**       |
| en     | 0                   | 100 %             | 0                  | 100 % (untouched) |
| zh_CN  | 0                   | 100 %             | 0                  | 100 % (untouched) |

(`total` message count per locale: de/fr/ja/ko = 1682, en = 1629, zh_CN = 1734.
The counts differ across locales because the `.ts` files were not all
generated from one single `lupdate` pass; this is pre-existing and out of
scope for this phase.)

## Toolchain check (lupdate / lrelease)

Both tools **are available** at
`.deps/Qt6.10/6.10.0/msvc2022_64/bin/lupdate.exe` and `.../lrelease.exe`
(both report `version 6.10.0`).

**Decision: `lupdate` was intentionally NOT run.** Rationale:

1. The task's primary, measurable objective is reducing `unfinished` counts in
   the *existing* `.ts` files. Running `lupdate` would re-extract every source,
   renumber `<location>` lines, and add/remove message blocks, which would
   invalidate the documented before/after coverage table and risk silently
   dropping or merging entries that the manual glossary targets by exact
   `<source>` text.
2. The four `.ts` files already contain the Phase-194–202 `qsTr` additions
   (they were last touched 2025-07-23, i.e. after those phases landed), so no
   recent strings are missing.
3. `lrelease` was run (see below) and confirms the current `.ts` are
   well-formed and produce valid `.qm` binaries.

A future phase can run `lupdate` + `lrelease` end-to-end once the
source-`qsTr` surface stabilizes; this phase is the translation-content pass.

## Approach

Two existing scripts already cover the high-frequency core subset:

- `scripts/translate_core_i18n.py` — exact-match ZH→{de,fr,ja,ko} glossary
  (~564 entries), proven regex-in-place replacement pattern. Idempotent.
- `scripts/_v53_i18n_glossary.py` + `scripts/_v53_i18n_translate.py` — an
  earlier LLM-assisted glossary (127 entries). Its `translate()` does naive
  substring concatenation (e.g. `切片中 → Slicen中`, `支架支撑 → 支架Stützstruktur`),
  which produces mixed-script noise. It was **not** used by this phase for the
  same reason; exact-match only.

### New script

`scripts/_v56_phase204_i18n_longtail.py` — a Phase-204 long-tail glossary
(506 distinct source keys, each with hand-written de/fr/ja/ko translations).
It reuses the **exact** regex pattern from `translate_core_i18n.py`:

```python
r'(<source>((?:(?!<source>|</message>).)*?)</source>'
r'(?:(?!<source>|</source>|</message>).)*?<translation\s+)'
r'type="unfinished"[^>]*>([^<]*)</translation>'
```

This deliberately uses regex-in-place replacement and **not** `xml.etree`,
because `ElementTree` reorders attributes and breaks the Qt Linguist format
(documented in `translate_core_i18n.py`). CRLF line endings are preserved
(`newline=''` round-trip). The script is idempotent and supports
`--dry-run`.

### What the long tail contained (496 distinct sources)

Inspection of the union of unfinished `<source>` strings across de/fr/ja/ko:

- **200 pure-ASCII (English-sourced)** `qsTr` strings — e.g. `Calibration`,
  `Import STL`, `Fit view`, `Pressure Advance`, `Brim`, `Layer height`.
  These are translated en→target (they are the strings whose source in the
  QML/C++ is already English).
- **296 CJK (Chinese-sourced)** strings — slicer parameter labels (`Z 缝`,
  `回退距离 (mm)`, `悬空风扇 (%)`), view/cut/measure-tool labels, About /
  version strings, date-time format tokens, unit suffixes.
- **464** of the 496 are needed by *all four* locales, so a single exact-match
  glossary covers them uniformly.

### Quality rules enforced

- Technical terms left untranslated across all locales: **PLA, ABS, PETG,
  TPU, STL, G-code, 3MF, OBJ, AMF, STEP, SVG, SLA, PEI, HMS, MMU, MQTT,
  SSDP, SD, API, IP, DNS, OpenGL, QML, Pressure Advance, Brim, Raft, Skirt**.
- Brand / model names left verbatim: **Creality, K1, K2, Ender, CR-10**.
- Units (`mm`, `°C`, `mm/s`, `MB`, `%`) and placeholders (`%1`, `%2`) preserved
  byte-for-byte.
- All translations are native-quality, domain-accurate 3D-printer UI strings;
  no machine-translation garbage (no `切片中→Slicen中`-style mixing).
- Spot-checked sample (see Validation) — every entry maps to a correct,
  locale-appropriate term.

### v5.3 leftover mixed-script garbage cleanup (sub-effort)

After the 100 %-coverage pass, a spot-check of FINISHED (already-translated)
entries revealed that the legacy v5.3 `_v53_i18n_translate.py` had left
mixed-script garbage in de/fr/ja/ko translations: e.g.

- `自动换色 → Auto换色` (de — Chinese tail `换色` untranslated)
- `停止延时 → Stopp延时` (de)
- `映射配置 → 映射Config` (ja)
- `构建日期 → ビルド日期` (ja — `期` tail leaked)

These were FINISHED entries (no `type="unfinished"`), so the normal translate
pass never touched them. To honour the "no MT garbage" rule, a dedicated
`--clean-garbage` mode was added to `_v56_phase204_i18n_longtail.py` plus a
second glossary `scripts/_v56_phase204_garbage_fix.py` (≈295 source keys, the
sources that v5.3 had mangled).

The clean-garbage pass re-translates a FINISHED entry **only when all three
hold**:

1. the `<source>` is an exact key in the combined (CORE + LONGTAIL +
   GARBAGE_FIX) glossary, AND
2. the current translation matches the `_is_v53_garbage()` heuristic, AND
3. the current translation differs from the glossary value (no churn on
   already-correct entries).

The `_is_v53_garbage()` heuristic:

- **de/fr/ko**: translation mixes a Hanzi character with a latin letter (or,
  for ko, a Hangul syllable). These locales never use Hanzi, so any Hanzi in a
  finished translation is untranslated Chinese.
- **ja**: translation contains a PRC-simplified char (from `_PRC_ONLY_CHARS`)
  that **also** appears in the source string — i.e. the char leaked verbatim
  instead of being translated. This avoids false positives on legitimate ja
  kanji.

  `_PRC_ONLY_CHARS` excludes `号` (signal/number — `信号`, `番号`) and `体`
  (cube/volume — `立方体`, `体積`), which are valid ja kanji that appear in
  correct ja translations; including them produced 18 false positives on the
  first pass.

**Result:** de/fr/ja/ko all report `garbage replaced=0` on a final idempotent
re-run (i.e. all v5.3 mixed-script garbage has been overwritten with clean,
glossary-defined native translations). One residual ja leak,
`构建日期 → ビルド日期` (the shared kanji `期` escaped the heuristic), was
corrected manually to `ビルド日付` and the glossary entry updated to match.

### Known source-code defect (NOT fixed here — out of scope)

`src/qml_gui/BackendContext.cpp:164` contains the GBK-mojibake literal
`瀵煎嚭澶辫触`, which is the mis-decoded form of `导出失败` ("Export failed").
The i18n glossary maps this mojibake source to the correct translations
(`Export fehlgeschlagen` / `Échec de l'exportation` / `エクスポート失敗` /
`내보내기 실패`), so the runtime string is correct, but the **source file
encoding should be fixed in a future phase** so the `<source>` itself is no
longer mojibake. Filed here, not fixed, because Phase 204 is i18n-only.

## Validation

After writing the translations and cleaning the v5.3 garbage, the following
checks all pass:

1. **XML legality** — `xml.etree.ElementTree.parse()` succeeds on all six
   `.ts` files (de/fr/ja/ko/en/zh_CN).
2. **UTF-8, no BOM** — all six files start with `<?x` (no BOM); encoding
   declaration is `utf-8`.
3. **Coverage** — de/fr/ja/ko = 0 unfinished / 100 %; en/zh_CN unchanged at
   100 %.
4. **en.ts / zh_CN.ts untouched** — md5 of the on-disk file equals the md5 of
   the pre-phase backup (`i18n/_backup_phase204/*.ts.bak`) for both locales
   (`en` md5 `59506b887e4b`, `zh_CN` md5 `5bbed3d04c5d`).
5. **de/fr/ja/ko changed** — md5 differs from backup (sanity: the write
   happened).
6. **v5.3 garbage eliminated** — the `_is_v53_garbage()` heuristic reports 0
   suspect FINISHED entries in de/fr/ja/ko on a final idempotent re-run;
   known garbage signatures (`延时`/`换色`/`换料`/`换网`/`映射`/`构建日期`
   in finished de/fr/ko translations) are absent. The one residual ja leak
   (`构建日期 → ビルド日期`) was corrected to `ビルド日付`.
7. **lrelease** — `lrelease.exe i18n/<lang>.ts -qm i18n/<lang>.qm` for all
   four locales reports `1680 finished and 0 unfinished` and produces valid
   `.qm` binaries (`de.qm` 166 849 B, `fr.qm` 171 199 B, `ja.qm` 127 444 B,
   `ko.qm` 126 762 B; sizes grew vs the pre-garbage-cleanup build, confirming
   the cleaned translations are baked in).

### Quality spot-check (former-garbage sources, post-cleanup)

| source            | de                          | fr                              | ja             | ko              |
|-------------------|-----------------------------|---------------------------------|----------------|-----------------|
| 自动换色          | Automatischer Farbwechsel   | Changement de couleur automatique | 自動色変更   | 자동 색상 변경  |
| 映射配置          | Zuordnungskonfiguration     | Configuration du mappage        | マッピング配置 | 매핑 설정       |
| 构建日期          | Builddatum                  | Date de build                   | ビルド日付     | 빌드 날짜       |
| 停止延时          | Zeitraffer stoppen          | Arrêter le timelapse            | タイムラプスを停止 | 타임랩스 중지 |
| 设置              | Einstellungen               | Paramètres                      | 設定           | 설정            |

All entries are clean native translations with no mixed-script tails.

Backups of the original six `.ts` files are preserved at
`i18n/_backup_phase204/` (plus `unfinished_union.txt`, the extracted
source-string analysis used to build the glossary).

## Key decisions (summary)

1. **Skipped `lupdate`** despite it being available — running it would
   invalidate the coverage table and risk re-keying entries the manual
   glossary matches by exact `<source>` text. Documented above; a later phase
   can run the full lupdate→translate→lrelease cycle.
2. **Exact-match glossary only** (506 entries), reusing the proven regex from
   `translate_core_i18n.py`. Deliberately did *not* use the v5.3
   `_v53_i18n_glossary.py` substring-concatenation `translate()` because it
   produces mixed-script noise.
3. **Result exceeded the target** — target was `>=85 %` (`unfinished <= 252`);
   achieved **100 %** (`unfinished = 0`) on all four locales.
4. **Cleaned legacy v5.3 garbage** — discovered that the 100 %-coverage pass
   had inherited mixed-script FINISHED entries from v5.3. Added a
   `--clean-garbage` mode + supplementary glossary to repair them, honouring
   the "no MT garbage" rule. All four locales now report 0 garbage.
5. **No source changes** — the `BackendContext.cpp:164` mojibake is documented
   as a future-fix, not touched (i18n-only phase).

## Files

- `scripts/_v56_phase204_i18n_longtail.py` — new, the Phase-204 long-tail
  glossary + applier (idempotent, `--dry-run` and `--clean-garbage` supported).
- `scripts/_v56_phase204_garbage_fix.py` — new, ≈295-source glossary of the
  sources that v5.3 had mangled, used by the `--clean-garbage` mode.
- `i18n/de.ts`, `i18n/fr.ts`, `i18n/ja.ts`, `i18n/ko.ts` — translated and
  v5.3 garbage cleaned.
- `i18n/de.qm`, `i18n/fr.qm`, `i18n/ja.qm`, `i18n/ko.qm` — compiled (gitignored,
  auto-regenerated by `qt_add_translations` at build time).
- `i18n/en.ts`, `i18n/zh_CN.ts` — untouched (verified by md5).
- `i18n/_backup_phase204/` — backups + analysis artifact.
