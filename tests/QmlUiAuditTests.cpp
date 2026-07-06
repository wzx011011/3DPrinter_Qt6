// QmlUiAuditTests — Phase 55-04 additions.
//
// AUTOMOC caveat (v3.0 retrospective, see ViewModelSmokeTests CMake comment):
// single-file QtTest with cpp-internal Q_OBJECT has weak moc dependency
// tracking. After adding a new private slot here, re-run cmake configure (the
// canonical verify script does this) or delete
//   build/QmlUiAuditTests_autogen/timestamp
// before rebuilding, otherwise the new slot silently does not execute.
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QtTest>

class QmlUiAuditTests final : public QObject
{
  Q_OBJECT

private slots:
  void topLevelUiHasNoVisiblePlaceholdersOrNoopActions();
  void mainChromeUsesThemeTokens();
  void sidebarCopyIsLocalizedAndOperationalTextIsReadable();
  void mainRegistersRhiViewportByDefaultWithSoftwareFallback();
  void mainRegistersRhiViewportOnlyBehindExplicitGate();
  void renderBenchmarkMatchesRhiBackendPolicy();
  void legacyOpenGlViewportPathsStayDeleted();
  void prepareViewportBindsBedAndPlateContext();
  void prepareReadinessControlsBindBackendAvailability();
  void exportUiUsesSaveDialogAndAvoidsSourcePathTarget();
  void importEntryPointsAdvertiseConsistentModelFormats();
  void rhiViewportRendererUsesPrepareSceneDataAndDirtyUploads();
  void rhiViewportRendererUsesModelBuffersAndCameraUniforms();
  void previewRhiRendererBindsPreviewStateAndUsesExactDrawSpans();
  void previewRhiViewportFitsCameraToPreviewDataBeforeOrbit();
  void previewRhiRendererResetsGpuStateAfterResourceRelease();
  void previewRhiInteractionSettersPreservePreviewPayload();
  void previewNormalPathCoversFullWorkflowBindingsAndDiagnostics();
  void previewLayoutRestoresScreenshotRegionsAndGcodePanel();
  void previewStatsPanelCallsOnlyQmlInvokableSetters();
  void rhiViewportSelectionPickingBridgeStaysCppOwned();
  void rhiViewportModelDragOrbitsAfterClickThreshold();
  void rhiMoveGizmoDragBridgeStaysCppOwned();
  void rhiRotateScaleGizmoBridgeStaysCppOwned();
  void rhiGizmosRenderAsDepthIndependentOverlay();
  void rhiCutPlaneAndWipeTowerStayCppOwned();
  void visiblePlaceholderSurfacesAreHonest();
  // Phase 22 (UI-3): actively guard the v3.0 Phase 17 plate-lifecycle menu wiring
  void plateContextMenuItemsWiredAndNonEmpty();
  // Phase 51-03 (SHELL-03): BBLTopbar action controls bind to the BackendContext gate properties.
  void shellActionsBindToBackendContextGates();
  void mainWindowDefaultsToFramelessMaximizedShell();
  // Phase 51-03 (SHELL-04): the 3 notification surfaces keep non-overlapping placement.
  void notificationSurfacesStayNonOverlapping();
  // Phase 52-03 (PREPSB-01..04): LeftSidebar + FilamentSlot bindings are
  // present and there is no silent dead UI or empty handler.
  void leftSidebarPresetControlsAreWiredAndHonest();
  // Phase 53: Prepare object, plate, and viewport actions bind to C++ gates.
  void prepareWorkflowActionsBindCppGates();
  // Phase 76: Prepare workflow panels must stay compact and backend-gated.
  void prepareWorkflowPanelsMatchRestorationContract();
  // Phase 77: Prepare viewport controls and gizmo panels must be icon-first.
  void prepareViewportControlsMatchRestorationContract();
  // Quick 260705-vkn: pixel-level Prepare left sidebar restoration.
  void prepareLeftSidebarMatchesPixelRestorationContract();
  // Quick 260706-r8m: full Prepare page visual parity anchors.
  void prepareFullVisualParityContract();
  // Quick 260706-uix: restored Prepare controls must be actionable.
  void prepareRestoredControlsAreActionable();
  // Phase 78: final Prepare cleanup must keep restored paths active and stale
  // paths absent.
  void prepareRestorationMilestoneHasCleanupCoverage();
  // Phase 55-04 (GCODE-04/05): source-audit guards for the SoftwareViewport
  // absence, the computePreviewDrawRanges role-skip block, and the
  // GcvPackedSegment sizeof wire-format lockstep.
  void previewPageNeverReferencesSoftwareViewport();
  void rhiViewportRendererComputePreviewDrawRangeAppliesRoleFilter();
  void rhiViewportRendererHasGcvPackedSegmentRoleGuard();
  // Phase 55-05 (GCODE-04): D3D11 default + SoftwareViewport fallback-only startup
  // policy. Phase-55-tagged so a future regression that flips the default points
  // directly at the requirement (complements the existing mainRegistersRhiViewport*
  // tests with a GCODE-04-specific assertion bundle).
  void gcode04RhiViewportIsDefaultRegistrationNoSoftwareViewportInPreviewPath();
  // Phase 56-01: Wave 0 RED test scaffolds for settings dialog UI audit.
  void settingsDialogUsesOnlyCxControls();
  void settingsDialogNoRawControls();
  void settingsDialogStringsQsTr();
  void settingsDialogMainQmlDispatchStructural();
  void settingsDialogFiltersByTabAndGroup();
  void settingsDialogReadOnlySaveOpensSaveAs();
  void leftSidebarParamsPanelUsesRealOptionRows();
  // Phase 57-02 (CLEAN-01/02 regression): the 7 obsolete QML files locked by
  // Phase 50 section 1.6 (SettingsPage/ConfigPage/ParamsPage/SearchDialog)
  // plus the legacy Sidebar/FilamentPanel/PrintSettings deferred by Phase 52
  // must stay deleted from disk and out of qml.qrc. Locks the Wave 1 deletion
  // as a permanent ctest invariant.
  void deletedSettingsPathsStayAbsent();
  // Phase 57-02 (CLEAN-01 regression): the 3 named routes plus the dead
  // deferred-config-exit machinery excised from BackendContext/ConfigViewModel
  // in Wave 1 must stay removed. Fails loudly if any token is reintroduced.
  void deletedRoutesStayAbsent();

private:
  QString readSource(const QString &relativePath) const;
};

QString QmlUiAuditTests::readSource(const QString &relativePath) const
{
  QFile file(QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR)).filePath(relativePath));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return {};
  return QString::fromUtf8(file.readAll());
}

void QmlUiAuditTests::topLevelUiHasNoVisiblePlaceholdersOrNoopActions()
{
  const QString mainQml = readSource(QStringLiteral("src/qml_gui/main.qml"));
  const QString topbar = readSource(QStringLiteral("src/qml_gui/BBLTopbar.qml"));
  const QString plater = readSource(QStringLiteral("src/qml_gui/pages/Plater.qml"));
  QVERIFY2(!mainQml.isEmpty(), "Unable to read main.qml");
  QVERIFY2(!topbar.isEmpty(), "Unable to read BBLTopbar.qml");
  QVERIFY2(!plater.isEmpty(), "Unable to read Plater.qml");

  QVERIFY2(!mainQml.contains(QStringLiteral("占位 Tab")),
           "main.qml must not expose placeholder tab copy at runtime");
  QVERIFY2(!topbar.contains(QStringLiteral("label: qsTr(\"占位\")")),
           "BBLTopbar must not expose placeholder notebook tabs");
  QVERIFY2(!plater.contains(QStringLiteral("reserved (v2.0 placeholder)")),
           "Plater must not expose implementation-phase placeholder copy");

  const QStringList noOpPatterns = {
    QStringLiteral("onExportProjectRequested: { /* TODO"),
    QStringLiteral("onExportModelRequested: { /* TODO"),
    QStringLiteral("onPreferencesRequested: { /* TODO"),
    QStringLiteral("CxMenuItem { text: qsTr(\"Documentation\"); onTriggered: {}"),
    QStringLiteral("MenuItem { text: qsTr(\"关于 OWzx\"); onTriggered: {}")
  };
  for (const QString &pattern : noOpPatterns) {
    QVERIFY2(!mainQml.contains(pattern) && !topbar.contains(pattern),
             qPrintable(QStringLiteral("Top-level no-op action remains: %1").arg(pattern)));
  }
}

void QmlUiAuditTests::mainChromeUsesThemeTokens()
{
  const QString topbar = readSource(QStringLiteral("src/qml_gui/BBLTopbar.qml"));
  QVERIFY2(!topbar.isEmpty(), "Unable to read BBLTopbar.qml");
  const QStringList forbiddenTopbarColors = {
    QStringLiteral("#181818"),
    QStringLiteral("#2a2a2a"),
    QStringLiteral("#4CD582"),
    QStringLiteral("#c0c0c0"),
    QStringLiteral("#303030")
  };

  for (const QString &color : forbiddenTopbarColors) {
    QVERIFY2(!topbar.contains(color),
             qPrintable(QStringLiteral("Topbar chrome should use Theme tokens instead of %1").arg(color)));
  }
}

void QmlUiAuditTests::sidebarCopyIsLocalizedAndOperationalTextIsReadable()
{
  const QString sidebar = readSource(QStringLiteral("src/qml_gui/panels/LeftSidebar.qml"));
  QVERIFY2(!sidebar.isEmpty(), "Unable to read LeftSidebar.qml");

  const QStringList mixedLanguageLabels = {
    QStringLiteral("qsTr(\"Process\")"),
    QStringLiteral("qsTr(\"Global\")"),
    QStringLiteral("qsTr(\"Objects\")"),
    QStringLiteral("qsTr(\"Advanced\")"),
    QStringLiteral("qsTr(\"Object Settings\")"),
    QStringLiteral("qsTr(\"Layer Height\")"),
    QStringLiteral("qsTr(\"Infill Density\")"),
    QStringLiteral("qsTr(\"Print Speed\")"),
    QStringLiteral("qsTr(\"Variable Layer Height\")"),
    QStringLiteral("qsTr(\"Variable layer height editor (reserved)\")"),
    QStringLiteral("qsTr(\"(parameter list — reserved)\")"),
    QStringLiteral("qsTr(\"Rename Preset\")")
  };

  for (const QString &label : mixedLanguageLabels) {
    QVERIFY2(!sidebar.contains(label),
             qPrintable(QStringLiteral("Mixed-language or reserved runtime label remains: %1").arg(label)));
  }

  const QRegularExpression tinyFont(QStringLiteral("font\\.pixelSize:\\s*(?:7|8|9)\\b"));
  QVERIFY2(!tinyFont.match(sidebar).hasMatch(),
           "LeftSidebar operational controls should not use sub-10px text");
}

void QmlUiAuditTests::mainRegistersRhiViewportByDefaultWithSoftwareFallback()
{
  const QString mainCpp = readSource(QStringLiteral("src/qml_gui/main_qml.cpp"));
  const QString verifyScript = readSource(QStringLiteral("scripts/auto_verify_with_vcvars.ps1"));
  const QString selectorSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiBackendSelector.cpp"));
  QVERIFY2(!mainCpp.isEmpty(), "Unable to read main_qml.cpp");
  QVERIFY2(!verifyScript.isEmpty(), "Unable to read auto_verify_with_vcvars.ps1");
  QVERIFY2(!selectorSource.isEmpty(), "Unable to read RhiBackendSelector.cpp");

  QVERIFY2(!mainCpp.contains(QStringLiteral("OWZX_OPENGL")),
            "main_qml.cpp must not keep the retired OpenGL viewport environment gate");
  QVERIFY2(mainCpp.contains(QStringLiteral("qputenv(\"OWZX_RHI_RENDERER\", \"auto\")")),
            "default startup must enable QRhi auto instead of the software viewport");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<RhiViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
            "default GLViewport registration must use RhiViewport when QRhi initializes");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<SoftwareViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
            "SoftwareViewport must remain registered as the QRhi fallback");
  QVERIFY2(!mainCpp.contains(QStringLiteral("qmlRegisterType<GLViewport>")),
            "retired OpenGL GLViewport must not be registered");
  const int defaultCandidatesStart = selectorSource.indexOf(QStringLiteral("QVector<RhiBackendCandidate> defaultWindowsCandidates()"));
  const int candidatesForRequestStart = selectorSource.indexOf(QStringLiteral("QVector<RhiBackendCandidate> candidatesForRequest"));
  QVERIFY2(defaultCandidatesStart >= 0 && candidatesForRequestStart > defaultCandidatesStart,
           "RhiBackendSelector default candidate boundaries changed; update app QRhi policy audit");
  const QString defaultCandidates = selectorSource.mid(defaultCandidatesStart,
                                                       candidatesForRequestStart - defaultCandidatesStart);
  QVERIFY2(defaultCandidates.indexOf(QStringLiteral("Direct3D11"))
               < defaultCandidates.indexOf(QStringLiteral("Direct3D12")),
           "default QRhi auto policy must prefer stable D3D11 before D3D12");
  QVERIFY2(!verifyScript.contains(QStringLiteral("OWZX_OPENGL")),
           "canonical startup smoke should cover the default QRhi/D3D11 path");
  QVERIFY2(!verifyScript.contains(QStringLiteral("$env:OWZX_RHI_RENDERER")),
           "canonical startup smoke should not force a non-default QRhi backend");
}

void QmlUiAuditTests::mainRegistersRhiViewportOnlyBehindExplicitGate()
{
  const QString mainCpp = readSource(QStringLiteral("src/qml_gui/main_qml.cpp"));
  const QString verifyScript = readSource(QStringLiteral("scripts/auto_verify_with_vcvars.ps1"));
  const QString selectorHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiBackendSelector.h"));
  const QString selectorSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiBackendSelector.cpp"));
  const QString rhiViewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString rhiViewportRendererHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.h"));
  const QString rhiViewportRenderer = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  const QString rhiVertexShader = readSource(QStringLiteral("src/qml_gui/Renderer/shaders/rhi_viewport.vert"));
  const QString rhiFragmentShader = readSource(QStringLiteral("src/qml_gui/Renderer/shaders/rhi_viewport.frag"));
  const QString cmake = readSource(QStringLiteral("CMakeLists.txt"));
  QVERIFY2(!mainCpp.isEmpty(), "Unable to read main_qml.cpp");
  QVERIFY2(!verifyScript.isEmpty(), "Unable to read auto_verify_with_vcvars.ps1");
  QVERIFY2(!selectorHeader.isEmpty(), "Unable to read RhiBackendSelector.h");
  QVERIFY2(!selectorSource.isEmpty(), "Unable to read RhiBackendSelector.cpp");
  QVERIFY2(!rhiViewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!rhiViewportRendererHeader.isEmpty(), "Unable to read RhiViewportRenderer.h");
  QVERIFY2(!rhiViewportRenderer.isEmpty(), "Unable to read RhiViewportRenderer.cpp");
  QVERIFY2(!rhiVertexShader.isEmpty(), "Unable to read rhi_viewport.vert");
  QVERIFY2(!rhiFragmentShader.isEmpty(), "Unable to read rhi_viewport.frag");
  QVERIFY2(!cmake.isEmpty(), "Unable to read CMakeLists.txt");

  QVERIFY2(mainCpp.contains(QStringLiteral("OWZX_RHI_RENDERER")),
           "QRhi viewport selection must keep OWZX_RHI_RENDERER override support");
  QVERIFY2(!mainCpp.contains(QStringLiteral("OWZX_OPENGL")),
           "legacy OWZX_OPENGL path must stay retired");
  QVERIFY2(!mainCpp.contains(QStringLiteral("qmlRegisterType<GLViewport>")),
           "legacy OpenGL GLViewport registration must stay removed");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<SoftwareViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
           "SoftwareViewport must remain the fallback GLViewport registration");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<RhiViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
           "RhiViewport must be registered under the existing OWzxGL.GLViewport type behind QRhi gate");
  QVERIFY2(!verifyScript.contains(QStringLiteral("OWZX_RHI_RENDERER")),
           "canonical verification must not enable QRhi by default");

  QVERIFY2(selectorHeader.contains(QStringLiteral("RhiBackendSelection")),
           "RhiBackendSelector must expose structured backend diagnostics");
  QVERIFY2(selectorSource.contains(QStringLiteral("Direct3D12")),
           "QRhi app selector must keep Direct3D12 available for explicit opt-in");
  QVERIFY2(selectorSource.contains(QStringLiteral("Direct3D11")),
           "QRhi app selector must keep Direct3D11 fallback on Windows");
  const int defaultCandidatesStart = selectorSource.indexOf(QStringLiteral("QVector<RhiBackendCandidate> defaultWindowsCandidates()"));
  const int candidatesForRequestStart = selectorSource.indexOf(QStringLiteral("QVector<RhiBackendCandidate> candidatesForRequest"));
  QVERIFY2(defaultCandidatesStart >= 0 && candidatesForRequestStart > defaultCandidatesStart,
           "RhiBackendSelector default candidate boundaries changed; update app QRhi policy audit");
  const QString defaultCandidates = selectorSource.mid(defaultCandidatesStart,
                                                       candidatesForRequestStart - defaultCandidatesStart);
  QVERIFY2(defaultCandidates.indexOf(QStringLiteral("Direct3D11"))
               < defaultCandidates.indexOf(QStringLiteral("Direct3D12")),
           "QRhi app auto policy must prefer stable D3D11 before D3D12 opt-in fallback");
  QVERIFY2(!selectorSource.contains(QStringLiteral("QRhi::Vulkan")),
           "Vulkan must not be part of the default app selector while QtGui Vulkan is disabled");
  QVERIFY2(rhiViewportHeader.contains(QStringLiteral("QQuickRhiItem")),
           "RhiViewport must use Qt Quick's QRhi item host");
  QVERIFY2(rhiViewportHeader.contains(QStringLiteral("Q_PROPERTY(QByteArray meshData")),
           "RhiViewport must expose GLViewport-compatible meshData binding");
  QVERIFY2(rhiViewportHeader.contains(QStringLiteral("CanvasPreview")),
           "RhiViewport must expose GLViewport-compatible canvas enum values");
  QVERIFY2(rhiViewportRendererHeader.contains(QStringLiteral("QQuickRhiItemRenderer")),
           "RhiViewportRenderer must use QQuickRhiItemRenderer");
  QVERIFY2(rhiViewportRenderer.contains(QStringLiteral(":/rhi_viewport/shaders/rhi_viewport.vert.qsb")),
           "RhiViewportRenderer must load app .qsb shader resources");
  QVERIFY2(cmake.contains(QStringLiteral("qt_add_shaders(OWzxSlicer \"rhi_viewport_shaders\"")),
           "OWzxSlicer must compile RhiViewport shaders through qt_add_shaders");
}

