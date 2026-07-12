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
  void guiStartupDeepLinkArgumentsAreExtensible();
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
  void previewLayerMoveControlsAreActionableAndRendererSafe();
  void previewRoleColorModesAreHonestAndPayloadSafe();
  void previewRestorationMilestoneHasFinalCleanupCoverage();
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
  void settingsDialogRestoresPhase85ShellContract();
  void settingsOptionRowsRestorePhase86ControlContract();
  void settingsDialogReadOnlySaveOpensSaveAs();
  void settingsDialogDirtyPendingActionsOpenUnsavedGuard();
  void leftSidebarParamsPanelUsesRealOptionRows();
  void settingsRestorationMilestoneHasFinalVerificationCoverage();
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
  // Phase 90-01 (ASMSHELL-01/02, ASMROUTE-01): the Plater.qml AssembleView
  // placeholder is replaced by a real AssemblePage canvas host, the
  // CanvasAssembleView enum is registered, the renderer has an AssembleView
  // branch, navigation is wired, and CanvasAssembleView routing exists.
  void assembleViewShellReplacesPlaceholderAndRegistersCanvasHost();
  // Phase 91-01 (ASMEXPLODE-01/02): the explosion-ratio control is wired
  // (EditorViewModel property + QML slider + RhiViewport re-render trigger) and
  // the renderer applies the per-volume offset on the CanvasAssembleView branch
  // with connector guide lines gated to ratio > 1.0. Prepare/Preview guards stay
  // intact.
  void assembleViewExplosionRatioWiredAndRenderBranchAppliesOffset();
  // Phase 92-01 (ASMMEASURE-01/02): the Assembly measurement gizmo is wired:
  // GizmoAssemblyMeasure enum value exists, EditorViewModel has the
  // activability gate (AssembleView + explosion ratio ~ 1.0 + >=2 volumes) +
  // measure Q_PROPERTYs + activateAssemblyMeasureGizmo, AssemblePage has
  // Ctrl+Y + the 测量 panel bound to the measure properties, the renderer has
  // the overlay gated to gizmo mode 19 + CanvasAssembleView, and the
  // AssemblyMeasureGeometry helper exists. Prepare/Preview guards stay intact.
  void assembleViewMeasurementGizmoWiredAndOverlayRenders();
  // Phase 93-01 (ASMVERIFY-01): consolidated AssembleView-restoration
  // milestone verification. Locks: placeholder removed, AssemblePage.qml
  // present + registered, CanvasAssembleView enum, explosion-ratio wiring,
  // Assembly gizmo anchors, data pool present. Mirrors Phase 88's
  // settingsRestorationMilestoneHasFinalVerificationCoverage.
  void assembleViewRestorationMilestoneHasFinalVerificationCoverage();
  // Phase 93-01 (ASMVERIFY-01 regression): the AssembleView placeholder
  // tokens removed by Phase 90 must stay absent. Fails CI deterministically
  // if any reappear. Mirrors Phase 88's deletedSettingsPathsStayAbsent.
  void assembleViewPlaceholderArtifactsStayAbsent();
  // Phase 95-01 (THUMBCAP-01/02/03): the requestThumbnailCapture solid-color
  // stub is gone and real QRhi texture readback is wired (offscreen RT +
  // readBackTexture + render-thread request queue mirroring synchronize() +
  // queued QImage callback). Source-level only so it runs in the regression
  // ctest without launching the app.
  void rhiViewportThumbnailCaptureUsesRealReadback();

  // Phase 96-01 (THUMBWRITE-01/02/03): source-audit guard proving the 3MF
  // write-side thumbnail hooks are populated with real captured pixels
  // (qimageToThumbnailData helper + plate_thumbnail populate + thumbnail_data
  // populate + deferred markers gone). Source-level only so it runs in the
  // regression ctest. Runtime save-reload round-trip proof is routed to
  // Phase 97.
  void projectServiceWrites3mfThumbnails();
  // Phase 102-01 (WTVERIFY-01): consolidated wipe-tower source-audit lock
  // across all 8 WT-* regions from the Phase 99 gap matrix
  // (99-GAP-MATRIX.md). Each assertion names the WT-* region it locks so a
  // future regression's failure output is directly attributable to the gap-
  // matrix row. Mirrors the deterministic Phase 101 source-audit pattern
  // (QFile + QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with a
  // region-named message). Source-level only; runs in the regression ctest.
  void wipeTowerReadbackAndRenderAnchorsPresent();
  // Phase 103-01 (FIXTURE-02): the argv startup-fixture gate waits for the
  // first QQuickWindow::frameSwapped (NOT the old zero-delay timer trick) so
  // external screenshot capture is deterministic (the scene graph has rendered
  // at least one frame before open-page / load-model / open-dialog are applied).
  // Source-audit locks: frameSwapped present, the old singleShot(0 gate gone,
  // and applyStartupOpenRequests still exists. Mirrors the Phase 102
  // wipeTowerReadbackAndRenderAnchorsPresent pattern (QFile +
  // QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with a
  // FIXTURE-02-named message). Source-level only; runs in the regression ctest.
  void argvFixtureGateUsesFrameSwappedNotSingleShot();
  // Phase 104-01 (FIXTURE-01/03/04): the multi-material fixture model, the
  // argv recipe doc, and the anti-feature comment all exist on disk. Locks:
  // (a) tests/data/multi_material_fixture.3mf exists and is non-empty
  // (FIXTURE-01); (b) tests/data/fixture_recipes.md exists and contains each
  // major GUI state name (FIXTURE-03); (c) main_qml.cpp contains the anti-
  // feature comment anchor (FIXTURE-04). Mirrors the Phase 102/103 source-
  // audit pattern (QFile + QT_TESTCASE_SOURCEDIR + QString::contains +
  // QVERIFY2 with a FIXTURE-named message). Source-level only; runs in the
  // regression ctest.
  void cliFixtureRecipesAndMultiMaterialModelPresent();
  // Phase 105-01 (D3D12-01): the D3D12 debug layer is gated on the
  // OWZX_D3D12_DEBUG env flag in RhiBackendSelector.cpp (probe-path
  // enableDebugLayer) AND forwarded to Qt's QSG RHI debug mechanism in
  // main_qml.cpp (QSG_RHI_DEBUG before QGuiApplication) so Phase 106 can
  // triage the startup 0xc0000005 access violation. Source-audit locks:
  // (a) OWZX_D3D12_DEBUG appears in RhiBackendSelector.cpp; (b) enableDebugLayer
  // appears in RhiBackendSelector.cpp; (c) the enablement is conditional (env-
  // gated, not unconditionally enabled); (d) QSG_RHI_DEBUG appears in
  // main_qml.cpp BEFORE the QGuiApplication construction. Mirrors the Phase
  // 102/103/104 source-audit pattern (QFile + QT_TESTCASE_SOURCEDIR +
  // QString::contains + QVERIFY2 with a D3D12-01-named message). Source-level
  // only; runs in the regression ctest.
  void d3d12DebugLayerWiredBehindEnvFlag();
  // Phase 106-01 (D3D12-03): D3D12 stays opt-in and the default Windows
  // candidate order keeps D3D11 before D3D12 so the v3.2 startup
  // 0xc0000005 access violation cannot recur for default-`auto` users. The
  // Phase 106-01 root-cause investigation (.planning/research/
  // D3D12-CRASH-ROOT-CAUSE.md) is time-boxed per DR-04; until a confirmed
  // root cause + clean repro on the original machine land, D3D12 must NOT be
  // promoted to default. This slot locks that decision at the source level:
  // (a) defaultWindowsCandidates() keeps D3D11 before D3D12 (position-ordered
  // assertion, mirrors the Phase 105 position-order check); (b) the D3D11-first
  // comment is still present (documents WHY the order is load-bearing); (c)
  // D3D12 remains reachable only via the explicit OWZX_RHI_RENDERER=d3d12 opt-in
  // (the candidatesForRequest exact-match path). Mirrors the Phase 102/103/104/
  // 105 source-audit pattern (QFile + QT_TESTCASE_SOURCEDIR + QString::contains
  // + QVERIFY2 with a D3D12-03-named message). Source-level only; runs in the
  // regression ctest.
  void d3d12StaysOptInBehindEnvFlag();

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

