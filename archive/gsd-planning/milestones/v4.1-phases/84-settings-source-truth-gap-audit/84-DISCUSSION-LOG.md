# Phase 84: Settings Source-Truth Gap Audit - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md; this log preserves the alternatives considered.

**Date:** 2026-07-07
**Phase:** 84-Settings Source-Truth Gap Audit
**Mode:** `--auto`
**Areas discussed:** Audit artifact shape, Phase 56 residual reconciliation, visible settings layout classification, verification routing, scope guard

---

## Audit Artifact Shape

| Option | Description | Selected |
|---|---|---|
| Canonical gap matrix | Produce `84-GAP-MATRIX.md` with region IDs, target observation, current evidence, Qt targets, upstream source, decision, severity, owner phase, requirement mapping, and verification. | Yes |
| High-level notes only | Capture a short narrative without a row-by-row region map. | |
| Skip audit and start code changes | Move straight into settings QML changes without freezing source-truth mapping. | |

**User's choice:** Auto-selected recommended default.
**Notes:** This follows the Phase 79 Preview audit pattern and satisfies `SETAUDIT-01`.

---

## Phase 56 Residual Reconciliation

| Option | Description | Selected |
|---|---|---|
| Preserve semantics and reopen visual gaps | Keep Phase 56 backend semantics while reopening deferred visual/runtime evidence as v4.1 work. | Yes |
| Replace all settings implementation | Discard the existing settings backend and rebuild from scratch. | |
| Accept Phase 56 UI as complete | Treat existing settings UI as visually complete despite screenshots and deferred evidence. | |

**User's choice:** Auto-selected recommended default.
**Notes:** Phase 56 passed semantic tests but explicitly deferred visual parity, typed-control rendering visuals, and non-modal live-edit evidence.

---

## Visible Settings Layout Classification

| Option | Description | Selected |
|---|---|---|
| Screenshot-first restoration | Audit current QML for screenshot parity, preserve proven C++ semantics, and remove/hide off-design visible surfaces such as left group navigation unless source truth requires them. | Yes |
| Keep current left group navigation | Treat current `GroupNavSidebar.qml` as target-visible layout. | |
| Invent a new OWzx settings design | Use a new design not mapped to OrcaSlicer screenshots/source. | |

**User's choice:** Auto-selected recommended default.
**Notes:** The printer and material screenshots show no visible left group sidebar. This conflicts with older Phase 56 assumptions and must be reconciled in Phase 84.

---

## Verification Routing

| Option | Description | Selected |
|---|---|---|
| Docs/source audit now, final visual evidence later | Phase 84 verifies the gap matrix and planning docs; Phase 88 runs canonical verifier, app launch, and visual evidence. | Yes |
| Full build and launch in Phase 84 | Run the canonical build during the audit phase despite no production source changes. | |
| Manual notes only | Skip automated/source checks for the audit artifact. | |

**User's choice:** Auto-selected recommended default.
**Notes:** This keeps Phase 84 lightweight and reserves runtime proof for the final verification phase.

---

## Scope Guard

| Option | Description | Selected |
|---|---|---|
| Local/offline settings only | Keep v4.1 focused on printer/material/process settings dialogs and existing semantic preservation. | Yes |
| Include removed device/network work | Reopen LAN/device/cloud/Monitor/network scope. | |
| Include renderer/backend promotion | Pull D3D12/Vulkan/backend promotion into settings restoration. | |

**User's choice:** Auto-selected recommended default.
**Notes:** User direction removed LAN/device/network work from forward scope. D3D12 remains a future investigation.

---

## the agent's Discretion

- The planner may choose the exact `84-GAP-MATRIX.md` table format, provided it maps regions to source, Qt targets, decisions, requirements, owners, and verification.
- The planner may decide whether each current QML region is modify vs replace after source reads, while preserving verified C++ settings semantics.
- The planner may add lightweight static audit checks for Phase 84.

## Deferred Ideas

- AssembleView remains a future source-truth milestone.
- D3D12 root-cause and Vulkan evaluation remain future backend work.
- Full upstream preset-bundle import/export compatibility remains outside v4.1 unless explicitly reopened.
- Network/device/cloud/Monitor/ModelMall/live-camera workflows remain removed from forward scope.
