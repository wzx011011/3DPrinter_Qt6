# Phase v12: Slicing Pipeline Gaps - Research

**Researched:** 2026-06-02
**Domain:** CrealityPrint Qt6/QML slicing pipeline (load -> render -> config -> slice -> export -> preview)
**Confidence:** HIGH (code-verified, line-level references)

## Summary

This research examines the full slicing pipeline in the Qt6/QML migration of CrealityPrint, comparing each stage against the upstream source truth in `third_party/CrealityPrint/src/slic3r/GUI/Plater.cpp`. The pipeline spans six stages: model loading, rendering, preset application, slice engine invocation, G-code export, and post-slice preview.

**Primary recommendation:** Fix the multi-plate 3MF loading bug first (it breaks the most fundamental workflow), then add auto-arrange on load, then implement PartPlateList equivalent, then fix plate metadata initialization gaps. The slice engine integration itself is functional end-to-end.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Model loading & parsing | Service (ProjectServiceMock) | — | libslic3r Model::read_from_file is a backend operation |
| Plate data management | Service (ProjectServiceMock) | — | Plate metadata is domain data owned by the service layer |
| Mesh extraction for GL | Service (ProjectServiceMock) | Renderer (GLViewport) | Service produces TLV bytes; renderer consumes them |
| Auto-arrange on load | Service (ProjectServiceMock) | — | arrange_objects() is a libslic3r algorithm |
| Preset config merge | ViewModel (ConfigViewModel) | Service (SliceService) | ViewModel accumulates user selections; injects into SliceService before slice |
| Slice engine invocation | Service (SliceService) | — | libslic3r Print::process() is pure backend |
| G-code export | Service (SliceService) | — | Print::export_gcode() is a backend operation |
| Post-slice preview parsing | ViewModel (PreviewViewModel) | Renderer (GCodeRenderer) | ViewModel parses gcode text; renderer draws segments |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| libslic3r (Model API) | v7.0.1 upstream | Model loading, plate data, mesh data | Source truth -- the engine being wrapped |
| libslic3r (Print API) | v7.0.1 upstream | Slice engine, G-code export | Source truth -- Print::process(), Print::export_gcode() |
| libslic3r (ModelArrange) | v7.0.1 upstream | Auto-arrange algorithm | Already linked and used |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| QtConcurrent | Qt 6.10 | Async model loading, async slicing | All background operations |
| QOpenGLFramebufferObject | Qt 6.10 | Viewport rendering | GLViewport / GCodeRenderer |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Custom TLV mesh format | QOpenGLBuffer streaming | TLV is working; no need to change |

## Architecture Patterns

### System Architecture Diagram

```
[QML FileDialog]
       |
       v
[EditorViewModel::loadFile()]
       |
       v
[ProjectServiceMock::loadFile()]  -- QtConcurrent::run
       |
       +--> libslic3r::Model::read_from_file()
       |         |
       |         +--> plateDataList extracted (STL/3MF)
       |         +--> model->objects populated
       |         |
       |         v
       |    [Main Thread callback]
       |         |
       |         +--> Replace model_, sync transforms
       |         +--> Set plateNames_, plateObjectIndices_
       |         +--> emit projectChanged()
       |                |
       |                v
       |         [EditorViewModel refreshMeshCacheAndFitHint()]
       |                |
       |                +--> projectService_->meshData() -- TLV extraction
       |                +--> emit stateChanged()
       |                       |
       |                       v
       |                [QML binding: editorVm.meshData -> GLViewport.meshData]
       |                       |
       |                       v
       |                [GLViewportRenderer::uploadMesh() -- GPU upload]
       |
       v
[User clicks "Slice"]
       |
       v
[EditorViewModel::requestSlice()]
       |
       +--> configViewModel_->mergedConfigValues() --> sliceService_->setMergedPresetConfig()
       +--> sliceService_->startSlice()
       |
       v
[SliceService::startSlice()]  -- QtConcurrent::run
       |
       +--> projectService_->cloneCurrentPlateModel()
       +--> DynamicPrintConfig::full_print_config()
       +--> injectPresetConfig(config, mergedPresetConfig_)
       +--> Print::apply(*modelForSlice, config)
       +--> Print::validate()
       +--> Print::process()
       +--> Print::export_gcode()
       |
       v
[Main Thread: emit sliceFinished()]
       |
       +--> [PreviewViewModel::rebuildFromGCode(outputPath)]
       |         |
       |         +--> Parse gcode file line-by-line
       |         +--> Build segments_, layerTimes_, legendItems_
       |         +--> emit stateChanged()
       |                |
       |                v
       |         [QML binding: previewVm.gcodePreviewData -> GLViewport.previewData]
       |
       +--> [EditorViewModel updates status, slice results]
```