void QmlUiAuditTests::guiStartupDeepLinkArgumentsAreExtensible()
{
  const QString mainCpp = readSource(QStringLiteral("src/qml_gui/main_qml.cpp"));
  QVERIFY2(!mainCpp.isEmpty(), "Unable to read main_qml.cpp");

  const QStringList requiredTokens = {
      QStringLiteral("QCommandLineParser"),
      QStringLiteral("QCommandLineOption openPageOption"),
      QStringLiteral("QCommandLineOption openDialogOption"),
      QStringLiteral("QCommandLineOption loadModelOption"),
      QStringLiteral("QCommandLineOption skipFirstRunOption"),
      QStringLiteral("QStringList modelPaths"),
      QStringLiteral("QStringLiteral(\"open-page\")"),
      QStringLiteral("QStringLiteral(\"open-dialog\")"),
      QStringLiteral("QStringLiteral(\"load-model\")"),
      QStringLiteral("QStringLiteral(\"skip-first-run\")"),
      QStringLiteral("struct StartupPageRoute"),
      QStringLiteral("struct StartupDialogRoute"),
      QStringLiteral("applyStartupOpenRequests"),
      QStringLiteral("parser.values(openDialogOption)"),
      QStringLiteral("parser.values(loadModelOption)"),
      QStringLiteral("backend.topbarImportModel(modelPath)"),
      QStringLiteral("startupSkipFirstRun"),
      QStringLiteral("&QQuickWindow::frameSwapped")
  };
  for (const QString &token : requiredTokens) {
    QVERIFY2(mainCpp.contains(token),
             qPrintable(QStringLiteral("GUI startup deep-link support missing token: %1").arg(token)));
  }

  const QStringList pageAliases = {
      QStringLiteral("QStringLiteral(\"home\")"),
      QStringLiteral("QStringLiteral(\"prepare\")"),
      QStringLiteral("QStringLiteral(\"preview\")"),
      QStringLiteral("QStringLiteral(\"project\")"),
      QStringLiteral("QStringLiteral(\"calibration\")")
  };
  for (const QString &alias : pageAliases) {
    QVERIFY2(mainCpp.contains(alias),
             qPrintable(QStringLiteral("GUI startup page registry missing alias: %1").arg(alias)));
  }

  const QStringList dialogRoutes = {
      QStringLiteral("QStringLiteral(\"settings:printer\")"),
      QStringLiteral("QStringLiteral(\"settings:filament\")"),
      QStringLiteral("QStringLiteral(\"settings:material\")"),
      QStringLiteral("QStringLiteral(\"settings:process\")"),
      QStringLiteral("QStringLiteral(\"settings:print\")"),
      QStringLiteral("backend.forwardSettingsRequest")
  };
  for (const QString &route : dialogRoutes) {
    QVERIFY2(mainCpp.contains(route),
             qPrintable(QStringLiteral("GUI startup dialog registry missing route: %1").arg(route)));
  }

  const QString mainQml = readSource(QStringLiteral("src/qml_gui/main.qml"));
  QVERIFY2(!mainQml.isEmpty(), "Unable to read main.qml");
  QVERIFY2(mainQml.contains(QStringLiteral("!startupSkipFirstRun && !backend.configWizardCompleted")),
           "GUI startup skip-first-run must suppress the wizard for this launch without persisting QSettings");
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
    QStringLiteral("void RhiViewport::setGcodeViewMode"),
    QStringLiteral("void RhiViewport::setRoleVisibility")
  };
  for (int i = 0; i < setters.size(); ++i) {
    const int start = viewportSource.indexOf(setters.at(i));
    QVERIFY2(start >= 0,
             qPrintable(QStringLiteral("Missing Preview interaction setter: %1").arg(setters.at(i))));
    int end = viewportSource.indexOf(QStringLiteral("\nvoid RhiViewport::"), start + 1);
    if (end < 0)
      end = viewportSource.size();
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

void QmlUiAuditTests::previewLayerMoveControlsAreActionableAndRendererSafe()
{
  const QString previewPage = readSource(QStringLiteral("src/qml_gui/pages/PreviewPage.qml"));
  const QString layerRail = readSource(QStringLiteral("src/qml_gui/components/PreviewLayerRail.qml"));
  const QString moveSlider = readSource(QStringLiteral("src/qml_gui/components/MoveSlider.qml"));
  const QString previewHeader = readSource(QStringLiteral("src/core/viewmodels/PreviewViewModel.h"));
  const QString viewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString viewportSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.cpp"));
  QVERIFY2(!previewPage.isEmpty(), "Unable to read PreviewPage.qml");
  QVERIFY2(!layerRail.isEmpty(), "Unable to read PreviewLayerRail.qml");
  QVERIFY2(!moveSlider.isEmpty(), "Unable to read MoveSlider.qml");
  QVERIFY2(!previewHeader.isEmpty(), "Unable to read PreviewViewModel.h");
  QVERIFY2(!viewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!viewportSource.isEmpty(), "Unable to read RhiViewport.cpp");

  QVERIFY2(previewPage.contains(QStringLiteral("Components.PreviewLayerRail")),
           "PreviewPage must use the compact PreviewLayerRail component");
  QVERIFY2(previewPage.contains(QStringLiteral("previewViewport.requestPreviewFit()")),
           "PreviewPage must expose an explicit fit-to-preview camera action");
  QVERIFY2(previewPage.contains(QStringLiteral("root.previewVm.stepCurrentMove(")),
           "Preview keyboard shortcuts must route move stepping through PreviewViewModel");

  const QStringList layerCalls = {
    QStringLiteral("root.previewVm.setLayerRange("),
    QStringLiteral("root.previewVm.jumpToLayer("),
    QStringLiteral("root.previewVm.moveLayerRange(")
  };
  for (const QString &call : layerCalls) {
    QVERIFY2(layerRail.contains(call),
             qPrintable(QStringLiteral("PreviewLayerRail missing ViewModel call: %1").arg(call)));
  }
  QVERIFY2(layerRail.contains(QStringLiteral("RangeSlider")),
           "PreviewLayerRail must expose a vertical range control, not a single-value slider");

  QVERIFY2(moveSlider.contains(QStringLiteral("root.previewVm.stepCurrentMove(")),
           "MoveSlider step buttons must use PreviewViewModel::stepCurrentMove");
  QVERIFY2(moveSlider.contains(QStringLiteral("root.previewVm.togglePlayPause()")),
           "MoveSlider must keep the playback timer routed through PreviewViewModel");
  QVERIFY2(!moveSlider.contains(QStringLiteral("root.previewVm.currentMove +"))
              && !moveSlider.contains(QStringLiteral("root.previewVm.currentMove -")),
           "MoveSlider must not duplicate move clamping arithmetic in QML");

  QVERIFY2(previewHeader.contains(QStringLiteral("Q_INVOKABLE void stepCurrentMove(int delta)")),
           "PreviewViewModel must expose stepCurrentMove to QML");
  QVERIFY2(viewportHeader.contains(QStringLiteral("Q_INVOKABLE void requestPreviewFit()")),
           "RhiViewport must expose a Preview-data fit action to QML");

  const int start = viewportSource.indexOf(QStringLiteral("void RhiViewport::requestPreviewFit"));
  QVERIFY2(start >= 0, "RhiViewport::requestPreviewFit implementation missing");
  const int end = viewportSource.indexOf(QStringLiteral("\nvoid RhiViewport::"), start + 1);
  const QString body = viewportSource.mid(start, end > start ? end - start : viewportSource.size() - start);
  QVERIFY2(body.contains(QStringLiteral("m_previewFitHint")),
           "requestPreviewFit must reuse cached Preview fit bounds");
  QVERIFY2(body.contains(QStringLiteral("update();")),
           "requestPreviewFit must schedule a redraw");
  QVERIFY2(!body.contains(QStringLiteral("m_previewData =")),
           "requestPreviewFit must not mutate Preview payload data");
}

void QmlUiAuditTests::previewRoleColorModesAreHonestAndPayloadSafe()
{
  const QString previewPage = readSource(QStringLiteral("src/qml_gui/pages/PreviewPage.qml"));
  const QString visibilityFilter = readSource(QStringLiteral("src/qml_gui/components/VisibilityFilter.qml"));
  const QString legend = readSource(QStringLiteral("src/qml_gui/components/Legend.qml"));
  const QString previewHeader = readSource(QStringLiteral("src/core/viewmodels/PreviewViewModel.h"));
  const QString previewSource = readSource(QStringLiteral("src/core/viewmodels/PreviewViewModel.cpp"));
  QVERIFY2(!previewPage.isEmpty(), "Unable to read PreviewPage.qml");
  QVERIFY2(!visibilityFilter.isEmpty(), "Unable to read VisibilityFilter.qml");
  QVERIFY2(!legend.isEmpty(), "Unable to read Legend.qml");
  QVERIFY2(!previewHeader.isEmpty(), "Unable to read PreviewViewModel.h");
  QVERIFY2(!previewSource.isEmpty(), "Unable to read PreviewViewModel.cpp");

  QVERIFY2(previewPage.contains(QStringLiteral("root.previewVm.setViewModeIndex(currentIndex)")),
           "Preview view mode combo must call PreviewViewModel::setViewModeIndex");
  QVERIFY2(previewPage.contains(QStringLiteral("root.previewVm.currentViewModeAvailable"))
              && previewPage.contains(QStringLiteral("root.previewVm.currentViewModeStatus")),
           "Preview header must surface honest availability for data-unavailable view modes");
  QVERIFY2(previewPage.contains(QStringLiteral("gcodeViewMode: root.previewVm.viewModeIndex")),
           "Renderer view mode must bind to PreviewViewModel::viewModeIndex");
  QVERIFY2(previewPage.contains(QStringLiteral("roleVisibility: root.previewVm.roleVisibilityMask")),
           "Renderer role visibility must bind to the dense mask, not UI rows");

  QVERIFY2(visibilityFilter.contains(QStringLiteral("root.previewVm.roleVisibilities")),
           "VisibilityFilter must render QML role rows from roleVisibilities");
  QVERIFY2(visibilityFilter.contains(QStringLiteral("root.previewVm.toggleRoleVisibility(roleVisibilityRow.modelData.roleIndex)")),
           "VisibilityFilter rows must call toggleRoleVisibility with the canonical role index");

  QVERIFY2(legend.contains(QStringLiteral("root.previewVm.legendType"))
              && legend.contains(QStringLiteral("root.previewVm.legendGradientMinLabel"))
              && legend.contains(QStringLiteral("root.previewVm.legendItems")),
           "Legend must use ViewModel legend semantics for discrete, gradient, and extruder modes");

  QVERIFY2(previewHeader.contains(QStringLiteral("Q_PROPERTY(bool currentViewModeAvailable READ currentViewModeAvailable NOTIFY stateChanged)"))
              && previewHeader.contains(QStringLiteral("Q_PROPERTY(QString currentViewModeStatus READ currentViewModeStatus NOTIFY stateChanged)"))
              && previewHeader.contains(QStringLiteral("Q_INVOKABLE bool viewModeAvailable(int index) const"))
              && previewHeader.contains(QStringLiteral("Q_INVOKABLE QString viewModeStatusText(int index) const")),
           "PreviewViewModel must expose view-mode availability and status APIs");
  QVERIFY2(previewSource.contains(QStringLiteral("viewModeUsesUnavailableData"))
              && previewSource.contains(QStringLiteral("VT_ActualSpeed"))
              && previewSource.contains(QStringLiteral("VT_PressureAdvance")),
           "PreviewViewModel must centrally list data-unavailable view modes");
}

void QmlUiAuditTests::previewRestorationMilestoneHasFinalCleanupCoverage()
{
  const QString previewPage = readSource(QStringLiteral("src/qml_gui/pages/PreviewPage.qml"));
  const QString qmlQrc = readSource(QStringLiteral("src/qml_gui/qml.qrc"));
  const QString layerRail = readSource(QStringLiteral("src/qml_gui/components/PreviewLayerRail.qml"));
  const QString moveSlider = readSource(QStringLiteral("src/qml_gui/components/MoveSlider.qml"));
  const QString statsPanel = readSource(QStringLiteral("src/qml_gui/components/StatsPanel.qml"));
  const QString visibilityFilter = readSource(QStringLiteral("src/qml_gui/components/VisibilityFilter.qml"));
  const QString legend = readSource(QStringLiteral("src/qml_gui/components/Legend.qml"));
  const QString previewHeader = readSource(QStringLiteral("src/core/viewmodels/PreviewViewModel.h"));
  const QString previewSource = readSource(QStringLiteral("src/core/viewmodels/PreviewViewModel.cpp"));
  const QString viewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString viewportSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.cpp"));
  const QString mainCpp = readSource(QStringLiteral("src/qml_gui/main_qml.cpp"));
  QVERIFY2(!previewPage.isEmpty(), "Unable to read PreviewPage.qml");
  QVERIFY2(!qmlQrc.isEmpty(), "Unable to read qml.qrc");
  QVERIFY2(!layerRail.isEmpty(), "Unable to read PreviewLayerRail.qml");
  QVERIFY2(!moveSlider.isEmpty(), "Unable to read MoveSlider.qml");
  QVERIFY2(!statsPanel.isEmpty(), "Unable to read StatsPanel.qml");
  QVERIFY2(!visibilityFilter.isEmpty(), "Unable to read VisibilityFilter.qml");
  QVERIFY2(!legend.isEmpty(), "Unable to read Legend.qml");
  QVERIFY2(!previewHeader.isEmpty(), "Unable to read PreviewViewModel.h");
  QVERIFY2(!previewSource.isEmpty(), "Unable to read PreviewViewModel.cpp");
  QVERIFY2(!viewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!viewportSource.isEmpty(), "Unable to read RhiViewport.cpp");
  QVERIFY2(!mainCpp.isEmpty(), "Unable to read main_qml.cpp");

  const QStringList requiredPreviewResources = {
    QStringLiteral("<file>pages/PreviewPage.qml</file>"),
    QStringLiteral("<file>components/PreviewLayerRail.qml</file>"),
    QStringLiteral("<file>components/MoveSlider.qml</file>"),
    QStringLiteral("<file>components/StatsPanel.qml</file>"),
    QStringLiteral("<file>components/VisibilityFilter.qml</file>"),
    QStringLiteral("<file>components/Legend.qml</file>"),
    QStringLiteral("<file>components/ToolPositionTooltip.qml</file>")
  };
  for (const QString &resource : requiredPreviewResources) {
    QVERIFY2(qmlQrc.contains(resource),
             qPrintable(QStringLiteral("Preview restored resource missing from qml.qrc: %1").arg(resource)));
  }

  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<RhiViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
           "Normal Preview path must register RhiViewport as GLViewport");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<SoftwareViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
           "SoftwareViewport must remain an explicit QRhi-unavailable fallback");
  QVERIFY2(previewPage.contains(QStringLiteral("GLViewport {")),
           "PreviewPage must instantiate the registered GLViewport type");
  QVERIFY2(!previewPage.contains(QStringLiteral("SoftwareViewport")),
           "PreviewPage must not instantiate SoftwareViewport in the normal path");

  const QStringList requiredPreviewBindings = {
    QStringLiteral("previewData: root.previewVm.gcodePreviewData"),
    QStringLiteral("layerMin: root.previewVm.currentLayerMin"),
    QStringLiteral("layerMax: root.previewVm.currentLayerMax"),
    QStringLiteral("moveEnd: root.previewVm.currentMove"),
    QStringLiteral("showTravelMoves: root.previewVm.showTravelMoves"),
    QStringLiteral("roleVisibility: root.previewVm.roleVisibilityMask"),
    QStringLiteral("showBed: root.previewVm.showBed"),
    QStringLiteral("showMarker: root.previewVm.showMarker"),
    QStringLiteral("gcodeViewMode: root.previewVm.viewModeIndex"),
    QStringLiteral("markerX: root.previewVm.toolX"),
    QStringLiteral("markerY: root.previewVm.toolY"),
    QStringLiteral("markerZ: root.previewVm.toolZ")
  };
  for (const QString &binding : requiredPreviewBindings) {
    QVERIFY2(previewPage.contains(binding),
             qPrintable(QStringLiteral("PreviewPage missing restored renderer binding: %1").arg(binding)));
  }

  QVERIFY2(previewPage.contains(QStringLiteral("Components.PreviewLayerRail"))
              && previewPage.contains(QStringLiteral("Components.MoveSlider"))
              && previewPage.contains(QStringLiteral("Components.StatsPanel"))
              && previewPage.contains(QStringLiteral("Components.VisibilityFilter"))
              && previewPage.contains(QStringLiteral("Components.Legend"))
              && previewPage.contains(QStringLiteral("Components.ToolPositionTooltip")),
           "PreviewPage must use the restored Preview component set");
  QVERIFY2(previewPage.contains(QStringLiteral("root.previewVm.currentViewModeAvailable"))
              && previewPage.contains(QStringLiteral("root.previewVm.currentViewModeStatus")),
           "PreviewPage must surface honest view-mode availability state");
  QVERIFY2(previewPage.contains(QStringLiteral("previewViewport.requestPreviewFit()")),
           "PreviewPage fit action must use the Preview-data camera fit path");
  const QRegularExpression rawSliderRegex(QStringLiteral("(^|\\n)\\s*Slider\\s*\\{"));
  QVERIFY2(!previewPage.contains(QStringLiteral("verticalLayerSlider"))
              && !rawSliderRegex.match(previewPage).hasMatch(),
           "PreviewPage must not retain the replaced simple layer slider path");

  const QStringList layerControlCalls = {
    QStringLiteral("root.previewVm.setLayerRange("),
    QStringLiteral("root.previewVm.jumpToLayer("),
    QStringLiteral("root.previewVm.moveLayerRange(")
  };
  for (const QString &call : layerControlCalls) {
    QVERIFY2(layerRail.contains(call),
             qPrintable(QStringLiteral("PreviewLayerRail missing actionable ViewModel call: %1").arg(call)));
  }
  QVERIFY2(layerRail.contains(QStringLiteral("RangeSlider")),
           "PreviewLayerRail must keep the range control restored in Phase 81");

  const QStringList moveControlCalls = {
    QStringLiteral("root.previewVm.stepCurrentMove("),
    QStringLiteral("root.previewVm.setCurrentMove("),
    QStringLiteral("root.previewVm.togglePlayPause()")
  };
  for (const QString &call : moveControlCalls) {
    QVERIFY2(moveSlider.contains(call),
             qPrintable(QStringLiteral("MoveSlider missing actionable ViewModel call: %1").arg(call)));
  }
  QVERIFY2(!moveSlider.contains(QStringLiteral("root.previewVm.currentMove +"))
              && !moveSlider.contains(QStringLiteral("root.previewVm.currentMove -")),
           "MoveSlider must not reintroduce duplicated QML move arithmetic");

  QVERIFY2(statsPanel.contains(QStringLiteral("root.previewVm.setStealthMode(checked)"))
              && statsPanel.contains(QStringLiteral("root.previewVm.setShowTravelMoves(checked)"))
              && statsPanel.contains(QStringLiteral("root.previewVm.setShowBed(checked)"))
              && statsPanel.contains(QStringLiteral("root.previewVm.setShowMarker(checked)")),
           "Preview stats controls must remain backed by invokable ViewModel setters");
  QVERIFY2(visibilityFilter.contains(QStringLiteral("root.previewVm.toggleRoleVisibility(roleVisibilityRow.modelData.roleIndex)")),
           "Role visibility controls must remain actionable");
  QVERIFY2(legend.contains(QStringLiteral("root.previewVm.legendItems"))
              && legend.contains(QStringLiteral("root.previewVm.legendGradientMinLabel"))
              && legend.contains(QStringLiteral("root.previewVm.legendGradientMaxLabel")),
           "Preview legend must remain ViewModel-backed");

  QVERIFY2(previewHeader.contains(QStringLiteral("Q_INVOKABLE void stepCurrentMove(int delta)"))
              && previewHeader.contains(QStringLiteral("Q_PROPERTY(QVariantList roleVisibilityMask READ roleVisibilityMask NOTIFY stateChanged)"))
              && previewHeader.contains(QStringLiteral("Q_PROPERTY(bool currentViewModeAvailable READ currentViewModeAvailable NOTIFY stateChanged)")),
           "PreviewViewModel public API must keep restored Preview controls callable from QML");
  QVERIFY2(previewSource.contains(QStringLiteral("viewModeUsesUnavailableData"))
              && previewSource.contains(QStringLiteral("roleVisibilityMask() const")),
           "PreviewViewModel implementation must keep color availability and role mask semantics centralized");
  QVERIFY2(viewportHeader.contains(QStringLiteral("Q_INVOKABLE void requestPreviewFit()")),
           "RhiViewport must keep the Preview fit action callable from QML");

  const int fitStart = viewportSource.indexOf(QStringLiteral("void RhiViewport::requestPreviewFit"));
  QVERIFY2(fitStart >= 0, "RhiViewport::requestPreviewFit implementation missing");
  const int fitEnd = viewportSource.indexOf(QStringLiteral("\nvoid RhiViewport::"), fitStart + 1);
  const QString fitBody = viewportSource.mid(fitStart, fitEnd > fitStart ? fitEnd - fitStart : viewportSource.size() - fitStart);
  QVERIFY2(fitBody.contains(QStringLiteral("m_previewFitHint"))
              && fitBody.contains(QStringLiteral("m_camera.fitView")),
           "requestPreviewFit must fit against cached Preview bounds");
  QVERIFY2(!fitBody.contains(QStringLiteral("m_previewData =")),
           "requestPreviewFit must not clear or replace Preview payload data");

  const QStringList restoredSurfaces = {
    previewPage,
    layerRail,
    moveSlider,
    statsPanel,
    visibilityFilter,
    legend
  };
  for (const QString &surface : restoredSurfaces) {
    QVERIFY2(!surface.contains(QStringLiteral("TODO"))
                && !surface.contains(QStringLiteral("placeholder"))
                && !surface.contains(QStringLiteral("reserved (")),
             "Restored Preview QML surfaces must not expose placeholder markers");
  }
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

  QVERIFY2(topbar.contains(QStringLiteral("function selectWorkflowTab(tab)"))
              && topbar.contains(QStringLiteral("backend.requestSelectTab(tab.pos)")),
           "Workflow tabs must route through a shared backend.requestSelectTab helper");
  QVERIFY2(topbar.contains(QStringLiteral("delegate: Button"))
              && topbar.contains(QStringLiteral("Accessible.name: workflowTab.modelData.label"))
              && topbar.contains(QStringLiteral("onClicked: root.selectWorkflowTab(workflowTab.modelData)")),
           "Workflow tabs must be real accessible buttons wired to the tab-selection helper");
  QVERIFY2(topbar.contains(QStringLiteral("activeFocusOnTab: true"))
              && topbar.contains(QStringLiteral("Qt.Key_Space")),
           "Workflow tabs must be keyboard-operable");

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
  QVERIFY2(!sidebar.contains(QStringLiteral("presetActionBlocker(2, root.configVm.currentPrinterPreset, \"rename\")"))
              && !sidebar.contains(QStringLiteral("presetActionBlocker(1, root.configVm.currentFilamentPreset, \"rename\")")),
           "LeftSidebar settings entry buttons must stay clickable even when a preset cannot be renamed");

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
    QStringLiteral("anchors.bottomMargin: root.targetViewportBottomInset"),
    QStringLiteral("function thumbnailSource(data)")
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
    QStringLiteral("id: titleToolRow"),
    QStringLiteral("id: workflowBar"),
    QStringLiteral("id: prepareExportGcodeButton"),
    QStringLiteral("root.exportGcodeRequested()"),
    QStringLiteral("visible: false")
  };
  for (const QString &token : shellTokens) {
    QVERIFY2(mainQml.contains(token) || topbar.contains(token),
             qPrintable(QStringLiteral("Prepare shell visual parity token missing: %1").arg(token)));
  }
  QVERIFY2(topbar.contains(QStringLiteral("id: titleToolRow\n            anchors.left: parent.left\n            anchors.right: parent.right\n            anchors.top: parent.top\n            height: 36"))
              && !topbar.contains(QStringLiteral("id: titleToolRow\n            anchors.fill: parent")),
           "BBLTopbar title tools must be constrained to the 36px title row so workflow tabs cannot overlap them");
  QVERIFY2(topbar.contains(QStringLiteral("anchors.topMargin: 36"))
              && topbar.contains(QStringLiteral("height: 34")),
           "BBLTopbar workflowBar must remain below the 36px title row");
  QVERIFY2(preparePage.contains(QStringLiteral("readonly property int prepareBottomViewControlsBottomMargin"))
              && preparePage.contains(QStringLiteral("viewControlsBottomMargin: root.prepareBottomViewControlsBottomMargin"))
              && glToolbars.contains(QStringLiteral("property int viewControlsBottomMargin"))
              && glToolbars.contains(QStringLiteral("anchors.bottomMargin: root.viewControlsBottomMargin")),
           "Prepare lower-left view controls must use a caller-provided safe bottom margin instead of overlapping plate thumbnails");
  QVERIFY2(preparePage.contains(QStringLiteral("data.indexOf(\"data:image/\") === 0 ? data"))
              && !preparePage.contains(QStringLiteral("\"data:image/png;base64,\" + glThumb")),
           "Prepare plate thumbnails must not prepend a data URL prefix when GL already returned a data URL");
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
  // OptionRow must dispatch via the Cx family (CxCheckBox/CxSpinBox/CxComboBox).
  const QString optionRow = readSource(QStringLiteral("src/qml_gui/components/OptionRow.qml"));
  QVERIFY2(optionRow.contains(QStringLiteral("CxCheckBox")) && optionRow.contains(QStringLiteral("CxComboBox")) &&
               optionRow.contains(QStringLiteral("CxSpinBox")),
           "OptionRow must dispatch bool/enum/int via CxCheckBox/CxComboBox/CxSpinBox");
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

