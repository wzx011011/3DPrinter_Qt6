# Phase 226: Source-Mapped Process Hierarchy - Research

**Researched:** 2026-08-03
**Domain:** OrcaSlicer FFF Process page/group manifest migration to Qt6/QML
**Confidence:** HIGH

## User Constraints (from CONTEXT.md)

### Locked Decisions

No locked implementation decision was recorded because discuss was skipped.

### the agent's Discretion

All implementation choices are at the agent's discretion because discuss was
skipped. Follow the ROADMAP success criteria, v5.11 research summary, project
conventions, and the locked OrcaSlicer source.

### Deferred Ideas (OUT OF SCOPE)

Group disclosure, filtering, typed rows, preset feedback, multi-nozzle UI,
Printer/Material expansion, network, and SLA are out of this phase.

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| HIER-01 | Show Process pages in upstream order: Quality, Strength, Speed, Support, Multimaterial, Others. | Canonical six-page manifest and QML tab contract below. [CITED: .planning/REQUIREMENTS.md] |
| HIER-02 | Reach every supported Process option from its upstream-derived page and group. | Ordered page-qualified membership manifest, fail-closed mapping, and model/QML test seams below. [CITED: .planning/REQUIREMENTS.md] |

## Summary

Phase 226 should replace the current print-tier `QHash` mapping and category
fallback with one ordered C++ manifest transcribed from `TabPrint::build()`.
That manifest must be the sole source for both page/group lookup and Process
row order. QML selects a page and renders the already ordered result; it must
not sort, regroup, or provide an `Other` fallback. [CITED:
third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:2005-2376] [CITED:
.planning/phases/226-source-mapped-process-hierarchy/226-UI-SPEC.md]

The current implementation is materially divergent: `SettingsDialog.qml`
hard-codes `Base`, `Cooling`, `Retraction`, and singular `Other`, while omitting
`Multimaterial` and `Others`; `ConfigOptionModel` also keeps category-derived
fallback pages and a cross-tier page order. [VERIFIED: codebase grep]

**Primary recommendation:** Introduce a single ordered, page-qualified Process
manifest in `ConfigOptionModel`, derive lookup and ordered page projections from
it, and make unmapped loaded print keys a C++ mapping failure rather than a UI
fallback. [CITED: .planning/research/v5.11-PROCESS-SETTINGS-SUMMARY.md]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|--------------|----------------|-----------|
| Canonical Process page/group/key order | C++ model | -- | `ConfigOptionModel` owns schema metadata and applies page/group values after schema loading. [CITED: src/qml_gui/Models/ConfigOptionModel.cpp:1527-1612] |
| Active Process page selection | QML presentation | C++ model projection | The dialog owns local tab focus/selection, while the C++ manifest determines which rows are valid for that page. [CITED: src/qml_gui/dialogs/SettingsDialog.qml:52-115] |
| Ordered mapped-row projection | C++ model | QML ListView | QML may render a projection but must not reproduce upstream hierarchy rules. [CITED: .codex/rules/qml-boundaries.md] |
| Option value, dirty, preset, and slice behavior | Existing C++ model/viewmodel | QML OptionRow | This phase preserves the current `ConfigOptionModel -> ConfigViewModel` edit path. [CITED: .planning/phases/226-source-mapped-process-hierarchy/226-UI-SPEC.md] |

## Exact Upstream Contract

`TabPrint::build()` defines these technical page keys and exact page-qualified
group order. Group membership and option order are the active
`append_single_option_line` sequence in that group, including calls that append
an already obtained `Option`, not only string-literal overloads. [CITED:
third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:2005-2376]

| Page | Ordered groups |
|------|----------------|
| Quality | Layer height; Line width; Seam; Precision; Ironing; Wall generator; Walls and surfaces; Bridging; Overhangs |
| Strength | Walls; Top/bottom shells; Infill; Advanced |
| Speed | Initial layer speed; Other layers speed; Overhang speed; Travel speed; Acceleration; Jerk(XY); Advanced |
| Support | Support; Raft; Support filament; Advanced; Tree supports |
| Multimaterial | Prime tower; Filament for Features; Ooze prevention; Flush options; Advanced |
| Others | Skirt; Brim; Special mode; G-code output; Post-processing Scripts; Notes |

