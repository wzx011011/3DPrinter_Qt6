# ==============================================================================
# BuildDepsFromSource.cmake - Compile all embedded dependencies from source
#
# This cmake file compiles all embedded dependencies from upstream source
# instead of importing pre-built .lib files.
#
# Strategy:
# - Simple standalone deps: compile from source (qoi, miniz, semver, glu-libtess, mcut, clipper2)
# - Complex/coupled deps: use pre-built from upstream build (admesh, clipper, libnest2d, nowide)
# - External deps: use pre-built from deps (qhull, assimp, cr_tpms)
# ==============================================================================

message(STATUS "=== BuildDepsFromSource: Compiling embedded deps from source ===")

# ─── Path definitions ────────────────────────────────────────────────────────
set(UPSTREAM_ROOT    "${CMAKE_SOURCE_DIR}/third_party/CrealityPrint")
set(UPSTREAM_SRC     "${UPSTREAM_ROOT}/src")
set(DEPS_GEN_DIR     "${CMAKE_CURRENT_BINARY_DIR}/deps_generated")
set(UPSTREAM_BUILD   "E:/ai/3D-Printer/out/vs2026-x64-release/build/src")

file(MAKE_DIRECTORY "${DEPS_GEN_DIR}")

# ═══════════════════════════════════════════════════════════════════════════════
# SECTION A: Compile from source (standalone deps)
# ═══════════════════════════════════════════════════════════════════════════════

# ─── A1. clipper2 - polygon clipping v2 (standalone) ──────────────────────────
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