void QmlUiAuditTests::settingsDialogRestoresPhase85ShellContract()
{
  const QString settingsDialog = readSource(QStringLiteral("src/qml_gui/dialogs/SettingsDialog.qml"));
  QVERIFY2(!settingsDialog.isEmpty(), "Unable to read SettingsDialog.qml");

  const QStringList restoredLabels = {
      QStringLiteral("qsTr(\"打印机设置\")"),
      QStringLiteral("qsTr(\"材料设置\")"),
      QStringLiteral("qsTr(\"工艺设置\")"),
      QStringLiteral("qsTr(\"基础信息\")"),
      QStringLiteral("qsTr(\"打印机G-code\")"),
      QStringLiteral("qsTr(\"材料\")"),
      QStringLiteral("qsTr(\"挤出机\")"),
      QStringLiteral("qsTr(\"移动能力\")"),
      QStringLiteral("qsTr(\"注释\")"),
      QStringLiteral("qsTr(\"耗材丝\")"),
      QStringLiteral("qsTr(\"冷却\")"),
      QStringLiteral("qsTr(\"参数覆盖\")"),
      QStringLiteral("qsTr(\"高级\")"),
      QStringLiteral("qsTr(\"依赖\")"),
      QStringLiteral("qsTr(\"质量\")"),
      QStringLiteral("qsTr(\"强度\")"),
      QStringLiteral("qsTr(\"速度\")"),
      QStringLiteral("qsTr(\"支撑\")"),
      QStringLiteral("qsTr(\"底板\")"),
      QStringLiteral("qsTr(\"回抽\")"),
      QStringLiteral("qsTr(\"其他\")"),
  };
  for (const QString &label : restoredLabels) {
    QVERIFY2(settingsDialog.contains(label),
             qPrintable(QStringLiteral("SettingsDialog missing restored Phase 85 label: %1").arg(label)));
  }

  const QStringList oldShellTokens = {
      QStringLiteral("selectedGroup"),
      QStringLiteral("filterIndicesByGroup"),
      QStringLiteral("GroupNavSidebar {"),
      QStringLiteral("topSearchField"),
      QStringLiteral("text: qsTr(\"Save\")"),
      QStringLiteral("text: qsTr(\"Save As...\")"),
      QStringLiteral("closeMouseArea"),
  };
  for (const QString &token : oldShellTokens) {
    QVERIFY2(!settingsDialog.contains(token),
             qPrintable(QStringLiteral("SettingsDialog still contains off-design Phase 85 shell token: %1").arg(token)));
  }

  QVERIFY2(settingsDialog.contains(QStringLiteral("width: 736"))
               && settingsDialog.contains(QStringLiteral("height: 593"))
               && settingsDialog.contains(QStringLiteral("Qt.NonModal")),
           "SettingsDialog must keep the screenshot-sized non-modal ApplicationWindow shell");
  QVERIFY2(settingsDialog.contains(QStringLiteral("filterIndicesByPage")),
           "SettingsDialog rebuildFilter must narrow option indices by active tab/page");
  QVERIFY2(settingsDialog.contains(QStringLiteral("CxIconButton"))
               && settingsDialog.contains(QStringLiteral("searchExpanded"))
               && settingsDialog.contains(QStringLiteral("root.requestSaveAndMaybeClose(false)"))
               && settingsDialog.contains(QStringLiteral("saveAsDialog.open()"))
               && settingsDialog.contains(QStringLiteral("CxSwitch"))
               && settingsDialog.contains(QStringLiteral("advancedMode")),
           "SettingsDialog compact top-row actions must keep save, save-as, search, and advanced-mode wiring");
}

void QmlUiAuditTests::settingsOptionRowsRestorePhase86ControlContract()
{
  const QString optionRow = readSource(QStringLiteral("src/qml_gui/components/OptionRow.qml"));
  const QString settingsDialog = readSource(QStringLiteral("src/qml_gui/dialogs/SettingsDialog.qml"));
  QVERIFY2(!optionRow.isEmpty(), "Unable to read OptionRow.qml");
  QVERIFY2(!settingsDialog.isEmpty(), "Unable to read SettingsDialog.qml");

  const QStringList sectionTokens = {
      QStringLiteral("sectionIconRail"),
      QStringLiteral("sectionDivider"),
      QStringLiteral("sectionGlyph"),
      QStringLiteral("Theme.accent")
  };
  for (const QString &token : sectionTokens) {
    QVERIFY2(optionRow.contains(token),
             qPrintable(QStringLiteral("OptionRow missing Phase 86 section-header token: %1").arg(token)));
  }
  QVERIFY2(!optionRow.contains(QStringLiteral("color: root.compact ? \"transparent\" : Theme.bgSurface")),
           "Phase 86 section headers must not keep the old plain text/card header branch");

  const QStringList typedControlTokens = {
      QStringLiteral("CxCheckBox"),
      QStringLiteral("CxSpinBox"),
      QStringLiteral("CxComboBox"),
      QStringLiteral("CxTextField"),
      QStringLiteral("CxTextArea"),
      QStringLiteral("optionModel.setValue(root.optIdx"),
      QStringLiteral("readonly property string oSidetext"),
      QStringLiteral("readonly property string displayUnit"),
      QStringLiteral("optSidetext(root.optIdx)")
  };
  for (const QString &token : typedControlTokens) {
    QVERIFY2(optionRow.contains(token),
             qPrintable(QStringLiteral("OptionRow missing Phase 86 typed-control token: %1").arg(token)));
  }

  const QStringList rangeAndColorTokens = {
      QStringLiteral("readonly property bool isRangeLike"),
      QStringLiteral("rangeCluster"),
      QStringLiteral("rangeMinLabel"),
      QStringLiteral("rangeMaxLabel"),
      QStringLiteral("rangeMinEditor"),
      QStringLiteral("rangeMaxEditor"),
      QStringLiteral("readonly property bool isColorLike"),
      QStringLiteral("colorSwatchButton"),
      QStringLiteral("colorSwatch")
  };
  for (const QString &token : rangeAndColorTokens) {
    QVERIFY2(optionRow.contains(token),
             qPrintable(QStringLiteral("OptionRow missing Phase 86 range/color token: %1").arg(token)));
  }

  const QStringList stateTokens = {
      QStringLiteral("metadataLane"),
      QStringLiteral("dirtyBadge"),
      QStringLiteral("sourceBadge"),
      QStringLiteral("readOnlyBadge"),
      QStringLiteral("nullableBadge"),
      QStringLiteral("vectorBadge"),
      QStringLiteral("boundsBadge"),
      QStringLiteral("readonly property int metadataLaneWidth"),
      QStringLiteral("readonly property int controlColumnWidth")
  };
  for (const QString &token : stateTokens) {
    QVERIFY2(optionRow.contains(token),
             qPrintable(QStringLiteral("OptionRow missing fixed state-indicator token: %1").arg(token)));
  }

  QVERIFY2(settingsDialog.contains(QStringLiteral("showGroupHeader: optDelegate.showGroupHeader"))
               && settingsDialog.contains(QStringLiteral("oGroup: optDelegate.optGroup"))
               && settingsDialog.contains(QStringLiteral("compactLabelWidth:"))
               && settingsDialog.contains(QStringLiteral("compactFieldWidth:"))
               && settingsDialog.contains(QStringLiteral("compactEnumWidth:"))
               && settingsDialog.contains(QStringLiteral("valueSource:")),
           "SettingsDialog must continue passing group, compact width, and value-source metadata into OptionRow");
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
  QVERIFY2(settingsDialog.contains(QStringLiteral("root.requestSaveAndMaybeClose(root.closeAfterUnsavedResolution)")),
           "Unsaved-dialog Save action must close only for close-guard flows");
}