The commented-out support group and deprecated commented options in the
upstream source are not part of the active manifest. `Base`, `Cooling`,
`Retraction`, `Temperature`, and singular `Other` are not Process pages.
[CITED: third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:2005-2376] [CITED:
.planning/phases/226-source-mapped-process-hierarchy/226-UI-SPEC.md]

## Local Gap Inventory

| Local surface | Gap | Required Phase 226 correction |
|---------------|-----|-------------------------------|
| `SettingsDialog.qml` tab array | The Process branch hard-codes eight divergent tabs and filters using that string key. [VERIFIED: codebase grep] | Bind the Process branch to the exact six C++ manifest page keys and preserve Printer/Material arrays unchanged. |
| `ConfigOptionModel::pageNames()` | It uses a cross-tier known order containing old print pages, then appends unknown pages alphabetically. [VERIFIED: codebase grep] | Add a print-only exact manifest API; do not change generic Printer/Material ordering for this phase. |
| `pageForCategory()` and `optPage()` | Empty print mapping resolves through category and finally singular `Other`. [VERIFIED: codebase grep] | Do not invoke this fallback in the Process projection. Missing manifest membership is a test/assertion failure and must not be rendered. |
| `kPrintPageGroupMap()` | It is an unordered lookup table, so it cannot express source group or option sequence. [VERIFIED: codebase grep] | Replace it as the authority with ordered page/group/key definitions; derive a lookup index only as an implementation detail. |
| `kDesiredKeys` | It mixes Process candidates with filament/printer/output keys and does not ensure every loaded Process key has an active `TabPrint::build()` location. [VERIFIED: codebase grep] | Make the Process loader consume the canonical manifest key union, or explicitly remove non-Process keys from this tier before projection. |
| `OptionRow.qml` | Existing compact headings are inferred from adjacent rows and render a decorative `+` rail. [VERIFIED: codebase grep] | For the Process branch, render source-projected static title/divider rows only; no `+`, chevron, click behavior, counts, or disclosure state in this phase. |
| `qml.qrc` | `GroupNavSidebar.qml` remains unregistered, although the source file remains on disk. [VERIFIED: codebase grep] | Keep it unregistered and do not reintroduce a sidebar. |

### Bounded Manifest Audit

A static comparison of the 302 current `kDesiredKeys` against active direct
`TabPrint::build()` string-option calls found 215 intersecting keys: 93 have a
wrong current page/group and 16 are entirely absent from `kPrintPageGroupMap`.
This is an audit signal, not the implementation oracle: it excludes upstream
`Option` wrapper calls and current non-Process keys, which is why the new
manifest must be transcribed from source rather than generated from this
comparison. [VERIFIED: codebase grep]

The 16 directly observed unassigned supported keys are:
`counterbore_hole_bridging`, `max_travel_detour_distance`,
`solid_infill_filament`, `sparse_infill_filament`,
`spiral_mode_max_xy_smoothing`, `tree_support_branch_angle_organic`,
`tree_support_branch_diameter_angle`,
`tree_support_branch_diameter_double_wall`,
`tree_support_branch_diameter_organic`,
`tree_support_branch_distance_organic`, `wall_filament`,
`wipe_tower_bridging`, `wipe_tower_cone_angle`,
`wipe_tower_extra_spacing`, `wipe_tower_no_sparse_layers`, and
`wipe_tower_rotation_angle`. [VERIFIED: codebase grep]

Misplaced examples show the structural nature of the defect: Prime tower and
Flush options entries are currently placed under Others; Quality Wall generator
and Walls and surfaces entries are placed under Strength or Others; and several
Support Advanced entries are placed in Support. [VERIFIED: codebase grep]

## Standard Stack

