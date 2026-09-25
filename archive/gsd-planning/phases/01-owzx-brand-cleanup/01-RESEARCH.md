# Phase 1: OWzx Brand Cleanup - Research

**Researched:** 2026-06-15
**Domain:** Brand string removal, namespace migration, submodule cleanup, version alignment
**Confidence:** HIGH -- All findings verified via grep audit of the actual codebase

## Summary

The codebase contains extensive Creality/Crality brand residue across ~50 files spanning QML UI, C++ source, CMake build config, translation files, test code, and historical documentation. The upstream submodule has already been switched from `third_party/CrealityPrint` to `third_party/OrcaSlicer` in `.gitmodules`, but the old CrealityPrint directory still physically exists on disk and many CMake/scripts/docs references persist. The internal namespace `Crality3D` (note: not "Creality" -- a deliberate shortening) is used in 3 header files and 4 source files. The QML module is registered as `CrealityGL` in one location. The CMake option `CREALITY_QML_GUI` and CMake target `creality_app_core`/`creality-cli` carry the old brand.

**Primary recommendation:** Execute in 8 ordered waves: (1) submodule removal, (2) CMake target/option rename, (3) namespace migration, (4) QML UI strings, (5) C++ source strings, (6) i18n files, (7) docs/skills/config, (8) build verification gate.

<user_constraints>
## User Constraints (from CONTEXT.md)

No CONTEXT.md found for Phase 1. All requirements from REQUIREMENTS.md apply directly.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| BRAND-01 | Application window title, about dialog, shortcut dialog: all "Creality Print" / "CrealityPrint" strings replaced with "OWzx" | ~20 string hits identified across 8 QML/C++ files; see String Residue Inventory below |
| BRAND-02 | Remove `third_party/CrealityPrint` submodule reference and all related CMake/config paths | Physical dir still exists; `.gitmodules` already switched; cmake comments + version vars remain |
| BRAND-03 | Internal namespace `Crality3D` / `creality` migrated to `OWzx` / `owzx` | 2 namespace declarations, ~15 qualified references across 6 files; QML module `CrealityGL` |
| BRAND-04 | Resource files (icons, configs) with Creality brand replaced with OWzx | hints.json has 4 entries; icons are generic SVGs (no brand logos found); QSettings keys |
| BRAND-05 | Startup/about dialog version aligned to OrcaSlicer main branch (no longer v7.0.1) | OrcaSlicer version.inc: `SoftFever_VERSION "2.4.0-dev"`, `SLIC3R_VERSION "02.06.00.51"` |
</phase_requirements>

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Window title / app identity | Frontend Server (main_qml.cpp) | Browser (QML bindings) | `setOrganizationName`, `setApplicationName`, `title:` property |
| Brand strings in UI | Browser (QML) | Frontend Server (C++ strings in Widgets pages) | QML `qsTr()` strings, hardcoded C++ strings in Widgets fallback pages |
| Namespace identifiers | API / Backend (C++ core) | -- | `namespace Crality3D` in rendering/viewmodel headers |
| CMake targets/options | Build System (CMake) | -- | Target names, option names, compile definitions |
| Translation strings | Browser (i18n .ts files) | -- | `qsTr()` source strings + translations |
| Version display | Frontend Server | Browser | AboutDialog.qml, HomePage.qml version strings |
| Submodule references | Build System (git/CMake) | -- | `.gitmodules`, cmake include paths |

## Standard Stack

This phase uses no external packages. It is a pure string-replacement and config migration task using:
- CMake (target/option rename)
- Git (submodule removal)
- Standard text editing tools
- The existing build system for verification

## Package Legitimacy Audit

> Not applicable -- Phase 1 installs zero external packages.

## Architecture Patterns

### System Architecture Diagram

