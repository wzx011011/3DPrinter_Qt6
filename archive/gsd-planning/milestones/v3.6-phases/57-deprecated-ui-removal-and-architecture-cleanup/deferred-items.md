# Phase 57 — Deferred Items (out-of-scope discoveries)

Logged per executor scope-boundary rule: "Only auto-fix issues DIRECTLY caused
by the current task's changes. Pre-existing warnings, linting errors, or
failures in unrelated files are out of scope."

## 57-02 Task 4 — Pre-existing mojibake in Wave 1 source files

**Found during:** Task 4 encoding-guard pass.

**Files affected (not exhaustive — guard flagged a sample):**
- `src/qml_gui/BackendContext.h` lines 37, 73, 183, 345, 499 — Chinese
  ///< comments decoded as GBK-as-UTF-8 garbage (e.g. `鎴愬姛/瀹屾垚`
  instead of `成功/完成`, `鈥?` instead of `→`).
- `src/core/viewmodels/ConfigViewModel.cpp` lines 490, 1105, 1410 — same
  pattern (`M枚ller` instead of `Möller`, `瀛愪覆` instead of `子串`).

**Origin:** Confirmed pre-existing via `git show 264413c:<path>` (parent of
the first Wave 1 commit `36e6dad`). The mojibake was already present before
Phase 57 began; Wave 1 did not introduce it.

**Why deferred:** CLEAN-04 covers encoding for files Wave 1 + Wave 2
*changed*, not a codebase-wide mojibake remediation. The flagged lines are
inside comment blocks that Wave 1/Wave 2 did not touch (Wave 1 removed method
impls, not Chinese comments on unrelated enum/property docs). Fixing would
require rewriting dozens of Chinese comment lines across many files, which
is its own dedicated cleanup phase, not a Phase 57 side-quest.

**Suggested follow-up phase:** A future "encoding normalization" pass should
audit the entire `src/` tree for GBK-as-UTF-8 mojibake and restore the
intended Chinese text from upstream OrcaSlicer source where applicable.

## 57-02 Task 4 — qml.qrc BOM (FIXED in-wave)

The BOM in `src/qml_gui/qml.qrc` also predated Phase 57 (present at
`264413c`), but CLEAN-04 explicitly covers `qml.qrc` as a Wave 1 file
(Wave 1 removed 7 `<file>` entries from it), so the BOM was stripped via
`encoding_guard.py --fix` and committed in Wave 2. Only the 3-byte prefix
was removed; content is byte-identical otherwise.

## 57-02 Task 3 — `openSettings()` misleading "legacy" comment

`BackendContext.cpp:425-432 openSettings()` carries a comment "This stub is
retained for any legacy caller that still wants to land on the Project tab".
The method is LIVE (5 QML callers: LeftSidebar.qml ×2, PreparePage.qml ×2,
main.qml ×1) and routes through the live page router; "stub" / "legacy" is
misleading wording. Not dead UI and not a C++ comment-cleanup target of
CLEAN-02 (which scopes UI files), so deferred. A future wording cleanup can
rephrase the comment.