void QmlUiAuditTests::settingsDialogDirtyPendingActionsOpenUnsavedGuard()
{
  const QString settingsDialog = readSource(QStringLiteral("src/qml_gui/dialogs/SettingsDialog.qml"));
  QVERIFY2(!settingsDialog.isEmpty(), "Unable to read SettingsDialog.qml");

  const QStringList requiredTokens = {
      QStringLiteral("function openUnsavedChangesGuard"),
      QStringLiteral("property bool closeAfterUnsavedResolution"),
      QStringLiteral("function onPendingUnsavedChangesRequested()"),
      QStringLiteral("root.openUnsavedChangesGuard(false)"),
      QStringLiteral("root.openUnsavedChangesGuard(true)"),
      QStringLiteral("root.configVm.requestCancelPendingChanges()"),
      QStringLiteral("root.configVm.requestDiscardPendingChanges()"),
      QStringLiteral("if (root.closeAfterUnsavedResolution)")
  };
  for (const QString &token : requiredTokens) {
    QVERIFY2(settingsDialog.contains(token),
             qPrintable(QStringLiteral("SettingsDialog missing Phase 87 dirty-pending token: %1").arg(token)));
  }

  QVERIFY2(settingsDialog.contains(QStringLiteral("target: root.configVm")),
           "SettingsDialog must connect directly to ConfigViewModel signals");
  QVERIFY2(settingsDialog.contains(QStringLiteral("root.requestSaveAndMaybeClose(root.closeAfterUnsavedResolution)")),
           "Save from the unsaved dialog must close only for close-guard flows, not preset-switch flows");
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
  QVERIFY2(sidebar.contains(QStringLiteral("onActionTriggered: backend.forwardSettingsRequest(\"printer\")"))
              && !sidebar.contains(QStringLiteral("onActionTriggered: backend.showConfigWizard()")),
           "LeftSidebar printer settings header must open printer settings, not the first-run config wizard");
  QVERIFY2(!sidebar.contains(QStringLiteral("presetActionBlocker(2, root.configVm.currentPrinterPreset, \"rename\")"))
              && !sidebar.contains(QStringLiteral("presetActionBlocker(1, root.configVm.currentFilamentPreset, \"rename\")")),
           "LeftSidebar printer and filament settings entry buttons must not be disabled by preset rename blockers");
  QVERIFY2(preparePage.contains(QStringLiteral("backend.forwardSettingsRequest(\"process\")")),
           "Prepare context menu process settings entries must open the process SettingsDialog");
  QVERIFY2(settingsDialog.contains(QStringLiteral("key: \"Other\""))
               && !settingsDialog.contains(QStringLiteral("key: \"Others\"")),
           "Process SettingsDialog tabs must use ConfigOptionModel page keys such as Other");
}

void QmlUiAuditTests::settingsRestorationMilestoneHasFinalVerificationCoverage()
{
  const QString qrc = readSource(QStringLiteral("src/qml_gui/qml.qrc"));
  const QString mainQml = readSource(QStringLiteral("src/qml_gui/main.qml"));
  const QString audits = readSource(QStringLiteral("tests/QmlUiAuditTests.cpp"));
  QVERIFY2(!qrc.isEmpty(), "Unable to read qml.qrc");
  QVERIFY2(!mainQml.isEmpty(), "Unable to read main.qml");
  QVERIFY2(!audits.isEmpty(), "Unable to read QmlUiAuditTests.cpp");

  const QStringList restoredResourceEntries = {
      QStringLiteral("        <file>dialogs/SettingsDialog.qml</file>"),
      QStringLiteral("        <file>dialogs/SavePresetDialog.qml</file>"),
      QStringLiteral("        <file>dialogs/UnsavedChangesDialog.qml</file>"),
      QStringLiteral("        <file>components/OptionRow.qml</file>"),
      QStringLiteral("        <file>components/GroupNavSidebar.qml</file>")
  };
  for (const QString &entry : restoredResourceEntries) {
    QVERIFY2(qrc.contains(entry),
             qPrintable(QStringLiteral("qml.qrc missing normalized settings resource entry: %1").arg(entry.trimmed())));
  }

  QVERIFY2(mainQml.contains(QStringLiteral("printerSettingsDialog.show()"))
               && mainQml.contains(QStringLiteral("materialSettingsDialog.show()"))
               && mainQml.contains(QStringLiteral("processSettingsDialog.show()")),
           "Final settings milestone must keep printer/material/process dialog dispatch");
  const QRegularExpression settingsDialogInstancePattern(QStringLiteral("(^|\\n)\\s+SettingsDialog \\{"));
  int settingsDialogInstanceCount = 0;
  auto instanceMatches = settingsDialogInstancePattern.globalMatch(mainQml);
  while (instanceMatches.hasNext()) {
    instanceMatches.next();
    ++settingsDialogInstanceCount;
  }
  QCOMPARE(settingsDialogInstanceCount, 3);

  const QStringList auditAnchors = {
      QStringLiteral("settingsDialogRestoresPhase85ShellContract"),
      QStringLiteral("settingsOptionRowsRestorePhase86ControlContract"),
      QStringLiteral("settingsDialogDirtyPendingActionsOpenUnsavedGuard"),
      QStringLiteral("deletedSettingsPathsStayAbsent"),
      QStringLiteral("deletedRoutesStayAbsent")
  };
  for (const QString &anchor : auditAnchors) {
    QVERIFY2(audits.contains(anchor),
             qPrintable(QStringLiteral("Final settings milestone missing audit anchor: %1").arg(anchor)));
  }
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

void QmlUiAuditTests::assembleViewShellReplacesPlaceholderAndRegistersCanvasHost()
{
  // Phase 90-01: the Plater.qml AssembleView placeholder is replaced by a real
  // AssemblePage canvas host, the CanvasAssembleView=2 enum is registered in
  // RhiViewport, the renderer has an AssembleView mesh-render branch, the
  // BBLTopbar navigation toggle is wired, AssemblePage is in qml.qrc, and
  // CanvasAssembleView routing exists in BackendContext/EditorViewModel.
  const QString plater = readSource(QStringLiteral("src/qml_gui/pages/Plater.qml"));
  const QString assemblePage = readSource(QStringLiteral("src/qml_gui/pages/AssemblePage.qml"));
  const QString rhiViewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString rhiViewportRenderer = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  const QString topbar = readSource(QStringLiteral("src/qml_gui/BBLTopbar.qml"));
  const QString qmlQrc = readSource(QStringLiteral("src/qml_gui/qml.qrc"));
  const QString backendContext = readSource(QStringLiteral("src/qml_gui/BackendContext.cpp"));
  const QString editorHeader = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString editorSource = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  QVERIFY2(!plater.isEmpty(), "Unable to read Plater.qml");
  QVERIFY2(!assemblePage.isEmpty(), "Unable to read AssemblePage.qml");
  QVERIFY2(!rhiViewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!rhiViewportRenderer.isEmpty(), "Unable to read RhiViewportRenderer.cpp");
  QVERIFY2(!topbar.isEmpty(), "Unable to read BBLTopbar.qml");
  QVERIFY2(!qmlQrc.isEmpty(), "Unable to read qml.qrc");
  QVERIFY2(!backendContext.isEmpty(), "Unable to read BackendContext.cpp");
  QVERIFY2(!editorHeader.isEmpty(), "Unable to read EditorViewModel.h");
  QVERIFY2(!editorSource.isEmpty(), "Unable to read EditorViewModel.cpp");

  // (1) Placeholder removed (ASM-SHELL / ASMSHELL-01).
  QVERIFY2(!plater.contains(QStringLiteral("\u88c5\u914d\u89c6\u56fe\u6682\u4e0d\u53ef\u7528")),
           "Plater.qml must not contain the 'assemble view unavailable' placeholder text");
  QVERIFY2(!plater.contains(QStringLiteral("id: assembleSlot")),
           "Plater.qml must not contain the placeholder assembleSlot Item");
  QVERIFY2(!plater.contains(QStringLiteral("Out of Scope\u3001\u4ec5\u4fdd\u7559\u679a\u4e3e\u5165\u53e3")),
           "Plater.qml must not contain the v2.0 Out-of-Scope comment");

  // (2) AssemblePage exists and is a real canvas host (ASM-SHELL / ASMSHELL-02).
  QVERIFY2(assemblePage.contains(QStringLiteral("canvasType: GLViewport.CanvasAssembleView")),
           "AssemblePage must instantiate GLViewport with canvasType CanvasAssembleView");
  QVERIFY2(assemblePage.contains(QStringLiteral("meshData: root.editorVm")),
           "AssemblePage must bind meshData from the shared editorVm");
  QVERIFY2(assemblePage.contains(QStringLiteral("LeftSidebar")),
           "AssemblePage must reuse the LeftSidebar component");

  // (3) Plater instantiates AssemblePage (ASM-SHELL).
  QVERIFY2(plater.contains(QStringLiteral("AssemblePage")),
           "Plater.qml must instantiate AssemblePage");
  QVERIFY2(plater.contains(QStringLiteral("vmAssembleView: 2")),
           "Plater.qml must preserve the vmAssembleView: 2 routing anchor");
  QVERIFY2(plater.contains(QStringLiteral("visible: root.viewMode === root.vmAssembleView")),
           "Plater.qml must gate the AssemblePage slot on viewMode === vmAssembleView");

  // (4) CanvasAssembleView enum registered (ASMSHELL-02).
  QVERIFY2(rhiViewportHeader.contains(QStringLiteral("CanvasAssembleView = 2")),
           "RhiViewport.h must declare CanvasAssembleView = 2 in the CanvasType enum");

  // (5) Renderer has AssembleView branch; Preview guards intact
  //     (ASMSHELL-02 / ASMROUTE-01).
  QVERIFY2(rhiViewportRenderer.contains(QStringLiteral("CanvasAssembleView")),
           "RhiViewportRenderer.cpp must reference CanvasAssembleView in the render path");
  QVERIFY2(rhiViewportRenderer.contains(QStringLiteral("m_canvasType == RhiViewport::CanvasPreview")),
           "RhiViewportRenderer.cpp must keep the CanvasPreview-only guards unchanged");

  // (6) Navigation toggle wired (ASM-NAVIGATION / ASMSHELL-01).
  QVERIFY2(topbar.contains(QStringLiteral("backend.requestChangeViewMode(backend.vmAssembleView)")),
           "BBLTopbar must wire the AssembleView view-mode toggle");

  // (7) qml.qrc registers AssemblePage (ASM-SHELL).
  QVERIFY2(qmlQrc.contains(QStringLiteral("pages/AssemblePage.qml")),
           "qml.qrc must register pages/AssemblePage.qml");

  // (8) Routing branches present (ASMROUTE-01).
  QVERIFY2(backendContext.contains(QStringLiteral("CanvasAssembleView"))
               || backendContext.contains(QStringLiteral("ViewMode::AssembleView")),
           "BackendContext.cpp must carry a CanvasAssembleView/AssembleView routing branch");
  QVERIFY2(editorHeader.contains(QStringLiteral("activeCanvasType"))
               || editorSource.contains(QStringLiteral("activeCanvasType")),
           "EditorViewModel must expose an active-canvas-type routing surface");
}

void QmlUiAuditTests::assembleViewExplosionRatioWiredAndRenderBranchAppliesOffset()
{
  // Phase 91-01 (ASMEXPLODE-01/02): the explosion-ratio Q_PROPERTY exists on
  // EditorViewModel (default 1.0), the AssemblePage 爆炸比例 slider is bound to
  // editorVm.explosionRatio with a reset, the RhiViewport re-render trigger is
  // wired, the renderer applies the per-volume offset on the CanvasAssembleView
  // branch, and connector guide lines render when ratio > 1.0. Preview guards
  // stay intact.
  const QString editorHeader = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString editorSource = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString assemblePage = readSource(QStringLiteral("src/qml_gui/pages/AssemblePage.qml"));
  const QString rhiViewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString rhiViewportSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.cpp"));
  const QString rhiViewportRenderer = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  QVERIFY2(!editorHeader.isEmpty(), "Unable to read EditorViewModel.h");
  QVERIFY2(!editorSource.isEmpty(), "Unable to read EditorViewModel.cpp");
  QVERIFY2(!assemblePage.isEmpty(), "Unable to read AssemblePage.qml");
  QVERIFY2(!rhiViewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!rhiViewportSource.isEmpty(), "Unable to read RhiViewport.cpp");
  QVERIFY2(!rhiViewportRenderer.isEmpty(), "Unable to read RhiViewportRenderer.cpp");

  // (1) EditorViewModel explosionRatio Q_PROPERTY + reset (ASMEXPLODE-01).
  QVERIFY2(editorHeader.contains(QStringLiteral("Q_PROPERTY(float explosionRatio")),
           "EditorViewModel.h must declare the explosionRatio Q_PROPERTY");
  QVERIFY2(editorHeader.contains(QStringLiteral("resetExplosionRatio")),
           "EditorViewModel.h must declare resetExplosionRatio()");
  QVERIFY2(editorHeader.contains(QStringLiteral("m_explosionRatio = 1.0f"))
               || editorSource.contains(QStringLiteral("m_explosionRatio = 1.0f")),
           "EditorViewModel must initialize m_explosionRatio to 1.0f");

  // (2) AssemblePage slider bound to editorVm.explosionRatio (ASMEXPLODE-01).
  QVERIFY2(assemblePage.contains(QStringLiteral("\u7206\u70b8\u6bd4\u4f8b")),
           "AssemblePage must contain the '\u7206\u70b8\u6bd4\u4f8b' (Explosion Ratio) label");
  QVERIFY2(assemblePage.contains(QStringLiteral("CxSlider")),
           "AssemblePage must use a CxSlider for the explosion ratio");
  QVERIFY2(assemblePage.contains(QStringLiteral("editorVm.explosionRatio")),
           "AssemblePage must bind the slider to editorVm.explosionRatio");
  QVERIFY2(assemblePage.contains(QStringLiteral("resetExplosionRatio")),
           "AssemblePage must call editorVm.resetExplosionRatio() from a reset affordance");

  // (3) RhiViewport property + re-render trigger (ASMEXPLODE-01).
  QVERIFY2(rhiViewportHeader.contains(QStringLiteral("Q_PROPERTY(float explosionRatio")),
           "RhiViewport.h must declare the explosionRatio Q_PROPERTY");
  QVERIFY2(rhiViewportSource.contains(QStringLiteral("setExplosionRatio")),
           "RhiViewport.cpp must implement setExplosionRatio");
  QVERIFY2(rhiViewportSource.contains(QStringLiteral("update()")),
           "RhiViewport::setExplosionRatio must call update() to trigger re-render");

  // (4) Renderer offset gated to AssembleView (ASMEXPLODE-02).
  QVERIFY2(rhiViewportRenderer.contains(QStringLiteral("m_explosionRatio")),
           "RhiViewportRenderer.cpp must read m_explosionRatio");
  QVERIFY2(rhiViewportRenderer.contains(QStringLiteral("CanvasAssembleView")),
           "RhiViewportRenderer.cpp must gate the explosion offset on CanvasAssembleView");
  QVERIFY2(rhiViewportRenderer.contains(QStringLiteral("m_canvasType == RhiViewport::CanvasPreview")),
           "RhiViewportRenderer.cpp must keep the CanvasPreview guards intact");

  // (5) Connector guide lines gated to ratio > 1.0 (ASMEXPLODE-02).
  QVERIFY2(rhiViewportRenderer.contains(QStringLiteral("assemblyConnector"))
               || rhiViewportRenderer.contains(QStringLiteral("renderAssemblyConnectors")),
           "RhiViewportRenderer.cpp must have an assembly-connector buffer/render path");
  QVERIFY2(rhiViewportRenderer.contains(QStringLiteral("m_explosionRatio > 1.0")),
           "RhiViewportRenderer.cpp must gate connector rendering on m_explosionRatio > 1.0");
}

void QmlUiAuditTests::assembleViewMeasurementGizmoWiredAndOverlayRenders()
{
  // Phase 92-01 (ASMMEASURE-01/02): the Assembly measurement gizmo wiring is
  // source-audited: GizmoAssemblyMeasure enum value, EditorViewModel
  // activability gate + measure Q_PROPERTYs + activator, AssemblePage Ctrl+Y +
  // the 测量 panel bound to the measure properties, the renderer overlay gated
  // to gizmo mode 19 + CanvasAssembleView, the AssemblyMeasureGeometry helper
  // file present, and the CanvasPreview guards intact (Prepare/Preview
  // regression-free).
  const QString editorHeader = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString editorSource = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString rhiViewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString rhiViewportRenderer = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  const QString assemblePage = readSource(QStringLiteral("src/qml_gui/pages/AssemblePage.qml"));
  QVERIFY2(!editorHeader.isEmpty(), "Unable to read EditorViewModel.h");
  QVERIFY2(!editorSource.isEmpty(), "Unable to read EditorViewModel.cpp");
  QVERIFY2(!rhiViewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!rhiViewportRenderer.isEmpty(), "Unable to read RhiViewportRenderer.cpp");
  QVERIFY2(!assemblePage.isEmpty(), "Unable to read AssemblePage.qml");

  // (1) Gizmo enum value (ASMMEASURE-01): GizmoAssemblyMeasure exists in the
  // RhiViewport GizmoMode enum (distinct from GizmoMeasure = 3 / Prepare).
  QVERIFY2(rhiViewportHeader.contains(QStringLiteral("GizmoAssemblyMeasure")),
           "RhiViewport.h must declare the GizmoAssemblyMeasure enum value");

  // (2) EditorViewModel activability + state (ASMMEASURE-01/02).
  QVERIFY2(editorHeader.contains(QStringLiteral("Q_PROPERTY(bool assemblyMeasureGizmoActive")),
           "EditorViewModel.h must declare the assemblyMeasureGizmoActive Q_PROPERTY");
  QVERIFY2(editorHeader.contains(QStringLiteral("activateAssemblyMeasureGizmo")),
           "EditorViewModel.h must declare activateAssemblyMeasureGizmo()");
  QVERIFY2(editorHeader.contains(QStringLiteral("assemblyMeasureDistanceText")),
           "EditorViewModel.h must declare the assemblyMeasureDistanceText accessor");
  QVERIFY2(editorHeader.contains(QStringLiteral("assemblyMeasureAngleText")),
           "EditorViewModel.h must declare the assemblyMeasureAngleText accessor");
  QVERIFY2(editorHeader.contains(QStringLiteral("assemblyMeasurePlaneText")),
           "EditorViewModel.h must declare the assemblyMeasurePlaneText accessor");
  // Activability gate references AssembleView + explosion ratio + 2 volumes.
  QVERIFY2(editorSource.contains(QStringLiteral("m_activeCanvasType == 2")),
           "EditorViewModel.cpp activability must gate on AssembleView (m_activeCanvasType == 2)");
  QVERIFY2(editorSource.contains(QStringLiteral("m_explosionRatio")),
           "EditorViewModel.cpp activability must reference m_explosionRatio");
  QVERIFY2(editorSource.contains(QStringLiteral("m_selectedSourceIndices.size() >= 2")),
           "EditorViewModel.cpp activability must require >=2 selected volumes");
  // The AssembleView mask advertises the GizmoAssemblyMeasure bit (1 << 19).
  QVERIFY2(editorSource.contains(QStringLiteral("(1 << 19)")),
           "EditorViewModel.cpp availableGizmoMask AssembleView branch must return (1 << 19)");

  // (3) AssemblePage shortcut + 测量 panel (ASMMEASURE-01/02).
  QVERIFY2(assemblePage.contains(QStringLiteral("Ctrl+Y")),
           "AssemblePage must bind the Ctrl+Y shortcut");
  QVERIFY2(assemblePage.contains(QStringLiteral("\u6d4b\u91cf")),
           "AssemblePage must contain the '\u6d4b\u91cf' (Measurement) panel");
  QVERIFY2(assemblePage.contains(QStringLiteral("activateAssemblyMeasureGizmo")),
           "AssemblePage must call editorVm.activateAssemblyMeasureGizmo() from Ctrl+Y");
  QVERIFY2(assemblePage.contains(QStringLiteral("assemblyMeasureDistanceText")),
           "AssemblePage must bind the distance label to assemblyMeasureDistanceText");
  QVERIFY2(assemblePage.contains(QStringLiteral("assemblyMeasureAngleText")),
           "AssemblePage must bind the angle label to assemblyMeasureAngleText");

  // (4) Renderer overlay gated to AssembleView + gizmo mode 19 (ASMMEASURE-02).
  QVERIFY2(rhiViewportRenderer.contains(QStringLiteral("m_gizmoMode == 19"))
               || rhiViewportRenderer.contains(QStringLiteral("GizmoAssemblyMeasure")),
           "RhiViewportRenderer.cpp must gate the overlay on m_gizmoMode == 19 / GizmoAssemblyMeasure");
  QVERIFY2(rhiViewportRenderer.contains(QStringLiteral("CanvasAssembleView")),
           "RhiViewportRenderer.cpp must gate the overlay on CanvasAssembleView");
  QVERIFY2(rhiViewportRenderer.contains(QStringLiteral("assemblyMeasureLineBuffer"))
               || rhiViewportRenderer.contains(QStringLiteral("renderAssemblyMeasureOverlay")),
           "RhiViewportRenderer.cpp must have an assembly-measure overlay buffer/render path");
  // Prepare/Preview regression gate: CanvasPreview guards must still be present.
  QVERIFY2(rhiViewportRenderer.contains(QStringLiteral("m_canvasType == RhiViewport::CanvasPreview")),
           "RhiViewportRenderer.cpp must keep the CanvasPreview guards intact");

  // (5) Geometry helper file exists (ASMMEASURE-02). Resolve against
  // QT_TESTCASE_SOURCEDIR (repo root) the same way the other file-existence
  // checks do (QmlUiAuditTests.cpp:2361-2368) — the test exe runs from build/.
  const QString assemblyMeasureHeaderPath =
      QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR))
          .filePath(QStringLiteral("src/core/rendering/AssemblyMeasureGeometry.h"));
  QVERIFY2(QFileInfo::exists(assemblyMeasureHeaderPath),
           qPrintable(QStringLiteral("AssemblyMeasureGeometry.h must exist at %1")
                          .arg(assemblyMeasureHeaderPath)));
}