```
[Brand Strings Inventory]
         |
         v
[Wave 1: Submodule Removal]
    .gitmodules (already done)
    third_party/CrealityPrint/ (physical dir still exists)
         |
         v
[Wave 2: CMake Rename]
    CREALITY_QML_GUI option -> OWZX_QML_GUI
    creality_app_core target -> owzx_app_core
    creality_cli_core target -> owzx_cli_core
    creality-cli executable -> owzx-cli executable
    CREALITYPRINT_VERSION vars -> OWZX_VERSION vars
    scripts: cmake flags, ninja target name
         |
         v
[Wave 3: Namespace Migration]
    namespace Crality3D -> namespace OWzx
    Crality3D:: qualified refs -> OWzx::
    CrealityGL QML module -> OWzxGL
         |
         v
[Wave 4: QML UI Strings]
    main.qml title/about
    AboutDialog.qml
    HomePage.qml
    PreferencesPage.qml
    hints.json
    ModelMallPage.qml
    PrintHostDialog.qml
    ConfigWizardDialog.qml
         |
         v
[Wave 5: C++ Source Strings]
    main_qml.cpp (org name, app name, env vars)
    BackendContext.cpp (env var)
    PresetServiceMock.cpp (preset names -- PRESERVE: these are upstream vendor names)
    ModelMallViewModel.cpp (comment only)
    ConfigOptionModel.cpp (comment only)
    PreviewViewModel.cpp (comment only)
    PreparePage.qml (comment only)
    AuxiliaryService.cpp (path)
    CloudServiceMock.cpp (email)
    CliRunner.cpp/cli.h/main_cli.cpp (app name/usage)
    MainWindow.cpp (QSettings)
         |
         v
[Wave 6: i18n Files]
    zh_CN.ts, en.ts, de.ts, fr.ts, ja.ts, ko.ts
         |
         v
[Wave 7: Docs/Skills/Config]
    docs/CrealityPrint_Qt_GUI重写架构.md (rename file? or update content)
    .claude/skills/migrating-source-truth/SKILL.md
    .claude/agents/source-truth-migration.md
    .claude/skills/analyzing-source-truth-gap/SKILL.md
    .claude/rules/qml-boundaries.md
    CLAUDE.md
    README.md
    cmake/BuildLibslic3r*.cmake
         |
         v
[Wave 8: Build Verification Gate]
    scripts/auto_verify_with_vcvars.ps1 passes
    grep assertion: zero 'creality' in src/ (with carve-outs)
```

### Recommended Task Groupings

```
T01: Remove CrealityPrint submodule (BRAND-02)
T02: Rename CMake targets and options (BRAND-02, BRAND-03 partial)
T03: Migrate Crality3D namespace to OWzx (BRAND-03)
T04: Replace brand strings in QML UI files (BRAND-01, BRAND-04)
T05: Replace brand strings in C++ source files (BRAND-01)
T06: Update i18n translation files (BRAND-01)
T07: Update docs, skills, and CLAUDE.md (BRAND-01, BRAND-02)
T08: Align version to OrcaSlicer main branch (BRAND-05)
T09: Rename CLI binary from creality-cli to owzx-cli (BRAND-01, BRAND-03)
T10: Build verification gate (all requirements)
```

## String Residue Inventory

### Category 1: Brand Strings (MUST REPLACE)

#### Window Title & Application Identity
| File | Line | Current String | Replacement | Note |
|------|------|----------------|-------------|------|
| `src/qml_gui/main.qml` | 16 | `"Creality Print 7.0 - QML"` | `"OWzx Slicer"` | Window title |
| `src/qml_gui/main_qml.cpp` | 103 | `setOrganizationName("CrealityDemo")` | `setOrganizationName("OWzx")` | QSettings org |
| `src/qml_gui/main_qml.cpp` | 104 | `setApplicationName("Print7Shell")` | `setApplicationName("OWzxSlicer")` | QSettings app |
| `src/MainWindow.cpp` | 198 | `QSettings("CrealityDemo", "Print7Shell")` | `QSettings("OWzx", "OWzxSlicer")` | Widgets fallback |
| `src/MainWindow.cpp` | 214 | `QSettings("CrealityDemo", "Print7Shell")` | `QSettings("OWzx", "OWzxSlicer")` | Widgets fallback |

#### About Dialog
| File | Line | Current String | Replacement |
|------|------|----------------|-------------|
| `src/qml_gui/main.qml` | 278 | `qsTr("关于 Creality Print")` | `qsTr("关于 OWzx")` |
| `src/qml_gui/main.qml` | 406 | `qsTr("关于 Creality Print")` | `qsTr("关于 OWzx")` |
| `src/qml_gui/main.qml` | 418 | `"Creality Print 7.0 - QML"` | `"OWzx Slicer"` |
| `src/qml_gui/main.qml` | 424 | `qsTr("基于 CrealityPrint v7.0.1 开源版本")` | `qsTr("基于 OrcaSlicer 开源版本")` |
| `src/qml_gui/dialogs/AboutDialog.qml` | 12 | `qsTr("关于 Creality Print")` | `qsTr("关于 OWzx")` |
| `src/qml_gui/dialogs/AboutDialog.qml` | 40 | `"Creality Print"` | `"OWzx Slicer"` |
| `src/qml_gui/dialogs/AboutDialog.qml` | 46 | `qsTr("版本 7.0.0.0  (Qt6 QML 重写)")` | Update to OrcaSlicer version |
| `src/qml_gui/dialogs/AboutDialog.qml` | 73 | `qsTr("官方网站");   value: "www.creality.com"` | `"www.orcaslicer.org"` or OWzx URL |

