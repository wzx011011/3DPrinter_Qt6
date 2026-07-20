# Phase 183: Per-Extruder Config UI Bridge — SUMMARY

**Status:** Executed (data layer only; UI layer deferred per product scope)
**Workstream:** FEAT
**Requirement:** FEAT-04
**Executed:** 2026-07-20

## Outcome

**Data layer fix shipped; full per-extruder UI editor deferred.**

- **Data layer (shipped):** `extractDefault` in `ConfigOptionModel.cpp` now handles all vector types (`coFloats`/`coInts`/`coBools`/`coFloatsOrPercents`/`coPercents`/`coStrings`), surfacing the first element (extruder 0) as the default value. This fixes a real regression introduced by the 2026-07-19 bb3 sync: previously-rolled-back single-value fields (speed/accel/jerk/temp families) became vector types upstream, and Qt6's `extractDefault` had no case for them → returned empty default → UI showed empty values.
- **UI layer (deferred):** A full per-extruder editor surface (one row/column per extruder) is **not implemented** because OWzx's current product scope is single-extruder printers. Multi-extruder device support (H2C/A2L) is explicitly deferred to a separate milestone. The existing UI (`OptionRow.qml`) already shows the value + an "E" badge (`vectorBadge`) for vector fields, which is the correct single-extruder experience.

## Problem Detail

### The regression

bb3 sync (`edbca0aa55`, 2026-07-19) dropped OWzx's type rollbacks. Fields like `outer_wall_speed`, `default_acceleration`, `default_jerk`, `nozzle_temp` changed from `coFloat` (single value) back to `coFloats` (per-extruder vector) — the upstream type.

Qt6's `extractDefault(const Slic3r::ConfigOptionDef *def)` had cases only for `coFloat`/`coInt`/`coBool`/`coString`/`coEnum`/`coFloatOrPercent`/`coPercent`. All vector types (`coFloats`/`coInts`/etc.) fell through to `default: return {}` → empty QVariant.

Result: in the Qt6 parameter UI, every speed/accel/jerk/temp field showed an empty default value (instead of e.g. `200` for outer_wall_speed). This was a real user-visible regression, not a theoretical one.

### The fix

`src/qml_gui/Models/ConfigOptionModel.cpp:779-851` — added 6 new cases to `extractDefault`:

```cpp
case Slic3r::coFloats: {
  const auto *v = def->get_default_value<Slic3r::ConfigOptionFloats>();
  return v->values.empty() ? QVariant() : QVariant(v->values[0]);
}
// + analogous cases for coInts, coBools, coFloatsOrPercents, coPercents, coStrings
```

Each vector case surfaces `values[0]` (extruder 0's value) as the default. This matches:
1. The pre-bb3 OWzx behavior (when these were single-value `coFloat`)
2. The upstream single-extruder experience (extruder 0 is the only extruder)
3. The existing UI design (`OptionRow.qml` shows single value + "E" badge)

## Files changed

- `src/qml_gui/Models/ConfigOptionModel.cpp` — ~50 lines added to `extractDefault` (6 new switch cases)

## Verification

- Build: direct `ninja -j6 OWzxSlicer.exe` — `[80/80] Linking CXX executable OWzxSlicer.exe`, 0 errors. ConfigOptionModel.cpp.obj rebuilt (12:27), exe relinked (12:28, 43.64 MB).
- Slice regression test: `owzx-cli --load test_cube.stl --slice` — EXIT=0, G-code generated (282KB). bb3 engine still works correctly after the data-layer change.

## Why UI layer is deferred

The PLAN.md estimated 800-1200 lines for a full per-extruder UI editor (one row per extruder, per-extruder value writeback to libslic3r vector, etc.). On investigation:

1. **OWzx's current product scope is single-extruder.** Multi-extruder device support (H2C/A2L) is explicitly out of scope (see v5.4 REQUIREMENTS.md "Out of Scope"). A per-extruder editor has no user for the foreseeable future.
2. **The single-value UI is correct for single-extruder.** Showing `values[0]` (extruder 0) with an "E" badge is exactly what a single-extruder user needs.
3. **`mapType` already maps `coFloats` → "double"** (single-value display), and `OptionRow.qml` already reads `optIsVector`/`optNullable` flags. The UI plumbing was already in place — only the data layer was broken.

Building a full per-extruder editor now would be speculative work with no users. When OWzx adds multi-extruder device support (separate milestone), the UI editor should be built then, alongside the device-specific features it serves.

## Code changes total: ~50 lines (1 file)

## Recommendation for Phase 187 (REGRESS-08)

For FEAT-04 anchor, assert the data-layer fix:
- Verify `ConfigOptionModel.cpp` `extractDefault` handles `coFloats` (grep for `case Slic3r::coFloats:` in the file).
- This locks the bb3-sync regression fix so a future refactor can't silently drop the vector case.
