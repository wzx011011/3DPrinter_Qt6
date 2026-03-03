cmake_minimum_required(VERSION 3.16)

project(FramelessDialogDemo LANGUAGES CXX)

option(CREALITY_QML_GUI "Build QML-first GUI architecture" ON)
option(ENABLE_VISUAL_REGRESSION "Enable visual regression tests" ON)
option(ENABLE_COVERAGE "Enable compiler coverage instrumentation" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(CTest)

if(CREALITY_QML_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Qml Quick QuickControls2 LinguistTools OpenGL)

    qt_add_executable(FramelessDialogDemo
        src/qml_gui/main_qml.cpp
        src/qml_gui/BackendContext.cpp
        src/qml_gui/BackendContext.h
        src/core/services/SliceServiceMock.cpp
        src/core/services/SliceServiceMock.h
        src/core/services/PresetServiceMock.cpp
        src/core/services/PresetServiceMock.h
        src/core/services/DeviceServiceMock.cpp
        src/core/services/DeviceServiceMock.h
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
        src/qml_gui/qml.qrc
    )

    target_link_libraries(FramelessDialogDemo PRIVATE Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::OpenGL)
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

target_include_directories(FramelessDialogDemo PRIVATE src)

if(WIN32)
    set_target_properties(FramelessDialogDemo PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# I2 — i18n 翼译文件
qt_add_translations(FramelessDialogDemo
    TS_FILES
        i18n/zh_CN.ts
        i18n/en.ts
)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test Quick Qml OpenGL)

    if(ENABLE_COVERAGE AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
        add_compile_options(-O0 -g --coverage)
        add_link_options(--coverage)
    endif()

    qt_add_executable(ViewModelSmokeTests
        tests/ViewModelSmokeTests.cpp
        src/core/services/SliceServiceMock.cpp
        src/core/services/SliceServiceMock.h
        src/core/services/PresetServiceMock.cpp
        src/core/services/PresetServiceMock.h
        src/core/services/DeviceServiceMock.cpp
        src/core/services/DeviceServiceMock.h
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
    )

    target_include_directories(ViewModelSmokeTests PRIVATE src)
    target_link_libraries(ViewModelSmokeTests PRIVATE Qt6::Core Qt6::Test)

    add_test(
        NAME ViewModelSmokeTests
        COMMAND
            ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            $<TARGET_FILE:ViewModelSmokeTests>
    )

    if(ENABLE_VISUAL_REGRESSION)
        qt_add_executable(VisualRegressionTests
            tests/visual/VisualRegressionTests.cpp
            src/qml_gui/BackendContext.cpp
            src/qml_gui/BackendContext.h
            src/core/services/SliceServiceMock.cpp
            src/core/services/SliceServiceMock.h
            src/core/services/PresetServiceMock.cpp
            src/core/services/PresetServiceMock.h
            src/core/services/DeviceServiceMock.cpp
            src/core/services/DeviceServiceMock.h
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
            tests/visual/GLViewportTestStub.cpp
            tests/visual/GLViewportTestStub.h
            src/qml_gui/qml.qrc
        )

        target_include_directories(VisualRegressionTests PRIVATE src tests/visual)
        target_link_libraries(VisualRegressionTests PRIVATE Qt6::Core Qt6::Test Qt6::Quick Qt6::Qml)

        add_test(
            NAME VisualRegressionTests
            COMMAND
                ${CMAKE_COMMAND} -E env
                "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
                "CREALITY_VISUAL_COMPARE_MODE=1"
                "QSG_RHI_BACKEND=opengl"
                "CREALITY_UI_CAPTURE_ONLY=1"
                $<TARGET_FILE:VisualRegressionTests>
        )
    endif()

    if(ENABLE_COVERAGE AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
        find_program(GCOVR_EXECUTABLE gcovr)
        if(GCOVR_EXECUTABLE)
            add_custom_target(coverage_report
                COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
                COMMAND ${GCOVR_EXECUTABLE} --root ${CMAKE_SOURCE_DIR} --exclude "tests/.*" --xml-pretty --output ${CMAKE_BINARY_DIR}/coverage.xml
                COMMAND ${GCOVR_EXECUTABLE} --root ${CMAKE_SOURCE_DIR} --exclude "tests/.*" --html-details ${CMAKE_BINARY_DIR}/coverage.html
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Running tests and generating coverage report"
            )
        endif()
    endif()
endif()
