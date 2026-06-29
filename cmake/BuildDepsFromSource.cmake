# ==============================================================================
# BuildDepsFromSource.cmake - Compile embedded dependencies from upstream source
#
# Aligned with OrcaSlicer's deps_src/CMakeLists.txt target definitions.
# OrcaSlicer places deps in deps_src/ and uses find_package(Eigen3) for Eigen.
# ==============================================================================

message(STATUS "=== BuildDepsFromSource: Compiling deps from source ===")

# ─── Path definitions ────────────────────────────────────────────────────────
set(UPSTREAM_ROOT    "${CMAKE_SOURCE_DIR}/third_party/OrcaSlicer")
set(UPSTREAM_SRC     "${UPSTREAM_ROOT}/src")
set(UPSTREAM_DEPS    "${UPSTREAM_ROOT}/deps_src")
set(DEPS_GEN_DIR     "${CMAKE_CURRENT_BINARY_DIR}/deps_generated")
set(UPSTREAM_BUILD_DIR "E:/ai/3D-Printer/out/vs2026-x64-release/build/src")

file(MAKE_DIRECTORY "${DEPS_GEN_DIR}")

# ─── Eigen3 (external, same as OrcaSlicer upstream) ──────────────────────────
find_package(Eigen3 5.0 CONFIG REQUIRED)

# ═══════════════════════════════════════════════════════════════════════════════
# SECTION A: Header-only INTERFACE libraries (matching upstream CMakeLists)
# ═══════════════════════════════════════════════════════════════════════════════

add_library(ankerl_from_source INTERFACE)
target_include_directories(ankerl_from_source SYSTEM INTERFACE "${UPSTREAM_DEPS}/ankerl")

add_library(fast_float_from_source INTERFACE)
target_include_directories(fast_float_from_source SYSTEM INTERFACE "${UPSTREAM_DEPS}/fast_float")

add_library(nlohmann_from_source INTERFACE)
target_include_directories(nlohmann_from_source SYSTEM INTERFACE "${UPSTREAM_DEPS}/nlohmann")

add_library(nanosvg_from_source INTERFACE)
target_include_directories(nanosvg_from_source SYSTEM INTERFACE "${UPSTREAM_DEPS}/nanosvg")

add_library(stb_dxt_from_source INTERFACE)
target_include_directories(stb_dxt_from_source SYSTEM INTERFACE "${UPSTREAM_DEPS}/stb_dxt")

add_library(libigl_from_source INTERFACE)
target_include_directories(libigl_from_source SYSTEM INTERFACE "${UPSTREAM_DEPS}/libigl")

# ═══════════════════════════════════════════════════════════════════════════════
# SECTION B: Simple standalone deps (C/C++ static libs)
# ═══════════════════════════════════════════════════════════════════════════════

