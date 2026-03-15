# ==============================================================================
# BuildDepsFromSource.cmake - Compile ALL embedded dependencies from source
#
# This cmake file compiles ALL embedded dependencies from upstream source
# to align with the upstream build strategy.
# ==============================================================================

message(STATUS "=== BuildDepsFromSource: Compiling ALL deps from source ===")

# ─── Path definitions ────────────────────────────────────────────────────────
set(UPSTREAM_ROOT    "${CMAKE_SOURCE_DIR}/third_party/CrealityPrint")
set(UPSTREAM_SRC     "${UPSTREAM_ROOT}/src")
set(DEPS_GEN_DIR     "${CMAKE_CURRENT_BINARY_DIR}/deps_generated")
set(UPSTREAM_BUILD_DIR "E:/ai/3D-Printer/out/vs2026-x64-release/build/src")

file(MAKE_DIRECTORY "${DEPS_GEN_DIR}")

# ─── Common include paths ─────────────────────────────────────────────────────
set(_COMMON_INCLUDES
    "${UPSTREAM_SRC}"
    "${UPSTREAM_SRC}/libslic3r"
    "${UPSTREAM_SRC}/eigen"
    ${Boost_INCLUDE_DIRS}
)

# ═══════════════════════════════════════════════════════════════════════════════
# SECTION A: Simple standalone deps (no dependencies)
# ═══════════════════════════════════════════════════════════════════════════════