void QmlUiAuditTests::renderBenchmarkMatchesRhiBackendPolicy()
{
  const QString benchmark = readSource(QStringLiteral("tools/render_bench/main.cpp"));
  const QString verifyScript = readSource(QStringLiteral("scripts/auto_verify_with_vcvars.ps1"));
  const QString cmake = readSource(QStringLiteral("CMakeLists.txt"));
  QVERIFY2(!benchmark.isEmpty(), "Unable to read render_bench/main.cpp");
  QVERIFY2(!verifyScript.isEmpty(), "Unable to read auto_verify_with_vcvars.ps1");
  QVERIFY2(!cmake.isEmpty(), "Unable to read CMakeLists.txt");

  QVERIFY2(benchmark.contains(QStringLiteral("selectedBackend")),
           "Render benchmark JSON must use the same selectedBackend diagnostic name as app QRhi selection");
  QVERIFY2(benchmark.contains(QStringLiteral("attemptedBackends")),
           "Render benchmark JSON must report attemptedBackends for fallback traceability");
  QVERIFY2(benchmark.contains(QStringLiteral("fallbackFailures")),
           "Render benchmark JSON must preserve fallback failure diagnostics");
  QVERIFY2(benchmark.contains(QStringLiteral("QStringLiteral(\"d3d12\"), QRhi::D3D12"))
               && benchmark.contains(QStringLiteral("QStringLiteral(\"d3d11\"), QRhi::D3D11")),
           "Render benchmark auto policy must include D3D12 and D3D11");
  QVERIFY2(benchmark.contains(QStringLiteral("stableAuto"))
               && !benchmark.mid(benchmark.indexOf(QStringLiteral("stableAuto")),
                                  benchmark.indexOf(QStringLiteral("if (requested == QLatin1String(\"auto\""))
                                      - benchmark.indexOf(QStringLiteral("stableAuto")))
                       .contains(QStringLiteral("vulkan")),
           "Render benchmark auto policy must not include Vulkan");
  QVERIFY2(benchmark.contains(QStringLiteral("QtGui was built without public Vulkan support")),
           "Render benchmark all-mode must report Vulkan disabled status when QtGui lacks Vulkan");
  QVERIFY2(verifyScript.contains(QStringLiteral("OWZX_RENDER_BENCH"))
               && verifyScript.contains(QStringLiteral("-DOWZX_RENDER_BENCH=$(if ($renderBenchEnabled) { 'ON' } else { 'OFF' })")),
           "Canonical verification must keep render benchmark optional behind OWZX_RENDER_BENCH");
  QVERIFY2(!verifyScript.contains(QStringLiteral("$env:OWZX_RHI_RENDERER")),
           "Canonical verification must not enable the app QRhi viewport while running benchmark checks");
  QVERIFY2(cmake.contains(QStringLiteral("option(OWZX_RENDER_BENCH"))
                && cmake.contains(QStringLiteral("qt_add_executable(owzx-render-bench"))
                && cmake.contains(QStringLiteral("qt_add_shaders(owzx-render-bench \"render_bench_shaders\"")),
            "Render benchmark target and shaders must remain build-gated in CMake");
}

void QmlUiAuditTests::legacyOpenGlViewportPathsStayDeleted()
{
  const QString mainCpp = readSource(QStringLiteral("src/qml_gui/main_qml.cpp"));
  const QString cmake = readSource(QStringLiteral("CMakeLists.txt"));
  QVERIFY2(!mainCpp.isEmpty(), "Unable to read main_qml.cpp");
  QVERIFY2(!cmake.isEmpty(), "Unable to read CMakeLists.txt");

  const QStringList deletedFiles = {
    QStringLiteral("src/qml_gui/Renderer/GLViewport.cpp"),
    QStringLiteral("src/qml_gui/Renderer/GLViewport.h"),
    QStringLiteral("src/qml_gui/Renderer/GLViewportRenderer.cpp"),
    QStringLiteral("src/qml_gui/Renderer/GLViewportRenderer.h"),
    QStringLiteral("src/qml_gui/Renderer/GCodeRenderer.cpp"),
    QStringLiteral("src/qml_gui/Renderer/GCodeRenderer.h")
  };

  for (const QString &relativePath : deletedFiles) {
    QFileInfo fileInfo(QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR)).filePath(relativePath));
    QVERIFY2(!fileInfo.exists(),
             qPrintable(QStringLiteral("Retired OpenGL viewport source must stay deleted: %1").arg(relativePath)));
    QVERIFY2(!cmake.contains(relativePath),
             qPrintable(QStringLiteral("Retired OpenGL viewport source must stay out of CMake: %1").arg(relativePath)));
  }

  QVERIFY2(!mainCpp.contains(QStringLiteral("GLViewport.h")),
           "main_qml.cpp must not include the retired OpenGL viewport header");
  QVERIFY2(!mainCpp.contains(QStringLiteral("OWZX_OPENGL")),
           "OWZX_OPENGL must no longer activate a retired OpenGL viewport path");
  QVERIFY2(!mainCpp.contains(QStringLiteral("qmlRegisterType<GLViewport>")),
           "OpenGL GLViewport must not be registered after retirement");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<RhiViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
           "RhiViewport must remain registered under the existing QML type name");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<SoftwareViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
           "SoftwareViewport must remain the non-RHI fallback registration");
}

void QmlUiAuditTests::prepareViewportBindsBedAndPlateContext()
{
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString rhiViewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString rhiViewportSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.cpp"));
  const QString softwareViewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/SoftwareViewport.h"));
  const QString editorHeader = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!rhiViewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!rhiViewportSource.isEmpty(), "Unable to read RhiViewport.cpp");
  QVERIFY2(!softwareViewportHeader.isEmpty(), "Unable to read SoftwareViewport.h");
  QVERIFY2(!editorHeader.isEmpty(), "Unable to read EditorViewModel.h");

  QVERIFY2(editorHeader.contains(QStringLiteral("activePlateObjectIndices")),
           "EditorViewModel must expose renderer-facing activePlateObjectIndices");
  QVERIFY2(editorHeader.contains(QStringLiteral("meshBatchSourceObjectIndices")),
           "EditorViewModel must expose renderer-facing meshBatchSourceObjectIndices");
  QVERIFY2(editorHeader.contains(QStringLiteral("selectedSourceObjectIndex")),
           "EditorViewModel must expose renderer-facing selectedSourceObjectIndex");
  const QStringList requiredBindings = {
    QStringLiteral("bedWidth: root.editorVm ? root.editorVm.bedWidth"),
    QStringLiteral("bedDepth: root.editorVm ? root.editorVm.bedDepth"),
    QStringLiteral("bedOriginX: root.editorVm ? root.editorVm.bedOriginX"),
    QStringLiteral("bedOriginY: root.editorVm ? root.editorVm.bedOriginY"),
    QStringLiteral("bedShapeType: root.editorVm ? root.editorVm.bedShapeType"),
    QStringLiteral("bedDiameter: root.editorVm ? root.editorVm.bedDiameter"),
    QStringLiteral("currentPlateIndex: root.editorVm ? root.editorVm.currentPlateIndex"),
    QStringLiteral("plateCount: root.editorVm ? root.editorVm.plateCount"),
    QStringLiteral("activePlateObjectIndices: root.editorVm ? root.editorVm.activePlateObjectIndices"),
    QStringLiteral("meshBatchSourceObjectIndices: root.editorVm ? root.editorVm.meshBatchSourceObjectIndices"),
    QStringLiteral("selectedSourceObjectIndex: root.editorVm ? root.editorVm.selectedSourceObjectIndex")
  };
  for (const QString &binding : requiredBindings) {
    QVERIFY2(preparePage.contains(binding),
             qPrintable(QStringLiteral("Prepare GLViewport must bind %1").arg(binding)));
  }

  QVERIFY2(rhiViewportHeader.contains(QStringLiteral("Q_PROPERTY(float bedWidth")),
           "RhiViewport must expose bedWidth");
  QVERIFY2(rhiViewportHeader.contains(QStringLiteral("Q_PROPERTY(QVariantList activePlateObjectIndices")),
           "RhiViewport must expose activePlateObjectIndices");
  QVERIFY2(rhiViewportHeader.contains(QStringLiteral("Q_PROPERTY(QVariantList meshBatchSourceObjectIndices")),
           "RhiViewport must expose meshBatchSourceObjectIndices");
  QVERIFY2(rhiViewportHeader.contains(QStringLiteral("Q_PROPERTY(int selectedSourceObjectIndex")),
           "RhiViewport must expose selectedSourceObjectIndex");
  QVERIFY2(softwareViewportHeader.contains(QStringLiteral("Q_PROPERTY(QVariantList activePlateObjectIndices")),
           "SoftwareViewport must expose activePlateObjectIndices for the default QML registration path");
  QVERIFY2(softwareViewportHeader.contains(QStringLiteral("Q_PROPERTY(QVariantList meshBatchSourceObjectIndices")),
           "SoftwareViewport must expose meshBatchSourceObjectIndices for the default QML registration path");
  QVERIFY2(softwareViewportHeader.contains(QStringLiteral("Q_PROPERTY(int selectedSourceObjectIndex")),
           "SoftwareViewport must expose selectedSourceObjectIndex for the default QML registration path");
  QVERIFY2(rhiViewportSource.contains(QStringLiteral("setActivePlateObjectIndices")),
           "RhiViewport must store activePlateObjectIndices through a setter");
  QVERIFY2(rhiViewportSource.contains(QStringLiteral("setMeshBatchSourceObjectIndices")),
           "RhiViewport must store meshBatchSourceObjectIndices through a setter");
  QVERIFY2(rhiViewportSource.contains(QStringLiteral("setSelectedSourceObjectIndex")),
           "RhiViewport must store selectedSourceObjectIndex through a setter");
  QVERIFY2(!preparePage.contains(QStringLiteral("activePlateObjectIndices.filter")),
           "PreparePage must not filter renderer object membership in QML");
  QVERIFY2(!preparePage.contains(QStringLiteral("activePlateObjectIndices.map")),
           "PreparePage must not transform renderer object membership in QML");
  QVERIFY2(!preparePage.contains(QStringLiteral("meshBatchSourceObjectIndices.filter")),
           "PreparePage must not filter renderer batch source metadata in QML");
  QVERIFY2(!preparePage.contains(QStringLiteral("meshBatchSourceObjectIndices.map")),
           "PreparePage must not transform renderer batch source metadata in QML");
}

void QmlUiAuditTests::prepareReadinessControlsBindBackendAvailability()
{
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString sliceProgress = readSource(QStringLiteral("src/qml_gui/panels/SliceProgress.qml"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!sliceProgress.isEmpty(), "Unable to read SliceProgress.qml");

  QVERIFY2(preparePage.contains(QStringLiteral("root.editorVm.canExportGCode")),
           "Prepare export dialog must be guarded by EditorViewModel::canExportGCode");
  QVERIFY2(preparePage.contains(QStringLiteral("root.editorVm.requestExportGCode(\"\"")),
           "Prepare export guard must route blocked attempts through the backend for a reason");
  QVERIFY2(preparePage.contains(QStringLiteral("plateSliceResultStatus(index)")),
           "Prepare plate cards must render explicit valid/stale/missing result status");

  const QStringList backendBindings = {
    QStringLiteral("root.editorVm.canPreview"),
    QStringLiteral("root.editorVm.canExportGCode"),
    QStringLiteral("root.editorVm.previewActionHint"),
    QStringLiteral("root.editorVm.exportActionHint"),
    QStringLiteral("root.editorVm.plateSliceResultStatus(index)")
  };
  for (const QString &binding : backendBindings) {
    QVERIFY2(sliceProgress.contains(binding),
             qPrintable(QStringLiteral("SliceProgress must bind backend readiness property: %1").arg(binding)));
  }

  QVERIFY2(sliceProgress.contains(QStringLiteral("enabled: root.canPreview")),
           "Preview action must be disabled through backend availability");
  QVERIFY2(sliceProgress.contains(QStringLiteral("enabled: root.canExportGCode")),
           "Export action must be disabled through backend availability");
  QVERIFY2(!sliceProgress.contains(QStringLiteral("visible: root.hasSliceResult && !root.slicingNow")),
           "Preview/export action row must not disappear solely because the current result is missing or stale");
}

void QmlUiAuditTests::exportUiUsesSaveDialogAndAvoidsSourcePathTarget()
{
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString sliceProgress = readSource(QStringLiteral("src/qml_gui/panels/SliceProgress.qml"));
  const QString editorHeader = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString mainQml = readSource(QStringLiteral("src/qml_gui/main.qml"));
  const QString topbar = readSource(QStringLiteral("src/qml_gui/BBLTopbar.qml"));
  const QString errorToast = readSource(QStringLiteral("src/qml_gui/components/ErrorToast.qml"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!sliceProgress.isEmpty(), "Unable to read SliceProgress.qml");
  QVERIFY2(!editorHeader.isEmpty(), "Unable to read EditorViewModel.h");
  QVERIFY2(!mainQml.isEmpty(), "Unable to read main.qml");
  QVERIFY2(!topbar.isEmpty(), "Unable to read BBLTopbar.qml");
  QVERIFY2(!errorToast.isEmpty(), "Unable to read ErrorToast.qml");

  const int dialogStart = preparePage.indexOf(QStringLiteral("id: exportGCodeDlg"));
  QVERIFY2(dialogStart >= 0, "PreparePage exportGCodeDlg missing");
  const int nextDialog = preparePage.indexOf(QStringLiteral("FileDialog {"), dialogStart + 1);
  const QString exportDialog = preparePage.mid(dialogStart,
                                               nextDialog > dialogStart ? nextDialog - dialogStart : preparePage.size() - dialogStart);
  QVERIFY2(exportDialog.contains(QStringLiteral("fileMode: FileDialog.SaveFile")),
           "G-code export must use a SaveFile dialog, not an open-file dialog");
  QVERIFY2(exportDialog.contains(QStringLiteral("defaultExportGCodeFileName")),
           "G-code export dialog must use a backend-provided default filename");

  QVERIFY2(editorHeader.contains(QStringLiteral("defaultExportGCodeFileName")),
           "EditorViewModel must expose a default G-code export filename to QML");
  QVERIFY2(sliceProgress.contains(QStringLiteral("exportRequested")),
           "SliceProgress export button must ask the page to open a save dialog");
  QVERIFY2(!sliceProgress.contains(QStringLiteral("requestExportGCode(path)"))
               && !sliceProgress.contains(QStringLiteral("requestExportGCode(root.outputPath)"))
               && !sliceProgress.contains(QStringLiteral("? root.outputPath")),
           "SliceProgress must not export directly to the generated source outputPath");
  QVERIFY2(!errorToast.contains(QStringLiteral("requestExportGCode(\"output.gcode\")")),
           "Notification export button must open the export dialog instead of writing output.gcode");
  QVERIFY2(errorToast.contains(QStringLiteral("exportGCodeRequested")),
           "Notification export button must route through the page export dialog");
  QVERIFY2(topbar.contains(QStringLiteral("exportAllGcodeRequested")),
           "Topbar must expose an all-plate G-code export action");
  QVERIFY2(mainQml.contains(QStringLiteral("FolderDialog"))
               && mainQml.contains(QStringLiteral("requestExportAllGCode")),
           "All-plate export must ask for a destination directory and call the backend all-plate API");
}

void QmlUiAuditTests::rhiViewportRendererUsesPrepareSceneDataAndDirtyUploads()
{
  const QString rendererHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.h"));
  const QString rendererSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  QVERIFY2(!rendererHeader.isEmpty(), "Unable to read RhiViewportRenderer.h");
  QVERIFY2(!rendererSource.isEmpty(), "Unable to read RhiViewportRenderer.cpp");

  QVERIFY2(rendererHeader.contains(QStringLiteral("PrepareSceneData")),
           "RhiViewportRenderer must own PrepareSceneData");
  QVERIFY2(rendererHeader.contains(QStringLiteral("m_bedFillBuffer")),
           "RhiViewportRenderer must own a bed fill vertex buffer");
  QVERIFY2(rendererHeader.contains(QStringLiteral("m_bedLineBuffer")),
           "RhiViewportRenderer must own a bed/grid line vertex buffer");
  QVERIFY2(rendererHeader.contains(QStringLiteral("m_fillPipeline"))
               && rendererHeader.contains(QStringLiteral("m_linePipeline")),
           "RhiViewportRenderer must keep separate fill and line pipelines");
  QVERIFY2(rendererSource.contains(QStringLiteral("QRhiGraphicsPipeline::Triangles")),
           "Prepare bed fill must use a triangle draw path");
  QVERIFY2(rendererSource.contains(QStringLiteral("QRhiGraphicsPipeline::Lines")),
           "Prepare bed grid/origin overlay must use a line draw path");
  QVERIFY2(rendererSource.contains(QStringLiteral("takeDirtyFlags()")),
           "Renderer upload logic must consume PrepareSceneData dirty flags");
  const qsizetype uploadFailureBranch = rendererSource.indexOf(
      QStringLiteral("if (!uploadSceneBuffers(updates, dirtyFlags))"));
  const qsizetype uploadSuccessBranch = rendererSource.indexOf(QStringLiteral("} else {"), uploadFailureBranch);
  const qsizetype dirtyConsume = rendererSource.indexOf(
      QStringLiteral("m_prepareScene.takeDirtyFlags();"), uploadSuccessBranch);
  QVERIFY2(uploadFailureBranch >= 0
               && uploadSuccessBranch > uploadFailureBranch
               && dirtyConsume > uploadSuccessBranch
               && !rendererSource.mid(uploadFailureBranch, uploadSuccessBranch - uploadFailureBranch)
                       .contains(QStringLiteral("takeDirtyFlags();")),
           "Renderer must consume dirty flags only after scene upload scheduling succeeds");
  QVERIFY2(rendererSource.contains(QStringLiteral("DirtyBed"))
               && rendererSource.contains(QStringLiteral("DirtyPlate"))
               && rendererSource.contains(QStringLiteral("DirtyGpu")),
           "Renderer uploads must be gated by PrepareSceneData dirty state");
  QVERIFY2(rendererSource.contains(QStringLiteral("uploadStaticBuffer(m_bedFillBuffer.get()"))
               && rendererSource.contains(QStringLiteral("uploadStaticBuffer(m_bedLineBuffer.get()")),
           "Renderer must upload fill and line buffers explicitly");
  QVERIFY2(!rendererSource.contains(QStringLiteral("kDiagnosticVertices")),
           "Prepare QRhi path must not remain diagnostic-triangle only");
}

void QmlUiAuditTests::rhiViewportRendererUsesModelBuffersAndCameraUniforms()
{
  const QString viewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString viewportSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.cpp"));
  const QString rendererHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.h"));
  const QString rendererSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  const QString vertexLayoutHeader = readSource(QStringLiteral("src/core/rendering/GizmoVertex.h"));
  const QString vertexShader = readSource(QStringLiteral("src/qml_gui/Renderer/shaders/rhi_viewport.vert"));
  QVERIFY2(!viewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!viewportSource.isEmpty(), "Unable to read RhiViewport.cpp");
  QVERIFY2(!rendererHeader.isEmpty(), "Unable to read RhiViewportRenderer.h");
  QVERIFY2(!rendererSource.isEmpty(), "Unable to read RhiViewportRenderer.cpp");
  QVERIFY2(!vertexLayoutHeader.isEmpty(), "Unable to read GizmoVertex.h");
  QVERIFY2(!vertexShader.isEmpty(), "Unable to read rhi_viewport.vert");

  QVERIFY2(rendererHeader.contains(QStringLiteral("using Vertex = GizmoVertex"))
               && vertexLayoutHeader.contains(QStringLiteral("float x;"))
               && vertexLayoutHeader.contains(QStringLiteral("float y;"))
               && vertexLayoutHeader.contains(QStringLiteral("float z;")),
           "RhiViewportRenderer vertex format must carry 3D positions");
  QVERIFY2(rendererHeader.contains(QStringLiteral("m_modelVertexBuffer")),
           "RhiViewportRenderer must own a persistent model vertex buffer");
  QVERIFY2(rendererHeader.contains(QStringLiteral("m_modelVertexBufferBytes"))
               && rendererHeader.contains(QStringLiteral("m_modelVertexCount")),
           "RhiViewportRenderer must track model buffer size and draw count");
  QVERIFY2(rendererHeader.contains(QStringLiteral("m_cameraUniformBuffer")),
           "RhiViewportRenderer must own a camera MVP uniform buffer");
  QVERIFY2(rendererHeader.contains(QStringLiteral("m_cameraUniformBufferUploaded")),
           "RhiViewportRenderer must track camera uniform upload state separately");
  QVERIFY2(rendererHeader.contains(QStringLiteral("QMatrix4x4 m_cameraMvp")),
           "RhiViewportRenderer must receive a render-ready camera MVP matrix");
  QVERIFY2(rendererSource.contains(QStringLiteral("setModelMeshData(")),
           "RhiViewportRenderer synchronization must feed mesh data into PrepareSceneData");
  QVERIFY2(rendererSource.contains(QStringLiteral("uploadModelBuffer")),
           "RhiViewportRenderer must isolate model buffer upload logic");
  QVERIFY2(rendererSource.contains(QStringLiteral("uploadCameraUniform")),
           "RhiViewportRenderer must isolate camera uniform upload logic");
  QVERIFY2(rendererSource.contains(QStringLiteral("m_modelVertexBuffer.get()")),
           "RhiViewportRenderer must bind and draw the model vertex buffer");
  QVERIFY2(rendererSource.contains(QStringLiteral("PrepareSceneData::DirtyMesh"))
               && rendererSource.contains(QStringLiteral("PrepareSceneData::DirtyCamera")),
           "Renderer dirty logic must distinguish model uploads from camera uniform updates");
  QVERIFY2(!rendererSource.contains(QStringLiteral("DirtyCamera | PrepareSceneData::DirtyMesh")),
           "Camera dirty state must not be ORed into full model mesh upload conditions");

  QVERIFY2(viewportHeader.contains(QStringLiteral("#include \"CameraController.h\"")),
           "RhiViewport must use the shared CameraController");
  QVERIFY2(viewportHeader.contains(QStringLiteral("CameraController m_camera")),
           "RhiViewport must own CameraController state on the GUI item side");
  QVERIFY2(viewportHeader.contains(QStringLiteral("mousePressEvent(QMouseEvent *event) override"))
               && viewportHeader.contains(QStringLiteral("wheelEvent(QWheelEvent *event) override")),
           "RhiViewport must override mouse and wheel events for camera interaction");
  QVERIFY2(viewportSource.contains(QStringLiteral("m_camera.orbit"))
               && viewportSource.contains(QStringLiteral("m_camera.pan"))
               && viewportSource.contains(QStringLiteral("m_camera.zoom")),
           "RhiViewport input handling must route orbit, pan, and zoom through CameraController");
  QVERIFY2(viewportSource.contains(QStringLiteral("m_camera.fitView"))
               && viewportSource.contains(QStringLiteral("m_camera.viewIso")),
           "RhiViewport must route fit and preset changes through CameraController");
  const int bedSetterStart = viewportSource.indexOf(QStringLiteral("void RhiViewport::setBedWidth"));
  const int plateSetterStart = viewportSource.indexOf(QStringLiteral("void RhiViewport::setCurrentPlateIndex"));
  QVERIFY2(bedSetterStart >= 0 && plateSetterStart > bedSetterStart,
           "RhiViewport bed/plate setter order changed; update model-generation audit");
  QVERIFY2(!viewportSource.mid(bedSetterStart, plateSetterStart - bedSetterStart).contains(QStringLiteral("m_modelGeneration")),
           "Bed-only changes must not increment modelGeneration or reupload model buffers");
  QVERIFY2(viewportSource.contains(QStringLiteral("++m_modelGeneration")),
           "Mesh or active-plate changes must explicitly increment modelGeneration");

  QVERIFY2(vertexShader.contains(QStringLiteral("layout(location = 0) in vec3 position")),
           "RhiViewport vertex shader must consume 3D positions");
  QVERIFY2(vertexShader.contains(QStringLiteral("layout(std140, binding = 0) uniform CameraBlock"))
               && vertexShader.contains(QStringLiteral("mvp")),
           "RhiViewport vertex shader must use a camera MVP uniform");
}

void QmlUiAuditTests::importEntryPointsAdvertiseConsistentModelFormats()
{
  const QString mainQml = readSource(QStringLiteral("src/qml_gui/main.qml"));
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString projectPage = readSource(QStringLiteral("src/qml_gui/pages/ProjectPage.qml"));
  QVERIFY2(!mainQml.isEmpty(), "Unable to read main.qml");
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!projectPage.isEmpty(), "Unable to read ProjectPage.qml");

  const QStringList requiredExtensions = {
    QStringLiteral("*.3mf"),
    QStringLiteral("*.stl"),
    QStringLiteral("*.obj"),
    QStringLiteral("*.amf"),
    QStringLiteral("*.step"),
    QStringLiteral("*.stp")
  };

  for (const QString &ext : requiredExtensions) {
    QVERIFY2(mainQml.contains(ext),
             qPrintable(QStringLiteral("Topbar import dialog must advertise %1").arg(ext)));
    QVERIFY2(preparePage.contains(ext),
             qPrintable(QStringLiteral("Prepare import dialog must advertise %1").arg(ext)));
    QVERIFY2(projectPage.contains(ext),
             qPrintable(QStringLiteral("Project import dialog must advertise %1").arg(ext)));
  }

  QVERIFY2(mainQml.contains(QStringLiteral("backend.topbarImportModel(selectedFile.toString())")),
           "Topbar import dialog must route through BackendContext::topbarImportModel");
  QVERIFY2(preparePage.contains(QStringLiteral("root.editorVm.loadFile(selectedFile.toString())")),
           "Prepare import dialog must route through EditorViewModel::loadFile");
  QVERIFY2(projectPage.contains(QStringLiteral("root.editorVm.loadFile(currentFile.toString().replace(\"file:///\", \"\"))")),
           "Project import dialog must route through EditorViewModel::loadFile");
}

void QmlUiAuditTests::previewRhiRendererBindsPreviewStateAndUsesExactDrawSpans()
{
  const QString previewPage = readSource(QStringLiteral("src/qml_gui/pages/PreviewPage.qml"));
  const QString mainCpp = readSource(QStringLiteral("src/qml_gui/main_qml.cpp"));
  const QString rendererHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.h"));
  const QString rendererSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  QVERIFY2(!previewPage.isEmpty(), "Unable to read PreviewPage.qml");
  QVERIFY2(!mainCpp.isEmpty(), "Unable to read main_qml.cpp");
  QVERIFY2(!rendererHeader.isEmpty(), "Unable to read RhiViewportRenderer.h");
  QVERIFY2(!rendererSource.isEmpty(), "Unable to read RhiViewportRenderer.cpp");

  const QStringList requiredBindings = {
    QStringLiteral("canvasType: GLViewport.CanvasPreview"),
    QStringLiteral("previewData: root.previewVm.gcodePreviewData"),
    QStringLiteral("layerMin: root.previewVm.currentLayerMin"),
    QStringLiteral("layerMax: root.previewVm.currentLayerMax"),
    QStringLiteral("moveEnd: root.previewVm.currentMove"),
    QStringLiteral("showTravelMoves: root.previewVm.showTravelMoves"),
    QStringLiteral("gcodeViewMode: root.previewVm.viewModeIndex")
  };
  for (const QString &binding : requiredBindings) {
    QVERIFY2(previewPage.contains(binding),
             qPrintable(QStringLiteral("Preview GLViewport must bind %1").arg(binding)));
  }

  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<RhiViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
           "Preview normal path must keep RhiViewport registered as GLViewport");
  QVERIFY2(!previewPage.contains(QStringLiteral("SoftwareViewport")),
           "PreviewPage must not instantiate SoftwareViewport directly as the normal path");

  QVERIFY2(rendererHeader.contains(QStringLiteral("struct PreviewDrawSpan")),
           "RhiViewportRenderer must keep explicit per-segment preview draw spans");
  QVERIFY2(rendererHeader.contains(QStringLiteral("int move;")),
           "Preview draw spans must store the packed segment move index");
  QVERIFY2(rendererSource.contains(QStringLiteral("m_previewDrawSpans.append({seg[i].layer, seg[i].move")),
           "Preview parse must index draw spans by packed layer and move");
  QVERIFY2(rendererSource.contains(QStringLiteral("if (m_moveEnd <= 0)")),
           "Preview move playback must draw zero segments at moveEnd <= 0");
  QVERIFY2(rendererSource.contains(QStringLiteral("span.move >= m_moveEnd")),
           "Preview draw range must clamp by exact packed segment move index");
  QVERIFY2(!rendererSource.contains(QStringLiteral("const bool isTravel = (seg[i].move == 0)")),
           "Renderer must not treat move index zero as a travel marker");
  QVERIFY2(!rendererSource.contains(QStringLiteral("quint64(firstVertex + vertexCount) * quint64(m_moveEnd) / quint64(m_previewSegmentVertexCount)")),
           "Renderer must not approximate Preview playback by proportional vertex-count clipping");
}

