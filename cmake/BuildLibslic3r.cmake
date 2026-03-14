# ==============================================================================
# BuildLibslic3r.cmake — 导入上游预编译好的 libslic3r 静态库
#
# 方案：直接使用 E:\ai\3D-Printer\out\vs2026-x64-release 构建产物
#   - .lib 文件来自上游 VS2026 构建输出
#   - 头文件来自 third_party/CrealityPrint/src（git submodule）
#   - 生成头文件来自上游构建输出目录
#   - 外部依赖来自上游相同的 deps 目录
#
# 优点：
#   1. 绝不修改/编译 third_party/CrealityPrint/ 下的任何源码
#   2. ABI 完全兼容（同一台机器、同一个 VS2026 编译器）
#   3. 构建速度极快（仅需链接，无需编译 ~480 个源文件）
# ==============================================================================

message(STATUS "=== BuildLibslic3r: importing pre-built libraries ===")

# ─── Path definitions ────────────────────────────────────────────────────────
set(UPSTREAM_ROOT    "${CMAKE_SOURCE_DIR}/third_party/CrealityPrint")
set(UPSTREAM_SRC     "${UPSTREAM_ROOT}/src")
set(UPSTREAM_BUILD   "E:/ai/3D-Printer/out/vs2026-x64-release/build/src")
set(UPSTREAM_GEN_DIR "${UPSTREAM_BUILD}/libslic3r")   # buildinfo.h, libslic3r_version.h

# ─── 1. Find external packages (from pre-built deps) ────────────────────────
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

set(GMP_INCLUDE_DIR "${DEPS_PREFIX}/include" CACHE PATH "")
set(GMP_LIBRARIES "${DEPS_PREFIX}/lib/libgmp-10.lib" CACHE FILEPATH "")
set(MPFR_INCLUDE_DIR "${DEPS_PREFIX}/include" CACHE PATH "")
set(MPFR_LIBRARIES "${DEPS_PREFIX}/lib/libmpfr-4.lib" CACHE FILEPATH "")
set(CGAL_DO_NOT_WARN_ABOUT_CMAKE_BUILD_TYPE ON CACHE BOOL "" FORCE)
find_package(CGAL REQUIRED CONFIG)

find_package(OpenCV QUIET COMPONENTS core)

# OpenCASCADE (OCCT) — STEP/SVG geometry processing (DLL, import libs)
set(OCCT_LIB_DIR "${DEPS_PREFIX}/lib/occt")
set(OCCT_LIBS
    TKXDESTEP    TKSTEP       TKSTEP209    TKSTEPAttr   TKSTEPBase
    TKXCAF       TKXSBase     TKVCAF       TKCAF        TKLCAF
    TKCDF        TKV3d        TKService    TKMesh       TKBO
    TKPrim       TKHLR        TKShHealing  TKTopAlgo    TKGeomAlgo
    TKBRep       TKGeomBase   TKG3d        TKG2d        TKMath
    TKernel
)

# 获取 CGAL 的 include 路径（CGAL 是 header-only，但其 CMake config 会
# 强制传播 BOOST_ALL_DYN_LINK=1，与上游静态编译的 Boost 冲突，
# 所以不链接 CGAL::CGAL target，只取 include 路径）
get_target_property(_cgal_inc CGAL::CGAL INTERFACE_INCLUDE_DIRECTORIES)
if(NOT _cgal_inc)
    set(_cgal_inc "${DEPS_PREFIX}/include")
endif()

# ─── 2. Helper: import a pre-built static library ───────────────────────────
macro(import_prebuilt_lib _name _lib_path)
    add_library(${_name} STATIC IMPORTED GLOBAL)
    set_target_properties(${_name} PROPERTIES
        IMPORTED_LOCATION "${_lib_path}"
    )
    if(NOT EXISTS "${_lib_path}")
        message(FATAL_ERROR "Pre-built library not found: ${_lib_path}")
    endif()
endmacro()

# ─── 3. Import all pre-built .lib files ─────────────────────────────────────
set(_LIB_DIR "${UPSTREAM_BUILD}")

import_prebuilt_lib(libslic3r_imported    "${_LIB_DIR}/libslic3r/Release/libslic3r.lib")
import_prebuilt_lib(libslic3r_cgal_imported "${_LIB_DIR}/libslic3r/Release/libslic3r_cgal.lib")
import_prebuilt_lib(admesh_imported       "${_LIB_DIR}/admesh/Release/admesh.lib")
import_prebuilt_lib(clipper_imported      "${_LIB_DIR}/clipper/Release/clipper.lib")
import_prebuilt_lib(clipper2_imported     "${_LIB_DIR}/clipper2/Release/Clipper2.lib")
import_prebuilt_lib(glu_libtess_imported  "${_LIB_DIR}/glu-libtess/Release/glu-libtess.lib")
import_prebuilt_lib(libnest2d_imported    "${_LIB_DIR}/libnest2d/Release/libnest2d.lib")
import_prebuilt_lib(mcut_imported         "${_LIB_DIR}/mcut/Release/mcut.lib")
import_prebuilt_lib(miniz_imported        "${_LIB_DIR}/miniz/Release/miniz_static.lib")
import_prebuilt_lib(nowide_imported       "${_LIB_DIR}/boost/Release/nowide.lib")
import_prebuilt_lib(qoi_imported          "${_LIB_DIR}/qoi/Release/qoi.lib")
import_prebuilt_lib(semver_imported       "${_LIB_DIR}/semver/Release/semver.lib")