### Recommended Project Structure
```
src/core/services/
├── ProjectServiceMock.h/cpp    # Model loading, plate management, mesh extraction
├── SliceService.h/cpp          # Slice engine invocation, G-code export
└── (future) PartPlateService.h/cpp  # Spatial plate management (PartPlateList equivalent)
src/core/viewmodels/
├── EditorViewModel.h/cpp       # Prepare page orchestration
├── ConfigViewModel.h/cpp       # Preset hierarchy + config injection
└── PreviewViewModel.h/cpp      # Post-slice gcode parsing + preview state
src/qml_gui/Renderer/
├── GLViewport.h/cpp            # QML OpenGL viewport (prepare + preview)
├── GLViewportRenderer.h/cpp    # Mesh rendering + gizmos
└── GCodeRenderer.h/cpp         # G-code preview rendering
```

### Pattern 1: Async Load with Main-Thread Commit
**What:** Background thread calls libslic3r, main thread commits results via QMetaObject::invokeMethod
**When to use:** All model loading and slicing operations
**Example:**
```cpp
// Source: ProjectServiceMock.cpp:416-749
QtConcurrent::run([receiver, localPath, cancelFlag]() {
    auto *loadedModel = new Slic3r::Model();
    // ... libslic3r work on background thread ...
    QMetaObject::invokeMethod(receiver, [...]() {
        receiver->model_ = loadedModel; // main-thread commit
        emit receiver->projectChanged();
    }, Qt::QueuedConnection);
});
```

### Pattern 2: Config Injection via mergedConfigValues
**What:** ConfigViewModel accumulates all user preset selections into globalOptionValues_, which is injected into SliceService just before slicing
**When to use:** Every slice operation
**Example:**
```cpp
// Source: EditorViewModel.cpp:3298-3302, SliceService.cpp:301-309
sliceService_->setMergedPresetConfig(configViewModel_->mergedConfigValues());
// ...
Slic3r::DynamicPrintConfig config = Slic3r::DynamicPrintConfig::full_print_config();
injectPresetConfig(config, receiver->mergedPresetConfig_);
```

### Anti-Patterns to Avoid
- **Releasing plate data before extracting it:** The loadProject() bug -- always extract data from plateDataList BEFORE calling release_PlateData_list()
- **Using InfiniteBed for multi-plate arrange:** Upstream uses real plate boundaries from PartPlateList; InfiniteBed ignores plate constraints entirely

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Multi-plate spatial management | Custom plate tracking in QHash<QList<int>> | PartPlateList (upstream pattern) | Upstream has 200+ lines of plate-object containment checks, exclude areas, height limits |
| Arrange with real plate boundaries | Custom arrange with hardcoded bed | libslic3r arrange_objects with real bed shape | The algorithm needs actual bed polygon, exclude areas, plate-to-plate spacing |
| G-code file parsing | Custom gcode parser | PreviewViewModel::rebuildFromGCode (already works) | Already implemented and functional |

**Key insight:** The biggest gaps are NOT in the slice engine itself (which works end-to-end), but in the plate management layer that upstream handles via PartPlateList -- a ~600-line class managing spatial relationships, containment checks, and per-plate slicing contexts.

## Common Pitfalls