| Component | Version | Purpose | Direction |
|-----------|---------|---------|-----------|
| C++17 and Qt 6.10 `QAbstractListModel` | Project-pinned | Own the ordered Process manifest and expose projections to QML. | Reuse existing stack; add no package. [CITED: AGENTS.md] |
| Existing `ConfigOptionModel` | In-repository | Schema metadata, option roles, and source mapping. | Extend only for print-tier manifest/projection. [CITED: src/qml_gui/Models/ConfigOptionModel.h] |
| Existing `ConfigViewModel` | In-repository | Existing filter and preset/value boundary. | Preserve; delegate Process filtering to it before manifest projection. [CITED: src/core/viewmodels/ConfigViewModel.cpp:1231-1288] |
| Existing QML `SettingsDialog` and `OptionRow` | In-repository | Tab and row presentation. | Change only `presetTier == "print"`. [CITED: .planning/phases/226-source-mapped-process-hierarchy/226-UI-SPEC.md] |

No external packages, registry installs, CMake dependency changes, or package
legitimacy audit are required. [VERIFIED: codebase grep]

## Architecture Patterns

### System Architecture Diagram

```text
TabPrint::build() source truth
          |
          v
Ordered C++ Process manifest (page -> group -> ordered keys)
          |                              |
          | lookup during schema load    | exact page/group projection
          v                              v
ConfigOptionModel page/group metadata -> ConfigViewModel filtering
                                             |
                                             v
SettingsDialog Process tabs -> source-ordered rows -> static group title + OptionRow
```

### Pattern 1: Ordered Manifest, Derived Lookup

**What:** Define page, group, and key ordering once as nested C++ data. Build
any `key -> (page, group)` lookup from that ordered data only for lookup speed.
Expose an ordered Process-page API and an ordered filtered-index projection.
[CITED: third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:2005-2376]

**Why:** A `QHash` can answer membership but cannot represent the source order
required by HIER-01/HIER-02. [VERIFIED: codebase grep]

```cpp
// Source: third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:2005-2376
struct ProcessGroupDef {
  QString key;
  QStringList optionKeys;
};
struct ProcessPageDef {
  QString key;
  QList<ProcessGroupDef> groups;
};

// The ordered manifest is the only source for page/group lookup and projection.
```

### Pattern 2: Fail-Closed Process Assignment

**What:** After loading the print schema, validate every row intended for the
Process dialog against the manifest. Preserve an explicit non-Process key list
only where an existing source-backed tier owns it; otherwise do not load it in
the Process model. [CITED: src/qml_gui/Models/ConfigOptionModel.cpp:1544-1612]

**Do not:** Map unknown rows by `category`, append pages alphabetically, or put
them under `Others`. [CITED: .planning/phases/226-source-mapped-process-hierarchy/226-UI-SPEC.md]

### Migration-Safe Implementation Sequence

1. Transcribe the full active `TabPrint::build()` hierarchy, including wrapper
   option lines, into an ordered C++ manifest and add a source-reference comment
   in English ASCII only. [CITED: third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:2005-2376] [CITED: AGENTS.md]
2. Derive existing page/group metadata from that manifest, then restrict the
   Process projection to manifest-mapped rows in manifest order. Retain the
   generic fallback APIs for non-Process tiers only. [CITED:
   .planning/phases/226-source-mapped-process-hierarchy/226-UI-SPEC.md]
3. Expose a Process-specific page list and preordered row/group projection
   through C++; QML forwards only selected page and uses rows as supplied.
   [CITED: .codex/rules/qml-boundaries.md]
4. Replace only the Process tab array and decorative Process header path in
   `SettingsDialog.qml`; preserve current options, edits, presets, and
   Printer/Material branches. [CITED: .planning/phases/226-source-mapped-process-hierarchy/226-UI-SPEC.md]
5. Remove/update existing assertions that require `Other`, then add exact
   manifest and UI audit assertions. [VERIFIED: codebase grep]

## Don't Hand-Roll

