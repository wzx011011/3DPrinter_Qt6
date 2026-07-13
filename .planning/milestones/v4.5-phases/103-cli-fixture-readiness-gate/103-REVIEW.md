---
phase: 103-cli-fixture-readiness-gate
review: 103-REVIEW
base_commit: 92fda78
head: 8060f17
status: clean
files_reviewed:
  - src/qml_gui/main_qml.cpp
  - tests/QmlUiAuditTests.cpp
counts:
  critical: 0
  warning: 0
  info: 3
  total: 3
---

# Phase 103 Code Review

Scope: diff `92fda78..HEAD` (3 commits) on `main_qml.cpp` + `QmlUiAuditTests.cpp`.
Replaces the `QTimer::singleShot(0, &backend, ...)` argv-fixture gate with a
deterministic one-shot wait on `QQuickWindow::frameSwapped`, plus a source-audit
regression slot and a one-token swap in the existing deep-link audit.

## Findings

| ID | Severity | File | Line(s) | Summary |
|----|----------|------|---------|---------|
| F-01 | info | src/qml_gui/main_qml.cpp | 367-375 | Theoretical leak of the `new QMetaObject::Connection` if `frameSwapped` never fires before process exit. Unreachable in the fixture scenario and consistent with the project's documented intentional-leak-at-exit policy (engine leak at :311). Non-blocking. |
| F-02 | info | tests/QmlUiAuditTests.cpp | 3658, 3671 | Two of the five FIXTURE-02 assertions use generic tokens (`singleShot(0`, `QObject::disconnect`). Acceptable for a regression lock per the plan, but weaker than fixture-specific anchors (e.g. the `frameSwapped` + `applyStartupOpenRequests` + `degraded mode` tokens, which are specific). Non-blocking. |
| F-03 | info | src/qml_gui/main_qml.cpp | 213-257 | `applyStartupOpenRequests` lost its empty-request early-return (now lives only at the call-site `if` at :360-362). Harmless — empty request iterates zero-length loops — but the function is now safe to call with an empty request only by coincidence, not by guard. Non-blocking. |

## Detail

### F-01 — One-shot Connection lifetime (info)

The gate allocates `auto *frameGateConnection = new QMetaObject::Connection;`
and frees it inside the lambda (`disconnect + delete`). For the normal path
this is correct and fires exactly once:

- `frameSwapped` is emitted every frame; the handler disconnects on the first
  emission, so the apply runs exactly once (double-fire impossible).
- `Qt::QueuedConnection` defers delivery to the event loop, so the handler
  cannot re-enter during its own execution (no re-entrancy).
- `backend` (stack local in `main()`, :307) outlives the connection; the
  receiver context is `rootWindow`, owned by the intentionally-leaked engine.
- `startupOpenRequest` is captured **by value** — no dangling reference.

The one residual path: if `frameSwapped` never fires before process exit (root
window never renders), the `new`'d `Connection` is not QObject-parented, so Qt
severs the link on `rootWindow` destruction but does not `delete` the object.
This is unreachable for the fixture scenario (the root ApplicationWindow is
visible and renders) and is consistent with the project's stated
intentional-leak-at-exit policy (engine at :309-311). No action required; noted
for completeness.

### F-02 — Regression-lock token specificity (info)

The new `argvFixtureGateUsesFrameSwappedNotSingleShot` slot's 5 assertions:
three are fixture-specific (`frameSwapped`, `applyStartupOpenRequests`,
`degraded mode`) and robustly anchor the contract. Two are generic
(`!singleShot(0`, `QObject::disconnect`): `QObject::disconnect` in particular
could appear in unrelated future code. This matches the Phase 102
source-audit pattern and the plan explicitly accepts the brittleness tradeoff
("acceptable for a regression lock"). Non-blocking; the specific tokens carry
the real weight.

### F-03 — Empty-request guard relocation (info)

Pre-change, `applyStartupOpenRequests` opened with
`if (page.isEmpty() && dialogs.isEmpty() && modelPaths.isEmpty()) return;`.
Post-change the guard moved to the call site (`if (!page.isEmpty() || ...)`
at :360-362), so `applyStartupOpenRequests` is only ever called with work to
do. The function itself is now harmless when called with an empty request
(the `!page.isEmpty()` guard at :216 and zero-length `for` loops handle it),
but only by coincidence rather than by explicit guard. Acceptable given the
single call site; noted for future maintainers.

## Verified

- `git diff --check` exits 0 (no whitespace errors).
- No residual `QTimer` / `singleShot` / `<QTimer>` in main_qml.cpp (grep exit 1).
- Token swap in `guiStartupDeepLinkArgumentsAreExtensible` is isolated: exactly
  one `-`/`+` pair in the requiredTokens list; the other 17 tokens unchanged and
  all still resolve in main_qml.cpp.
- `&QQuickWindow::frameSwapped` resolves at main_qml.cpp:369; `degraded mode`
  resolves at :379 (the new slot's assertions pass).
- All new/changed comments are English ASCII (pre-existing non-ASCII comments
  are outside the changed regions).
- No business logic added to QML; changes confined to C++ main + test file.

## Conclusion

Status: **clean**. No critical or warning findings. The gate semantics
(one-shot, first-frame, GUI-thread, QueuedConnection), lifetime safety
(value-captured request, reference-captured backend that outlives the
connection), defensive fallback (apply-immediate + warning, no hang), and
regression test (5 named assertions + correct isolated token swap) are all
sound. The three INFO items are non-blocking observations. Recommend proceed.

Full report: `.planning/phases/103-cli-fixture-readiness-gate/103-REVIEW.md`
