---
status: resolved
trigger: "ViewModelSmokeTests.exe crashes in testTabPositionEnumValues while constructing BackendContext: access violation 0xc0000005 in QtPrivate::QStringList_contains -> PresetServiceMock::loadSingleVendor line 391 m_loadedVendors.contains(vendorKey), after loadUpstreamSchemaDefaults."
created: 2026-08-01T11:51:42+08:00
updated: 2026-08-02T22:50:43+08:00
---

## Current Focus
<!-- OVERWRITE on each update - reflects NOW -->

hypothesis: "Confirmed and resolved: a stale BackendContext.cpp object allocated the pre-WIZ-03 PresetServiceMock layout, and a clean canonical rebuild restored a coherent object layout and dependency database."
test: "Run the complete canonical verifier after rebuilding all targets, including the original focused test and the full ViewModelSmokeTests suite."
expecting: "BackendContext construction and all regression suites remain green."
next_action: "None for this incident; re-open only if Ninja dependency records regress to zero after a header change."

## Symptoms
<!-- Written during gathering, then IMMUTABLE -->

expected: "ViewModelSmokeTests.exe constructs BackendContext and testTabPositionEnumValues runs without crashing."
actual: "After the canonical build, ViewModelSmokeTests.exe crashes while BackendContext is being constructed."
errors: "Access violation 0xc0000005 in QtPrivate::QStringList_contains -> PresetServiceMock::loadSingleVendor line 391 at m_loadedVendors.contains(vendorKey)."
reproduction: "Run build\\ViewModelSmokeTests.exe after the canonical build; testTabPositionEnumValues crashes during BackendContext construction after loadUpstreamSchemaDefaults."
started: "Current reproducible failure; timeline before the supplied report is unknown."

## Eliminated
<!-- APPEND only - prevents re-investigating -->

## Evidence
<!-- APPEND only - facts discovered -->

- timestamp: 2026-08-01T11:51:42+08:00
  checked: "User-supplied reproduction and debugger trace"
  found: "The fault occurs inside QStringList::contains at PresetServiceMock::loadSingleVendor line 391 during BackendContext construction."
  implication: "Investigate object lifetime, initialization order, and preceding writes to m_loadedVendors before changing behavior."

- timestamp: 2026-08-01T11:53:00+08:00
  checked: ".planning/debug/knowledge-base.md"
  found: "No knowledge base exists, so there is no prior matching resolved pattern."
  implication: "The investigation starts from the observed code path rather than a historical candidate."

- timestamp: 2026-08-01T11:53:00+08:00
  checked: "Repository file inventory"
  found: "The active implementation is src/core/services/PresetServiceMock.cpp and .h; ViewModelSmokeTests.cpp is the focused regression-test surface."
  implication: "The failure can be isolated within the assigned ownership boundary."

- timestamp: 2026-08-01T11:55:00+08:00
  checked: "PresetServiceMock constructor and member declarations"
  found: "m_loadedVendors is a value-member QStringList declared before the constructor body executes; the constructor calls loadUpstreamSchemaDefaults immediately before loadVendorPresets."
  implication: "Normal member initialization order cannot itself leave m_loadedVendors unconstructed; investigate stale caller layout and preceding memory corruption."

- timestamp: 2026-08-01T11:57:00+08:00
  checked: "loadSingleVendor and loadUpstreamSchemaDefaults implementations"
  found: "loadUpstreamSchemaDefaults mutates m_presetStore only; loadSingleVendor's first use of m_loadedVendors is the reported line 391."
  implication: "The stack timing fits an object-layout overrun that becomes visible at the first access to the newly added member."

- timestamp: 2026-08-01T11:57:00+08:00
  checked: "Build artifact timestamps"
  found: "PresetServiceMock.cpp.obj and ViewModelSmokeTests.exe were written on 2026-08-01, while BackendContext.cpp.obj remains from 2026-07-25 11:30, before the WIZ-03 commit at 2026-07-25 20:18 that added m_loadedVendors."
  implication: "A stale caller translation unit is a concrete, high-probability ABI mismatch candidate."

