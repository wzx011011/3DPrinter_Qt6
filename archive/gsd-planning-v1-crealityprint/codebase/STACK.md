# Technology Stack

**Analysis Date:** 2026-05-31

## Languages

**Primary:**
- C++17 - All native backend code, viewmodels, services, rendering, and the main Qt application. Used in `src/core/`, `src/qml_gui/`, `third_party/CrealityPrint/src/libslic3r/`.
- QML/JavaScript - All UI layer. Used in `src/qml_gui/pages/`, `src/qml_gui/components/`, `src/qml_gui/controls/`, `src/qml_gui/dialogs/`, `src/qml_gui/panels/`.

**Secondary:**
- C - Embedded dependencies compiled from source: miniz, qoi, semver, glu-libtess, mcut. Located in `third_party/CrealityPrint/src/`.
- Objective-C++ - macOS-specific sources in upstream (e.g., `MacUtils.mm`, `ModelIO.mm`). Not used in the Qt6 migration on Windows.
- PowerShell - Build scripts (`scripts/auto_verify_with_vcvars.ps1`, `scripts/smoke_test.ps1`, etc.).
- CMake - Build system configuration in `CMakeLists.txt`, `cmake/`.

## Runtime

**Environment:**
- Target platform: Windows 10/11 (win64), MSVC toolchain (Visual Studio 2022)
- CRT: Dynamic CRT (`/MD`) for the Qt6 executable; upstream libslic3r static libs use static CRT (`/MT`)
- MSVC runtime: `MultiThreaded$<$<CONFIG:Debug>:Debug>DLL` (CMP0091 policy)
- Build output: `build/FramelessDialogDemo.exe`

**Package Manager:**
- vcpkg for external dependency resolution (`vcpkg.json`)
- Lockfile: `vcpkg.json` baseline `6f29f12e82a8293156836ad81cc9bf5af41fe836`
- Upstream pre-built dependencies consumed from `E:/ai/3D-Printer/deps/build/OrcaSlicer_dep/usr/local` (via `DEPS_PREFIX`)

**Build System:**
- CMake 3.16+ minimum, CMake 3.21+ for Qt 6.10
- Generator: Ninja
- Build type: Release
- CMake options: `BUILD_LIBSLIC3R=ON`, `LIBSLIC3R_FROM_SOURCE=ON`, `CREALITY_QML_GUI=ON`
- Precompiled headers enabled for libslic3r (`pchheader.hpp`)

## Frameworks

**Core UI Framework:**
- Qt 6.10 - QML-first GUI architecture. Components used: Qt6::Qml, Qt6::Quick, Qt6::QuickControls2, Qt6::OpenGL, Qt6::Concurrent, Qt6::LinguistTools.
- Qt Quick Controls 2 "Basic" style (`src/qml_gui/qtquickcontrols2.conf`).
- QSGRendererInterface::OpenGL forced as graphics API (required for `QQuickFramebufferObject`).

**3D Rendering:**
- OpenGL (legacy GL) via Qt OpenGL module and custom `QQuickFramebufferObject` subclass (`src/qml_gui/Renderer/GLViewport`).
- Custom shader utilities in `src/core/rendering/GLShaderUtil`.

**Upstream GUI (reference only, not migrated):**
- wxWidgets 3.x - The original CrealityPrint GUI framework.
- GLEW - OpenGL extension loading in upstream.
- ImGui - Immediate mode GUI used in upstream for certain overlays.

**Testing:**
- Qt Test (`QtTest`) - Smoke tests in `tests/ViewModelSmokeTests.cpp`, using `QSignalSpy` for signal verification.
- CTest integration enabled in root `CMakeLists.txt` (`include(CTest)`).

**Build/Dev:**
- vcpkg for dependency management
- CMake with Ninja generator
- PowerShell for build automation scripts

## Key Dependencies

**Critical (vcpkg-managed):**
- Boost 1.84+ (filesystem, algorithm, spirit, log, property_tree, beast, thread, locale, regex, chrono, atomic, date-time, iostreams, program-options, nowide, assign, bimap, multi-index, geometry, endian, foreach, format, functional, lexical-cast, multiprecision, polygon, process, signals2, stacktrace, uuid) - Core infrastructure for libslic3r and slicer algorithms
- TBB (Threading Building Blocks) - Parallel computation for slicing and mesh processing
- OpenSSL - HTTPS/TLS for cloud communication (NetworkAgent), certificate handling
- CGAL - Computational geometry: mesh boolean ops, triangulation, cut surface (linked as separate `libslic3r_cgal` target)
- cereal - C++11 serialization (project files, configuration persistence)
- NLopt 1.4+ - Nonlinear optimization (used in slicing algorithms)
- zlib - Compression (STL processing, model I/O)
- libpng - PNG image handling
- libjpeg-turbo - JPEG image handling
- Expat - XML parsing

