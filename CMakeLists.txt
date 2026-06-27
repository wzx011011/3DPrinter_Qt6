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

# Eigen3 from vcpkg (header-only, not in DEPS_PREFIX)
set(VCPKG_INSTALLED_DIR "E:/vcpkg/installed/x64-windows" CACHE PATH "vcpkg installed packages")
if(EXISTS "${VCPKG_INSTALLED_DIR}/share/eigen3/Eigen3Config.cmake")
    list(APPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}/share")
endif()

project(OWzxSlicer LANGUAGES C CXX)

option(OWZX_QML_GUI "Build QML-first GUI architecture" ON)
option(ENABLE_VISUAL_REGRESSION "Enable visual regression tests" ON)
option(ENABLE_COVERAGE "Enable compiler coverage instrumentation" OFF)
option(BUILD_LIBSLIC3R "Build libslic3r backend (3MF loading etc.)" ON)
option(BUILD_CLI "Build headless CLI executable (owzx-cli)" OFF)
option(OWZX_RENDER_BENCH "Build experimental Qt RHI render benchmark" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(CTest)

# ── 构建 libslic3r 后端 ──────────────────────────────────────────────────
if(BUILD_LIBSLIC3R)
    option(LIBSLIC3R_FROM_SOURCE "Build libslic3r from source (fixes static init crash from CRT mismatch with pre-built libs)" ON)
    if(LIBSLIC3R_FROM_SOURCE)
        message(STATUS "=== Building libslic3r FROM SOURCE ===")
        include("${CMAKE_SOURCE_DIR}/cmake/BuildLibslic3rFromSource.cmake")
    else()
        message(STATUS "=== Importing pre-built libslic3r ===")
        include("${CMAKE_SOURCE_DIR}/cmake/BuildLibslic3r.cmake")
    endif()
endif()

if(OWZX_QML_GUI)
    if(NOT BUILD_LIBSLIC3R)
        message(WARNING "OWZX_QML_GUI: BUILD_LIBSLIC3R=OFF — OCCT/model loading will be stubbed out for testing.")
    endif()
    if(BUILD_LIBSLIC3R)
        # BUILD_LIBSLIC3R=ON 时 Boost_ALL_NO_LIB 干扰 Qt 的 auto-link，
        # 需要跳过低版本检查以避免 Qt 查找失败
        find_package(Qt6 REQUIRED COMPONENTS Qml Quick QuickControls2 LinguistTools OpenGL Concurrent Test Gui GuiPrivate ShaderTools)
    else()
        # 低版本 Qt 兼容：允许 Qt 3.21 与 Qt 6.10 配对
        find_package(Qt6 REQUIRED COMPONENTS Qml Quick QuickControls2 LinguistTools OpenGL Concurrent Test Gui GuiPrivate ShaderTools)
        set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/third_party/Qt6_10")
        set(QT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT 3.21)
    endif()

    # ── 共享源码 OBJECT 库（主程序和测试共用） ────────────────────────────
    add_library(owzx_app_core OBJECT
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
        src/core/services/UndoRedoManager.cpp
        src/core/services/UndoRedoManager.h
        src/core/services/UndoCommands.cpp
        src/core/services/UndoCommands.h
        src/core/services/JobBase.h
        src/core/services/JobBase.cpp
        src/core/services/JobManager.h
        src/core/services/JobManager.cpp
        src/core/services/NetworkServiceMock.cpp
        src/core/services/NetworkServiceMock.h
        src/core/services/CalibrationServiceMock.cpp
        src/core/services/CalibrationServiceMock.h
        src/core/services/AuxiliaryService.cpp
        src/core/services/AuxiliaryService.h
        src/core/services/AppSettingsService.cpp
        src/core/services/AppSettingsService.h
        src/core/rendering/GLShaderUtil.cpp
        src/core/rendering/GLShaderUtil.h
        src/core/model/PartPlate.cpp
        src/core/model/PartPlate.h
        src/core/model/PartPlateList.cpp
        src/core/model/PartPlateList.h
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
        src/qml_gui/Renderer/SoftwareViewport.cpp
        src/qml_gui/Renderer/SoftwareViewport.h
        src/qml_gui/Renderer/RhiBackendSelector.cpp
        src/qml_gui/Renderer/RhiBackendSelector.h
        src/qml_gui/Renderer/RhiViewport.cpp
        src/qml_gui/Renderer/RhiViewport.h
        src/qml_gui/Renderer/RhiViewportRenderer.cpp
        src/qml_gui/Renderer/RhiViewportRenderer.h
        src/qml_gui/Renderer/PrepareSceneData.cpp
        src/qml_gui/Renderer/PrepareSceneData.h
        src/qml_gui/Renderer/GCodeRenderer.cpp
        src/qml_gui/Renderer/GCodeRenderer.h
    )
    if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/src/core/debug/CrashHandlerWin.cpp")
        target_sources(owzx_app_core PRIVATE
            src/core/debug/CrashHandlerWin.cpp
            src/core/debug/CrashHandlerWin.h
            src/core/debug/EarlyCrashDiag.cpp
        )
    endif()

    # v2.5 DEV-01: MQTT 客户端（paho-mqtt C 异步接口）
    target_sources(owzx_app_core PRIVATE
        src/core/services/MqttClient.h
        src/core/services/MqttClient.cpp
    )
    # v2.6 SSDP-01: SSDP 设备发现
    target_sources(owzx_app_core PRIVATE
        src/core/services/SsdpDiscovery.h
        src/core/services/SsdpDiscovery.cpp
    )
    # v2.6 CAM-01: 摄像头视频流（FFmpeg RTSP 解码）
    target_sources(owzx_app_core PRIVATE
        src/core/services/CameraStream.h
        src/core/services/CameraStream.cpp
    )
    # v2.6 CAM-03: 摄像头图像提供者（QQuickImageProvider → QML Image）
    target_sources(owzx_app_core PRIVATE
        src/qml_gui/CameraImageProvider.h
        src/qml_gui/CameraImageProvider.cpp
    )
    # v2.8 P2-C: FTP 上传服务（libcurl FTPS，发送打印任务到 Bambu 打印机）
    target_sources(owzx_app_core PRIVATE
        src/core/services/FtpUploader.h
        src/core/services/FtpUploader.cpp
    )
    # 链接 libcurl 静态库（DEPS_PREFIX/lib/libcurl.lib）
    # CURL_STATICLIB 必须定义，否则 curl.h 用 __declspec(dllimport) 声明符号
    # （静态库不提供 __imp_ 存根 → 链接错误 __imp_curl_easy_*）
    find_library(CURL_LIB NAMES libcurl curl
                 PATHS "${DEPS_PREFIX}/lib" NO_DEFAULT_PATH)
    if(CURL_LIB)
        target_compile_definitions(owzx_app_core PUBLIC CURL_STATICLIB)
        target_link_libraries(owzx_app_core PUBLIC ${CURL_LIB})
        target_include_directories(owzx_app_core PUBLIC "${DEPS_PREFIX}/include")
    else()
        message(WARNING "libcurl not found in DEPS_PREFIX, FTP upload disabled")
    endif()
    # 链接 FFmpeg 库（avcodec/avformat/avutil/swscale）
    foreach(ffmpeg_lib avcodec avformat avutil swscale)
        find_library(FFMPEG_${ffmpeg_lib}_LIB NAMES ${ffmpeg_lib}
                     PATHS "${DEPS_PREFIX}/lib" NO_DEFAULT_PATH)
        if(FFMPEG_${ffmpeg_lib}_LIB)
            target_link_libraries(owzx_app_core PUBLIC ${FFMPEG_${ffmpeg_lib}_LIB})
        else()
            message(WARNING "FFmpeg ${ffmpeg_lib} not found in DEPS_PREFIX")
        endif()
    endforeach()
    # 链接 paho-mqtt 静态库（DEPS_PREFIX/lib/paho-mqtt3a-static.lib）
    find_library(PAHO_MQTT_LIB NAMES paho-mqtt3a-static paho-mqtt3as-static
                 PATHS "${DEPS_PREFIX}/lib" NO_DEFAULT_PATH)
    if(PAHO_MQTT_LIB)
        target_link_libraries(owzx_app_core PUBLIC ${PAHO_MQTT_LIB})
    else()
        message(WARNING "paho-mqtt not found in DEPS_PREFIX, MQTT features disabled")
    endif()

    target_include_directories(owzx_app_core PUBLIC
        src
        ${CMAKE_SOURCE_DIR}/third_party/OrcaSlicer/src
    )
    target_compile_definitions(owzx_app_core PUBLIC OWZX_QML_GUI=1)
    if(BUILD_LIBSLIC3R AND TARGET libslic3r)
        target_compile_definitions(owzx_app_core PUBLIC HAS_LIBSLIC3R=1)
        if(MSVC)
            target_compile_definitions(owzx_app_core PRIVATE
                BOOST_ALL_NO_LIB
                BOOST_USE_WINAPI_VERSION=0x602
                BOOST_SYSTEM_USE_UTF8
            )
        endif()
    endif()
    # Qt AUTOMOC needs compile definitions and include paths from Qt modules.
    # libslic3r PUBLIC linkage propagates generated-header dirs (libslic3r_version.h).
    target_link_libraries(owzx_app_core PUBLIC
        Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::OpenGL Qt6::Concurrent Qt6::Network Qt6::GuiPrivate
    )
    if(BUILD_LIBSLIC3R AND TARGET libslic3r)
        target_link_libraries(owzx_app_core PUBLIC libslic3r)
    endif()

    # ── 主程序 ────────────────────────────────────────────────────────────
    qt_add_executable(OWzxSlicer
        src/qml_gui/main_qml.cpp
        src/qml_gui/qml.qrc
    )
    target_link_libraries(OWzxSlicer PRIVATE owzx_app_core)
    target_link_libraries(OWzxSlicer PRIVATE
        Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::OpenGL Qt6::Concurrent
    )
    qt_add_shaders(OWzxSlicer "rhi_viewport_shaders"
        PREFIX
            "/rhi_viewport/shaders"
        BASE
            "src/qml_gui/Renderer/shaders"
        FILES
            src/qml_gui/Renderer/shaders/rhi_viewport.vert
            src/qml_gui/Renderer/shaders/rhi_viewport.frag
    )
    if(MSVC)
        target_compile_options(OWzxSlicer PRIVATE /Zi)
        target_link_options(OWzxSlicer PRIVATE /DEBUG:FULL)
    # v2.8 P2-C: libcurl(OpenSSL) 与 paho-mqtt 都带静态 SHA1 实现 → LNK2005。
    # /FORCE:MULTIPLE 让链接器取第一个定义（paho 的 SHA1 先链接），忽略后续重复。
    target_link_options(OWzxSlicer PRIVATE /FORCE:MULTIPLE)
    endif()
    foreach(_runtime_dll freetype.dll libmpfr-4.dll libgmp-10.dll qhull.dll qhull_p.dll qhull_r.dll)
        if(EXISTS "${DEPS_PREFIX}/bin/${_runtime_dll}")
            add_custom_command(TARGET OWzxSlicer POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DEPS_PREFIX}/bin/${_runtime_dll}"
                    "$<TARGET_FILE_DIR:OWzxSlicer>/${_runtime_dll}")
        endif()
    endforeach()
    if(BUILD_LIBSLIC3R AND TARGET libslic3r)
        # Copy OCCT runtime DLLs (delay-loaded, needed when libslic3r actually calls them)
        file(GLOB _occt_dlls "${DEPS_PREFIX}/bin/occt/TK*.dll")
        foreach(_occt_dll ${_occt_dlls})
            add_custom_command(TARGET OWzxSlicer POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_occt_dll}"
                    "$<TARGET_FILE_DIR:OWzxSlicer>/")
        endforeach()
    endif()
    # v2.6 CAM: Copy FFmpeg runtime DLLs (avcodec/avformat/avutil/swscale + deps)
    # FFmpeg is linked via import libs (.lib) but DLLs are version-suffixed in bin/ffmpeg/.
    # Without these the EXE fails to launch with STATUS_DLL_NOT_FOUND (0xC0000135).
    file(GLOB _ffmpeg_dlls "${DEPS_PREFIX}/bin/ffmpeg/*.dll")
    foreach(_ffmpeg_dll ${_ffmpeg_dlls})
        add_custom_command(TARGET OWzxSlicer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_ffmpeg_dll}"
                "$<TARGET_FILE_DIR:OWzxSlicer>/")
    endforeach()
    # Note: paho-mqtt is linked statically (paho-mqtt3a-static.lib), no runtime DLL needed.
    # v2.7 P0: Copy libslic3r runtime resources (nozzle_info.json etc.) — the slice
    # engine reads resources/info/nozzle_info.json at runtime; missing it causes
    # get_hrc_by_nozzle_type parse errors and slice failures.
    add_custom_command(TARGET OWzxSlicer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:OWzxSlicer>/resources/info"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/third_party/OrcaSlicer/resources/info"
            "$<TARGET_FILE_DIR:OWzxSlicer>/resources/info")