### Pitfall 1: Plate data released before extraction (CONFIRMED BUG)
**What goes wrong:** `loadProject()` calls `release_PlateData_list()` at line 4318-4319, then tries to read `plateDataList` at line 4350 -- but it's already empty
**Why it happens:** The release call and the extraction are in the same scope but the extraction comes AFTER the release
**How to avoid:** Extract all plate data BEFORE calling release, as loadFile() correctly does (extracts at lines 507-537, releases at lines 579-580)
**Warning signs:** Multi-plate 3MF files always load as single-plate "Plate 1" with all objects

### Pitfall 2: Plate metadata arrays not reset on load
**What goes wrong:** After loading a new model, `plateBedTypes_`, `plateLockedStates_`, `platePrintSequences_`, `plateSpiralModes_` are NOT cleared in the success path
**Why it happens:** These arrays are cleared only in the failure path (line 738-742); the success path (lines 700-723) sets plateNames_ and plateObjectIndices_ but leaves per-plate settings arrays stale
**How to avoid:** Clear all per-plate arrays at the start of the success commit, matching plateObjectIndices_ size
**Warning signs:** Changing bed type on plate 1 of a previous project persists into a newly loaded project

### Pitfall 3: No auto-arrange on load
**What goes wrong:** Models load at whatever position libslic3r places them (often stacked on top of each other or off-bed)
**Why it happens:** loadFile() never calls arrange_objects() after loading; upstream calls arrange_loaded_object_to_new_position() for each new instance
**How to avoid:** After loadFile succeeds and model_ is committed, call arrangeObjects() or arrange_loaded_object_to_new_position()
**Warning signs:** Loading multiple STL files results in overlapping objects

### Pitfall 4: Single-plate arrange only (InfiniteBed)
**What goes wrong:** arrangeObjects() uses `Slic3r::InfiniteBed` which has no real bed boundaries -- objects can be placed anywhere in infinite space
**Why it happens:** Line 2064: `Slic3r::InfiniteBed bed;` -- no actual bed geometry provided
**How to avoid:** Use real bed shape from printer config, aligned with upstream ArrangeJob which uses `get_bed_shape(*config)` and PartPlateList boundaries
**Warning signs:** Arranged objects may end up outside the printable area

### Pitfall 5: loadProject() ignores loaded config
**What goes wrong:** When loading a 3MF project, `loadedConfig` (DynamicPrintConfig from the 3MF) is extracted but never applied to anything -- it goes out of scope
**Why it happens:** The loadProject lambda declares `Slic3r::DynamicPrintConfig loadedConfig` at line 4253, passes it to read_from_archive, but never propagates it to ConfigViewModel or PresetServiceMock
**How to avoid:** Emit a signal with the loaded config, or pass it to ConfigViewModel for preset matching
**Warning signs:** Opening a 3MF project always shows default preset values, not the settings saved in the project

## Code Examples

### The loadProject() plate data bug (lines 4318-4370)
```cpp
// BROKEN: ProjectServiceMock.cpp loadProject(), background thread
// Line 4318-4319: release happens FIRST
if (!plateDataList.empty())
    Slic3r::release_PlateData_list(plateDataList);
// Line 4350: plateDataList is now EMPTY, this check always fails
if (!plateDataList.empty())
{
    // NEVER REACHED -- always falls to else
}
else
{
    // ALWAYS reaches here: single-plate fallback
    loadedPlateCount = 1;
    loadedPlateNames << QObject::tr("Plate 1");
    // all objects on single plate
}

// CORRECT: ProjectServiceMock.cpp loadFile(), background thread
// Lines 507-537: extract FIRST
if (!plateDataList.empty())
{
    for (int plateIdx = 0; plateIdx < loadedPlateCount; ++plateIdx)
    {
        const auto *plate = plateDataList[size_t(plateIdx)];
        // ... extract plate names and object indices ...
    }
}
// Lines 579-580: release AFTER
if (!plateDataList.empty())
    Slic3r::release_PlateData_list(plateDataList);
```

### Upstream plate data usage (Plater.cpp:4645-4654)
```cpp
// Source: third_party/CrealityPrint/src/slic3r/GUI/Plater.cpp
// Upstream uses PartPlateList to manage plates from 3MF:
partplate_list.load_from_3mf_structure(plate_data);
partplate_list.update_slice_context_to_current_plate(background_process);
this->preview->update_gcode_result(partplate_list.get_current_slice_result());
release_PlateData_list(plate_data);
sidebar->obj_list()->reload_all_plates();
```

