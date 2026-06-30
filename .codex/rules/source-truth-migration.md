# Source-Truth Migration Rules

This repository is an OrcaSlicer-to-Qt6/QML migration project for OWzx Slicer.

## Source Truth

- Active upstream behavior source: `third_party/OrcaSlicer`.
- Qt6/QML code must match upstream user-visible behavior and workflow semantics.
- Do not invent product behavior unless it is explicitly documented as an OWzx-only decision.
- Historical CrealityPrint-era documents are evidence only. New source-truth work must cite OrcaSlicer upstream paths unless the task is explicitly about legacy compatibility cleanup.

## Screenshot And Source Dual Truth

- When a milestone is screenshot-driven, screenshots under `shotScreen/` define visual layout truth, while OrcaSlicer source defines behavior truth.
- Screenshot-visible controls are not complete until their upstream behavior, Qt target surface, status, and verification path are mapped.
- Every restored module must record the screenshot region, upstream source files, Qt/QML targets, required behavior list, remaining gaps, and verification method.
- If screenshot evidence and upstream behavior appear to disagree, stop and classify the gap before implementing product behavior.

## Completeness Over Legacy Compatibility

- Future milestones must implement the complete target behavior for their declared scope, not an MVP or partial compatibility layer unless the user explicitly asks for an MVP.
- If an existing Qt implementation is simplified, mock, legacy, or semantically wrong for the milestone target, replace or rewrite it instead of preserving the old behavior for compatibility.
- Keep old behavior only when it is still source-truth-correct, or when it is explicitly documented as a temporary fallback with a status classification, removal condition, and follow-up owner.
- Do not build parallel old/new logic paths merely to avoid changing callers. Update callers to the correct source-truth contract.
- If a page or component is materially off-design and not worth repairing, create a replacement and remove the old files, registrations, routes, tests, imports, and resource entries in the same milestone.
- Do not keep `legacy`, `old`, `unused`, `deprecated`, or disconnected UI code paths after a replacement lands.
- Tests should lock the intended complete behavior, including rejecting the old incorrect behavior when that behavior previously existed.

## Implementation Boundaries

- Preserve libslic3r slicing behavior. Do not modify slicing algorithms as part of GUI migration work unless a task explicitly authorizes it.
- Business logic, validation, persistence, and upstream behavior mapping belong in C++ services/viewmodels under `src/core/`.
- QML is for presentation, layout, state binding, and interaction wiring.
- If QML logic becomes durable business behavior, move it into C++ or document it as migration debt.

## Text Encoding And Comments

- New or modified source comments must be English and ASCII-only.
- Do not add Chinese text to comments, planning-facing source annotations, TODOs, or inline migration notes.
- User-visible Chinese belongs in `qsTr()` strings and translation files, not in comments.
- When touching a file that already contains mojibake comments, replace only the comments whose intent is clear from code or source-truth context; do not guess lost text.
- Preserve UTF-8 without BOM for all text files.

## Upstream Mapping

For each migrated workflow:

1. Identify the relevant upstream OrcaSlicer GUI or libslic3r files.
2. Identify the Qt6 target service/viewmodel/QML surface.
3. Implement the smallest verifiable source-truth slice.
4. Record remaining gaps as Real, Hybrid, Mock, Blocked, or Placeholder.
5. Verify before marking the requirement complete.

## Status Terms

- `Real`: source-truth behavior is implemented and verified.
- `Hybrid`: a real path exists, but fallback/mock behavior remains or verification is incomplete.
- `Mock`: local simulation only.
- `Blocked`: unavailable dependency, credential, protocol, or product decision.
- `Placeholder`: visible UI or enum exists without meaningful backend behavior.

Do not mark a workflow complete merely because a UI surface, class, or enum exists.

## Required Verification

- Use the canonical build command from `.Codex/rules/build-rules.md` for full verification.
- Lightweight text checks are acceptable for documentation-only phases.
- When a test is built but not run, state that explicitly.
- When a workflow needs live hardware, credentials, or unavailable dependencies, document the manual verification requirement separately from automated regression coverage.

## Prohibited Patterns

- Do not add new mock behavior to hide missing upstream behavior.
- Do not treat broad page coverage as workflow completion.
- Do not bypass upstream source reading for user-visible behavior.
- Do not modify unrelated dirty files while executing a focused phase.
- Do not create alternate build directories or non-canonical build scripts.