#### HomePage
| File | Line | Current String | Replacement |
|------|------|----------------|-------------|
| `src/qml_gui/pages/HomePage.qml` | 63 | `"Creality Print 7.0"` | `"OWzx Slicer"` |
| `src/qml_gui/pages/HomePage.qml` | 276 | `qsTr("登录 Creality 账号")` | `qsTr("登录 OWzx 账号")` |
| `src/qml_gui/pages/HomePage.qml` | 287 | `qsTr("登录 Creality 账号")` | `qsTr("登录 OWzx 账号")` |
| `src/qml_gui/pages/HomePage.qml` | 520 | `qsTr("版本 7.0.0  |  Qt 6.10  |  ©2026 Creality")` | `qsTr("版本 2.4.0  |  Qt 6.10  |  OWzx Slicer")` |

#### PreferencesPage
| File | Line | Current String | Replacement |
|------|------|----------------|-------------|
| `src/qml_gui/pages/PreferencesPage.qml` | 582 | `qsTr("上游基线：CrealityPrint v7.0.1 (0d4ac73)")` | `qsTr("上游基线：OrcaSlicer main branch")` |
| `src/qml_gui/pages/PreferencesPage.qml` | 636 | `qsTr("...连接 Creality 更新服务器...")` | `qsTr("...连接更新服务器...")` |

#### ModelMallPage
| File | Line | Current String | Replacement |
|------|------|----------------|-------------|
| `src/qml_gui/pages/ModelMallPage.qml` | 756 | `qsTr("Powered by Creality Cloud")` | `qsTr("Powered by OrcaSlicer Cloud")` |

#### PrintHostDialog
| File | Line | Current String | Replacement |
|------|------|----------------|-------------|
| `src/qml_gui/dialogs/PrintHostDialog.qml` | 23 | `presetName: "Creality CR-10 SE"` | `presetName: "OrcaSlicer Default"` |
| `src/qml_gui/dialogs/PrintHostDialog.qml` | 24 | `hostType: "Creality Cloud"` | `hostType: "OrcaSlicer Cloud"` |
| `src/qml_gui/dialogs/PrintHostDialog.qml` | 25 | `hostUrl: "https://api.creality.com"` | `hostUrl: "https://api.orcaslicer.org"` |
| `src/qml_gui/dialogs/PrintHostDialog.qml` | 69 | `"Creality Cloud"` in model | `"OrcaSlicer Cloud"` |

#### CLI
| File | Line | Current String | Replacement |
|------|------|----------------|-------------|
| `src/cli/CliRunner.cpp` | 52 | `"CrealityPrint CLI — headless slicing pipeline"` | `"OWzx CLI — headless slicing pipeline"` |
| `src/cli/CliRunner.cpp` | 152 | `"Usage: creality-cli [options]"` | `"Usage: owzx-cli [options]"` |
| `src/cli/main_cli.cpp` | 7 | `setApplicationName("creality-cli")` | `setApplicationName("owzx-cli")` |
| `src/cli/main_cli.cpp` | 8 | `setApplicationVersion("1.0.0")` | Update to OrcaSlicer version |

#### Other C++ Source
| File | Line | Current String | Replacement |
|------|------|----------------|-------------|
| `src/core/services/AuxiliaryService.cpp` | 14 | `"/creality_auxiliary"` | `"/owzx_auxiliary"` |
| `src/core/services/CloudServiceMock.cpp` | 45 | `"@creality.com"` | `"@owzx.com"` |
| `src/qml_gui/BackendContext.cpp` | 58 | `qgetenv("CREALITY_VISUAL_COMPARE_MODE")` | `qgetenv("OWZX_VISUAL_COMPARE_MODE")` |
| `src/qml_gui/main_qml.cpp` | 83 | `qEnvironmentVariableIsSet("CREALITY_OPENGL")` | `qEnvironmentVariableIsSet("OWZX_OPENGL")` |

#### Hints (JSON)
| File | Line | Current String | Replacement |
|------|------|----------------|-------------|
| `src/qml_gui/data/hints.json` | 88 | `"CrealityPrint 支持腔体温度设置..."` | `"OWzx 支持腔体温度设置..."` |
| `src/qml_gui/data/hints.json` | 94 | `"在 CrealityPrint 中可以使用内置校准..."` | `"可以使用内置校准..."` |
| `src/qml_gui/data/hints.json` | 100 | `"CrealityPrint 支持辅助部件冷却风扇..."` | `"OWzx 支持辅助部件冷却风扇..."` |
| `src/qml_gui/data/hints.json` | 106 | `"CrealityPrint 支持空气净化..."` | `"OWzx 支持空气净化..."` |

### Category 2: Namespace Identifiers (MUST MIGRATE)

#### `Crality3D` namespace (2 declarations + ~15 references)
| File | Usage |
|------|-------|
| `src/core/rendering/SupportPaintTypes.h:5` | `namespace Crality3D {` declaration |
| `src/core/rendering/SupportPaintTypes.h:162` | `} // namespace Crality3D` closing |
| `src/core/rendering/TickCodeTypes.h:5` | `namespace Crality3D {` declaration |
| `src/core/rendering/TickCodeTypes.h:26` | `} // namespace Crality3D` closing |
| `src/core/viewmodels/EditorViewModel.h:737` | `QList<Crality3D::ObjectPaintData>` |
| `src/core/viewmodels/EditorViewModel.cpp:539,549,555,558` | `Crality3D::SupportPaintState`, `Crality3D::ObjectPaintData` |
| `src/core/viewmodels/PreviewViewModel.h:222` | `QList<Crality3D::TickCode>` |
| `src/core/viewmodels/PreviewViewModel.cpp:1098,1100,1114,1116,1154,1156` | `Crality3D::TickCode`, `Crality3D::TickType` |