### Current arrangeObjects() with InfiniteBed (line 2064)
```cpp
// Source: ProjectServiceMock.cpp:2049-2079
bool ProjectServiceMock::arrangeObjects(float spacing, bool allowRotation, bool alignY)
{
    Slic3r::ArrangeParams params;
    params.min_obj_distance = static_cast<coord_t>(spacing * 1000.0);
    params.allow_rotations = allowRotation;
    params.align_to_y_axis = alignY;

    Slic3r::InfiniteBed bed;  // BUG: no real bed boundaries
    Slic3r::arrange_objects(*model_, bed, params);
    return true;
}
```

### Slice engine end-to-end (working correctly)
```cpp
// Source: SliceService.cpp:279-409
Slic3r::DynamicPrintConfig config = Slic3r::DynamicPrintConfig::full_print_config();
injectPresetConfig(config, receiver->mergedPresetConfig_);
// Apply per-plate overrides (bed type, print sequence, spiral mode)
Slic3r::Print print;
print.set_is_CX_printer(true);
print.apply(*modelForSlice, config);
Slic3r::StringObjectException validationError = print.validate();
print.process();
const std::string generated = print.export_gcode(targetPath.toStdString(), &result);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| JSON mock save | Real 3MF via store_bbs_3mf | Phase v11-01 | 3MF save now produces real files |
| Mock slicing (fake progress) | Real libslic3r Print::process() | Phase v11-02 | Slice engine works end-to-end |
| Single plate only | Multi-plate tracking (QList<int> arrays) | Phase v11-01 | Plate structure exists but has bugs |

**Deprecated/outdated:**
- JSON mock load/save: Still present as fallback when HAS_LIBSLIC3R is off
- Mock slice simulation: Still present as `#else` branch when HAS_LIBSLIC3R is off

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Print::export_gcode() produces valid G-code that printers accept | Slice Engine | G-code might have header/footer format issues |
| A2 | DynamicPrintConfig::full_print_config() provides sufficient defaults for slicing | Config/Preset | Some configs might need printer-specific initialization |
| A3 | PreviewViewModel::rebuildFromGCode() parses all gcode dialects CrealityPrint produces | Post-slice Preview | Custom gcode commands might be skipped |
| A4 | The 3MF files being loaded always have non-empty plateDataList | Model Loading | Some 3MF variants might not have plate data |

## Open Questions

1. **Does Print::validate() cover all upstream validation?**
   - What we know: It checks basic config consistency
   - What's unclear: Whether upstream has additional pre-slice checks in BackgroundSlicingProcess
   - Recommendation: Test with real files; if validation passes, proceed

2. **What bed shape does the printer preset provide?**
   - What we know: Current arrange uses InfiniteBed
   - What's unclear: Whether printer presets in PresetServiceMock contain bed_shape config
   - Recommendation: Check PresetServiceMock for bed_shape parsing; if present, use it for arrange

3. **Should PartPlateList be ported as a service or kept as metadata arrays?**
   - What we know: Current approach uses parallel QHash/QList arrays; upstream uses a dedicated class
   - What's unclear: Whether the array approach can scale to all upstream PartPlateList features
   - Recommendation: For this phase, fix the bugs in the current approach; a PartPlateList port is a larger undertaking

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| MSVC / vcvars64 | Build | Yes | VS 2022 | -- |
| Qt 6.10 | QML GUI | Yes | 6.10 | -- |
| libslic3r (from source) | Slice engine | Yes | v7.0.1 upstream | -- |
| CGAL | Mesh boolean ops | Yes | linked | -- |
| Ninja | Build system | Yes | -- | -- |
| CMake 3.21+ | Build config | Yes | -- | -- |

**Missing dependencies with no fallback:** None identified.