void QmlUiAuditTests::previewRhiViewportFitsCameraToPreviewDataBeforeOrbit()
{
  const QString viewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString viewportSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.cpp"));
  const QString previewPage = readSource(QStringLiteral("src/qml_gui/pages/PreviewPage.qml"));
  QVERIFY2(!viewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!viewportSource.isEmpty(), "Unable to read RhiViewport.cpp");
  QVERIFY2(!previewPage.isEmpty(), "Unable to read PreviewPage.qml");

  QVERIFY2(previewPage.contains(QStringLiteral("canvasType: GLViewport.CanvasPreview")),
           "PreviewPage must route G-code preview through the GLViewport-compatible RhiViewport path");
  QVERIFY2(viewportHeader.contains(QStringLiteral("fitPreviewCameraToData")),
           "RhiViewport must keep a Preview-data driven camera fit hook before user orbit");
  QVERIFY2(viewportHeader.contains(QStringLiteral("m_previewCameraFitted")),
           "RhiViewport must remember whether Preview camera has already been fitted");
  QVERIFY2(viewportHeader.contains(QStringLiteral("m_previewFitHint")),
           "RhiViewport must cache the Preview fit bounds to avoid resetting orbit on recolor-only updates");
  QVERIFY2(viewportSource.contains(QStringLiteral("fitPreviewCameraToData();")),
           "setPreviewData/setCanvasType must invoke Preview fit when G-code preview data enters the viewport");
  QVERIFY2(viewportSource.contains(QStringLiteral("std::memcmp(m_previewData.constData(), \"GCV1\", 4)")),
           "Preview camera fit must parse the same GCV1 packed data consumed by the RHI preview renderer");
  QVERIFY2(viewportSource.contains(QStringLiteral("includePoint(seg[i].x1, seg[i].z1, seg[i].y1)"))
               && viewportSource.contains(QStringLiteral("includePoint(seg[i].x2, seg[i].z2, seg[i].y2)")),
           "Preview camera fit must use the renderer axis mapping: G-code Z is viewport Y and G-code Y is viewport Z");
  QVERIFY2(viewportSource.contains(QStringLiteral("m_camera.fitView(cx, cy, cz, radius)")),
           "Preview camera fit must update CameraController target/distance before orbit");
  QVERIFY2(viewportSource.contains(QStringLiteral("m_cameraDirty = true")),
           "Preview camera fit must mark the camera uniform dirty for the next RHI sync");
}

void QmlUiAuditTests::previewRhiRendererResetsGpuStateAfterResourceRelease()
{
  const QString rendererHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.h"));
  const QString rendererSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  QVERIFY2(!rendererHeader.isEmpty(), "Unable to read RhiViewportRenderer.h");
  QVERIFY2(!rendererSource.isEmpty(), "Unable to read RhiViewportRenderer.cpp");

  QVERIFY2(rendererHeader.contains(QStringLiteral("resetPreviewGpuState")),
           "RhiViewportRenderer must centralize Preview GPU state reset for QRhi resource rebuilds");
  QVERIFY2(rendererSource.contains(QStringLiteral("resetPreviewGpuState(true)")),
           "releaseResources/initialize must reset Preview GPU upload state while keeping CPU staging data");
  QVERIFY2(rendererSource.contains(QStringLiteral("resetPreviewGpuState(false)")),
           "Preview payload changes must clear stale GPU upload state and CPU staging together");

  const int helperStart = rendererSource.indexOf(QStringLiteral("void RhiViewportRenderer::resetPreviewGpuState"));
  QVERIFY2(helperStart >= 0, "resetPreviewGpuState implementation missing");
  const int nextFunction = rendererSource.indexOf(QStringLiteral("\nbool RhiViewportRenderer::ensurePipelines"), helperStart);
  QVERIFY2(nextFunction > helperStart,
           "resetPreviewGpuState must live before ensurePipelines; update audit boundaries if moved");
  const QString helper = rendererSource.mid(helperStart, nextFunction - helperStart);
  const QStringList requiredResets = {
    QStringLiteral("m_previewSegmentBuffer.reset()"),
    QStringLiteral("m_previewSegmentBufferBytes = 0"),
    QStringLiteral("m_previewSegmentBufferUploaded = false"),
    QStringLiteral("m_previewLastUploadMs = -1"),
    QStringLiteral("m_previewLastFrameMs = -1"),
    QStringLiteral("m_previewFirstFrameMs = -1"),
    QStringLiteral("m_previewFirstFrameDone = false")
  };
  for (const QString &reset : requiredResets) {
    QVERIFY2(helper.contains(reset),
             qPrintable(QStringLiteral("Preview GPU reset helper must include: %1").arg(reset)));
  }
  QVERIFY2(helper.contains(QStringLiteral("if (!keepCpuStaging)"))
               && helper.contains(QStringLiteral("m_previewVertices.clear()"))
               && helper.contains(QStringLiteral("m_previewDrawSpans.clear()"))
               && helper.contains(QStringLiteral("m_previewSegmentVertexCount = 0")),
           "Preview GPU reset helper must optionally clear CPU staging and vertex count for payload changes");

  const int rangeStart = rendererSource.indexOf(QStringLiteral("QVector<RhiViewportRenderer::PreviewDrawRange> RhiViewportRenderer::computePreviewDrawRanges"));
  QVERIFY2(rangeStart >= 0, "computePreviewDrawRanges implementation missing");
  const QString drawRange = rendererSource.mid(rangeStart);
  QVERIFY2(drawRange.contains(QStringLiteral("const int layerLow = std::min(m_layerMin, m_layerMax)"))
               && drawRange.contains(QStringLiteral("const int layerHigh = std::max(m_layerMin, m_layerMax)")),
           "Preview draw range must normalize transient inverted layer bounds instead of blanking valid data");
}

void QmlUiAuditTests::previewRhiInteractionSettersPreservePreviewPayload()
{
  const QString viewportSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.cpp"));
  QVERIFY2(!viewportSource.isEmpty(), "Unable to read RhiViewport.cpp");

  const QStringList setters = {
    QStringLiteral("void RhiViewport::setLayerMin"),
    QStringLiteral("void RhiViewport::setLayerMax"),
    QStringLiteral("void RhiViewport::setMoveEnd"),
    QStringLiteral("void RhiViewport::setShowTravelMoves"),
    QStringLiteral("void RhiViewport::setShowBed"),
    QStringLiteral("void RhiViewport::setShowMarker"),
    QStringLiteral("void RhiViewport::setGcodeViewMode")
  };
  for (int i = 0; i < setters.size(); ++i) {
    const int start = viewportSource.indexOf(setters.at(i));
    QVERIFY2(start >= 0,
             qPrintable(QStringLiteral("Missing Preview interaction setter: %1").arg(setters.at(i))));
    int end = viewportSource.size();
    for (int j = 0; j < setters.size(); ++j) {
      if (i == j)
        continue;
      const int candidate = viewportSource.indexOf(setters.at(j), start + 1);
      if (candidate > start)
        end = std::min(end, candidate);
    }
    const QString body = viewportSource.mid(start, end - start);
    QVERIFY2(body.contains(QStringLiteral("update();")),
             qPrintable(QStringLiteral("%1 must schedule a redraw").arg(setters.at(i))));
    QVERIFY2(!body.contains(QStringLiteral("m_previewData =")),
             qPrintable(QStringLiteral("%1 must not mutate the Preview payload during interaction").arg(setters.at(i))));
    QVERIFY2(!body.contains(QStringLiteral("fitPreviewCameraToData();")),
             qPrintable(QStringLiteral("%1 must not refit camera or rebuild payload on pure interaction").arg(setters.at(i))));
  }
}

