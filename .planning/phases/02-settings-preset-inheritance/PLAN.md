# Phase 2: Settings and Preset Inheritance — Plan

## Tasks

### Group 1: Real Preset Loading (PresetServiceMock)
1. **Add HAS_LIBSLIC3R real preset loading** to PresetServiceMock
   - Add `loadVendorPresets()` method using upstream PresetBundle
   - Load from `resources/profiles/Creality/Creality.json`
   - Resolve inheritance chains (inherits field)
   - Populate m_presetStore with effective values per preset
   - Populate m_categoryPresets with real preset names per category
   - Replace initBuiltinDefaults() call with loadVendorPresets() when HAS_LIBSLIC3R
   - Files: `src/core/services/PresetServiceMock.h`, `src/core/services/PresetServiceMock.cpp`

2. **Add preset inherits metadata** to PresetServiceMock
   - Store parent preset name for inheritance chain display
   - Add `presetInherits(name)` method
   - Files: `src/core/services/PresetServiceMock.h`, `src/core/services/PresetServiceMock.cpp`

### Group 2: Schema Coverage (ConfigOptionModel)
3. **Audit and extend kDesiredKeys** to cover all ~110 upstream config keys
   - Cross-reference with upstream print_config_def options
   - Add missing keys (especially support tree params, seam params, bridge params)
   - Add "page" field grouping (Quality, Speed, Infill, Support, Adhesion, Cooling, Retraction, Other)
   - Files: `src/qml_gui/Models/ConfigOptionModel.cpp`

4. **Add page grouping to ConfigOption struct**
   - Add `page` field to group categories under pages (对齐上游 Tab.cpp Page grouping)
   - Update mapCategory to also set page
   - Files: `src/qml_gui/Models/ConfigOptionModel.h`, `src/qml_gui/Models/ConfigOptionModel.cpp`

### Group 3: Data Flow
5. **Wire loaded presets into ConfigViewModel**
   - Ensure mergePresetHierarchy() works with real preset values
   - Verify value source chain shows correct tier for each option
   - Files: `src/core/viewmodels/ConfigViewModel.cpp`

6. **Update PresetListModel to reflect real presets**
   - Populate from loaded vendor preset names
   - Show vendor/inheritance info
   - Files: `src/qml_gui/Models/PresetListModel.h`

### Group 4: Verification
7. **Build and verify**
   - Full build with auto_verify_with_vcvars.ps1
   - Verify all 4 success criteria