# ─── A2. glu-libtess - tessellation (standalone C code) ───────────────────────
file(GLOB GLU_LIBTESS_SOURCES "${UPSTREAM_SRC}/glu-libtess/src/*.c")
add_library(glu_libtess_from_source STATIC ${GLU_LIBTESS_SOURCES})
target_include_directories(glu_libtess_from_source PUBLIC "${UPSTREAM_SRC}/glu-libtess/include")
if(MSVC)
    target_compile_definitions(glu_libtess_from_source PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ─── A3. mcut - mesh cutting ─────────────────────────────────────────────────
file(GLOB MCUT_SOURCES
    "${UPSTREAM_SRC}/mcut/source/*.cpp"
    "${UPSTREAM_SRC}/mcut/source/*.c"
)
add_library(mcut_from_source STATIC ${MCUT_SOURCES})
target_include_directories(mcut_from_source PUBLIC "${UPSTREAM_SRC}/mcut/include")
if(MSVC)
    target_compile_definitions(mcut_from_source PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ─── A4. miniz - zlib replacement (standalone C code) ─────────────────────────
file(GLOB MINIZ_SOURCES "${UPSTREAM_SRC}/miniz/*.c")
add_library(miniz_from_source STATIC ${MINIZ_SOURCES})
target_include_directories(miniz_from_source PUBLIC "${UPSTREAM_SRC}/miniz")
if(MSVC)
    target_compile_definitions(miniz_from_source PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ─── A5. qoi - image format (standalone C code) ───────────────────────────────
file(GLOB QOI_SOURCES "${UPSTREAM_SRC}/qoi/*.c")
add_library(qoi_from_source STATIC ${QOI_SOURCES})
target_include_directories(qoi_from_source PUBLIC "${UPSTREAM_SRC}/qoi")

# ─── A6. semver - version parsing (standalone C code) ─────────────────────────
file(GLOB SEMVER_SOURCES "${UPSTREAM_SRC}/semver/*.c")
add_library(semver_from_source STATIC ${SEMVER_SOURCES})
target_include_directories(semver_from_source PUBLIC "${UPSTREAM_SRC}/semver")
if(MSVC)
    target_compile_definitions(semver_from_source PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# SECTION B: Use pre-built from upstream build (coupled deps)
# These deps have been modified by upstream and depend on libslic3r headers
# ═══════════════════════════════════════════════════════════════════════════════

# ─── B1. admesh - STL mesh processing (coupled with libslic3r) ───────────────
if(EXISTS "${UPSTREAM_BUILD}/admesh/Release/admesh.lib")
    add_library(admesh_from_source STATIC IMPORTED GLOBAL)
    set_target_properties(admesh_from_source PROPERTIES
        IMPORTED_LOCATION "${UPSTREAM_BUILD}/admesh/Release/admesh.lib"
    )
else()
    message(WARNING "admesh pre-built lib not found")
    add_library(admesh_from_source INTERFACE)
endif()

# ─── B2. clipper (1.x) - polygon clipping (coupled with libslic3r) ───────────
if(EXISTS "${UPSTREAM_BUILD}/clipper/Release/clipper.lib")
    add_library(clipper_from_source STATIC IMPORTED GLOBAL)
    set_target_properties(clipper_from_source PROPERTIES
        IMPORTED_LOCATION "${UPSTREAM_BUILD}/clipper/Release/clipper.lib"
    )
else()
    message(WARNING "clipper pre-built lib not found")
    add_library(clipper_from_source INTERFACE)
endif()

# ─── B3. libnest2d - 2D bin packing (complex template library) ───────────────
if(EXISTS "${UPSTREAM_BUILD}/libnest2d/Release/libnest2d.lib")
    add_library(libnest2d_from_source STATIC IMPORTED GLOBAL)
    set_target_properties(libnest2d_from_source PROPERTIES
        IMPORTED_LOCATION "${UPSTREAM_BUILD}/libnest2d/Release/libnest2d.lib"
    )
else()
    message(WARNING "libnest2d pre-built lib not found")
    add_library(libnest2d_from_source INTERFACE)
endif()

# ─── B4. nowide (boost) - Windows UTF-8 (tightly coupled with Boost) ─────────
if(EXISTS "${UPSTREAM_BUILD}/boost/Release/nowide.lib")
    add_library(nowide_from_source STATIC IMPORTED GLOBAL)
    set_target_properties(nowide_from_source PROPERTIES
        IMPORTED_LOCATION "${UPSTREAM_BUILD}/boost/Release/nowide.lib"
    )
else()
    message(WARNING "nowide pre-built lib not found")
    add_library(nowide_from_source INTERFACE)
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# SECTION C: External pre-built deps (from deps prefix)
# ═══════════════════════════════════════════════════════════════════════════════

# ─── C1. qhull - convex hull ──────────────────────────────────────────────────
add_library(qhull_from_source STATIC IMPORTED GLOBAL)
set_target_properties(qhull_from_source PROPERTIES
    IMPORTED_LOCATION "${DEPS_PREFIX}/lib/qhullstatic_r.lib"
)
add_library(qhullcpp_from_source STATIC IMPORTED GLOBAL)
set_target_properties(qhullcpp_from_source PROPERTIES
    IMPORTED_LOCATION "${DEPS_PREFIX}/lib/qhullcpp.lib"
)

# ─── C2. assimp - 3D model import ─────────────────────────────────────────────
add_library(assimp_from_source STATIC IMPORTED GLOBAL)
set_target_properties(assimp_from_source PROPERTIES
    IMPORTED_LOCATION "${DEPS_PREFIX}/lib/assimp-vc142-mt.lib"
)

# ─── C3. cr_tpms_library - TPMS infill (closed source) ────────────────────────
add_library(cr_tpms_from_source STATIC IMPORTED GLOBAL)
set_target_properties(cr_tpms_from_source PROPERTIES
    IMPORTED_LOCATION "${DEPS_PREFIX}/lib/cr_tpms_library.lib"
)

# ─── C4. libigl - geometry processing (header-only) ───────────────────────────
add_library(libigl_from_source INTERFACE)
target_include_directories(libigl_from_source INTERFACE "${UPSTREAM_SRC}/libigl")

# ─── Create ALIAS targets for compatibility ───────────────────────────────────
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

message(STATUS "=== BuildDepsFromSource: All deps configured ===")
