# Phase 2: E2E Workflow Verification - Research

**Researched:** 2026-06-01
**Domain:** End-to-end slice pipeline verification (model load -> slice -> G-code export -> preview switch -> G-code parse)
**Confidence:** HIGH

## Summary

This phase verifies that the complete data flow chain from model import through slicing to G-code preview works correctly end-to-end. The pipeline components are all REAL (not mock): model loading via libslic3r `read_from_archive`, slicing via `Slic3r::Print::process()`, G-code export via `print.export_gcode()`, and G-code parsing in `PreviewViewModel::rebuildFromGCode()`.

**Primary recommendation:** Focus verification on six concrete handoff points where data crosses component boundaries: (1) ProjectServiceMock -> SliceService (model clone + source path), (2) SliceService internal (config -> Print::apply), (3) SliceService -> PreviewViewModel (output path), (4) EditorViewModel -> QML (previewRequested signal), (5) QML -> BackendContext (page switch), and (6) PreviewViewModel -> GCodeRenderer (binary preview data). Each handoff point has a known gap or risk that must be verified or fixed.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Model loading & plate management | API / Backend (ProjectServiceMock) | -- | Owns Slic3r::Model*, plate assignment, mesh snapshots |
| Slice execution | API / Backend (SliceService) | -- | Owns Slic3r::Print, slicing thread, G-code export |
| Slice status bridging | ViewModel (EditorViewModel) | -- | Delegates to SliceService, forwards progress/results to QML |
| G-code parsing & preview data | ViewModel (PreviewViewModel) | -- | Parses raw G-code file, builds binary segment buffer |
| Page navigation | Browser / Client (QML) | Frontend Server (BackendContext) | QML triggers page switch via backend.setCurrentPage() |
| Config application to slice | API / Backend (SliceService) | -- | Applies DynamicPrintConfig to Print::apply() |
| Notification routing | Frontend Server (BackendContext) | -- | Routes slice progress/completion to QML notification system |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| libslic3r | v7.0.1 (upstream) | Slice engine, model I/O, G-code export | Upstream source-truth, compiled from source |
| Qt 6.10 | 6.10 | QML framework, signal/slot, concurrent | Project standard |
| Qt Test | 6.10 | Unit/smoke test framework | Existing test infrastructure |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| QtConcurrent | 6.10 | Background slice execution | SliceService::startSlice() runs slice in thread pool |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Manual G-code text parsing in PreviewViewModel | libslic3r GCodeProcessor | libslic3r parser is more complete but adds coupling; text parser is sufficient for preview rendering segments [VERIFIED: code review] |

**Installation:**
No new dependencies needed. All components already exist in the build.

## Architecture Patterns

### System Architecture Diagram

```text
[Model File (STL/3MF)]
        |
        v
[ProjectServiceMock.loadFile()]  -->  sourceFilePath_ set
        |                              model_ (Slic3r::Model*) loaded
        |
        v
[EditorViewModel.requestSlice()]  -->  calls SliceService::startSlice()
        |
        v
[SliceService::startSlice()]
   |-- cloneCurrentPlateModel()   <-- ProjectServiceMock (filters to current plate printable objects)
   |-- sourceFilePath()           <-- used for G-code output naming
   |-- DynamicPrintConfig::full_print_config()  <-- DEFAULT config only (no user preset merge!)
   |-- per-plate overrides        <-- bed_type, print_sequence, spiral_mode
   |-- Print::apply(model, config)
   |-- Print::process()           <-- actual slicing
   |-- Print::export_gcode()      <-- writes to applicationDirPath() with timestamp filename
   |
   |  on success:
   |-- outputPath_ = generated file path
   |-- emit sliceFinished(estimatedTime)
   |       |
   |       +--> EditorViewModel: records m_sliceEstimatedTime, m_sliceResultPlateIndex
   |       +--> PreviewViewModel: calls rebuildFromGCode(outputPath)
   |       +--> BackendContext: calls postSlicingComplete()
   |
   v
[PreviewViewModel::rebuildFromGCode(filePath)]
   |-- Opens file, parses G0/G1 commands
   |-- Builds segments_ vector with positions, colors, metadata
   |-- recolorAndPackSegments() --> gcodePreviewData_ (binary "GCV1" format)
   |-- emit stateChanged() --> QML rebinds
   |
   v
[EditorViewModel::switchToPreview()]
   |-- emit previewRequested()
   |       |
   |       +--> QML PreparePage: backend.setCurrentPage(2)
   |                     |
   |                     +--> StackLayout switches to PreviewPage
   |
   v
[GCodeRenderer consumes gcodePreviewData_]
   <-- OpenGL rendering of G-code segments
```