void QmlUiAuditTests::previewNormalPathCoversFullWorkflowBindingsAndDiagnostics()
{
  const QString previewPage = readSource(QStringLiteral("src/qml_gui/pages/PreviewPage.qml"));
  const QString mainCpp = readSource(QStringLiteral("src/qml_gui/main_qml.cpp"));
  const QString selectorSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiBackendSelector.cpp"));
  const QString viewportSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.cpp"));
  const QString rendererSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  const QString projectSource = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  const QString sliceSource = readSource(QStringLiteral("src/core/services/SliceService.cpp"));
  const QString previewVmSource = readSource(QStringLiteral("src/core/viewmodels/PreviewViewModel.cpp"));
  QVERIFY2(!previewPage.isEmpty(), "Unable to read PreviewPage.qml");
  QVERIFY2(!mainCpp.isEmpty(), "Unable to read main_qml.cpp");
  QVERIFY2(!selectorSource.isEmpty(), "Unable to read RhiBackendSelector.cpp");
  QVERIFY2(!viewportSource.isEmpty(), "Unable to read RhiViewport.cpp");
  QVERIFY2(!rendererSource.isEmpty(), "Unable to read RhiViewportRenderer.cpp");
  QVERIFY2(!projectSource.isEmpty(), "Unable to read ProjectServiceMock.cpp");
  QVERIFY2(!sliceSource.isEmpty(), "Unable to read SliceService.cpp");
  QVERIFY2(!previewVmSource.isEmpty(), "Unable to read PreviewViewModel.cpp");

  QVERIFY2(previewPage.contains(QStringLiteral("GLViewport {")),
           "PreviewPage must use the registered GLViewport type for the normal path");
  QVERIFY2(!previewPage.contains(QStringLiteral("SoftwareViewport")),
           "PreviewPage normal path must never instantiate SoftwareViewport directly");

  const QStringList workflowBindings = {
    QStringLiteral("canvasType: GLViewport.CanvasPreview"),
    QStringLiteral("previewData: root.previewVm.gcodePreviewData"),
    QStringLiteral("layerMin: root.previewVm.currentLayerMin"),
    QStringLiteral("layerMax: root.previewVm.currentLayerMax"),
    QStringLiteral("moveEnd: root.previewVm.currentMove"),
    QStringLiteral("showTravelMoves: root.previewVm.showTravelMoves"),
    QStringLiteral("showBed: root.previewVm.showBed"),
    QStringLiteral("showMarker: root.previewVm.showMarker"),
    QStringLiteral("gcodeViewMode: root.previewVm.viewModeIndex"),
    QStringLiteral("markerX: root.previewVm.toolX"),
    QStringLiteral("markerY: root.previewVm.toolY"),
    QStringLiteral("markerZ: root.previewVm.toolZ")
  };
  for (const QString &binding : workflowBindings) {
    QVERIFY2(previewPage.contains(binding),
             qPrintable(QStringLiteral("Preview normal path must keep binding: %1").arg(binding)));
  }

  QVERIFY2(mainCpp.contains(QStringLiteral("qputenv(\"OWZX_RHI_RENDERER\", \"auto\")")),
           "Default startup must select the QRhi auto policy");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<RhiViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
           "QRhi startup path must register RhiViewport as the GLViewport QML type");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<SoftwareViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
           "SoftwareViewport must stay fallback-only behind failed QRhi initialization");
  const int defaultCandidatesStart = selectorSource.indexOf(QStringLiteral("QVector<RhiBackendCandidate> defaultWindowsCandidates()"));
  const int candidatesForRequestStart = selectorSource.indexOf(QStringLiteral("QVector<RhiBackendCandidate> candidatesForRequest"));
  QVERIFY2(defaultCandidatesStart >= 0 && candidatesForRequestStart > defaultCandidatesStart,
           "RhiBackendSelector default candidate boundaries changed; update Phase 43 audit");
  const QString defaultCandidates = selectorSource.mid(defaultCandidatesStart,
                                                       candidatesForRequestStart - defaultCandidatesStart);
  QVERIFY2(defaultCandidates.indexOf(QStringLiteral("Direct3D11"))
               < defaultCandidates.indexOf(QStringLiteral("Direct3D12")),
           "Normal Windows Preview path must keep D3D11 before D3D12");
  QVERIFY2(!defaultCandidates.contains(QStringLiteral("Vulkan")),
           "Vulkan must not become the normal Preview path during v3.4 closeout");

  const QStringList interactionSetters = {
    QStringLiteral("void RhiViewport::setLayerMin"),
    QStringLiteral("void RhiViewport::setLayerMax"),
    QStringLiteral("void RhiViewport::setMoveEnd")
  };
  for (const QString &setter : interactionSetters) {
    const int start = viewportSource.indexOf(setter);
    QVERIFY2(start >= 0, qPrintable(QStringLiteral("Missing interaction setter: %1").arg(setter)));
    const int end = viewportSource.indexOf(QStringLiteral("\nvoid RhiViewport::"), start + 1);
    const QString body = viewportSource.mid(start, end > start ? end - start : viewportSource.size() - start);
    QVERIFY2(body.contains(QStringLiteral("update();")),
             qPrintable(QStringLiteral("%1 must schedule a redraw").arg(setter)));
    QVERIFY2(!body.contains(QStringLiteral("m_previewData =")),
             qPrintable(QStringLiteral("%1 must not clear Preview payload").arg(setter)));
  }

  QVERIFY2(rendererSource.contains(QStringLiteral("qInfo(\"[RHI] preview payload"))
               && rendererSource.contains(QStringLiteral("qInfo(\"[RHI] preview range")),
           "RhiViewportRenderer must log Preview payload and draw-range diagnostics");
  QVERIFY2(projectSource.contains(QStringLiteral("[ProjectService] import"))
               && projectSource.contains(QStringLiteral("sourceFilePath_ = localPath")),
           "ProjectServiceMock must log import path/status and preserve source path diagnostics");
  QVERIFY2(sliceSource.contains(QStringLiteral("[SliceService] slice"))
               && sliceSource.contains(QStringLiteral("[SliceService] slice failed"))
               && sliceSource.contains(QStringLiteral("[SliceService] slice cancelled"))
               && sliceSource.contains(QStringLiteral("[SliceService] export")),
           "SliceService must log slice/export success, failure, and cancellation diagnostics");
  QVERIFY2(sliceSource.contains(QStringLiteral("logExportFailure")),
           "SliceService export failures must pass through a deterministic diagnostic helper");
  QVERIFY2(previewVmSource.contains(QStringLiteral("[PreviewViewModel] payload")),
           "PreviewViewModel must log payload size/range diagnostics after parsing");
}

void QmlUiAuditTests::previewLayoutRestoresScreenshotRegionsAndGcodePanel()
{
  const QString previewPage = readSource(QStringLiteral("src/qml_gui/pages/PreviewPage.qml"));
  const QString previewHeader = readSource(QStringLiteral("src/core/viewmodels/PreviewViewModel.h"));
  const QString statsPanel = readSource(QStringLiteral("src/qml_gui/components/StatsPanel.qml"));
  const QString visibilityFilter = readSource(QStringLiteral("src/qml_gui/components/VisibilityFilter.qml"));
  const QString legend = readSource(QStringLiteral("src/qml_gui/components/Legend.qml"));
  const QString moveSlider = readSource(QStringLiteral("src/qml_gui/components/MoveSlider.qml"));
  QVERIFY2(!previewPage.isEmpty(), "Unable to read PreviewPage.qml");
  QVERIFY2(!previewHeader.isEmpty(), "Unable to read PreviewViewModel.h");
  QVERIFY2(!statsPanel.isEmpty(), "Unable to read StatsPanel.qml");
  QVERIFY2(!visibilityFilter.isEmpty(), "Unable to read VisibilityFilter.qml");
  QVERIFY2(!legend.isEmpty(), "Unable to read Legend.qml");
  QVERIFY2(!moveSlider.isEmpty(), "Unable to read MoveSlider.qml");

  const QStringList requiredRegions = {
    QStringLiteral("id: previewHeader"),
    QStringLiteral("id: leftPanel"),
    QStringLiteral("id: centerArea"),
    QStringLiteral("id: rightPanel"),
    QStringLiteral("id: verticalLayerRail"),
    QStringLiteral("id: moveSliderBar"),
    QStringLiteral("LeftSidebar {"),
    QStringLiteral("Components.MoveSlider"),
    QStringLiteral("Components.StatsPanel"),
    QStringLiteral("Components.VisibilityFilter"),
    QStringLiteral("Components.Legend"),
    QStringLiteral("Components.ToolPositionTooltip"),
    QStringLiteral("id: gcodeList")
  };
  for (const QString &region : requiredRegions) {
    QVERIFY2(previewPage.contains(region),
             qPrintable(QStringLiteral("Phase 54 Preview layout missing region: %1").arg(region)));
  }

  const QStringList requiredStateBindings = {
    QStringLiteral("root.previewVm.currentLayerLabel"),
    QStringLiteral("root.previewVm.currentMoveLabel"),
    QStringLiteral("editorVm: root.editorVm"),
    QStringLiteral("configVm: root.configVm"),
    QStringLiteral("processCategory: root.processCategory"),
    QStringLiteral("root.previewVm.gcodeLines"),
    QStringLiteral("root.previewVm.currentGcodeLine"),
    QStringLiteral("root.leftPanelWidth"),
    QStringLiteral("root.rightPanelExpanded")
  };
  for (const QString &binding : requiredStateBindings) {
    QVERIFY2(previewPage.contains(binding),
             qPrintable(QStringLiteral("Phase 54 Preview layout missing binding: %1").arg(binding)));
  }

  QVERIFY2(!previewPage.contains(QStringLiteral("SoftwareViewport")),
           "PreviewPage must keep SoftwareViewport out of the normal layout path");
  QVERIFY2(!previewPage.contains(QStringLiteral("QFile"))
               && !previewPage.contains(QStringLiteral("FileReader"))
               && !previewPage.contains(QStringLiteral("XMLHttpRequest")),
           "PreviewPage must not read or parse G-code files in QML");
  QVERIFY2(previewHeader.contains(QStringLiteral("Q_PROPERTY(QVariantList gcodeLines READ gcodeLines"))
               && previewHeader.contains(QStringLiteral("Q_PROPERTY(QString currentLayerLabel READ currentLayerLabel"))
               && previewHeader.contains(QStringLiteral("Q_PROPERTY(QString plateSummary READ plateSummary")),
           "PreviewViewModel must own the Phase 54 Preview summary and G-code text data");
  QVERIFY2(statsPanel.contains(QStringLiteral("root.previewVm.setShowTravelMoves(checked)"))
               && statsPanel.contains(QStringLiteral("root.previewVm.setShowBed(checked)"))
               && statsPanel.contains(QStringLiteral("root.previewVm.setShowMarker(checked)")),
           "StatsPanel visibility toggles must route through PreviewViewModel invokable setters");
  QVERIFY2(moveSlider.contains(QStringLiteral("root.previewVm.setCurrentMove(Math.round(value))")),
           "MoveSlider must update the move range through PreviewViewModel");

  const QStringList phase80Sources = { previewPage, statsPanel, visibilityFilter, legend, moveSlider };
  const QStringList mojibakeTokens = {
    QString(QChar(0x68f0)),
    QString(QChar(0x9352)),
    QString(QChar(0x93c4)),
    QString(QChar(0x93c3)),
    QString(QChar(0x7ec9)),
    QString(QChar(0x705e)),
    QString(QChar(0x741b)),
    QString(QChar(0x9365)),
    QString(QChar(0x93c6)),
    QString(QChar(0x9470)),
    QString(QChar(0x93ac)),
    QString(QChar(0x93b8)),
    QString(QChar(0x95c8)),
    QString(QChar(0x7ecc)),
    QString(QChar(0x7039))
  };
  for (const QString &source : phase80Sources) {
    for (const QString &token : mojibakeTokens) {
      QVERIFY2(!source.contains(token),
               qPrintable(QStringLiteral("Phase 80 Preview QML must not expose mojibake token: %1").arg(token)));
    }
  }

  const QStringList requiredPhase80Tokens = {
    QStringLiteral("readonly property int targetPreviewLeftWidth: 392"),
    QStringLiteral("readonly property int targetPreviewRightWidth: 300"),
    QStringLiteral("id: rightAnalysisStack"),
    QStringLiteral("id: gcodeSourcePanel"),
    QStringLiteral("root.previewVm.roleVisibilities"),
    QStringLiteral("root.previewVm.legendItems"),
    QStringLiteral("root.previewVm.gcodeLines")
  };
  for (const QString &token : requiredPhase80Tokens) {
    QVERIFY2(previewPage.contains(token) || statsPanel.contains(token)
                 || visibilityFilter.contains(token) || legend.contains(token),
             qPrintable(QStringLiteral("Phase 80 Preview restoration token missing: %1").arg(token)));
  }
}

void QmlUiAuditTests::previewStatsPanelCallsOnlyQmlInvokableSetters()
{
  const QString statsPanel = readSource(QStringLiteral("src/qml_gui/components/StatsPanel.qml"));
  const QString previewHeader = readSource(QStringLiteral("src/core/viewmodels/PreviewViewModel.h"));
  QVERIFY2(!statsPanel.isEmpty(), "Unable to read StatsPanel.qml");
  QVERIFY2(!previewHeader.isEmpty(), "Unable to read PreviewViewModel.h");

  const QStringList requiredSetters = {
    QStringLiteral("setStealthMode"),
    QStringLiteral("setShowTravelMoves"),
    QStringLiteral("setShowBed"),
    QStringLiteral("setShowMarker")
  };
  for (const QString &setter : requiredSetters) {
    QVERIFY2(statsPanel.contains(QStringLiteral("root.previewVm.%1(checked)").arg(setter)),
             qPrintable(QStringLiteral("StatsPanel must still route %1 through PreviewViewModel").arg(setter)));
    const QRegularExpression invokablePattern(
        QStringLiteral("Q_INVOKABLE\\s+void\\s+%1\\s*\\(\\s*bool\\s+enabled\\s*\\)").arg(setter));
    QVERIFY2(invokablePattern.match(previewHeader).hasMatch(),
             qPrintable(QStringLiteral("%1 must be Q_INVOKABLE because StatsPanel calls it as a QML method").arg(setter)));
  }
}

void QmlUiAuditTests::rhiViewportSelectionPickingBridgeStaysCppOwned()
{
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString editorHeader = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString viewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString viewportSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.cpp"));
  const QString rendererHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.h"));
  const QString rendererSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  const QString objectPickingHeader = readSource(QStringLiteral("src/core/rendering/ObjectPicking.h"));
  const QString objectPickingSource = readSource(QStringLiteral("src/core/rendering/ObjectPicking.cpp"));
  const QString softwareViewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/SoftwareViewport.h"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!editorHeader.isEmpty(), "Unable to read EditorViewModel.h");
  QVERIFY2(!viewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!viewportSource.isEmpty(), "Unable to read RhiViewport.cpp");
  QVERIFY2(!rendererHeader.isEmpty(), "Unable to read RhiViewportRenderer.h");
  QVERIFY2(!rendererSource.isEmpty(), "Unable to read RhiViewportRenderer.cpp");
  QVERIFY2(!objectPickingHeader.isEmpty(), "Unable to read ObjectPicking.h");
  QVERIFY2(!objectPickingSource.isEmpty(), "Unable to read ObjectPicking.cpp");
  QVERIFY2(!softwareViewportHeader.isEmpty(), "Unable to read SoftwareViewport.h");

  QVERIFY2(editorHeader.contains(QStringLiteral("selectSourceObject(int sourceIndex)")),
           "EditorViewModel must expose a source-index selection bridge for renderer picking");
  QVERIFY2(preparePage.contains(QStringLiteral("onObjectPickedSource: function(sourceIndex)"))
               && preparePage.contains(QStringLiteral("root.editorVm.selectSourceObject(sourceIndex)")),
           "PreparePage must only forward QRhi source-object picks to EditorViewModel");
  QVERIFY2(!preparePage.contains(QStringLiteral("pickSourceObjectAt"))
               && !preparePage.contains(QStringLiteral("intersect"))
               && !preparePage.contains(QStringLiteral("ray")),
           "PreparePage must not own picking or geometry-hit logic");

  QVERIFY2(viewportHeader.contains(QStringLiteral("Q_PROPERTY(int hoveredSourceObjectIndex")),
           "RhiViewport must expose hover state independently from selection");
  QVERIFY2(viewportHeader.contains(QStringLiteral("void objectPickedSource(int sourceIndex);")),
           "RhiViewport must emit source-object pick signals");
  QVERIFY2(softwareViewportHeader.contains(QStringLiteral("Q_PROPERTY(int hoveredSourceObjectIndex"))
               && softwareViewportHeader.contains(QStringLiteral("void objectPickedSource(int sourceIndex);")),
           "SoftwareViewport fallback must keep QML signal/property compatibility");

  const int pickStart = viewportSource.indexOf(QStringLiteral("int RhiViewport::pickSourceObjectAt"));
  const int nextFunction = viewportSource.indexOf(QStringLiteral("\nQRectF RhiViewport::projectBoundsToScreenRect"), pickStart);
  const int fallbackEnd = viewportSource.indexOf(QStringLiteral("\n// ==========================================================================="), pickStart);
  const int pickEnd = nextFunction > pickStart ? nextFunction : fallbackEnd;
  QVERIFY2(pickStart >= 0 && pickEnd > pickStart,
           "RhiViewport pickSourceObjectAt body must stay discoverable for the precise-picking audit");
  const QString pickBody = viewportSource.mid(pickStart, pickEnd - pickStart);

  QVERIFY2(objectPickingHeader.contains(QStringLiteral("class ObjectPicking"))
               && objectPickingHeader.contains(QStringLiteral("pickSourceObject")),
           "ObjectPicking must expose a pure CPU source-object picking helper");
  QVERIFY2(objectPickingSource.contains(QStringLiteral("rayAABB"))
               && objectPickingSource.contains(QStringLiteral("Moller"))
               && objectPickingSource.contains(QStringLiteral("rayTriangle")),
           "ObjectPicking must own ray-AABB and Moller ray-triangle picking logic");
  QVERIFY2(!objectPickingHeader.contains(QStringLiteral("QRhi"))
               && !objectPickingHeader.contains(QStringLiteral("QQuick"))
               && !objectPickingSource.contains(QStringLiteral("QRhi"))
               && !objectPickingSource.contains(QStringLiteral("QQuick"))
               && !objectPickingSource.contains(QStringLiteral("QOpenGL"))
               && !objectPickingSource.contains(QStringLiteral("glBind"))
               && !objectPickingSource.contains(QStringLiteral("glDraw"))
               && !objectPickingSource.contains(QStringLiteral("GL_")),
           "ObjectPicking must stay free of renderer, QML item, and OpenGL dependencies");

  QVERIFY2(viewportSource.contains(QStringLiteral("#include \"core/rendering/ObjectPicking.h\""))
               && pickBody.contains(QStringLiteral("GizmoMath::computeRay"))
               && pickBody.contains(QStringLiteral("ObjectPicking::pickSourceObject"))
               && pickBody.contains(QStringLiteral("m_pickScene.modelVertices()"))
               && pickBody.contains(QStringLiteral("m_pickScene.modelBatches()"))
               && viewportSource.contains(QStringLiteral("emit objectPickedSource")),
           "RhiViewport picking must stay in C++ and delegate precise mesh hits to ObjectPicking");
  QVERIFY2(!pickBody.contains(QStringLiteral("projectBoundsToScreenRect(")),
           "RhiViewport precise picking must not select through projected AABB screen rectangles");
  QVERIFY2(viewportSource.contains(QStringLiteral("setHoveredSourceObjectIndex"))
               && viewportSource.contains(QStringLiteral("hoverMoveEvent(QHoverEvent *event)")),
           "RhiViewport must maintain hover state from C++ pointer motion");

  QVERIFY2(rendererHeader.contains(QStringLiteral("m_highlightVertexBuffer"))
               && rendererHeader.contains(QStringLiteral("m_highlightVertexBufferUploaded")),
           "RhiViewportRenderer must own a separate selection/hover highlight buffer");
  QVERIFY2(rendererSource.contains(QStringLiteral("buildHighlightVertices"))
               && rendererSource.contains(QStringLiteral("uploadHighlightBuffer"))
               && rendererSource.contains(QStringLiteral("setHoveredSourceObjectIndex")),
           "RhiViewportRenderer must update visual feedback through highlight uploads");
  QVERIFY2(rendererSource.contains(QStringLiteral("PrepareSceneData::DirtySelection")),
           "Selection/hover updates must be tracked with DirtySelection");

  const int uploadModelStart = rendererSource.indexOf(QStringLiteral("bool RhiViewportRenderer::uploadModelBuffer"));
  const int uploadHighlightStart = rendererSource.indexOf(QStringLiteral("bool RhiViewportRenderer::uploadHighlightBuffer"));
  QVERIFY2(uploadModelStart >= 0 && uploadHighlightStart > uploadModelStart,
           "RhiViewportRenderer uploadModelBuffer boundaries changed; update selection upload audit");
  QVERIFY2(!rendererSource.mid(uploadModelStart, uploadHighlightStart - uploadModelStart)
               .contains(QStringLiteral("DirtySelection")),
           "Selection/hover changes must not reupload the full model vertex buffer");
}

void QmlUiAuditTests::rhiViewportModelDragOrbitsAfterClickThreshold()
{
  const QString viewportSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.cpp"));
  QVERIFY2(!viewportSource.isEmpty(), "Unable to read RhiViewport.cpp");

  const int moveStart = viewportSource.indexOf(QStringLiteral("void RhiViewport::mouseMoveEvent"));
  const int releaseStart = viewportSource.indexOf(QStringLiteral("\nvoid RhiViewport::mouseReleaseEvent"), moveStart);
  QVERIFY2(moveStart >= 0 && releaseStart > moveStart,
           "RhiViewport mouseMoveEvent body must stay discoverable for drag/orbit audit");
  const QString moveBlock = viewportSource.mid(moveStart, releaseStart - moveStart);

  QVERIFY2(moveBlock.contains(QStringLiteral("const bool becameDrag"))
               && moveBlock.contains(QStringLiteral("std::hypot"))
               && moveBlock.contains(QStringLiteral("m_pressPickedSourceObjectIndex = -1"))
               && moveBlock.contains(QStringLiteral("m_camera.orbit")),
           "Model-surface left drags must orbit after the click threshold instead of only pinning hover");
}