**Missing dependencies with fallback:** None identified.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Qt Test (QTest) |
| Config file | CMakeLists.txt (CTest enabled) |
| Quick run command | `cmake --build build --config Release && ctest --test-dir build` |
| Full suite command | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| LOAD-01 | loadFile() correctly extracts multi-plate data before release | unit | Build verification | No -- Wave 0 |
| LOAD-02 | loadProject() extracts multi-plate data from 3MF | unit | Build verification | No -- Wave 0 |
| LOAD-03 | Plate metadata arrays reset on new model load | unit | Build verification | No -- Wave 0 |
| ARRANGE-01 | Auto-arrange runs after model load | integration | Build + run | No -- Wave 0 |
| ARRANGE-02 | arrangeObjects() uses real bed boundaries | unit | Build verification | No -- Wave 0 |
| CONFIG-01 | loadedConfig from 3MF project propagates to UI | integration | Build + run | No -- Wave 0 |
| SLICE-01 | Full slice pipeline produces valid gcode | integration | `scripts/auto_verify_with_vcvars.ps1` | Existing smoke test |
| PREVIEW-01 | Post-slice preview loads gcode correctly | integration | Build + run | No -- Wave 0 |

### Sampling Rate
- **Per task commit:** `cmake --build build --config Release`
- **Per wave merge:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- **Phase gate:** Full build + 0 QML warnings + manual pipeline test

### Wave 0 Gaps
- [ ] Test: loadFile multi-plate extraction correctness
- [ ] Test: loadProject plate data handling
- [ ] Test: plate metadata reset on load
- [ ] Test: arrange with real bed shape
- [ ] Smoke test: full pipeline (load -> arrange -> config -> slice -> export -> preview)

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V5 Input Validation | yes | libslic3r validates 3MF/STL geometry; Qt file path validation |
| V6 Cryptography | no | No crypto in slicing pipeline |

### Known Threat Patterns for C++/Qt Slicing Pipeline

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Malicious STL/3MF with huge mesh | Denial of Service | QtConcurrent + cancel flag; mesh size checks in uploadMesh |
| Path traversal in file save | Tampering | QFileInfo sanitization |
| G-code injection | Tampering | libslic3r produces gcode, not user input |

## Sources

### Primary (HIGH confidence)
- `src/core/services/ProjectServiceMock.cpp` -- line-by-line code audit
- `src/core/services/SliceService.cpp` -- line-by-line code audit
- `src/core/viewmodels/EditorViewModel.cpp` -- constructor signal wiring, requestSlice flow
- `src/core/viewmodels/ConfigViewModel.h` -- mergedConfigValues() definition
- `src/core/viewmodels/PreviewViewModel.cpp` -- rebuildFromGCode flow
- `src/qml_gui/Renderer/GLViewportRenderer.cpp` -- uploadMesh, render flow
- `third_party/CrealityPrint/src/slic3r/GUI/Plater.cpp` -- upstream load_files, arrange_loaded_object_to_new_position
- `third_party/CrealityPrint/src/slic3r/GUI/PartPlate.cpp` -- upstream load_from_3mf_structure

### Secondary (MEDIUM confidence)
- `third_party/CrealityPrint/src/slic3r/GUI/Jobs/ArrangeJob.cpp` -- upstream arrange with real plate boundaries

## Detailed Gap Analysis

### A. Model Loading Pipeline

#### A1. loadFile() -- STL/OBJ/3MF loading (WORKING with caveat)

**Status: FUNCTIONAL**
- Correctly calls `Slic3r::Model::read_from_file()` with `LoadStrategy::AddDefaultInstances | LoadStrategy::LoadModel`
- Correctly extracts plate data BEFORE releasing it (lines 507-537 vs 579-580) -- this is the CORRECT pattern
- Correctly syncs transforms from libslic3r to Qt arrays with coordinate conversion (slic3r Z-up -> GL Y-up)
- **MISSING:** No auto-arrange after load. Upstream calls `arrange_loaded_object_to_new_position()` for each new instance (Plater.cpp:5907). Qt6 loadFile() does not call arrangeObjects() anywhere.
- **MISSING:** No fit-to-view trigger after load. EditorViewModel has `refreshMeshCacheAndFitHint()` which computes fitHint, but it is only called internally. The QML side needs to use fitHint to zoom the camera.