### Recommended Project Structure
No structural changes needed. Existing structure is correct:
```
src/core/services/SliceService.cpp       -- slice engine bridge
src/core/viewmodels/EditorViewModel.cpp  -- prepare page state + slice bridge
src/core/viewmodels/PreviewViewModel.cpp -- G-code parse + preview state
src/qml_gui/BackendContext.cpp           -- notification routing
src/qml_gui/pages/PreparePage.qml        -- slice UI + preview navigation
src/qml_gui/panels/SliceProgress.qml     -- slice result panel
tests/ViewModelSmokeTests.cpp            -- existing smoke tests
```

### Pattern 1: Async Slice with Signal Chain
**What:** Slice runs in QtConcurrent::run(), communicates progress/result via QMetaObject::invokeMethod(Qt::QueuedConnection).
**When to use:** All slice operations (startSlice, loadGCodeFromPrevious).
**Example:**
```cpp
// Source: SliceService.cpp:189
QtConcurrent::run([receiver, cancelFlag, ...]() mutable {
    // background work...
    QMetaObject::invokeMethod(receiver, [...]() {
        // main-thread result delivery
        emit receiver->sliceFinished(estimatedTime);
    }, Qt::QueuedConnection);
});
```

### Pattern 2: Model Clone for Thread Safety
**What:** cloneCurrentPlateModel() deep-copies the Slic3r::Model, filtering to current plate's printable objects.
**When to use:** Before passing model to slice thread (prevents data race with main thread).
**Example:**
```cpp
// Source: ProjectServiceMock.cpp:362-384
auto clonedModel = std::make_unique<Slic3r::Model>(*model_);
// Remove objects not on current plate or not printable
for (int i = clonedModel->objects.size() - 1; i >= 0; --i) {
    if (!allowedObjectIndices.contains(i))
        clonedModel->delete_object(size_t(i));
}
return clonedModel;
```

### Anti-Patterns to Avoid
- **Testing only happy path:** The slice pipeline has many failure modes (empty model, cancel, validate error, export fail) that must each be verified.
- **Assuming config from UI reaches slice engine:** Currently it does NOT -- SliceService uses `full_print_config()` defaults, not user-selected presets.
- **Treating mock-mode results as valid E2E proof:** HAS_LIBSLIC3R must be ON for real E2E verification; mock mode skips the actual engine.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| G-code export | Custom file writer | `Slic3r::Print::export_gcode()` | Handles all G-code flavor nuances, metadata headers, thumbnails |
| Model cloning | Manual object copy | `Slic3r::Model` copy constructor + delete_object | Proper deep copy of volumes, instances, configs |
| Print validation | Pre-slice checks | `Slic3r::Print::validate()` | Upstream validation logic for bed fit, config conflicts |
| Progress reporting | Custom callbacks | `Slic3r::Print::set_status_callback()` | Thread-safe progress from slice engine |

**Key insight:** The upstream engine handles the hard parts (mesh repair, support generation, path planning). The Qt6 code's job is correct data handoff, not reimplementation.

## Common Pitfalls

### Pitfall 1: Slice Config Uses Defaults, Not User Presets
**What goes wrong:** SliceService calls `Slic3r::DynamicPrintConfig::full_print_config()` which gives factory defaults. User's printer/filament/process preset selections in ConfigViewModel are NOT applied to the slice. The per-plate overrides (bed_type, print_sequence, spiral_mode) are applied, but core slicing parameters (layer height, infill, speed, supports) come from defaults.
**Why it happens:** ConfigViewModel/PresetServiceMock stores preset values as QHash<QString,QVariant> but there is no bridge to inject them into the Slic3r::DynamicPrintConfig used by SliceService.
**How to avoid:** For this E2E verification phase, document this as a KNOWN GAP. The slice will succeed with default settings, producing valid G-code, but the output will not reflect user parameter changes. Fixing this is a separate config-injection task.
**Warning signs:** Sliced G-code uses 0.2mm layer height regardless of user preset selection; no support structures even when user enables supports.
**Confidence:** HIGH [VERIFIED: SliceService.cpp:228 shows `full_print_config()` with no preset injection]