void QmlUiAuditTests::assembleViewRestorationMilestoneHasFinalVerificationCoverage()
{
  // Phase 93-01 (ASMVERIFY-01): consolidated AssembleView-restoration milestone
  // verification. Mirrors Phase 88's settingsRestorationMilestoneHasFinalVerifi
  // cationCoverage: reads the relevant sources + self-reads the audit file and
  // asserts the whole v4.2 milestone is covered. Locks: (1) placeholder
  // removed; (2) AssemblePage present + registered; (3) CanvasAssembleView
  // enum; (4) explosion-ratio wiring; (5) Assembly gizmo anchors; (6) data
  // pool present; (7) Phase 90/91/92/93 audit anchors referenced.
  const QString qrc = readSource(QStringLiteral("src/qml_gui/qml.qrc"));
  const QString plater = readSource(QStringLiteral("src/qml_gui/pages/Plater.qml"));
  const QString rhiViewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString rhiViewportRenderer = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  const QString editorHeader = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString assemblePage = readSource(QStringLiteral("src/qml_gui/pages/AssemblePage.qml"));
  const QString audits = readSource(QStringLiteral("tests/QmlUiAuditTests.cpp"));
  QVERIFY2(!qrc.isEmpty(), "Unable to read qml.qrc");
  QVERIFY2(!plater.isEmpty(), "Unable to read Plater.qml");
  QVERIFY2(!rhiViewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!rhiViewportRenderer.isEmpty(), "Unable to read RhiViewportRenderer.cpp");
  QVERIFY2(!editorHeader.isEmpty(), "Unable to read EditorViewModel.h");
  QVERIFY2(!assemblePage.isEmpty(), "Unable to read AssemblePage.qml");
  QVERIFY2(!audits.isEmpty(), "Unable to read QmlUiAuditTests.cpp");

  // (1) Placeholder removed (ASM-CLEANUP): the placeholder Text, the
  //     assembleSlot Item, and the v2.0 Out-of-Scope comment must be absent.
  QVERIFY2(!plater.contains(QStringLiteral("\u88c5\u914d\u89c6\u56fe\u6682\u4e0d\u53ef\u7528")),
           "Plater.qml must not contain the '\u88c5\u914d\u89c6\u56fe\u6682\u4e0d\u53ef\u7528' placeholder text");
  QVERIFY2(!plater.contains(QStringLiteral("id: assembleSlot")),
           "Plater.qml must not contain the 'id: assembleSlot' placeholder item");
  QVERIFY2(!plater.contains(QStringLiteral("Out of Scope\u3001\u4ec5\u4fdd\u7559\u679a\u4e3e\u5165\u53e3")),
           "Plater.qml must not contain the v2.0 Out-of-Scope comment");

  // (2) AssemblePage present + registered (ASMSHELL): qml.qrc registers it and
  //     Plater.qml instantiates it.
  QVERIFY2(qrc.contains(QStringLiteral("pages/AssemblePage.qml")),
           "qml.qrc must register pages/AssemblePage.qml");
  QVERIFY2(plater.contains(QStringLiteral("AssemblePage")),
           "Plater.qml must instantiate AssemblePage");

  // (3) CanvasAssembleView enum (ASMSHELL-02/ASMROUTE-01).
  QVERIFY2(rhiViewportHeader.contains(QStringLiteral("CanvasAssembleView = 2")),
           "RhiViewport.h must declare CanvasAssembleView = 2");
  QVERIFY2(rhiViewportRenderer.contains(QStringLiteral("CanvasAssembleView")),
           "RhiViewportRenderer.cpp must branch on CanvasAssembleView");
  // Prepare/Preview regression: CanvasPreview guards must still be present.
  QVERIFY2(rhiViewportRenderer.contains(QStringLiteral("m_canvasType == RhiViewport::CanvasPreview")),
           "RhiViewportRenderer.cpp must keep the CanvasPreview guards intact");

  // (4) Explosion-ratio wiring (ASMEXPLODE).
  QVERIFY2(editorHeader.contains(QStringLiteral("Q_PROPERTY(float explosionRatio")),
           "EditorViewModel.h must declare the explosionRatio Q_PROPERTY");
  QVERIFY2(assemblePage.contains(QStringLiteral("\u7206\u70b8\u6bd4\u4f8b")),
           "AssemblePage must contain the '\u7206\u70b8\u6bd4\u4f8b' (Explosion Ratio) label");
  QVERIFY2(assemblePage.contains(QStringLiteral("editorVm.explosionRatio")),
           "AssemblePage must bind to editorVm.explosionRatio");

  // (5) Assembly gizmo anchors (ASMMEASURE).
  QVERIFY2(rhiViewportHeader.contains(QStringLiteral("GizmoAssemblyMeasure = 19")),
           "RhiViewport.h must declare GizmoAssemblyMeasure = 19");
  QVERIFY2(editorHeader.contains(QStringLiteral("activateAssemblyMeasureGizmo")),
           "EditorViewModel.h must declare activateAssemblyMeasureGizmo()");
  QVERIFY2(assemblePage.contains(QStringLiteral("\u6d4b\u91cf")),
           "AssemblePage must contain the '\u6d4b\u91cf' (Measurement) panel");

  // (6) Data pool present (ASMROUTE-02): the AssembleViewDataPool header
  //     exists on disk + EditorViewModel references the pool. File existence
  //     resolves against QT_TESTCASE_SOURCEDIR (repo root) — the Phase 92
  //     fixture convention.
  const QString poolHeaderPath =
      QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR))
          .filePath(QStringLiteral("src/core/rendering/AssembleViewDataPool.h"));
  QVERIFY2(QFileInfo::exists(poolHeaderPath),
           qPrintable(QStringLiteral("AssembleViewDataPool.h must exist at %1")
                          .arg(poolHeaderPath)));
  QVERIFY2(editorHeader.contains(QStringLiteral("AssembleViewDataPool"))
               || editorHeader.contains(QStringLiteral("m_assembleViewDataPool"))
               || editorHeader.contains(QStringLiteral("refreshAssembleViewDataPool")),
           "EditorViewModel.h must reference the AssembleView data pool");

  // (7) Milestone anchor coverage: the Phase 90/91/92/93 audit slot names must
  //     all be present in this audit file.
  const QStringList milestoneAnchors = {
      QStringLiteral("assembleViewShellReplacesPlaceholderAndRegistersCanvasHost"),
      QStringLiteral("assembleViewExplosionRatioWiredAndRenderBranchAppliesOffset"),
      QStringLiteral("assembleViewMeasurementGizmoWiredAndOverlayRenders"),
      QStringLiteral("assembleViewRestorationMilestoneHasFinalVerificationCoverage"),
      QStringLiteral("assembleViewPlaceholderArtifactsStayAbsent")
  };
  for (const QString &anchor : milestoneAnchors) {
    QVERIFY2(audits.contains(anchor),
             qPrintable(QStringLiteral("AssembleView milestone missing audit anchor: %1").arg(anchor)));
  }
}