#### A2. loadProject() -- 3MF project loading (BUG)

**Status: BROKEN for multi-plate 3MF files**
- Line 4318-4319: `release_PlateData_list(plateDataList)` is called BEFORE plate data extraction at line 4350
- Result: `plateDataList.empty()` is always true at line 4350, so the code falls to the else branch (lines 4361-4370) which creates a single plate with all objects
- Compare with loadFile() which correctly extracts FIRST (lines 507-537) then releases (lines 579-580)
- **FIX:** Move plate data extraction (building loadedPlateNames, loadedPlateObjectIndices from plateDataList) to BEFORE the release call

#### A3. Plate metadata arrays not reset on load (BUG)

**Status: BROKEN**
- In the success path of loadFile() (lines 700-723), these arrays are NOT cleared:
  - `plateBedTypes_`
  - `platePrintSequences_`
  - `plateSpiralModes_`
  - `plateLockedStates_`
  - `plateFirstLayerSeqChoices_`
  - `plateFirstLayerSeqOrders_`
  - `plateOtherLayersSeqChoices_`
  - `plateOtherLayersSeqEntries_`
- These arrays grow via `while (size <= plateIndex)` lazy-init pattern in setters, but never shrink
- Loading a 5-plate project after a 3-plate project leaves stale entries for plates 4 and 5
- Same issue exists in loadProject() success path (lines 4436-4456)

#### A4. loadProject() ignores loaded DynamicPrintConfig (GAP)

**Status: MISSING**
- Line 4253: `Slic3r::DynamicPrintConfig loadedConfig` is declared
- Line 4286: It is passed to `read_from_archive()` and populated with the 3MF's embedded config
- After loading, `loadedConfig` goes out of scope without being used
- Upstream (Plater.cpp:4660-4700) loads embedded presets from project_presets and applies them to PresetBundle
- **IMPACT:** Opening a saved project always reverts to default printer/filament/print settings instead of the settings the project was saved with

### B. Rendering Pipeline

#### B1. Mesh extraction to GL (WORKING)

**Status: FUNCTIONAL**
- `ProjectServiceMock::meshData()` (line 4635) produces TLV format: `[int32 objCount]` then per-object `[int32 objId][int32 triCount][float*triCount*9]` then bbox trailer
- Coordinate conversion: slic3r(X,Y,Z) -> GL(X,Z,Y) correctly applied per vertex
- Volume transforms (instMat * volMat) correctly composed
- Stable object IDs via hash mixing (lines 4668-4684)
- EditorViewModel::refreshMeshCacheAndFitHint() (line 26) calls meshData() and computes bbox

#### B2. Mesh upload to GPU (WORKING)

**Status: FUNCTIONAL**
- QML binding: `meshData: root.editorVm ? root.editorVm.meshData : null` in PreparePage.qml:1435
- GLViewport::setMeshData() stores data with version counter
- GLViewportRenderer::synchronize() picks up new data via takeMesh()
- uploadMesh() (line 1081) parses TLV, creates VAO/VBO per object batch, computes bbox
- Robust error handling with bounds checks and exception guards

#### B3. Per-object transform tracking in renderer (WORKING)

**Status: FUNCTIONAL**
- GLViewportRenderer maintains `m_objectTransforms` map (objectId -> ObjectTransform)
- Transform updates flow: QML gizmo drag -> EditorViewModel setter -> projectService_ setter -> projectChanged signal -> stateChanged -> QML rebinds meshData -> GLViewport gets updated mesh

### C. Config/Preset Application

#### C1. Config injection into slice engine (WORKING)

**Status: FUNCTIONAL**
- `EditorViewModel::requestSlice()` (line 3288):
  1. Calls `configViewModel_->mergedConfigValues()` to get all user-selected settings
  2. Calls `sliceService_->setMergedPresetConfig(config)` to inject
  3. Calls `sliceService_->startSlice()` which uses the injected config
- `SliceService::startSlice()` (line 301):
  1. Creates `DynamicPrintConfig::full_print_config()` (factory defaults)
  2. Calls `injectPresetConfig(config, mergedPresetConfig_)` to overwrite with user selections
  3. Applies per-plate overrides (bed type, print sequence, spiral mode)
  4. Passes config to `Print::apply(*modelForSlice, config)`
