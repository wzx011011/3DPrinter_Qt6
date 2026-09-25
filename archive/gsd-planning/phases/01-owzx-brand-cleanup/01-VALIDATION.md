---
phase: 1
slug: owzx-brand-cleanup
status: approved
nyquist_compliant: true
wave_0_complete: false
created: 2026-06-15
---

# Phase 1 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Qt Test (QtTest) + CTest + grep assertions |
| **Config file** | `CMakeLists.txt` (CTest enabled in root) |
| **Quick run command** | `cmake --build build --config Release` |
| **Full suite command** | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` |
| **Estimated runtime** | ~120-300 seconds (incremental) / ~600-900s (full clean build) |

---

## Sampling Rate

- **After every task commit:** `cmake --build build --config Release` (incremental compile to catch breakage)
- **After every plan wave:** Full grep assertion suite (see Per-Task Verification Map) + incremental build
- **Before `/gsd:verify-work`:** `scripts/auto_verify_with_vcvars.ps1` must pass AND all grep assertions must return empty
- **Max feedback latency:** 120 seconds (incremental build)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 1-01-01 | 01 | 1 | BRAND-01 | — | Zero "Creality" brand strings in user-facing UI | grep assertion | `grep -ri "creality" src/ --exclude-dir=third_party \| grep -v "vendor\|upstream\|Creality K1\|Creality Ender\|Creality Generic\|Crality3D"` | ❌ W0 | ⬜ pending |
| 1-01-02 | 01 | 1 | BRAND-02 | — | No `third_party/CrealityPrint` references in build configs | grep assertion | `grep -ri "third_party/CrealityPrint" cmake/ scripts/ CMakeLists.txt .gitmodules` | ❌ W0 | ⬜ pending |
| 1-01-03 | 01 | 2 | BRAND-03 | — | No `Crality3D` / `CrealityGL` references after rename | grep assertion | `grep -ri "Crality3D\|CrealityGL" src/` | ❌ W0 | ⬜ pending |
| 1-01-04 | 01 | 2 | BRAND-03 | — | New `OWzx` namespace declarations present | grep assertion | `grep -r "namespace OWzx" src/core/` | ❌ W0 | ⬜ pending |
| 1-01-05 | 01 | 3 | BRAND-04 | — | No Creality brand in resource files | grep assertion | `grep -ri "creality" src/qml_gui/data/ src/qml_gui/assets/ 2>/dev/null` | ❌ W0 | ⬜ pending |
| 1-01-06 | 01 | 3 | BRAND-05 | — | Version string matches OrcaSlicer main branch | grep assertion | `grep -E "(7\.0\.1\|SoftFever\|2\.4\.0)" CMakeLists.txt cmake/buildinfo.h.in.stub` | ❌ W0 | ⬜ pending |
| 1-01-07 | 01 | 4 | All | — | Build still passes after all changes | build | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `scripts/verify_brand_cleanup.ps1` — new grep assertion script aggregating all BRAND-01~05 checks; returns exit code 0 only when all grep patterns return empty AND build passes. **Created in Plan 03 Task 2 (Wave 3) as a permanent verification tool.** Earlier waves (01-01, 01-02) use inline grep assertions per-task per the Per-Task Verification Map below.
- [ ] `tests/CliTests.cpp` and `tests/ViewModelSmokeTests.cpp` — update `#include` paths and `creality_app_core` → `owzx_app_core` target name after rename (handled in Plan 02 Task 1)
- [ ] No new test framework installation required — Qt Test + CTest already configured

*Existing infrastructure (Qt Test + auto_verify_with_vcvars.ps1) covers Phase 1 verification; only the brand-cleanup-specific assertion script is new (deferred to Wave 3).*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Window title displays "OWzx" at runtime | BRAND-01 | Qt ApplicationWindow.title is set at runtime; visual confirmation needed | Launch app, verify title bar shows "OWzx" not "Creality Print" |
| About dialog shows OWzx brand + correct version | BRAND-01, BRAND-05 | Dialog content rendered through Qt logic | Open About dialog, verify brand string + version string match OrcaSlicer main branch |
| Shortcut overview dialog OWzx-branded | BRAND-01 | Modal dialog content | Open shortcut dialog (Ctrl+?), verify no Creality strings |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references (verify_brand_cleanup.ps1 deferred to Plan 03 Wave 3; earlier waves use inline grep)
- [x] No watch-mode flags
- [x] Feedback latency < 300s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** approved 2026-06-15
