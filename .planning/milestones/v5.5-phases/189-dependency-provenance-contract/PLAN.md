# Phase 189 Plan: Dependency Provenance Contract

**Requirement:** PROV-01
**Goal:** Document the exact source of Qt, upstream OrcaSlicer source, and the
Orca dependency bundle in both CI and local development.

## Files

- Create: `.planning/milestones/v5.5-phases/189-dependency-provenance-contract/PROVENANCE.md`
- Reference: `.github/workflows/tag-build.yml`
- Reference: `scripts/auto_verify_with_vcvars.ps1`

## Steps

- [x] Read `.github/workflows/tag-build.yml` and record the Qt action settings:
  `jurplel/install-qt-action@v4`, version `6.10.0`, target `desktop`,
  arch `win64_msvc2022_64`, module `qtshadertools`.
- [x] Read `scripts/auto_verify_with_vcvars.ps1` and record local Qt candidate
  order, upstream source candidate order, and deps prefix candidate order.
- [x] Write `PROVENANCE.md` with these sections:
  `Qt`, `OrcaSlicer upstream source`, `Orca dependency bundle`, `What v5.5 does not do`.
- [x] State explicitly: Qt is downloaded/prebuilt, not compiled from source by
  this project.
- [x] Verify by running:

```powershell
rg -n "install-qt-action|6\.10\.0|win64_msvc2022_64|\\.deps\\Qt6\.10|D:\\work\\OrcaSlicer|prebuilt|not compiled from source" .planning\milestones\v5.5-phases\189-dependency-provenance-contract\PROVENANCE.md
```

Expected: all provenance anchors are present in the new document.