void QmlUiAuditTests::assembleViewPlaceholderArtifactsStayAbsent()
{
  // Phase 93-01 (ASMVERIFY-01 regression): the AssembleView placeholder tokens
  // removed by Phase 90 must stay absent. Mirrors Phase 88's
  // deletedSettingsPathsStayAbsent: a token list + a source-file list +
  // QVERIFY2(!content.contains(token), ...). Fails CI deterministically if any
  // reappear.
  const QString plater = readSource(QStringLiteral("src/qml_gui/pages/Plater.qml"));
  const QString qrc = readSource(QStringLiteral("src/qml_gui/qml.qrc"));
  QVERIFY2(!plater.isEmpty(), "Unable to read Plater.qml");
  QVERIFY2(!qrc.isEmpty(), "Unable to read qml.qrc");

  // Placeholder tokens that must be absent from Plater.qml (the Phase 90
  // removal surface). CJK literals as \uXXXX escapes.
  const QStringList placeholderTokens = {
      QStringLiteral("\u88c5\u914d\u89c6\u56fe\u6682\u4e0d\u53ef\u7528"),  // 装配视图暂不可用
      QStringLiteral("id: assembleSlot"),
      QStringLiteral("Out of Scope\u3001\u4ec5\u4fdd\u7559\u679a\u4e3e\u5165\u53e3"),  // v2.0 Out-of-Scope comment
      QStringLiteral("\u4ec5\u4fdd\u7559\u679a\u4e3e\u5165\u53e3")  // 仅保留枚举入口
  };
  for (const QString &token : placeholderTokens) {
    QVERIFY2(!plater.contains(token),
             qPrintable(QStringLiteral("Plater.qml reintroduced placeholder token: %1").arg(token)));
  }

  // qml.qrc must not reference any stale AssembleView placeholder file. The
  // only AssembleView entry is the normalized pages/AssemblePage.qml.
  QVERIFY2(!qrc.contains(QStringLiteral("AssembleViewPlaceholder")),
           "qml.qrc must not reference a stale AssembleView placeholder resource");
  // Confirm exactly the normalized AssemblePage entry is present (count == 1).
  int assemblePageEntries = 0;
  int idx = 0;
  while ((idx = qrc.indexOf(QStringLiteral("pages/AssemblePage.qml"), idx)) != -1) {
    ++assemblePageEntries;
    idx += QStringLiteral("pages/AssemblePage.qml").size();
  }
  QVERIFY2(assemblePageEntries == 1,
           qPrintable(QStringLiteral("qml.qrc must have exactly one pages/AssemblePage.qml "
                                     "entry, got %1")
                          .arg(assemblePageEntries)));
}

void QmlUiAuditTests::rhiViewportThumbnailCaptureUsesRealReadback()
{
  // Phase 95-01 (THUMBCAP-01/02/03): source-audit guard proving the
  // requestThumbnailCapture solid-color stub is replaced with real QRhi texture
  // readback. Mirrors the rhiViewportRendererUsesPrepareSceneDataAndDirtyUploads
  // pattern: reads the item + renderer sources and asserts the replacement is
  // real. Source-level only — no GPU/runtime dependency — so this runs in the
  // regression ctest. Runtime pixel proof (non-solid-color capture) is routed
  // to Phase 98.
  const QString viewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString viewportSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.cpp"));
  const QString rendererHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.h"));
  const QString rendererSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  QVERIFY2(!viewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!viewportSource.isEmpty(), "Unable to read RhiViewport.cpp");
  QVERIFY2(!rendererHeader.isEmpty(), "Unable to read RhiViewportRenderer.h");
  QVERIFY2(!rendererSource.isEmpty(), "Unable to read RhiViewportRenderer.cpp");

  // (1) Stub gone (THUMBCAP-01): the #18222c solid-color literal unique to the
  //     old stub is no longer present anywhere in RhiViewport.cpp.
  QVERIFY2(!viewportSource.contains(QStringLiteral("#18222c")),
           "RhiViewport.cpp must not retain the #18222c solid-color stub literal");

  // (2) Real readback wired in the renderer (THUMBCAP-01): readBackTexture +
  //     QRhiReadbackDescription/QRhiReadbackResult + offscreen texture RT +
  //     its own compatible render-pass descriptor.
  QVERIFY2(rendererSource.contains(QStringLiteral("readBackTexture")),
           "RhiViewportRenderer.cpp must call QRhiResourceUpdateBatch::readBackTexture");
  QVERIFY2(rendererSource.contains(QStringLiteral("QRhiReadbackDescription"))
               || rendererSource.contains(QStringLiteral("QRhiReadbackResult")),
           "RhiViewportRenderer.cpp must use QRhiReadbackDescription/QRhiReadbackResult");
  QVERIFY2(rendererSource.contains(QStringLiteral("newTextureRenderTarget")),
           "RhiViewportRenderer.cpp must create an offscreen QRhiTextureRenderTarget");
  QVERIFY2(rendererSource.contains(QStringLiteral("newCompatibleRenderPassDescriptor")),
           "RhiViewportRenderer.cpp must create the offscreen RT's own render-pass descriptor");

  // (3) Offscreen RT is single-sample (THUMBCAP-02): thumbnail texture created
  //     with sample count 1, no multisample resolve in the thumbnail path.
  QVERIFY2(rendererSource.contains(QStringLiteral("QRhiTexture::RGBA8")),
           "RhiViewportRenderer.cpp must create the thumbnail texture as RGBA8");
  QVERIFY2(rendererSource.contains(QStringLiteral("QRhiTexture::RenderTarget")),
           "RhiViewportRenderer.cpp must flag the thumbnail texture as a render target");
  QVERIFY2(!rendererSource.contains(QStringLiteral("resolveTexture")),
           "RhiViewportRenderer.cpp thumbnail path must not perform MSAA resolve (single-sample offscreen RT)");

  // (4) Render-thread request queue mirrors synchronize() (THUMBCAP-03): the
  //     renderer copies the item-side request and clears the item-side flag
  //     (m_cameraDirty=false consumption pattern).
  QVERIFY2(rendererSource.contains(QStringLiteral("viewport->m_thumbnailRequestPending = false")),
           "RhiViewportRenderer.cpp synchronize() must clear the item-side request flag after copying");
  QVERIFY2(rendererSource.contains(QStringLiteral("m_thumbnailRequestPending")),
           "RhiViewportRenderer.cpp must mirror the thumbnail request flag");
  QVERIFY2(rendererSource.contains(QStringLiteral("m_viewportItem")),
           "RhiViewportRenderer.cpp must cache the item pointer for the queued callback");

  // (5) Async readback poll at the start of render() (THUMBCAP-01): the
  //     renderer checks the pending result before the on-screen pass and
  //     requests a follow-up frame so the async readback is polled.
  QVERIFY2(rendererSource.contains(QStringLiteral("m_thumbnailReadbackInFlight")),
           "RhiViewportRenderer.cpp must track the async readback in-flight flag");
  QVERIFY2(rendererSource.contains(QStringLiteral("m_thumbnailReadbackResult")),
           "RhiViewportRenderer.cpp must poll the async readback result");

  // (6) Queued callback (THUMBCAP-03): deliverThumbnail is defined on the item
  //     and the renderer posts the QImage via Qt::QueuedConnection.
  QVERIFY2(viewportHeader.contains(QStringLiteral("deliverThumbnail(const QImage &image, int plateIndex)")),
           "RhiViewport.h must declare the deliverThumbnail GUI-thread delivery slot");
  QVERIFY2(viewportSource.contains(QStringLiteral("void RhiViewport::deliverThumbnail")),
           "RhiViewport.cpp must define the deliverThumbnail delivery slot");
  QVERIFY2(rendererSource.contains(QStringLiteral("Qt::QueuedConnection")),
           "RhiViewportRenderer.cpp must post the QImage via Qt::QueuedConnection");

  // (7) Public contract preserved (THUMBCAP-01): requestThumbnailCapture still
  //     sets the request + calls update(); lastThumbnailData/thumbnailCaptured
  //     remain the Q_PROPERTY/signal contract.
  QVERIFY2(viewportHeader.contains(QStringLiteral("Q_INVOKABLE void requestThumbnailCapture(int plateIndex, int size = 128)")),
           "RhiViewport.h must keep the requestThumbnailCapture Q_INVOKABLE signature unchanged");
  QVERIFY2(viewportHeader.contains(QStringLiteral("Q_PROPERTY(QString lastThumbnailData")),
           "RhiViewport.h must keep the lastThumbnailData Q_PROPERTY contract");
  QVERIFY2(viewportHeader.contains(QStringLiteral("void thumbnailCaptured();")),
           "RhiViewport.h must keep the thumbnailCaptured signal contract");
  QVERIFY2(viewportSource.contains(QStringLiteral("m_thumbnailRequestPending = true")),
           "RhiViewport::requestThumbnailCapture must set the render-thread request flag");
  QVERIFY2(viewportSource.contains(QStringLiteral("m_thumbnailSize = qMax(32, size)")),
           "RhiViewport::requestThumbnailCapture must clamp the thumbnail size to >= 32");
}

void QmlUiAuditTests::projectServiceWrites3mfThumbnails()
{
  // Phase 96-01 (THUMBWRITE-01/02/03): source-audit guard proving the 3MF
  // write-side thumbnail hooks are populated with real captured pixels.
  // Mirrors the rhiViewportThumbnailCaptureUsesRealReadback pattern
  // (Phase 95): reads ProjectServiceMock.cpp and asserts the replacement is
  // real. Source-level only — no 3MF save runtime dependency — so this runs
  // in the regression ctest. Runtime save-reload round-trip proof is routed
  // to Phase 97.
  const QString src = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  QVERIFY2(!src.isEmpty(), "Unable to read ProjectServiceMock.cpp");

  // (1) qimageToThumbnailData helper exists (THUMBWRITE-01/02 enabler):
  //     file-local, Format_RGBA8888 conversion for read-side symmetry.
  QVERIFY2(src.contains(QStringLiteral("qimageToThumbnailData")),
           "ProjectServiceMock.cpp must define a qimageToThumbnailData helper");
  QVERIFY2(src.contains(QStringLiteral("convertToFormat(QImage::Format_RGBA8888")),
           "qimageToThumbnailData must convertToFormat(Format_RGBA8888) for read-side symmetry (ProjectServiceMock.cpp:5455)");

  // (2) buildPlateDataList populates PlateData::plate_thumbnail (THUMBWRITE-01):
  //     the per-plate populate guarded by !thumbnail().isNull().
  QVERIFY2(src.contains(QStringLiteral("pd->plate_thumbnail = qimageToThumbnailData")),
           "buildPlateDataList must populate pd->plate_thumbnail via qimageToThumbnailData");
  QVERIFY2(src.contains(QStringLiteral("!p->thumbnail().isNull()")),
           "buildPlateDataList must guard the plate_thumbnail populate with !p->thumbnail().isNull()");

  // (3) saveProject populates StoreParams::thumbnail_data (THUMBWRITE-02):
  //     a real ThumbnailData pointer pushed before store_bbs_3mf.
  QVERIFY2(src.contains(QStringLiteral("params.thumbnail_data.push_back")),
           "saveProject must push a ThumbnailData* into params.thumbnail_data before store_bbs_3mf");

  // (4) The deferred-write markers are gone (THUMBWRITE-01/02 closure):
  //     the v3.2 "deferred to THUMB-03" / "throws a non-std exception"
  //     comments that documented the omitted write side must no longer be
  //     present at the populate sites.
  QVERIFY2(!src.contains(QStringLiteral("throws a non-std exception on the Qt6 mock pipeline")),
           "ProjectServiceMock.cpp must not retain the 'throws a non-std exception' deferred-write marker (THUMBWRITE-03: real pixels make the PNG path complete)");
  QVERIFY2(!src.contains(QStringLiteral("deferred to THUMB-03")),
           "ProjectServiceMock.cpp must not retain the 'deferred to THUMB-03' marker at the thumbnail populate sites");

  // (5) HAS_LIBSLIC3R guard present around the write-side code (the store
  //     call is guarded at ProjectServiceMock.cpp:5109; the thumbnail
  //     populate must inherit the same guard).
  QVERIFY2(src.contains(QStringLiteral("#ifdef HAS_LIBSLIC3R")),
           "ProjectServiceMock.cpp must guard the libslic3r write-side under #ifdef HAS_LIBSLIC3R");
}