else()
    find_package(Qt6 REQUIRED COMPONENTS Widgets)

    qt_add_executable(OWzxSlicer
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

    target_link_libraries(OWzxSlicer PRIVATE Qt6::Widgets)
endif()

if(WIN32)
    set_target_properties(OWzxSlicer PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# ── Tests ──────────────────────────────────────────────────────────────────
if(BUILD_TESTING AND OWZX_QML_GUI)
    # Ensure Qt DLLs are on PATH when running tests via ctest
    get_target_property(_qt6_core_location Qt6::Core LOCATION)
    get_filename_component(_qt6_bin_dir "${_qt6_core_location}" DIRECTORY)

    qt_add_executable(E2EWorkflowTests tests/E2EWorkflowTests.cpp)
    target_link_libraries(E2EWorkflowTests PRIVATE owzx_app_core
        Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::OpenGL Qt6::Concurrent Qt6::Network Qt6::Test
    )
    target_link_options(E2EWorkflowTests PRIVATE /FORCE:MULTIPLE)
    add_test(NAME E2EWorkflowTests COMMAND E2EWorkflowTests)
    set_tests_properties(E2EWorkflowTests PROPERTIES
        ENVIRONMENT "PATH=${_qt6_bin_dir};$ENV{PATH}"
    )

    qt_add_executable(ViewModelSmokeTests tests/ViewModelSmokeTests.cpp)
    # AUTOMOC fix (v3.0 retrospective): single-file tests with cpp-internal
    # Q_OBJECT have weak moc dependency tracking — adding a new slot to the
    # .cpp doesn't reliably re-run AUTOMOC, so the test binary uses stale moc
    # metadata and the new test silently doesn't execute. CMake's
    # CMAKE_AUTOMOC_DEPEND_FILTERS doesn't cover cpp-internal Q_OBJECT; the
    # robust fix is to re-run cmake configure (which re-scans AUTOMOC) after
    # editing the test class. The canonical verify script does this; for
    # incremental builds, delete the autogen timestamp before building:
    #   rm build/ViewModelSmokeTests_autogen/timestamp
    target_link_libraries(ViewModelSmokeTests PRIVATE owzx_app_core
        Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::OpenGL Qt6::Concurrent Qt6::Network Qt6::Test
    )
    target_link_options(ViewModelSmokeTests PRIVATE /FORCE:MULTIPLE)
    add_test(NAME ViewModelSmokeTests COMMAND ViewModelSmokeTests)
    set_tests_properties(ViewModelSmokeTests PROPERTIES
        ENVIRONMENT "PATH=${_qt6_bin_dir};$ENV{PATH}"
    )

    qt_add_executable(QmlUiAuditTests tests/QmlUiAuditTests.cpp)
    target_link_libraries(QmlUiAuditTests PRIVATE Qt6::Test)
    target_compile_definitions(QmlUiAuditTests PRIVATE
        QT_TESTCASE_SOURCEDIR="${CMAKE_SOURCE_DIR}"
    )
    add_test(NAME QmlUiAuditTests COMMAND QmlUiAuditTests)
    set_tests_properties(QmlUiAuditTests PROPERTIES
        ENVIRONMENT "PATH=${_qt6_bin_dir};$ENV{PATH}"
    )

    qt_add_executable(PrepareSceneDataTests
        tests/PrepareSceneDataTests.cpp
        src/qml_gui/Renderer/PrepareSceneData.cpp
        src/qml_gui/Renderer/PrepareSceneData.h
    )
    target_include_directories(PrepareSceneDataTests PRIVATE src)
    target_link_libraries(PrepareSceneDataTests PRIVATE Qt6::Test Qt6::Core)
    add_test(NAME PrepareSceneDataTests COMMAND PrepareSceneDataTests)
    set_tests_properties(PrepareSceneDataTests PROPERTIES
        ENVIRONMENT "PATH=${_qt6_bin_dir};$ENV{PATH}"
    )

endif()

# ── CLI ────────────────────────────────────────────────────────────────────
if(OWZX_RENDER_BENCH AND OWZX_QML_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui GuiPrivate ShaderTools)

    qt_add_executable(owzx-render-bench
        tools/render_bench/main.cpp
    )
    target_include_directories(owzx-render-bench PRIVATE
        tools/render_bench
    )
    set_target_properties(owzx-render-bench PROPERTIES WIN32_EXECUTABLE FALSE)
    target_link_libraries(owzx-render-bench PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::GuiPrivate
    )
    qt_add_shaders(owzx-render-bench "render_bench_shaders"
        PREFIX
            "/render_bench/shaders"
        BASE
            "tools/render_bench/shaders"
        FILES
            tools/render_bench/shaders/segment.vert
            tools/render_bench/shaders/segment.frag
    )
endif()

if(BUILD_CLI AND BUILD_LIBSLIC3R AND TARGET libslic3r)
    add_library(owzx_cli_core OBJECT
        src/nanosvg_impl.cpp
        src/core/model/PartPlate.cpp
        src/core/model/PartPlate.h
        src/core/model/PartPlateList.cpp
        src/core/model/PartPlateList.h
        src/core/services/ProjectServiceMock.cpp
        src/core/services/ProjectServiceMock.h
        src/core/services/SliceService.cpp
        src/core/services/SliceService.h
        src/core/services/PresetServiceMock.cpp
        src/core/services/PresetServiceMock.h
        src/core/services/JobBase.cpp
        src/core/services/JobBase.h
        src/core/services/JobManager.cpp
        src/core/services/JobManager.h
        src/cli/CliRunner.cpp
        src/cli/CliRunner.h
    )
    target_include_directories(owzx_cli_core PUBLIC
        src
        ${CMAKE_SOURCE_DIR}/third_party/OrcaSlicer/src
    )
    target_compile_definitions(owzx_cli_core PUBLIC
        HAS_LIBSLIC3R=1
    )
    if(MSVC)
        target_compile_definitions(owzx_cli_core PRIVATE
            BOOST_ALL_NO_LIB
            BOOST_USE_WINAPI_VERSION=0x602
            BOOST_SYSTEM_USE_UTF8
        )
    endif()
    target_link_libraries(owzx_cli_core PUBLIC
        Qt6::Core Qt6::Gui Qt6::Concurrent libslic3r
    )

    add_executable(owzx-cli src/cli/main_cli.cpp)
    target_link_libraries(owzx-cli PRIVATE owzx_cli_core)
    # Console app — NOT WIN32_EXECUTABLE

    if(BUILD_TESTING)
        find_package(Qt6 REQUIRED COMPONENTS Test)
        qt_add_executable(CliTests tests/CliTests.cpp)
        target_link_libraries(CliTests PRIVATE Qt6::Test)
        target_compile_definitions(CliTests PRIVATE
            QT_TESTCASE_SOURCEDIR="${CMAKE_SOURCE_DIR}"
        )
        add_dependencies(CliTests owzx-cli)
        add_test(NAME CliTests COMMAND CliTests)
        set_tests_properties(CliTests PROPERTIES
            ENVIRONMENT "PATH=${_qt6_bin_dir};$ENV{PATH}"
            TIMEOUT 300
        )
    endif()
endif()

# I2 -- i18n 翻译文件
qt_add_translations(OWzxSlicer
    TS_FILES
        i18n/zh_CN.ts
        i18n/en.ts
        i18n/ja.ts
        i18n/ko.ts
        i18n/de.ts
        i18n/fr.ts
)
