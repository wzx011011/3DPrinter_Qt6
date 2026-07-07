# Phase 85: Settings Shell And Tab Layout Restoration - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md; this log preserves the alternatives considered.

**Date:** 2026-07-07
**Phase:** 85-settings-shell-and-tab-layout-restoration
**Areas discussed:** window shell, top action row, tabs and labels, entry points, verification routing

---

## Window Shell

| Option | Description | Selected |
|--------|-------------|----------|
| Screenshot shell | Keep independent 736x593 non-modal `ApplicationWindow`, clean native title, no duplicate in-row close. | yes |
| Free redesign | Allow a new OWzx-specific shell. | no |
| Preserve current row close | Keep the current top-row manual close button. | no |

**Auto choice:** Screenshot shell.
**Notes:** Both target screenshots are 736x593 independent dark windows with a native title/close area and no embedded page chrome.

---

## Top Action Row

| Option | Description | Selected |
|--------|-------------|----------|
| Compact icon row | Preset combo on the left, compact action/search/mode icons on the right, semantics preserved. | yes |
| Current text row | Keep permanent search field plus text `Save` and `Save As...` buttons. | no |
| Defer row visuals | Leave row visual restoration to a later phase. | no |

**Auto choice:** Compact icon row.
**Notes:** Phase 85 owns `SET-PRESET-ACTIONS`; Phase 87 owns deeper semantic hardening.

---

## Tabs And Labels

| Option | Description | Selected |
|--------|-------------|----------|
| Screenshot labels | Use exact printer/material tab labels and order from screenshots; process uses source parity. | yes |
| Existing labels | Keep current labels and rely on later cleanup. | no |
| Backend key rewrite | Rewrite tab keys broadly. | no |

**Auto choice:** Screenshot labels.
**Notes:** Keep stable filtering keys where possible. Clean visible labels are required for `SETLAYOUT-02` and `SETLAYOUT-03`.

---

## Entry Points

| Option | Description | Selected |
|--------|-------------|----------|
| Preserve existing routing | Keep `forwardSettingsRequest` and the three `SettingsDialog` instances. | yes |
| Replace routing | Add a new settings router or page. | no |
| Broaden to preferences | Treat topbar preferences as the same workflow. | no |

**Auto choice:** Preserve existing routing.
**Notes:** Phase 85 should not expand into general preferences or device/network settings.

---

## Verification Routing

| Option | Description | Selected |
|--------|-------------|----------|
| Source/QML audits now, screenshots later | Add static coverage for shell labels/actions/routing; final runtime evidence remains Phase 88. | yes |
| Runtime screenshots now | Require full app visual capture in Phase 85. | no |
| No new audits | Rely on manual review only. | no |

**Auto choice:** Source/QML audits now, screenshots later.
**Notes:** Phase 88 owns canonical verifier, launch, and final visual evidence after Phase 85-87 land.

## the agent's Discretion

- Exact icon assets and dimensions are left to implementation after inspecting available QML assets.
- The planner may patch `SettingsDialog.qml` in place or recompose it if that produces a cleaner, source-truth-aligned shell.

## Deferred Ideas

- Option rows and section visuals: Phase 86.
- Preset workflow hardening: Phase 87.
- Cleanup, canonical build, runtime launch, and final screenshot evidence: Phase 88.