**Embedded (compiled from upstream source in `cmake/BuildDepsFromSource.cmake`):**
- admesh - STL mesh repair and processing
- clipper (v1) + clipper2 - Polygon clipping/offsetting
- glu-libtess - Tessellation
- mcut - Mesh cutting
- miniz - zlib replacement
- qoi - QOI image format
- semver - Semantic version parsing
- libnest2d - 2D bin packing (pre-built from upstream build)
- qhull / qhullcpp - Convex hull computation
- assimp - 3D model import (STL, OBJ, STEP, etc.)
- libigl - Geometry processing library
- cr_tpms - Closed-source TPMS infill library (delay-loaded DLL)

**Embedded header-only (from upstream `third_party/CrealityPrint/src/`):**
- Eigen - Linear algebra
- nlohmann/json - JSON parsing
- nanosvg - SVG parsing
- spline - Spline math
- stb_dxt - DXT texture compression
- ankerl/unordered_dense - Fast hash maps

**Not Available / Blocked:**
- OpenVDB - Link failure; required for hollow/support paint gizmos (HMS/VDB-dependent)
- FFmpeg - Not found; required for RTSP camera stream decoding (blocked)
- MetaRTC/WebRTC - Required for WebRTC camera streams (blocked)
- Paho MQTT C++ - Used in upstream for device communication (not yet migrated)

## Configuration

**Environment:**
- `CMAKE_PREFIX_PATH` - Must include Qt 6.10 path and pre-built deps path (`DEPS_PREFIX`)
- `Qt6_DIR` - Points to Qt 6.10 installation (`E:/Qt6.10` on local dev, installed via `jurplel/install-qt-action@v4` in CI)
- `DEPS_PREFIX` - Pre-built dependencies from upstream OrcaSlicer build (default: `E:/ai/3D-Printer/deps/build/OrcaSlicer_dep/usr/local`)
- `QML_DEBUG_LOG` - Set to enable QML debug logging to `startup_diagnostics.log`
- `QT_LOGGING_RULES` - Hardcoded to `qt.qml.binding=true;qt.qml.connections=true` in `src/qml_gui/main_qml.cpp`

**Build:**
- Root config: `CMakeLists.txt`
- libslic3r build: `cmake/BuildLibslic3rFromSource.cmake` (compiles from upstream source)
- Embedded deps build: `cmake/BuildDepsFromSource.cmake` (compiles miniz, qoi, mcut, etc. from source)
- Pre-built libslic3r import: `cmake/BuildLibslic3r.cmake` (alternative, not used by default)
- QML resource file: `src/qml_gui/qml.qrc`
- QML type registration: `src/qml_gui/qmldir`
- Qt Quick Controls 2 config: `src/qml_gui/qtquickcontrols2.conf`
- Generated headers: `libslic3r_version.h`, `buildinfo.h` (configured by CMake)

**CMake Compile Definitions (main target):**
- `CREALITY_QML_GUI=1` - QML GUI mode
- `HAS_LIBSLIC3R=1` - libslic3r available (when `BUILD_LIBSLIC3R=ON`)
- `BOOST_ALL_NO_LIB`, `BOOST_USE_WINAPI_VERSION=0x602`, `BOOST_SYSTEM_USE_UTF8` - Static Boost linking on MSVC
- `USE_TBB`, `TBB_USE_CAPTURED_EXCEPTION=0` - TBB configuration
- `NOMINMAX`, `UNICODE`, `_UNICODE` - Windows Unicode setup

**CMake Link Options (MSVC):**
- `/NODEFAULTLIB:LIBCMT`, `/NODEFAULTLIB:libcpmt` - Exclude static CRT to avoid conflict
- `/DELAYLOAD:TK*.dll` - OCCT libraries delay-loaded (only loaded when OCCT code paths are hit)
- `/DELAYLOAD:cr_tpms_library.dll` - TPMS library delay-loaded
- `delayimp.lib` - MSVC delay-load helper

**i18n:**
- Qt Linguist translations: `i18n/zh_CN.ts`, `i18n/en.ts`, `i18n/ja.ts`, `i18n/ko.ts`, `i18n/de.ts`, `i18n/fr.ts`
- QTranslator used for runtime language switching
- `engine->retranslate()` called on language change

## Platform Requirements

**Development:**
- Windows 10/11 (64-bit)
- Visual Studio 2022 with MSVC toolchain (vcvars64.bat)
- Qt 6.10 (Qt6::Qml, Qt6::Quick, Qt6::QuickControls2, Qt6::OpenGL, Qt6::Concurrent, Qt6::LinguistTools)
- CMake 3.16+ (3.21+ for Qt 6.10)
- Ninja build system
- vcpkg for dependency management
- Pre-built upstream dependencies from OrcaSlicer build
- OpenGL-capable GPU and drivers

**Production:**
- Windows 10/11 desktop application (WIN32_EXECUTABLE)
- Distributed as standalone `.zip` with `windeployqt`-collected DLLs
- OCCT DLLs excluded from release package (previously caused 0xC0000005 at startup)
- Qt deployment: `windeployqt --release --qmldir src` in CI

---

*Stack analysis: 2026-05-31*
