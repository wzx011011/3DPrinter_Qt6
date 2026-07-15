# Roadmap: OWzx Slicer

## Milestones

- v4.6 Core Feature Completion Sweep ！ Phases 117-128 (shipped 2026-07-15)
- v4.7 Polish, i18n & Advanced Feature Recovery ！ Phases 129-135 (shipped 2026-07-15, tech_debt)
- v4.8 Dependency Unlock, Assembly Transform & i18n Completion ！ Phases 136-140 (in progress)

## Current Milestone: v4.8 Dependency Unlock, Assembly Transform & i18n Completion

**Goal:** Crack CGAL 5.6+ upgrade to unlock MeshBoolean + Drill; complete ASM-01 assembly transformation; fill en.ts remaining translations.

## Phases

- [ ] Phase 136: CGAL 5.6+ Dependency Upgrade (WS1)
- [ ] Phase 137: MeshBoolean + Drill Activation (WS1)
- [ ] Phase 138: Assembly Transformation Actions ASM-01 (WS2)
- [ ] Phase 139: en.ts Full Translation + Baseline Advance (WS3)
- [ ] Phase 140: v4.8 Verification And Cross-Workstream Regression

| Phase | Name | Goal | Requirements |
|---|---|---|---|
| 136 | CGAL 5.6+ Dependency Upgrade | Rebuild/download CGAL 5.6+ in DEPS_PREFIX + build links clean | CGAL-01 |
| 137 | MeshBoolean + Drill Activation | Flip flag + activate ~200 lines written logic | CGAL-02, CGAL-03 |
| 138 | Assembly Transformation Actions ASM-01 | Assembly-mode move/rotate/scale per-volume | ASM-01 |
| 139 | en.ts Full Translation + Baseline Advance | Fill remaining ~1372 translations + advance de/fr/ja/ko | I18N-04, I18N-05 |
| 140 | v4.8 Verification And Cross-Workstream Regression | Canonical build + ctest + regression-free | REGRESS-03 |

### Build Order

- **Wave A (parallel):** Phase 136 (CGAL upgrade) + Phase 138 (ASM-01) + Phase 139 (i18n). Independent.
- **Wave B (after 136):** Phase 137 (activate MeshBoolean + Drill).
- **Wave C (last):** Phase 140 (verification).

---

*v4.8 roadmap created: 2026-07-15 ！ 5 phases (136-140), 7 requirements.*
