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
  void mainRegistersSoftwareViewportByDefault();
  void mainRegistersRhiViewportOnlyBehindExplicitGate();
  void renderBenchmarkMatchesRhiBackendPolicy();
  void prepareViewportBindsBedAndPlateContext();
  void rhiViewportRendererUsesPrepareSceneDataAndDirtyUploads();
  void rhiViewportRendererUsesModelBuffersAndCameraUniforms();
  void visiblePlaceholderSurfacesAreHonest();
  // Phase 22 (UI-3): actively guard the v3.0 Phase 17 plate-lifecycle menu wiring
  void plateContextMenuItemsWiredAndNonEmpty();

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

void QmlUiAuditTests::mainRegistersSoftwareViewportByDefault()
{
  const QString mainCpp = readSource(QStringLiteral("src/qml_gui/main_qml.cpp"));
  const QString verifyScript = readSource(QStringLiteral("scripts/auto_verify_with_vcvars.ps1"));
  QVERIFY2(!mainCpp.isEmpty(), "Unable to read main_qml.cpp");
  QVERIFY2(!verifyScript.isEmpty(), "Unable to read auto_verify_with_vcvars.ps1");

  QVERIFY2(mainCpp.contains(QStringLiteral("qEnvironmentVariableIsSet(\"OWZX_OPENGL\")")),
           "main_qml.cpp must gate OpenGL viewport selection on OWZX_OPENGL");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<SoftwareViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
           "default GLViewport registration must use SoftwareViewport");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<GLViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
           "OpenGL GLViewport registration must remain available behind OWZX_OPENGL");
  QVERIFY2(!verifyScript.contains(QStringLiteral("OWZX_OPENGL")),
           "canonical startup smoke should cover the default software viewport path");
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
           "QRhi viewport selection must be behind OWZX_RHI_RENDERER");
  QVERIFY2(mainCpp.contains(QStringLiteral("qEnvironmentVariableIsSet(\"OWZX_OPENGL\")")),
           "legacy OWZX_OPENGL path must stay independent from QRhi");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<SoftwareViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
           "SoftwareViewport must remain the default/fallback GLViewport registration");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<RhiViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
           "RhiViewport must be registered under the existing OWzxGL.GLViewport type behind QRhi gate");
  QVERIFY2(!verifyScript.contains(QStringLiteral("OWZX_RHI_RENDERER")),
           "canonical verification must not enable QRhi by default");

  QVERIFY2(selectorHeader.contains(QStringLiteral("RhiBackendSelection")),
           "RhiBackendSelector must expose structured backend diagnostics");
  QVERIFY2(selectorSource.contains(QStringLiteral("Direct3D12")),
           "QRhi app selector must try Direct3D12 on Windows");
  QVERIFY2(selectorSource.contains(QStringLiteral("Direct3D11")),
           "QRhi app selector must keep Direct3D11 fallback on Windows");
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

QTEST_MAIN(QmlUiAuditTests)
#include "QmlUiAuditTests.moc"