void QmlUiAuditTests::rhiMoveGizmoDragBridgeStaysCppOwned()
{
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString editorHeader = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString viewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString viewportSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.cpp"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!editorHeader.isEmpty(), "Unable to read EditorViewModel.h");
  QVERIFY2(!viewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!viewportSource.isEmpty(), "Unable to read RhiViewport.cpp");

  QVERIFY2(editorHeader.contains(QStringLiteral("beginGizmoMoveDrag()"))
               && editorHeader.contains(QStringLiteral("applyGizmoMoveDelta(float dx, float dy, float dz)"))
               && editorHeader.contains(QStringLiteral("endGizmoMoveDrag()")),
           "EditorViewModel must expose move-gizmo drag begin/apply/end methods");

  QVERIFY2(viewportHeader.contains(QStringLiteral("void gizmoMoveRequested(const QVector3D &worldDelta);"))
               && viewportHeader.contains(QStringLiteral("void gizmoDragBegin();"))
               && viewportHeader.contains(QStringLiteral("void gizmoDragEnd();")),
           "RhiViewport must expose move-gizmo drag signals to QML");

  QVERIFY2(viewportSource.contains(QStringLiteral("GizmoMath::computeRay"))
               && viewportSource.contains(QStringLiteral("GizmoMath::pickMoveAxis"))
               && viewportSource.contains(QStringLiteral("GizmoMath::rayToAxisT")),
           "RhiViewport must own move-axis ray, pick, and drag-delta math");
  QVERIFY2(viewportSource.contains(QStringLiteral("event->accept();"))
               && viewportSource.contains(QStringLiteral("emit gizmoMoveRequested(frameDelta);")),
           "RhiViewport must consume active gizmo drags and emit frame deltas");

  QVERIFY2(preparePage.contains(QStringLiteral("onGizmoDragBegin"))
               && preparePage.contains(QStringLiteral("root.editorVm.beginGizmoMoveDrag()"))
               && preparePage.contains(QStringLiteral("onGizmoMoveRequested: function(worldDelta)"))
               && preparePage.contains(QStringLiteral("root.editorVm.applyGizmoMoveDelta(worldDelta.x, worldDelta.y, worldDelta.z)"))
               && preparePage.contains(QStringLiteral("onGizmoDragEnd"))
               && preparePage.contains(QStringLiteral("root.editorVm.endGizmoMoveDrag()")),
           "PreparePage must forward RHI move-gizmo drag signals to EditorViewModel");

  QVERIFY2(!preparePage.contains(QStringLiteral("pickMoveAxis"))
               && !preparePage.contains(QStringLiteral("computeRay"))
               && !preparePage.contains(QStringLiteral("rayToAxisT"))
               && !preparePage.contains(QStringLiteral("intersect")),
           "PreparePage must not own gizmo picking, ray, intersection, or drag math");
}

void QmlUiAuditTests::rhiRotateScaleGizmoBridgeStaysCppOwned()
{
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString editorHeader = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString viewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString viewportSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.cpp"));
  const QString rendererHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.h"));
  const QString rendererSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!editorHeader.isEmpty(), "Unable to read EditorViewModel.h");
  QVERIFY2(!viewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!viewportSource.isEmpty(), "Unable to read RhiViewport.cpp");
  QVERIFY2(!rendererHeader.isEmpty(), "Unable to read RhiViewportRenderer.h");
  QVERIFY2(!rendererSource.isEmpty(), "Unable to read RhiViewportRenderer.cpp");

  QVERIFY2(editorHeader.contains(QStringLiteral("beginGizmoRotateDrag()"))
               && editorHeader.contains(QStringLiteral("applyGizmoRotateDelta(int axis, float radians)"))
               && editorHeader.contains(QStringLiteral("endGizmoRotateDrag()"))
               && editorHeader.contains(QStringLiteral("beginGizmoScaleDrag()"))
               && editorHeader.contains(QStringLiteral("applyGizmoScaleFactor(int axis, float factor)"))
               && editorHeader.contains(QStringLiteral("endGizmoScaleDrag()")),
           "EditorViewModel must expose rotate/scale gizmo drag begin/apply/end methods");

  QVERIFY2(viewportHeader.contains(QStringLiteral("void gizmoRotateRequested(int axis, float radians);"))
               && viewportHeader.contains(QStringLiteral("void gizmoScaleRequested(int axis, float factor);")),
           "RhiViewport must expose rotate/scale drag signals to QML");

  QVERIFY2(viewportSource.contains(QStringLiteral("GizmoMath::pickRotateAxis"))
               && viewportSource.contains(QStringLiteral("GizmoMath::computeRotateAngle"))
               && viewportSource.contains(QStringLiteral("GizmoMath::pickScaleAxis"))
               && viewportSource.contains(QStringLiteral("GizmoMath::rayToAxisT")),
           "RhiViewport must own rotate/scale pick and drag math");
  QVERIFY2(viewportSource.contains(QStringLiteral("emit gizmoRotateRequested"))
               && viewportSource.contains(QStringLiteral("emit gizmoScaleRequested"))
               && viewportSource.contains(QStringLiteral("event->accept();")),
           "RhiViewport must consume active rotate/scale drags and emit frame updates");

  QVERIFY2(rendererHeader.contains(QStringLiteral("GizmoGeometryOffsets"))
               && rendererHeader.contains(QStringLiteral("renderRotateGizmo"))
               && rendererHeader.contains(QStringLiteral("renderScaleGizmo")),
           "RhiViewportRenderer must store gizmo offsets and expose rotate/scale render helpers");
  QVERIFY2(rendererSource.contains(QStringLiteral("GizmoGeometry::buildRotateGizmoVertices"))
               && rendererSource.contains(QStringLiteral("GizmoGeometry::buildScaleGizmoVertices"))
               && rendererSource.contains(QStringLiteral("renderRotateGizmo(cb)"))
               && rendererSource.contains(QStringLiteral("renderScaleGizmo(cb)")),
           "RhiViewportRenderer must upload and draw rotate/scale gizmo geometry");
  QVERIFY2(rendererSource.contains(QStringLiteral("verts.reserve(moveVerts.size() + rotateVerts.size() + scaleVerts.size())")),
           "RhiViewportRenderer must reserve the combined gizmo vertex count before GPU upload");

  const QString geometrySource = readSource(QStringLiteral("src/core/rendering/GizmoGeometry.cpp"));
  QVERIFY2(!geometrySource.isEmpty(), "Unable to read GizmoGeometry.cpp");
  QVERIFY2(geometrySource.contains(QStringLiteral("verts.reserve(3 * kRotateVertsPerRing);")),
           "Rotate gizmo geometry must reserve vertex count, not byte count");
  QVERIFY2(!geometrySource.contains(QStringLiteral("kRotateVertsPerRing * int(sizeof(GizmoVertex))")),
           "Rotate gizmo geometry must not multiply reserve count by sizeof(GizmoVertex)");

  QVERIFY2(preparePage.contains(QStringLiteral("onGizmoRotateRequested: function(axis, radians)"))
               && preparePage.contains(QStringLiteral("root.editorVm.applyGizmoRotateDelta(axis, radians)"))
               && preparePage.contains(QStringLiteral("onGizmoScaleRequested: function(axis, factor)"))
               && preparePage.contains(QStringLiteral("root.editorVm.applyGizmoScaleFactor(axis, factor)")),
           "PreparePage must forward RHI rotate/scale drag signals to EditorViewModel");

  const QStringList forbiddenQmlMath = {
      QStringLiteral("pickRotateAxis"),
      QStringLiteral("pickScaleAxis"),
      QStringLiteral("computeRotateAngle"),
      QStringLiteral("rayToAxisT"),
      QStringLiteral("computeRay"),
      QStringLiteral("scaleFactor"),
      QStringLiteral("intersect")
  };
  for (const QString &token : forbiddenQmlMath) {
    QVERIFY2(!preparePage.contains(token),
             qPrintable(QStringLiteral("PreparePage must not own rotate/scale gizmo math token: %1").arg(token)));
  }
}

void QmlUiAuditTests::rhiGizmosRenderAsDepthIndependentOverlay()
{
  const QString rendererSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  QVERIFY2(!rendererSource.isEmpty(), "Unable to read RhiViewportRenderer.cpp");

  const int pipelineStart = rendererSource.indexOf(QStringLiteral("bool RhiViewportRenderer::ensureGizmoPipeline"));
  const int renderStart = rendererSource.indexOf(QStringLiteral("\nvoid RhiViewportRenderer::renderMoveGizmo"), pipelineStart);
  QVERIFY2(pipelineStart >= 0 && renderStart > pipelineStart,
           "RhiViewportRenderer ensureGizmoPipeline body must stay discoverable for overlay audit");
  const QString pipelineBlock = rendererSource.mid(pipelineStart, renderStart - pipelineStart);

  QVERIFY2(pipelineBlock.contains(QStringLiteral("pipe->setDepthTest(false)"))
               && pipelineBlock.contains(QStringLiteral("pipe->setDepthWrite(false)")),
           "RHI gizmo pipelines must render as overlays: no depth test and no depth writes");
  QVERIFY2(!pipelineBlock.contains(QStringLiteral("pipe->setDepthTest(true)")),
           "RHI gizmo pipelines must not test against model depth already written earlier in the pass");
}

void QmlUiAuditTests::rhiCutPlaneAndWipeTowerStayCppOwned()
{
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString geometryHeader = readSource(QStringLiteral("src/core/rendering/GizmoGeometry.h"));
  const QString geometrySource = readSource(QStringLiteral("src/core/rendering/GizmoGeometry.cpp"));
  const QString viewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString rendererHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.h"));
  const QString rendererSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!geometryHeader.isEmpty(), "Unable to read GizmoGeometry.h");
  QVERIFY2(!geometrySource.isEmpty(), "Unable to read GizmoGeometry.cpp");
  QVERIFY2(!viewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!rendererHeader.isEmpty(), "Unable to read RhiViewportRenderer.h");
  QVERIFY2(!rendererSource.isEmpty(), "Unable to read RhiViewportRenderer.cpp");

  QVERIFY2(geometryHeader.contains(QStringLiteral("buildCutPlaneVertices"))
               && geometryHeader.contains(QStringLiteral("buildCutPlaneOutlineVertices"))
               && geometryHeader.contains(QStringLiteral("buildWipeTowerVertices")),
           "GizmoGeometry must expose pure CPU cut-plane and wipe-tower builders");
  QVERIFY2(geometrySource.contains(QStringLiteral("0.30f"))
               && geometrySource.contains(QStringLiteral("0.90f"))
               && geometrySource.contains(QStringLiteral("-0.04f"))
               && geometrySource.contains(QStringLiteral("0.50f")),
           "GizmoGeometry must preserve GL cut/wipe alpha and bed placement constants");

  QVERIFY2(viewportHeader.contains(QStringLiteral("Q_PROPERTY(bool showWipeTower"))
               && viewportHeader.contains(QStringLiteral("Q_PROPERTY(float wipeTowerWidth"))
               && viewportHeader.contains(QStringLiteral("Q_PROPERTY(float wipeTowerDepth"))
               && viewportHeader.contains(QStringLiteral("Q_PROPERTY(float wipeTowerHeight"))
               && viewportHeader.contains(QStringLiteral("Q_PROPERTY(float wipeTowerX"))
               && viewportHeader.contains(QStringLiteral("Q_PROPERTY(float wipeTowerZ"))
               && viewportHeader.contains(QStringLiteral("Q_PROPERTY(int cutAxis"))
               && viewportHeader.contains(QStringLiteral("Q_PROPERTY(float cutPosition")),
           "RhiViewport must keep cut and wipe tower properties available to QML");

  QVERIFY2(rendererHeader.contains(QStringLiteral("m_cutPlaneFillBuffer"))
               && rendererHeader.contains(QStringLiteral("m_cutPlaneOutlineBuffer"))
               && rendererHeader.contains(QStringLiteral("m_wipeTowerBuffer"))
               && rendererHeader.contains(QStringLiteral("m_translucentLinePipeline"))
               && rendererHeader.contains(QStringLiteral("m_cutPlaneDirty"))
               && rendererHeader.contains(QStringLiteral("m_wipeTowerDirty"))
               && rendererHeader.contains(QStringLiteral("renderCutPlane"))
               && rendererHeader.contains(QStringLiteral("renderWipeTower")),
           "RhiViewportRenderer must own cut/wipe buffers, dirty state, and render helpers");

  QVERIFY2(rendererSource.contains(QStringLiteral("m_showWipeTower = viewport->m_showWipeTower"))
               && rendererSource.contains(QStringLiteral("m_wipeTowerWidth = viewport->m_wipeTowerWidth"))
               && rendererSource.contains(QStringLiteral("m_wipeTowerDepth = viewport->m_wipeTowerDepth"))
               && rendererSource.contains(QStringLiteral("m_wipeTowerHeight = viewport->m_wipeTowerHeight"))
               && rendererSource.contains(QStringLiteral("m_wipeTowerX = viewport->m_wipeTowerX"))
               && rendererSource.contains(QStringLiteral("m_wipeTowerZ = viewport->m_wipeTowerZ")),
           "RhiViewportRenderer::synchronize must read all wipe tower properties");

  QVERIFY2(rendererSource.contains(QStringLiteral("GizmoGeometry::buildCutPlaneVertices"))
               && rendererSource.contains(QStringLiteral("GizmoGeometry::buildCutPlaneOutlineVertices"))
               && rendererSource.contains(QStringLiteral("GizmoGeometry::buildWipeTowerVertices"))
               && rendererSource.contains(QStringLiteral("selectedBounds.minX"))
               && rendererSource.contains(QStringLiteral("std::min(selectedBounds.minX")),
           "RhiViewportRenderer must consume builders and merge selected-object batches before building cut geometry");

  QVERIFY2(rendererSource.contains(QStringLiteral("QRhiGraphicsPipeline::TargetBlend"))
               && rendererSource.contains(QStringLiteral("enable.srcColor = QRhiGraphicsPipeline::SrcAlpha"))
               && rendererSource.contains(QStringLiteral("enable.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha"))
               && rendererSource.contains(QStringLiteral("setDepthWrite(false)")),
           "RhiViewportRenderer must configure alpha blending with no depth writes for translucent cut/wipe surfaces");

  const int cutRenderStart = rendererSource.indexOf(QStringLiteral("void RhiViewportRenderer::renderCutPlane"));
  QVERIFY2(cutRenderStart >= 0, "RhiViewportRenderer must define renderCutPlane");
  const int nextRender = rendererSource.indexOf(QStringLiteral("\nvoid RhiViewportRenderer::renderWipeTower"), cutRenderStart);
  const QString cutRenderBlock = rendererSource.mid(cutRenderStart, nextRender > cutRenderStart ? nextRender - cutRenderStart : 1200);
  QVERIFY2(cutRenderBlock.contains(QStringLiteral("m_gizmoMode != 5"))
               && cutRenderBlock.contains(QStringLiteral("m_gizmoMode != 14"))
               && cutRenderBlock.contains(QStringLiteral("selectedSourceObjectIndex() < 0")),
           "RHI cut plane render must be gated to Cut/AdvancedCut mode and selected objects");

  QVERIFY2(preparePage.contains(QStringLiteral("cutAxis: root.editorVm ? root.editorVm.cutAxis : 2"))
               && preparePage.contains(QStringLiteral("cutPosition: root.editorVm ? root.editorVm.cutPosition : 0.0")),
           "PreparePage must bind cut axis and position into the viewport");

  const QStringList forbiddenQmlGeometry = {
      QStringLiteral("buildCutPlane"),
      QStringLiteral("buildWipeTower"),
      QStringLiteral("QRhi"),
      QStringLiteral("bboxMin"),
      QStringLiteral("bboxMax"),
      QStringLiteral("TargetBlend")
  };
  for (const QString &token : forbiddenQmlGeometry) {
    QVERIFY2(!preparePage.contains(token),
             qPrintable(QStringLiteral("PreparePage must not own cut/wipe geometry or QRhi token: %1").arg(token)));
  }
}

void QmlUiAuditTests::visiblePlaceholderSurfacesAreHonest()
{
  const QString topbar = readSource(QStringLiteral("src/qml_gui/BBLTopbar.qml"));
  const QString mainQml = readSource(QStringLiteral("src/qml_gui/main.qml"));
  const QString sidebar = readSource(QStringLiteral("src/qml_gui/panels/LeftSidebar.qml"));
  const QString modelMall = readSource(QStringLiteral("src/qml_gui/pages/ModelMallPage.qml"));
  const QString preferences = readSource(QStringLiteral("src/qml_gui/pages/PreferencesPage.qml"));
  QVERIFY2(!topbar.isEmpty(), "Unable to read BBLTopbar.qml");
  QVERIFY2(!mainQml.isEmpty(), "Unable to read main.qml");
  QVERIFY2(!sidebar.isEmpty(), "Unable to read LeftSidebar.qml");
  QVERIFY2(!modelMall.isEmpty(), "Unable to read ModelMallPage.qml");
  QVERIFY2(!preferences.isEmpty(), "Unable to read PreferencesPage.qml");

  const QRegularExpression emptyClick(QStringLiteral("\\bon(?:Clicked|Triggered):\\s*\\{\\s*\\}"));
  QVERIFY2(!emptyClick.match(topbar).hasMatch(), "Topbar must not contain empty click/trigger handlers");
  QVERIFY2(!emptyClick.match(mainQml).hasMatch(), "main.qml must not contain empty click/trigger handlers");
  QVERIFY2(!emptyClick.match(sidebar).hasMatch(), "LeftSidebar must not contain empty click/trigger handlers");
  QVERIFY2(!emptyClick.match(preferences).hasMatch(), "PreferencesPage must not contain empty click/trigger handlers");

  const QStringList forbiddenRuntimeCopy = {
    QStringLiteral("qsTr(\"Search models, authors, tags...\")"),
    QStringLiteral("qsTr(\"Publish\")"),
    QStringLiteral("qsTr(\"Recommended\")"),
    QStringLiteral("qsTr(\"Popular\")"),
    QStringLiteral("qsTr(\"Newest\")"),
    QStringLiteral("qsTr(\"Free\")"),
    QStringLiteral("qsTr(\"Favorites\")")
  };
  for (const QString &copy : forbiddenRuntimeCopy) {
    QVERIFY2(!modelMall.contains(copy),
             qPrintable(QStringLiteral("ModelMall must not expose fake marketplace copy: %1").arg(copy)));
  }

  const QStringList forbiddenSidebarMarkers = {
    QStringLiteral("TODO SIDEBAR-08"),
    QStringLiteral("TODO SIDEBAR-09"),
    QStringLiteral("TODO SIDEBAR-10"),
    QStringLiteral("TODO SIDEBAR-14"),
    QStringLiteral("TODO SIDEBAR-15")
  };
  for (const QString &marker : forbiddenSidebarMarkers) {
    QVERIFY2(!sidebar.contains(marker),
             qPrintable(QStringLiteral("LeftSidebar runtime placeholder marker remains: %1").arg(marker)));
  }

  QVERIFY2(preferences.contains(QStringLiteral("enabled: false")),
           "Unavailable Preferences update check must be visibly disabled");
}

