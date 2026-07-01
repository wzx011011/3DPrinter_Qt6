#include <QDir>
#include <QFile>
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
  void visiblePlaceholderSurfacesAreHonest();
  // Phase 22 (UI-3): actively guard the v3.0 Phase 17 plate-lifecycle menu wiring
  void plateContextMenuItemsWiredAndNonEmpty();
  // Phase 51-03 (SHELL-03): BBLTopbar action controls bind to the BackendContext gate properties.
  void shellActionsBindToBackendContextGates();
  // Phase 51-03 (SHELL-04): the 3 notification surfaces keep non-overlapping placement.
  void notificationSurfacesStayNonOverlapping();
  // Phase 52-03 (PREPSB-01..04): LeftSidebar + FilamentSlot bindings are
  // present and there is no silent dead UI or empty handler.
  void leftSidebarPresetControlsAreWiredAndHonest();
  // Phase 53: Prepare object, plate, and viewport actions bind to C++ gates.
  void prepareWorkflowActionsBindCppGates();

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

  QVERIFY2(mainCpp.contains(QStringLiteral("qEnvironmentVariableIsSet(\"OWZX_OPENGL\")")),
            "main_qml.cpp must gate OpenGL viewport selection on OWZX_OPENGL");
  QVERIFY2(mainCpp.contains(QStringLiteral("qputenv(\"OWZX_RHI_RENDERER\", \"auto\")")),
            "default startup must enable QRhi auto instead of the software viewport");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<RhiViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
            "default GLViewport registration must use RhiViewport when QRhi initializes");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<SoftwareViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
           "SoftwareViewport must remain registered as the QRhi fallback");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<GLViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
            "OpenGL GLViewport registration must remain available behind OWZX_OPENGL");
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
  QVERIFY2(mainCpp.contains(QStringLiteral("qEnvironmentVariableIsSet(\"OWZX_OPENGL\")")),
           "legacy OWZX_OPENGL path must stay independent from QRhi");
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

void QmlUiAuditTests::prepareViewportBindsBedAndPlateContext()
{
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString rhiViewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString rhiViewportSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.cpp"));
  const QString softwareViewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/SoftwareViewport.h"));
  const QString glViewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/GLViewport.h"));
  const QString editorHeader = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!rhiViewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!rhiViewportSource.isEmpty(), "Unable to read RhiViewport.cpp");
  QVERIFY2(!softwareViewportHeader.isEmpty(), "Unable to read SoftwareViewport.h");
  QVERIFY2(!glViewportHeader.isEmpty(), "Unable to read GLViewport.h");
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
  QVERIFY2(glViewportHeader.contains(QStringLiteral("Q_PROPERTY(QVariantList activePlateObjectIndices")),
           "GLViewport must expose activePlateObjectIndices for the legacy OpenGL registration path");
  QVERIFY2(glViewportHeader.contains(QStringLiteral("Q_PROPERTY(QVariantList meshBatchSourceObjectIndices")),
           "GLViewport must expose meshBatchSourceObjectIndices for the legacy OpenGL registration path");
  QVERIFY2(glViewportHeader.contains(QStringLiteral("Q_PROPERTY(int selectedSourceObjectIndex")),
           "GLViewport must expose selectedSourceObjectIndex for the legacy OpenGL registration path");
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
  QVERIFY2(rendererSource.contains(QStringLiteral("} else {\n        m_prepareScene.takeDirtyFlags();")),
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
  const QString vertexShader = readSource(QStringLiteral("src/qml_gui/Renderer/shaders/rhi_viewport.vert"));
  QVERIFY2(!viewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!viewportSource.isEmpty(), "Unable to read RhiViewport.cpp");
  QVERIFY2(!rendererHeader.isEmpty(), "Unable to read RhiViewportRenderer.h");
  QVERIFY2(!rendererSource.isEmpty(), "Unable to read RhiViewportRenderer.cpp");
  QVERIFY2(!vertexShader.isEmpty(), "Unable to read rhi_viewport.vert");

  QVERIFY2(rendererHeader.contains(QStringLiteral("float z")),
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

  const int rangeStart = rendererSource.indexOf(QStringLiteral("void RhiViewportRenderer::computePreviewDrawRange"));
  QVERIFY2(rangeStart >= 0, "computePreviewDrawRange implementation missing");
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
  const QString moveSlider = readSource(QStringLiteral("src/qml_gui/components/MoveSlider.qml"));
  QVERIFY2(!previewPage.isEmpty(), "Unable to read PreviewPage.qml");
  QVERIFY2(!previewHeader.isEmpty(), "Unable to read PreviewViewModel.h");
  QVERIFY2(!statsPanel.isEmpty(), "Unable to read StatsPanel.qml");
  QVERIFY2(!moveSlider.isEmpty(), "Unable to read MoveSlider.qml");

  const QStringList requiredRegions = {
    QStringLiteral("id: previewHeader"),
    QStringLiteral("id: leftPanel"),
    QStringLiteral("id: centerArea"),
    QStringLiteral("id: rightPanel"),
    QStringLiteral("id: verticalLayerRail"),
    QStringLiteral("id: moveSliderBar"),
    QStringLiteral("Components.LayerSlider"),
    QStringLiteral("Components.MoveSlider"),
    QStringLiteral("Components.StatsPanel"),
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
    QStringLiteral("root.previewVm.plateSummary"),
    QStringLiteral("root.previewVm.warningSummary"),
    QStringLiteral("root.previewVm.gcodeLines"),
    QStringLiteral("root.previewVm.currentGcodeLine"),
    QStringLiteral("root.leftPanelExpanded"),
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
  const QString softwareViewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/SoftwareViewport.h"));
  const QString glViewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/GLViewport.h"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!editorHeader.isEmpty(), "Unable to read EditorViewModel.h");
  QVERIFY2(!viewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!viewportSource.isEmpty(), "Unable to read RhiViewport.cpp");
  QVERIFY2(!rendererHeader.isEmpty(), "Unable to read RhiViewportRenderer.h");
  QVERIFY2(!rendererSource.isEmpty(), "Unable to read RhiViewportRenderer.cpp");
  QVERIFY2(!softwareViewportHeader.isEmpty(), "Unable to read SoftwareViewport.h");
  QVERIFY2(!glViewportHeader.isEmpty(), "Unable to read GLViewport.h");

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
  QVERIFY2(glViewportHeader.contains(QStringLiteral("Q_PROPERTY(int hoveredSourceObjectIndex"))
               && glViewportHeader.contains(QStringLiteral("void objectPickedSource(int sourceIndex);")),
           "GLViewport fallback must keep QML signal/property compatibility");

  QVERIFY2(viewportSource.contains(QStringLiteral("pickSourceObjectAt"))
               && viewportSource.contains(QStringLiteral("projectBoundsToScreenRect"))
               && viewportSource.contains(QStringLiteral("emit objectPickedSource")),
           "RhiViewport picking must stay in C++ with camera/projected bounds");
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

  // PREPSB-04: search box wired to filterOptionIndices (onAccepted + onTextChanged).
  QVERIFY2(sidebar.count(QStringLiteral("filterOptionIndices")) >= 2,
           "LeftSidebar search must call filterOptionIndices on both onAccepted and onTextChanged");

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

QTEST_MAIN(QmlUiAuditTests)
#include "QmlUiAuditTests.moc"
