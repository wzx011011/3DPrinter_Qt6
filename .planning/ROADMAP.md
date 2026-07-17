# Roadmap: OWzx Slicer

## Milestones

- ✅ **v2.9** Implementation Realignment and Stabilization — Phases 10-15 (shipped 2026-06-25)
- ✅ **v3.0** PartPlate Core — Phases 16-22 (shipped 2026-06-26)
- ✅ **v3.1** QRhi Rendering — Phases 23-28 (shipped 2026-06-28)
- ✅ **v3.2** Multi-Plate Data Polish — Phases 29-32 (audited 2026-06-28)
- ✅ **v3.3** Slice Preview Main Flow MVP — Phases 33-36 (superseded by v3.4)
- ✅ **v3.4** Import to G-code Complete Workflow — Phases 37-43 (closed by automated E2E)
- ✅ **v3.5** Preset Authoring Complete Workflow — Phases 44-49 (superseded after Phase 46)
- ✅ **v3.6** Screenshot-Driven OrcaSlicer UI Restoration — Phases 50-58 (shipped 2026-07-03)
- ✅ **v3.7** Screenshot-Level UI Parity Closure — Phases 59-64 (2026-07-04)
- ✅ **v3.8** RHI Gizmo Parity — Phases 65-73 (shipped 2026-07-04)
- ✅ **v3.9** Prepare Page UI Restoration — Phases 74-78 (shipped 2026-07-06)
- ✅ **v4.0** Preview Page UI Restoration — Phases 79-83 (shipped 2026-07-07)
- ✅ **v4.1** Parameter Settings Dialogs Source-Truth Restoration — Phases 84-88 (shipped 2026-07-09)
- ✅ **v4.2** AssembleView Source-Truth Restoration — Phases 89-93 (shipped 2026-07-09)
- ✅ **v4.3** Real Thumbnail Capture And 3MF Round-Trip — Phases 94-98 (shipped 2026-07-10)
- ✅ **v4.4** Wipe-Tower Geometry Readback And Real Rendering — Phases 99-102 (shipped 2026-07-12)
- ✅ **v4.5** Backlog Closure — Phases 103-116 (shipped 2026-07-13)
- ✅ **v4.6** Core Feature Completion Sweep — Phases 117-128 (shipped 2026-07-15)
- ✅ **v4.7** Polish, i18n & Advanced Feature Recovery — Phases 129-135 (shipped 2026-07-15, tech_debt)
- ✅ **v4.8** Dependency Unlock, Assembly Transform & i18n Completion — Phases 136-140 (shipped 2026-07-16, tech_debt)
- ✅ **v5.0** Advanced Feature Recovery & Tech-Debt Closure — Phases 141-153 (shipped 2026-07-17, tech_debt)

## Next Milestone

Not started. After v5.0 shipped, run `/gsd:new-milestone` to define the next cycle.

**Candidate backlog (post-v5.0):**
- **v5.1+ SLA sub-milestone** (highest-value follow-up): wire `SLAPrint` into SliceService (currently FFF-only); bundle SLA presets; implement `.png` layer output; close VDB-06 (hollowEnabled produces hollowed G-code); unblock SlaSupports + FaceDetector gizmos. Scope: ~35 files in `libslic3r/SLA/` + `SLAPrint.cpp` 1261 lines + SLAPrintSteps + GLGizmoSlaSupports + SLA preset bundle.
- de/fr/ja/ko translation long tail (~906 messages/lang remaining after v4.8 I18N-05).
- Calibration `.drc` tower geometry (v4.6 CALIB tech debt).
- Emboss follow-ups: 3MF text-metadata persistence (`TextConfigurationSerialization`); style controls (boldness/variable-font axes); Emboss SVG advanced features.
- Preset diff view (QML consumer for the Phase 149 comparePresets primitive).
- PartPlate runtime thumbnail capture scheduler (PLATE-05 refined scope).
- Live multi-plate round-trip ctest (PLATE-06 — needs a PresetServiceMock/ProjectServiceMock test fixture).

**Archives:** `.planning/milestones/v{version}-ROADMAP.md`, `v{version}-REQUIREMENTS.md`, `v{version}-MILESTONE-AUDIT.md`, `v{version}-phases/`.

---

*Last updated: 2026-07-17 — v5.0 shipped (tech_debt). 13 phases (141-153), 32 requirements (31 satisfied + VDB-06 deferred to v5.1+ SLA sub-milestone). See `.planning/milestones/v5.0-ROADMAP.md` for the full v5.0 roadmap and `.planning/v5.0-MILESTONE-AUDIT.md` for the audit.*