#### `CrealityGL` QML module
| File | Line | Current | Replacement |
|------|------|---------|-------------|
| `src/qml_gui/main_qml.cpp` | 112 | `qmlRegisterType<GLViewport>("CrealityGL", 1, 0, "GLViewport")` | `qmlRegisterType<GLViewport>("OWzxGL", 1, 0, "GLViewport")` |
| `src/qml_gui/pages/PreparePage.qml` | 10 | `import CrealityGL 1.0` | `import OWzxGL 1.0` |
| `src/qml_gui/pages/PreviewPage.qml` | 4 | `import CrealityGL 1.0` | `import OWzxGL 1.0` |
| `src/qml_gui/Renderer/GLViewport.h` | 10 | Comment: `CrealityGL 1.0` | Comment: `OWzxGL 1.0` |

### Category 3: Historical Comments (PRESERVE -- do not modify)
| File | Line | String | Reason |
|------|------|--------|--------|
| `src/cli/CliRunner.h` | 10 | `Aligns with upstream CLI::run() in CrealityPrint.cpp.` | Upstream code reference |
| `src/core/viewmodels/ModelMallViewModel.cpp` | 9 | `Categories aligned with upstream CrealityPrint model mall taxonomy` | Upstream reference |
| `src/core/viewmodels/PreviewViewModel.cpp` | 67 | `10-color Range_Colors from CrealityPrint GCodeViewer` | Upstream reference |
| `src/qml_gui/pages/PreparePage.qml` | 55 | `Keyboard shortcuts matching upstream CrealityPrint Plater` | Upstream reference |
| `src/qml_gui/Models/ConfigOptionModel.cpp` | 660 | `对齐上游 print_config_def + Creality vendor process preset keys` | Upstream reference |

**Decision needed:** These comments reference "CrealityPrint" as the upstream project name. Options:
- (A) Keep as-is -- they accurately describe the upstream origin
- (B) Replace with "OrcaSlicer" -- current upstream name
- (C) Add "(now OrcaSlicer)" annotation

Recommendation: (A) Preserve as-is. They are accurate historical references. The ROADMAP success criteria says "仅保留历史注释和 third_party 引用", which covers these.

### Category 4: Path References (MUST UPDATE)

#### CMake Scripts
| File | Line | Current | Replacement |
|------|------|---------|-------------|
| `cmake/BuildLibslic3r.cmake` | 6 | `third_party/CrealityPrint/src` | `third_party/OrcaSlicer/src` |
| `cmake/BuildLibslic3r.cmake` | 11 | `third_party/CrealityPrint/` | `third_party/OrcaSlicer/` |
| `cmake/BuildLibslic3rFromSource.cmake` | 6 | `third_party/CrealityPrint/src/libslic3r/CMakeLists.txt` | `third_party/OrcaSlicer/src/libslic3r/CMakeLists.txt` |

#### Test Data
| File | Line | Current | Replacement |
|------|------|---------|-------------|
| `tests/CliTests.cpp` | 20 | `"/creality-cli"` | `"/owzx-cli"` |
| `tests/CliTests.cpp` | 118 | `"creality_models/Block20XY.stl"` | `"test_models/Block20XY.stl"` |

#### Preset Paths (PRESERVE -- these are upstream vendor names)
| File | Lines | Context |
|------|-------|---------|
| `src/core/services/PresetServiceMock.cpp` | 210-234 | References to `Creality.json`, `Creality/` vendor subdirectory |

**These MUST be PRESERVED.** The upstream OrcaSlicer repository uses `Creality` as the vendor directory name in `resources/profiles/Creality/`. Renaming these paths would break preset loading.

### Category 5: Preset/Model Names (PRESERVE -- upstream vendor names)

| File | Lines | Strings | Reason |
|------|-------|---------|--------|
| `src/core/services/PresetServiceMock.cpp` | 45,65,86,105,124 | `"Creality K1C 0.4"`, `"Creality Ender-3 S1"`, `"Creality Generic PLA/ABS/PETG"` | Upstream vendor preset names |
| `src/qml_gui/Models/PresetListModel.cpp` | 9,21-23 | `"Creality K1C 0.4 nozzle"`, `"Creality K1 Max"`, `"Creality Ender-3 V3"` | Upstream vendor preset names |
| `src/core/viewmodels/ModelMallViewModel.cpp` | 49,85 | `"CrealityDesign"` author name | Upstream model author |
| `src/qml_gui/dialogs/ConfigWizardDialog.qml` | 118 | `"Creality K1C 0.4 nozzle"` | Upstream vendor preset name |
| `src/pages/PreparePage.cpp` | 65 | `"Creality K1C 0.4 nozzle"`, `"K1 Max 0.4 nozzle"` | Upstream vendor preset names (Widgets fallback) |
| `src/pages/PreparePage.cpp` | 178 | `"0.20mm Standard @Creality K1C"` | Upstream preset name |
| `src/pages/PreviewPage.cpp` | 67,96,173 | Same vendor preset names | Upstream preset names |