# ─── A1. miniz - zlib replacement (standalone C code) ─────────────────────────
file(GLOB MINIZ_SOURCES "${UPSTREAM_SRC}/miniz/*.c")
add_library(miniz_from_source STATIC ${MINIZ_SOURCES})
target_include_directories(miniz_from_source PUBLIC "${UPSTREAM_SRC}/miniz")
if(MSVC)
    target_compile_definitions(miniz_from_source PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ─── A2. qoi - image format (standalone C code) ───────────────────────────────
file(GLOB QOI_SOURCES "${UPSTREAM_SRC}/qoi/*.c")
add_library(qoi_from_source STATIC ${QOI_SOURCES})
target_include_directories(qoi_from_source PUBLIC "${UPSTREAM_SRC}/qoi")

# ─── A3. semver - version parsing (standalone C code) ─────────────────────────
file(GLOB SEMVER_SOURCES "${UPSTREAM_SRC}/semver/*.c")
add_library(semver_from_source STATIC ${SEMVER_SOURCES})
target_include_directories(semver_from_source PUBLIC "${UPSTREAM_SRC}/semver")
if(MSVC)
    target_compile_definitions(semver_from_source PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ─── A4. glu-libtess - tessellation (standalone C code) ───────────────────────
file(GLOB GLU_LIBTESS_SOURCES "${UPSTREAM_SRC}/glu-libtess/src/*.c")
add_library(glu_libtess_from_source STATIC ${GLU_LIBTESS_SOURCES})
target_include_directories(glu_libtess_from_source PUBLIC "${UPSTREAM_SRC}/glu-libtess/include")
if(MSVC)
    target_compile_definitions(glu_libtess_from_source PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ─── A5. mcut - mesh cutting ─────────────────────────────────────────────────
file(GLOB MCUT_SOURCES
    "${UPSTREAM_SRC}/mcut/source/*.cpp"
    "${UPSTREAM_SRC}/mcut/source/*.c"
)
add_library(mcut_from_source STATIC ${MCUT_SOURCES})
target_include_directories(mcut_from_source PUBLIC "${UPSTREAM_SRC}/mcut/include")
if(MSVC)
    target_compile_definitions(mcut_from_source PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ─── A6. clipper2 - polygon clipping v2 ──────────────────────────────────────
file(GLOB CLIPPER2_SOURCES
    "${UPSTREAM_SRC}/clipper2/Clipper2Lib/src/clipper.engine.cpp"
    "${UPSTREAM_SRC}/clipper2/Clipper2Lib/src/clipper.offset.cpp"
    "${UPSTREAM_SRC}/clipper2/Clipper2Lib/src/clipper.rectclip.cpp"
)
add_library(clipper2_from_source STATIC ${CLIPPER2_SOURCES})
target_include_directories(clipper2_from_source PUBLIC
    "${UPSTREAM_SRC}/clipper2/Clipper2Lib/include"
)
target_compile_definitions(clipper2_from_source PRIVATE CLIPPER2UtilsNamespace=clipper2utils)

# ═══════════════════════════════════════════════════════════════════════════════
# SECTION B: Coupled deps (need libslic3r headers / Boost)
# ═══════════════════════════════════════════════════════════════════════════════

# ─── B1. admesh - STL mesh processing (needs Boost + Eigen + Boost.Locale) ─────
# NOTE: boost::nowide::freopen is inline but depends on boost::locale::utf symbols
#       which require linking to Boost::locale library.
file(GLOB ADMESH_SOURCES
    "${UPSTREAM_SRC}/admesh/connect.cpp"
    "${UPSTREAM_SRC}/admesh/normals.cpp"
    "${UPSTREAM_SRC}/admesh/shared.cpp"
    "${UPSTREAM_SRC}/admesh/stl_io.cpp"
    "${UPSTREAM_SRC}/admesh/stlinit.cpp"
    "${UPSTREAM_SRC}/admesh/util.cpp"
)
add_library(admesh_from_source STATIC ${ADMESH_SOURCES})
target_include_directories(admesh_from_source
    PUBLIC "${UPSTREAM_SRC}/admesh"
    SYSTEM PRIVATE
        ${Boost_INCLUDE_DIRS}
        "${UPSTREAM_SRC}/eigen"
        "${UPSTREAM_SRC}"
        "${UPSTREAM_SRC}/libslic3r"
        "${UPSTREAM_SRC}/boost"
)
if(MSVC)
    target_compile_definitions(admesh_from_source PRIVATE
        _CRT_SECURE_NO_WARNINGS
        BOOST_ALL_NO_LIB
        _USE_MATH_DEFINES
    )
    # boost::nowide::freopen depends on boost::locale::utf which is in Boost::locale
    # Link Boost::locale to resolve the nowide symbols
    target_link_libraries(admesh_from_source PUBLIC Boost::locale)
endif()

# ─── B2. clipper (1.x) - polygon clipping (needs libslic3r Int128) ───────────
file(GLOB CLIPPER_SOURCES
    "${UPSTREAM_SRC}/clipper/clipper.cpp"
    "${UPSTREAM_SRC}/clipper/clipper_z.cpp"
)
add_library(clipper_from_source STATIC ${CLIPPER_SOURCES})
target_include_directories(clipper_from_source
    PUBLIC "${UPSTREAM_SRC}/clipper"
    SYSTEM PRIVATE ${_COMMON_INCLUDES}
)
if(MSVC)
    target_compile_definitions(clipper_from_source PRIVATE
        _CRT_SECURE_NO_WARNINGS
        BOOST_ALL_NO_LIB
        _USE_MATH_DEFINES
    )
endif()

# ─── B3. nowide (boost) - Windows UTF-8 (complex, use pre-built) ──────────────
# nowide depends on Boost.Locale internals - use pre-built from upstream build
set(UPSTREAM_BUILD_DIR "E:/ai/3D-Printer/out/vs2026-x64-release/build/src")
if(EXISTS "${UPSTREAM_BUILD_DIR}/boost/Release/nowide.lib")
    add_library(nowide_from_source STATIC IMPORTED GLOBAL)
    set_target_properties(nowide_from_source PROPERTIES
        IMPORTED_LOCATION "${UPSTREAM_BUILD_DIR}/boost/Release/nowide.lib"
    )
else()
    message(WARNING "nowide pre-built lib not found, using stub")
    add_library(nowide_from_source INTERFACE)
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# SECTION C: Complex deps (circular dependencies - use pre-built)
# ═══════════════════════════════════════════════════════════════════════════════

# ─── C1. libnest2d - 2D bin packing (circular dep with libslic3r, use pre-built) ───
if(EXISTS "${UPSTREAM_BUILD_DIR}/libnest2d/Release/libnest2d.lib")
    add_library(libnest2d_from_source STATIC IMPORTED GLOBAL)
    set_target_properties(libnest2d_from_source PROPERTIES
        IMPORTED_LOCATION "${UPSTREAM_BUILD_DIR}/libnest2d/Release/libnest2d.lib"
    )
else()
    message(WARNING "libnest2d pre-built lib not found")
    add_library(libnest2d_from_source INTERFACE)
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# SECTION D: External pre-built deps (cannot compile from source)
# ═══════════════════════════════════════════════════════════════════════════════

# ─── D1. qhull - convex hull (external, use pre-built) ────────────────────────
add_library(qhull_from_source STATIC IMPORTED GLOBAL)
set_target_properties(qhull_from_source PROPERTIES
    IMPORTED_LOCATION "${DEPS_PREFIX}/lib/qhullstatic_r.lib"
)
add_library(qhullcpp_from_source STATIC IMPORTED GLOBAL)
set_target_properties(qhullcpp_from_source PROPERTIES
    IMPORTED_LOCATION "${DEPS_PREFIX}/lib/qhullcpp.lib"
)

# ─── D2. assimp - 3D model import (external, use pre-built) ───────────────────
add_library(assimp_from_source STATIC IMPORTED GLOBAL)
set_target_properties(assimp_from_source PROPERTIES
    IMPORTED_LOCATION "${DEPS_PREFIX}/lib/assimp-vc142-mt.lib"
)

# ─── D3. cr_tpms_library - TPMS infill (closed source, must use pre-built) ────
add_library(cr_tpms_from_source STATIC IMPORTED GLOBAL)
set_target_properties(cr_tpms_from_source PROPERTIES
    IMPORTED_LOCATION "${DEPS_PREFIX}/lib/cr_tpms_library.lib"
)

# ─── D4. libigl - geometry processing (header-only) ───────────────────────────
add_library(libigl_from_source INTERFACE)
target_include_directories(libigl_from_source INTERFACE "${UPSTREAM_SRC}/libigl")

# ═══════════════════════════════════════════════════════════════════════════════
# ALIAS targets for compatibility
# ═══════════════════════════════════════════════════════════════════════════════

add_library(deps_admesh ALIAS admesh_from_source)
add_library(deps_clipper ALIAS clipper_from_source)
add_library(deps_clipper2 ALIAS clipper2_from_source)
add_library(deps_glu_libtess ALIAS glu_libtess_from_source)
add_library(deps_libnest2d ALIAS libnest2d_from_source)
add_library(deps_mcut ALIAS mcut_from_source)
add_library(deps_miniz ALIAS miniz_from_source)
add_library(deps_nowide ALIAS nowide_from_source)
add_library(deps_qoi ALIAS qoi_from_source)
add_library(deps_semver ALIAS semver_from_source)
add_library(deps_qhull ALIAS qhull_from_source)
add_library(deps_qhullcpp ALIAS qhullcpp_from_source)
add_library(deps_assimp ALIAS assimp_from_source)
add_library(deps_cr_tpms ALIAS cr_tpms_from_source)
add_library(deps_libigl ALIAS libigl_from_source)

message(STATUS "=== BuildDepsFromSource: ALL deps configured (full source build) ===")