| Problem | Do not build | Use instead | Why |
|---------|--------------|-------------|-----|
| Page/group truth | Category heuristics or a second QML mapping | `TabPrint::build()`-transcribed C++ manifest | Upstream defines page, group, membership, and order together. [CITED: third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:2005-2376] |
| Option/preset state | New Process option store or preset service | Existing `ConfigOptionModel` and `ConfigViewModel` route | Existing state carries schema, dirty, source, and slice-invalidation behavior. [CITED: .planning/research/v5.11-PROCESS-SETTINGS-SUMMARY.md] |
| Group navigation | Re-register `GroupNavSidebar.qml` | Existing six-tab dialog layout | The UI contract explicitly retires the sidebar for this workflow. [CITED: .planning/phases/226-source-mapped-process-hierarchy/226-UI-SPEC.md] |

## Common Pitfalls

### Pitfall 1: Repairing Individual `QHash` Entries

**What goes wrong:** Correcting visible examples leaves unassigned or later
upstream keys on fallback pages and still cannot recover order. [VERIFIED:
codebase grep]

**Avoidance:** Replace the table's authority with the nested ordered manifest
and test its complete page/group/key contract. [CITED:
third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:2005-2376]

### Pitfall 2: Treating `Others` as a Fallback Bucket

**What goes wrong:** `Others` is a real final upstream page, not a route for
unmapped `Base`, cooling, retraction, printer, or filament options. [CITED:
third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:2322-2376]

**Avoidance:** Fail test/model validation for an unmapped Process row; never
show it under a fabricated group. [CITED:
.planning/phases/226-source-mapped-process-hierarchy/226-UI-SPEC.md]

### Pitfall 3: Pulling Phase 227 State Into This Phase

**What goes wrong:** Adding chevrons, collapse keys, counts, filtering changes,
or reset actions creates an untested presentation-state contract ahead of its
planned phase. [CITED: .planning/ROADMAP.md]

**Avoidance:** Static source-derived headings only; Phase 226 owns no group
interaction or persistence. [CITED: .planning/phases/226-source-mapped-process-hierarchy/226-UI-SPEC.md]

## Test Seams And Validation Architecture

| Requirement | Behavior | Test type | Automated command | Gap |
|-------------|----------|-----------|-------------------|-----|
| HIER-01 | Exact six Process page keys and order; reject old keys. | QtTest unit | `ctest --test-dir build -R ViewModelSmokeTests --output-on-failure` | Add focused manifest test. [VERIFIED: codebase grep] |
| HIER-02 | Exact page-qualified group order, source membership/order, and no mapped-row fallback. | QtTest unit | `ctest --test-dir build -R ViewModelSmokeTests --output-on-failure` | Add focused manifest/projection test. [VERIFIED: codebase grep] |
| HIER-01, HIER-02 | Process QML binds the six-page C++ contract, has no old tabs/sidebar/decorative plus, and leaves Printer/Material intact. | Source-level QtTest audit | `ctest --test-dir build -R QmlUiAuditTests --output-on-failure` | Replace legacy `Other` token assertions. [VERIFIED: codebase grep] |

`ViewModelSmokeTests` already constructs `ConfigViewModel` and asserts basic
page/group access, while `QmlUiAuditTests` currently locks the obsolete singular
`Other` key. Those are the narrowest stable seams for this phase. [VERIFIED:
codebase grep]

Add an independent expected manifest in the unit test, not an assertion that
merely re-reads production data. Assert all 36 page-qualified groups, the
ordered key list in each group, every loaded Process row maps once, and no row
uses category or `Other` fallback. [CITED:
third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:2005-2376]

**Per task commit:** targeted CTest commands above. [CITED: CMakeLists.txt]

**Per wave merge and phase gate:**
`powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
Report configure, build, app smoke, and whether each QtTest executable ran.
[CITED: .codex/rules/build-rules.md]

Manual visual check after implementation: open the existing Process dialog at
736x593 and the existing higher-scale fixture; verify six one-line tabs,
source-order static headings, no sidebar, and no `+`/chevron. [CITED:
.planning/phases/226-source-mapped-process-hierarchy/226-UI-SPEC.md]

## Project Constraints (from AGENTS.md)

- Use UTF-8 without BOM; use patch/edit APIs for text and run the encoding guard
  before committing. [CITED: AGENTS.md]
- OrcaSlicer is behavior truth; screenshots are layout truth and materially
  wrong UI must be replaced with its obsolete registrations/imports/tests removed.
  [CITED: AGENTS.md]
- Business logic belongs in `src/core`; QML is presentation only. [CITED: AGENTS.md]
- Use only `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
  and `build/` for full verification. [CITED: AGENTS.md]