void QmlUiAuditTests::plateContextMenuItemsWiredAndNonEmpty()
{
  // Phase 22 (UI-3): actively guard the v3.0 Phase 17 plate-lifecycle menu wiring.
  // Without this, a regression (deleted menu item / empty onTriggered / broken
  // enabled) would pass the other 7 audit tests, since they only check generic
  // honest-UI rules, not the specific Phase 17 items.
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");

  // The three Phase 17 operations must be wired to real menu actions.
  QVERIFY2(preparePage.contains(QStringLiteral("clonePlate(")),
           "PreparePage must wire a clonePlate plate-context-menu action (PLATE-03)");
  QVERIFY2(preparePage.contains(QStringLiteral("movePlate(")),
           "PreparePage must wire a movePlate plate-context-menu action (PLATE-04)");
  QVERIFY2(preparePage.contains(QStringLiteral("setPlatePrintable(")),
           "PreparePage must wire a setPlatePrintable plate-context-menu action (PLATE-05)");

  // Strengthen the honest-UI contract: no empty onTriggered handlers anywhere in
  // PreparePage (Phase 14 forbade these; Phase 22 extends the guard here).
  QVERIFY2(!preparePage.contains(QStringLiteral("onTriggered: {}")),
           "PreparePage must not contain empty onTriggered: {} handlers");
}

// Phase 51-03 (SHELL-03): shell actions bind to BackendContext gates.
// Locks the BBLTopbar binding contract from Plan 51-02 against regression:
// Undo/Redo/Slice/Save must bind `enabled` to the C++ gate properties so the
// undo-stack-empty UX bug (Undo clickable when the stack is empty) cannot
// silently return. This is the QML route/registration automated test for
// SHELL-03 (CONTEXT: "QML route/resource registration test").

void QmlUiAuditTests::shellActionsBindToBackendContextGates()
{
  const QString topbar = readSource(QStringLiteral("src/qml_gui/BBLTopbar.qml"));
  QVERIFY2(!topbar.isEmpty(), "Unable to read BBLTopbar.qml");

  // Undo/Redo toolbar buttons must bind enabled to BOTH the page gate AND the
  // C++ undo/redo-stack gate (defense-in-depth; CONTEXT decision to keep both
  // checks in QML). This is the concrete fix for the stack-empty UX bug.
  QVERIFY2(topbar.contains(QStringLiteral("backend.currentPage === backend.tp3DEditor && backend.canUndo")),
           "Undo toolbar button must bind enabled to page-gate AND backend.canUndo");
  QVERIFY2(topbar.contains(QStringLiteral("backend.currentPage === backend.tp3DEditor && backend.canRedo")),
           "Redo toolbar button must bind enabled to page-gate AND backend.canRedo");

  // Page gate preserved as defense-in-depth (the new gate is ADDITIONAL).
  QVERIFY2(topbar.contains(QStringLiteral("backend.currentPage === backend.tp3DEditor")),
           "Undo/Redo must keep the Prepare page gate");

  // Save toolbar button gated on canSave (forwards to !isSlicing && !isBusy).
  QVERIFY2(topbar.contains(QStringLiteral("enabled: backend.canSave")),
           "Save toolbar button must bind enabled to backend.canSave");

  // Slice side-tool button gated on canSlice.
  QVERIFY2(topbar.contains(QStringLiteral("enabled: backend.canSlice")),
           "Slice side-tool button must bind enabled to backend.canSlice");

  // canUndo / canRedo must each appear at least twice (toolbar icon + Edit-menu
  // item) so a regression deleting one binding is caught.
  const int canUndoCount = topbar.count(QStringLiteral("backend.canUndo"));
  QVERIFY2(canUndoCount >= 2,
           qPrintable(QStringLiteral("backend.canUndo must appear >= 2 times (toolbar + menu), got %1").arg(canUndoCount)));
  const int canRedoCount = topbar.count(QStringLiteral("backend.canRedo"));
  QVERIFY2(canRedoCount >= 2,
           qPrintable(QStringLiteral("backend.canRedo must appear >= 2 times (toolbar + menu), got %1").arg(canRedoCount)));
}

void QmlUiAuditTests::mainWindowDefaultsToFramelessMaximizedShell()
{
  const QString mainQml = readSource(QStringLiteral("src/qml_gui/main.qml"));
  const QString mainCpp = readSource(QStringLiteral("src/qml_gui/main_qml.cpp"));
  QVERIFY2(!mainQml.isEmpty(), "Unable to read main.qml");
  QVERIFY2(!mainCpp.isEmpty(), "Unable to read main_qml.cpp");

  QVERIFY2(mainQml.contains(QStringLiteral("flags: Qt.Window | Qt.FramelessWindowHint")),
           "Main window must default to the custom frameless shell for screenshot parity");
  QVERIFY2(mainQml.contains(QStringLiteral("visibility: Window.Maximized")),
           "Main window must start maximized so runtime screenshots align with the reference frame");
  QVERIFY2(!mainQml.contains(QStringLiteral("flags: owzxUseFramelessShell ?")),
           "Frameless shell must not be disabled by default behind an environment flag");
  QVERIFY2(!mainCpp.contains(QStringLiteral("owzxUseFramelessShell")),
           "main_qml.cpp must not expose an opt-in-only frameless context property");
}

// Phase 51-03 (SHELL-04): notification surfaces stay non-overlapping.
// Guards the existing non-overlapping placement of the 3 notification surfaces
// (ErrorBanner inline / ErrorToast z=200 floating / NotificationCenter top-right
// popup). Phase 51 verifies + guards, it does not redesign placement (CONTEXT).
// This asserts the anchoring contract is unchanged so a future edit cannot
// accidentally overlap the viewport/sidebar or collapse the surfaces.

void QmlUiAuditTests::notificationSurfacesStayNonOverlapping()
{
  const QString banner = readSource(QStringLiteral("src/qml_gui/components/ErrorBanner.qml"));
  const QString toast = readSource(QStringLiteral("src/qml_gui/components/ErrorToast.qml"));
  const QString center = readSource(QStringLiteral("src/qml_gui/components/NotificationCenter.qml"));
  const QString mainQml = readSource(QStringLiteral("src/qml_gui/main.qml"));
  QVERIFY2(!banner.isEmpty(), "Unable to read ErrorBanner.qml");
  QVERIFY2(!toast.isEmpty(), "Unable to read ErrorToast.qml");
  QVERIFY2(!center.isEmpty(), "Unable to read NotificationCenter.qml");
  QVERIFY2(!mainQml.isEmpty(), "Unable to read main.qml");

  // ErrorBanner: inline push-down between top bar and content. It must NOT float
  // at z=200 (that placement belongs to ErrorToast); a z=200 on the banner
  // would overlap the viewport.
  QVERIFY2(banner.contains(QStringLiteral("Layout.fillWidth: true")),
           "ErrorBanner must stay inline Layout.fillWidth (no overlap with viewport)");
  QVERIFY2(!banner.contains(QStringLiteral("z: 200")),
           "ErrorBanner must not float at z=200 (that is the ErrorToast placement)");

  // ErrorToast: floating centered overlay anchored to the bottom, z=200. It
  // anchors to the bottom (not covering the left sidebar or the slice/print
  // dropdowns).
  QVERIFY2(toast.contains(QStringLiteral("z: 200")),
           "ErrorToast must keep z=200 floating overlay placement");
  QVERIFY2(toast.contains(QStringLiteral("anchors.bottom")),
           "ErrorToast must anchor to the bottom (not covering the sidebar)");

  // NotificationCenter: keeps explicit placement (top-right popup, not floating
  // free over the viewport). The popup host in main.qml sets x: root.width-340.
  QVERIFY2(center.contains(QStringLiteral("anchors")) || center.contains(QStringLiteral("Popup")) || center.contains(QStringLiteral("x:")),
           "NotificationCenter must keep explicit placement (anchors/Popup/x:)");

  // main.qml keeps all 3 surfaces mounted so none is silently dropped.
  QVERIFY2(mainQml.contains(QStringLiteral("ErrorBanner { }")),
           "main.qml must mount ErrorBanner");
  QVERIFY2(mainQml.contains(QStringLiteral("ErrorToast { }")),
           "main.qml must mount ErrorToast");
  QVERIFY2(mainQml.contains(QStringLiteral("NotificationCenter {")),
           "main.qml must mount NotificationCenter");
}

// Phase 52-03 (PREPSB-01..04): LeftSidebar + FilamentSlot bindings audit.
// Source-text audit that guards every Plan 52-02 binding against regression:
// dynamic extruder-count slot count (PREPSB-01), hidden dead color picker
// (PREPSB-01), dirty dots + read-only gating (PREPSB-03), enabled Setting
// entry point (PREPSB-02), live search filter (PREPSB-04), and the complete
// Global/Object/Plate scope triad (PREPSB-04). Mirrors the Phase 51-03
// shellActionsBindToBackendContextGates source-grep pattern.

void QmlUiAuditTests::leftSidebarPresetControlsAreWiredAndHonest()
{
  const QString sidebar = readSource(QStringLiteral("src/qml_gui/panels/LeftSidebar.qml"));
  const QString slot = readSource(QStringLiteral("src/qml_gui/components/FilamentSlot.qml"));
  QVERIFY2(!sidebar.isEmpty(), "Unable to read LeftSidebar.qml");
  QVERIFY2(!slot.isEmpty(), "Unable to read FilamentSlot.qml");

  // PREPSB-01: filament slot count is dynamic (no hard-coded model: 5).
  QVERIFY2(!sidebar.contains(QStringLiteral("\n                            model: 5\n")),
           "LeftSidebar must not use the hard-coded model: 5 filament slot count");
  QVERIFY2(sidebar.contains(QStringLiteral("extruderCount")),
           "LeftSidebar filament Repeater must bind to editorVm.extruderCount");
  QVERIFY2(sidebar.contains(QStringLiteral("Math.max(1,")),
           "LeftSidebar slot count must guard against 0 with Math.max(1, ...)");

  // PREPSB-01: dead color picker popup must be hidden (no toggle onClicked).
  QVERIFY2(!slot.contains(QStringLiteral("colorPickerLoader.active = !colorPickerLoader.active")),
           "FilamentSlot must not toggle the dead color picker popup");
  QVERIFY2(slot.contains(QStringLiteral("TODO(Phase 56)")),
           "FilamentSlot must carry an honest Phase-56 TODO for the color picker");

  // PREPSB-03: dirty state surfaced via isPresetDirty (printer + process).
  QVERIFY2(sidebar.count(QStringLiteral("isPresetDirty")) >= 2,
           "LeftSidebar must surface isPresetDirty on >= 2 preset rows (printer + process)");
  QVERIFY2(sidebar.contains(QStringLiteral("presetActionBlocker(2,")),
           "LeftSidebar printer edit must gate on presetActionBlocker (read-only)");

  // PREPSB-02: Setting button visible+enabled and forwards the request.
  QVERIFY2(sidebar.contains(QStringLiteral("backend.forwardSettingsRequest(\"process\")")),
           "LeftSidebar Setting button must call backend.forwardSettingsRequest(\"process\")");

  // PREPSB-04: search box rebuilds the shared ParamsPanel filter on accept/change.
  QVERIFY2(sidebar.contains(QStringLiteral("function rebuildParamsFilter()"))
               && sidebar.contains(QStringLiteral("filterOptionIndices")),
           "LeftSidebar must centralize ParamsPanel filtering through filterOptionIndices");
  QVERIFY2(sidebar.count(QStringLiteral("root.rebuildParamsFilter()")) >= 3,
           "LeftSidebar search/tab changes must call rebuildParamsFilter");

  // PREPSB-04: scope toggles complete (Global/Object/Plate).
  QVERIFY2(sidebar.contains(QStringLiteral("requestGlobalScope")),
           "LeftSidebar must have a Global scope toggle");
  QVERIFY2(sidebar.contains(QStringLiteral("requestObjectScope")),
           "LeftSidebar must have an Object scope toggle");
  QVERIFY2(sidebar.contains(QStringLiteral("requestPlateScope")),
           "LeftSidebar must have a Plate scope toggle");
}