### Pitfall 2: Preview Data Format Mismatch
**What goes wrong:** PreviewViewModel produces binary "GCV1" format with PackedSegment structs. If the GCodeRenderer expects a different layout or the float packing is wrong, preview renders garbage.
**Why it happens:** The binary format is a custom protocol between PreviewViewModel and GCodeRenderer, not standardized.
**How to avoid:** Verify that PackedSegment field order and sizes match what GCodeRenderer reads. Check the sizeof(PackedSegment) matches between writer and reader.
**Warning signs:** Preview shows random lines, all segments at origin, or crash in GCodeRenderer.
**Confidence:** MEDIUM [VERIFIED: PreviewViewModel.cpp:46-65 defines PackedSegment, GCodeRenderer reads it]

### Pitfall 3: Slice Result Not Invalidated on Model Change
**What goes wrong:** After slicing, if the user modifies the model (transform, add, delete) and tries to slice again, `canRequestSlice()` returns false because `hasSliceResult()` is still true.
**Why it happens:** `invalidateSliceResultsForCurrentPlate()` must be called after every model mutation, and it removes the plate from `m_slicedPlateIndices`. If a mutation path misses this call, the slice button stays disabled.
**How to avoid:** Verify all model mutation paths in EditorViewModel call `invalidateSliceResultsForCurrentPlate()`. Current code does this for transforms, deletes, printable toggles, bed type changes -- but may miss edge cases.
**Warning signs:** After moving an object, the "Slice" button remains grayed out.
**Confidence:** HIGH [VERIFIED: grep shows consistent invalidation calls]

### Pitfall 4: G-code Output Path in Application Directory
**What goes wrong:** SliceService writes G-code to `QCoreApplication::applicationDirPath()` with a timestamp-based filename. If the application directory is read-only (e.g., Program Files), export_gcode will fail silently or throw.
**Why it happens:** The output path is hardcoded to the exe directory rather than using a temp directory or user-selected path.
**How to avoid:** For E2E testing, run from a writable directory. In production, this path should use a temp directory.
**Warning signs:** Slice fails with "export_gcode" in error message; no .gcode file produced.
**Confidence:** HIGH [VERIFIED: SliceService.cpp:301-307]

### Pitfall 5: Race Condition on Slice Completion
**What goes wrong:** PreviewViewModel connects to `sliceFinished` and immediately calls `rebuildFromGCode(sliceService_->outputPath())`. If `outputPath_` is not yet set when the signal handler runs, rebuild receives an empty string and produces no preview data.
**Why it happens:** The lambda in SliceService's final invokeMethod sets `outputPath_` and then emits `sliceFinished` in the same call. This should be safe because they're in the same QueuedConnection delivery. But if PreviewViewModel connects via a different connection type, ordering could break.
**How to avoid:** Verify that PreviewViewModel's `rebuildFromGCode` handles empty filePath gracefully (it does -- early return on file open failure).
**Warning signs:** After slice completes, preview shows no data; estimatedTime is set but gcodePreviewData is empty.
**Confidence:** MEDIUM [VERIFIED: PreviewViewModel.cpp:183-188 shows correct signal connection; line 438-440 shows early return on file open failure]

### Pitfall 6: Mock Mode vs Real Mode Discrepancy
**What goes wrong:** Tests run without HAS_LIBSLIC3R see mock slice results with fake file paths like "(mock) model_plate1.gcode". PreviewViewModel tries to open this fake path and fails silently.
**Why it happens:** Mock mode in SliceService generates a fake outputPath string that doesn't correspond to a real file.
**How to avoid:** E2E tests MUST run with HAS_LIBSLIC3R=ON. The mock path is not suitable for preview verification.
**Warning signs:** Slice "succeeds" but preview stays empty; outputPath starts with "(mock)".
**Confidence:** HIGH [VERIFIED: SliceService.cpp:360-362]

## Code Examples

### Complete Slice Chain (EditorViewModel -> SliceService -> PreviewViewModel)
```cpp
// Source: EditorViewModel.cpp:3282-3294
void EditorViewModel::requestSlice()
{
    if (!canRequestSlice()) {
        statusText_ = sliceActionHint();
        emit stateChanged();
        return;
    }
    m_sliceEstimatedTime.clear();
    m_sliceResultPlateIndex = -1;
    sliceService_->startSlice(projectService_->projectName());
}

// Source: PreviewViewModel.cpp:183-188
connect(sliceService_, &SliceService::sliceFinished, this, [this](const QString &time) {
    estimatedTime_ = time;
    totalTime_ = time;
    rebuildFromGCode(sliceService_->outputPath());
    emit stateChanged();
});
```