- Preserve current dirty work and do not modify unrelated files. [CITED: AGENTS.md]

## Security Domain

| ASVS Category | Applies | Standard control |
|---------------|---------|------------------|
| V2 Authentication | No | This local hierarchy projection has no authentication surface. [ASSUMED] |
| V3 Session Management | No | This phase adds no persisted session or credential state. [CITED: .planning/phases/226-source-mapped-process-hierarchy/226-UI-SPEC.md] |
| V4 Access Control | No | The dialog is local and does not introduce privilege boundaries. [ASSUMED] |
| V5 Input Validation | Yes | Validate page/group/key lookup in C++ against the fixed manifest; reject unknown Process mapping. [CITED: .codex/rules/qml-boundaries.md] |
| V6 Cryptography | No | No cryptographic data or transport is added. [ASSUMED] |

The relevant integrity threat is a malformed or stale UI mapping silently
exposing a Process option under the wrong semantic group. The mitigation is an
ordered source manifest plus independent regression assertions, not QML
sanitization. [CITED: third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:2005-2376]

## Environment Availability

| Dependency | Required by | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| CMake | Configure/test discovery | Yes | Installed; version not sampled | None. [VERIFIED: local environment] |
| Ninja | Canonical build | Yes | Installed; version not sampled | None. [VERIFIED: local environment] |
| CTest | Targeted QtTest execution | Yes | Installed; version not sampled | None. [VERIFIED: local environment] |
| `build/ViewModelSmokeTests.exe` | Model assertions | Yes | Existing executable | Rebuild only with canonical command. [VERIFIED: local environment] |
| `build/QmlUiAuditTests.exe` | QML source audit | Yes | Existing executable | Rebuild only with canonical command. [VERIFIED: local environment] |

## Explicit Non-Goals

- Group disclosure, collapse persistence, search/mode behavior, counts, group
  reset, and group keyboard toggling. [CITED: .planning/phases/226-source-mapped-process-hierarchy/226-UI-SPEC.md]
- Typed editor, validation, inheritance/source, vector, or per-extruder UI
  redesign. [CITED: .planning/phases/226-source-mapped-process-hierarchy/226-UI-SPEC.md]
- Preset save/compatibility/dirty workflow changes or slice invalidation changes.
  [CITED: .planning/phases/226-source-mapped-process-hierarchy/226-UI-SPEC.md]
- Printer/Material hierarchy changes, sidebar revival, device/network/SLA work,
  new packages, persistence, or libslic3r algorithm changes. [CITED:
  .planning/phases/226-source-mapped-process-hierarchy/226-UI-SPEC.md]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | The phase adds no authentication, authorization, or cryptographic surface. | Security Domain | Security review scope would need expansion. |

## Sources

### Primary (HIGH confidence)

- `third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:2005-2376` - locked Process page, group, and option ordering.
- `src/qml_gui/Models/ConfigOptionModel.{h,cpp}` - current schema, mapping, fallback, and load seams.
- `src/qml_gui/dialogs/SettingsDialog.qml` and `src/qml_gui/components/OptionRow.qml` - current Process presentation path.
- `.planning/phases/226-source-mapped-process-hierarchy/226-CONTEXT.md` and `226-UI-SPEC.md` - phase boundaries and UI contract.

### Secondary (MEDIUM confidence)

- `.planning/research/v5.11-PROCESS-SETTINGS-SUMMARY.md` - cross-phase architecture and regression context.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - existing in-repository Qt/C++ stack only. [VERIFIED: codebase grep]
- Architecture: HIGH - upstream and current integration points were read directly. [CITED: third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:2005-2376]
- Pitfalls: HIGH - confirmed by static mapping comparison and current tests. [VERIFIED: codebase grep]

**Research date:** 2026-08-03
**Valid until:** 2026-09-02
