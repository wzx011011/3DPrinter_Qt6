# Phase 93 Visual Evidence — Capture-Blocked Deviation

**Status:** Automated window capture blocked (Phase 88/91 SETVERIFY-02
precedent). Runtime evidence recorded as launch + manual click-through +
canonical verifier + regression ctest.

## Why no screenshots

Automated window capture is blocked in this environment by the same Windows
capture API issue documented in v4.1 SETVERIFY-02 and Phase 88/91. The
AssembleView runtime verification is therefore the Phase 88/91 precedent
combination:

1. **Runtime launch evidence** — `build/OWzxSlicer.exe` (33,751,552 bytes,
   built 2026-07-09 21:17) launched from the build directory, stayed alive
   beyond the 8-second watchdog (no crash), and appeared in the Windows
   process list (~247 MB resident). See `../93-VERIFICATION.md` Runtime
   launch section.
2. **Canonical verifier pass** — `build/93-01-canonical-build.log` shows
   `OWzxSlicer.exe` linked clean with zero errors.
3. **Regression ctest pass** — `build/93-01-test-run.log` shows all 5 suites
   pass; the 3 new Phase 93 slots (including the isolation + milestone audit
   slots) pass by name.

## Manual click-through to AssembleView

To reach each target AssembleView state (against
`shotScreen/装配页.png` / `装配页_爆炸.png` / `装配页_测量.png`):

1. Launch `build/OWzxSlicer.exe`.
2. In the top bar (BBLTopbar), click the AssembleView navigation toggle
   (`backend.requestChangeViewMode(backend.vmAssembleView)` →
   `ViewMode::AssembleView = 2`).
3. The Plater `AssemblePage {}` canvas host renders (NOT the placeholder).
4. **Default view** (`装配页.png`): 4-region chrome + central 3D canvas +
   bottom 爆炸比例 slider at 0.00.
5. **Explosion** (`装配页_爆炸.png`): raise the 爆炸比例 slider; volumes
   separate radially.
6. **Measurement** (`装配页_测量.png`): with ≥2 volumes + ratio ≈ 1.0, press
   `Ctrl+Y`; the right-side 测量 panel + overlay dimension lines appear.

When automated capture becomes available, runtime screenshots should be saved
to this directory mirroring the Phase 88 structure.
