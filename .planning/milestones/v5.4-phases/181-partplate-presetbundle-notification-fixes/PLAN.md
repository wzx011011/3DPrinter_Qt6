# Phase 181: PartPlate + PresetBundle + Notification Fixes

**Status:** Planned
**Workstream:** CL
**Requirement:** CRASH-02
**Dependencies:** none (Wave A parallel)

## Goal

Port three upstream bb3 fixes touching data model, dialog rendering, and notification UI. All three are confirmed applicable (Qt6 has equivalent code paths/files).

## Scope

### A4 — PartPlate stale instance id (upstream `4b7182b048`)

- **Upstream fix:** `src/slic3r/GUI/PartPlate.cpp` + `.hpp` — deleting an instance left a stale instance id in `obj_to_instance_set`, causing an out-of-bounds scan. The fix makes `valid_instance()` const and adds bidirectional bounds checks at all 6 scan sites.
- **Qt6 equivalent:** `src/core/model/PartPlate.{cpp,h}` (already exists as an independent port).
- **Approach:** Audit `PartPlate.cpp` for all `obj_to_instance_set` / instance id scan loops. Add `valid_instance()` bounds checks (both lower and upper) at each site. Mirror upstream's 6-site fix.

### A8 — PresetBundleDialog type field (upstream `8e868118d2`)

- **Upstream fix:** `src/slic3r/GUI/PresetBundleDialog.cpp` + `resources/web/dialog/PresetBundleDialog/index.js` — the type field for process presets was rendered as `"presets"` instead of `"processes"`.
- **Qt6 equivalent:** `src/qml_gui/dialogs/ExportPresetBundleDialog.qml`.
- **Approach:** Audit ExportPresetBundleDialog.qml for any hardcoded `"presets"` string where the upstream intent is `"processes"` for the process preset type. Likely a one-line fix per occurrence.

### A9 — Shared profile notification minimize + wrap (upstream `b2adfb5c13`)

- **Upstream fix:** `src/slic3r/GUI/NotificationManager.cpp` — SharedProfilesNotification had a minimize-state bug and text-wrapping issue.
- **Qt6 equivalent:** `src/qml_gui/components/NotificationCenter.qml`.
- **Approach:** Audit NotificationCenter.qml for the shared-profile notification rendering. Replicate the minimize-state fix (collapsed/expanded state machine) and the text-wrap fix (likely a `wrapMode: Text.Wrap` or elide-mode change). The Qt6 notification system is independently implemented (not a port), so the exact fix points must be re-derived.

## Out of Scope

- Any new product behavior beyond matching upstream bb3.
- Changes to libslic3r.

## Verification

- QmlUiAuditTests: add CRASH-02 anchor in Phase 187.
- Manual test A4: add multi-instance object to plate, delete one instance, switch plates — no crash / no stale data.
- Manual test A8: open ExportPresetBundleDialog, verify process presets show "processes" type, not "presets".
- Manual test A9: trigger a shared-profile notification, verify minimize + wrap behavior.
- Canonical build exits 0, 0 errors.

## Risk Notes

- A4 is the highest-value fix (potential crash on multi-instance workflows). The Qt6 PartPlate.cpp is an independent port — confirm it actually has the same instance-id scan pattern before assuming the fix applies 1:1.
- A9's notification system in Qt6 is a complete rewrite of upstream NotificationManager — the fix logic translates but the exact lines don't.