### Page Switch on Preview Request
```cpp
// Source: EditorViewModel.cpp:3485-3488
void EditorViewModel::switchToPreview() {
    emit previewRequested();
}

// Source: PreparePage.qml:3514-3516
Connections {
    target: root.editorVm
    function onPreviewRequested() { backend.setCurrentPage(2) }
}
```

### G-code Export to User-Selected Path
```cpp
// Source: SliceService.cpp:574-604
bool SliceService::exportGCodeToPath(const QString &targetPath) {
    if (outputPath_.isEmpty()) return false;
    const QFileInfo srcInfo(outputPath_);
    if (!srcInfo.exists() || !srcInfo.isFile()) return false;
    if (QFile::exists(targetPath)) QFile::remove(targetPath);
    return QFile::copy(outputPath_, targetPath);
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Mock-only slice in tests | Real libslic3r slice with HAS_LIBSLIC3R | Phase 1-5 migration | E2E tests can use real engine |
| No config injection | SliceService uses full_print_config() defaults | Current | Slice succeeds but ignores user presets |

**Deprecated/outdated:**
- Mock slice mode (`#else` branch in startSlice): Produces fake results, not suitable for E2E verification.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | SliceService::cloneCurrentPlateModel() produces a valid model with correct objects for current plate | Standard Stack | Slice fails with "no objects" even though objects exist |
| A2 | Slic3r::DynamicPrintConfig::full_print_config() produces valid defaults that allow slicing standard STL models | Pitfalls | Default config may lack required printer/machine settings |
| A3 | G-code output to applicationDirPath() works in the development environment (build/ directory is writable) | Pitfalls | G-code export fails |
| A4 | PreviewViewModel's text-based G-code parser handles the G-code format produced by CrealityPrint's export_gcode() | Architecture | Preview shows no segments or wrong positions |
| A5 | The binary "GCV1" format in gcodePreviewData_ is correctly consumed by GCodeRenderer | Pitfalls | Preview renders garbage or crashes |
| A6 | hasSliceResult() correctly tracks whether the current plate has a valid slice result | Pitfalls | Slice button stays disabled after model changes |

## Open Questions

1. **Config Injection Gap**
   - What we know: SliceService uses `full_print_config()` defaults; ConfigViewModel stores user presets in PresetServiceMock.
   - What's unclear: Whether this gap should be fixed in this E2E verification phase or deferred to a separate config-injection phase.
   - Recommendation: Document as known gap; E2E verification confirms the pipeline works with defaults. Config injection is a separate task.

2. **GCodeRenderer Binary Format Compatibility**
   - What we know: PreviewViewModel writes "GCV1" format with PackedSegment structs. GCodeRenderer reads this data.
   - What's unclear: Whether the sizeof(PackedSegment) matches between the writer (PreviewViewModel.cpp:46-65) and the reader in GCodeRenderer.
   - Recommendation: Verify by checking GCodeRenderer source for struct layout compatibility.

3. **Thread Safety of activePrint_ Access**
   - What we know: SliceService stores active Slic3r::Print* in std::atomic for cancel support.
   - What's unclear: Whether cancelSlice() can safely call print.cancel() while the slice thread is still using the print object.
   - Recommendation: Verify that Print::cancel() is thread-safe in libslic3r.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| MSVC (vcvars64) | Build | Yes | VS 2022 | -- |
| Qt 6.10 | Runtime + Tests | Yes | 6.10 | -- |
| libslic3r (HAS_LIBSLIC3R) | Real slice | Yes | v7.0.1 | Mock mode (not suitable for E2E) |
| Ninja | Build | Yes | -- | -- |
| Test STL (hotend.stl) | Test asset | Yes | -- | -- |
| Test 3MF (arc.3mf) | Test asset | Yes | -- | -- |
| Test G-code (ams_load.gcode) | Test asset | Yes | -- | -- |
| Multi-plate 3MF (ksr_fdmtest_v4.3mf) | Test asset | Yes | -- | -- |

**Missing dependencies with no fallback:**
- None -- all required dependencies are available.

