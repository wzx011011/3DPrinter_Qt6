# Phase 241 Summary — Page Honesty And CLI Surface

**Completed:** 2026-08-16
**Requirements:** PAGE-01..04, CLI-01..02 — all done. Final phase of v5.16.
**Verification:** canonical `auto_verify_with_vcvars.ps1` exit 0 (agent run +
independent re-run before commit; all 8 suites, app launch, E2E). Manual
suite runs: ViewModelSmoke 155/0, QmlUiAudit 148/0, CliTests 21 passed
(+1 pre-existing testSliceBlock20XY fixture gap, out of gate).

## Changes (27 files, +2319/−237)

1. **PAGE-01 HomePage honesty**: recent projects bind the real persistence
   (QSettings recentProjects, upstream app_config recent_projects; mock
   paths removed) with clickable cards → open; quick actions wired to real
   handlers; dead ModelMall entry removed; DailyTips rotates the hints.json
   database (prev/next + documentation link) without spamming the
   notification stack.
2. **PAGE-02 ProjectPage honesty**: console.log new-project button →
   backend.topbarNewProject(); file tree from the real loaded project
   (project file + plates + object source modules; honest empty state);
   Size/Modified via QFileInfo accessors.
3. **PAGE-03 calibration completeness**: pa_pattern (in-code generation via
   upstream CalibPressureAdvancePattern + SuggestedConfigCalibPAPattern,
   Plater.cpp:9418-9563) and pa_tower (bundled tower_with_seam.stl,
   Plater.cpp:9585) modes; FlowRate two-pass flow (pass1/pass2.3mf assets,
   Plater.cpp:9784-9791) with the Fine Calibration (Pass 2) continuation;
   saveCalibrationResultToPreset writes PA → pressure_advance and flow
   ratio → filament_flow_ratio via the Phase 235 preset-write path
   (read-only presets honestly fail with a notification); history persists
   to AppDataLocation/calibration_history.json across service instances.
4. **PAGE-04 preferences take effect**: startup applies the saved
   defaultPage (main_qml.cpp applyStartupPagePreference before QML load);
   use_inches drives mm↔in display conversion in the transform panel and
   measure readout (storage stays mm; SettingsViewModel displayLength/
   storageLength); autoSave renamed honestly to 定期备份 with
   minute-level save-on-change snapshots to the app-data backup dir
   (writeProjectSnapshot without hijacking currentProjectPath_; upstream
   backup_switch seconds-level crash backup documented as the delta);
   dead userRole/reducedMotion/compactMode settings REMOVED (no upstream
   mapping, zero consumers); region/autoUpload/checkUpdates/dev-knobs show
   visible not-yet-effective hints; Developer category gated by
   developerMode.
5. **CLI-01/02**: pre-slice transforms --arrange/--orient/--cut <axis:pos>/
   --split/--assemble/--repair/--scale-to-fit <x,y,z> (all routed to real
   service methods; bad axis rejected); exports --export-stl (merged)/
   --export-3mf/--export-slicedata <dir> (per-plate <base>_plateN.gcode.3mf
   via exportGcode3mf, requires --slice); arbitrary --key value PrintConfig
   overrides schema-validated via print_config_def +
   set_deserialize_strict (unknown key / bad value → error exit) and
   reaching the G-code config block (tested).

## Tests

- CliTests +10 (transform combos, scale-to-fit + bad-axis, export-3mf,
  slicedata guard + end-to-end bundle, unknown-key/bad-value rejection,
  override reaches config block); stale testLoadHotend assertion fixed to
  the real "Objects:" header.
- ViewModelSmokeTests +5 (155 green): recent persistence + signal routing,
  save-to-preset, history persistence across instances, startup-page
  mapping + inches math, backup snapshot file; calibration mode tables
  extended with PA_Pattern/PA_Tower.
- QmlUiAuditTests +pageHonestyAndCliSourceAudit (148 green): all PAGE/CLI
  honesty tokens (no console.log buttons, real recent binding, no dead
  ModelMall, preference consumers or hints, CLI flags, key-override
  validation).

## Honest gates / upstream deltas (documented in code)

- --cut is an OWzx superset (upstream CLI prints "Cut operation is not
  supported yet", OrcaSlicer.cpp:3599); --repair runs the real fixMesh
  where upstream no-ops; --arrange is flag-form vs upstream 0/1/auto
  tri-state; unplaceable arrange keeps original positions.
- PA Pattern generation uses default-config values (preset-derived
  accel/jerk normalization is GUI-side upstream), runtime-guarded.
- Backup snapshots are minute-level save-on-change (label says exactly
  that), not the upstream seconds-level crash backup.
- Follow-ups documented: ProjectPage plate thumbnails need a base64 image
  provider (tree shows structure); pre-existing HomePage login/bind dialog
  ReferenceError warnings (unmodified code, non-fatal); CliRunner::
  printUsage unreachable legacy (Qt help is authoritative).
