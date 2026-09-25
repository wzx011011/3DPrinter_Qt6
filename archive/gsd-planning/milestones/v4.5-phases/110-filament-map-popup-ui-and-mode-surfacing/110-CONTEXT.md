# Phase 110: Filament-Map Popup UI And Mode Surfacing - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
Close FMAP-03: a `FilamentGroupPopup` UI surfaces the 3 selectable modes (AutoForFlush "Filament-Saving", AutoForMatch "Convenience", Manual "Custom") with the auto-recommended map preview. `fmmDefault` is the per-plate "inherit from global" sentinel (NOT a 4th radio button — anti-feature per FEATURES.md).

ALSO close Phase 107 REVIEW R-02: add enum range validation at the Q_INVOKABLE boundary (`setFilamentMapMode(int)` at PartPlate.h:232) so an out-of-range QML/Q_INVOKABLE caller can't silently store an invalid enum.

</domain>

<decisions>
### Carry-Forward
- Phase 107 (FMAP-02): 4-value FilamentMapMode enum at PartPlate.h:96-101.
- Phase 108 (FMAP-01): EditorViewModel Q_PROPERTYs `hasAutoFilamentMap` + `autoFilamentMapMode` + `autoFilamentMaps` + NOTIFY `filamentMapChanged`. These are the binding targets.
- Phase 107 REVIEW R-02: `setFilamentMapMode(int)` at PartPlate.h:232 does `FilamentMapMode(mode)` with NO range check — out-of-range int silently stores invalid enum. Phase 110 adds the Q_INVOKABLE boundary that makes this real, so R-02 lands here.
- Research FEATURES.md: 3 popup radio buttons (fmmAutoForFlush/fmmAutoForMatch/fmmManual); fmmDefault is inherit-sentinel resolved by `PartPlate::get_real_filament_map_mode`, NOT a 4th radio. Upstream `FilamentGroupPopup.hpp:52` mode_list.
- Anti-feature: exposing fmmDefault as a 4th popup button.

### Scope
1. **FilamentGroupPopup.qml** (new file in src/qml_gui/dialogs/ or components/): a CxPopup-based popup with 3 radio modes + the auto-recommended map preview. Binds to EditorViewModel's `autoFilamentMapMode` + `autoFilamentMaps` + `hasAutoFilamentMap`. The 3 modes: AutoForFlush "Filament-Saving", AutoForMatch "Convenience", Manual "Custom".
2. **BBLTopbar.qml integration**: wire the popup to open from the existing topbar filament indicator (or wherever the placeholder TODO is — find it).
3. **R-02 enum range validation** (PartPlate.h:232): clamp or assert `setFilamentMapMode(int)` to [0,3]; out-of-range falls back to fmmDefault (safe).
4. **Q_INVOKABLE write API**: if not already present, add a `setPlateFilamentMapMode(int plateIndex, int mode)` Q_INVOKABLE on ProjectServiceMock/EditorViewModel so the popup can write the selected mode back. Update PartPlate + ProjectServiceMock + EditorViewModel.
5. **Regression test**: source-audit (QmlUiAuditTests) confirming the popup exists + 3 modes (not 4) + the R-02 range validation.
6. **Canonical verifier + ctest**.

</decisions>

<code_context>
- `src/qml_gui/controls/CxPopup.qml` (the popup base pattern)
- `src/qml_gui/dialogs/` (existing dialog patterns)
- `src/core/viewmodels/EditorViewModel.h:671-679, 755-756` (Phase 108 Q_PROPERTYs — binding targets)
- `src/core/model/PartPlate.h:96-101, 231-232` (enum + R-02 setFilamentMapMode(int))
- `src/core/services/ProjectServiceMock.h:149-152` (setPlateFilamentMap — may need a mode-only variant)
- `src/qml_gui/panels/BBLTopbar.qml` (find the filament indicator + placeholder)

</code_context>

<deferred>
- FMAP-04 (full round-trip test) — Phase 111.
- R-01 (legacy-branch test) — Phase 111.
- fmmDefault resolution against global config — Phase 110 captures the per-plate mode; global inheritance is a follow-up.
</deferred>