**These MUST NOT be changed.** They reflect actual Creality hardware vendor names and upstream preset names that users expect to see.

## Submodule Status

### Current State
- `.gitmodules`: Already contains only `third_party/OrcaSlicer` -- CrealityPrint entry removed [VERIFIED]
- Physical directory: `third_party/CrealityPrint/` still EXISTS on disk
- `git submodule status` returns error (submodules likely not initialized in this clone)

### Removal Sequence
1. `git submodule deinit -f third_party/CrealityPrint` (may be no-op if not initialized)
2. `git rm third_party/CrealityPrint` (removes from git tracking)
3. Delete physical directory `third_party/CrealityPrint/` if git rm didn't
4. `git add .gitmodules` and commit
5. Update all cmake/script references from `third_party/CrealityPrint` to `third_party/OrcaSlicer`

### Risk
- The CrealityPrint submodule was likely never properly initialized as a submodule in this repo (based on git submodule status error). It may be just a plain directory that was tracked. Confirm with `git ls-tree HEAD third_party/CrealityPrint` before attempting `git submodule deinit`.
- The `third_party/OrcaSlicer/` submodule IS properly configured in `.gitmodules`.

## Namespace Migration Strategy

### Files to Rename (none)
No files have `creality` or `Crality` in their filename. The namespace is purely internal.

### Migration Approach: Hard Rename (no alias)
The BRAND-03 requirement says "alias 阶段可省略" which means a backward-compat alias is optional. Since this is a full brand cleanup phase, recommend hard rename:

1. `namespace Crality3D` -> `namespace OWzx` in `SupportPaintTypes.h` and `TickCodeTypes.h`
2. All `Crality3D::` qualified references -> `OWzx::` in consuming files
3. `CrealityGL` QML module -> `OWzxGL` in registration + imports

### Impact Assessment
- `Crality3D` namespace used by: EditorViewModel (paint data), PreviewViewModel (tick codes)
- `CrealityGL` QML import used by: PreparePage.qml, PreviewPage.qml
- No external consumers (these are all internal to the project)
- Low risk: straightforward find-and-replace within `src/`

## Version Alignment (BRAND-05)

### Upstream OrcaSlicer Version
From `third_party/OrcaSlicer/version.inc`:
```
set(SLIC3R_APP_NAME "OrcaSlicer")
set(SoftFever_VERSION "2.4.0-dev")
set(SLIC3R_VERSION "02.06.00.51")
```

### Current Version Strings in Codebase
| Location | Current Value | Should Be |
|----------|---------------|-----------|
| `AboutDialog.qml` | `"版本 7.0.0.0  (Qt6 QML 重写)"` | `"版本 2.4.0-dev  (Qt6 QML)"` |
| `AboutDialog.qml` | `"2026-03-03"` build date | Dynamic build date |
| `main.qml` | `"Creality Print 7.0 - QML"` | `"OWzx Slicer"` (no hardcoded version in title) |
| `main.qml` | `"基于 CrealityPrint v7.0.1 开源版本"` | `"基于 OrcaSlicer 开源版本"` |
| `HomePage.qml` | `"Creality Print 7.0"` | `"OWzx Slicer"` |
| `HomePage.qml` | `"版本 7.0.0  |  Qt 6.10  |  ©2026 Creality"` | `"版本 2.4.0-dev  |  Qt 6.10  |  OWzx"` |
| `PreferencesPage.qml` | `"上游基线：CrealityPrint v7.0.1 (0d4ac73)"` | `"上游基线：OrcaSlicer 2.4.0-dev"` |
| `src/cli/main_cli.cpp` | `setApplicationVersion("1.0.0")` | `"2.4.0-dev"` |
| `cmake/BuildLibslic3rFromSource.cmake` | `CREALITYPRINT_VERSION "1.0.0"` | Should align to OrcaSlicer |

### Recommendation
- Use `SoftFever_VERSION "2.4.0-dev"` as the displayed version
- Remove commit hash references (they were CrealityPrint-specific)
- Make build date dynamic if possible (CMake `configure_file` with `@TODAY@`)

## CMake Target & Option Rename