**Missing dependencies with fallback:**
- None.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Qt Test 6.10 |
| Config file | None -- tests defined in CMakeLists.txt |
| Quick run command | `cmake --build build --target ViewModelSmokeTests && build/ViewModelSmokeTests.exe` |
| Full suite command | Same as above (single test binary) |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| E2E-01 | Load STL -> model count > 0 -> sourceFilePath set | unit | `ViewModelSmokeTests.exe editor_import_model_updates_state` | Yes (existing) |
| E2E-02 | Load 3MF -> mesh data generated -> model count > 0 | unit | `ViewModelSmokeTests.exe topbar_import_3mf_generates_mesh` | Yes (existing) |
| E2E-03 | Slice produces non-empty G-code file at outputPath | unit | New test needed | No -- Wave 0 |
| E2E-04 | Slice results propagate to EditorViewModel (estimatedTime, weight, etc.) | unit | New test needed | No -- Wave 0 |
| E2E-05 | PreviewViewModel receives outputPath and parses G-code segments | unit | New test needed | No -- Wave 0 |
| E2E-06 | switchToPreview emits previewRequested -> page switches to 2 | unit | New test needed | No -- Wave 0 |
| E2E-07 | exportGCodeToPath copies G-code to target location | unit | New test needed | No -- Wave 0 |
| E2E-08 | Slice cancel works and emits sliceFailed | unit | New test needed | No -- Wave 0 |
| E2E-09 | Slice validation error propagates to UI | unit | New test needed | No -- Wave 0 |
| E2E-10 | Model change after slice invalidates slice result | unit | New test needed | No -- Wave 0 |
| E2E-11 | Full chain: load -> slice -> preview switch -> G-code segments rendered | integration | New test needed | No -- Wave 0 |
| E2E-12 | G-code load from previous file works | unit | `ViewModelSmokeTests.exe slice_reuse_previous_gcode_file` | Yes (existing) |

### Sampling Rate
- **Per task commit:** `cmake --build build --target ViewModelSmokeTests && build/ViewModelSmokeTests.exe`
- **Per wave merge:** Full build via `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- **Phase gate:** Full suite green + manual runtime verification of slice + preview

### Wave 0 Gaps
- [ ] `tests/E2EWorkflowTests.cpp` -- new test file covering E2E-03 through E2E-11
- [ ] Verify ViewModelSmokeTests builds and passes before adding new tests
- [ ] Confirm test binary links against libslic3r (HAS_LIBSLIC3R must be ON for real E2E tests)

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | No | No auth in desktop app |
| V3 Session Management | No | No sessions |
| V4 Access Control | No | No multi-user access |
| V5 Input Validation | Yes | File path validation in SliceService (QFileInfo::exists) |
| V6 Cryptography | No | No crypto in slice pipeline |

### Known Threat Patterns for C++/Qt Desktop Slice Pipeline

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| File path traversal (load/save) | Tampering | QFileInfo canonicalization, url.toLocalFile() |
| Crash from malformed 3MF/STL | Denial of Service | libslic3r try/catch in slice thread |
| G-code output to unexpected location | Information Disclosure | Fixed output path calculation from appDir + timestamp |

## Sources

### Primary (HIGH confidence)
- Code review of `src/core/services/SliceService.cpp` -- complete file analyzed
- Code review of `src/core/viewmodels/EditorViewModel.cpp` -- slice bridge methods analyzed
- Code review of `src/core/viewmodels/PreviewViewModel.cpp` -- complete file analyzed
- Code review of `src/qml_gui/BackendContext.cpp` -- notification routing verified
- Code review of `src/qml_gui/pages/PreparePage.qml` -- preview signal connection verified
- Code review of `src/core/services/ProjectServiceMock.cpp` -- cloneCurrentPlateModel verified
- Code review of `third_party/CrealityPrint/src/slic3r/GUI/Plater.cpp` -- upstream on_process_completed analyzed

### Secondary (MEDIUM confidence)
- Test infrastructure review of `tests/ViewModelSmokeTests.cpp` -- existing test patterns analyzed
- Build configuration review of `CMakeLists.txt` -- HAS_LIBSLIC3R and test setup verified

### Tertiary (LOW confidence)
- None -- all findings are based on direct code review

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all components verified in codebase
- Architecture: HIGH -- complete data flow traced through source files
- Pitfalls: HIGH -- each pitfall verified by code inspection
- Config gap: HIGH -- confirmed by tracing SliceService.cpp:228 vs ConfigViewModel

**Research date:** 2026-06-01
**Valid until:** 2026-07-01 (stable codebase, no fast-moving dependencies)
