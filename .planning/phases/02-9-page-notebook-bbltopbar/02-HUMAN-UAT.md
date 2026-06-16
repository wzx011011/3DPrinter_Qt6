---
status: partial
phase: 02-9-page-notebook-bbltopbar
source: [02-VERIFICATION.md]
started: 2026-06-16T16:10:00Z
updated: 2026-06-16T16:10:00Z
---

# Phase 02: Human UAT — Deferred Runtime Verification

> These items require manual runtime testing. Automated build + unit tests pass (5/5 ViewModelSmokeTests). Source assertions via grep pass. The items below cannot be verified without launching OWzxSlicer.exe.

## Current Test

[awaiting human testing]

## Tests

### 1. Fresh QML warning scan

**expected:** After recent edits to BBLTopbar.qml (CR-01/WR-01/WR-04/WR-06/WR-07 fixes), `build/startup_diagnostics.log` should show ZERO QML warnings related to:
- `backend.TabPosition.tpX of undefined` (replaced by direct Q_PROPERTY accessors)
- HoverHandler/ToolTip ordering warnings
- `displayProjectTitle` binding loop warnings

**test instructions:**
```powershell
# 1. Run the canonical build first
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1

# 2. Launch the executable
./build/OWzxSlicer.exe

# 3. After the window renders (wait ~3s), close it.

# 4. Inspect the log
Get-Content build/startup_diagnostics.log | Select-String "BBLTopbar|main.qml|displayProjectTitle|TabPosition"
```

**result:** [pending]

### 2. Tab click single-dispatch behavior

**expected:** Clicking each of the 9 tabs in BBLTopbar triggers `tabSelectRequested` signal EXACTLY ONCE per click (not twice). Verify via console output:

```powershell
./build/OWzxSlicer.exe 2>&1 | Select-String "tabSelectRequested|currentPage"
```

After clicking each tab once, expect 9 lines total (one per click). Two lines per click indicates a regression of WR-01.

**test instructions:**
1. Launch OWzxSlicer.exe with stderr capture.
2. Click Home → Prepare → Preview → Device → MultiDevice → Project → Calibration → Placeholder1 → Placeholder2 (in order, 1 click each).
3. Close app.
4. Verify exactly 9 `[Backend] tabSelectRequested` log lines appear.

**result:** [pending]

### 3. [File ▾] and [▾] menu visual inspection

**expected:** Both menus render with upstream-aligned order:

- **[File ▾]:** New / Open / Recent ▸ / Save / Save As / Import ▸ (3MF/STL/OBJ/STEP/AMF) / Export ▸ (G-code/3MF/Model) / Quit
- **[▾]:** Edit ▸ (Undo/Redo/Cut/Copy/Paste/Delete/Select All/Invert Selection) / View ▸ / Preferences / Calibration ▸ (9 entries, visible-disabled) / Help ▸

**test instructions:**
1. Launch OWzxSlicer.exe.
2. Click `[File ▾]` — verify menu items appear in the order above.
3. Hover `Import ▸` — verify 5-item submenu expands.
4. Hover `Export ▸` — verify 3-item submenu expands.
5. Click `[▾]` — verify 5 top-level items.
6. Hover `Calibration ▸` — verify 9 disabled entries appear.
7. Close app.

**result:** [pending]

### 4. macOS MenuBar Loader non-activation on Windows (advisory)

**expected:** No native Windows menu bar appears (Windows build should not activate the macOS MenuBar Loader). App should have no `Qt.platform.os === "osx"` triggered code paths.

**test instructions:**
1. Launch OWzxSlicer.exe.
2. Inspect Windows title bar — should show app icon + window title only (no native menu strip like File/Edit/View).
3. Verify QML-rendered BBLTopbar is the only menu surface.

**result:** [pending]

## Summary

total: 4
passed: 0
issues: 0
pending: 4
skipped: 0
blocked: 0

## Gaps

None yet — pending first runtime pass.

## Resolution Path

After all 4 items pass: update `status: partial` → `status: resolved` and frontmatter `started`/`updated` timestamps. Commit with:

```bash
gsd-sdk query commit "test(02): resolve deferred UAT after manual runtime verification" --files .planning/phases/02-9-page-notebook-bbltopbar/02-HUMAN-UAT.md
```

If any item fails: file an issue against Phase 02 commits, or run `/gsd:plan-phase 02 --gaps` for targeted fix plans.