### Current Names
| Current | Proposed | Files Affected |
|---------|----------|----------------|
| `option(CREALITY_QML_GUI ...)` | `option(OWZX_QML_GUI ...)` | CMakeLists.txt, cmake/*.cmake, scripts |
| `creality_app_core` (library target) | `owzx_app_core` | CMakeLists.txt (8+ references) |
| `creality_cli_core` (library target) | `owzx_cli_core` | CMakeLists.txt (5+ references) |
| `creality-cli` (executable target) | `owzx-cli` | CMakeLists.txt, scripts/auto_verify_with_vcvars.ps1, tests/CliTests.cpp |
| `CREALITY_QML_GUI=1` (compile def) | `OWZX_QML_GUI=1` | Propagated via target_compile_definitions |
| `CREALITYPRINT_VERSION` (cmake var) | `OWZX_VERSION` | cmake/BuildLibslic3rFromSource.cmake |
| `FramelessDialogDemo` (project name) | `OWzxSlicer` | CMakeLists.txt project() call |

**Risk:** The executable name `FramelessDialogDemo` is also the output binary name. Changing `project()` changes this to `OWzxSlicer.exe`. This is a brand cleanup win but will change the .exe name in `build/`.

**Risk:** `CREALITY_QML_GUI` is referenced in multiple cmake files and scripts. Must update ALL references atomically.

## i18n Translation Files

### Files to Update
| File | Hits | Content |
|------|------|---------|
| `i18n/zh_CN.ts` | 2 | `<name>CrealityPrint</name>`, preset name `@Creality K1C` (PRESERVE) |
| `i18n/en.ts` | 5 | About dialog, version string, preset name (PRESERVE) |
| `i18n/de.ts` | 1 | preset name (PRESERVE) |
| `i18n/fr.ts` | 1 | preset name (PRESERVE) |
| `i18n/ja.ts` | 1 | preset name (PRESERVE) |
| `i18n/ko.ts` | 1 | preset name (PRESERVE) |

### Strategy
- Update source strings first (QML/C++), then run `lupdate` to regenerate `.ts` files
- Preset names containing "Creality" are upstream vendor names -- DO NOT translate or replace these
- The `<name>CrealityPrint</name>` in zh_CN.ts is the translator comment context -- update to `OWzx`

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Find-and-replace across codebase | Manual regex one-liners | `grep` + targeted `sed`/Edit tool calls per category | Risk of collateral damage in third_party/ is high -- must scope replacements carefully |

## Runtime State Inventory

> This is a rename/refactor phase. Runtime state audit required.

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | QSettings keys stored in Windows Registry under `HKEY_CURRENT_USER\Software\CrealityDemo\Print7Shell` | Users will need to reconfigure after org name change. Old registry keys become orphaned (harmless). |
| Live service config | None | -- |
| OS-registered state | None (no installer, no Task Scheduler entries) | -- |
| Secrets/env vars | `CREALITY_VISUAL_COMPARE_MODE` env var name | Rename to `OWZX_VISUAL_COMPARE_MODE`. Document in migration notes. |
| Secrets/env vars | `CREALITY_OPENGL` env var name | Rename to `OWZX_OPENGL`. Document in migration notes. |
| Build artifacts | `build/FramelessDialogDemo.exe` -> becomes `build/OWzxSlicer.exe` | Automatic -- rebuild after project() rename |

**Nothing found in category:** Live service config, OS-registered state.

## Common Pitfalls

### Pitfall 1: Modifying third_party/OrcaSlicer/ by accident
**What goes wrong:** A careless `grep -rl "creality" | xargs sed` touches upstream source files.
**Why it happens:** grep default scope may include `third_party/OrcaSlicer/` which contains `CrealityDiscoveryDialog.cpp` referencing "Creality" as a hardware brand.
**How to avoid:** All replacements MUST be scoped to exclude `third_party/`. Use `--exclude-dir=third_party` or explicit file lists.
**Warning signs:** `git diff` showing changes in `third_party/`.

### Pitfall 2: Replacing upstream vendor preset names
**What goes wrong:** "Creality K1C" gets replaced to "OWzx K1C", breaking preset matching.
**Why it happens:** Blind regex replacement without categorizing strings first.
**How to avoid:** Use the String Residue Inventory above. Every replacement must be classified as "brand string" vs "vendor name" vs "historical comment".
**Warning signs:** PresetServiceMock fails to match vendor directories.

### Pitfall 3: Breaking QML module import after rename
**What goes wrong:** `import CrealityGL 1.0` removed but QML still uses `GLViewport` component.
**Why it happens:** Renaming the QML module registration in C++ without updating QML imports.
**How to avoid:** Change `CrealityGL` -> `OWzxGL` in both the C++ registration AND all QML import statements atomically.
**Warning signs:** QML runtime error "GLViewport is not a type".

### Pitfall 4: CMake option rename breaks build
**What goes wrong:** `CREALITY_QML_GUI` removed but cmake scripts still reference it.
**Why it happens:** The option is referenced in 5+ locations across CMakeLists.txt and cmake/*.cmake.
**How to avoid:** Change ALL references in a single atomic edit. Verify with `grep -rn "CREALITY_QML_GUI" . --exclude-dir=third_party --exclude-dir=build`.
**Warning signs:** cmake configure fails with unknown option.

### Pitfall 5: docs/ references create confusion after cleanup
**What goes wrong:** Docs still reference `third_party/CrealityPrint` but the directory no longer exists.
**Why it happens:** Historical docs (architecture, dependency audit) predate the upstream switch.
**How to avoid:** Update `docs/` files OR add a prominent "archived -- see third_party/OrcaSlicer" header.
**Warning signs:** New developers following outdated docs.

### Pitfall 6: .planning-v1-crealityprint-archive/ references
**What goes wrong:** Archive directory contains extensive CrealityPrint references that look like residue.
**Why it happens:** It IS historical archive -- must be preserved as-is.
**How to avoid:** Explicitly EXCLUDE `.planning-v1-crealityprint-archive/` from all replacement scopes.
**Warning signs:** Archive files modified by grep-replace.

## Code Examples

### Namespace Rename Pattern
```cpp
// Before:
namespace Crality3D {
enum class SupportPaintState : int8_t { None = 0, Enforcer = 1, Blocker = 2 };
struct ObjectPaintData { /* ... */ };
} // namespace Crality3D

