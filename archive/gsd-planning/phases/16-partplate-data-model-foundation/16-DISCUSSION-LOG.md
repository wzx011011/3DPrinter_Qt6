# Phase 16: PartPlate Data Model Foundation - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-06-25
**Phase:** 16-PartPlate Data Model Foundation
**Areas discussed:** Class location & boundaries, Member relationship granularity, Per-plate config map form, Migration strategy

**Mode:** default (interactive, single-question turns)
**Prior context:** PROJECT.md (v3.0 milestone), REQUIREMENTS.md (PLATE-01/02/06), STATE.md, gap analysis audit (§1 data model, §6 3MF, §7 complexity). No prior CONTEXT.md files (v2.9 phases did not generate them).

---

## Gray Area Selection

| Option | Description | Selected |
|--------|-------------|----------|
| PartPlate 类的位置与边界 | Where the new class lives, naming, GL/wx field boundary | ✓ |
| 成员关系粒度 | object-level (QList<int>) vs instance-level (set<pair>) | ✓ |
| per-plate config map 的形态 | DynamicPrintConfig vs QVariant map vs dual-layer | ✓ |
| 迁移策略 | big-bang vs gradual bridge layer | ✓ |

**User's choice:** All four areas (full multi-select).
**Notes:** Phase 16 is the data-structure foundation; all four are genuinely interdependent (class location + migration strategy are coupled; granularity + config form must be locked now under big-bang).

---

## Class Location & Boundaries

| Option | Description | Selected |
|--------|-------------|----------|
| 新建 src/core/model/ | Pure domain objects (POD/value, no Qt signals); ProjectServiceMock adapts to Qt | ✓ |
| 并入 ProjectServiceMock | Struct inside the service; minimal change | |
| src/core/services/ 独立文件 | Separate file, same dir as services | |

**User's choice:** 新建 src/core/model/ (Recommended)
**Notes:** Separates domain logic from Qt wiring; enables independent unit testing; aligns with ARCHITECTURE.md "business logic in core/".

---

## Migration Strategy

| Option | Description | Selected |
|--------|-------------|----------|
| 大爆炸重构 | Delete 9 parallel QLists; PartPlateList as sole truth; rewrite ~30 Q_INVOKABLE in Phase 16 | ✓ |
| 桥接层(渐进) | PartPlateList as underlying truth + 9 QLists as derived cache; QML unchanged; migrate gradually | |

**User's choice:** 大爆炸重构 (Phase 16 one-shot)
**Notes:** User explicitly rejected the gradual bridge to avoid leaving temporary two-layer sync debt. Accepts larger single-phase change surface for a clean result.

---

## Member Relationship Granularity

| Option | Description | Selected |
|--------|-------------|----------|
| instance 级(完全对齐上游) | std::set<std::pair<int,int>> per upstream obj_to_instance_set | ✓ |
| object 级(先类化,instance 延后) | QList<int> as today, but encapsulated; instance deferred to v3.1+ | |
| 双层(同时存 object + instance) | Both QList<int> and QSet<QPair<int,int>>, kept in sync | |

**User's choice:** instance 级(完全对齐上游)
**Notes:** Complete source-truth alignment. Plate 16 changes are larger (plateObjectIndices API semantics redesign, load path instance parsing) but no "upgrade object→instance later" debt. Consistent with the big-bang migration choice.

---

## Per-Plate Config Map Form

| Option | Description | Selected |
|--------|-------------|----------|
| DynamicPrintConfig 原生(对齐上游) | Native Slic3r::DynamicPrintConfig m_config; removes QVariant map | ✓ |
| QVariant map(Qt 友好) | QHash<QString,QVariant>; convert to DynamicPrintConfig at slice time | |
| 双层 | DynamicPrintConfig (truth) + QVariant view (derived) | |

**User's choice:** DynamicPrintConfig 原生(对齐上游)
**Notes:** Makes Phase 18 3MF persistence native and Phase 19 slice merge a native apply(). ProjectServiceMock provides a QVariant-string adaptation view for QML, but config truth is DynamicPrintConfig. Fixes the `return false` stub at ProjectServiceMock.cpp:1336.

---

## Claude's Discretion

- Exact PartPlate field naming (mirror upstream m_ prefix or establish new src/core/model/ idiom).
- Whether instance_outside_set lands in Phase 16 or 17 (obj_to_instance_set is required now).
- PartPlateList internal container structure.
- How plateObjectIndices Q_INVOKABLE adapts to instance-level truth while preserving QML behavior.

## Decision Pattern Observed

All four decisions point the same direction: **complete source-truth alignment, no half-measures, no deferred structural debt.** This is a high-risk-tolerance, high-correctness-preference signal. CONTEXT.md records this as the guiding posture for the planner.

## Deferred Ideas

None — discussion stayed within Phase 16 scope. Adjacent work (clone/duplicate/reorder, 3MF write path, slice refactor, AssembleView) correctly belongs to Phases 17-19 and v3.1.
