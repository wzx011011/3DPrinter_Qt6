# Roadmap: Milestone v1.2 — Slicing Pipeline Hardening

## Overview

Fix confirmed bugs in the core model loading and multi-plate pipeline, add auto-arrange on load, and ensure the full load → arrange → config → slice → export → preview chain is robust. The slice engine itself works end-to-end; gaps are in plate management, arrange, and project config propagation.

## Phases

- [ ] **Phase 1: Multi-Plate Loading Fixes** — Fix loadProject() plate data bug, reset plate metadata on new load, add auto-arrange on load
- [ ] **Phase 2: Project Config & Arrange Hardening** — Propagate 3MF embedded config to UI, use real bed shape for arrange
- [ ] **Phase 3: Pipeline Verification** — End-to-end verification with real multi-plate 3MF files, fix any remaining gaps

## Phase Details

### Phase 1: Multi-Plate Loading Fixes
**Mode:** MVP
**Goal:** Multi-plate 3MF files load correctly, plate metadata is clean, models auto-arrange after load.
**Depends on:** Nothing (builds on v1.1 foundation)
**Requirements:** PIPE-01
**Success Criteria:**
  1. loadProject() extracts plate data BEFORE releasing it (matching loadFile() pattern)
  2. Plate metadata arrays reset when loading a new model/project
  3. Auto-arrange runs after loadFile() and loadProject() complete
  4. Multi-plate 3MF (e.g., 城楼.3mf) loads with correct plate count and object distribution
  5. Loading a new model after a previous one doesn't carry stale plate settings
**Plans:** 1 plan
Plans:
- [ ] v12-01-01-PLAN.md — Fix plate data extraction, reset metadata, add auto-arrange ✅ WRITTEN

### Phase 2: Project Config & Arrange Hardening
**Mode:** MVP
**Goal:** Opening a saved 3MF project restores its settings; arrange uses real bed boundaries.
**Depends on:** Phase 1
**Requirements:** PIPE-02
**Success Criteria:**
  1. loadProject() propagates embedded DynamicPrintConfig to ConfigViewModel
  2. arrangeObjects() uses real bed shape from printer preset instead of InfiniteBed
  3. Opening a previously saved project shows the correct preset values
**Plans:** 1 plan
Plans:
- [ ] v12-02-01-PLAN.md — Propagate project config, use real bed for arrange ✅ WRITTEN

### Phase 3: Pipeline Verification
**Mode:** MVP
**Goal:** Full pipeline verified with real files — no crashes, no data loss, no stale state.
**Depends on:** Phase 2
**Requirements:** PIPE-03
**Success Criteria:**
  1. E2E smoke test: load 城楼.3mf → verify plates → arrange → config → slice → export → preview
  2. STL load → single plate → arrange → slice → export → preview
  3. Load project A, then load project B — no stale state from A
  4. All existing E2EWorkflowTests and ViewModelSmokeTests still pass
**Plans:** 1 plan
Plans:
- [ ] v12-03-01-PLAN.md — E2E pipeline verification with real files ✅ WRITTEN

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Multi-Plate Loading Fixes | 0/1 | Not started | — |
| 2. Project Config & Arrange | 0/1 | Not started | — |
| 3. Pipeline Verification | 0/1 | Not started | — |