// After:
namespace OWzx {
enum class SupportPaintState : int8_t { None = 0, Enforcer = 1, Blocker = 2 };
struct ObjectPaintData { /* ... */ };
} // namespace OWzx
```

### QML Module Rename Pattern
```cpp
// Before (src/qml_gui/main_qml.cpp:112):
qmlRegisterType<GLViewport>("CrealityGL", 1, 0, "GLViewport");

// After:
qmlRegisterType<GLViewport>("OWzxGL", 1, 0, "GLViewport");
```

### CMake Target Rename Pattern
```cmake
# Before:
option(CREALITY_QML_GUI "Build QML-first GUI architecture" ON)
add_library(creality_app_core OBJECT ...)
add_executable(creality-cli src/cli/main_cli.cpp)

# After:
option(OWZX_QML_GUI "Build QML-first GUI architecture" ON)
add_library(owzx_app_core OBJECT ...)
add_executable(owzx-cli src/cli/main_cli.cpp)
```

### Verification Grep Command
```bash
# After all replacements, verify zero brand strings outside carve-outs:
grep -ri "creality" src/ cmake/ i18n/ tests/ scripts/ CMakeLists.txt vcpkg.json README.md \
  --exclude-dir=third_party --exclude-dir=build \
  | grep -v "Crality3D\|CrealityGL\|third_party/\|@Creality\|Creality K1\|Creality Ender\|Creality Generic\|Creality CR-10\|CrealityDesign\|CrealityPrint.cpp\|Creality vendor\|fdm_creality_common\|Creality Cloud\|CrealityDiscovery"
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `third_party/CrealityPrint` submodule | `third_party/OrcaSlicer` submodule | 2026-06-11 (MEMORY.md) | `.gitmodules` already updated, physical dir still present |
| `namespace Crality3D` | Should be `namespace OWzx` | Pending | 6 source files affected |
| `CrealityGL` QML module | Should be `OWzxGL` | Pending | 3 files (1 C++, 2 QML) |
| `FramelessDialogDemo.exe` | Should be `OWzxSlicer.exe` | Pending | Changes project() name |
| `creality-cli.exe` | Should be `owzx-cli.exe` | Pending | CMake target + CLI source |

**Deprecated/outdated:**
- `CREALITY_QML_GUI` cmake option: Will be renamed to `OWZX_QML_GUI`
- `CREALITYPRINT_VERSION` cmake variables: Will be renamed/aligned to `OWZX_VERSION`
- `docs/CrealityPrint_Qt_GUI重写架构.md`: Consider renaming to reflect OrcaSlicer upstream

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Upstream preset vendor directory remains `Creality/` in OrcaSlicer repo | Category 4: Preset Paths | If OrcaSlicer renamed vendor dirs, preset loading would already be broken |
| A2 | `.planning-v1-crealityprint-archive/` should be preserved as-is | Common Pitfalls #6 | If user wants archive cleaned too, scope expands significantly |
| A3 | `FramelessDialogDemo` project name should become `OWzxSlicer` | CMake Target Rename | If user prefers keeping `FramelessDialogDemo` as internal codename |
| A4 | OrcaSlicer version `2.4.0-dev` from `version.inc` is current for main branch | Version Alignment | Main branch may have advanced since last pull |
| A5 | No icon/logo files contain Creality branding (only generic SVG icons found) | Category 5: Resources | A dedicated logo icon may exist elsewhere or be added later |

## Open Questions