- `injectPresetConfig()` (line 140) handles type-aware value injection (Float, Int, Bool, String/Enum)

#### C2. ConfigViewModel mergedConfigValues() (WORKING but limited)

**Status: FUNCTIONAL for UI-accessible options**
- Returns `globalOptionValues_` which is a QHash<QString, QVariant> of all settings the user has touched or preset has set
- This is built by `mergePresetHierarchy()` which layers printer->filament->print preset values
- **LIMITATION:** Only contains keys that the UI settings panel has exposed. Any libslic3r config option not in the ConfigOptionModel will use its factory default from full_print_config()

### D. Slice Engine Integration

#### D1. Print::process() invocation (WORKING)

**Status: FUNCTIONAL**
- Line 337: `Slic3r::Print print;` created on stack (scoped to the background thread)
- Line 338: `print.set_is_CX_printer(true)` -- Creality-specific flag
- Line 358: `print.apply(*modelForSlice, config)` -- model + config
- Line 367: `print.validate()` -- pre-slice validation with error check
- Line 373: `print.process()` -- actual slicing
- Cancel support: atomic_bool cancel flag checked between stages

#### D2. Model preparation for slice (WORKING)

**Status: FUNCTIONAL**
- `cloneCurrentPlateModel()` (line 362) clones the model, then removes objects not on the current plate
- Filters by printable state: only objects where `objectPrintableStates_[objectIndex]` is true
- Uses deep copy: `std::make_unique<Slic3r::Model>(*model_)`

#### D3. Slice progress reporting (WORKING)

**Status: FUNCTIONAL**
- Line 341-355: Print status callback wired to emit progressUpdated signal
- Progress percent from libslic3r mapped to QML progress bar
- Status text (Chinese localized) flows through statusLabel property

### E. G-code Export

#### E1. Export mechanism (WORKING)

**Status: FUNCTIONAL**
- Line 381-389: `print.export_gcode(targetPath.toStdString(), &result)` produces actual gcode file
- Output path: `{appDir}/{basename}_{timestamp}.gcode`
- Result parsing: time, weight, filament length, layer count, cost from GCodeProcessorResult
- `exportGCodeToPath()` allows user to copy the gcode to a custom location (line 655)

#### E2. G-code export from previous file (PARTIAL)

**Status: PARTIAL**
- `loadGCodeFromPrevious()` (line 543) exists but only calls `print.export_gcode_from_previous_file()` without actually loading results back
- No statistics extraction (time, weight, etc.) after re-export
- Returns success but with empty result data

### F. Post-slice Preview

#### F1. G-code loading after slice (WORKING)

**Status: FUNCTIONAL**
- PreviewViewModel connects to SliceService::sliceFinished signal (line 183)
- On slice finish: `rebuildFromGCode(sliceService_->outputPath())` is called
- `rebuildFromGCode()` (line 411) parses the gcode file line-by-line:
  - Tracks G0/G1 moves with X/Y/Z/E coordinates
  - Classifies segments as TRAVEL, PERIMETER, INFILL, SUPPORT, etc.
  - Computes per-layer times, tool changes, filament usage
  - Builds segment data with color coding for different feature types
  - Produces legend items and statistics

#### F2. Preview rendering in GCodeRenderer (PARTIAL)

**Status: PARTIAL**
- GCodeRenderer exists with VAO/VBO upload pipeline
- `loadResult()` method exists but is a no-op (`Q_UNUSED(result)`, line 427)
- Preview rendering relies on PreviewViewModel's gcodePreviewData byte array
- The segment data flows through QML binding to GLViewport.previewData
- Basic rendering works but lacks upstream's full visual fidelity (extrusion width rendering, tool markers, etc.)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all code verified by direct file reading
- Architecture: HIGH -- traced complete data flow from load through slice to preview
- Pitfalls: HIGH -- confirmed bugs with line-level references

**Research date:** 2026-06-02
**Valid until:** 2026-07-02 (stable codebase, no fast-moving dependencies)