void QmlUiAuditTests::wipeTowerReadbackAndRenderAnchorsPresent()
{
  // Phase 102-01 (WTVERIFY-01): consolidated wipe-tower source-audit lock
  // covering all 8 WT-* regions from 99-GAP-MATRIX.md. This is the regression
  // lock that prevents a future refactor from silently removing the readback
  // wiring (Phase 100), the PreparePage binding (Phase 100), the Option A
  // baseline comment (Phase 101), or the SoftwareViewport QPainter box
  // (Phase 101). Each QVERIFY2 message names the WT-* region it locks so a
  // failure points directly at the gap-matrix row.
  //
  // Pattern: QFile + QT_TESTCASE_SOURCEDIR + QString::contains (deterministic,
  // build-dir-independent). Mirrors the Phase 101
  // wipeTowerRealDimsReachRendererPipeline pattern (ViewModelSmokeTests.cpp)
  // and the Phase 96 projectServiceWrites3mfThumbnails pattern in this file.

  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString sliceService = readSource(QStringLiteral("src/core/services/SliceService.cpp"));
  const QString editorVmHeader = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString editorVmSource = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString gizmoGeometry = readSource(QStringLiteral("src/core/rendering/GizmoGeometry.cpp"));
  const QString rhiViewportRenderer = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  const QString softwareViewport = readSource(QStringLiteral("src/qml_gui/Renderer/SoftwareViewport.cpp"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!sliceService.isEmpty(), "Unable to read SliceService.cpp");
  QVERIFY2(!editorVmHeader.isEmpty(), "Unable to read EditorViewModel.h");
  QVERIFY2(!editorVmSource.isEmpty(), "Unable to read EditorViewModel.cpp");
  QVERIFY2(!gizmoGeometry.isEmpty(), "Unable to read GizmoGeometry.cpp");
  QVERIFY2(!rhiViewportRenderer.isEmpty(), "Unable to read RhiViewportRenderer.cpp");
  QVERIFY2(!softwareViewport.isEmpty(), "Unable to read SoftwareViewport.cpp");

  // (1) WT-VIEWPORT-DEFAULTS (Phase 100): PreparePage.qml GLViewport must bind
  //     all 6 wipe-tower Q_PROPERTYs to root.editorVm so the renderer no longer
  //     falls through to the hardcoded 10/10/50/100/25 defaults. Each binding
  //     is whitespace-sensitive; a reformatter that breaks this signals a real
  //     change to review.
  QVERIFY2(preparePage.contains(QStringLiteral("showWipeTower: root.editorVm ? root.editorVm.showWipeTower")),
           "WT-VIEWPORT-DEFAULTS: PreparePage.qml GLViewport must bind showWipeTower to editorVm");
  QVERIFY2(preparePage.contains(QStringLiteral("wipeTowerWidth: root.editorVm ? root.editorVm.wipeTowerWidth")),
           "WT-VIEWPORT-DEFAULTS: PreparePage.qml GLViewport must bind wipeTowerWidth to editorVm");
  QVERIFY2(preparePage.contains(QStringLiteral("wipeTowerDepth: root.editorVm ? root.editorVm.wipeTowerDepth")),
           "WT-VIEWPORT-DEFAULTS: PreparePage.qml GLViewport must bind wipeTowerDepth to editorVm");
  QVERIFY2(preparePage.contains(QStringLiteral("wipeTowerHeight: root.editorVm ? root.editorVm.wipeTowerHeight")),
           "WT-VIEWPORT-DEFAULTS: PreparePage.qml GLViewport must bind wipeTowerHeight to editorVm");
  QVERIFY2(preparePage.contains(QStringLiteral("wipeTowerX: root.editorVm ? root.editorVm.wipeTowerX")),
           "WT-VIEWPORT-DEFAULTS: PreparePage.qml GLViewport must bind wipeTowerX to editorVm");
  QVERIFY2(preparePage.contains(QStringLiteral("wipeTowerZ: root.editorVm ? root.editorVm.wipeTowerZ")),
           "WT-VIEWPORT-DEFAULTS: PreparePage.qml GLViewport must bind wipeTowerZ to editorVm");

  // (2) WT-PRINT-DATA (Phase 100): SliceService.cpp must read the upstream
  //     Print::wipe_tower_data() AND the has_wipe_tower() gate inside the
  //     worker where the Print is valid. Zero references means the real
  //     sliced geometry never reaches the GUI.
  QVERIFY2(sliceService.contains(QStringLiteral("print.wipe_tower_data()")),
           "WT-PRINT-DATA: SliceService must read print.wipe_tower_data()");
  QVERIFY2(sliceService.contains(QStringLiteral("print.has_wipe_tower()")),
           "WT-PRINT-DATA: SliceService must read print.has_wipe_tower()");

  // (3) WT-READBACK-POINT (Phase 100, Frozen Decision 1): the readback must
  //     occur between print.process() (where wipe_tower_data is populated) and
  //     activePrint_.store(nullptr) (where the Print is invalidated). The
  //     capturedGeometry.valid = true marker confirms the valid path is set
  //     inside that window. No Print* may escape the worker.
  QVERIFY2(sliceService.contains(QStringLiteral("print.process()")),
           "WT-READBACK-POINT: SliceService must call print.process() before reading wipe_tower_data");
  QVERIFY2(sliceService.contains(QStringLiteral("activePrint_.store(nullptr")),
           "WT-READBACK-POINT: SliceService must invalidate the Print via activePrint_.store(nullptr) after readback");
  QVERIFY2(sliceService.contains(QStringLiteral("capturedGeometry.valid = true")),
           "WT-READBACK-POINT: SliceService must set capturedGeometry.valid = true inside the readback window (between print.process() and activePrint_.store(nullptr))");

  // (4) WT-HAS-WIPE-GATE (Phase 100, Frozen Decision 3): the readback gates
  //     on has_wipe_tower() inside SliceService.cpp, and the EditorViewModel
  //     slot forces m_showWipeTower from geometry.valid so single-material
  //     slices show no placeholder box leak.
  QVERIFY2(sliceService.contains(QStringLiteral("if (print.has_wipe_tower())")),
           "WT-HAS-WIPE-GATE: SliceService readback must be gated on if (print.has_wipe_tower())");
  QVERIFY2(editorVmSource.contains(QStringLiteral("m_showWipeTower = true")),
           "WT-HAS-WIPE-GATE: EditorViewModel::onWipeTowerGeometryReady must set m_showWipeTower = true on the valid path");
  QVERIFY2(editorVmSource.contains(QStringLiteral("m_showWipeTower = false")),
           "WT-HAS-WIPE-GATE: EditorViewModel::onWipeTowerGeometryReady must force m_showWipeTower = false on the invalid path (no placeholder leak)");

  // (5) WT-PLACEHOLDER-BOX (Phase 101, Option A baseline): buildWipeTowerVertices
  //     must accept the real width/depth/height/position dims fed from the
  //     readback. The Option A structure (36-vertex rectangular prism) is the
  //     v4.4 baseline; Option B real mesh is deferred.
  QVERIFY2(gizmoGeometry.contains(QStringLiteral("GizmoGeometry::buildWipeTowerVertices(float x,")),
           "WT-PLACEHOLDER-BOX: GizmoGeometry::buildWipeTowerVertices must accept width/depth/height/position dims (Option A structure)");

  // (6) WT-RENDERER-BUFFER (Phase 101 verified): RhiViewportRenderer::
  //     uploadWipeTowerBuffer must call buildWipeTowerVertices with the
  //     m_wipeTower* members so the real dims reach the GPU buffer. The
  //     m_wipeTowerDirty rebuild path was already correct end-to-end after
  //     Phase 100.
  QVERIFY2(rhiViewportRenderer.contains(QStringLiteral("buildWipeTowerVertices(m_wipeTowerX,")),
           "WT-RENDERER-BUFFER: RhiViewportRenderer::uploadWipeTowerBuffer must call buildWipeTowerVertices with the m_wipeTower* dims");

  // (7) WT-RENDER-UPGRADE (Phase 101, Frozen Decision 2): the Option A
  //     baseline must be documented in the GizmoGeometry.cpp comment block
  //     with the upstream load_wipe_tower_preview anchor and the Option B
  //     deferral marker. This locks the v4.4 baseline against accidental
  //     upgrade to Option B without re-opening WTAUDIT-02.
  QVERIFY2(gizmoGeometry.contains(QStringLiteral("Option A")),
           "WT-RENDER-UPGRADE: GizmoGeometry.cpp must document the Option A baseline");
  QVERIFY2(gizmoGeometry.contains(QStringLiteral("load_wipe_tower_preview")),
           "WT-RENDER-UPGRADE: GizmoGeometry.cpp must cite the upstream load_wipe_tower_preview anchor");
  QVERIFY2(gizmoGeometry.contains(QStringLiteral("Option B")),
           "WT-RENDER-UPGRADE: GizmoGeometry.cpp must document Option B as the deferred future upgrade");

  // (8) WT-SOFTWARE-VIEWPORT (Phase 101): SoftwareViewport.cpp paint must
  //     consume m_showWipeTower (the WTREAD-02 gate) and the dim members
  //     (m_wipeTowerWidth/Depth/Height/X/Z) so the software fallback path
  //     does not lag the RHI path. The QPainter box mirrors the RHI box
  //     geometry + color.
  QVERIFY2(softwareViewport.contains(QStringLiteral("m_showWipeTower")),
           "WT-SOFTWARE-VIEWPORT: SoftwareViewport.cpp paint must consume m_showWipeTower (the WTREAD-02 gate)");
  QVERIFY2(softwareViewport.contains(QStringLiteral("m_wipeTowerWidth")),
           "WT-SOFTWARE-VIEWPORT: SoftwareViewport.cpp paint must consume m_wipeTowerWidth (real dim from readback)");
}

void QmlUiAuditTests::argvFixtureGateUsesFrameSwappedNotSingleShot()
{
  // Phase 103-01 (FIXTURE-02): the argv startup-fixture gate must wait for the
  // first QQuickWindow::frameSwapped (scene graph rendered at least one frame)
  // before applying open-page / load-model / open-dialog, NOT the old
  // zero-delay timer trick that fired on the next event-loop iteration before a
  // frame was guaranteed. This is the canonical workaround for the recurring
  // Windows-capture-API runtime-evidence blocker (Pitfall 4 readiness gate).
  //
  // Pattern: QFile + QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with
  // a FIXTURE-02-named message (deterministic, build-dir-independent). Mirrors
  // the Phase 102 wipeTowerReadbackAndRenderAnchorsPresent pattern in this file.

  const QString mainCpp = readSource(QStringLiteral("src/qml_gui/main_qml.cpp"));
  QVERIFY2(!mainCpp.isEmpty(), "Unable to read main_qml.cpp");

  // FIXTURE-02 (gate present): the frameSwapped signal must gate fixture
  // application so screenshots are deterministic.
  QVERIFY2(mainCpp.contains(QStringLiteral("frameSwapped")),
           "FIXTURE-02: argv fixture gate must use QQuickWindow::frameSwapped (deterministic screenshots; the scene graph has rendered at least one frame)");

  // FIXTURE-02 (old gate removed): the zero-delay timer gate signature must be
  // gone. The literal singleShot(0 form is targeted because singleShot may
  // legitimately appear elsewhere; this targets the gate signature.
  QVERIFY2(!mainCpp.contains(QStringLiteral("singleShot(0")),
           "FIXTURE-02: argv fixture gate must NOT use the old singleShot(0) zero-delay timer trick (it fired before the first frame was guaranteed)");

  // FIXTURE-02 (apply function present): the apply function must still exist so
  // the open-page / load-model / open-dialog routing logic is intact (only the
  // scheduling wrapper changed, not the apply logic).
  QVERIFY2(mainCpp.contains(QStringLiteral("applyStartupOpenRequests")),
           "FIXTURE-02: applyStartupOpenRequests must still exist (the apply routing logic is preserved; only the gate scheduling changed)");

  // FIXTURE-02 (one-shot connection contract): the gate must disconnect itself
  // in the handler so it fires exactly once on the first frameSwapped, not on
  // every subsequent frame (the old singleShot also ran exactly once; the new
  // gate must preserve that contract).
  QVERIFY2(mainCpp.contains(QStringLiteral("QObject::disconnect")),
           "FIXTURE-02: the frameSwapped gate must disconnect itself in the handler (one-shot connection; fires exactly once)");

  // FIXTURE-02 (defensive fallback): if the root object is not a QQuickWindow,
  // the gate must apply immediately (degraded mode) rather than hang forever.
  QVERIFY2(mainCpp.contains(QStringLiteral("degraded mode")),
           "FIXTURE-02: the gate must have a defensive fallback when the root object is not a QQuickWindow (apply immediately, never hang)");
}

void QmlUiAuditTests::cliFixtureRecipesAndMultiMaterialModelPresent()
{
  // Phase 104-01 (FIXTURE-01/03/04): the multi-material fixture model, the
  // argv recipe doc, and the anti-feature comment all exist on disk. This is
  // the regression lock that prevents a future refactor from silently deleting
  // the fixture content Phase 103's readiness gate now deterministically
  // applies. Each QVERIFY2 message names the FIXTURE-0X contract it locks so a
  // failure points directly at the requirement.
  //
  // Pattern: QFile + QT_TESTCASE_SOURCEDIR + QFileInfo::exists +
  // QString::contains + QVERIFY2 (deterministic, build-dir-independent).
  // Mirrors the Phase 102 wipeTowerReadbackAndRenderAnchorsPresent pattern and
  // the Phase 103 argvFixtureGateUsesFrameSwappedNotSingleShot pattern in this
  // file.

  // FIXTURE-01 (multi-material fixture present): the 2-extruder fixture must
  // exist on disk and be non-empty so multi-material-dependent features (wipe-
  // tower, filament-map, AMS) can be exercised deterministically. The fixture
  // is a hand-authored 3MF (no suitable multi-material 3MF exists in
  // third_party/OrcaSlicer/tests/data/).
  QFileInfo fixtureInfo(
      QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR))
          .filePath(QStringLiteral("tests/data/multi_material_fixture.3mf")));
  QVERIFY2(fixtureInfo.exists(),
           "FIXTURE-01: tests/data/multi_material_fixture.3mf must exist so multi-material features can be exercised deterministically");
  QVERIFY2(fixtureInfo.size() > 0,
           "FIXTURE-01: tests/data/multi_material_fixture.3mf must be non-empty");

  // FIXTURE-03 (argv recipe doc present + covers major GUI states): the recipe
  // doc must exist and name each major GUI state so visual verification can
  // reach them without simulated clicks.
  const QString recipes = readSource(
      QStringLiteral("tests/data/fixture_recipes.md"));
  QVERIFY2(!recipes.isEmpty(),
           "FIXTURE-03: tests/data/fixture_recipes.md must exist and be readable");
  const QStringList majorStates = {
      QStringLiteral("Prepare"),
      QStringLiteral("Preview"),
      QStringLiteral("AssembleView"),
      QStringLiteral("settings")
  };
  for (const QString &state : majorStates) {
    QVERIFY2(recipes.contains(state),
             qPrintable(QStringLiteral("FIXTURE-03: fixture_recipes.md must cover the major GUI state: %1").arg(state)));
  }

  // FIXTURE-03 (recipes reference the canonical flags): the doc must reference
  // all 4 argv flags so the recipe set is complete.
  const QStringList canonicalFlags = {
      QStringLiteral("--open-page"),
      QStringLiteral("--open-dialog"),
      QStringLiteral("--load-model"),
      QStringLiteral("--skip-first-run")
  };
  for (const QString &flag : canonicalFlags) {
    QVERIFY2(recipes.contains(flag),
             qPrintable(QStringLiteral("FIXTURE-03: fixture_recipes.md must reference the canonical argv flag: %1").arg(flag)));
  }

  // FIXTURE-04 (anti-feature comment present): main_qml.cpp must contain the
  // anti-feature comment anchor documenting that these 4 flags are OWzx-only
  // test-evidence plumbing, NOT a user-facing deep-link product feature.
  const QString mainCpp = readSource(QStringLiteral("src/qml_gui/main_qml.cpp"));
  QVERIFY2(!mainCpp.isEmpty(), "Unable to read main_qml.cpp");
  QVERIFY2(mainCpp.contains(QStringLiteral("test-evidence plumbing")),
           "FIXTURE-04: main_qml.cpp must document the argv flags as test-evidence plumbing (anti-feature anchor)");
  QVERIFY2(mainCpp.contains(QStringLiteral("OWzx-only")),
           "FIXTURE-04: main_qml.cpp must mark the argv flags as OWzx-only (no upstream equivalent)");
  QVERIFY2(mainCpp.contains(QStringLiteral("OrcaSlicer.cpp:7183")),
           "FIXTURE-04: main_qml.cpp anti-feature comment must cite the upstream argv anchor (OrcaSlicer.cpp:7183) so the no-upstream-equivalent claim is traceable");
  QVERIFY2(mainCpp.contains(QStringLiteral("MUST NOT be promoted")),
           "FIXTURE-04: main_qml.cpp anti-feature comment must explicitly state the flags MUST NOT be promoted to a user-facing product feature");
}

