# ── QML module (S8, 2026-10-14) ──────────────────────────────────────────
# The bare .qrc manifests (qml_pages/qml_dialogs/qml_components) are replaced
# by a real qt_add_qml_module module tree:
#   * qmlcachegen compiles every QML file to bytecode (startup no longer
#     parses QML source),
#   * the module auto-generates qmldir/qmltypes (the handwritten
#     src/qml_gui/qmldir singleton declaration is retired),
#   * qmllint/qmlls tooling targets are created (all_qmllint feeds the
#     canonical verify gate),
#   * relative imports ("import \"..\"" etc.) are replaced by the module
#     URI `import OWzx`.
# This subdirectory CMakeLists exists so QML_FILES are listed relative to the
# module root (main.qml, pages/HomePage.qml, ...): the resource paths become
# :/qml/OWzx/pages/HomePage.qml instead of carrying a src/qml_gui/ prefix.
#
# Resource layout: RESOURCE_PREFIX /qml + URI OWzx puts the module at
# :/qml/OWzx/... — the sibling :/qml/assets, :/qml/data, :/web/chat and
# :/qtquickcontrols2.conf trees keep their historical paths (qml_assets/
# qml_web.qrc stay plain AUTORCC manifests, attached to owzx_qml so the app
# and QmlSmokeTests embed the identical tree from one source of truth).
# The module is attached to a dedicated STATIC library rather than the
# OWzxSlicer exe because QmlSmokeTests (Q14) must embed the exact same
# shipped QML tree, and rather than owzx_app_core because qt_add_qml_module
# does not support OBJECT libraries (Qt6QmlMacros.cmake: "need to reject
# ${target} being an object library as unsupported").
qt_add_library(owzx_qml STATIC)
# Theme is the module singleton (pragma Singleton in the file); the CMake
# source property is the other required half — without it the generated
# qmldir lists Theme as an instantiable type and every Theme.* lookup in
# deferred/style-created components resolves to undefined.
set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
# QTP0004 NEW: emit "prefer :/qml" qmldir files for the module's
# subdirectories (pages/controls/...) so filesystem-loaded QML (the
# --qml-dev flow) resolves subdirectory types from the embedded tree.
qt_policy(SET QTP0004 NEW)
qt_add_qml_module(owzx_qml
    URI OWzx
    VERSION 1.0
    RESOURCE_PREFIX /qml
    NO_PLUGIN
    # Build-dir layout mirrors the URI so qmllint/qmlls resolve the module
    # via import paths (silences the "target path" tooling warning); resource
    # paths inside the binaries are unaffected.
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/OWzx
    # C++ QML-facing types owned by the module (S8): BackendContext is the
    # `backend` singleton (moved out of the owzx_app_core OBJECT library,
    # which qt_add_qml_module cannot host); UiSettingsStore is the
    # `appSettings` singleton (formerly a cpp-local class behind a root
    # context property). QML_ELEMENT/QML_SINGLETON here make both statically
    # visible to qmlcachegen/qmllint. The types must live in THIS target:
    # qmltyperegistrar only processes the module target's own metatypes.
    SOURCES
        BackendContext.h
        BackendContext.cpp
        UiSettingsStore.h
    QML_FILES
        main.qml
        BBLTopbar.qml
        Theme.qml
        pages/HomePage.qml
        pages/PreparePage.qml
        pages/PreviewPage.qml
        pages/AssemblePage.qml
        pages/Plater.qml
        pages/MonitorPage.qml
        pages/ProjectPage.qml
        pages/CalibrationPage.qml
        pages/PreferencesPage.qml
        pages/MultiMachinePage.qml
        panels/LeftSidebar.qml
        panels/DockableSidebar.qml
        panels/ChatSidebar.qml
        components/NavigatorLabels.qml
        components/ErrorToast.qml
        components/CommandPalette.qml
        components/ErrorBanner.qml
        components/CustomGcodeDialog.qml
        components/MoveSlider.qml
        components/PreviewLayerRail.qml
        components/Legend.qml
        components/VisibilityFilter.qml
        components/StatsPanel.qml
        components/ToolPositionTooltip.qml
        components/NotificationCenter.qml
        components/OptionRow.qml
        components/GLToolbars.qml
        components/PrepareContextMenus.qml
        controls/CxBadge.qml
        controls/CxButton.qml
        controls/CxBusyIndicator.qml
        controls/CxComboBox.qml
        controls/CxEmptyState.qml
        controls/CxTextField.qml
        controls/CxSlider.qml
        controls/CxCheckBox.qml
        controls/CxSpinBox.qml
        controls/CxIconButton.qml
        controls/CxNumericEdit.qml
        controls/CxStepButton.qml
        controls/CxSwitch.qml
        controls/CxDialog.qml
        controls/CxPopup.qml
        controls/CxMenu.qml
        controls/CxMenuItem.qml
        controls/CxScrollView.qml
        controls/CxProgressBar.qml
        controls/CxTextArea.qml
        dialogs/PrintDialog.qml
        dialogs/CalibrationDialog.qml
        dialogs/CaliHistoryDialog.qml
        dialogs/AboutDialog.qml
        dialogs/ConfigWizardDialog.qml
        dialogs/BedShapeDialog.qml
        dialogs/EditGCodeDialog.qml
        dialogs/AccessCodeInputDialog.qml
        dialogs/AMSSettingsDialog.qml
        dialogs/FirmwareDialog.qml
        dialogs/WipeTowerDialog.qml
        dialogs/PrintHostDialog.qml
        dialogs/PluginManagerDialog.qml
        dialogs/EnableLiteModeDialog.qml
        dialogs/KBShortcutsDialog.qml
        dialogs/SavePresetDialog.qml
        dialogs/CreatePresetsDialog.qml
        dialogs/UnsavedChangesDialog.qml
        dialogs/PresetDiffDialog.qml
        dialogs/ConfirmDialog.qml
        dialogs/SelectionSettingsDialog.qml
        dialogs/ObjectLayersDialog.qml
        dialogs/ExportPresetBundleDialog.qml
        dialogs/FilamentGroupPopup.qml
        dialogs/SettingsDialog.qml
        dialogs/NetworkTestDialog.qml
        dialogs/TroubleshootDialog.qml
        dialogs/SelectMachineDialog.qml
        dialogs/SingleChoiceDialog.qml
        dialogs/RecenterDialog.qml
        dialogs/FileArchiveDialog.qml
        dialogs/ObjColorDialog.qml
        dialogs/SysInfoDialog.qml
)
# NOTE: qml_assets.qrc / qml_web.qrc (the non-QML :/qml/assets, :/qml/data,
# :/web/chat and :/qtquickcontrols2.conf trees) deliberately stay plain
# AUTORCC manifests of the CONSUMING executables (OWzxSlicer, QmlSmokeTests),
# not sources of this static library: a static archive drops unreferenced
# resource TUs, and the qrc registration objects have no incoming symbol
# references (verified 2026-10-14 — the conf/fonts/icons vanished from the
# linked test exe while the module's own resources survived via qmlcache).
target_link_libraries(owzx_qml PRIVATE
    Qt6::Qml Qt6::Quick
    # BackendContext.cpp compiles against the whole viewmodel/service surface;
    # the OBJECT library's usage requirements (includes, HAS_LIBSLIC3R, Qt
    # modules) feed this target's compilation and its objects ride the link
    # closure of every owzx_qml consumer.
    owzx_app_core
)