void QmlUiAuditTests::prepareWorkflowActionsBindCppGates()
{
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString glToolbars = readSource(QStringLiteral("src/qml_gui/components/GLToolbars.qml"));
  const QString objectList = readSource(QStringLiteral("src/qml_gui/panels/ObjectList.qml"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!glToolbars.isEmpty(), "Unable to read GLToolbars.qml");
  QVERIFY2(!objectList.isEmpty(), "Unable to read ObjectList.qml");

  QVERIFY2(!preparePage.contains(QStringLiteral("plateCount < 10"))
               && !preparePage.contains(QStringLiteral("plateCount < 36"))
               && !glToolbars.contains(QStringLiteral("plateCount < 10"))
               && !glToolbars.contains(QStringLiteral("plateCount < 36")),
           "Prepare plate add/clone controls must not hard-code the upstream plate limit");
  QVERIFY2(!preparePage.contains(QStringLiteral("canAddPlate()"))
               && !glToolbars.contains(QStringLiteral("canAddPlate()")),
           "QML must bind canAddPlate as a NOTIFY property, not call it as a function");
  QVERIFY2(preparePage.contains(QStringLiteral("root.editorVm.canAddPlate"))
               && glToolbars.contains(QStringLiteral("root.editorVm.canAddPlate")),
           "Prepare plate controls must bind to EditorViewModel::canAddPlate");

  QVERIFY2(!preparePage.contains(QStringLiteral("canActivateGizmo("))
               && !glToolbars.contains(QStringLiteral("canActivateGizmo("))
               && !objectList.contains(QStringLiteral("canActivateGizmo(")),
           "QML gizmo controls must bind to availableGizmoMask for reactive updates");
  QVERIFY2(preparePage.contains(QStringLiteral("availableGizmoMask"))
               && glToolbars.contains(QStringLiteral("availableGizmoMask"))
               && objectList.contains(QStringLiteral("availableGizmoMask")),
           "Prepare gizmo controls must bind to EditorViewModel::availableGizmoMask");
  QVERIFY2(!preparePage.contains(QStringLiteral("deleteSelectedObjects("))
               && !objectList.contains(QStringLiteral("deleteSelectedObjects("))
               && !objectList.contains(QStringLiteral("deleteObject(")),
           "Prepare delete actions must route through EditorViewModel::deleteSelection");

  const QStringList requiredObjectGates = {
    QStringLiteral("canRenameSelectedObject"),
    QStringLiteral("canDuplicateSelectedObjects"),
    QStringLiteral("canDeleteSelection"),
    QStringLiteral("canSetSelectionPrintable"),
    QStringLiteral("canTransformSelection")
  };
  for (const QString &gate : requiredObjectGates) {
    QVERIFY2(preparePage.contains(gate) || objectList.contains(gate),
             qPrintable(QStringLiteral("Prepare object actions must bind to %1").arg(gate)));
  }
}

// ── Phase 55-04 (GCODE-04/05): Preview renderer source-audit guards ──
// These three source-audit tests lock the renderer/UI contracts that defend
// the disappearing-preview regression class: PreviewPage must never instantiate
// SoftwareViewport, computePreviewDrawRanges must consult the role-visibility
// mask for render-side filtering, and the GcvPackedSegment wire format must
// stay sizeof-locked at 76 bytes (Plan 02 lockstep).

// Phase 55 (GCODE-04): PreviewPage.qml must never reference SoftwareViewport.
// The normal Preview path is RhiViewport (registered as GLViewport); the
// SoftwareViewport only exists as a QRhi init fallback. This is the regression
// gate that fails the build if anyone re-adds SoftwareViewport to PreviewPage.
void QmlUiAuditTests::prepareWorkflowPanelsMatchRestorationContract()
{
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString glToolbars = readSource(QStringLiteral("src/qml_gui/components/GLToolbars.qml"));
  const QString objectList = readSource(QStringLiteral("src/qml_gui/panels/ObjectList.qml"));
  const QString sliceProgress = readSource(QStringLiteral("src/qml_gui/panels/SliceProgress.qml"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!glToolbars.isEmpty(), "Unable to read GLToolbars.qml");
  QVERIFY2(!objectList.isEmpty(), "Unable to read ObjectList.qml");
  QVERIFY2(!sliceProgress.isEmpty(), "Unable to read SliceProgress.qml");

  QVERIFY2(!glToolbars.contains(QStringLiteral("id: sliceButton")),
           "Phase 76 removes the large floating viewport Slice button");
  QVERIFY2(!glToolbars.contains(QStringLiteral("text: \">>\"")),
           "Viewport overlays must not expose the legacy floating >> Slice affordance");

  const QStringList objectListTokens = {
    QStringLiteral("readonly property int objectRowHeight: 38"),
    QStringLiteral("readonly property int volumeRowHeight: 26"),
    QStringLiteral("readonly property int groupHeaderHeight: 18"),
    QStringLiteral("objectListStatusPill"),
    QStringLiteral("row.objPrintable ? Theme.accent : Theme.textDisabled"),
    QStringLiteral("enabled: !!root.editorVm && root.editorVm.canDeleteSelection")
  };
  for (const QString &token : objectListTokens) {
    QVERIFY2(objectList.contains(token),
             qPrintable(QStringLiteral("ObjectList must preserve Phase 76 compact/tree contract token: %1").arg(token)));
  }

  const QStringList plateStripTokens = {
    QStringLiteral("height: 44"),
    QStringLiteral("width: 86"),
    QStringLiteral("plateStateText"),
    QStringLiteral("sliceResultStatus === 1"),
    QStringLiteral("sliceResultStatus === 2"),
    QStringLiteral("root.editorVm.canAddPlate")
  };
  for (const QString &token : plateStripTokens) {
    QVERIFY2(preparePage.contains(token),
             qPrintable(QStringLiteral("Prepare plate strip must preserve Phase 76 compact/state contract token: %1").arg(token)));
  }

  QVERIFY2(sliceProgress.contains(QStringLiteral("readonly property bool primaryActionEnabled")),
           "SliceProgress must expose one backend-gated primary action state");
  QVERIFY2(sliceProgress.contains(QStringLiteral("readonly property bool canSliceAll")),
           "Slice-all must be explicitly gated instead of being an always-clickable button");
  QVERIFY2(sliceProgress.contains(QStringLiteral("enabled: root.canSliceAll")),
           "Slice-all button must bind to the explicit backend-derived gate");
}

void QmlUiAuditTests::prepareViewportControlsMatchRestorationContract()
{
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString glToolbars = readSource(QStringLiteral("src/qml_gui/components/GLToolbars.qml"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!glToolbars.isEmpty(), "Unable to read GLToolbars.qml");

  const QStringList toolbarTokens = {
    QStringLiteral("readonly property int viewportToolbarHeight: 34"),
    QStringLiteral("readonly property int toolbarButtonSize: 30"),
    QStringLiteral("readonly property int gizmoToolbarWidth: 36"),
    QStringLiteral("id: viewportActionToolbar"),
    QStringLiteral("id: viewportGizmoToolbar"),
    QStringLiteral("id: viewportViewControls"),
    QStringLiteral("iconSource: iconForTool(toolId)")
  };
  for (const QString &token : toolbarTokens) {
    QVERIFY2(glToolbars.contains(token),
             qPrintable(QStringLiteral("GLToolbars must preserve Phase 77 icon/placement token: %1").arg(token)));
  }

  const QStringList removedTextLabels = {
    QStringLiteral("label: \"M\""),
    QStringLiteral("label: \"R\""),
    QStringLiteral("label: \"S\""),
    QStringLiteral("label: \"T\""),
    QStringLiteral("label: \"P+\""),
    QStringLiteral("label: \"AC\""),
    QStringLiteral("label: \"SVG\"")
  };
  for (const QString &label : removedTextLabels) {
    QVERIFY2(!glToolbars.contains(label),
             qPrintable(QStringLiteral("Viewport toolbars must be icon-first, not text-only: %1").arg(label)));
  }

  QVERIFY2(preparePage.contains(QStringLiteral("readonly property int gizmoPanelTopOffset")),
           "PreparePage must centralize gizmo panel top offset");
  QVERIFY2(preparePage.contains(QStringLiteral("transformMiniPanel")),
           "Move/rotate/scale modes must expose a compact transform mini panel");
  QVERIFY2(preparePage.contains(QStringLiteral("viewport3d.gizmoMode === GLViewport.GizmoMove"))
               && preparePage.contains(QStringLiteral("viewport3d.gizmoMode === GLViewport.GizmoRotate"))
               && preparePage.contains(QStringLiteral("viewport3d.gizmoMode === GLViewport.GizmoScale")),
           "Transform mini panel must bind to move, rotate, and scale modes");
}

void QmlUiAuditTests::prepareLeftSidebarMatchesPixelRestorationContract()
{
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString platerPage = readSource(QStringLiteral("src/qml_gui/pages/Plater.qml"));
  const QString dockableSidebar = readSource(QStringLiteral("src/qml_gui/panels/DockableSidebar.qml"));
  const QString leftSidebar = readSource(QStringLiteral("src/qml_gui/panels/LeftSidebar.qml"));
  const QString backendContext = readSource(QStringLiteral("src/qml_gui/BackendContext.h"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!platerPage.isEmpty(), "Unable to read Plater.qml");
  QVERIFY2(!dockableSidebar.isEmpty(), "Unable to read DockableSidebar.qml");
  QVERIFY2(!leftSidebar.isEmpty(), "Unable to read LeftSidebar.qml");
  QVERIFY2(!backendContext.isEmpty(), "Unable to read BackendContext.h");

  QVERIFY2(preparePage.contains(QStringLiteral("property int sidebarWidth: 392")),
           "Prepare page default sidebar width must match the screenshot left-panel boundary");
  QVERIFY2(preparePage.contains(QStringLiteral("property int sidebarMinWidth: 392"))
               && preparePage.contains(QStringLiteral("property int sidebarMaxWidth: 392")),
           "Pixel-restored Prepare sidebar must not start narrower than the screenshot width");
  QVERIFY2(platerPage.contains(QStringLiteral("property int sidebarWidth: 392"))
               && platerPage.contains(QStringLiteral("property int sidebarMinWidth: 392"))
               && platerPage.contains(QStringLiteral("property int sidebarMaxWidth: 392")),
           "Plater fallback sidebar width must match the Prepare pixel contract");
  QVERIFY2(backendContext.contains(QStringLiteral("kSidebarDefaultWidth = 392"))
               && backendContext.contains(QStringLiteral("kSidebarMinWidth = 392"))
               && backendContext.contains(QStringLiteral("kSidebarMaxWidth = 392")),
           "Backend persisted sidebar contract must not override the Prepare pixel width");

  QVERIFY2(!dockableSidebar.contains(QStringLiteral("id: titleBar")),
           "Expanded DockableSidebar must not add the extra settings title strip");
  QVERIFY2(dockableSidebar.contains(QStringLiteral("anchors.top: parent.top")),
           "LeftSidebar must start at the top of the expanded dock body");

  const QStringList layoutTokens = {
    QStringLiteral("readonly property int targetSidebarWidth: 392"),
    QStringLiteral("id: printerHeroCard"),
    QStringLiteral("id: printerThumbnail"),
    QStringLiteral("id: nozzleTile"),
    QStringLiteral("id: buildPlateTile"),
    QStringLiteral("id: heaterTile"),
    QStringLiteral("id: filamentPixelRow"),
    QStringLiteral("id: processScopeBar"),
    QStringLiteral("id: processPresetRow"),
    QStringLiteral("id: paramsInlinePanel"),
    QStringLiteral("id: paramsTabDelegate"),
    QStringLiteral("required property var modelData")
  };
  for (const QString &token : layoutTokens) {
    QVERIFY2(leftSidebar.contains(token),
             qPrintable(QStringLiteral("Prepare left sidebar lost pixel-restoration token: %1").arg(token)));
  }

  QVERIFY2(!leftSidebar.contains(QStringLiteral("ObjectList {")),
           "Default screenshot sidebar must not mount ObjectList in the left parameter column");
  QVERIFY2(!leftSidebar.contains(QStringLiteral("SliceProgress {")),
           "Default screenshot sidebar must not mount SliceProgress in the left parameter column");
  QVERIFY2(leftSidebar.contains(QStringLiteral("#303236"))
               || leftSidebar.contains(QStringLiteral("#313337"))
               || leftSidebar.contains(QStringLiteral("#323438")),
           "Prepare left sidebar palette must move toward the screenshot gray panel surface");
}

void QmlUiAuditTests::prepareFullVisualParityContract()
{
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString glToolbars = readSource(QStringLiteral("src/qml_gui/components/GLToolbars.qml"));
  const QString mainQml = readSource(QStringLiteral("src/qml_gui/main.qml"));
  const QString topbar = readSource(QStringLiteral("src/qml_gui/BBLTopbar.qml"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!glToolbars.isEmpty(), "Unable to read GLToolbars.qml");
  QVERIFY2(!mainQml.isEmpty(), "Unable to read main.qml");
  QVERIFY2(!topbar.isEmpty(), "Unable to read BBLTopbar.qml");

  const QStringList prepareTokens = {
    QStringLiteral("readonly property bool visualCompareActive"),
    QStringLiteral("readonly property int targetViewportTopInset: 58"),
    QStringLiteral("readonly property int targetViewportBottomInset: 50"),
    QStringLiteral("id: prepareVisualStatusPill"),
    QStringLiteral("anchors.bottomMargin: root.targetViewportBottomInset")
  };
  for (const QString &token : prepareTokens) {
    QVERIFY2(preparePage.contains(token),
             qPrintable(QStringLiteral("Prepare full visual parity token missing: %1").arg(token)));
  }

  const QStringList toolbarTokens = {
    QStringLiteral("readonly property int targetActionToolbarTop: 22"),
    QStringLiteral("readonly property int targetActionToolbarLeft: 598"),
    QStringLiteral("readonly property int targetRightToolbarTop: 392"),
    QStringLiteral("readonly property int targetViewControlsBottom: 42"),
    QStringLiteral("id: prepareTopActionToolbar"),
    QStringLiteral("id: prepareRightGizmoToolbar"),
    QStringLiteral("id: prepareBottomViewControls"),
    QStringLiteral("buttonSize: root.targetToolbarButtonSize")
  };
  for (const QString &token : toolbarTokens) {
    QVERIFY2(glToolbars.contains(token),
             qPrintable(QStringLiteral("Prepare GL toolbar visual parity token missing: %1").arg(token)));
  }

  const QStringList shellTokens = {
    QStringLiteral("readonly property int prepareChromeHeight: 70"),
    QStringLiteral("Layout.preferredHeight: root.prepareChromeHeight"),
    QStringLiteral("id: workflowBar"),
    QStringLiteral("id: prepareExportGcodeButton"),
    QStringLiteral("root.exportGcodeRequested()"),
    QStringLiteral("visible: false")
  };
  for (const QString &token : shellTokens) {
    QVERIFY2(mainQml.contains(token) || topbar.contains(token),
             qPrintable(QStringLiteral("Prepare shell visual parity token missing: %1").arg(token)));
  }
}

void QmlUiAuditTests::prepareRestoredControlsAreActionable()
{
  const QString glToolbars = readSource(QStringLiteral("src/qml_gui/components/GLToolbars.qml"));
  const QString topbar = readSource(QStringLiteral("src/qml_gui/BBLTopbar.qml"));
  const QString mainQml = readSource(QStringLiteral("src/qml_gui/main.qml"));
  QVERIFY2(!glToolbars.isEmpty(), "Unable to read GLToolbars.qml");
  QVERIFY2(!topbar.isEmpty(), "Unable to read BBLTopbar.qml");
  QVERIFY2(!mainQml.isEmpty(), "Unable to read main.qml");

  QVERIFY2(!glToolbars.contains(QStringLiteral("targetActionIcons.slice")),
           "Prepare GL toolbar must not generate visible placeholder buttons from a raw icon list");
  QVERIFY2(!glToolbars.contains(QStringLiteral("Unavailable in the current Prepare state")),
           "Prepare GL toolbar must not expose screenshot-only placeholder tooltips");
  QVERIFY2(!glToolbars.contains(QStringLiteral("opacity: root.targetDisabledToolOpacity")),
           "Prepare GL toolbar must not dim fake controls instead of wiring real actions");

  const QStringList requiredToolbarActions = {
    QStringLiteral("root.editorVm.deleteSelection()"),
    QStringLiteral("root.editorVm.copySelectedObjects()"),
    QStringLiteral("root.editorVm.pasteObjects()"),
    QStringLiteral("root.editorVm.mirrorSelectedObjects(0)"),
    QStringLiteral("root.editorVm.fixMeshSelected()"),
    QStringLiteral("root.editorVm.requestSelectionSettings()")
  };
  for (const QString &token : requiredToolbarActions) {
    QVERIFY2(glToolbars.contains(token),
             qPrintable(QStringLiteral("Prepare GL toolbar restored action missing: %1").arg(token)));
  }
  QVERIFY2(glToolbars.contains(QStringLiteral("root.editorVm.gizmoStatusText(mode)")),
           "Prepare right gizmo toolbar must surface backend capability reasons in tooltips");
  QVERIFY2(glToolbars.contains(QStringLiteral("availableGizmoMask & (1 << mode)")),
           "Prepare right gizmo toolbar must bind enabled state to EditorViewModel::availableGizmoMask");

  QVERIFY2(topbar.contains(QStringLiteral("enabled: backend.editorViewModel && backend.editorViewModel.canRequestSlice")),
           "Prepare top slice button must bind to EditorViewModel::canRequestSlice");
  QVERIFY2(topbar.contains(QStringLiteral("onClicked: root.sliceSinglePlateRequested()")),
           "Prepare top slice button must directly request single-plate slicing");
  QVERIFY2(topbar.contains(QStringLiteral("enabled: backend.editorViewModel && backend.editorViewModel.canExportGCode")),
           "Prepare top export button must bind to EditorViewModel::canExportGCode");
  QVERIFY2(topbar.contains(QStringLiteral("toolTipText: backend.editorViewModel ? backend.editorViewModel.exportActionHint")),
           "Prepare top export button must show backend export readiness");
  QVERIFY2(mainQml.contains(QStringLiteral("onSliceSinglePlateRequested: if (backend.editorViewModel) backend.editorViewModel.requestSlice()")),
           "main.qml must wire the topbar single-plate slice signal to EditorViewModel::requestSlice");
}

void QmlUiAuditTests::prepareRestorationMilestoneHasCleanupCoverage()
{
  const QString qrc = readSource(QStringLiteral("src/qml_gui/qml.qrc"));
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString previewPage = readSource(QStringLiteral("src/qml_gui/pages/PreviewPage.qml"));
  const QString mainQml = readSource(QStringLiteral("src/qml_gui/main.qml"));
  const QString dockableSidebar = readSource(QStringLiteral("src/qml_gui/panels/DockableSidebar.qml"));
  const QString leftSidebar = readSource(QStringLiteral("src/qml_gui/panels/LeftSidebar.qml"));
  const QString objectList = readSource(QStringLiteral("src/qml_gui/panels/ObjectList.qml"));
  const QString sliceProgress = readSource(QStringLiteral("src/qml_gui/panels/SliceProgress.qml"));
  const QString glToolbars = readSource(QStringLiteral("src/qml_gui/components/GLToolbars.qml"));
  const QString cxTextArea = readSource(QStringLiteral("src/qml_gui/controls/CxTextArea.qml"));
  const QString auditTests = readSource(QStringLiteral("tests/QmlUiAuditTests.cpp"));
  QVERIFY2(!qrc.isEmpty(), "Unable to read qml.qrc");
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!previewPage.isEmpty(), "Unable to read PreviewPage.qml");
  QVERIFY2(!mainQml.isEmpty(), "Unable to read main.qml");
  QVERIFY2(!dockableSidebar.isEmpty(), "Unable to read DockableSidebar.qml");
  QVERIFY2(!leftSidebar.isEmpty(), "Unable to read LeftSidebar.qml");
  QVERIFY2(!objectList.isEmpty(), "Unable to read ObjectList.qml");
  QVERIFY2(!sliceProgress.isEmpty(), "Unable to read SliceProgress.qml");
  QVERIFY2(!glToolbars.isEmpty(), "Unable to read GLToolbars.qml");
  QVERIFY2(!cxTextArea.isEmpty(), "Unable to read CxTextArea.qml");

  const QStringList requiredQrcPaths = {
    QStringLiteral("pages/PreparePage.qml"),
    QStringLiteral("pages/Plater.qml"),
    QStringLiteral("panels/LeftSidebar.qml"),
    QStringLiteral("panels/ObjectList.qml"),
    QStringLiteral("panels/SliceProgress.qml"),
    QStringLiteral("panels/DockableSidebar.qml"),
    QStringLiteral("components/GLToolbars.qml")
  };
  for (const QString &path : requiredQrcPaths) {
    QVERIFY2(qrc.contains(path),
             qPrintable(QStringLiteral("Restored Prepare QML path missing from qml.qrc: %1").arg(path)));
  }

  const QStringList deletedPreparePaths = {
    QStringLiteral("panels/Sidebar.qml"),
    QStringLiteral("panels/FilamentPanel.qml"),
    QStringLiteral("panels/PrintSettings.qml"),
    QStringLiteral("components/PrepareToolbar.qml"),
    QStringLiteral("components/PrepareSidebar.qml"),
    QStringLiteral("components/GLViewport.qml")
  };
  const QString qmlRoot = QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR))
                              .filePath(QStringLiteral("src/qml_gui"));
  for (const QString &path : deletedPreparePaths) {
    QVERIFY2(!qrc.contains(path),
             qPrintable(QStringLiteral("Deprecated Prepare QML path reintroduced in qml.qrc: %1").arg(path)));
    const QString fullPath = QDir(qmlRoot).filePath(path);
    QVERIFY2(!QFileInfo::exists(fullPath),
             qPrintable(QStringLiteral("Deprecated Prepare QML file reappeared on disk: %1").arg(fullPath)));
  }

  const QStringList restoredStructureTokens = {
    QStringLiteral("DockableSidebar {"),
    QStringLiteral("GLViewport {"),
    QStringLiteral("GLToolbars {"),
    QStringLiteral("id: plateBar"),
    QStringLiteral("id: transformMiniPanel")
  };
  for (const QString &token : restoredStructureTokens) {
    QVERIFY2(preparePage.contains(token),
             qPrintable(QStringLiteral("PreparePage lost restored structure token: %1").arg(token)));
  }
  QVERIFY2(dockableSidebar.contains(QStringLiteral("LeftSidebar {")),
           "DockableSidebar must wrap the restored LeftSidebar path");
  QVERIFY2(!leftSidebar.contains(QStringLiteral("ObjectList {"))
              && !leftSidebar.contains(QStringLiteral("SliceProgress {")),
           "Default screenshot sidebar must stay focused on printer, filament, process, and params");
  QVERIFY2(dockableSidebar.contains(QStringLiteral("onExportRequested: root.exportRequested()"))
              && preparePage.contains(QStringLiteral("onExportRequested: root.openExportDialog()")),
           "SliceProgress export must propagate to PreparePage.openExportDialog instead of exporting directly");
  QVERIFY2(objectList.contains(QStringLiteral("objectListStatusPill")),
           "ObjectList must keep the compact restored status pill");
  QVERIFY2(sliceProgress.contains(QStringLiteral("primaryActionEnabled"))
              && sliceProgress.contains(QStringLiteral("enabled: root.canSliceAll")),
           "SliceProgress must keep backend-gated primary and slice-all controls");
  QVERIFY2(glToolbars.contains(QStringLiteral("id: viewportActionToolbar"))
              && glToolbars.contains(QStringLiteral("id: viewportGizmoToolbar"))
              && glToolbars.contains(QStringLiteral("id: viewportViewControls")),
           "GLToolbars must keep the restored top, right, and lower-left toolbar groups");

  const QStringList removedViewportTokens = {
    QStringLiteral("id: sliceButton"),
    QStringLiteral("text: \">>\""),
    QStringLiteral("label: \"P+\""),
    QStringLiteral("label: \"AC\""),
    QStringLiteral("label: \"SVG\"")
  };
  for (const QString &token : removedViewportTokens) {
    QVERIFY2(!glToolbars.contains(token),
             qPrintable(QStringLiteral("Legacy Prepare viewport token returned: %1").arg(token)));
  }

  const QStringList requiredAuditSlots = {
    QStringLiteral("leftSidebarPresetControlsAreWiredAndHonest"),
    QStringLiteral("prepareWorkflowActionsBindCppGates"),
    QStringLiteral("prepareWorkflowPanelsMatchRestorationContract"),
    QStringLiteral("prepareViewportControlsMatchRestorationContract"),
    QStringLiteral("deletedSettingsPathsStayAbsent")
  };
  for (const QString &slotName : requiredAuditSlots) {
    QVERIFY2(auditTests.contains(slotName),
             qPrintable(QStringLiteral("Phase 78 requires existing audit slot to remain present: %1").arg(slotName)));
  }

  QVERIFY2(!cxTextArea.contains(QStringLiteral("ScrollBar.vertical")),
           "CxTextArea must not attach ScrollBar.vertical directly to TextArea; Qt only accepts Flickable/ScrollView targets");
  QVERIFY2(!dockableSidebar.contains(QStringLiteral("anchors.right: dragHandle.left")),
           "DockableSidebar titleBar must not anchor to dragHandle because it is not a sibling");
  const int plateBarStart = preparePage.indexOf(QStringLiteral("id: plateBar"));
  const int objectInfoStart = preparePage.indexOf(QStringLiteral("id: objectInfoBar"));
  QVERIFY2(plateBarStart >= 0 && objectInfoStart > plateBarStart,
           "PreparePage must keep plateBar before objectInfoBar for the final overlay audit");
  const QString plateBarBlock = preparePage.mid(plateBarStart, objectInfoStart - plateBarStart);
  const QString objectInfoBlock = preparePage.mid(objectInfoStart, 700);
  QVERIFY2(plateBarBlock.contains(QStringLiteral("parent: viewportArea"))
              && plateBarBlock.contains(QStringLiteral("anchors.bottom: parent.bottom"))
              && plateBarBlock.contains(QStringLiteral("anchors.left: parent.left"))
              && plateBarBlock.contains(QStringLiteral("anchors.right: parent.right"))
              && !plateBarBlock.contains(QStringLiteral("anchors.left: viewportArea.left"))
              && !plateBarBlock.contains(QStringLiteral("anchors.right: viewportArea.right"))
              && objectInfoBlock.contains(QStringLiteral("parent: viewportArea"))
              && objectInfoBlock.contains(QStringLiteral("anchors.bottom: parent.bottom"))
              && objectInfoBlock.contains(QStringLiteral("anchors.left: parent.left"))
              && !objectInfoBlock.contains(QStringLiteral("anchors.left: viewportArea.left")),
           "Prepare bottom overlays must be reparented into viewportArea before anchoring to viewportArea items");
  QVERIFY2(!previewPage.contains(QStringLiteral("parent.height * 0.34")),
           "Preview right panel must not use parent.height in a Layout.preferredHeight expression");
  QVERIFY2(!mainQml.contains(QStringLiteral("anchors.fill: parent\n                }\n                // Page 3"))
              && mainQml.contains(QStringLiteral("Layout.fillWidth: true\n                    Layout.fillHeight: true")),
           "StackLayout children should use Layout fill constraints instead of anchors.fill");
}

void QmlUiAuditTests::previewPageNeverReferencesSoftwareViewport()
{
  const QString previewPage = readSource(QStringLiteral("src/qml_gui/pages/PreviewPage.qml"));
  QVERIFY2(!previewPage.isEmpty(), "Unable to read PreviewPage.qml");
  QVERIFY2(!previewPage.contains(QStringLiteral("SoftwareViewport")),
           "PreviewPage.qml must not reference SoftwareViewport (GCODE-04 no-regression; "
           "the normal path is RhiViewport registered as GLViewport)");
}

// Phase 55 (GCODE-02): computePreviewDrawRanges must consult m_roleVisibility
// and return multiple draw ranges so masked spans in the middle are not drawn
// by one broad contiguous draw call.
void QmlUiAuditTests::rhiViewportRendererComputePreviewDrawRangeAppliesRoleFilter()
{
  const QString rendererSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  QVERIFY2(!rendererSource.isEmpty(), "Unable to read RhiViewportRenderer.cpp");

  QVERIFY2(rendererSource.contains(QStringLiteral("computePreviewDrawRanges")),
           "RhiViewportRenderer must define computePreviewDrawRanges (Plan 02 contract)");

  // The role-skip block must reference the per-role visibility mask. This is
  // the supporting evidence for the ViewModelSmokeTests no-repack guard.
  const int rangeStart = rendererSource.indexOf(QStringLiteral("computePreviewDrawRanges"));
  QVERIFY2(rangeStart >= 0, "computePreviewDrawRanges implementation not found");
  QVERIFY2(rendererSource.contains(QStringLiteral("m_roleVisibility")),
           "computePreviewDrawRanges must consult m_roleVisibility for render-side role filtering "
           "(Plan 02 contract)");
  QVERIFY2(rendererSource.contains(QStringLiteral("QVector<RhiViewportRenderer::PreviewDrawRange>")),
           "computePreviewDrawRanges must return a QVector of draw ranges, not one broad range");
  QVERIFY2(rendererSource.contains(QStringLiteral("ranges.append")),
           "computePreviewDrawRanges must append separate visible ranges");
  QVERIFY2(rendererSource.contains(QStringLiteral("for (const PreviewDrawRange &range : drawRanges)")),
           "render() must iterate all visible preview draw ranges");
  QVERIFY2(rendererSource.contains(QStringLiteral("cb->draw(range.vertexCount, 1, range.firstVertex)")),
           "render() must draw each visible range independently");
}

// Phase 55 (GCODE-05): GcvPackedSegment must carry a compile-time sizeof==76
// guard so the GCV1 wire format stays lockstep between PreviewViewModel packing
// and renderer unpacking. A struct layout change would silently break the
// preview at runtime; the static_assert makes it a build error.
void QmlUiAuditTests::rhiViewportRendererHasGcvPackedSegmentRoleGuard()
{
  const QString rendererSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  QVERIFY2(!rendererSource.isEmpty(), "Unable to read RhiViewportRenderer.cpp");

  QVERIFY2(rendererSource.contains(QStringLiteral("static_assert(sizeof(GcvPackedSegment) == 76")),
           "RhiViewportRenderer must contain static_assert(sizeof(GcvPackedSegment) == 76) "
           "(Plan 02 wire-format lockstep guard)");
}

// Phase 55/73 (GCODE-04/GRET-02): D3D11 default + SoftwareViewport fallback
// policy. main_qml.cpp must register RhiViewport as the QML type "GLViewport"
// on the normal QRhi/D3D11 path, and SoftwareViewport only when QRhi is
// unavailable. PreviewPage.qml must bind GLViewport (which resolves to
// RhiViewport on the default path) and never reference SoftwareViewport.
void QmlUiAuditTests::gcode04RhiViewportIsDefaultRegistrationNoSoftwareViewportInPreviewPath()
{
  const QString mainQml = readSource(QStringLiteral("src/qml_gui/main_qml.cpp"));
  const QString previewPage = readSource(QStringLiteral("src/qml_gui/pages/PreviewPage.qml"));
  QVERIFY2(!mainQml.isEmpty(), "Unable to read main_qml.cpp");
  QVERIFY2(!previewPage.isEmpty(), "Unable to read PreviewPage.qml");

  QVERIFY2(mainQml.contains(QStringLiteral("qmlRegisterType<RhiViewport>")),
           "main_qml.cpp must register RhiViewport as a QML type (GCODE-04 default path)");
  QVERIFY2(mainQml.contains(QStringLiteral("\"GLViewport\"")),
           "RhiViewport must be registered under the QML name 'GLViewport' (GCODE-04 default path)");
  // SoftwareViewport may be present as the QRhi-unavailable fallback, but the
  // retired OWZX_OPENGL path must not reintroduce a separate OpenGL viewport.
  if (mainQml.contains(QStringLiteral("SoftwareViewport")))
  {
    QVERIFY2(mainQml.contains(QStringLiteral("rhiSelection.canUseRhi")),
             "SoftwareViewport registration, if present, must be guarded behind QRhi availability");
  }
  // Belt-and-suspenders vs previewPageNeverReferencesSoftwareViewport (Plan 04):
  // the Preview page must resolve to the hardware-accelerated RhiViewport path.
  QVERIFY2(!previewPage.contains(QStringLiteral("SoftwareViewport")),
           "PreviewPage.qml must never reference SoftwareViewport (GCODE-04)");
  QVERIFY2(previewPage.contains(QStringLiteral("GLViewport")),
           "PreviewPage.qml must bind GLViewport (resolves to RhiViewport on the default "
           "D3D11 path) (GCODE-04)");
}

// -- Phase 56-03: settings dialog UI audit (Cx-only controls, qsTr, main.qml dispatch) --

// SETTINGS-01/02: the three new settings-dialog files use Cx* controls and the
// OptionRow/GroupNavSidebar pattern; TextArea is allowed inside OptionRow for
// the multiline string option (no CxTextArea covers it yet).
void QmlUiAuditTests::settingsDialogUsesOnlyCxControls()
{
  const QStringList files = {
    QStringLiteral("src/qml_gui/dialogs/SettingsDialog.qml"),
    QStringLiteral("src/qml_gui/components/OptionRow.qml"),
    QStringLiteral("src/qml_gui/components/GroupNavSidebar.qml"),
  };
  for (const auto &path : files)
  {
    const QString src = readSource(path);
    QVERIFY2(!src.isEmpty(), qPrintable(QStringLiteral("Unable to read %1").arg(path)));
    // Each file must reference at least one Cx* control (proof it uses the
    // project control set, not raw QtQuick.Controls).
    QVERIFY2(src.contains(QStringLiteral("Cx")),
             qPrintable(QStringLiteral("%1 must use Cx* controls").arg(path)));
  }
  // OptionRow must dispatch via the Cx family (CxSwitch/CxSlider/CxSpinBox/CxComboBox).
  const QString optionRow = readSource(QStringLiteral("src/qml_gui/components/OptionRow.qml"));
  QVERIFY2(optionRow.contains(QStringLiteral("CxSwitch")) && optionRow.contains(QStringLiteral("CxComboBox")) &&
               optionRow.contains(QStringLiteral("CxSpinBox")),
           "OptionRow must dispatch bool/enum/int via CxSwitch/CxComboBox/CxSpinBox");
}

// SETTINGS-01/02: no raw QtQuick.Controls control declarations in the three new
// files. TextArea is permitted inside OptionRow for the multiline string option.
void QmlUiAuditTests::settingsDialogNoRawControls()
{
  // Raw control declarations start at column 0 (after indentation) as `Name {`.
  // Cx* controls all start with "Cx", so this regex catches only the raw ones.
  const QRegularExpression rawControl(QStringLiteral("^\\s*(Switch|Slider|SpinBox|ComboBox|Button|TextField|Dialog)\\s*\\{"),
                                      QRegularExpression::MultilineOption);
  const QStringList files = {
    QStringLiteral("src/qml_gui/dialogs/SettingsDialog.qml"),
    QStringLiteral("src/qml_gui/components/GroupNavSidebar.qml"),
  };
  for (const auto &path : files)
  {
    const QString src = readSource(path);
    QVERIFY2(!src.isEmpty(), qPrintable(QStringLiteral("Unable to read %1").arg(path)));
    QVERIFY2(!rawControl.match(src).hasMatch(),
             qPrintable(QStringLiteral("%1 must not declare raw QtQuick.Controls (use Cx*)").arg(path)));
  }
  // OptionRow: TextArea is allowed (multiline string); other raw controls are not.
  const QString optionRow = readSource(QStringLiteral("src/qml_gui/components/OptionRow.qml"));
  QVERIFY2(!rawControl.match(optionRow).hasMatch(),
           "OptionRow must not declare raw Switch/Slider/SpinBox/ComboBox/Button/TextField/Dialog (TextArea allowed)");
}

// SETTINGS-01: user-visible strings in the new files use qsTr() per the
// UI-SPEC copywriting contract. We assert meaningful qsTr usage (>= 3 calls per
// file) rather than parsing every literal; full per-string visual parity is the
// Phase 58 manual UAT.
void QmlUiAuditTests::settingsDialogStringsQsTr()
{
  const QStringList files = {
    QStringLiteral("src/qml_gui/dialogs/SettingsDialog.qml"),
    QStringLiteral("src/qml_gui/components/OptionRow.qml"),
    QStringLiteral("src/qml_gui/components/GroupNavSidebar.qml"),
  };
  // SettingsDialog is label-heavy (many static strings); OptionRow and
  // GroupNavSidebar are mostly dynamic bindings (optLabel/group names) with a
  // few static strings each. Require qsTr usage proportional to that.
  static const struct { QString path; int minHits; } checks[] = {
    {QStringLiteral("src/qml_gui/dialogs/SettingsDialog.qml"), 5},
    {QStringLiteral("src/qml_gui/components/OptionRow.qml"), 1},
    {QStringLiteral("src/qml_gui/components/GroupNavSidebar.qml"), 1},
  };
  for (const auto &c : checks)
  {
    const QString src = readSource(c.path);
    QVERIFY2(!src.isEmpty(), qPrintable(QStringLiteral("Unable to read %1").arg(c.path)));
    int hits = 0;
    int from = 0;
    forever
    {
      const int idx = src.indexOf(QStringLiteral("qsTr("), from);
      if (idx < 0) break;
      ++hits;
      from = idx + 5;
    }
    QVERIFY2(hits >= c.minHits,
             qPrintable(QStringLiteral("%1 must wrap user-visible strings in qsTr() (found %2, need %3)")
                            .arg(c.path).arg(hits).arg(c.minHits)));
  }
}

// SETTINGS-01: main.qml structurally wires backend.settingsRequested to the
// three SettingsDialog show() calls (runtime open-path proof is the
// ViewModelSmokeTests::testSettingsDialogOpenFromSidebar spy test).
void QmlUiAuditTests::settingsDialogMainQmlDispatchStructural()
{
  const QString mainQml = readSource(QStringLiteral("src/qml_gui/main.qml"));
  QVERIFY2(!mainQml.isEmpty(), "Unable to read main.qml");
  QVERIFY2(mainQml.contains(QStringLiteral("function onSettingsRequested")),
           "main.qml must define an onSettingsRequested handler on backend");
  QVERIFY2(mainQml.contains(QStringLiteral("printerSettingsDialog.show()")) &&
               mainQml.contains(QStringLiteral("materialSettingsDialog.show()")) &&
               mainQml.contains(QStringLiteral("processSettingsDialog.show()")),
           "main.qml must dispatch settingsRequested to all three SettingsDialog instances");
  QVERIFY2(mainQml.contains(QStringLiteral("SettingsDialog {")),
           "main.qml must instantiate SettingsDialog");
}

void QmlUiAuditTests::settingsDialogFiltersByTabAndGroup()
{
  const QString settingsDialog = readSource(QStringLiteral("src/qml_gui/dialogs/SettingsDialog.qml"));
  const QString groupNav = readSource(QStringLiteral("src/qml_gui/components/GroupNavSidebar.qml"));
  QVERIFY2(!settingsDialog.isEmpty(), "Unable to read SettingsDialog.qml");
  QVERIFY2(!groupNav.isEmpty(), "Unable to read GroupNavSidebar.qml");

  QVERIFY2(settingsDialog.contains(QStringLiteral("filterIndicesByPage")),
           "SettingsDialog rebuildFilter must narrow option indices by active tab/page");
  QVERIFY2(settingsDialog.contains(QStringLiteral("filterIndicesByGroup")),
           "SettingsDialog rebuildFilter must narrow option indices by selected group");
  QVERIFY2(settingsDialog.contains(QStringLiteral("root.selectedGroup = qsTr(\"All\")")),
           "SettingsDialog tab switches must reset group navigation to All");
  QVERIFY2(groupNav.contains(QStringLiteral("countForGroup")),
           "GroupNavSidebar badges must count options by group, not by category");
}

void QmlUiAuditTests::settingsDialogReadOnlySaveOpensSaveAs()
{
  const QString settingsDialog = readSource(QStringLiteral("src/qml_gui/dialogs/SettingsDialog.qml"));
  QVERIFY2(!settingsDialog.isEmpty(), "Unable to read SettingsDialog.qml");

  QVERIFY2(settingsDialog.contains(QStringLiteral("function requestSaveAndMaybeClose")),
           "SettingsDialog must centralize save handling so false returns do not close the dialog");
  QVERIFY2(settingsDialog.contains(QStringLiteral("function onSaveAsRequired()")),
           "SettingsDialog must handle ConfigViewModel::saveAsRequired");
  QVERIFY2(settingsDialog.contains(QStringLiteral("saveAsDialog.open()")),
           "Read-only preset save must open SavePresetDialog");
  QVERIFY2(settingsDialog.contains(QStringLiteral("closeAfterSaveAs")),
           "Unsaved close flow must remember whether Save As should close the settings window");
  QVERIFY2(settingsDialog.contains(QStringLiteral("root.requestSaveAndMaybeClose(true)")),
           "Unsaved-dialog Save action must close only after a successful save or Save As");
}

void QmlUiAuditTests::leftSidebarParamsPanelUsesRealOptionRows()
{
  const QString sidebar = readSource(QStringLiteral("src/qml_gui/panels/LeftSidebar.qml"));
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString settingsDialog = readSource(QStringLiteral("src/qml_gui/dialogs/SettingsDialog.qml"));
  QVERIFY2(!sidebar.isEmpty(), "Unable to read LeftSidebar.qml");
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!settingsDialog.isEmpty(), "Unable to read SettingsDialog.qml");

  QVERIFY2(!sidebar.contains(QStringLiteral("参数列表暂不可用")),
           "LeftSidebar ParamsPanel must not show the old unavailable placeholder");
  QVERIFY2(sidebar.contains(QStringLiteral("OptionRow {")),
           "LeftSidebar ParamsPanel must render real ConfigOptionModel rows");
  QVERIFY2(sidebar.contains(QStringLiteral("paramsOptionModel")),
           "LeftSidebar ParamsPanel must choose an option model for the active tab");
  QVERIFY2(sidebar.contains(QStringLiteral("rebuildParamsFilter")),
           "LeftSidebar search/tab changes must rebuild the params filter");
  QVERIFY2(sidebar.contains(QStringLiteral("backend.forwardSettingsRequest(\"process\")")),
           "LeftSidebar process edit button must open the process SettingsDialog");
  QVERIFY2(sidebar.contains(QStringLiteral("backend.forwardSettingsRequest(\"printer\")"))
               && sidebar.contains(QStringLiteral("backend.forwardSettingsRequest(\"filament\")")),
           "LeftSidebar preset edit buttons must open tier-specific SettingsDialog windows");
  QVERIFY2(preparePage.contains(QStringLiteral("backend.forwardSettingsRequest(\"process\")")),
           "Prepare context menu process settings entries must open the process SettingsDialog");
  QVERIFY2(settingsDialog.contains(QStringLiteral("key: \"Other\""))
               && !settingsDialog.contains(QStringLiteral("key: \"Others\"")),
           "Process SettingsDialog tabs must use ConfigOptionModel page keys such as Other");
}

// Phase 57-02 (CLEAN-02 regression): the 7 obsolete QML files removed in Wave 1
// (4 from Phase 50 section 1.6 locked Settings cleanup + 3 legacy sidebar panels
// deferred by Phase 52) must stay deleted. Asserts qml.qrc cleanliness AND
// on-disk absence so a future regression that re-adds any of them fails CI
// deterministically. Source paths are resolved against QT_TESTCASE_SOURCEDIR
// (the repository root) so the test runs from any build directory.
void QmlUiAuditTests::deletedSettingsPathsStayAbsent()
{
  const QStringList deletedPaths = {
      QStringLiteral("pages/SettingsPage.qml"),
      QStringLiteral("pages/ConfigPage.qml"),
      QStringLiteral("components/ParamsPage.qml"),
      QStringLiteral("components/SearchDialog.qml"),
      QStringLiteral("panels/Sidebar.qml"),
      QStringLiteral("panels/FilamentPanel.qml"),
      QStringLiteral("panels/PrintSettings.qml"),
  };
  const QString qmlRoot = QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR))
                              .filePath(QStringLiteral("src/qml_gui"));
  // qml.qrc must not reference any of the deleted files. readSource resolves
  // relative to QT_TESTCASE_SOURCEDIR (the repository root) the same way the
  // other Phase 55/56 audit tests do.
  const QString qrcContent = readSource(QStringLiteral("src/qml_gui/qml.qrc"));
  QVERIFY2(!qrcContent.isEmpty(),
           "Cannot read src/qml_gui/qml.qrc");
  for (const QString &p : deletedPaths) {
    QVERIFY2(!qrcContent.contains(p),
             qPrintable(QStringLiteral("qml.qrc reintroduced deleted entry: %1").arg(p)));
  }
  // The deleted files must not exist on disk under src/qml_gui/.
  for (const QString &p : deletedPaths) {
    const QString full = QDir(qmlRoot).filePath(p);
    QVERIFY2(!QFileInfo::exists(full),
             qPrintable(QStringLiteral("Deleted QML file reappeared on disk: %1").arg(full)));
  }
}

