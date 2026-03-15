# ==============================================================================
# BuildLibslic3rFromSource.cmake - Compile libslic3r from upstream source
#
# This cmake file compiles libslic3r from source instead of importing the
# pre-built library. It mirrors the upstream CMakeLists.txt from:
#   third_party/CrealityPrint/src/libslic3r/CMakeLists.txt
#
# Strategy:
#   - Glob all .cpp and .hpp files from the libslic3r directory
#   - Set up the same include paths, definitions, and dependencies
#   - Create STATIC library targets: libslic3r_from_source, libslic3r_cgal_from_source
#   - Generate libslic3r_version.h from the template
#   - Handle PCH via pchheader.hpp
#   - Use the same OCCT delay-load configuration as the pre-built version
#   - Export an INTERFACE target `libslic3r` for consumers to link against
#
# Note: This is a drop-in replacement for BuildLibslic3r.cmake
# ==============================================================================

message(STATUS "=== BuildLibslic3rFromSource: Compiling libslic3r from source ===")

# ─── Path definitions ────────────────────────────────────────────────────────
set(UPSTREAM_ROOT    "${CMAKE_SOURCE_DIR}/third_party/CrealityPrint")
set(LIBSLIC3R_SRC_DIR "${UPSTREAM_ROOT}/src/libslic3r")
set(UPSTREAM_SRC     "${UPSTREAM_ROOT}/src")
set(LIBSLIC3R_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/libslic3r_generated")
set(LIBSLIC3R_RELATIVE_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/libslic3r")

# Create generated header directory
file(MAKE_DIRECTORY "${LIBSLIC3R_GEN_DIR}")
file(MAKE_DIRECTORY "${LIBSLIC3R_RELATIVE_GEN_DIR}")

# ─── Timestamp for build info ────────────────────────────────────────────────
string(TIMESTAMP COMPILE_TIME "%y-%m-%d %H:%M")
set(SLIC3R_BUILD_TIME ${COMPILE_TIME})

# ─── 1. Generate libslic3r_version.h ─────────────────────────────────────────
# Provide default values for the version template variables
if(NOT DEFINED SLIC3R_APP_NAME)
    set(SLIC3R_APP_NAME "CrealityPrint")
endif()
if(NOT DEFINED SLIC3R_APP_KEY)
    set(SLIC3R_APP_KEY "CrealityPrint")
endif()
if(NOT DEFINED SLIC3R_APP_USE_FORDER)
    set(SLIC3R_APP_USE_FORDER "Creality")
endif()
if(NOT DEFINED PROCESS_NAME)
    set(PROCESS_NAME "CrealityPrint")
endif()
if(NOT DEFINED SLIC3R_VERSION)
    set(SLIC3R_VERSION "1.0.0")
endif()
if(NOT DEFINED CREALITYPRINT_VERSION)
    set(CREALITYPRINT_VERSION "1.0.0")
endif()
if(NOT DEFINED CREALITYPRINT_VERSION_MAJOR)
    set(CREALITYPRINT_VERSION_MAJOR "1")
endif()
if(NOT DEFINED CREALITYPRINT_VERSION_MINOR)
    set(CREALITYPRINT_VERSION_MINOR "0")
endif()
if(NOT DEFINED CREALITYPRINT_VERSION_PATCH)
    set(CREALITYPRINT_VERSION_PATCH "0")
endif()
if(NOT DEFINED SLIC3R_BUILD_ID)
    set(SLIC3R_BUILD_ID "${SLIC3R_VERSION}-${COMPILE_TIME}")
endif()
if(NOT DEFINED BBL_RELEASE_TO_PUBLIC)
    set(BBL_RELEASE_TO_PUBLIC 0)
endif()
if(NOT DEFINED BBL_INTERNAL_TESTING)
    set(BBL_INTERNAL_TESTING 0)
endif()
if(NOT DEFINED PROJECT_VERSION_EXTRA)
    set(PROJECT_VERSION_EXTRA "")
endif()
if(NOT DEFINED UPDATE_ONLINE_MACHINES)
    set(UPDATE_ONLINE_MACHINES 0)
endif()
if(NOT DEFINED DUMPTOOL_USER)
    set(DUMPTOOL_USER "")
endif()
if(NOT DEFINED DUMPTOOL_PASS)
    set(DUMPTOOL_PASS "")
endif()
if(NOT DEFINED DUMPTOOL_HOST)
    set(DUMPTOOL_HOST "")
endif()
if(NOT DEFINED DUMPTOOL_TO)
    set(DUMPTOOL_TO "")
endif()
set(AUTOMATION_TOOL 0)
set(AUTO_CONVERT_3MF 0)

# Configure the version header
configure_file(
    "${LIBSLIC3R_SRC_DIR}/libslic3r_version.h.in"
    "${LIBSLIC3R_GEN_DIR}/libslic3r_version.h"
    @ONLY
)

# Some upstream translation units include libslic3r_version.h via a relative
# path such as ../libslic3r/libslic3r_version.h, so mirror the generated header
# into the expected build-relative directory layout as well.
configure_file(
    "${LIBSLIC3R_SRC_DIR}/libslic3r_version.h.in"
    "${LIBSLIC3R_RELATIVE_GEN_DIR}/libslic3r_version.h"
    @ONLY
)

# ─── 1b. Generate buildinfo.h ─────────────────────────────────────────────────
# The upstream uses cmake/BuildInfoUtil.cmake with cmake/buildinfo.h.in.
# We replicate the essential generation with sensible defaults.
if(NOT DEFINED BUILD_OS)
    if(WIN32)
        set(BUILD_OS "win64")
    elseif(APPLE)
        set(BUILD_OS "macos")
    else()
        set(BUILD_OS "linux")
    endif()
endif()
if(NOT DEFINED APP_TYPE)
    set(APP_TYPE 0)
endif()
if(NOT DEFINED BIN_OUTPUT_DIR)
    set(BIN_OUTPUT_DIR "")
endif()
if(NOT DEFINED DEBUG_RESOURCES_DIR)
    set(DEBUG_RESOURCES_DIR "${BIN_OUTPUT_DIR}/Debug/resources/")
endif()
if(NOT DEFINED RELEASE_RESOURCES_DIR)
    set(RELEASE_RESOURCES_DIR "${BIN_OUTPUT_DIR}/Release/resources/")
endif()
if(NOT DEFINED MAIN_GIT_HASH)
    # Try to get from git
    execute_process(
        COMMAND git rev-parse HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/third_party/CrealityPrint"
        OUTPUT_VARIABLE MAIN_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _git_result
    )
    if(_git_result OR NOT MAIN_GIT_HASH)
        set(MAIN_GIT_HASH "unknown")
    endif()
endif()
if(NOT DEFINED SHIPPING_LEVEL)
    set(SHIPPING_LEVEL 0)
endif()
if(NOT DEFINED PROJECT_DLL_NAME_WIN32)
    set(PROJECT_DLL_NAME_WIN32 "CrealityPrint_Slicer")
endif()
if(NOT DEFINED CPACK_ORGANIZATION)
    set(CPACK_ORGANIZATION "")
endif()
if(NOT DEFINED BUNDLE_NAME)
    set(BUNDLE_NAME "")
endif()
if(NOT DEFINED CMAKE_MODULE_SOURCE_DIR)
    set(CMAKE_MODULE_SOURCE_DIR "")
endif()
set(BUILD_INFO_HEAD "BUILDINFO_H_")
set(__SUB_BUILD_INFO_DEFINE "")
# No module_info/build_info cmake files in our simplified build

configure_file(
    "${UPSTREAM_ROOT}/cmake/buildinfo.h.in"
    "${LIBSLIC3R_GEN_DIR}/buildinfo.h"
)

# ─── 2. Glob source files ─────────────────────────────────────────────────────
# Core libslic3r sources (from main directory)
file(GLOB LIBSLIC3R_SOURCES
    "${LIBSLIC3R_SRC_DIR}/*.cpp"
    "${LIBSLIC3R_SRC_DIR}/*.hpp"
)

# Algorithm subdirectory
file(GLOB LIBSLIC3R_ALGORITHM_SOURCES
    "${LIBSLIC3R_SRC_DIR}/Algorithm/*.cpp"
    "${LIBSLIC3R_SRC_DIR}/Algorithm/*.hpp"
)

# Arachne subdirectory (wall toolpaths)
file(GLOB LIBSLIC3R_ARACHNE_SOURCES
    "${LIBSLIC3R_SRC_DIR}/Arachne/*.cpp"
    "${LIBSLIC3R_SRC_DIR}/Arachne/*.hpp"
    "${LIBSLIC3R_SRC_DIR}/Arachne/BeadingStrategy/*.cpp"
    "${LIBSLIC3R_SRC_DIR}/Arachne/BeadingStrategy/*.hpp"
    "${LIBSLIC3R_SRC_DIR}/Arachne/utils/*.cpp"
    "${LIBSLIC3R_SRC_DIR}/Arachne/utils/*.hpp"
)

# CSGMesh subdirectory (header-only)
file(GLOB LIBSLIC3R_CSGMESH_SOURCES
    "${LIBSLIC3R_SRC_DIR}/CSGMesh/*.hpp"
)

# Execution subdirectory (header-only)
file(GLOB LIBSLIC3R_EXECUTION_SOURCES
    "${LIBSLIC3R_SRC_DIR}/Execution/*.hpp"
)

# Fill subdirectory (infill patterns)
file(GLOB LIBSLIC3R_FILL_SOURCES
    "${LIBSLIC3R_SRC_DIR}/Fill/*.cpp"
    "${LIBSLIC3R_SRC_DIR}/Fill/*.hpp"
    "${LIBSLIC3R_SRC_DIR}/Fill/Cross/*.cpp"
    "${LIBSLIC3R_SRC_DIR}/Fill/Cross/*.h"
    "${LIBSLIC3R_SRC_DIR}/Fill/Cross/*.hpp"
    "${LIBSLIC3R_SRC_DIR}/Fill/Lightning/*.cpp"
    "${LIBSLIC3R_SRC_DIR}/Fill/Lightning/*.hpp"
    "${LIBSLIC3R_SRC_DIR}/Fill/Quarter/*.cpp"
    "${LIBSLIC3R_SRC_DIR}/Fill/Quarter/*.h"
)

# Format subdirectory (file formats: 3mf, STL, OBJ, STEP, etc.)
file(GLOB LIBSLIC3R_FORMAT_SOURCES
    "${LIBSLIC3R_SRC_DIR}/Format/*.cpp"
    "${LIBSLIC3R_SRC_DIR}/Format/*.hpp"
)

# GCode subdirectory (G-code generation and processing)
file(GLOB LIBSLIC3R_GCODE_SOURCES
    "${LIBSLIC3R_SRC_DIR}/GCode/*.cpp"
    "${LIBSLIC3R_SRC_DIR}/GCode/*.hpp"
)

# Geometry subdirectory (geometry utilities)
file(GLOB LIBSLIC3R_GEOMETRY_SOURCES
    "${LIBSLIC3R_SRC_DIR}/Geometry/*.cpp"
    "${LIBSLIC3R_SRC_DIR}/Geometry/*.hpp"
)

# Interlocking subdirectory
file(GLOB LIBSLIC3R_INTERLOCKING_SOURCES
    "${LIBSLIC3R_SRC_DIR}/Interlocking/*.cpp"
    "${LIBSLIC3R_SRC_DIR}/Interlocking/*.hpp"
)

# Optimize subdirectory (header-only optimizers)
file(GLOB LIBSLIC3R_OPTIMIZE_SOURCES
    "${LIBSLIC3R_SRC_DIR}/Optimize/*.hpp"
)

# Shape subdirectory
file(GLOB LIBSLIC3R_SHAPE_SOURCES
    "${LIBSLIC3R_SRC_DIR}/Shape/*.cpp"
    "${LIBSLIC3R_SRC_DIR}/Shape/*.hpp"
)

# SLA subdirectory (SLA printing support)
file(GLOB LIBSLIC3R_SLA_SOURCES
    "${LIBSLIC3R_SRC_DIR}/SLA/*.cpp"
    "${LIBSLIC3R_SRC_DIR}/SLA/*.hpp"
)

# support_new subdirectory (tree supports)
file(GLOB LIBSLIC3R_SUPPORT_SOURCES
    "${LIBSLIC3R_SRC_DIR}/support_new/*.cpp"
    "${LIBSLIC3R_SRC_DIR}/support_new/*.hpp"
)

# FDM subdirectory (Creality-specific FDM features)
file(GLOB LIBSLIC3R_FDM_SOURCES
    "${LIBSLIC3R_SRC_DIR}/FDM/*.cpp"
    "${LIBSLIC3R_SRC_DIR}/FDM/*.hpp"
)

# common_header subdirectory
file(GLOB LIBSLIC3R_COMMON_HEADER_SOURCES
    "${LIBSLIC3R_SRC_DIR}/common_header/*.cpp"
    "${LIBSLIC3R_SRC_DIR}/common_header/*.h"
)

# Combine all sources
set(ALL_LIBSLIC3R_SOURCES
    ${LIBSLIC3R_SOURCES}
    ${LIBSLIC3R_ALGORITHM_SOURCES}
    ${LIBSLIC3R_ARACHNE_SOURCES}
    ${LIBSLIC3R_CSGMESH_SOURCES}
    ${LIBSLIC3R_EXECUTION_SOURCES}
    ${LIBSLIC3R_FILL_SOURCES}
    ${LIBSLIC3R_FORMAT_SOURCES}
    ${LIBSLIC3R_GCODE_SOURCES}
    ${LIBSLIC3R_GEOMETRY_SOURCES}
    ${LIBSLIC3R_INTERLOCKING_SOURCES}
    ${LIBSLIC3R_OPTIMIZE_SOURCES}
    ${LIBSLIC3R_SHAPE_SOURCES}
    ${LIBSLIC3R_SLA_SOURCES}
    ${LIBSLIC3R_SUPPORT_SOURCES}
    ${LIBSLIC3R_FDM_SOURCES}
    ${LIBSLIC3R_COMMON_HEADER_SOURCES}
)

# Exclude files that are commented out in the upstream CMakeLists.txt
list(FILTER ALL_LIBSLIC3R_SOURCES EXCLUDE REGEX "GCodeSender\\.")
list(FILTER ALL_LIBSLIC3R_SOURCES EXCLUDE REGEX "SupportSpotsGenerator\\.")
# Only exclude the legacy top-level support sources that remain commented out
# upstream. Keep support_new/* in the build because PrintObject.cpp depends on
# their concrete implementations.
list(FILTER ALL_LIBSLIC3R_SOURCES EXCLUDE REGEX ".*/libslic3r/[Tt]ree[Ss]upport\\.(cpp|hpp)$")
list(FILTER ALL_LIBSLIC3R_SOURCES EXCLUDE REGEX ".*/libslic3r/SupportMaterial\\.(cpp|hpp)$")
list(FILTER ALL_LIBSLIC3R_SOURCES EXCLUDE REGEX "SLA/SupportTreeIGL\\.")
list(FILTER ALL_LIBSLIC3R_SOURCES EXCLUDE REGEX "Arachne/utils/ExtrusionJunction\\.cpp$")
list(APPEND ALL_LIBSLIC3R_SOURCES
    "${LIBSLIC3R_GEN_DIR}/libslic3r_version.h"
    "${LIBSLIC3R_GEN_DIR}/buildinfo.h"
)

# ─── Platform-specific sources ───────────────────────────────────────────────
if(WIN32)
    # Windows-specific sources (automation, baseline testing)
    list(APPEND ALL_LIBSLIC3R_SOURCES
        "${LIBSLIC3R_SRC_DIR}/baseline.hpp"
        "${LIBSLIC3R_SRC_DIR}/baseline.cpp"
        "${LIBSLIC3R_SRC_DIR}/baselineorcinput.hpp"
        "${LIBSLIC3R_SRC_DIR}/baselineorcinput.cpp"
        "${LIBSLIC3R_SRC_DIR}/UnittestFlow.cpp"
        "${LIBSLIC3R_SRC_DIR}/UnittestFlow.hpp"
        "${LIBSLIC3R_SRC_DIR}/AutomationMgr.hpp"
        "${LIBSLIC3R_SRC_DIR}/AutomationMgr.cpp"
    )
endif()

if(APPLE)
    # Apple-specific Objective-C++ sources
    list(APPEND ALL_LIBSLIC3R_SOURCES
        "${LIBSLIC3R_SRC_DIR}/MacUtils.mm"
        "${LIBSLIC3R_SRC_DIR}/Format/ModelIO.hpp"
        "${LIBSLIC3R_SRC_DIR}/Format/ModelIO.mm"
    )
endif()

# ─── OpenVDB conditional sources ─────────────────────────────────────────────
# OpenVDBUtils is only included if openvdb_libs target exists
# For now, we include it unconditionally but the upstream checks for the target
# If OpenVDB is not available, these sources should be excluded
set(OpenVDBUtils_SOURCES "")
if(TARGET openvdb_libs)
    message(STATUS "OpenVDB detected - including OpenVDBUtils sources")
    set(OpenVDBUtils_SOURCES
        "${LIBSLIC3R_SRC_DIR}/OpenVDBUtils.cpp"
        "${LIBSLIC3R_SRC_DIR}/OpenVDBUtils.hpp"
    )
else()
    # Remove OpenVDBUtils from the glob results if present
    list(FILTER ALL_LIBSLIC3R_SOURCES EXCLUDE REGEX "OpenVDBUtils\\.(cpp|hpp)$")
endif()

# ─── 3. Find external packages (from pre-built deps) ─────────────────────────
find_package(Boost 1.84 REQUIRED COMPONENTS
    system filesystem thread log log_setup locale regex
    chrono atomic date_time iostreams program_options)
find_package(TBB REQUIRED CONFIG)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(cereal CONFIG REQUIRED)
if(TARGET cereal AND NOT TARGET cereal::cereal)
    add_library(cereal::cereal ALIAS cereal)
endif()
find_package(NLopt CONFIG REQUIRED)
find_package(EXPAT REQUIRED)

# GMP/MPFR for CGAL
set(GMP_INCLUDE_DIR "${DEPS_PREFIX}/include" CACHE PATH "")
set(GMP_LIBRARIES "${DEPS_PREFIX}/lib/libgmp-10.lib" CACHE FILEPATH "")
set(MPFR_INCLUDE_DIR "${DEPS_PREFIX}/include" CACHE PATH "")
set(MPFR_LIBRARIES "${DEPS_PREFIX}/lib/libmpfr-4.lib" CACHE FILEPATH "")
set(CGAL_DO_NOT_WARN_ABOUT_CMAKE_BUILD_TYPE ON CACHE BOOL "" FORCE)
find_package(CGAL REQUIRED CONFIG)

find_package(OpenCV QUIET COMPONENTS core)

# ─── 4. Build embedded deps from source ─────────────────────────────────────
include("${CMAKE_SOURCE_DIR}/cmake/BuildDepsFromSource.cmake")

# Create aliases with _imported suffix for compatibility
add_library(admesh_imported ALIAS admesh_from_source)
add_library(clipper_imported ALIAS clipper_from_source)
add_library(clipper2_imported ALIAS clipper2_from_source)
add_library(glu_libtess_imported ALIAS glu_libtess_from_source)
add_library(libnest2d_imported ALIAS libnest2d_from_source)
add_library(mcut_imported ALIAS mcut_from_source)
add_library(miniz_imported ALIAS miniz_from_source)
add_library(nowide_imported ALIAS nowide_from_source)
add_library(qoi_imported ALIAS qoi_from_source)
add_library(semver_imported ALIAS semver_from_source)
add_library(qhull_imported ALIAS qhull_from_source)
add_library(qhullcpp_imported ALIAS qhullcpp_from_source)
add_library(assimp_imported ALIAS assimp_from_source)
add_library(cr_tpms_imported ALIAS cr_tpms_from_source)
add_library(libigl_imported ALIAS libigl_from_source)

# tbb_libs - TBB wrapper target (create if not exists)
if(NOT TARGET tbb_libs)
    add_library(tbb_libs INTERFACE)
    target_link_libraries(tbb_libs INTERFACE TBB::tbb TBB::tbbmalloc)
endif()

# ─── 5. Create libslic3r_cgal static library ─────────────────────────────────
# CGAL-dependent sources are in a separate library to isolate CGAL compile options
set(LIBSLIC3R_CGAL_SOURCES
    "${LIBSLIC3R_SRC_DIR}/CutSurface.hpp"
    "${LIBSLIC3R_SRC_DIR}/CutSurface.cpp"
    "${LIBSLIC3R_SRC_DIR}/IntersectionPoints.hpp"
    "${LIBSLIC3R_SRC_DIR}/IntersectionPoints.cpp"
    "${LIBSLIC3R_SRC_DIR}/TryCatchSignal.hpp"
    "${LIBSLIC3R_SRC_DIR}/TryCatchSignal.cpp"
    "${LIBSLIC3R_SRC_DIR}/Triangulation.hpp"
    "${LIBSLIC3R_SRC_DIR}/Triangulation.cpp"
)

# MeshBoolean is excluded when BUILD_CLOUD_SLICER is defined
if(NOT DEFINED BUILD_CLOUD_SLICER OR NOT BUILD_CLOUD_SLICER)
    list(APPEND LIBSLIC3R_CGAL_SOURCES
        "${LIBSLIC3R_SRC_DIR}/MeshBoolean.hpp"
        "${LIBSLIC3R_SRC_DIR}/MeshBoolean.cpp"
    )
endif()

add_library(libslic3r_cgal_from_source STATIC ${LIBSLIC3R_CGAL_SOURCES})

# Compile definitions needed by CGAL-dependent sources
target_compile_definitions(libslic3r_cgal_from_source PRIVATE
    _USE_MATH_DEFINES
    NOMINMAX
)

target_include_directories(libslic3r_cgal_from_source PRIVATE
    "${LIBSLIC3R_SRC_DIR}"
    "${LIBSLIC3R_GEN_DIR}"
    "${UPSTREAM_SRC}"
)

# Get CGAL target for linking
get_target_property(_cgal_tgt CGAL::CGAL ALIASED_TARGET)
if(NOT TARGET ${_cgal_tgt})
    set(_cgal_tgt CGAL::CGAL)
endif()

# Reset CGAL compile options propagation (same as upstream)
get_target_property(_cgal_opts ${_cgal_tgt} INTERFACE_COMPILE_OPTIONS)
if(_cgal_opts)
    set(_cgal_opts_bad "${_cgal_opts}")
    set(_cgal_opts_good "${_cgal_opts}")
    list(FILTER _cgal_opts_bad INCLUDE REGEX "frounding-math")
    list(FILTER _cgal_opts_good EXCLUDE REGEX "frounding-math")
    set_target_properties(${_cgal_tgt} PROPERTIES INTERFACE_COMPILE_OPTIONS "${_cgal_opts_good}")
    target_compile_options(libslic3r_cgal_from_source PRIVATE "${_cgal_opts_bad}")
endif()

# Remove CGAL's BOOST_ALL_DYN_LINK definition — our Boost is static, not dynamic.
# CGALConfig.cmake from the pre-built deps adds this, but it conflicts with the
# static Boost linking used by the rest of the project.
get_target_property(_cgal_defs ${_cgal_tgt} INTERFACE_COMPILE_DEFINITIONS)
if(_cgal_defs)
    list(FILTER _cgal_defs_good EXCLUDE REGEX "BOOST_ALL_DYN_LINK")
    set_target_properties(${_cgal_tgt} PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${_cgal_defs_good}")
endif()

# Get CGAL include directories for later use
get_target_property(_cgal_inc CGAL::CGAL INTERFACE_INCLUDE_DIRECTORIES)
if(NOT _cgal_inc)
    set(_cgal_inc "${DEPS_PREFIX}/include")
endif()

# System include directories needed by CGAL-dependent sources
target_include_directories(libslic3r_cgal_from_source SYSTEM PRIVATE
    "${UPSTREAM_SRC}/eigen"
    ${_cgal_inc}
)

target_link_libraries(libslic3r_cgal_from_source PRIVATE
    ${_cgal_tgt}
    libigl_imported
    mcut_imported
    tbb_libs
)

if(TARGET cereal::cereal)
    target_link_libraries(libslic3r_cgal_from_source PRIVATE cereal::cereal)
endif()

if(MSVC AND "${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")  # 32-bit MSVC workaround
    target_compile_definitions(libslic3r_cgal_from_source PRIVATE CGAL_DO_NOT_USE_MPZF)
endif()

# ─── 6. Create the main libslic3r static library ──────────────────────────────
add_library(libslic3r_from_source STATIC
    ${ALL_LIBSLIC3R_SOURCES}
    ${OpenVDBUtils_SOURCES}
)

# MinGW big-obj workaround
if(MINGW)
    target_compile_options(libslic3r_from_source PRIVATE -Wa,-mbig-obj)
endif()

# Compile definitions
target_compile_definitions(libslic3r_from_source PUBLIC
    USE_TBB
    TBB_USE_CAPTURED_EXCEPTION=0
    _USE_MATH_DEFINES
    _WIN32
    _WIN32_WINNT=0x0601
    BOOST_ALL_NO_LIB
    BOOST_USE_WINAPI_VERSION=0x602
    BOOST_SYSTEM_USE_UTF8
    _CRT_SECURE_NO_WARNINGS
    _SCL_SECURE_NO_WARNINGS
    NOMINMAX
    UNICODE
    _UNICODE
)

# Include directories
target_include_directories(libslic3r_from_source
    PRIVATE
        "${LIBSLIC3R_SRC_DIR}"
        "${LIBSLIC3R_GEN_DIR}"
    PUBLIC
        "${UPSTREAM_SRC}"          # For #include <libslic3r/xxx>
        "${LIBSLIC3R_GEN_DIR}"     # Generated headers
)

# System include directories (external dependencies)
target_include_directories(libslic3r_from_source SYSTEM PUBLIC
    ${Boost_INCLUDE_DIRS}
    ${_cgal_inc}
    ${EXPAT_INCLUDE_DIRS}
    "${UPSTREAM_SRC}/eigen"
    "${UPSTREAM_SRC}/fast_float"
    "${UPSTREAM_SRC}/nlohmann"
    "${UPSTREAM_SRC}/ankerl"
    "${UPSTREAM_SRC}/nanosvg"
    "${UPSTREAM_SRC}/spline"
    "${UPSTREAM_SRC}/stb_dxt"
    "${UPSTREAM_SRC}/libigl"
    "${UPSTREAM_SRC}/clipper2/Clipper2Lib/include"
    "${UPSTREAM_SRC}/miniz"
    "${UPSTREAM_SRC}/glu-libtess/include"
    "${UPSTREAM_SRC}/libnest2d/include"
    "${DEPS_PREFIX}/include/occt"
)

# ─── 7. OCCT (OpenCASCADE) libraries with delay-load ──────────────────────────
# NOTE: OCCT delay-load causes 0xC0000005 at startup when TK*.dll are not present.
# The QML GUI doesn't use OCCT, so we disable delay-load. OCCT symbols are still
# linked (needed by CGAL compilation) but won't cause runtime load failures.
set(OCCT_LIBS
    TKXDESTEP    TKSTEP       TKSTEP209    TKSTEPAttr   TKSTEPBase
    TKXCAF       TKXSBase     TKVCAF       TKCAF        TKLCAF
    TKCDF        TKV3d        TKService    TKMesh       TKBO
    TKPrim       TKHLR        TKShHealing  TKTopAlgo    TKGeomAlgo
    TKBRep       TKGeomBase   TKG3d        TKG2d        TKMath
    TKernel
)
set(OCCT_LIB_DIR "${DEPS_PREFIX}/lib/occt")

# Disable delay-load to prevent 0xC0000005 when OCCT DLLs are absent.
# OCCT is only needed for STEP/OBJ import which the QML GUI doesn't use.
set(_delayload_libs "")

# Also delay-load cr_tpms_library (closed-source TPMS infill DLL)
list(APPEND _delayload_libs "/DELAYLOAD:cr_tpms_library.dll")

# ─── 8. Link all dependencies to libslic3r_from_source ───────────────────────
target_link_libraries(libslic3r_from_source PUBLIC
    # Embedded deps from upstream build
    admesh_imported
    clipper_imported
    clipper2_imported
    glu_libtess_imported
    libnest2d_imported
    mcut_imported
    miniz_imported
    nowide_imported
    qoi_imported
    semver_imported
    qhull_imported
    qhullcpp_imported
    assimp_imported
    cr_tpms_imported
    libigl_imported
    # CGAL-dependent sources
    libslic3r_cgal_from_source
    # External deps - Boost
    Boost::system
    Boost::filesystem
    Boost::thread
    Boost::log
    Boost::log_setup
    Boost::locale
    Boost::regex
    Boost::chrono
    Boost::atomic
    Boost::date_time
    Boost::iostreams
    Boost::program_options
    # TBB
    TBB::tbb
    TBB::tbbmalloc
    # Other external deps
    cereal::cereal
    ${EXPAT_LIBRARIES}
    PNG::PNG
    ZLIB::ZLIB
    JPEG::JPEG
    OpenSSL::SSL
    OpenSSL::Crypto
    ${GMP_LIBRARIES}
    ${MPFR_LIBRARIES}
    NLopt::nlopt
    ${CMAKE_DL_LIBS}
    # CGAL (for main library too)
    ${_cgal_tgt}
)

# ─── 7.5 OCCT (OpenCASCADE) libraries ────────────────────────────────────────
# Link OCCT libraries with full paths for STEP/SVG import
foreach(_occt_lib ${OCCT_LIBS})
    target_link_libraries(libslic3r_from_source PUBLIC "${OCCT_LIB_DIR}/${_occt_lib}.lib")
endforeach()

# Platform-specific libraries
if(WIN32)
    target_link_libraries(libslic3r_from_source PUBLIC Psapi.lib bcrypt.lib)
endif()

if(OpenCV_FOUND)
    target_link_libraries(libslic3r_from_source PUBLIC ${OpenCV_LIBS})
endif()

if(TARGET openvdb_libs)
    target_link_libraries(libslic3r_from_source PUBLIC openvdb_libs)
endif()

if(TARGET openssl::openssl)
    target_link_libraries(libslic3r_cgal_from_source PRIVATE openssl::openssl)
    target_link_libraries(libslic3r_from_source PUBLIC openssl::openssl)
endif()

if(APPLE)
    find_library(FOUNDATION Foundation REQUIRED)
    find_library(MODELIO ModelIO REQUIRED)
    target_link_libraries(libslic3r_from_source PUBLIC ${FOUNDATION} ${MODELIO})
endif()

# ─── 9. Delay-load linker options (MSVC) ─────────────────────────────────────
# /DELAYLOAD - OCCT DLLs are not loaded during PE initialization, avoiding
# CRT deadlock between /MT libslic3r and /MD OCCT DLLs in DllMain.
if(MSVC)
    target_link_options(libslic3r_from_source PUBLIC
        /NODEFAULTLIB:LIBCMT
        /NODEFAULTLIB:libcpmt
        ${_delayload_libs}
    )
    target_link_libraries(libslic3r_from_source PUBLIC delayimp.lib)
endif()

# ─── 10. Precompiled header support ──────────────────────────────────────────
# Use the upstream pchheader.hpp for PCH if enabled
option(LIBSLIC3R_PCH "Enable precompiled headers for libslic3r" ON)
if(LIBSLIC3R_PCH AND MSVC)
    # PCH header file exists in the source directory
    if(EXISTS "${LIBSLIC3R_SRC_DIR}/pchheader.hpp")
        target_precompile_headers(libslic3r_from_source PRIVATE
            "${LIBSLIC3R_SRC_DIR}/pchheader.hpp"
        )
        message(STATUS "libslic3r: Precompiled header enabled (pchheader.hpp)")
    endif()
endif()

# ─── 11. Create the INTERFACE libslic3r target ───────────────────────────────
# This is what the rest of the project links against.
# It provides the same interface as the pre-built import version.
add_library(libslic3r INTERFACE)

# Forward compile definitions
target_compile_definitions(libslic3r INTERFACE
    USE_TBB
    TBB_USE_CAPTURED_EXCEPTION=0
    NOMINMAX
    _USE_MATH_DEFINES
    BOOST_ALL_NO_LIB
    BOOST_USE_WINAPI_VERSION=0x602
    BOOST_SYSTEM_USE_UTF8
    HAS_CGAL
)

# Forward include directories
target_include_directories(libslic3r INTERFACE
    "${UPSTREAM_SRC}"
    "${LIBSLIC3R_GEN_DIR}"
)
target_include_directories(libslic3r SYSTEM INTERFACE
    ${Boost_INCLUDE_DIRS}
    ${_cgal_inc}
    "${UPSTREAM_SRC}/eigen"
    "${UPSTREAM_SRC}/fast_float"
    "${UPSTREAM_SRC}/nlohmann"
    "${UPSTREAM_SRC}/ankerl"
    "${UPSTREAM_SRC}/nanosvg"
    "${UPSTREAM_SRC}/spline"
    "${UPSTREAM_SRC}/stb_dxt"
    "${UPSTREAM_SRC}/libigl"
    "${UPSTREAM_SRC}/clipper2/Clipper2Lib/include"
    "${UPSTREAM_SRC}/miniz"
    "${UPSTREAM_SRC}/glu-libtess/include"
    "${UPSTREAM_SRC}/libnest2d/include"
    ${EXPAT_INCLUDE_DIRS}
    "${DEPS_PREFIX}/include/occt"
)

# Link to the compiled library
target_link_libraries(libslic3r INTERFACE libslic3r_from_source)

# Forward the delay-load options
if(MSVC)
    target_link_options(libslic3r INTERFACE
        /NODEFAULTLIB:LIBCMT
        /NODEFAULTLIB:libcpmt
        ${_delayload_libs}
    )
    target_link_libraries(libslic3r INTERFACE delayimp.lib)
endif()

# Source group for IDE organization (only files under LIBSLIC3R_SRC_DIR)
set(_sg_files "")
foreach(_f ${ALL_LIBSLIC3R_SOURCES})
    string(FIND "${_f}" "${LIBSLIC3R_SRC_DIR}" _pos)
    if(_pos GREATER_EQUAL 0)
        list(APPEND _sg_files "${_f}")
    endif()
endforeach()
if(_sg_files)
    source_group(TREE "${LIBSLIC3R_SRC_DIR}" FILES ${_sg_files})
endif()

message(STATUS "=== libslic3r: Compiled from source ===")
message(STATUS "    Source dir: ${LIBSLIC3R_SRC_DIR}")
message(STATUS "    Generated headers: ${LIBSLIC3R_GEN_DIR}")
