# Phase 181: PartPlate + PresetBundle + Notification Fixes — RESEARCH SUMMARY

**Status:** Researched — all three items NOT APPLICABLE, no code changes
**Workstream:** CL
**Requirement:** CRASH-02
**Researched:** 2026-07-20

## Outcome

**A4, A8, A9 are all NOT APPLICABLE to Qt6 architecture. Zero code changes.**

Same root cause as Phase 180: these upstream fixes target OrcaSlicer's wxWidgets/ImGui GUI layer specific classes that either have completely different architecture in Qt6 (PartPlate) or do not exist at all in Qt6 (PresetBundleDialog browser, SharedProfilesNotification).

## A4 — PartPlate stale instance id (upstream `4b7182b048`)

**Verdict: NOT APPLICABLE.**

### Why Qt6 is immune

Upstream bug requires: (1) `obj_to_instance_set` retains stale instance ids after deletion (lazy prune), (2) a scan function uses `obj_id` to index `m_model->objects` without checking `instances` bounds, dereferencing garbage `ModelInstance*`.

Qt6 architecture:
- **PartPlate is a pure value object** (`src/core/model/PartPlate.h` 341 lines, `.cpp` 41 lines). Header explicitly notes: "does not compile wxWidgets GUI, does not mirror GL rendering layer".
- **No `m_model` pointer**: PartPlate cannot reach `ModelObject::instances`, so the OOB indexing attack surface is structurally absent.
- **Synchronous rebuild, not lazy prune**: every object mutation path (`deleteObject:4153-4172`, `meshBoolean:3560-3574`, `swapObjectOrder:4630-4645`, `duplicateObject:5053-5066`) calls `clearInstances()` + full re-add loop, with new-index truncation (`if (adjusted >= 0 && adjusted < modelCount_)`). No stale-id window exists.
- **6 upstream functions do not exist in Qt6** (`has_printable_instances`, `printable_instance_size`, `is_all_instances_unprintable`, `get_extruders_under_cli`, `duplicate_all_instance`, `set_pos_and_size` — grep zero matches).
- The one place Qt6 actually indexes `instances[i]` (`ProjectServiceMock.cpp:4457`) already has complete bidirectional bounds check.

### Evidence files

- `src/core/model/PartPlate.{h,cpp}` — pure value object, no m_model
- `src/core/services/ProjectServiceMock.cpp:3560-3574, 4153-4172, 4457, 4630-4645, 5053-5066` — synchronous rebuild pattern

## A8 — PresetBundleDialog type field (upstream `8e868118d2`)

**Verdict: NOT APPLICABLE.**

### Why Qt6 is immune

Upstream bug: shared/remote preset bundle browser (`PresetBundleDialog` web-based) rendered the type field for process presets as `"presets"` instead of `"processes"`. Two fixes: C++ JSON key (`metadata.print_presets` → `temp["processes"]`) + JS renderer (5 variable renames).

Qt6 has no equivalent:
- **No shared/remote bundle browser**: `grep "PresetBundleDialog|bundleId|printersByBundle|processesByBundle|metadata.print_presets|ListBundles"` zero matches in `src/`.
- `ExportPresetBundleDialog.qml` (87 lines) is a local export dialog: just a description + "Choose path..." button + FileDialog. No bundle list, no type labels, no three-category mapping.
- Qt6 export uses **different JSON schema** (`PresetServiceMock.cpp:871-874`): `root["presets"]` is a flat array of all user presets (each item has a `category` field for print/filament/printer). The key name `"presets"` is semantically correct here (it IS a collection of presets), not the upstream's "should be processes".
- Forcing a rename to `"processes"` would break Qt6's own `importBundle` round-trip (`PresetServiceMock.cpp:915` reads `"presets"`) AND be semantically wrong (array contains all three categories, not just process).

### Evidence files

- `src/qml_gui/dialogs/ExportPresetBundleDialog.qml` — local export dialog, no bundle browser
- `src/core/services/PresetServiceMock.cpp:871-874, 915` — Qt6 JSON schema with `"presets"` flat array

## A9 — SharedProfilesNotification minimize + wrap (upstream `b2adfb5c13`)

**Verdict: NOT APPLICABLE.**

### Why Qt6 is immune

Upstream bug: `SharedProfilesNotification::render_text` rendered "Browse shared profiles" + "Don't show again" hyperlinks even when minimized (added a "More" link instead), and `init` over-counted lines because `PopNotification::count_lines()` appended a duplicate placeholder endline.

Qt6 architecture has no equivalent at any level:
- **No SharedProfilesNotification class** — grep `"SharedProfile|shared_profile|browseShared|show_shared_profiles"` zero matches in `src/`.
- **NotificationType enum has no SharedProfiles member** (`BackendContext.h:33-65` — 10 NotificationLevel + 16 NotificationType values, none for shared profiles).
- **No shared profile feature** — Qt6 does not implement OrcaSlicer's cloud/remote shared-preset-bundle browsing (out of scope per v5.3 DECLINED list).
- **No m_multiline / m_lines_count / m_endlines concept** — Qt6 notification system is QML-based with native text wrapping (`NotificationCenter.qml:179 wrapMode: Text.Wrap; maximumLineCount: 3; elide: Text.ElideRight`, `ErrorToast.qml:111 elide: Text.ElideRight`). The line-counting attack surface doesn't exist.
- **No minimize/expand interaction** — ErrorToast is fixed-layout (notiType switches button regions). No "More" button concept.

### Evidence files

- `src/qml_gui/components/NotificationCenter.qml:177-180` — QML native wrap+elide
- `src/qml_gui/components/ErrorToast.qml:111` — single-line elide
- `src/qml_gui/BackendContext.h:33-65` — NotificationType enum, no SharedProfiles

## Code changes: 0 lines

No code modifications. All three fixes are upstream-wxWidgets/ImGui-specific with no Qt6 equivalent.

## Pattern observation (across Phase 180 + 181)

All 5 researched items (A1/A2/A4/A8/A9) are NOT APPLICABLE. The common pattern: upstream bb3's GUI-layer fixes target either (a) wxWidgets destructor-ordering traps, (b) wxGrid/ImGui widget bugs, (c) remote/cloud features Qt6 hasn't implemented, or (d) OpenGL rendering specifics that Qt6's RHI architecture replaces. **None of these bug classes have attack surface in Qt6.**

This strongly suggests Phase 182's A3/A5/A6/A10 research items will also be N/A (they're the same bug classes). Phase 182's only real work is A7 (STEP reload_from_disk), which DOES have a Qt6 equivalent (`EditorViewModel::reloadFromDisk`).

## Recommendation for Phase 187 (REGRESS-08)

For CRASH-02 anchor, assert the architectural immunities:
- A4 anchor: verify PartPlate.h has no `m_model` member + `clearInstances()` method exists (the synchronous-rebuild mechanism).
- A8 anchor: verify Qt6 export JSON schema uses `"presets"` flat array (not the upstream three-category mapping).
- A9 anchor: verify `NotificationCenter.qml` uses `wrapMode: Text.Wrap` (native wrapping, not line-counting).