1. **QSettings migration strategy**: Changing `setOrganizationName("CrealityDemo")` to `setOrganizationName("OWzx")` means all existing user preferences (window geometry, recent files, etc.) stored in the Windows Registry under `HKCU\Software\CrealityDemo\Print7Shell` will be orphaned. Should we:
   - Accept the loss (clean slate)?
   - Add a one-time migration to copy settings?
   - **Recommendation:** Accept the loss. This is a pre-release cleanup; there are no real end users yet.

2. **docs/ file treatment**: Several docs files reference `third_party/CrealityPrint` extensively (especially `docs/CrealityPrint_Qt_GUI重写架构.md` with 50+ references). Should we:
   - Update all paths to `third_party/OrcaSlicer`?
   - Rename `docs/CrealityPrint_Qt_GUI重写架构.md` -> `docs/OrcaSlicer_Qt_GUI重写架构.md`?
   - **Recommendation:** Update paths. Consider renaming the file in a follow-up, but path updates are essential for docs accuracy.

3. **CMake option backward compat**: Should we add `CREALITY_QML_GUI` as a deprecated alias for `OWZX_QML_GUI`?
   - **Recommendation:** No. Phase 1 is a clean break. No backward compat aliases needed.

4. **`FramelessDialogDemo` vs `OWzxSlicer` project name**: The current CMake project name `FramelessDialogDemo` is a leftover placeholder. Confirm rename to `OWzxSlicer`.

## Environment Availability

> Step 2.6: SKIPPED (no external dependencies -- this phase is pure code/config changes within the existing toolchain)

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Qt Test (QtTest) + CTest |
| Config file | CTest in root CMakeLists.txt |
| Quick run command | `cmake --build build --config Release && ctest --test-dir build -N` (dry-run) |
| Full suite command | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| BRAND-01 | Zero "Creality" brand strings in UI | grep assertion | `grep -ri "creality" src/ --exclude-dir=third_party \| grep -v "vendor\|upstream\|Creality K1\|Creality Ender\|Creality Generic\|Crality3D"` (expect empty) | Wave 0 |
| BRAND-02 | No CrealityPrint submodule references | grep assertion | `grep -ri "third_party/CrealityPrint" cmake/ scripts/ CMakeLists.txt` (expect empty) | Wave 0 |
| BRAND-03 | No Crality3D/CrealityGL references | grep assertion | `grep -ri "Crality3D\|CrealityGL" src/` (expect empty after rename) | Wave 0 |
| BRAND-04 | No Creality brand in resources | grep assertion | `grep -ri "creality" src/qml_gui/data/ --exclude-dir=third_party` (expect empty) | Wave 0 |
| BRAND-05 | Version string matches OrcaSlicer | grep assertion | `grep "7.0" src/qml_gui/dialogs/AboutDialog.qml src/qml_gui/pages/HomePage.qml` (expect empty) | Wave 0 |
| All | Build passes | build | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` | Existing |

### Sampling Rate
- **Per task commit:** `cmake --build build --config Release` (incremental compile)
- **Per wave merge:** Full grep assertion suite for completed requirements
- **Phase gate:** Full build pass via `scripts/auto_verify_with_vcvars.ps1` + all grep assertions

### Wave 0 Gaps
- [ ] No dedicated test file for brand string assertions -- must create a verification script or add to smoke tests
- [ ] Existing `tests/ViewModelSmokeTests.cpp` and `tests/E2EWorkflowTests.cpp` link against `creality_app_core` -- will need target rename

## Security Domain

> Phase 1 is brand cleanup only. No new security surfaces introduced. However, note:

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V5 Input Validation | No | N/A -- no user input changes |
| V6 Cryptography | No | N/A -- no crypto changes |
| V2 Authentication | No | N/A -- CloudServiceMock email domain is cosmetic |

### Known Risk: QSettings Key Migration
Changing the organization/application name changes where QSettings stores data. This is not a security risk but is a user-data continuity concern. See Open Questions #1.

## Sources

### Primary (HIGH confidence)
- Direct grep audit of codebase files (2026-06-15) -- all findings from `grep -ri "creality"` across `src/`, `cmake/`, `i18n/`, `tests/`, `scripts/`, `CMakeLists.txt`, `vcpkg.json`, `README.md`
- `third_party/OrcaSlicer/version.inc` -- upstream version verified

### Secondary (MEDIUM confidence)
- MEMORY.md -- upstream switch date confirmed
- `.gitmodules` -- submodule state verified
- `.planning/ROADMAP.md` and `.planning/REQUIREMENTS.md` -- phase scope verified

### Tertiary (LOW confidence)
- None -- all findings are directly verified from codebase

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- no external packages needed
- Architecture: HIGH -- pure string replacement/config migration
- Pitfalls: HIGH -- all identified via direct codebase audit
- Version alignment: MEDIUM -- depends on whether OrcaSlicer main branch has advanced

**Research date:** 2026-06-15
**Valid until:** 30 days (stable domain -- brand cleanup has no moving parts)