// Phase 57-02 (CLEAN-01 regression): the 3 named routes
// (canLeaveSettingsPage / requestConfigPageExitIfNeeded /
// requestLeaveSettingsPage), their dead deferred-config-exit helpers
// (executeDeferredConfigExit / clearDeferredConfigExit), and the
// leave-settings-page pending action must stay excised from the BackendContext
// page router and ConfigViewModel. Any reintroduction fails CI here.
void QmlUiAuditTests::deletedRoutesStayAbsent()
{
  const QStringList deadTokens = {
      QStringLiteral("canLeaveSettingsPage"),
      QStringLiteral("requestConfigPageExitIfNeeded"),
      QStringLiteral("executeDeferredConfigExit"),
      QStringLiteral("clearDeferredConfigExit"),
      QStringLiteral("requestLeaveSettingsPage"),
      QStringLiteral("leave-settings-page"),
  };
  const QStringList sourceFiles = {
      QStringLiteral("src/qml_gui/BackendContext.h"),
      QStringLiteral("src/qml_gui/BackendContext.cpp"),
      QStringLiteral("src/core/viewmodels/ConfigViewModel.h"),
      QStringLiteral("src/core/viewmodels/ConfigViewModel.cpp"),
  };
  for (const QString &path : sourceFiles) {
    const QString content = readSource(path);
    QVERIFY2(!content.isEmpty(),
             qPrintable(QStringLiteral("Cannot open %1").arg(path)));
    for (const QString &token : deadTokens) {
      QVERIFY2(!content.contains(token),
               qPrintable(QStringLiteral("%1 reintroduced deleted token: %2")
                              .arg(path, token)));
    }
  }
}

QTEST_MAIN(QmlUiAuditTests)
#include "QmlUiAuditTests.moc"