- timestamp: 2026-08-01T11:59:00+08:00
  checked: "BackendContext construction and Ninja dependency database"
  found: "BackendContext.cpp calls new PresetServiceMock(this); Ninja reports zero recorded dependencies for BackendContext.cpp.obj."
  implication: "The stale object allocates the old PresetServiceMock size and Ninja cannot notice that the header changed."

- timestamp: 2026-08-01T11:59:00+08:00
  checked: "Focused reproduction: build\\ViewModelSmokeTests.exe testTabPositionEnumValues"
  found: "The test exits with -1073740940 after StaticPrintConfigs initialization, reproducing a native heap failure during BackendContext construction."
  implication: "The failure is deterministic in the assigned regression surface and occurs before test assertions."

- timestamp: 2026-08-01T12:02:00+08:00
  checked: "build/CMakeFiles/rules.ninja and build/build.ninja"
  found: "The CXX rules use deps = msvc and /showIncludes, but msvc_deps_prefix is localized Chinese-language output; BackendContext.cpp.obj has no implicit header dependencies."
  implication: "On this locale, header edits do not invalidate dependent objects, allowing the PresetServiceMock ABI mismatch to persist."

- timestamp: 2026-08-01T12:19:00+08:00
  checked: "Canonical verifier rebuild progress"
  found: "BackendContext.cpp.obj and ViewModelSmokeTests.exe have been regenerated after refreshing only BackendContext.cpp's timestamp; the file hash before and after the timestamp refresh was identical."
  implication: "The next focused test is an isolated counterfactual for stale object layout, not a behavior change."

- timestamp: 2026-08-01T12:21:00+08:00
  checked: "Exact focused reproduction after stale-object rebuild: build\\ViewModelSmokeTests.exe testTabPositionEnumValues"
  found: "The same test now exits 0 after the single caller-object rebuild; before it, the identical command exited -1073740940."
  implication: "The stale-layout hypothesis is confirmed by a one-variable counterfactual."

- timestamp: 2026-08-01T12:24:00+08:00
  checked: "Repeated focused test and canonical verifier smoke stage"
  found: "Three consecutive focused runs exited 0. The canonical command rebuilt all requested targets and passed PrepareSceneDataTests, PartPlateTests, and ObjectPickingTests; ViewModelSmokeTests reached configEnumNullKeysMapGuards and then crashed with 0xc0000005, after testTabPositionEnumValues had already passed in the rebuilt binary."
  implication: "The reported PresetServiceMock crash is resolved by coherent recompilation; the full canonical gate remains blocked by a separate existing ViewModelSmokeTests failure."

- timestamp: 2026-08-01T12:32:00+08:00
  checked: "Focused canonical blocker: build\\ViewModelSmokeTests.exe configEnumNullKeysMapGuards"
  found: "The separate test crashes immediately at Slic3r::ConfigOptionEnumGeneric scalar(nullptr, 17), before any PresetServiceMock construction or assertion."
  implication: "This is an independent upstream/config-enum compatibility issue and not evidence against the stale PresetServiceMock layout diagnosis."

## Resolution
<!-- OVERWRITE as understanding evolves -->

root_cause: "The WIZ-03 header added value member m_loadedVendors to PresetServiceMock, but localized Chinese-language MSVC /showIncludes output did not match Ninja's dependency parser. BackendContext.cpp.obj therefore remained from before the header change and its old operator-new size under-allocated PresetServiceMock; the new constructor's m_loadedVendors.contains() accessed beyond the allocation."
fix: "No PresetServiceMock or ViewModelSmokeTests source change is safe or necessary. Refreshing BackendContext.cpp's timestamp and running the canonical verifier rebuilt the stale caller object (source bytes unchanged). A permanent fix belongs in build dependency handling (localized msvc_deps_prefix) or a clean rebuild policy."
verification: "Before rebuild, build\\ViewModelSmokeTests.exe testTabPositionEnumValues exited -1073740940. After rebuilding the stale caller object, the same test exited 0 three consecutive times. On 2026-08-02 the complete canonical command passed PrepareSceneDataTests, PartPlateTests, ObjectPickingTests, ViewModelSmokeTests, QmlUiAuditTests, ViewportContextMenuTests, PreviewParserTests, all E2E pipeline tests, and the OWzxSlicer launch-liveness check. The independent null enum-map crash was fixed and its focused regression test also passes."
files_changed: []