# qhull — from pre-built deps (system-level install, not in upstream build/src)
import_prebuilt_lib(qhull_imported "${DEPS_PREFIX}/lib/qhullstatic_r.lib")
import_prebuilt_lib(qhullcpp_imported "${DEPS_PREFIX}/lib/qhullcpp.lib")

# assimp — 3D model import (STEP, OBJ, etc.)
import_prebuilt_lib(assimp_imported "${DEPS_PREFIX}/lib/assimp-vc142-mt.lib")

# cr_tpms_library — TPMS infill scalar field (closed-source DLL)
import_prebuilt_lib(cr_tpms_imported "${DEPS_PREFIX}/lib/cr_tpms_library.lib")

# ─── 4. Create the INTERFACE libslic3r target ────────────────────────────────
# This is what the rest of project links against.
# It bundles: include paths + all pre-built .lib + all external deps.
add_library(libslic3r INTERFACE)

# Compile definitions consumers need
target_compile_definitions(libslic3r INTERFACE
    USE_TBB
    TBB_USE_CAPTURED_EXCEPTION=0
    NOMINMAX
    _USE_MATH_DEFINES
    BOOST_ALL_NO_LIB
    HAS_CGAL
)

# Include paths: upstream headers + generated headers + header-only deps
target_include_directories(libslic3r INTERFACE
    "${UPSTREAM_SRC}"                # for #include <libslic3r/xxx>
    "${UPSTREAM_GEN_DIR}"            # buildinfo.h, libslic3r_version.h
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
    "${UPSTREAM_SRC}/libigl/include" # libigl headers
    "${UPSTREAM_SRC}/clipper2/Clipper2Lib/include"
    "${UPSTREAM_SRC}/miniz"
    "${UPSTREAM_SRC}/glu-libtess/include"
    "${UPSTREAM_SRC}/libnest2d/include"
    ${EXPAT_INCLUDE_DIRS}
    "${DEPS_PREFIX}/include/occt"    # OpenCASCADE headers
)

# Link all pre-built static libs
target_link_libraries(libslic3r INTERFACE
    # Core libraries
    libslic3r_imported
    libslic3r_cgal_imported
    # Embedded deps
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
    # External deps — Boost component targets (${Boost_LIBRARIES} 在 Config mode 下可能为空)
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
    TBB::tbb
    TBB::tbbmalloc
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
)

# OpenCASCADE (OCCT) — import libs for STEP/SVG/geometry
# Use /DELAYLOAD to avoid DllMain deadlock at startup: the OCCT DLLs are
# loaded during PE init, but libslic3r is /MT (static CRT) and the exe is
# /MD (dynamic CRT), causing a deadlock in the loader lock.  Delay-load
# removes OCCT from the import table entirely; the DLLs are only loaded on
# first function call, which the QML GUI never does.
set(_delayload_libs "")
foreach(_occt_lib ${OCCT_LIBS})
    target_link_libraries(libslic3r INTERFACE "${OCCT_LIB_DIR}/${_occt_lib}.lib")
    list(APPEND _delayload_libs "/DELAYLOAD:${_occt_lib}.dll")
endforeach()
# Also delay-load cr_tpms_library (scalar TPMS infill, closed-source DLL)
list(APPEND _delayload_libs "/DELAYLOAD:cr_tpms_library.dll")

if(WIN32)
    target_link_libraries(libslic3r INTERFACE Psapi.lib bcrypt.lib)
endif()

if(OpenCV_FOUND)
    target_link_libraries(libslic3r INTERFACE ${OpenCV_LIBS})
endif()

# /DELAYLOAD — OCCT DLLs 在 PE 初始化时不加载，避免 /MT 的 libslic3r 与
# /MD 的 OCCT DLL 在 DllMain 期间产生 CRT 死锁。
if(MSVC)
    target_link_options(libslic3r INTERFACE
        /NODEFAULTLIB:LIBCMT
        /NODEFAULTLIB:libcpmt
        ${_delayload_libs}
    )
    target_link_libraries(libslic3r INTERFACE delayimp.lib)
endif()

message(STATUS "=== libslic3r: imported pre-built Release libraries ===")