# ─── miniz ────────────────────────────────────────────────────────────────────
file(GLOB MINIZ_SOURCES "${UPSTREAM_DEPS}/miniz/miniz.c")
add_library(miniz_from_source STATIC ${MINIZ_SOURCES})
target_include_directories(miniz_from_source SYSTEM PUBLIC "${UPSTREAM_DEPS}/miniz")
if(MSVC)
    target_compile_definitions(miniz_from_source PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ─── qoi ──────────────────────────────────────────────────────────────────────
file(GLOB QOI_SOURCES "${UPSTREAM_DEPS}/qoi/qoilib.c")
add_library(qoi_from_source STATIC ${QOI_SOURCES})
target_include_directories(qoi_from_source SYSTEM PUBLIC "${UPSTREAM_DEPS}/qoi")

# ─── semver ───────────────────────────────────────────────────────────────────
file(GLOB SEMVER_SOURCES "${UPSTREAM_DEPS}/semver/semver.c")
add_library(semver_from_source STATIC ${SEMVER_SOURCES})
target_include_directories(semver_from_source SYSTEM
    PUBLIC
        $<BUILD_INTERFACE:${UPSTREAM_DEPS}/semver>
        $<INSTALL_INTERFACE:include>
)
set_target_properties(semver_from_source PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_definitions(semver_from_source PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ─── glu-libtess ──────────────────────────────────────────────────────────────
file(GLOB GLU_LIBTESS_SOURCES "${UPSTREAM_DEPS}/glu-libtess/src/*.c")
add_library(glu_libtess_from_source STATIC ${GLU_LIBTESS_SOURCES})
target_include_directories(glu_libtess_from_source SYSTEM
    PRIVATE "${UPSTREAM_DEPS}/glu-libtess"
    PUBLIC  "${UPSTREAM_DEPS}/glu-libtess/include"
)
if(MSVC)
    target_compile_definitions(glu_libtess_from_source PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ─── mcut ─────────────────────────────────────────────────────────────────────
file(GLOB MCUT_SOURCES
    "${UPSTREAM_DEPS}/mcut/source/*.cpp"
    "${UPSTREAM_DEPS}/mcut/source/*.c"
)
add_library(mcut_from_source STATIC ${MCUT_SOURCES})
target_include_directories(mcut_from_source SYSTEM PRIVATE "${UPSTREAM_DEPS}/mcut/include")
if(MSVC)
    target_compile_definitions(mcut_from_source PRIVATE
        _CRT_SECURE_NO_WARNINGS
        MCUT_WITH_COMPUTE_HELPER_THREADPOOL=1
    )
    target_compile_options(mcut_from_source PRIVATE /W4 /wd26812 /wd4131 /wd4704 /wd4245 /wd4297 /wd4505 /wd4701 /bigobj)
endif()

# ─── clipper2 ─────────────────────────────────────────────────────────────────
file(GLOB CLIPPER2_SOURCES
    "${UPSTREAM_DEPS}/clipper2/Clipper2Lib/src/clipper.engine.cpp"
    "${UPSTREAM_DEPS}/clipper2/Clipper2Lib/src/clipper.offset.cpp"
    "${UPSTREAM_DEPS}/clipper2/Clipper2Lib/src/clipper.rectclip.cpp"
    "${UPSTREAM_DEPS}/clipper2/Clipper2Lib/src/clipper2_z.cpp"
)
add_library(clipper2_from_source STATIC ${CLIPPER2_SOURCES})
target_include_directories(clipper2_from_source SYSTEM
    PUBLIC "${UPSTREAM_DEPS}/clipper2/Clipper2Lib/include"
)
target_compile_definitions(clipper2_from_source PRIVATE CLIPPER2UtilsNamespace=clipper2utils)

# ═══════════════════════════════════════════════════════════════════════════════
# SECTION C: Coupled deps (need Eigen3 / TBB / Boost — matching upstream)
# ═══════════════════════════════════════════════════════════════════════════════

# ─── admesh (links Eigen3::Eigen, PUBLIC include = self + parent) ──────────────
file(GLOB ADMESH_SOURCES
    "${UPSTREAM_DEPS}/admesh/connect.cpp"
    "${UPSTREAM_DEPS}/admesh/normals.cpp"
    "${UPSTREAM_DEPS}/admesh/shared.cpp"
    "${UPSTREAM_DEPS}/admesh/stl_io.cpp"
    "${UPSTREAM_DEPS}/admesh/stlinit.cpp"
    "${UPSTREAM_DEPS}/admesh/util.cpp"
)
add_library(admesh_from_source STATIC ${ADMESH_SOURCES})
target_include_directories(admesh_from_source SYSTEM
    PUBLIC
        "${UPSTREAM_DEPS}/admesh"
        "${UPSTREAM_DEPS}"           # For #include <admesh/stl.h> from consumers
    PRIVATE
        "${UPSTREAM_SRC}"            # For #include <libslic3r/xxx> (shared.cpp, stlinit.cpp)
)
target_link_libraries(admesh_from_source
    PUBLIC Eigen3::Eigen
    PRIVATE Boost::locale
)
if(MSVC)
    target_compile_definitions(admesh_from_source PRIVATE
        _CRT_SECURE_NO_WARNINGS
        BOOST_ALL_NO_LIB
        _USE_MATH_DEFINES
    )
endif()

# ─── clipper v1 (links Eigen3::Eigen + TBB, matching upstream) ───────────────
file(GLOB CLIPPER_SOURCES
    "${UPSTREAM_DEPS}/clipper/clipper_z.cpp"
)
add_library(clipper_from_source STATIC ${CLIPPER_SOURCES})
target_include_directories(clipper_from_source SYSTEM
    PUBLIC "${UPSTREAM_DEPS}/clipper"
    PRIVATE "${UPSTREAM_SRC}"        # For #include <libslic3r/Int128.hpp> (via clipper_z.cpp → clipper.cpp)
)
target_link_libraries(clipper_from_source
    PUBLIC Eigen3::Eigen
    PRIVATE TBB::tbb TBB::tbbmalloc
)
if(MSVC)
    target_compile_definitions(clipper_from_source PRIVATE
        _CRT_SECURE_NO_WARNINGS
        BOOST_ALL_NO_LIB
        _USE_MATH_DEFINES
    )
endif()

# ─── nowide (pre-built from upstream deps) ─────────────────────────────────────
# Use the pre-built Boost nowide lib which contains the nowide::detail symbols
set(NOWIDE_LIB "${DEPS_PREFIX}/lib/libboost_nowide-vc142-mt-x64-1_84.lib")
if(EXISTS "${NOWIDE_LIB}")
    add_library(nowide_from_source STATIC IMPORTED GLOBAL)
    set_target_properties(nowide_from_source PROPERTIES
        IMPORTED_LOCATION "${NOWIDE_LIB}"
        INTERFACE_LINK_LIBRARIES "Boost::locale"
    )
else()
    message(WARNING "nowide pre-built lib not found at ${NOWIDE_LIB}, using stub")
    add_library(nowide_from_source INTERFACE)
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# SECTION D: Pre-built external deps (cannot compile from source)
# ═══════════════════════════════════════════════════════════════════════════════

# libnest2d (circular dep with libslic3r, use pre-built)
# Set INTERFACE_INCLUDE_DIRECTORIES so consumers get the header path
# (matches upstream's target_include_directories(libnest2d SYSTEM PUBLIC include))
if(EXISTS "${UPSTREAM_BUILD_DIR}/libnest2d/Release/libnest2d.lib")
    add_library(libnest2d_from_source STATIC IMPORTED GLOBAL)
    set_target_properties(libnest2d_from_source PROPERTIES
        IMPORTED_LOCATION "${UPSTREAM_BUILD_DIR}/libnest2d/Release/libnest2d.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${UPSTREAM_DEPS}/libnest2d/include"
        INTERFACE_COMPILE_DEFINITIONS "LIBNEST2D_THREADING_tbb;LIBNEST2D_STATIC;LIBNEST2D_OPTIMIZER_nlopt;LIBNEST2D_GEOMETRIES_libslic3r"
    )
else()
    message(WARNING "libnest2d pre-built lib not found")
    add_library(libnest2d_from_source INTERFACE)
    target_include_directories(libnest2d_from_source SYSTEM INTERFACE "${UPSTREAM_DEPS}/libnest2d/include")
    target_compile_definitions(libnest2d_from_source INTERFACE LIBNEST2D_THREADING_tbb LIBNEST2D_STATIC LIBNEST2D_OPTIMIZER_nlopt LIBNEST2D_GEOMETRIES_libslic3r)
endif()

# qhull
if(EXISTS "${DEPS_PREFIX}/lib/qhullstatic_r.lib")
    add_library(qhull_from_source STATIC IMPORTED GLOBAL)
    set_target_properties(qhull_from_source PROPERTIES
        IMPORTED_LOCATION "${DEPS_PREFIX}/lib/qhullstatic_r.lib"
    )
    if(EXISTS "${DEPS_PREFIX}/lib/qhullcpp.lib")
        add_library(qhullcpp_from_source STATIC IMPORTED GLOBAL)
        set_target_properties(qhullcpp_from_source PROPERTIES
            IMPORTED_LOCATION "${DEPS_PREFIX}/lib/qhullcpp.lib"
        )
    endif()
else()
    add_subdirectory("${UPSTREAM_DEPS}/qhull" "${DEPS_GEN_DIR}/qhull")
    add_library(qhull_from_source INTERFACE)
    target_link_libraries(qhull_from_source INTERFACE qhull)
endif()

# assimp
if(EXISTS "${DEPS_PREFIX}/lib/assimp-vc142-mt.lib")
    add_library(assimp_from_source STATIC IMPORTED GLOBAL)
    set_target_properties(assimp_from_source PROPERTIES
        IMPORTED_LOCATION "${DEPS_PREFIX}/lib/assimp-vc142-mt.lib"
    )
else()
    message(WARNING "assimp pre-built lib not found at ${DEPS_PREFIX}/lib/assimp-vc142-mt.lib")
endif()

# cr_tpms_library (closed source)
if(EXISTS "${DEPS_PREFIX}/lib/cr_tpms_library.lib")
    add_library(cr_tpms_from_source STATIC IMPORTED GLOBAL)
    set_target_properties(cr_tpms_from_source PROPERTIES
        IMPORTED_LOCATION "${DEPS_PREFIX}/lib/cr_tpms_library.lib"
    )
else()
    message(WARNING "cr_tpms_library pre-built lib not found at ${DEPS_PREFIX}/lib/cr_tpms_library.lib")
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# ALIAS targets
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
if(TARGET qhullcpp_from_source)
    add_library(deps_qhullcpp ALIAS qhullcpp_from_source)
endif()
if(TARGET assimp_from_source)
    add_library(deps_assimp ALIAS assimp_from_source)
endif()
if(TARGET cr_tpms_from_source)
    add_library(deps_cr_tpms ALIAS cr_tpms_from_source)
endif()
add_library(deps_libigl ALIAS libigl_from_source)

message(STATUS "=== BuildDepsFromSource: ALL deps configured ===")
