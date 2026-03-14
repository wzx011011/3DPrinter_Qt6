cmake_minimum_required(VERSION 3.16)

# ── CRT 运行库：使用 /MD（动态CRT）匹配 Qt、TBB、Boost deps ────────────
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# ── 预编译依赖库路径（来自 E:\ai\3D-Printer\deps\build） ─────────────────
set(DEPS_PREFIX "E:/ai/3D-Printer/deps/build/OrcaSlicer_dep/usr/local"
    CACHE PATH "Pre-built dependencies prefix from upstream OrcaSlicer build")

# 将预编译依赖加入 CMAKE_PREFIX_PATH
list(APPEND CMAKE_PREFIX_PATH
    "${DEPS_PREFIX}"
    "${DEPS_PREFIX}/lib/cmake"
    "${DEPS_PREFIX}/share/cmake"
)

project(FramelessDialogDemo LANGUAGES C CXX)

option(CREALITY_QML_GUI "Build QML-first GUI architecture" ON)
option(ENABLE_VISUAL_REGRESSION "Enable visual regression tests" ON)
option(ENABLE_COVERAGE "Enable compiler coverage instrumentation" OFF)
option(BUILD_LIBSLIC3R "Build libslic3r backend (3MF loading etc.)" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(CTest)

# ── 构建 libslic3r 后端 ──────────────────────────────────────────────────
if(BUILD_LIBSLIC3R)
    option(LIBSLIC3R_FROM_SOURCE "Build libslic3r from source (slower but fixes static init crash)" OFF)
    if(LIBSLIC3R_FROM_SOURCE)
        message(STATUS "=== Building libslic3r FROM SOURCE ===")
        include("${CMAKE_SOURCE_DIR}/cmake/BuildLibslic3rFromSource.cmake")
    else()
        message(STATUS "=== Importing pre-built libslic3r ===")
        include("${CMAKE_SOURCE_DIR}/cmake/BuildLibslic3r.cmake")
    endif()
endif()

if(CREALITY_QML_GUI)
    if(NOT BUILD_LIBSLIC3R)
        message(WARNING "CREALITY_QML_GUI: BUILD_LIBSLIC3R=OFF — OCCT/model loading will be stubbed out for testing.")
    endif()
    if(BUILD_LIBSLIC3R)
        # BUILD_LIBSLIC3R=ON 时 Boost_ALL_NO_LIB 干扰 Qt 的 auto-link，
        # 需要跳过低版本检查以避免 Qt 查找失败
        find_package(Qt6 REQUIRED COMPONENTS Qml Quick QuickControls2 LinguistTools OpenGL Concurrent)
    else()
        # 低版本 Qt 兼容：允许 Qt 3.21 与 Qt 6.10 配对
        find_package(Qt6 REQUIRED COMPONENTS Qml Quick QuickControls2 LinguistTools OpenGL Concurrent)
        set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/third_party/Qt6_10")
        set(QT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT 3.21)
    endif()

    qt_add_executable(FramelessDialogDemo
        src/qml_gui/main_qml.cpp
        src/nanosvg_impl.cpp
        src/qml_gui/BackendContext.cpp
        src/qml_gui/BackendContext.h
        src/core/services/SliceService.cpp
        src/core/services/SliceService.h
        src/core/services/PresetServiceMock.cpp
        src/core/services/PresetServiceMock.h
        src/core/services/DeviceServiceMock.cpp
        src/core/services/DeviceServiceMock.h
        src/core/services/CameraServiceMock.cpp
        src/core/services/CameraServiceMock.h
        src/core/services/CloudServiceMock.cpp
        src/core/services/CloudServiceMock.h
        src/core/services/ProjectServiceMock.cpp
        src/core/services/ProjectServiceMock.h
        src/core/services/NetworkServiceMock.cpp
        src/core/services/NetworkServiceMock.h
        src/core/viewmodels/EditorViewModel.cpp
        src/core/viewmodels/EditorViewModel.h
        src/core/viewmodels/PreviewViewModel.cpp
        src/core/viewmodels/PreviewViewModel.h
        src/core/viewmodels/MonitorViewModel.cpp
        src/core/viewmodels/MonitorViewModel.h
        src/core/viewmodels/ConfigViewModel.cpp
        src/core/viewmodels/ConfigViewModel.h
        src/core/viewmodels/HomeViewModel.cpp
        src/core/viewmodels/HomeViewModel.h
        src/core/viewmodels/SettingsViewModel.cpp
        src/core/viewmodels/SettingsViewModel.h
        src/core/viewmodels/ProjectViewModel.cpp
        src/core/viewmodels/ProjectViewModel.h
        src/core/viewmodels/CalibrationViewModel.cpp
        src/core/viewmodels/CalibrationViewModel.h
        src/core/viewmodels/ModelMallViewModel.cpp
        src/core/viewmodels/ModelMallViewModel.h
        src/core/viewmodels/MultiMachineViewModel.cpp
        src/core/viewmodels/MultiMachineViewModel.h
        src/core/services/CalibrationServiceMock.cpp
        src/core/services/CalibrationServiceMock.h
        src/qml_gui/Models/ConfigOptionModel.cpp
        src/qml_gui/Models/ConfigOptionModel.h
        src/qml_gui/Models/PresetListModel.cpp
        src/qml_gui/Models/PresetListModel.h
        src/qml_gui/Renderer/CameraController.cpp
        src/qml_gui/Renderer/CameraController.h
        src/qml_gui/Renderer/GLViewport.cpp
        src/qml_gui/Renderer/GLViewport.h
        src/qml_gui/Renderer/GLViewportRenderer.cpp
        src/qml_gui/Renderer/GLViewportRenderer.h
        src/qml_gui/Renderer/GCodeRenderer.cpp
        src/qml_gui/Renderer/GCodeRenderer.h
        src/qml_gui/qml.qrc
    )

    if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/src/core/debug/CrashHandlerWin.cpp")
        target_sources(FramelessDialogDemo PRIVATE
            src/core/debug/CrashHandlerWin.cpp
            src/core/debug/CrashHandlerWin.h
            src/core/debug/EarlyCrashDiag.cpp
        )
    endif()

    target_link_libraries(FramelessDialogDemo PRIVATE Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::OpenGL Qt6::Concurrent)
    if(MSVC)
        target_compile_options(FramelessDialogDemo PRIVATE /Zi)
        target_link_options(FramelessDialogDemo PRIVATE /DEBUG:FULL)
    endif()
    foreach(_runtime_dll freetype.dll libmpfr-4.dll libgmp-10.dll qhull.dll qhull_p.dll qhull_r.dll)
        if(EXISTS "${DEPS_PREFIX}/bin/${_runtime_dll}")
            add_custom_command(TARGET FramelessDialogDemo POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DEPS_PREFIX}/bin/${_runtime_dll}"
                    "$<TARGET_FILE_DIR:FramelessDialogDemo>/${_runtime_dll}")
        endif()
    endforeach()
    if(BUILD_LIBSLIC3R AND TARGET libslic3r)
        target_link_libraries(FramelessDialogDemo PRIVATE libslic3r)
        target_compile_definitions(FramelessDialogDemo PRIVATE HAS_LIBSLIC3R=1)
        if(MSVC)
            target_compile_definitions(FramelessDialogDemo PRIVATE
                BOOST_ALL_NO_LIB
                BOOST_USE_WINAPI_VERSION=0x602
                BOOST_SYSTEM_USE_UTF8
            )
        endif()
        # Copy OCCT runtime DLLs (delay-loaded, needed when libslic3r actually calls them)
        file(GLOB _occt_dlls "${DEPS_PREFIX}/bin/occt/TK*.dll")
        foreach(_occt_dll ${_occt_dlls})
            add_custom_command(TARGET FramelessDialogDemo POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_occt_dll}"
                    "$<TARGET_FILE_DIR:FramelessDialogDemo>/")
        endforeach()
    endif()
    target_compile_definitions(FramelessDialogDemo PRIVATE CREALITY_QML_GUI=1)
else()
    find_package(Qt6 REQUIRED COMPONENTS Widgets)

    qt_add_executable(FramelessDialogDemo
        src/main.cpp
        src/MainWindow.cpp
        src/MainWindow.h
        src/theme/AppTheme.cpp
        src/theme/AppTheme.h
        src/pages/PreparePage.cpp
        src/pages/PreparePage.h
        src/pages/PreviewPage.cpp
        src/pages/PreviewPage.h
        src/pages/MonitorPage.cpp
        src/pages/MonitorPage.h
        src/pages/ParameterPage.cpp
        src/pages/ParameterPage.h
    )

    target_link_libraries(FramelessDialogDemo PRIVATE Qt6::Widgets)
endif()

target_include_directories(FramelessDialogDemo PRIVATE
    src
    ${CMAKE_SOURCE_DIR}/third_party/CrealityPrint/src
)

if(WIN32)
    set_target_properties(FramelessDialogDemo PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# I2 — i18n 翼译文件
qt_add_translations(FramelessDialogDemo
    TS_FILES
        i18n/zh_CN.ts
        i18n/en.ts
)