void QmlUiAuditTests::d3d12DebugLayerWiredBehindEnvFlag()
{
  // Phase 105-01 (D3D12-01): the D3D12 debug layer must be gated on the
  // OWZX_D3D12_DEBUG env flag so Phase 106 can triage the startup 0xc0000005
  // access violation with GPU validation output, WITHOUT leaking the debug
  // layer into the default OWzxSlicer.exe build (Pitfall 5). This is the
  // regression lock that prevents a future refactor from either (a) dropping
  // the env gate (unconditional enableDebugLayer = true would ship a 20-40%
  // GPU perf regression in Release) or (b) dropping the live-path QSG_RHI_DEBUG
  // forward (the crash fires in the live QQuickRhiItem render path at
  // RhiViewportRenderer.cpp:282-298, NOT at the probe QRhi::create, so the
  // probe-path enablement alone is insufficient). Each QVERIFY2 message names
  // the D3D12-01 contract it locks so a failure points directly at the truth.
  //
  // Pattern: QFile + QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with
  // a D3D12-01-named message (deterministic, build-dir-independent). Mirrors
  // the Phase 102 wipeTowerReadbackAndRenderAnchorsPresent pattern, the Phase
  // 103 argvFixtureGateUsesFrameSwappedNotSingleShot pattern, and the Phase
  // 104 cliFixtureRecipesAndMultiMaterialModelPresent pattern in this file.

  const QString selectorSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiBackendSelector.cpp"));
  const QString mainCpp = readSource(QStringLiteral("src/qml_gui/main_qml.cpp"));
  QVERIFY2(!selectorSource.isEmpty(), "Unable to read RhiBackendSelector.cpp");
  QVERIFY2(!mainCpp.isEmpty(), "Unable to read main_qml.cpp");

  // D3D12-01 / DL-01 (env flag present): the OWZX_D3D12_DEBUG env flag must be
  // read in RhiBackendSelector.cpp via the existing qEnvironmentVariableIsSet +
  // qgetenv pattern (mirrors OWZX_RHI_RENDERER at lines 117-124).
  QVERIFY2(selectorSource.contains(QStringLiteral("OWZX_D3D12_DEBUG")),
           "D3D12-01/DL-01: RhiBackendSelector.cpp must read the OWZX_D3D12_DEBUG env flag");

  // D3D12-01 / DL-02 (probe-path enablement): enableDebugLayer must appear in
  // RhiBackendSelector.cpp on the probe path (set before QRhi::create).
  QVERIFY2(selectorSource.contains(QStringLiteral("enableDebugLayer")),
           "D3D12-01/DL-02: RhiBackendSelector.cpp must set enableDebugLayer on the D3D12 probe path");

  // D3D12-01 / DL-04 (conditional gate, Pitfall 5): enableDebugLayer must NOT
  // be unconditionally enabled. The gate must be the d3d12DebugLayerRequested()
  // helper call (env-flag conditional), not a bare enableDebugLayer = true.
  // The literal "= true" must appear INSIDE the d3d12DebugLayerRequested() gate
  // block. A future regression that writes "enableDebugLayer = true;" at struct
  // scope (unconditional) would ship a 20-40% Release-build GPU perf
  // regression (Pitfall 5).
  QVERIFY2(selectorSource.contains(QStringLiteral("if (d3d12DebugLayerRequested())")),
           "D3D12-01/DL-04: enableDebugLayer must be gated on d3d12DebugLayerRequested() (env-flag conditional, NOT unconditional -- Pitfall 5 Release-build leak)");
  QVERIFY2(selectorSource.contains(QStringLiteral("d3d12DebugLayerRequested")),
           "D3D12-01/DL-04: the d3d12DebugLayerRequested() helper must exist in RhiBackendSelector.cpp");

  // D3D12-01 / DL-02 (enablement BEFORE QRhi::create): the enableDebugLayer
  // assignment must precede the QRhi::create call on the probe path. The
  // QRhi::create call site is the anchor; enableDebugLayer must appear before
  // it in the source.
  const int enableDebugLayerPos = selectorSource.indexOf(QStringLiteral("owner.d3d12Params.enableDebugLayer = true"));
  const int rhiCreatePos = selectorSource.indexOf(QStringLiteral("QRhi::create(candidate.implementation, params)"));
  QVERIFY2(enableDebugLayerPos >= 0,
           "D3D12-01/DL-02: owner.d3d12Params.enableDebugLayer = true must appear in RhiBackendSelector.cpp");
  QVERIFY2(rhiCreatePos > enableDebugLayerPos,
           "D3D12-01/DL-02: enableDebugLayer must be set BEFORE QRhi::create on the probe path (RhiBackendSelector.cpp)");

  // D3D12-01 / DL-03 (live-path coverage): the crash fires in the LIVE
  // QQuickRhiItem render path (RhiViewportRenderer.cpp:282-298 beginPass-after-
  // resourceUpdate), NOT at the probe QRhi::create. So the probe-path
  // enablement alone is insufficient; main_qml.cpp must forward
  // OWZX_D3D12_DEBUG to Qt's QSG_RHI_DEBUG mechanism before QGuiApplication
  // construction so the live render path emits GPU validation output.
  QVERIFY2(mainCpp.contains(QStringLiteral("QSG_RHI_DEBUG")),
           "D3D12-01/DL-03: main_qml.cpp must forward OWZX_D3D12_DEBUG to QSG_RHI_DEBUG (live render-path coverage; the crash fires in the live QQuickRhiItem path, not at the probe QRhi::create)");
  QVERIFY2(mainCpp.contains(QStringLiteral("qputenv(\"QSG_RHI_DEBUG\", \"1\")")),
           "D3D12-01/DL-03: main_qml.cpp must qputenv QSG_RHI_DEBUG=1 when OWZX_D3D12_DEBUG is set");

  // D3D12-01 / DL-03 (QGuiApplication-before timing): QSG_RHI_DEBUG must be
  // set BEFORE QGuiApplication construction because Qt Quick reads it during
  // QGuiApplication startup (scene-graph RHI backend init). The env-gate
  // comment must document this timing constraint.
  QVERIFY2(mainCpp.contains(QStringLiteral("BEFORE QGuiApplication")),
           "D3D12-01/DL-03: main_qml.cpp must document the QSG_RHI_DEBUG timing constraint (set BEFORE QGuiApplication construction)");
  const int qsgRhiDebugPos = mainCpp.indexOf(QStringLiteral("qputenv(\"QSG_RHI_DEBUG\", \"1\")"));
  const int guiAppPos = mainCpp.indexOf(QStringLiteral("QGuiApplication app(argc, argv)"));
  QVERIFY2(guiAppPos > qsgRhiDebugPos,
           "D3D12-01/DL-03: qputenv QSG_RHI_DEBUG must appear BEFORE QGuiApplication construction in main_qml.cpp (Qt Quick reads it during QGuiApplication startup)");
}

void QmlUiAuditTests::d3d12StaysOptInBehindEnvFlag()
{
  // Phase 106-01 (D3D12-03): D3D12 must stay opt-in and the default Windows
  // candidate order must keep D3D11 before D3D12. The Phase 106-01 root-cause
  // investigation (.planning/research/D3D12-CRASH-ROOT-CAUSE.md) is time-boxed
  // per DR-04: the 0xc0000005 access violation could NOT be reproduced in the
  // test environment, so D3D12 must NOT be promoted to default until a
  // confirmed root cause + clean repro on the original machine land. This slot
  // is the D3D12-03 hard-rule regression lock (DR-05): it prevents a future
  // refactor from reordering defaultWindowsCandidates() to put D3D12 first,
  // which would re-introduce the v3.2 startup crash for every default-`auto`
  // user. Each QVERIFY2 message names the D3D12-03 contract it locks so a
  // failure points directly at the truth.
  //
  // Pattern: QFile + QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with
  // a D3D12-03-named message (deterministic, build-dir-independent). Mirrors
  // the Phase 102/103/104/105 source-audit slots in this file, and reuses the
  // exact position-ordered defaultWindowsCandidates boundary check the Phase
  // 55/73/105 slots already assert.

  const QString selectorSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiBackendSelector.cpp"));
  QVERIFY2(!selectorSource.isEmpty(), "Unable to read RhiBackendSelector.cpp");

  // D3D12-03 / DR-05 (D3D11-first order): defaultWindowsCandidates() must keep
  // D3D11 before D3D12. Reuse the position-ordered boundary check established
  // by the Phase 55/73/105 slots: slice the defaultWindowsCandidates body
  // between its signature and candidatesForRequest, then assert Direct3D11
  // appears before Direct3D12 inside that slice. A future regression that
  // flips the order (or moves D3D12 above D3D11) fails here deterministically.
  const int defaultCandidatesStart = selectorSource.indexOf(QStringLiteral("QVector<RhiBackendCandidate> defaultWindowsCandidates()"));
  const int candidatesForRequestStart = selectorSource.indexOf(QStringLiteral("QVector<RhiBackendCandidate> candidatesForRequest"));
  QVERIFY2(defaultCandidatesStart >= 0 && candidatesForRequestStart > defaultCandidatesStart,
           "D3D12-03/DR-05: RhiBackendSelector default candidate boundaries changed; update the D3D12-03 audit");
  const QString defaultCandidates = selectorSource.mid(defaultCandidatesStart,
                                                       candidatesForRequestStart - defaultCandidatesStart);
  QVERIFY2(defaultCandidates.indexOf(QStringLiteral("Direct3D11"))
               < defaultCandidates.indexOf(QStringLiteral("Direct3D12")),
           "D3D12-03/DR-05: defaultWindowsCandidates() must keep D3D11 before D3D12 (promotion blocked until Phase 106 root cause is confirmed)");
  QVERIFY2(defaultCandidates.indexOf(QStringLiteral("QRhi::D3D11"))
               < defaultCandidates.indexOf(QStringLiteral("QRhi::D3D12")),
           "D3D12-03/DR-05: defaultWindowsCandidates() must keep the D3D11 QRhi implementation before the D3D12 one");

  // D3D12-03 (D3D11-first rationale comment present): the load-bearing
  // D3D11-first comment must stay so a future reader knows the order exists
  // BECAUSE of the unresolved D3D12 crash, not by accident. Removing the
  // comment would let a refactor reorder the candidates without understanding
  // the consequence.
  QVERIFY2(defaultCandidates.contains(QStringLiteral("D3D11-first")),
           "D3D12-03: defaultWindowsCandidates() must keep the D3D11-first rationale comment (documents WHY the order is load-bearing)");

  // D3D12-03 (opt-in only): D3D12 must remain reachable ONLY via the explicit
  // OWZX_RHI_RENDERER=d3d12 opt-in (candidatesForRequest exact-match), never
  // via the default `auto` selection. The selector must keep reading
  // OWZX_RHI_RENDERER as the gate.
  QVERIFY2(selectorSource.contains(QStringLiteral("OWZX_RHI_RENDERER")),
           "D3D12-03: RhiBackendSelector must keep OWZX_RHI_RENDERER as the D3D12 opt-in gate");

  // D3D12-03 (no Vulkan in the default selector): Vulkan is SDK-blocked (Qt
  // disables the `vulkan` public feature) and must not appear in the default
  // Windows candidate list. This locks the "Vulkan is evaluation-only, not a
  // v4.5 deliverable" part of D3D12-03.
  QVERIFY2(!defaultCandidates.contains(QStringLiteral("QRhi::Vulkan")),
           "D3D12-03: defaultWindowsCandidates() must not include Vulkan (SDK-blocked; PROJECT.md:143)");
}

QTEST_MAIN(QmlUiAuditTests)
#include "QmlUiAuditTests.moc"
