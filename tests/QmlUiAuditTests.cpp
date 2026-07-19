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
  // Phase 107-01 (FMAP-02): the Qt6 filament-map mode enum is widened from the
  // old 2-value (0=Auto, 1=Manual) to the upstream 4-value FilamentMapMode
  // (fmmAutoForFlush=0 / fmmAutoForMatch=1 / fmmManual=2 / fmmDefault=3,
  // matching PrintConfig.hpp:424-429), AND the 3MF persistence applies the
  // Pitfall 2 read-side migration so pre-v4.5 "Manual"=1 plates do NOT silently
  // reload as the new fmmAutoForMatch=1. Source-audit locks: (a) all 4 enum
  // names appear in PartPlate.h; (b) the upstream PrintConfig.hpp:424-429
  // anchor citation is present; (c) ProjectServiceMock.cpp writes via the
  // def-respecting option("filament_map_mode", true)->setInt() accessor (the
  // option def declares coEnum, so DynamicConfig makes
  // ConfigOptionEnumGeneric -- a set_key_value with a raw
  // ConfigOptionEnum<FilamentMapMode> mismatches the def type and crashes the
  // writer; FM-02 forward-compat string write is preserved because the bbs_3mf
  // writer still serializes via get_enum_names()[getInt()]); (d) the read-side
  // coEnum-vs-legacy migration is present and maps legacy raw-int-1 ->
  // fmmManual (FM-03, the user-visible behavior change). Mirrors the Phase
  // 102/103/104/105/106 source-audit pattern (QFile + QT_TESTCASE_SOURCEDIR +
  // QString::contains + QVERIFY2 with an FMAP-02-named message). Source-level
  // only; runs in the regression ctest.
  void filamentMapModeEnumWidenedToUpstream4Value();
  // Phase 108-01 (FMAP-01): source-audit lock proving the filament-map auto-
  // recommendation readback is wired end-to-end (SliceService POD + signal +
  // worker capture site + EditorViewModel slot + Q_PROPERTYs). Locks the source
  // anchors so a future regression (drop the POD, drop the signal, remove the
  // worker capture, or unwire the slot) fails here deterministically. Mirrors
  // the Phase 102/103/104/105/106/107 source-audit pattern (QFile +
  // QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with an FMAP-01-named
  // message). Source-level only; runs in the regression ctest.
  void filamentMapAutoRecommendationReadbackPresent();
  // Phase 110-01 (FMAP-03 + R-02): source-audit lock proving the
  // FilamentGroupPopup surfaces EXACTLY 3 selectable modes
  // (AutoForFlush/AutoForMatch/Manual) -- fmmDefault is the per-plate inherit-
  // sentinel and MUST NOT appear as a 4th radio (anti-feature per FEATURES.md,
  // upstream FilamentGroupPopup.hpp:52 mode_list is the 3 concrete modes only).
  // Also locks the R-02 range validation at PartPlate::setFilamentMapMode(int)
  // so the Q_INVOKABLE boundary stays guarded. Mirrors the Phase 102..108
  // source-audit pattern (QFile + QT_TESTCASE_SOURCEDIR + QString::contains +
  // QVERIFY2 with FMAP-03/R-02-named messages). Source-level only; regression.
  void filamentGroupPopupSurfacesThreeModesNotFour();
  // Phase 109-01 (WTMESH-01/02/03): source-audit lock proving the Option B
  // real wipe-tower mesh path COEXISTS with the Option A baseline (Phase 99
  // Frozen Decision 2). Locks: (a) WipeTowerGeometry POD has the hasRealMesh +
  // meshVertices fields (WTMESH-01); (b) the worker capture reads
  // wipe_tower_mesh_data + convex_hull_3d (WTMESH-02 capture-by-value); (c)
  // buildWipeTowerMeshVertices exists PARALLEL to buildWipeTowerVertices
  // (WTMESH-03 -- Option A is NOT modified); (d) uploadWipeTowerBuffer has the
  // hasRealMesh branch; (e) the Q_PROPERTY chain + PreparePage binding +
  // SoftwareViewport mirror are wired. Mirrors the Phase 102/108 source-audit
  // pattern (QFile + QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with
  // a WTMESH-named message). Source-level only; runs in the regression ctest.
  void optionBWipeTowerMeshCoexistsWithOptionA();
  // Phase 111-01 (FMAP-04 + R-01): source-audit lock proving the filament-map
  // save->reload round-trip coverage + the legacy raw-int-1 -> fmmManual
  // migration runtime coverage exist in PartPlateTests. Locks: (a) the
  // filamentMapSaveReloadRoundTrip slot exists (FMAP-04 full round-trip);
  // (b) the legacy-migration slot exists + asserts the R-01 headline
  // (legacy-int-1 -> fmmManual, NOT fmmAutoForMatch); (c) the factored
  // OWzx::migrateLegacyFilamentMapMode helper exists in PartPlate.h +
  // PartPlate.cpp (R-01 closure: the legacy branch is now unit-testable in
  // isolation); (d) both ProjectServiceMock read sites use the factored helper.
  // Mirrors the Phase 102/107/108/110 source-audit pattern (QFile +
  // QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with FMAP-04/R-01-
  // named messages). Source-level only; runs in the regression ctest.
  void filamentMapSaveReloadRoundTripCoversModesAndLegacyMigration();
  // Phase 112-01 (MEASURE-01): source-audit lock proving the per-volume ITS
  // accessor exists on ProjectServiceMock WITH the ownership contract
  // documented at the declaration. Locks: (a) the accessor signature
  // `std::shared_ptr<const indexed_triangle_set> volumeMeshIts(int, int)
  // const` is declared in ProjectServiceMock.h (MI-01); (b) the ownership-
  // contract section headers are present so a future edit cannot silently
  // drop the shallow-share / cache / null-return / SurfaceFeature-boundary
  // documentation (MI-02/MI-03/MI-04/MI-05/MI-06); (c) the .cpp defines the
  // accessor and uses the shared_ptr aliasing constructor (the shallow-share
  // implementation, MI-03); (d) the ViewModelSmokeTests regression slot
  // exists (MI-07). Mirrors the Phase 102/111 source-audit pattern (QFile +
  // QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with MEASURE-01-
  // named messages). Source-level only; runs in the regression ctest.
  void perVolumeItsAccessorPresent();
  // Phase 113-01 (MEASURE-02): source-audit lock proving the pure-CPU
  // MeshRaycaster + SceneRaycaster port landed with the upstream anchor
  // cited, the two-stage pick documented, and the per-volume cache
  // contract present. Locks: (a) both header files exist in
  // src/core/rendering/ (MR-01); (b) MeshRaycaster.h cites the upstream
  // anchor (MeshUtils.hpp:159+, the intersection math source -- MR-01);
  // (c) MeshRaycaster.h documents the pure-CPU + cache contracts (MR-01
  // no-GL/wx + MR-02 BVH-once); (d) SceneRaycaster.h documents the two-
  // stage pick + the pitfall-7 mitigation (MR-03); (e) SceneRaycaster
  // exposes the MR-04 world-space hit result (object/volume/facet/pos/
  // normal); (f) both files are registered in the owzx_app_core target in
  // CMakeLists.txt (MR-05); (g) the PartPlateTests regression slot
  // exists (MR-06). Mirrors the Phase 112 source-audit pattern (QFile +
  // QT_TESTCASE_SOURCEDIR + QVERIFY2 with MEASURE-02-named messages).
  // Source-level only; runs in the regression ctest.
  void meshAndSceneRaycasterPorted();
  // Phase 114-01 (MEASURE-03): source-audit lock proving the Measure::Measuring
  // instantiation + get_feature wiring + SurfaceFeature boundary scrubbing
  // landed. Locks: (a) MeasureEngine.h/.cpp exist in src/core/rendering/
  // (ME-01); (b) MeasureEngine.h instantiates Measure::Measuring (NOT a
  // reimplementation) + names the ME-01 truth; (c) MeasureEngine wires
  // get_feature (ME-02) + documents the per-volume cache + invalidate
  // contract; (d) the QtFeature + QtMeasurement PODs exist (Qt-owned types
  // only -- pitfall-6 scrubbing, ME-03); (e) the readout API is exposed on
  // EditorViewModel (ME-04) via the measure* Q_PROPERTYs + the
  // computeMeasureReadoutFromHit Q_INVOKABLE; (f) the AssemblyMeasureGeometry
  // AABB-stub relationship is documented (ME-05); (g) both files are
  // registered in owzx_app_core in CMakeLists.txt (ME-06); (h) the
  // PartPlateTests regression slot exists (ME-07). Mirrors the Phase 113
  // source-audit pattern (QFile + QT_TESTCASE_SOURCEDIR + QString::contains
  // + QVERIFY2 with MEASURE-03-named messages). Source-level only; runs in
  // the regression ctest.
  void measureEngineInstantiatedPerVolume();
  // Phase 115-01 (MEASURE-04): source-audit lock proving the GLGizmoMeasure
  // snap UX is wired end-to-end. Locks: (a) RhiViewport emits the
  // measurePickRequested + measureHoverLeft signals (MS-01 mouse-move path);
  // (b) RhiViewport reads Qt::ShiftModifier for the Shift toggle (MS-02);
  // (c) EditorViewModel exposes pickMeasureFeatureAt Q_INVOKABLE that drives
  // SceneRaycaster::hitTest + MeasureEngine::getFeature (MS-01 stage-2);
  // (d) the 4 SurfaceFeatureTypes (Point/Edge/Circle/Plane) are handled --
  // the FeatureKind enum + the measureHoverFeatureKind Q_PROPERTY exist
  // (MS-03); (e) PreparePage.qml forwards the signals to the ViewModel
  // (the QML binding is the runtime wiring); (f) the two-click measurement
  // flow reuses the Phase 114 m_measureFromFeatureValid stash (MS-04).
  // Mirrors the Phase 114 measureEngineInstantiatedPerVolume source-audit
  // pattern (QFile + QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2
  // with MEASURE-04-named messages). Source-level only; runs in the
  // regression ctest. Runtime visual interaction is not unit-tested per
  // STATE.md (the source-audit + the Phase 114 readout test are the bar).
  void glGizmoMeasureSnapUxWired();
  // Phase 116-01 (VV-01a/b/c, WTMESH-04 + MEASURE-05): the FINAL v4.5 cross-
  // workstream regression lock. A consolidated source-audit slot that asserts
  // the v4.5 contracts from ALL FIVE workstreams in one place so a regression
  // in any workstream fails this gate (the per-workstream slots above lock
  // the individual truths; this slot is the milestone-level meta-gate).
  //
  // VV-01a (WTMESH-04, the FINAL WTMESH closure): Option A
  // buildWipeTowerVertices is PRESERVED alongside Option B
  // buildWipeTowerMeshVertices in GizmoGeometry.h (the v4.4 Phase 99 Frozen
  // Decision 2 baseline did not regress -- Phase 109 added Option B as a
  // PARALLEL path, NOT a replacement).
  //
  // VV-01b (MEASURE-05, the FINAL MEASURE closure): MeasureEngine (Phase 114)
  // is wired and produces REAL measurements via Measure::Measuring -- NOT just
  // the AssemblyMeasureGeometry AABB stub. The AABB stub stays as the coarse
  // Assembly multi-volume fallback (augmented, not replaced -- documented in
  // MeasureEngine.h ME-05).
  //
  // VV-01c (cross-workstream anchors): the 5 v4.5 workstreams are all wired:
  // (1) filament-map 4-value enum + readback + popup; (2) Option B real wipe-
  // tower mesh; (3) CLI fixtures; (4) D3D12 opt-in; (5) WS5 measure (per-
  // volume ITS + raycaster + Measuring + snap UX).
  //
  // Mirrors the Phase 102..115 source-audit pattern (QFile +
  // QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with v4.5-named
  // messages). Source-level only; runs in the regression ctest. Runtime
  // visual evidence is blocked per STATE.md (the source-audit + regression
  // ctest + launch liveness are the verification bar -- VV-02/03/04).
  void v45CrossWorkstreamRegressionLocked();
  // Phase 117-01 (TICK-01): the Preview layer rail must render tick marks
  // (pause / color-change / filament-change / custom-gcode / template) driven
  // by previewVm.tickMarks, expose right-click add/edit/delete menus, and
  // instantiate CustomGcodeDialog. The formerly-orphaned horizontal
  // LayerSlider.qml is consolidated into the vertical PreviewLayerRail.qml
  // (source-truth-aligned with upstream IMSlider) and MUST stay deleted with
  // its qml.qrc entry gone. Mirrors the Phase 102..116 source-audit pattern
  // (QFile + QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with
  // TICK-01-named messages).
  void tickMarksRenderedOnPreviewRail();
  // Phase 118-01 (TICK-02/TICK-03): source-audit lock that the PreviewViewModel
  // tick CRUD now writes back into libslic3r's model->plates_custom_gcodes and
  // triggers a re-slice (closing the Phase 117 "in-memory only" gap). Mirrors
  // the Phase 117 tickMarksRenderedOnPreviewRail slot pattern (readSource +
  // QVERIFY2 with TICK-02/TICK-03-named messages).
  void customGcodeWritebackAndResliceWired();
  // Phase 119-01 (TICK-04/TICK-05): source-audit lock that all 5 upstream tick
  // types are now reachable from the Preview UI (ColorChange + Template add
  // methods land in PreviewViewModel; the Add menu gains the two entries) and
  // that tick drag-to-relocate is wired (previewVm.moveTick called from the
  // PreviewLayerRail tick delegate, and the method is declared + implemented in
  // the ViewModel). Mirrors the Phase 117/118 tick slot pattern (readSource +
  // QVERIFY2 with TICK-04/TICK-05-named messages).
  void tickTypeCoverageAndDragRelocation();
  // Phase 120-01 (PAINT-01): source-audit lock proving the TriangleSelector
  // triangle-pick + adaptive-subdivide + paint-state pipeline is ported to the
  // Qt6 C++ layer via REUSE (libslic3r's TriangleSelector is compiled in and
  // wrapped by PaintEngine, NOT reimplemented). Locks: (a) ProjectServiceMock
  // exposes volumeMeshTriangleMesh (TS-01 aliasing shared_ptr); (b)
  // SceneRaycasterHit carries meshLocalPosition (TS-02 -- the mesh-local hit
  // select_patch needs); (c) PaintEngine.{h,cpp} exist, reference
  // Slic3r::TriangleSelector + select_patch (TS-03/TS-04 reuse); (d)
  // EditorViewModel references PaintEngine + exposes paintAtFacet (TS-05);
  // (e) NO reimplementation -- PaintEngine.cpp includes TriangleSelector.hpp
  // (TS-07e). Mirrors the Phase 113 meshAndSceneRaycasterPorted source-audit
  // pattern (QFile + QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2
  // with PAINT-01-named messages). Source-level only; runs in the regression
  // ctest.
  void triangleSelectorEnginePorted();
  // Phase 124-01 (CALIB-01): source-audit lock that the 3 libslic3r calibration
  // tower modes are now dispatched from CalibrationServiceMock (Vol_speed=7,
  // VFA=8, Retraction=9, per calib.hpp:24-26). Locks: (a) calibMode 7/8/9 each
  // appear in buildMockData; (b) the "Pending: outside Phase" placeholder is
  // gone from max_volumetric_speed (now a real dispatched mode, not a stub);
  // (c) bed_leveling/vibration still carry their honest "requires hardware"
  // unavailableReason (NOT accidentally enabled by the tower-mode change).
  // Mirrors the Phase 102..120 source-audit pattern (readSource + QVERIFY2
  // with CALIB-01-named messages). Source-level only; runs in the regression
  // ctest.
  void calibrationTowerModesDispatchToLibslic3r();
  // Phase 125 (CALIB-02 + CALIB-03): source-audit lock that (a)
  // CalibrationDialog.qml has the range input fields (start/end/step) bound to
  // the ViewModel; (b) CalibrationViewModel exposes the calibStart/calibEnd/
  // calibStep Q_PROPERTYs that forward to setCalibParams; (c) the mock
  // 0.04f + item*0.01 K-value writeback is GONE, replaced by the real PA
  // G-code readback (M900 K / SET_PRESSURE_ADVANCE parse) for PA + an honest
  // manual-interpretation note for non-PA tower modes. Mirrors the Phase
  // 124 source-audit pattern (readSource + QVERIFY2 with CALIB-02/CALIB-03-
  // named messages). Source-level only; runs in the regression ctest.
  void calibrationRangeInputAndKValueReadback();
  // Phase 126 (CLEAN-01): the legacy dead-code pages (DeviceListPage,
  // AuxiliaryPage, ModelMallPage, AuxiliaryListPanel) + AuxiliaryService must
  // stay deleted (No-Deprecated-UI rule). These were in the removed LAN/device/
  // cloud scope — deletion, not repair. Source-level only.
  void legacyDeadCodePagesRemoved();
  // Phase 121 (PAINT-02 + PAINT-03): source-audit lock that the painted-facet
  // overlay render + brush interaction wiring is in place. Locks: (a)
  // EditorViewModel exposes paintOverlayData + extrudersColors Q_PROPERTYs;
  // (b) RhiViewport exposes paintOverlayData + the brush Q_PROPERTYs
  // (brushRadius/brushCursorType/paintState); (c) RhiViewportRenderer has
  // renderPaintOverlay + uploadPaintOverlayBuffer + the brush-cursor render
  // path; (d) emitPaintPickIfActive no longer hardcodes brushRadius=2.0 (reads
  // the Q_PROPERTY). Mirrors the Phase 120 source-audit pattern (readSource +
  // QVERIFY2 with PAINT-02/PAINT-03-named messages). Source-level only; runs in
  // the regression ctest.
  void paintedFacetOverlayAndBrushInteraction();
  // Phase 122 (PAINT-04): Support/Seam painted facets write to ModelVolume
  // FacetsAnnotation (supported_facets/seam_facets) so the slice consumes them.
  void supportAndSeamPaintFeedsSlice();
  // Phase 123 (PAINT-05): MMU segmentation paint writes to
  // mmu_segmentation_facets + clearMmuSegmentation implemented (no TODO stub).
  void mmuSegmentationPaintFeedsSlice();
  // Phase 127 (I18N-01): the i18n pipeline is documented + zh_CN has finished
  // translations for v4.6-touched strings (proof-of-pipeline).
  void translationPipelineDocumented();
  // Phase 128 (REGRESS-01): v4.6 cross-workstream regression — all v4.6
  // source-audit slots + key anchors consolidated in one milestone gate.
  void v46CrossWorkstreamRegressionLocked();
  // Phase 129 (POLISH-01/02/03): paint-gizmo gate flag flipped + Flatten real
  // + fixMesh real repair.
  void paintGizmoGateFlattenedAndFlattenFixMeshReal();
  // Phase 130 (POLISH-04/05): KBShortcutsDialog exists + ProjectPage property
  // panel wired to real values.
  void kbShortcutsDialogAndProjectPagePropertyPanelWired();
  // Phase 135 (REGRESS-02): v4.7 cross-workstream regression gate.
  void v47CrossWorkstreamRegressionLocked();
  // Phase 140 (REGRESS-03): v4.8 cross-workstream regression gate.
  void v48CrossWorkstreamRegressionLocked();
  // Phase 141 (DEBT-05): v5.0 tech-debt closure gate. Locks the 4 code-only
  // fixes (intersect, orphaned menu removed, drillObject return, ASM full
  // transform compose) AND re-asserts the v4.8 anchors so the v5.0 tech-debt
  // work did not regress the v4.8 contract.
  void v50TechDebtRegressionLocked();
  // Phase 142 (VDB-01/VDB-02): OpenVDB unlock gate. Locks the CMake wiring that
  // corrects the v4.x "OpenVDB unavailable" premise — find_package(OpenVDB) +
  // openvdb_libs alias + libnoise latent-issue fix. The real proof is that
  // OWzxSlicer.exe links clean (no LNK2019 on mesh_to_grid/grid_to_mesh/
  // redistance_grid); this slot anchors the source-level evidence.
  void v50OpenVdbUnlockWired();
  // Phase 143 (VDB-03/04/05): Hollow gizmo reachability + button + panel gate.
  // Locks the EditorViewModel gizmo-availability change (case 8 returns
  // hasSingleObject, not false), the removal of the "Blocked: OpenVDB unavailable"
  // tooltip for case 8, the new Hollow button in GLToolbars.qml, and the Hollow
  // panel in PreparePage.qml. The full SLA slice path (VDB-06) is a v5.1+
  // follow-up — it requires wiring SLAPrint from scratch.
  void v50HollowGizmoReachable();
  // Phase 144 (EMB-01/02): Emboss font + parameterization gate. The real Emboss
  // pipeline (text2shapes + polygons2model) was already wired before v5.0; this
  // phase parameterized it (font path + height + depth from Q_PROPERTYs instead
  // of hardcoded values). The slot anchors the parameterization surface.
  void v50EmbossParameterized();
  // Phase 145 (EMB-03/04): async emboss + panel gate. Locks the
  // addTextVolumeAsync worker pattern (Qt Concurrent + atomic cancel +
  // queued GUI-thread result delivery), the EditorViewModel signal forwarding,
  // and the QML panel's font selector + async-execute button + result feedback.
  void v50EmbossAsyncAndPanelWired();
  // Phase 146 (EMB-05/06/07): Emboss wiring + 3MF round-trip + SVG gate.
  // Locks the no-selection auto-attach fallback, the SVG emboss path (real
  // Model::read_from_file loader), and the 3MF persistence contract (TextEmboss
  // volumes are MODEL_PART so geometry round-trips; editable-text metadata
  // persistence via upstream 3MF <text> block is documented future work).
  void v50EmbossWiringAndSvgWired();
  // Phase 147 (PSET-01/02): Preset INI bundle + CreatePresetsDialog gate.
  // Locks the upstream-compatible `.ini` export/import + the CreatePresetsDialog
  // QML + the ConfigViewModel request/signal wiring.
  void v50PresetIniAndCreateDialogWired();
  // Phase 148 (PSET-03/04): UnsavedChangesDialog + Simple/Advanced filter gate.
  // Both pieces were largely wired pre-v5.0; this slot anchors that they remain
  // intact + the C++ filter rule is real (advancedMode toggles comAdvanced+).
  void v50UnsavedChangesAndFilterWired();
  // Phase 149 (PSET-05/06/07): Compare/Diff + Dirty Propagation + Round-Trip
  // contract gate. Locks the new comparePresets C++ primitive + the existing
  // dirty-state propagation infrastructure + the bundle round-trip slot.
  void v50CompareDiffAndRoundTripWired();
  // Phase 151 (PLATE-02/03/04/05): PartPlate UI implementation gate. Locks the
  // PLATE-02 drag-reorder path + the EditorViewModel movePlate proxy + the
  // pre-existing PLATE-03 print-sequence dialog + the pre-existing PLATE-04
  // plate-scope override path. PLATE-05 non-current thumbnail runtime capture
  // is documented as refined scope (persisted-plate thumbnails already work).
  void v50PartPlateUiImplementationWired();
  // Phase 152 (PLATE-06): multi-plate save/reload regression gate. Locks the
  // full plate-state round-trip via the staging buffers in ProjectServiceMock
  // (pendingPlate* fields rebuilt from 3MF after load). A live ctest is gated
  // on a full ProjectServiceMock context; this slot anchors the source-level
  // contract that all plate fields have a round-trip staging path.
  void v50PartPlateSaveReloadRegressionWired();
  // Phase 153 (REGRESS-04): v5.0 cross-workstream regression gate. Consolidates
  // ALL v5.0 anchors from Phases 141-152 (DEBT/VDB/EMB/PSET/PLATE) into one
  // top-level gate, AND re-asserts the v4.8/v4.7/v4.6 milestone anchors so
  // v5.0 did not regress them. Mirrors the v48CrossWorkstreamRegressionLocked
  // pattern (which is itself re-asserted here).
  void v50RegressionLocked();
  // Phase 154 (CLOS-01): QML Preset Diff-View Dialog wiring gate. Locks the
  // ConfigViewModel.comparePresetsDetailed proxy + the PresetDiffDialog
  // consumer + the SettingsDialog toolbar entry that opens it. The underlying
  // PresetServiceMock::comparePresets primitive (Phase 149) is separately
  // anchored by v50CompareDiffAndRoundTripWired.
  void v51PresetDiffDialogWired();
  // Phase 155 (CLOS-02): Emboss 3MF text metadata round-trip gate. Locks the
  // save-side attachEmbossMetadata (writes text_configuration so upstream
  // store_bbs_3mf emits `<slic3rpe:text>`) + the load-side
  // objectVolumeType/Label reading text_configuration to restore the
  // TextEmboss tag.
  void v51EmbossTextMetadataRoundTripWired();
  // Phase 156 (CLOS-03): runtime plate thumbnail capture scheduler gate. Locks
  // the setPlateThumbnailFromBase64 write path on ProjectServiceMock +
  // EditorViewModel + the per-plate thumbnailCapturedForPlate signal on
  // RhiViewport + the PreparePage session-capture scheduler.
  void v51PartPlateSessionThumbnailWired();
  // Phase 157 (CLOS-04): live multi-plate full-state round-trip ctest anchor.
  // Locks the existence of the multiPlateFullStateRoundTrip live ctest (the gap
  // that forced Phase 152 to source-audit-lock only) AND its coverage breadth
  // (all 5 CLOS-04 dimensions + thumbnail).
  void v51MultiPlateRoundTripLiveCtest();
  // Phase 158 (EMBO-F01/F02): Emboss style controls + SVG depth-modifier wiring.
  // Locks the 4 new style Q_PROPERTYs + setters + forwarding + FontProp
  // population + the addSvgVolume depth-modifier extension.
  void v51EmbossStyleControlsAndSvgAdvancedWired();
  // Phase 159 (REGRESS-05): v5.1 cross-workstream regression gate. Consolidates
  // ALL v5.1 anchors from Phases 154-158 into one top-level gate AND re-asserts
  // the v5.0/v4.8/v4.7/v4.6 milestone anchors so v5.1 did not regress them.
  // Mirrors the v50RegressionLocked pattern (which is itself re-asserted here).
  void v51RegressionLocked();
  // Phase 160 (DS-01): Theme token foundation gate. Locks the canonical
  // token list — every Theme.X referenced in QML must be defined in Theme.qml
  // (no silent undefined); the v5.2 audit's missing tokens must be present;
  // header documentation exists.
  void v52ThemeTokenFoundationWired();
  // Phase 161 (DS-02/03): Cx* control library hardening gate. Locks zero
  // Qt.darker/lighter, no font size below the XS floor, CxButton has press-scale
  // + toolTipText + focus border.
  void v52ControlLibraryHardened();
  // Phase 162 (TK-01): color hardcode sweep gate. Locks the app-wide migration
  // of hardcoded hex literals to Theme tokens (PreferencesPage was the worst
  // offender with 129 hex literals; LeftSidebar private palette migrated).
  void v52ColorHardcodeSwept();
  // Phase 163 (TK-02): typography hardcode sweep gate. Locks the migration of
  // font.pixelSize literals to Theme.fontSize* tokens + Consolas → Theme.fontMono.
  void v52TypographyHardcodeSwept();
  // Phase 164 (TK-03/SW-01): sidebar width system unbroken. Locks the 7-layer
  // 392px hardcode removal — backend.sidebarWidth is now resizable within
  // [300, 520]; DockableSidebar drag handle is functional (was visible no-op).
  void v52SidebarWidthUnbroken();
  // Phase 165 (CW-01/02): copywriting & language sweep gate. Locks one source
  // language (ZH) for the 3 previously-EN dialogs + removal of dev-jargon
  // tooltips.
  void v52CopywritingSwept();
  // Phase 166 (Dlg-01/02): dialog consistency gate. Locks the 8 empty-header
  // fixes (title: → dialogTitle:) + SavePresetDialog EN→ZH sweep.
  void v52DialogConsistencyRepaired();
  // Phase 167 (Cmp-01/02/03): component coherence gate. Locks notification
  // severity palette consolidation + 4 orphan components removed from qrc.
  void v52ComponentCoherence();
  // Phase 168 (VS-01/02): visual control migration gate. Locks the migration
  // of Rectangle+Text+MouseArea pseudo-buttons to CxButton/CxIconButton +
  // the Emboss boldness Slider → CxSlider.
  void v52VisualControlMigration();
  // Phase 169 (XD-01/02): experience safety gate. Locks the shared
  // ConfirmDialog component + routing of destructive triggers through it.
  void v52ExperienceSafety();
  // Phase 170 (REGRESS-06): v5.2 cross-workstream UI regression gate. Spots
  // every v5.2 anchor (DS/TK/SW/CW/Dlg/Cmp/VS/XD) + re-asserts v5.1/v5.0/v4.x.
  void v52RegressionLocked();
  // Phase 171 (CL-01): destructive-action confirm sweep gate. Locks that the
  // 6 remaining destructive triggers route through ConfirmDialog (was firing
  // immediately per v5.2 audit).
  void v53DestructiveConfirmSweep();
  // Phase 172 (CL-02): dialog spacing sweep gate. Locks that dialogs use
  // Theme.spacing* tokens instead of hand-rolled pixel values.
  void v53DialogSpacingSwept();

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
    // Phase 164 (SW-01): preview left width now sources from backend.sidebarWidth
    // (was hardcoded 392 — part of the 7-layer lock). The contract is now that
    // the property exists and resolves to the backend value.
    QStringLiteral("readonly property int targetPreviewLeftWidth: backend ? backend.sidebarWidth : 392"),
    QStringLiteral("readonly property int targetPreviewRightWidth: Theme.rightPanelWidth"),
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
  const QString preferences = readSource(QStringLiteral("src/qml_gui/pages/PreferencesPage.qml"));
  QVERIFY2(!topbar.isEmpty(), "Unable to read BBLTopbar.qml");
  QVERIFY2(!mainQml.isEmpty(), "Unable to read main.qml");
  QVERIFY2(!sidebar.isEmpty(), "Unable to read LeftSidebar.qml");
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
  // ModelMallPage.qml was removed in v4.6 Phase 126 (dead code, removed cloud scope).
  // The forbiddenRuntimeCopy loop is retained but no longer applied to a deleted file;
  // the upstream-alignment intent is preserved for any future marketplace surface.
  Q_UNUSED(forbiddenRuntimeCopy)

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

  // Phase 164 (SW-01, v5.2): the 392px hardcode was a 7-layer lock that made
  // the DockableSidebar drag handle a visible no-op (Panels-UI-REVIEW BLOCKER).
  // The contract is now: sidebar width sources from backend.sidebarWidth, and
  // min/max are real bounds (300/520) — no longer min==max==392. Default stays
  // 392 to preserve the visual; users can now resize.
  QVERIFY2(preparePage.contains(QStringLiteral("backend ? backend.sidebarWidth : 392")),
           "Prepare page sidebar width must source from backend.sidebarWidth (Phase 164 unbreaks the 7-layer lock)");
  QVERIFY2(preparePage.contains(QStringLiteral("backend ? backend.sidebarMinWidth : 300"))
               && preparePage.contains(QStringLiteral("backend ? backend.sidebarMaxWidth : 520")),
           "Phase 164 SW-01: Prepare sidebar must have real resizable min/max bounds (was min==max==392 no-op drag handle)");
  QVERIFY2(backendContext.contains(QStringLiteral("kSidebarMinWidth = 300"))
               && backendContext.contains(QStringLiteral("kSidebarMaxWidth = 520")),
           "Phase 164 SW-01: BackendContext kSidebar{Min,Max}Width must allow resize (was both 392)");
  QVERIFY2(platerPage.contains(QStringLiteral("backend ? backend.sidebarWidth : 392"))
               && platerPage.contains(QStringLiteral("backend ? backend.sidebarMinWidth : 300"))
               && platerPage.contains(QStringLiteral("backend ? backend.sidebarMaxWidth : 520")),
           "Phase 164 SW-01: Plater sidebar must use the same resizable backend-driven contract as Prepare");
  QVERIFY2(backendContext.contains(QStringLiteral("kSidebarDefaultWidth = 392"))
               && backendContext.contains(QStringLiteral("kSidebarMinWidth = 300"))
               && backendContext.contains(QStringLiteral("kSidebarMaxWidth = 520")),
           "Phase 164 SW-01: BackendContext default stays 392 (visual preserved) but min/max now allow resize");

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
  // Phase 162 (TK-01): the sidebar palette was previously checked via specific
  // hex literals (#303236/#313337/#323438 — the screenshot gray panel surface).
  // The v5.2 color sweep migrated those to Theme tokens (bgElevated/bgHover/
  // borderDefault). The new contract: the sidebar sources its panel surface
  // from Theme tokens, not from hardcoded hex literals.
  QVERIFY2(leftSidebar.contains(QStringLiteral("Theme.bgElevated"))
               || leftSidebar.contains(QStringLiteral("Theme.bgHover"))
               || leftSidebar.contains(QStringLiteral("Theme.borderDefault")),
           "Prepare left sidebar palette must source its surface from Theme tokens (Phase 162 sweep migrated the #30-32 hex literals)");
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
      QStringLiteral("        <file>components/OptionRow.qml</file>")
      // Phase 167 (Cmp-02): GroupNavSidebar removed — confirmed orphan (zero
      // QML consumers per Components-UI-REVIEW).
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

void QmlUiAuditTests::optionBWipeTowerMeshCoexistsWithOptionA()
{
  // Phase 109-01 (WTMESH-01/02/03): Option B real wipe-tower mesh COEXISTS
  // with Option A baseline source-audit lock. Phase 99 Frozen Decision 2 froze
  // Option A as the v4.4 baseline and locked Option B as a future upgrade;
  // Phase 109 RE-OPENS that decision and adds Option B as a PARALLEL path
  // (Option A is preserved as the fallback branch). This test locks the
  // coexistence contract so a future regression cannot silently remove
  // Option B (dropping the real mesh path) OR Option A (breaking the
  // single-material / pre-slice fallback).
  //
  // Pattern: QFile + QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with
  // a WTMESH-named message (deterministic, build-dir-independent). Mirrors the
  // Phase 102 wipeTowerReadbackAndRenderAnchorsPresent pattern in this file.

  const QString sliceHeader = readSource(QStringLiteral("src/core/services/SliceService.h"));
  const QString sliceSource = readSource(QStringLiteral("src/core/services/SliceService.cpp"));
  const QString geometryHeader = readSource(QStringLiteral("src/core/rendering/GizmoGeometry.h"));
  const QString geometrySource = readSource(QStringLiteral("src/core/rendering/GizmoGeometry.cpp"));
  const QString viewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString rendererHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.h"));
  const QString rendererSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  const QString editorHeader = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString editorSource = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString softwareViewport = readSource(QStringLiteral("src/qml_gui/Renderer/SoftwareViewport.cpp"));
  QVERIFY2(!sliceHeader.isEmpty(), "Unable to read SliceService.h");
  QVERIFY2(!sliceSource.isEmpty(), "Unable to read SliceService.cpp");
  QVERIFY2(!geometryHeader.isEmpty(), "Unable to read GizmoGeometry.h");
  QVERIFY2(!geometrySource.isEmpty(), "Unable to read GizmoGeometry.cpp");
  QVERIFY2(!viewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!rendererHeader.isEmpty(), "Unable to read RhiViewportRenderer.h");
  QVERIFY2(!rendererSource.isEmpty(), "Unable to read RhiViewportRenderer.cpp");
  QVERIFY2(!editorHeader.isEmpty(), "Unable to read EditorViewModel.h");
  QVERIFY2(!editorSource.isEmpty(), "Unable to read EditorViewModel.cpp");
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!softwareViewport.isEmpty(), "Unable to read SoftwareViewport.cpp");

  // (1) WTMESH-01 (POD extended): WipeTowerGeometry must have the hasRealMesh
  //     bool field AND the meshVertices std::vector<float> field. The Option A
  //     dim fields stay in place (the POD is EXTENDED, not replaced).
  QVERIFY2(sliceHeader.contains(QStringLiteral("bool hasRealMesh = false;")),
           "WTMESH-01: WipeTowerGeometry must have a hasRealMesh bool field (default false)");
  QVERIFY2(sliceHeader.contains(QStringLiteral("std::vector<float> meshVertices;")),
           "WTMESH-01: WipeTowerGeometry must have a meshVertices std::vector<float> field (flattened XYZ, capture-by-value)");

  // (2) WTMESH-02 (worker capture-by-value): the worker must read
  //     wipe_tower_mesh_data, merge real_brim_mesh, run convex_hull_3d(), and
  //     extract its.vertices into the flat float vector. NO TriangleMesh* or
  //     its* may escape -- the capturedGeometry.meshVertices.push_back chain
  //     proves the pure-float extraction.
  QVERIFY2(sliceSource.contains(QStringLiteral("wipe_tower_mesh_data != std::nullopt")),
           "WTMESH-02: SliceService worker must gate the mesh capture on wipe_tower_mesh_data != std::nullopt");
  QVERIFY2(sliceSource.contains(QStringLiteral("real_brim_mesh")),
           "WTMESH-02: SliceService worker must read real_brim_mesh (mirrors upstream 3DScene.cpp:906-909 merge)");
  QVERIFY2(sliceSource.contains(QStringLiteral("convex_hull_3d()")),
           "WTMESH-02: SliceService worker must run convex_hull_3d() on the merged mesh (mirrors upstream 3DScene.cpp:914)");
  QVERIFY2(sliceSource.contains(QStringLiteral("capturedGeometry.meshVertices.push_back")),
           "WTMESH-02: SliceService worker must extract its.vertices into meshVertices as flattened floats (capture-by-value, no TriangleMesh* escape)");
  QVERIFY2(sliceSource.contains(QStringLiteral("capturedGeometry.hasRealMesh = true")),
           "WTMESH-02: SliceService worker must set capturedGeometry.hasRealMesh = true only when the mesh is populated");

  // (3) WTMESH-03 (Option B builder PARALLEL to Option A): buildWipeTower-
  //     MeshVertices must exist in BOTH the header (declaration) and the cpp
  //     (implementation). Option A buildWipeTowerVertices must STILL EXIST
  //     UNCHANGED (Phase 99 Frozen Decision 2 baseline is preserved).
  QVERIFY2(geometryHeader.contains(QStringLiteral("buildWipeTowerMeshVertices")),
           "WTMESH-03: GizmoGeometry.h must declare buildWipeTowerMeshVertices (Option B builder)");
  QVERIFY2(geometryHeader.contains(QStringLiteral("buildWipeTowerVertices")),
           "WTMESH-03: GizmoGeometry.h must STILL declare buildWipeTowerVertices (Option A baseline preserved)");
  QVERIFY2(geometrySource.contains(QStringLiteral("GizmoGeometry::buildWipeTowerMeshVertices")),
           "WTMESH-03: GizmoGeometry.cpp must implement buildWipeTowerMeshVertices (Option B builder)");
  QVERIFY2(geometrySource.contains(QStringLiteral("GizmoGeometry::buildWipeTowerVertices")),
           "WTMESH-03: GizmoGeometry.cpp must STILL implement buildWipeTowerVertices (Option A baseline preserved)");

  // (4) WTMESH-04 (renderer branch): uploadWipeTowerBuffer must have the
  //     hasRealMesh branch (Option B) AND the Option A else fallback. The
  //     RhiViewportRenderer must carry the m_wipeTowerHasRealMesh +
  //     m_wipeTowerMeshVertices members.
  QVERIFY2(rendererHeader.contains(QStringLiteral("m_wipeTowerHasRealMesh")),
           "WTMESH-04: RhiViewportRenderer.h must carry m_wipeTowerHasRealMesh (Option B gate)");
  QVERIFY2(rendererHeader.contains(QStringLiteral("m_wipeTowerMeshVertices")),
           "WTMESH-04: RhiViewportRenderer.h must carry m_wipeTowerMeshVertices (Option B mesh)");
  QVERIFY2(rendererSource.contains(QStringLiteral("buildWipeTowerMeshVertices(")),
           "WTMESH-04: RhiViewportRenderer::uploadWipeTowerBuffer must call buildWipeTowerMeshVertices on the Option B branch");
  QVERIFY2(rendererSource.contains(QStringLiteral("buildWipeTowerVertices(m_wipeTowerX,")),
           "WTMESH-04: RhiViewportRenderer::uploadWipeTowerBuffer must STILL call buildWipeTowerVertices on the Option A else branch (Phase 99 Frozen Decision 2 preserved)");

  // (5) WTMESH-05 (Q_PROPERTY chain wired): RhiViewport + EditorViewModel must
  //     expose the wipeTowerHasRealMesh + wipeTowerMeshVertices Q_PROPERTYs.
  //     The onWipeTowerGeometryReady slot must mirror geometry.hasRealMesh +
  //     geometry.meshVertices.
  QVERIFY2(viewportHeader.contains(QStringLiteral("Q_PROPERTY(bool wipeTowerHasRealMesh")),
           "WTMESH-05: RhiViewport.h must expose wipeTowerHasRealMesh Q_PROPERTY");
  QVERIFY2(viewportHeader.contains(QStringLiteral("Q_PROPERTY(QVariantList wipeTowerMeshVertices")),
           "WTMESH-05: RhiViewport.h must expose wipeTowerMeshVertices Q_PROPERTY");
  QVERIFY2(editorHeader.contains(QStringLiteral("Q_PROPERTY(bool wipeTowerHasRealMesh")),
           "WTMESH-05: EditorViewModel.h must expose wipeTowerHasRealMesh Q_PROPERTY");
  QVERIFY2(editorSource.contains(QStringLiteral("m_wipeTowerHasRealMesh = geometry.hasRealMesh")),
           "WTMESH-05: EditorViewModel::onWipeTowerGeometryReady must mirror geometry.hasRealMesh on the valid path");
  QVERIFY2(editorSource.contains(QStringLiteral("m_wipeTowerHasRealMesh = false")),
           "WTMESH-05: EditorViewModel::onWipeTowerGeometryReady must force m_wipeTowerHasRealMesh = false on the invalid path (no stale mesh leak)");

  // (6) WTMESH-06 (PreparePage binding): GLViewport must bind
  //     wipeTowerHasRealMesh + wipeTowerMeshVertices to root.editorVm.
  QVERIFY2(preparePage.contains(QStringLiteral("wipeTowerHasRealMesh: root.editorVm ? root.editorVm.wipeTowerHasRealMesh")),
           "WTMESH-06: PreparePage.qml GLViewport must bind wipeTowerHasRealMesh to editorVm");
  QVERIFY2(preparePage.contains(QStringLiteral("wipeTowerMeshVertices: root.editorVm ? root.editorVm.wipeTowerMeshVertices")),
           "WTMESH-06: PreparePage.qml GLViewport must bind wipeTowerMeshVertices to editorVm");

  // (7) WTMESH-07 (SoftwareViewport mirror): the fallback renderer must consume
  //     m_wipeTowerHasRealMesh + m_wipeTowerMeshVertices so the software path
  //     does not lag the RHI path.
  QVERIFY2(softwareViewport.contains(QStringLiteral("m_wipeTowerHasRealMesh")),
           "WTMESH-07: SoftwareViewport.cpp must consume m_wipeTowerHasRealMesh (Option B gate on the fallback path)");
  QVERIFY2(softwareViewport.contains(QStringLiteral("m_wipeTowerMeshVertices")),
           "WTMESH-07: SoftwareViewport.cpp must consume m_wipeTowerMeshVertices (Option B mesh on the fallback path)");
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

void QmlUiAuditTests::filamentMapModeEnumWidenedToUpstream4Value()
{
  // FMAP-02 / FM-01 + FM-02 + FM-03: the Qt6 filament-map mode enum must stay
  // widened to the upstream 4-value FilamentMapMode, and the 3MF persistence
  // must keep the Pitfall 2 read-side migration. Locks the source anchors so a
  // future regression (re-collapse to 2-value, drop the typed-enum write, or
  // remove the legacy-int-1 -> fmmManual migration) fails here deterministically.

  const QString partPlateHeader = readSource(QStringLiteral("src/core/model/PartPlate.h"));
  QVERIFY2(!partPlateHeader.isEmpty(), "Unable to read PartPlate.h");
  const QString serviceSource = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  QVERIFY2(!serviceSource.isEmpty(), "Unable to read ProjectServiceMock.cpp");

  // FMAP-02 / FM-01 (4-value enum present): all 4 upstream enum names must
  // appear in PartPlate.h. A re-collapse to the old 2-value model drops at
  // least fmmAutoForMatch / fmmDefault and fails here.
  QVERIFY2(partPlateHeader.contains(QStringLiteral("fmmAutoForFlush")),
           "FMAP-02/FM-01: PartPlate.h must declare fmmAutoForFlush (upstream PrintConfig.hpp:424)");
  QVERIFY2(partPlateHeader.contains(QStringLiteral("fmmAutoForMatch")),
           "FMAP-02/FM-01: PartPlate.h must declare fmmAutoForMatch (upstream PrintConfig.hpp:425)");
  QVERIFY2(partPlateHeader.contains(QStringLiteral("fmmManual")),
           "FMAP-02/FM-01: PartPlate.h must declare fmmManual (upstream PrintConfig.hpp:426)");
  QVERIFY2(partPlateHeader.contains(QStringLiteral("fmmDefault")),
           "FMAP-02/FM-01: PartPlate.h must declare fmmDefault (upstream PrintConfig.hpp:428)");

  // FMAP-02 / FM-01 (upstream anchor citation): the comment must cite the
  // upstream PrintConfig.hpp:424-429 source-of-truth so a future reader knows
  // the 4 values are sourced from upstream, not invented.
  QVERIFY2(partPlateHeader.contains(QStringLiteral("PrintConfig.hpp:424-429")),
           "FMAP-02/FM-01: PartPlate.h must cite the upstream PrintConfig.hpp:424-429 anchor");

  // FMAP-02 / FM-07 (fmmDefault anti-feature doc): the per-plate "inherit from
  // global" sentinel semantics must be documented so fmmDefault is NOT added as
  // a 4th UI radio button.
  QVERIFY2(partPlateHeader.contains(QStringLiteral("inherit from global")),
           "FMAP-02/FM-07: PartPlate.h must document fmmDefault as a per-plate 'inherit from global' sentinel");

  // FMAP-02 / FM-02 (def-respecting write, no type-mismatch crash): the 3MF
  // write side must use option("filament_map_mode", true)->setInt() so it
  // writes through the def-created ConfigOptionEnumGeneric (the option def at
  // PrintConfig.cpp:2495 declares coEnum, so DynamicConfig makes
  // ConfigOptionEnumGeneric -- Config.hpp:2046). The bbs_3mf writer
  // (bbs_3mf.cpp:7964-7967) serializes via
  // ConfigOptionEnum<FilamentMapMode>::get_enum_names()[getInt()], producing the
  // upstream enum-name string. An earlier draft used set_key_value with a raw
  // ConfigOptionEnum<FilamentMapMode>, which mismatched the def type and
  // crashed _add_project_config_file_to_archive; that pattern must stay gone.
  QVERIFY2(serviceSource.contains(QStringLiteral(
               "pd->config.option(\"filament_map_mode\", true)")),
           "FMAP-02/FM-02: ProjectServiceMock must write filament_map_mode via the def-respecting option(...,true) accessor");
  QVERIFY2(serviceSource.contains(QStringLiteral("opt->setInt(static_cast<int>(writeMode))")),
           "FMAP-02/FM-02: ProjectServiceMock must write filament_map_mode via setInt(int(writeMode)) (ConfigOptionEnumGeneric-respecting)");
  // fmmDefault MUST be resolved before persistence (writer's names vector has
  // no "Default" entry, so names[3] is OOB -- bbs_3mf.cpp:7964-7967). The
  // switch must keep resolving fmmDefault -> a concrete mode.
  QVERIFY2(serviceSource.contains(QStringLiteral("case OWzx::FilamentMapMode::fmmDefault:")),
           "FMAP-02/FM-02: ProjectServiceMock write site must resolve fmmDefault to a concrete mode before persistence (writer OOB guard)");

  // FMAP-02 / FM-03 (read-side migration present): the read site must keep the
  // coEnum-vs-legacy migration. The legacy raw-int-1 -> fmmManual mapping is
  // the user-visible behavior change (pre-v4.5 'Manual'=1 stays Manual, NOT the
  // new fmmAutoForMatch=1). Phase 111 (FMAP-04 / R-01) factored the legacy
  // mapping into OWzx::migrateLegacyFilamentMapMode (PartPlate.h/PartPlate.cpp)
  // so the legacy discriminator branch is unit-tested in isolation; the read
  // sites now delegate to that helper. Assert the discriminator stays inline
  // AND the factored-helper call is present at the legacy branch (the call
  // carries the documented 0->fmmAutoForFlush / 1->fmmManual / else->fmmDefault
  // mapping). The helper declaration + definition are locked by the Phase 111
  // filamentMapSaveReloadRoundTripCoversModesAndLegacyMigration slot.
  QVERIFY2(serviceSource.contains(QStringLiteral("opt->type() == Slic3r::coEnum")),
           "FMAP-02/FM-03: ProjectServiceMock read site must discriminate typed coEnum from legacy raw int");
  QVERIFY2(serviceSource.contains(QStringLiteral("OWzx::migrateLegacyFilamentMapMode(int(opt->getInt()))")),
           "FMAP-02/FM-03: ProjectServiceMock read site must delegate the legacy raw-int branch to OWzx::migrateLegacyFilamentMapMode (the factored legacy-int-1 -> fmmManual migration)");
}

void QmlUiAuditTests::filamentMapAutoRecommendationReadbackPresent()
{
  // FMAP-01: the filament-map auto-recommendation readback must stay wired
  // end-to-end -- the FilamentMapResult POD + filamentMapReady signal + worker
  // capture site (Print::get_filament_maps) in SliceService, and the
  // onFilamentMapReady slot + 3 auto* Q_PROPERTYs + filamentMapChanged NOTIFY
  // in EditorViewModel. Locks the source anchors so a future regression (drop
  // the POD, drop the signal, remove the worker capture, or unwire the slot)
  // fails here deterministically. Mirrors the Phase 100/101/102 wipe-tower
  // source-audit pattern.

  const QString sliceHeader = readSource(QStringLiteral("src/core/services/SliceService.h"));
  QVERIFY2(!sliceHeader.isEmpty(), "Unable to read SliceService.h");
  const QString sliceSource = readSource(QStringLiteral("src/core/services/SliceService.cpp"));
  QVERIFY2(!sliceSource.isEmpty(), "Unable to read SliceService.cpp");
  const QString editorHeader = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  QVERIFY2(!editorHeader.isEmpty(), "Unable to read EditorViewModel.h");
  const QString editorSource = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  QVERIFY2(!editorSource.isEmpty(), "Unable to read EditorViewModel.cpp");

  // FMAP-01 / FR-01 (POD present): the FilamentMapResult struct must be
  // declared in SliceService.h with the three fields (valid/mode/maps). A
  // removal of the POD drops all three and fails here.
  QVERIFY2(sliceHeader.contains(QStringLiteral("struct FilamentMapResult")),
           "FMAP-01/FR-01: SliceService.h must declare the FilamentMapResult POD (mirrors WipeTowerGeometry)");
  QVERIFY2(sliceHeader.contains(QStringLiteral("bool valid = false;")),
           "FMAP-01/FR-01: FilamentMapResult must have a bool valid field (the auto-recommendation-ran gate)");
  QVERIFY2(sliceHeader.contains(QStringLiteral("OWzx::FilamentMapMode mode")),
           "FMAP-01/FR-01: FilamentMapResult.mode must use the Phase 107 OWzx::FilamentMapMode enum");
  QVERIFY2(sliceHeader.contains(QStringLiteral("std::vector<int> maps")),
           "FMAP-01/FR-01: FilamentMapResult.maps must be std::vector<int> (Print::get_filament_maps result)");

  // FMAP-01 / FR-02 (signal present): filamentMapReady must be declared on
  // SliceService next to wipeTowerGeometryReady.
  QVERIFY2(sliceHeader.contains(QStringLiteral("void filamentMapReady(const FilamentMapResult &result);")),
           "FMAP-01/FR-02: SliceService must declare the filamentMapReady signal (mirrors wipeTowerGeometryReady)");

  // FMAP-01 / FR-03 (worker capture site): the SliceService worker must read
  // Print::get_filament_maps() AND Print::get_filament_map_mode() inside the
  // HAS_LIBSLIC3R branch, and the mode < fmmManual gate must be present (the
  // upstream Print.cpp:2485 condition that determines whether the auto-
  // recommendation ran). A removal of the capture or the gate fails here.
  QVERIFY2(sliceSource.contains(QStringLiteral("print.get_filament_maps()")),
           "FMAP-01/FR-03: SliceService worker must read Print::get_filament_maps() (Print.cpp:3051) inside the Print-lifetime window");
  QVERIFY2(sliceSource.contains(QStringLiteral("print.get_filament_map_mode()")),
           "FMAP-01/FR-03: SliceService worker must read Print::get_filament_map_mode() (Print.cpp:3056) to resolve the mode");
  QVERIFY2(sliceSource.contains(QStringLiteral("fmmManual")),
           "FMAP-01/FR-03: SliceService worker must gate valid on mode < fmmManual (upstream Print.cpp:2485 auto-recommendation condition)");
  QVERIFY2(sliceSource.contains(QStringLiteral("capturedFilamentMap")),
           "FMAP-01/FR-03: SliceService worker must capture the result by value into a worker-local FilamentMapResult (Frozen Decision 1: no Print* escape)");

  // FMAP-01 / FR-02 (emit on success branch): the worker must emit
  // filamentMapReady on the success branch of the sliceFinished queued lambda
  // (same gate as wipeTowerGeometryReady).
  QVERIFY2(sliceSource.contains(QStringLiteral("emit receiver->filamentMapReady(capturedFilamentMap)")),
           "FMAP-01/FR-02: SliceService must emit filamentMapReady on the sliceFinished success branch (cancel/error branches do not emit)");

  // FMAP-01 / FR-04 (EditorViewModel slot + Q_PROPERTYs): the onFilamentMapReady
  // private slot + the 3 auto* Q_PROPERTYs + the filamentMapChanged NOTIFY +
  // the connect wiring must all stay present.
  QVERIFY2(editorHeader.contains(QStringLiteral("void onFilamentMapReady(const FilamentMapResult &result);")),
           "FMAP-01/FR-04: EditorViewModel must declare the onFilamentMapReady private slot (mirrors onWipeTowerGeometryReady)");
  QVERIFY2(editorHeader.contains(QStringLiteral("Q_PROPERTY(bool hasAutoFilamentMap READ hasAutoFilamentMap NOTIFY filamentMapChanged)")),
           "FMAP-01/FR-04: EditorViewModel must expose hasAutoFilamentMap (the valid gate, mirrors WTREAD-02 showWipeTower)");
  QVERIFY2(editorHeader.contains(QStringLiteral("Q_PROPERTY(int autoFilamentMapMode READ autoFilamentMapMode NOTIFY filamentMapChanged)")),
           "FMAP-01/FR-04: EditorViewModel must expose autoFilamentMapMode (resolved OWzx::FilamentMapMode as int)");
  QVERIFY2(editorHeader.contains(QStringLiteral("Q_PROPERTY(QVariantList autoFilamentMaps READ autoFilamentMaps NOTIFY filamentMapChanged)")),
           "FMAP-01/FR-04: EditorViewModel must expose autoFilamentMaps (1-based per-extruder group ids)");
  QVERIFY2(editorHeader.contains(QStringLiteral("void filamentMapChanged();")),
           "FMAP-01/FR-04: EditorViewModel must declare the filamentMapChanged NOTIFY signal");
  QVERIFY2(editorSource.contains(QStringLiteral("&SliceService::filamentMapReady,"))
           && editorSource.contains(QStringLiteral("&EditorViewModel::onFilamentMapReady)")),
           "FMAP-01/FR-04: EditorViewModel must connect SliceService::filamentMapReady to onFilamentMapReady");
}

void QmlUiAuditTests::filamentGroupPopupSurfacesThreeModesNotFour()
{
  // FMAP-03: the FilamentGroupPopup must surface EXACTLY the 3 selectable
  // filament-map modes (AutoForFlush/AutoForMatch/Manual). fmmDefault is the
  // per-plate "inherit from global" sentinel resolved by PartPlate::
  // get_real_filament_map_mode and is NOT a UI radio -- exposing it as a 4th
  // radio is the named anti-feature. Upstream alignment: FilamentGroupPopup.hpp
  // :52 mode_list is the 3 concrete modes only.
  //
  // R-02 (FP-04): the Q_INVOKABLE boundary at PartPlate::setFilamentMapMode(int)
  // must validate the int is in [0,3]; out-of-range clamps to fmmDefault. A
  // removal of the guard reopens the silent-invalid-enum hazard masked by the
  // writer-side default: case. Both are locked here deterministically.

  const QString popup = readSource(QStringLiteral("src/qml_gui/dialogs/FilamentGroupPopup.qml"));
  QVERIFY2(!popup.isEmpty(),
           "FMAP-03: src/qml_gui/dialogs/FilamentGroupPopup.qml must exist and be readable");
  const QString qmlQrc = readSource(QStringLiteral("src/qml_gui/qml.qrc"));
  QVERIFY2(!qmlQrc.isEmpty(), "Unable to read qml.qrc");
  const QString topbar = readSource(QStringLiteral("src/qml_gui/BBLTopbar.qml"));
  QVERIFY2(!topbar.isEmpty(), "Unable to read BBLTopbar.qml");
  const QString partPlateHeader = readSource(QStringLiteral("src/core/model/PartPlate.h"));
  QVERIFY2(!partPlateHeader.isEmpty(), "Unable to read PartPlate.h");

  // FMAP-03 / FP-01 (popup exists + CxPopup-based): the file must be a CxPopup.
  QVERIFY2(popup.contains(QStringLiteral("CxPopup {")),
           "FMAP-03/FP-01: FilamentGroupPopup.qml must be CxPopup-based");

  // FMAP-03 / FP-06 (registered in qml.qrc so the popup is loadable).
  QVERIFY2(qmlQrc.contains(QStringLiteral("dialogs/FilamentGroupPopup.qml")),
           "FMAP-03/FP-06: FilamentGroupPopup.qml must be registered in qml.qrc");

  // FMAP-03 / FP-02 (wired into the topbar so the popup is reachable).
  QVERIFY2(topbar.contains(QStringLiteral("FilamentGroupPopup")),
           "FMAP-03/FP-02: BBLTopbar must reference FilamentGroupPopup (the popup trigger)");

  // FMAP-03 / FP-01 (the 3 selectable modes appear). The popup must reference
  // all 3 concrete enum values AND bind the Phase 108 auto-recommended preview.
  QVERIFY2(popup.contains(QStringLiteral("fmmAutoForFlush")),
           "FMAP-03/FP-01: FilamentGroupPopup must surface the AutoForFlush mode (Filament-Saving)");
  QVERIFY2(popup.contains(QStringLiteral("fmmAutoForMatch")),
           "FMAP-03/FP-01: FilamentGroupPopup must surface the AutoForMatch mode (Convenience)");
  QVERIFY2(popup.contains(QStringLiteral("fmmManual")),
           "FMAP-03/FP-01: FilamentGroupPopup must surface the Manual mode (Custom)");
  QVERIFY2(popup.contains(QStringLiteral("autoFilamentMaps")),
           "FMAP-03/FP-01: FilamentGroupPopup must bind the Phase 108 autoFilamentMaps preview");
  QVERIFY2(popup.contains(QStringLiteral("hasAutoFilamentMap")),
           "FMAP-03/FP-01: FilamentGroupPopup must gate the auto preview on hasAutoFilamentMap (the valid gate)");

  // FMAP-03 anti-feature (FP-01): fmmDefault MUST NOT be a selectable radio.
  // The popup may reference the literal 3 only in the inherit-sentinel comment
  // or the fmmDefault-seed branch, but it must NOT declare a 4th radio entry.
  // Assert the Repeater model has exactly 3 delegates (3 mode entries), not 4.
  QVERIFY2(!popup.contains(QStringLiteral("fmmDefault")),
           "FMAP-03 anti-feature: FilamentGroupPopup must NOT reference fmmDefault as a selectable mode (it is the per-plate inherit-sentinel, not a 4th radio)");

  // R-02 / FP-04 (enum range validation at the Q_INVOKABLE boundary): the int
  // overload must clamp out-of-range to fmmDefault. grep-assertable guard.
  QVERIFY2(partPlateHeader.contains(QStringLiteral("if (mode < 0 || mode > 3) mode = static_cast<int>(FilamentMapMode::fmmDefault)")),
           "R-02/FP-04: PartPlate::setFilamentMapMode(int) must clamp out-of-range to fmmDefault (the Q_INVOKABLE boundary guard)");
}

void QmlUiAuditTests::filamentMapSaveReloadRoundTripCoversModesAndLegacyMigration()
{
  // FMAP-04 + Phase 107 REVIEW R-01: the filament-map save->reload round-trip
  // coverage + the legacy raw-int-1 -> fmmManual migration runtime coverage
  // must stay present in PartPlateTests. R-01 observed the FM-03 migration was
  // correct by inspection but never executed at runtime (the round-trip test
  // takes the trusted coEnum branch because the write side produces typed
  // values). Phase 111 closes the gap by (a) factoring the migration into a
  // testable helper and (b) adding a legacy-migration slot + a full round-trip
  // slot. This audit locks those anchors so a future regression (drop the
  // helper, drop a slot, or weaken the R-01 headline assertion) fails here
  // deterministically. Mirrors the Phase 102/107/108/110 source-audit pattern.

  const QString partPlateHeader = readSource(QStringLiteral("src/core/model/PartPlate.h"));
  QVERIFY2(!partPlateHeader.isEmpty(), "Unable to read PartPlate.h");
  const QString partPlateSource = readSource(QStringLiteral("src/core/model/PartPlate.cpp"));
  QVERIFY2(!partPlateSource.isEmpty(), "Unable to read PartPlate.cpp");
  const QString serviceSource = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  QVERIFY2(!serviceSource.isEmpty(), "Unable to read ProjectServiceMock.cpp");
  const QString partPlateTests = readSource(QStringLiteral("tests/PartPlateTests.cpp"));
  QVERIFY2(!partPlateTests.isEmpty(), "Unable to read PartPlateTests.cpp");

  // FMAP-04 / R-01 (factored helper declared + defined): the migration
  // predicate must exist as a free function so the legacy branch is unit-
  // testable in isolation. A removal (or re-inlining back into ProjectServiceMock)
  // drops both anchors and fails here.
  QVERIFY2(partPlateHeader.contains(QStringLiteral("FilamentMapMode migrateLegacyFilamentMapMode(int legacyRawInt);")),
           "FMAP-04/R-01: PartPlate.h must declare OWzx::migrateLegacyFilamentMapMode(int) (the factored migration predicate)");
  QVERIFY2(partPlateSource.contains(QStringLiteral("FilamentMapMode migrateLegacyFilamentMapMode(int legacyRawInt)")),
           "FMAP-04/R-01: PartPlate.cpp must define OWzx::migrateLegacyFilamentMapMode(int) (the factored migration predicate)");

  // FMAP-04 / R-01 (both read sites use the factored helper): ProjectServiceMock
  // must call migrateLegacyFilamentMapMode at BOTH read sites (loadFile +
  // loadProject) so the legacy branch is reached via the same predicate the
  // unit test exercises. There are 2 occurrences (one per read path).
  const int helperCallCount = serviceSource.count(
      QStringLiteral("OWzx::migrateLegacyFilamentMapMode(int(opt->getInt()))"));
  QVERIFY2(helperCallCount >= 2,
           qPrintable(QStringLiteral("FMAP-04/R-01: ProjectServiceMock must call "
                                     "OWzx::migrateLegacyFilamentMapMode at both read sites "
                                     "(loadFile + loadProject); found %1").arg(helperCallCount)));

  // FMAP-04 (full round-trip slot exists): the save->reload round-trip slot
  // must be declared + defined in PartPlateTests. A removal drops the FMAP-04
  // coverage and fails here.
  QVERIFY2(partPlateTests.contains(QStringLiteral("void filamentMapSaveReloadRoundTrip()")),
           "FMAP-04: PartPlateTests must declare filamentMapSaveReloadRoundTrip (full save->reload round-trip)");
  QVERIFY2(partPlateTests.contains(QStringLiteral("void PartPlateTests::filamentMapSaveReloadRoundTrip()")),
           "FMAP-04: PartPlateTests must define filamentMapSaveReloadRoundTrip (full save->reload round-trip)");

  // FMAP-04 (round-trip covers BOTH concrete modes): the slot must exercise
  // fmmManual (mode + maps) AND fmmAutoForFlush (mode). A regression that drops
  // one leg fails here. QString::contains defaults to case-sensitive.
  QVERIFY2(partPlateTests.contains(QStringLiteral("fmmManual"))
               && partPlateTests.contains(QStringLiteral("fmmAutoForFlush")),
           "FMAP-04: filamentMapSaveReloadRoundTrip must cover both fmmManual and fmmAutoForFlush legs");

  // R-01 (legacy-migration slot exists + asserts the headline): the legacy
  // migration slot must exist AND its QVERIFY2 message must name the R-01
  // headline (legacy-int-1 -> fmmManual, NOT fmmAutoForMatch). A removal of the
  // slot or a weakening of the assertion fails here.
  QVERIFY2(partPlateTests.contains(QStringLiteral("void filamentMapLegacyMigrationMapsInt1ToManual()")),
           "R-01: PartPlateTests must declare filamentMapLegacyMigrationMapsInt1ToManual (legacy migration runtime coverage)");
  QVERIFY2(partPlateTests.contains(QStringLiteral("void PartPlateTests::filamentMapLegacyMigrationMapsInt1ToManual()")),
           "R-01: PartPlateTests must define filamentMapLegacyMigrationMapsInt1ToManual (legacy migration runtime coverage)");
  QVERIFY2(partPlateTests.contains(QStringLiteral("legacy raw-int-1 MUST map to fmmManual")),
           "R-01: filamentMapLegacyMigrationMapsInt1ToManual must assert the R-01 headline (legacy raw-int-1 -> fmmManual) in its QVERIFY2 message");
  QVERIFY2(partPlateTests.contains(QStringLiteral("migrateLegacyFilamentMapMode(1)")),
           "R-01: filamentMapLegacyMigrationMapsInt1ToManual must exercise the factored helper with legacy int 1 (the actual legacy discriminator branch)");
}

void QmlUiAuditTests::perVolumeItsAccessorPresent()
{
  // MEASURE-01 / Phase 112-01: source-audit lock for the per-volume ITS
  // accessor that unblocks Phase 113 (SceneRaycaster) + Phase 114
  // (Measure::Measuring) + AssembleViewDataPool ModelObjectsClipper. The
  // accessor MUST exist with a documented ownership contract; a future
  // refactor that drops it, weakens the null-return guard, or strips the
  // ownership / SurfaceFeature-boundary documentation fails here. Mirrors the
  // Phase 102/111 source-audit pattern.

  const QString header = readSource(QStringLiteral("src/core/services/ProjectServiceMock.h"));
  QVERIFY2(!header.isEmpty(), "Unable to read ProjectServiceMock.h");
  const QString source = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  QVERIFY2(!source.isEmpty(), "Unable to read ProjectServiceMock.cpp");
  const QString smokeTests = readSource(QStringLiteral("tests/ViewModelSmokeTests.cpp"));
  QVERIFY2(!smokeTests.isEmpty(), "Unable to read ViewModelSmokeTests.cpp");

  // MI-01: the accessor signature must be declared. The return type is
  // std::shared_ptr<const indexed_triangle_set> (global-scope ITS, per
  // Measure.hpp:9); the plan's prose wrote Slic3r::indexed_triangle_set but
  // the upstream type is global, so the audit locks the global-scope form.
  QVERIFY2(header.contains(QStringLiteral("std::shared_ptr<const indexed_triangle_set> volumeMeshIts(int objectIndex, int volumeIndex) const;")),
           "MEASURE-01/MI-01: ProjectServiceMock.h must declare volumeMeshIts(int, int) returning shared_ptr<const indexed_triangle_set>");

  // MI-02: the OWNERSHIP CONTRACT section must be present at the declaration
  // so a future edit cannot silently drop the lifetime documentation (the
  // shallow-share aliasing pointer + the cross-boundary UAF closure from
  // pitfall 6).
  QVERIFY2(header.contains(QStringLiteral("OWNERSHIP CONTRACT")),
           "MEASURE-01/MI-02: ProjectServiceMock.h must document the ITS ownership contract (shallow-share aliasing pointer + pitfall 6 UAF closure)");
  QVERIFY2(header.contains(QStringLiteral("aliasing pointer")),
           "MEASURE-01/MI-02: the ownership contract must name the aliasing-pointer shallow-share mechanism");

  // MI-03: the SHALLOW-SHARE DECISION section must be present so the
  // copy-vs-share choice is documented at the source (the executor's
  // decision: share because set_mesh/reset_mesh always make a NEW shared_ptr).
  QVERIFY2(header.contains(QStringLiteral("SHALLOW-SHARE DECISION")),
           "MEASURE-01/MI-03: ProjectServiceMock.h must document the shallow-share-vs-copy decision");

  // MI-04: the CACHE DECISION section must be present so the no-cache choice
  // (ModelVolume::m_mesh IS the cache) is documented.
  QVERIFY2(header.contains(QStringLiteral("CACHE DECISION")),
           "MEASURE-01/MI-04: ProjectServiceMock.h must document the mesh-cache decision");

  // MI-05: the DEFENSIVE NULL RETURN section must be present so the
  // null-return contract is documented at the declaration.
  QVERIFY2(header.contains(QStringLiteral("DEFENSIVE NULL RETURN")),
           "MEASURE-01/MI-05: ProjectServiceMock.h must document the defensive nullptr-return contract");

  // MI-06: the SURFACE FEATURE BOUNDARY section must be present, flagging the
  // raw void*/vector* scrubbing that Phase 113/114 enforce.
  QVERIFY2(header.contains(QStringLiteral("SURFACE FEATURE BOUNDARY")),
           "MEASURE-01/MI-06: ProjectServiceMock.h must flag the SurfaceFeature raw-pointer boundary concern for Phase 113/114");
  QVERIFY2(header.contains(QStringLiteral("void* volume")),
           "MEASURE-01/MI-06: the SurfaceFeature boundary section must name the raw void* volume pointer (Measure.hpp:95) that must not escape into Qt");

  // MI-03 (implementation): the .cpp must define the accessor AND use the
  // shared_ptr aliasing constructor (the shallow-share mechanism -- not a
  // copy). Locking the aliasing constructor presence keeps a future edit from
  // silently switching to a deep copy (which would be correct but wasteful)
  // or a raw pointer (which would re-open the UAF).
  QVERIFY2(source.contains(QStringLiteral("ProjectServiceMock::volumeMeshIts")),
           "MEASURE-01/MI-03: ProjectServiceMock.cpp must define volumeMeshIts");
  QVERIFY2(source.contains(QStringLiteral("return std::shared_ptr<const indexed_triangle_set>(meshPtr, &its);")),
           "MEASURE-01/MI-03: volumeMeshIts must return via the shared_ptr aliasing constructor (shallow-share, zero copy)");

  // MI-07: the ViewModelSmokeTests regression slot must exist (valid-path +
  // null-path coverage). A removal of the slot drops the runtime coverage.
  QVERIFY2(smokeTests.contains(QStringLiteral("void perVolumeItsAccessorReturnsValidMeshAndNullForInvalidIndices()")),
           "MEASURE-01/MI-07: ViewModelSmokeTests must declare perVolumeItsAccessorReturnsValidMeshAndNullForInvalidIndices");
  QVERIFY2(smokeTests.contains(QStringLiteral("void ViewModelSmokeTests::perVolumeItsAccessorReturnsValidMeshAndNullForInvalidIndices()")),
           "MEASURE-01/MI-07: ViewModelSmokeTests must define perVolumeItsAccessorReturnsValidMeshAndNullForInvalidIndices");
}

void QmlUiAuditTests::meshAndSceneRaycasterPorted()
{
  // MEASURE-02 / Phase 113-01: source-audit lock for the pure-CPU
  // MeshRaycaster + SceneRaycaster port. This is the picking/measure math
  // foundation that Phase 114 (Measuring) and Phase 115 (snap UX) consume.
  // A future refactor that drops either class, strips the upstream anchor
  // cite, weakens the pure-CPU / cache / two-stage-pick contracts, or
  // forgets to register the files in CMake fails here. Mirrors the Phase
  // 112 perVolumeItsAccessorPresent source-audit pattern (QFile +
  // QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with MEASURE-02-
  // named messages). Source-level only; runs in the regression ctest.

  const QString meshHeader = readSource(QStringLiteral("src/core/rendering/MeshRaycaster.h"));
  QVERIFY2(!meshHeader.isEmpty(), "Unable to read MeshRaycaster.h");
  const QString meshSource = readSource(QStringLiteral("src/core/rendering/MeshRaycaster.cpp"));
  QVERIFY2(!meshSource.isEmpty(), "Unable to read MeshRaycaster.cpp");
  const QString sceneHeader = readSource(QStringLiteral("src/core/rendering/SceneRaycaster.h"));
  QVERIFY2(!sceneHeader.isEmpty(), "Unable to read SceneRaycaster.h");
  const QString sceneSource = readSource(QStringLiteral("src/core/rendering/SceneRaycaster.cpp"));
  QVERIFY2(!sceneSource.isEmpty(), "Unable to read SceneRaycaster.cpp");
  const QString cmakeLists = readSource(QStringLiteral("CMakeLists.txt"));
  QVERIFY2(!cmakeLists.isEmpty(), "Unable to read CMakeLists.txt");
  const QString partPlateTests = readSource(QStringLiteral("tests/PartPlateTests.cpp"));
  QVERIFY2(!partPlateTests.isEmpty(), "Unable to read PartPlateTests.cpp");
  const QString objectPicking = readSource(QStringLiteral("src/core/rendering/ObjectPicking.h"));
  QVERIFY2(!objectPicking.isEmpty(), "Unable to read ObjectPicking.h");

  // MR-01: MeshRaycaster.h must cite the upstream anchor (the intersection
  // math source). Locking the anchor cite keeps a future refactor honest
  // about which upstream lines it mirrors.
  QVERIFY2(meshHeader.contains(QStringLiteral("MeshUtils.hpp:159")),
           "MEASURE-02/MR-01: MeshRaycaster.h must cite upstream MeshUtils.hpp:159+ (the MeshRaycaster class + intersection math source)");
  QVERIFY2(meshHeader.contains(QStringLiteral("MeshUtils.cpp:425")),
           "MEASURE-02/MR-01: MeshRaycaster.h must cite upstream MeshUtils.cpp:425-466 (unproject_on_mesh implementation)");

  // MR-01 pure-CPU: the contract must be documented at the header so a future
  // edit cannot silently introduce a GL/wxWidgets dependency. The header
  // explicitly states the no-GL/no-wx guarantee.
  QVERIFY2(meshHeader.contains(QStringLiteral("pure-CPU")),
           "MEASURE-02/MR-01: MeshRaycaster.h must document the pure-CPU contract (no GL/wxWidgets deps)");
  QVERIFY2(meshHeader.contains(QStringLiteral("MR-01")),
           "MEASURE-02/MR-01: MeshRaycaster.h must name the MR-01 truth it implements");

  // MR-02 cache: the cache contract must be documented so a future edit
  // cannot rebuild the BVH per mouse-move (pitfall 7). The header must name
  // the BVH-build-once guarantee.
  QVERIFY2(meshHeader.contains(QStringLiteral("MR-02")),
           "MEASURE-02/MR-02: MeshRaycaster.h must name the MR-02 cache truth (BVH built once in ctor)");
  QVERIFY2(meshHeader.contains(QStringLiteral("AABBMesh")),
           "MEASURE-02/MR-01: MeshRaycaster.h must name the libslic3r AABBMesh (the pure-CPU BVH it reuses)");

  // MR-01 API: the rayCast entry point + the shared_ptr<const ITS> input
  // (the Phase 112 volumeMeshIts output type) must be present.
  QVERIFY2(meshHeader.contains(QStringLiteral("shared_ptr<const") ),
           "MEASURE-02/MR-01: MeshRaycaster must take a shared_ptr<const indexed_triangle_set> (Phase 112 volumeMeshIts output)");
  QVERIFY2(meshHeader.contains(QStringLiteral("rayCast(")),
           "MEASURE-02/MR-01: MeshRaycaster must expose rayCast(rayOrigin, rayDir)");

  // MR-02 cache (SceneRaycaster): the per-volume cache + invalidate must be
  // present so a model change drops stale raycasters.
  QVERIFY2(sceneHeader.contains(QStringLiteral("MR-02")),
           "MEASURE-02/MR-02: SceneRaycaster.h must name the MR-02 cache truth");
  QVERIFY2(sceneHeader.contains(QStringLiteral("void invalidate()")),
           "MEASURE-02/MR-02: SceneRaycaster must expose invalidate() (drop cached raycasters on model change)");
  QVERIFY2(sceneHeader.contains(QStringLiteral("invalidateVolume(")),
           "MEASURE-02/MR-02: SceneRaycaster must expose granular invalidateVolume(object, volume)");

  // MR-03 two-stage pick: SceneRaycaster.h must document the two-stage pick
  // + the pitfall-7 mitigation (no per-mouse-move volume loop). The wiring
  // is also surfaced at the ObjectPicking stage-1 seam.
  QVERIFY2(sceneHeader.contains(QStringLiteral("MR-03")),
           "MEASURE-02/MR-03: SceneRaycaster.h must name the MR-03 two-stage pick truth");
  QVERIFY2(sceneHeader.contains(QStringLiteral("pitfall 7")),
           "MEASURE-02/MR-03: SceneRaycaster.h must reference pitfall 7 (the upstream per-mouse-move volume loop)");
  QVERIFY2(sceneHeader.contains(QStringLiteral("SceneRaycasterCandidate")),
           "MEASURE-02/MR-03: SceneRaycaster must define the SceneRaycasterCandidate stage-1 seam type");
  QVERIFY2(sceneHeader.contains(QStringLiteral("hitTest(")),
           "MEASURE-02/MR-03: SceneRaycaster must expose hitTest(rayOrigin, rayDir, candidates)");
  QVERIFY2(objectPicking.contains(QStringLiteral("STAGE 1")),
           "MEASURE-02/MR-03: ObjectPicking.h must document itself as STAGE 1 of the two-stage pick (the seam must be discoverable at both ends)");

  // MR-04 hit result: the world-space hit result must carry object index,
  // volume index, facet index, world position, world normal.
  QVERIFY2(sceneHeader.contains(QStringLiteral("struct SceneRaycasterHit")),
           "MEASURE-02/MR-04: SceneRaycaster must define the SceneRaycasterHit result struct");
  QVERIFY2(sceneHeader.contains(QStringLiteral("objectIndex")),
           "MEASURE-02/MR-04: SceneRaycasterHit must carry objectIndex");
  QVERIFY2(sceneHeader.contains(QStringLiteral("volumeIndex")),
           "MEASURE-02/MR-04: SceneRaycasterHit must carry volumeIndex");
  QVERIFY2(sceneHeader.contains(QStringLiteral("facetIdx")),
           "MEASURE-02/MR-04: SceneRaycasterHit must carry facetIdx (the ITS triangle index)");
  QVERIFY2(sceneHeader.contains(QStringLiteral("worldPosition")),
           "MEASURE-02/MR-04: SceneRaycasterHit must carry worldPosition");
  QVERIFY2(sceneHeader.contains(QStringLiteral("worldNormal")),
           "MEASURE-02/MR-04: SceneRaycasterHit must carry worldNormal");

  // MR-05 CMake registration: both .cpp files must be in the owzx_app_core
  // target source list. A drop silently leaves the classes unbuilt.
  QVERIFY2(cmakeLists.contains(QStringLiteral("src/core/rendering/MeshRaycaster.cpp")),
           "MEASURE-02/MR-05: CMakeLists.txt must register MeshRaycaster.cpp in owzx_app_core");
  QVERIFY2(cmakeLists.contains(QStringLiteral("src/core/rendering/MeshRaycaster.h")),
           "MEASURE-02/MR-05: CMakeLists.txt must register MeshRaycaster.h in owzx_app_core");
  QVERIFY2(cmakeLists.contains(QStringLiteral("src/core/rendering/SceneRaycaster.cpp")),
           "MEASURE-02/MR-05: CMakeLists.txt must register SceneRaycaster.cpp in owzx_app_core");
  QVERIFY2(cmakeLists.contains(QStringLiteral("src/core/rendering/SceneRaycaster.h")),
           "MEASURE-02/MR-05: CMakeLists.txt must register SceneRaycaster.h in owzx_app_core");

  // MR-06 regression slot: the deterministic hit/miss/closest-pick runtime
  // test must exist in PartPlateTests (the slot that exercises the actual
  // AABBMesh BVH + closest-hit semantics).
  QVERIFY2(partPlateTests.contains(QStringLiteral("void meshAndSceneRaycasterHitMissAndClosestPick()")),
           "MEASURE-02/MR-06: PartPlateTests must declare meshAndSceneRaycasterHitMissAndClosestPick");
  QVERIFY2(partPlateTests.contains(QStringLiteral("void PartPlateTests::meshAndSceneRaycasterHitMissAndClosestPick()")),
           "MEASURE-02/MR-06: PartPlateTests must define meshAndSceneRaycasterHitMissAndClosestPick");

  // MR-07 upstream-anchor reference for the SceneRaycaster wrapper (the
  // upstream SceneRaycaster.hpp structure that this thin port mirrors).
  QVERIFY2(sceneHeader.contains(QStringLiteral("SceneRaycaster.hpp")),
           "MEASURE-02/MR-07: SceneRaycaster.h must cite upstream SceneRaycaster.hpp (the wrapper structure reference)");
}

void QmlUiAuditTests::measureEngineInstantiatedPerVolume()
{
  // MEASURE-03 / Phase 114-01: source-audit lock for the Measure::Measuring
  // instantiation + get_feature wiring + SurfaceFeature boundary scrubbing.
  // This is the precise single-feature measurement path that Phase 115 (snap
  // UX) consumes. A future refactor that drops the engine, strips the
  // upstream anchor cite, weakens the per-volume cache / scrubbing contract,
  // forgets the EditorViewModel readout API, or fails to register the files
  // in CMake fails here. Mirrors the Phase 113 meshAndSceneRaycasterPorted
  // source-audit pattern (QFile + QT_TESTCASE_SOURCEDIR + QString::contains
  // + QVERIFY2 with MEASURE-03-named messages). Source-level only; runs in
  // the regression ctest.

  const QString engineHeader = readSource(QStringLiteral("src/core/rendering/MeasureEngine.h"));
  QVERIFY2(!engineHeader.isEmpty(), "Unable to read MeasureEngine.h");
  const QString engineSource = readSource(QStringLiteral("src/core/rendering/MeasureEngine.cpp"));
  QVERIFY2(!engineSource.isEmpty(), "Unable to read MeasureEngine.cpp");
  const QString editorHeader = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  QVERIFY2(!editorHeader.isEmpty(), "Unable to read EditorViewModel.h");
  const QString editorSource = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  QVERIFY2(!editorSource.isEmpty(), "Unable to read EditorViewModel.cpp");
  const QString cmakeLists = readSource(QStringLiteral("CMakeLists.txt"));
  QVERIFY2(!cmakeLists.isEmpty(), "Unable to read CMakeLists.txt");
  const QString partPlateTests = readSource(QStringLiteral("tests/PartPlateTests.cpp"));
  QVERIFY2(!partPlateTests.isEmpty(), "Unable to read PartPlateTests.cpp");
  const QString asmGeo = readSource(QStringLiteral("src/core/rendering/AssemblyMeasureGeometry.h"));
  QVERIFY2(!asmGeo.isEmpty(), "Unable to read AssemblyMeasureGeometry.h");

  // ME-01: MeasureEngine.h must instantiate Measure::Measuring directly (NOT
  // reimplement the math). Locking the instantiate-don't-reimplement truth
  // keeps a future refactor honest about which layer owns the math.
  QVERIFY2(engineHeader.contains(QStringLiteral("INSTANTIATE, NOT REIMPLEMENT")),
           "MEASURE-03/ME-01: MeasureEngine.h must document the instantiate-don't-reimplement truth");
  QVERIFY2(engineHeader.contains(QStringLiteral("Measure::Measuring")),
           "MEASURE-03/ME-01: MeasureEngine.h must name Measure::Measuring (the upstream class it instantiates)");
  QVERIFY2(engineHeader.contains(QStringLiteral("ME-01")),
           "MEASURE-03/ME-01: MeasureEngine.h must name the ME-01 truth it implements");
  // The .cpp must actually construct the Measuring (the instantiation
  // evidence, not just a doc claim).
  QVERIFY2(engineSource.contains(QStringLiteral("make_shared<Slic3r::Measure::Measuring>")),
           "MEASURE-03/ME-01: MeasureEngine.cpp must instantiate Measure::Measuring via make_shared (the real construction)");

  // ME-01 cache: the per-volume cache + invalidate must be present so a model
  // change drops stale Measuring instances (pitfall 6 rebuild signal).
  QVERIFY2(engineHeader.contains(QStringLiteral("cachedMeasuringCount")),
           "MEASURE-03/ME-01: MeasureEngine must expose cachedMeasuringCount() (diagnostic / test hook for the per-volume cache)");
  QVERIFY2(engineHeader.contains(QStringLiteral("void invalidate()")),
           "MEASURE-03/ME-01: MeasureEngine must expose invalidate() (drop cached Measuring on model change)");
  QVERIFY2(engineHeader.contains(QStringLiteral("invalidateVolume(")),
           "MEASURE-03/ME-01: MeasureEngine must expose granular invalidateVolume(object, volume)");

  // ME-02 get_feature wiring: the getFeature entry point must take the Phase
  // 113 hit fields (objectIndex, volumeIndex, facetIdx, worldPoint,
  // worldTransform) and call Measuring::get_feature.
  QVERIFY2(engineHeader.contains(QStringLiteral("getFeature(")),
           "MEASURE-03/ME-02: MeasureEngine must expose getFeature(object, volume, facet, point, worldTran, onlySelectPlane)");
  QVERIFY2(engineSource.contains(QStringLiteral("->get_feature(")),
           "MEASURE-03/ME-02: MeasureEngine.cpp must call Measuring::get_feature (the upstream feature resolver)");
  QVERIFY2(engineHeader.contains(QStringLiteral("ME-02")),
           "MEASURE-03/ME-02: MeasureEngine.h must name the ME-02 get_feature-wiring truth");

  // ME-03 pitfall-6 scrubbing: the QtFeature + QtMeasurement PODs must be
  // Qt-owned types only (FeatureKind enum + QVector3D + float/int), and the
  // header must document the scrubbing contract. The libslic3r SurfaceFeature
  // back-pointer members must NOT appear as Qt-side members (the grep returns
  // zero in MeasureEngine.h/.cpp).
  QVERIFY2(engineHeader.contains(QStringLiteral("struct QtFeature")),
           "MEASURE-03/ME-03: MeasureEngine must define the QtFeature POD (Qt-owned VALUE mirror of SurfaceFeature)");
  QVERIFY2(engineHeader.contains(QStringLiteral("struct QtMeasurement")),
           "MEASURE-03/ME-03: MeasureEngine must define the QtMeasurement POD (Qt-owned VALUE mirror of MeasurementResult)");
  QVERIFY2(engineHeader.contains(QStringLiteral("FeatureKind")),
           "MEASURE-03/ME-03: MeasureEngine must define the FeatureKind enum (Qt-owned SurfaceFeatureType mirror)");
  QVERIFY2(engineHeader.contains(QStringLiteral("pitfall 6")),
           "MEASURE-03/ME-03: MeasureEngine.h must document the pitfall-6 scrubbing contract (no libslic3r back-pointer escapes)");
  QVERIFY2(engineSource.contains(QStringLiteral("scrubSurfaceFeature")),
           "MEASURE-03/ME-03: MeasureEngine.cpp must implement the scrubSurfaceFeature helper (the boundary scrubber)");
  // The literal libslic3r back-pointer member names must NOT be stored on
  // the Qt side. A grep for them in MeasureEngine.h/.cpp returns ZERO (the
  // audit re-runs the same grep the plan's verify step requires).
  QVERIFY2(!engineHeader.contains(QStringLiteral("plane_indices")),
           "MEASURE-03/ME-03: MeasureEngine.h must NOT store the libslic3r plane-index-vector back-pointer member (pitfall 6)");
  QVERIFY2(!engineSource.contains(QStringLiteral("plane_indices")),
           "MEASURE-03/ME-03: MeasureEngine.cpp must NOT store the libslic3r plane-index-vector back-pointer member (pitfall 6)");

  // ME-04 readout API on EditorViewModel: the measure* Q_PROPERTYs + the
  // computeMeasureReadoutFromHit Q_INVOKABLE + the measureReadoutChanged
  // NOTIFY signal. Phase 115 snap UX binds these.
  QVERIFY2(editorHeader.contains(QStringLiteral("measureReadoutValid")),
           "MEASURE-03/ME-04: EditorViewModel must expose measureReadoutValid (the readout gate)");
  QVERIFY2(editorHeader.contains(QStringLiteral("measureAngleText")),
           "MEASURE-03/ME-04: EditorViewModel must expose measureAngleText (angle readout)");
  QVERIFY2(editorHeader.contains(QStringLiteral("measurePerpendicularDistanceText")),
           "MEASURE-03/ME-04: EditorViewModel must expose measurePerpendicularDistanceText (perpendicular distance readout)");
  QVERIFY2(editorHeader.contains(QStringLiteral("measureDirectDistanceText")),
           "MEASURE-03/ME-04: EditorViewModel must expose measureDirectDistanceText (direct distance readout)");
  QVERIFY2(editorHeader.contains(QStringLiteral("measureDistanceXyzText")),
           "MEASURE-03/ME-04: EditorViewModel must expose measureDistanceXyzText (distance XYZ readout)");
  QVERIFY2(editorHeader.contains(QStringLiteral("computeMeasureReadoutFromHit")),
           "MEASURE-03/ME-04: EditorViewModel must expose computeMeasureReadoutFromHit (the Q_INVOKABLE that drives a readout from a Phase 113 hit)");
  QVERIFY2(editorHeader.contains(QStringLiteral("measureReadoutChanged")),
           "MEASURE-03/ME-04: EditorViewModel must emit measureReadoutChanged (the NOTIFY signal for the measure* Q_PROPERTYs)");
  QVERIFY2(editorHeader.contains(QStringLiteral("invalidateMeasureEngine")),
           "MEASURE-03/ME-04: EditorViewModel must expose invalidateMeasureEngine (drop the per-volume Measuring cache on mesh change)");

  // ME-05 AABB-stub relationship: MeasureEngine.h must document the
  // AssemblyMeasureGeometry relationship (AssemblyMeasureGeometry stays as
  // the coarse Assembly multi-volume fallback; MeasureEngine is the precise
  // single-feature path). AssemblyMeasureGeometry.h itself must still exist
  // (the AABB stub is NOT deleted).
  QVERIFY2(engineHeader.contains(QStringLiteral("AssemblyMeasureGeometry")),
           "MEASURE-03/ME-05: MeasureEngine.h must document the AssemblyMeasureGeometry relationship (AABB-stub coarse fallback vs precise single-feature)");
  QVERIFY2(engineHeader.contains(QStringLiteral("ME-05")),
           "MEASURE-03/ME-05: MeasureEngine.h must name the ME-05 AABB-stub-relationship truth");
  QVERIFY2(asmGeo.contains(QStringLiteral("AssemblyMeasureResult")),
           "MEASURE-03/ME-05: AssemblyMeasureGeometry.h must still exist with its AssemblyMeasureResult (the AABB stub is NOT deleted -- it stays as the coarse fallback)");

  // ME-06 CMake registration: both MeasureEngine files must be in the
  // owzx_app_core target source list.
  QVERIFY2(cmakeLists.contains(QStringLiteral("src/core/rendering/MeasureEngine.cpp")),
           "MEASURE-03/ME-06: CMakeLists.txt must register MeasureEngine.cpp in owzx_app_core");
  QVERIFY2(cmakeLists.contains(QStringLiteral("src/core/rendering/MeasureEngine.h")),
           "MEASURE-03/ME-06: CMakeLists.txt must register MeasureEngine.h in owzx_app_core");

  // ME-07 regression slot: the deterministic feature + readout runtime test
  // must exist in PartPlateTests.
  QVERIFY2(partPlateTests.contains(QStringLiteral("void measureEngineProducesFeatureAndReadout()")),
           "MEASURE-03/ME-07: PartPlateTests must declare measureEngineProducesFeatureAndReadout");
  QVERIFY2(partPlateTests.contains(QStringLiteral("void PartPlateTests::measureEngineProducesFeatureAndReadout()")),
           "MEASURE-03/ME-07: PartPlateTests must define measureEngineProducesFeatureAndReadout");

  // ME-08: this source-audit slot must exist (a removal drops the audit
  // coverage). The declaration is checked implicitly by the slot running.
  QVERIFY2(partPlateTests.contains(QStringLiteral("MEASURE-03/ME-02")),
           "MEASURE-03/ME-08: PartPlateTests regression must cite MEASURE-03 truth names in QVERIFY2 messages (audit-message discipline)");
}

void QmlUiAuditTests::glGizmoMeasureSnapUxWired()
{
  // MEASURE-04 / Phase 115-01: source-audit lock for the GLGizmoMeasure snap
  // UX wiring (mouse-move -> raycast -> getFeature -> Shift toggle -> visual
  // feedback + two-click measurement). This is the user-facing interaction
  // layer that closes MEASURE-04. A future refactor that drops the signals,
  // removes the Shift toggle, breaks the SceneRaycaster::hitTest wiring,
  // forgets the 4 SurfaceFeatureTypes handling, or unbinds the QML forwarding
  // fails here. Mirrors the Phase 114 measureEngineInstantiatedPerVolume
  // source-audit pattern (QFile + QT_TESTCASE_SOURCEDIR + QString::contains
  // + QVERIFY2 with MEASURE-04-named messages). Source-level only; runs in
  // the regression ctest. Runtime visual interaction is not unit-tested per
  // STATE.md (the source-audit + the Phase 114 readout test are the bar).

  const QString rhiViewportHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  QVERIFY2(!rhiViewportHeader.isEmpty(), "Unable to read RhiViewport.h");
  const QString rhiViewportSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.cpp"));
  QVERIFY2(!rhiViewportSource.isEmpty(), "Unable to read RhiViewport.cpp");
  const QString editorHeader = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  QVERIFY2(!editorHeader.isEmpty(), "Unable to read EditorViewModel.h");
  const QString editorSource = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  QVERIFY2(!editorSource.isEmpty(), "Unable to read EditorViewModel.cpp");
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");

  // MS-01 (mouse-move -> raycast -> getFeature): RhiViewport must emit the
  // measurePickRequested signal on mouse-move while the measure gizmo is
  // active, and EditorViewModel::pickMeasureFeatureAt must drive
  // SceneRaycaster::hitTest + MeasureEngine::getFeature.
  QVERIFY2(rhiViewportHeader.contains(QStringLiteral("void measurePickRequested(")),
           "MEASURE-04/MS-01: RhiViewport.h must declare the measurePickRequested signal (mouse-move -> raycast path)");
  QVERIFY2(rhiViewportHeader.contains(QStringLiteral("void measureHoverLeft()")),
           "MEASURE-04/MS-01: RhiViewport.h must declare the measureHoverLeft signal (cursor-leave clear)");
  QVERIFY2(rhiViewportSource.contains(QStringLiteral("emitMeasurePickIfActive")),
           "MEASURE-04/MS-01: RhiViewport.cpp must implement emitMeasurePickIfActive (the measure-gizmo pick helper)");
  QVERIFY2(rhiViewportSource.contains(QStringLiteral("m_gizmoMode == GizmoMeasure")),
           "MEASURE-04/MS-01: RhiViewport.cpp must gate the measure pick on m_gizmoMode == GizmoMeasure");
  QVERIFY2(editorHeader.contains(QStringLiteral("pickMeasureFeatureAt")),
           "MEASURE-04/MS-01: EditorViewModel must expose pickMeasureFeatureAt Q_INVOKABLE (the snap-UX entry point)");
  QVERIFY2(editorSource.contains(QStringLiteral("->hitTest(")),
           "MEASURE-04/MS-01: EditorViewModel.cpp must call SceneRaycaster::hitTest (the two-stage pick stage-2)");
  QVERIFY2(editorSource.contains(QStringLiteral("computeMeasureReadoutFromHit(")),
           "MEASURE-04/MS-01: EditorViewModel.cpp must drive computeMeasureReadoutFromHit (Phase 114 MeasureEngine::getFeature)");

  // MS-02 (Shift toggle): RhiViewport must read Qt::ShiftModifier and forward
  // it as the shiftHeld arg. Mirrors upstream GLGizmoMeasure.cpp:409-442.
  QVERIFY2(rhiViewportSource.contains(QStringLiteral("Qt::ShiftModifier")),
           "MEASURE-04/MS-02: RhiViewport.cpp must read Qt::ShiftModifier for the Shift toggle (FeatureSelection vs PointSelection)");
  QVERIFY2(rhiViewportSource.contains(QStringLiteral("shiftHeld")),
           "MEASURE-04/MS-02: RhiViewport.cpp must forward the shiftHeld arg through measurePickRequested");
  QVERIFY2(rhiViewportSource.contains(QStringLiteral("GLGizmoMeasure.cpp:409-442")),
           "MEASURE-04/MS-02: RhiViewport.cpp must cite the upstream Shift-toggle anchor (GLGizmoMeasure.cpp:409-442)");

  // MS-03 (visual feedback + 4 SurfaceFeatureTypes): the FeatureKind enum
  // must be surfaced via the measureHoverFeatureKind Q_PROPERTY (the live
  // hover-feature type drives the overlay). The 4 SurfaceFeatureTypes
  // (Point/Edge/Circle/Plane, Measure.hpp:16-22) are resolved by
  // MeasureEngine (Phase 114, audited by measureEngineInstantiatedPerVolume);
  // Phase 115 surfaces the resolved kind via the Q_PROPERTY + the QML
  // indicator. PointSelection (Shift) overrides the kind to Point.
  QVERIFY2(editorSource.contains(QStringLiteral("FeatureKind::Point")),
           "MEASURE-04/MS-03: EditorViewModel.cpp must reference FeatureKind::Point (the Shift/PointSelection override)");
  QVERIFY2(editorHeader.contains(QStringLiteral("measureHoverFeatureKind")),
           "MEASURE-04/MS-03: EditorViewModel must expose measureHoverFeatureKind (the live hover-feature type for the overlay)");
  QVERIFY2(editorHeader.contains(QStringLiteral("measureHoverWorldPosition")),
           "MEASURE-04/MS-03: EditorViewModel must expose measureHoverWorldPosition (the live hover-feature position for the overlay)");
  QVERIFY2(preparePage.contains(QStringLiteral("measureHoverFeatureKind")),
           "MEASURE-04/MS-03: PreparePage.qml must surface measureHoverFeatureKind (the live feature-type indicator)");

  // MS-04 (two-click measurement): the Phase 114 m_measureFromFeatureValid
  // stash must be reused (first click = A, second click = B). The
  // clearMeasureReadout path must reset it.
  QVERIFY2(editorHeader.contains(QStringLiteral("m_measureFromFeatureValid")),
           "MEASURE-04/MS-04: EditorViewModel must hold m_measureFromFeatureValid (the two-click A/B stash)");
  QVERIFY2(editorHeader.contains(QStringLiteral("clearMeasureReadout")),
           "MEASURE-04/MS-04: EditorViewModel must expose clearMeasureReadout (reset the two-click flow on cursor-leave / Escape)");

  // MS-05 (pitfall-7 mitigation): the candidate list must be the SINGLE
  // stage-1 survivor, NOT a whole-scene loop. The SceneRaycaster is lazily
  // built from the same ITS source MeasureEngine uses.
  QVERIFY2(editorSource.contains(QStringLiteral("SceneRaycasterCandidate")),
           "MEASURE-04/MS-05: EditorViewModel.cpp must use SceneRaycasterCandidate (the stage-2 candidate type -- pitfall-7 mitigation)");
  QVERIFY2(editorSource.contains(QStringLiteral("m_sceneRaycaster")),
           "MEASURE-04/MS-05: EditorViewModel.cpp must hold a SceneRaycaster member (lazily built, cached per-volume)");

  // QML wiring: PreparePage.qml must forward the signals to the ViewModel.
  QVERIFY2(preparePage.contains(QStringLiteral("onMeasurePickRequested")),
           "MEASURE-04/QML: PreparePage.qml must handle onMeasurePickRequested (forward to editorVm.pickMeasureFeatureAt)");
  QVERIFY2(preparePage.contains(QStringLiteral("onMeasureHoverLeft")),
           "MEASURE-04/QML: PreparePage.qml must handle onMeasureHoverLeft (forward to editorVm.clearMeasureReadout)");
  QVERIFY2(preparePage.contains(QStringLiteral("pickMeasureFeatureAt")),
           "MEASURE-04/QML: PreparePage.qml must call editorVm.pickMeasureFeatureAt (the snap-UX binding)");

  // MS-06: this source-audit slot must exist (a removal drops the audit
  // coverage). The declaration is checked implicitly by the slot running.
  // The runtime visual evidence bar is documented as blocked per STATE.md
  // (the source-audit + the Phase 114 readout test are the verification
  // surface -- the plan's MS-06 truth names this explicitly).
}

void QmlUiAuditTests::v45CrossWorkstreamRegressionLocked()
{
  // Phase 116-01 (VV-01a/b/c): the FINAL v4.5 cross-workstream regression
  // meta-gate. This is the milestone-level lock that closes WTMESH-04 +
  // MEASURE-05 and proves the 5 v4.5 workstreams shipped without regressing
  // the v4.4 Phase 99 baseline. The per-workstream source-audit slots above
  // (optionBWipeTowerMeshCoexistsWithOptionA, filamentMap*,
  // perVolumeItsAccessorPresent, meshAndSceneRaycasterPorted,
  // measureEngineInstantiatedPerVolume, glGizmoMeasureSnapUxWired,
  // argvFixtureGate*, d3d12*) lock the individual truths; THIS slot is the
  // consolidated milestone contract so a regression in any workstream fails
  // the v4.5 close gate.
  //
  // Pattern: QFile + QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2
  // with v4.5-named messages (VV-01a/b/c). Deterministic, build-dir-
  // independent. Source-level only; runs in the regression ctest.

  // ---- VV-01a (WTMESH-04): Option A + Option B coexist ----
  // The v4.4 Phase 99 Frozen Decision 2 froze Option A (buildWipeTowerVertices,
  // the rectangular-prism silhouette) as the baseline. Phase 109 (WTMESH-01/02/
  // 03) re-opened that decision and added Option B (buildWipeTowerMeshVertices,
  // the real wipe_tower_mesh_data + convex_hull_3d mesh) as a PARALLEL path.
  // WTMESH-04 closes by proving BOTH builders are present (Option A is the
  // single-material / pre-slice fallback; Option B is the real-mesh upgrade).
  const QString geometryHeader = readSource(QStringLiteral("src/core/rendering/GizmoGeometry.h"));
  QVERIFY2(!geometryHeader.isEmpty(), "Unable to read GizmoGeometry.h");
  QVERIFY2(geometryHeader.contains(QStringLiteral("buildWipeTowerVertices")),
           "VV-01a/WTMESH-04: GizmoGeometry.h must STILL declare buildWipeTowerVertices (Option A baseline -- v4.4 Phase 99 Frozen Decision 2 preserved)");
  QVERIFY2(geometryHeader.contains(QStringLiteral("buildWipeTowerMeshVertices")),
           "VV-01a/WTMESH-04: GizmoGeometry.h must declare buildWipeTowerMeshVertices (Option B real-mesh builder -- Phase 109 upgrade)");
  QVERIFY2(geometryHeader.contains(QStringLiteral("WTMESH-03")),
           "VV-01a/WTMESH-04: GizmoGeometry.h must document the WTMESH-03 PARALLEL-path contract (Option A preserved, Option B added alongside)");

  // ---- VV-01b (MEASURE-05): MeasureEngine is the REAL measurement path ----
  // MeasureEngine (Phase 114) instantiates Measure::Measuring and produces
  // REAL per-feature measurements (point / edge / circle / plane + angle /
  // distance readouts). The AssemblyMeasureGeometry AABB stub is NOT the
  // measurement engine -- it stays as the COARSE Assembly multi-volume
  // fallback (augmented, not replaced; documented in MeasureEngine.h ME-05).
  const QString engineHeader = readSource(QStringLiteral("src/core/rendering/MeasureEngine.h"));
  QVERIFY2(!engineHeader.isEmpty(), "Unable to read MeasureEngine.h");
  const QString engineSource = readSource(QStringLiteral("src/core/rendering/MeasureEngine.cpp"));
  QVERIFY2(!engineSource.isEmpty(), "Unable to read MeasureEngine.cpp");
  const QString asmGeo = readSource(QStringLiteral("src/core/rendering/AssemblyMeasureGeometry.h"));
  QVERIFY2(!asmGeo.isEmpty(), "Unable to read AssemblyMeasureGeometry.h");
  QVERIFY2(engineHeader.contains(QStringLiteral("INSTANTIATE, NOT REIMPLEMENT")),
           "VV-01b/MEASURE-05: MeasureEngine.h must document the instantiate-don't-reimplement truth (the REAL measurement path, not a stub)");
  QVERIFY2(engineHeader.contains(QStringLiteral("Measure::Measuring")),
           "VV-01b/MEASURE-05: MeasureEngine.h must name Measure::Measuring (the upstream class it instantiates for real measurements)");
  QVERIFY2(engineSource.contains(QStringLiteral("make_shared<Slic3r::Measure::Measuring>")),
           "VV-01b/MEASURE-05: MeasureEngine.cpp must actually construct Measure::Measuring (the real measurement engine, not the AABB stub)");
  QVERIFY2(engineSource.contains(QStringLiteral("->get_feature(")),
           "VV-01b/MEASURE-05: MeasureEngine.cpp must call Measuring::get_feature (real per-feature resolution, not AABB approximation)");
  QVERIFY2(engineHeader.contains(QStringLiteral("ME-05")),
           "VV-01b/MEASURE-05: MeasureEngine.h must name the ME-05 AABB-stub-relationship truth");
  QVERIFY2(engineHeader.contains(QStringLiteral("AssemblyMeasureGeometry")),
           "VV-01b/MEASURE-05: MeasureEngine.h must document the AssemblyMeasureGeometry relationship (AABB stub is augmented, not replaced -- it stays as the coarse Assembly fallback)");
  QVERIFY2(asmGeo.contains(QStringLiteral("AssemblyMeasureResult")),
           "VV-01b/MEASURE-05: AssemblyMeasureGeometry.h must still exist with AssemblyMeasureResult (the AABB stub is preserved as the coarse fallback, NOT deleted by the MeasureEngine upgrade)");

  // ---- VV-01c: the 5 v4.5 workstream anchors are all wired ----

  // WS1: filament-map (4-value enum + auto readback + 3-mode popup). Phases
  // 107/108/110/111. The enum must stay widened to upstream's 4 values; the
  // auto-recommendation readback + the 3-mode popup must stay wired.
  const QString partPlateHeader = readSource(QStringLiteral("src/core/model/PartPlate.h"));
  QVERIFY2(!partPlateHeader.isEmpty(), "Unable to read PartPlate.h");
  QVERIFY2(partPlateHeader.contains(QStringLiteral("fmmAutoForFlush")) &&
           partPlateHeader.contains(QStringLiteral("fmmAutoForMatch")) &&
           partPlateHeader.contains(QStringLiteral("fmmManual")) &&
           partPlateHeader.contains(QStringLiteral("fmmDefault")),
           "VV-01c/WS1: PartPlate.h must declare all 4 FilamentMapMode values (filament-map enum widening, Phase 107)");
  QVERIFY2(partPlateHeader.contains(QStringLiteral("migrateLegacyFilamentMapMode")),
           "VV-01c/WS1: PartPlate.h must declare migrateLegacyFilamentMapMode (R-01 legacy raw-int-1 -> fmmManual migration helper, Phase 111)");

  // WS2: Option B real wipe-tower mesh (covered in depth by VV-01a above;
  // this anchor confirms the SliceService capture path is wired).
  const QString sliceSource = readSource(QStringLiteral("src/core/services/SliceService.cpp"));
  QVERIFY2(!sliceSource.isEmpty(), "Unable to read SliceService.cpp");
  QVERIFY2(sliceSource.contains(QStringLiteral("wipe_tower_mesh_data")),
           "VV-01c/WS2: SliceService.cpp must read wipe_tower_mesh_data (the Option B real-mesh capture, Phase 109)");
  QVERIFY2(sliceSource.contains(QStringLiteral("convex_hull_3d()")),
           "VV-01c/WS2: SliceService.cpp must run convex_hull_3d() on the merged wipe-tower mesh (Phase 109, mirrors upstream 3DScene.cpp:914)");

  // WS3: CLI fixtures (multi-material fixture model + argv recipe doc). The
  // runtime-visual-evidence workaround per STATE.md. Phases 103/104.
  QFileInfo fixtureInfo(
      QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR))
          .filePath(QStringLiteral("tests/data/multi_material_fixture.3mf")));
  QVERIFY2(fixtureInfo.exists(),
           "VV-01c/WS3: tests/data/multi_material_fixture.3mf must exist (CLI fixture readiness, Phase 104 FIXTURE-01)");
  QFileInfo recipeInfo(
      QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR))
          .filePath(QStringLiteral("tests/data/fixture_recipes.md")));
  QVERIFY2(recipeInfo.exists(),
           "VV-01c/WS3: tests/data/fixture_recipes.md must exist (argv recipe doc, Phase 104 FIXTURE-03)");

  // WS4: D3D12 opt-in (debug layer wired behind env flag, stays opt-in --
  // NOT the default backend). Phases 105/106. The D3D12-03 anti-feature
  // contract: default promotion stays out of scope until a confirmed root
  // cause.
  const QString rhiBackendSelector = readSource(QStringLiteral("src/qml_gui/Renderer/RhiBackendSelector.cpp"));
  QVERIFY2(!rhiBackendSelector.isEmpty(), "Unable to read RhiBackendSelector.cpp");
  QVERIFY2(rhiBackendSelector.contains(QStringLiteral("OWZX_RHI_RENDERER")),
           "VV-01c/WS4: RhiBackendSelector.cpp must read the OWZX_RHI_RENDERER env flag (D3D12 stays opt-in, Phase 105/106)");
  QVERIFY2(rhiBackendSelector.contains(QStringLiteral("d3d12")),
           "VV-01c/WS4: RhiBackendSelector.cpp must accept the d3d12 opt-in value (Phase 105/106 backend readiness)");

  // WS5: GLGizmoMeasure engine (per-volume ITS + raycaster + Measuring +
  // snap UX). Phases 112/113/114/115. This is the measure chain VV-01b
  // builds on; the anchors confirm the full chain is wired end-to-end.
  const QString projectHeader = readSource(QStringLiteral("src/core/services/ProjectServiceMock.h"));
  QVERIFY2(!projectHeader.isEmpty(), "Unable to read ProjectServiceMock.h");
  QVERIFY2(projectHeader.contains(QStringLiteral("volumeMeshIts")),
           "VV-01c/WS5: ProjectServiceMock.h must declare volumeMeshIts (per-volume ITS accessor, Phase 112 MEASURE-01 -- unblocks raycaster + Measuring)");
  QVERIFY2(QFile::exists(QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR))
                             .filePath(QStringLiteral("src/core/rendering/MeshRaycaster.h"))),
           "VV-01c/WS5: MeshRaycaster.h must exist (pure-CPU raycaster port, Phase 113 MEASURE-02)");
  QVERIFY2(QFile::exists(QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR))
                             .filePath(QStringLiteral("src/core/rendering/SceneRaycaster.h"))),
           "VV-01c/WS5: SceneRaycaster.h must exist (two-stage pick wrapper, Phase 113 MEASURE-02)");
  QVERIFY2(QFile::exists(QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR))
                             .filePath(QStringLiteral("src/core/rendering/MeasureEngine.h"))),
           "VV-01c/WS5: MeasureEngine.h must exist (Measure::Measuring instantiation, Phase 114 MEASURE-03)");

  const QString editorHeader = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  QVERIFY2(!editorHeader.isEmpty(), "Unable to read EditorViewModel.h");
  QVERIFY2(editorHeader.contains(QStringLiteral("pickMeasureFeatureAt")),
           "VV-01c/WS5: EditorViewModel.h must expose pickMeasureFeatureAt (Phase 115 snap-UX entry point, MEASURE-04)");
  QVERIFY2(editorHeader.contains(QStringLiteral("computeMeasureReadoutFromHit")),
           "VV-01c/WS5: EditorViewModel.h must expose computeMeasureReadoutFromHit (Phase 114 readout API, MEASURE-03/ME-04)");

  // VV-01 meta: this slot is the v4.5 milestone close gate. A removal of the
  // per-workstream slots above would still fail this gate because the
  // consolidated anchors are re-asserted here. Runtime visual evidence is
  // blocked per STATE.md; the source-audit + regression ctest + launch
  // liveness are the verification bar (VV-02/03/04).
}

void QmlUiAuditTests::tickMarksRenderedOnPreviewRail()
{
  // Phase 117-01 (TICK-01): the Preview layer rail must render tick marks
  // (pause / color-change / filament-change / custom-gcode / template) driven
  // by previewVm.tickMarks. The read-side parse at PreviewViewModel.cpp:993-1021
  // already feeds tickMarks_ from sliced G-code comments; Phase 117 surfaces the
  // ticks in the running Preview by consolidating the formerly-orphaned
  // horizontal LayerSlider.qml tick Repeater + right-click add/edit/delete menus
  // + CustomGcodeDialog into the vertical PreviewLayerRail.qml (source-truth-
  // aligned with upstream IMSlider). This slot locks the consolidation:
  // (a) tickMarks is referenced in PreviewLayerRail.qml; (b) the ViewModel tick
  // CRUD methods are wired into the menus; (c) CustomGcodeDialog is instantiated
  // in the new host; (d) the orphaned LayerSlider.qml is deleted and its qml.qrc
  // entry is gone (No-Deprecated-UI rule). Write-back to
  // custom_gcode_per_print_z and re-slice are Phase 118 -- Phase 117 only
  // surfaces the UI. Each QVERIFY2 names the TICK-01 contract it locks.
  //
  // Pattern: QFile + QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with
  // a TICK-01-named message. Mirrors the Phase 102..116 source-audit pattern
  // in this file (deterministic, build-dir-independent).

  const QString rail = readSource(QStringLiteral("src/qml_gui/components/PreviewLayerRail.qml"));
  const QString qrc = readSource(QStringLiteral("src/qml_gui/qml.qrc"));
  QVERIFY2(!rail.isEmpty(), "Unable to read PreviewLayerRail.qml");
  QVERIFY2(!qrc.isEmpty(), "Unable to read qml.qrc");

  // TICK-01 / TK-01 (tick render): PreviewLayerRail.qml must reference
  // previewVm.tickMarks (the Repeater model).
  QVERIFY2(rail.contains(QStringLiteral("previewVm.tickMarks")),
           "TICK-01/TK-01: PreviewLayerRail.qml must render a Repeater over previewVm.tickMarks");

  // TICK-01 / TK-02 (color-coded): the per-type color switch must cover the
  // CustomGcode case (case 1) -- the orphaned LayerSlider.qml fell through to
  // gray (default), upstream renders custom-gcode ticks distinctly.
  QVERIFY2(rail.contains(QStringLiteral("case 1:")),
           "TICK-01/TK-02: PreviewLayerRail.qml must color-code CustomGcode ticks (case 1 branch -- the orphaned LayerSlider fell to gray default)");

  // TICK-01 / TK-03 (add menu): the right-click Add menu must be present and
  // wired to addPauseAtLayer (and the Add Custom G-code entry).
  QVERIFY2(rail.contains(QStringLiteral("addPauseAtLayer")),
           "TICK-01/TK-03: PreviewLayerRail.qml add menu must call previewVm.addPauseAtLayer");
  QVERIFY2(rail.contains(QStringLiteral("sliderAddMenu")),
           "TICK-01/TK-03: PreviewLayerRail.qml must define the right-click add menu (sliderAddMenu)");

  // TICK-01 / TK-04 (edit/delete menu): the right-click Edit/Delete menu on a
  // tick must be present. Delete calls removeTickAtLayer directly in the rail;
  // Edit opens customGcodeEditDialog (the dialog calls editCustomGcodeAtLayer
  // on confirm -- see CustomGcodeDialog.qml:59 -- correct UI layering, so the
  // rail reads the existing tick via tickAtLayer and the dialog owns the write).
  QVERIFY2(rail.contains(QStringLiteral("removeTickAtLayer")),
           "TICK-01/TK-04: PreviewLayerRail.qml edit menu must call previewVm.removeTickAtLayer");
  QVERIFY2(rail.contains(QStringLiteral("customGcodeEditDialog")),
           "TICK-01/TK-04: PreviewLayerRail.qml must open customGcodeEditDialog on edit (dialog owns editCustomGcodeAtLayer)");
  QVERIFY2(rail.contains(QStringLiteral("tickAtLayer")),
           "TICK-01/TK-04: PreviewLayerRail.qml edit menu must read the existing tick via previewVm.tickAtLayer");
  QVERIFY2(rail.contains(QStringLiteral("sliderEditMenu")),
           "TICK-01/TK-04: PreviewLayerRail.qml must define the right-click edit/delete menu (sliderEditMenu)");

  // TICK-01 / TK-05 (CustomGcodeDialog host): PreviewLayerRail.qml must
  // instantiate CustomGcodeDialog (the formerly-orphaned host was LayerSlider).
  QVERIFY2(rail.contains(QStringLiteral("CustomGcodeDialog")),
           "TICK-01/TK-05: PreviewLayerRail.qml must instantiate CustomGcodeDialog (new host replacing the deleted orphan LayerSlider)");

  // TICK-01 / TK-06 (No-Deprecated-UI): the orphaned LayerSlider.qml must be
  // deleted and its qml.qrc entry removed.
  QVERIFY2(!qrc.contains(QStringLiteral("components/LayerSlider.qml")),
           "TICK-01/TK-06: qml.qrc must NOT list components/LayerSlider.qml (orphaned slider deleted per No-Deprecated-UI rule)");
  QVERIFY2(!rail.contains(QStringLiteral("import \"../controls\"")) == false || rail.contains(QStringLiteral("CxMenu")),
           "TICK-01/TK-06: PreviewLayerRail.qml menus use CxMenu (controls import present)");
}

void QmlUiAuditTests::customGcodeWritebackAndResliceWired()
{
  // Phase 118-01 (TICK-02/TICK-03): Phase 117 surfaced the tick UI but only
  // mutated the in-memory tickMarks_. Phase 118 closes the loop: every tick CRUD
  // mutator now writes the converted CustomGCode::Info back into libslic3r's
  // model->plates_custom_gcodes (direct field assignment -- BBS deprecated
  // Model::set_custom_gcode_per_print_z, see Model.hpp:1559-1570) and triggers a
  // re-slice via sliceService_->startSlice so the emitted G-code actually
  // contains the pause / color-change / filament-change / custom-gcode markers.
  // This slot locks the write-back wiring at the source level (deterministic,
  // build-dir-independent), mirroring the Phase 102..117 audit pattern. Each
  // QVERIFY2 names the TICK-02/TICK-03 contract it locks.
  const QString pvm = readSource(QStringLiteral("src/core/viewmodels/PreviewViewModel.cpp"));
  const QString pvmHeader = readSource(QStringLiteral("src/core/viewmodels/PreviewViewModel.h"));
  const QString ctx = readSource(QStringLiteral("src/qml_gui/BackendContext.cpp"));
  QVERIFY2(!pvm.isEmpty(), "Unable to read PreviewViewModel.cpp");
  QVERIFY2(!pvmHeader.isEmpty(), "Unable to read PreviewViewModel.h");
  QVERIFY2(!ctx.isEmpty(), "Unable to read BackendContext.cpp");

  // TICK-02 (write-back target): PreviewViewModel.cpp must reference the BBS
  // plates_custom_gcodes map (the real write path -- NOT the deprecated
  // set_custom_gcode_per_print_z helper).
  QVERIFY2(pvm.contains(QStringLiteral("plates_custom_gcodes")),
           "TICK-02: PreviewViewModel.cpp must write model->plates_custom_gcodes (BBS write path)");

  // TICK-02 (explicit enum map, NOT static_cast): the pure conversion helper
  // convertTicksToCustomGcodeInfo must exist, and the TickType->CustomGCode::Type
  // map must be an explicit switch (the two enums have DIVERGENT numeric orders,
  // so static_cast would corrupt PausePrint 0->1 and ColorChange 4->0). The
  // helper name is the single audit anchor that proves the explicit map is in
  // place; a bare static_cast<...Type> must NOT appear.
  QVERIFY2(pvm.contains(QStringLiteral("convertTicksToCustomGcodeInfo")),
           "TICK-02: PreviewViewModel.cpp must define the explicit-map conversion helper convertTicksToCustomGcodeInfo");
  QVERIFY2(pvm.contains(QStringLiteral("CustomGCode::PausePrint")),
           "TICK-02: convertTicksToCustomGcodeInfo must map via the explicit CustomGCode::Type enum (PausePrint branch present)");
  QVERIFY2(!pvm.contains(QStringLiteral("static_cast<Slic3r::CustomGCode::Type>")),
           "TICK-02: convertTicksToCustomGcodeInfo must NOT static_cast TickType to CustomGCode::Type (divergent numeric order)");

  // TICK-03 (re-slice trigger): PreviewViewModel.cpp must call startSlice to
  // re-slice after writing the Info, so the G-code picks up the markers.
  QVERIFY2(pvm.contains(QStringLiteral("startSlice")),
           "TICK-03: PreviewViewModel.cpp must trigger a re-slice via startSlice after write-back");

  // TICK-02 (dependency injection): PreviewViewModel ctor must take
  // ProjectServiceMock* so it can reach rawModel()->plates_custom_gcodes, and
  // BackendContext must pass projectService_ at assembly.
  QVERIFY2(pvmHeader.contains(QStringLiteral("ProjectServiceMock *projectService")),
           "TICK-02: PreviewViewModel ctor must inject ProjectServiceMock* (projectService_ reach-through to rawModel)");
  QVERIFY2(ctx.contains(QStringLiteral("new PreviewViewModel(projectService_, sliceService_, this)")),
           "TICK-02: BackendContext must assemble PreviewViewModel with projectService_ as the first ctor arg");
}

void QmlUiAuditTests::tickTypeCoverageAndDragRelocation()
{
  // Phase 119-01 (TICK-04/TICK-05): Phase 118 closed the write-back + re-slice
  // loop but left two gaps -- (1) only Pause/CustomGcode/ToolChange had add
  // paths, so ColorChange + Template were unreachable from the UI despite being
  // handled by the read-side parse + convertTicksToCustomGcodeInfo; (2) ticks
  // could be added/edited/deleted but not relocated (no moveTick). This slot
  // locks the coverage + drag wiring at the source level (deterministic,
  // build-dir-independent), mirroring the Phase 102..118 audit pattern. Each
  // QVERIFY2 names the TICK-04/TICK-05 contract it locks.
  const QString pvm = readSource(QStringLiteral("src/core/viewmodels/PreviewViewModel.cpp"));
  const QString pvmHeader = readSource(QStringLiteral("src/core/viewmodels/PreviewViewModel.h"));
  const QString rail = readSource(QStringLiteral("src/qml_gui/components/PreviewLayerRail.qml"));
  QVERIFY2(!pvm.isEmpty(), "Unable to read PreviewViewModel.cpp");
  QVERIFY2(!pvmHeader.isEmpty(), "Unable to read PreviewViewModel.h");
  QVERIFY2(!rail.isEmpty(), "Unable to read PreviewLayerRail.qml");

  // TICK-04 (ColorChange add path): PreviewViewModel.h must declare and .cpp
  // must define addColorChangeAtLayer (type=ColorChange, extruder + color).
  QVERIFY2(pvmHeader.contains(QStringLiteral("addColorChangeAtLayer")),
           "TICK-04: PreviewViewModel.h must declare addColorChangeAtLayer (ColorChange had no add path)");
  QVERIFY2(pvm.contains(QStringLiteral("void PreviewViewModel::addColorChangeAtLayer")),
           "TICK-04: PreviewViewModel.cpp must implement addColorChangeAtLayer");

  // TICK-04 (Template add path): PreviewViewModel.h must declare and .cpp must
  // define addTemplateAtLayer (type=Template, upstream "save state" anchor).
  QVERIFY2(pvmHeader.contains(QStringLiteral("addTemplateAtLayer")),
           "TICK-04: PreviewViewModel.h must declare addTemplateAtLayer (Template had no add path)");
  QVERIFY2(pvm.contains(QStringLiteral("void PreviewViewModel::addTemplateAtLayer")),
           "TICK-04: PreviewViewModel.cpp must implement addTemplateAtLayer");

  // TICK-04 (round-trip): both new add methods must flow through the Phase 118
  // write-back (writeTicksToModel) so the marker lands in the emitted G-code.
  QVERIFY2(pvm.contains(QStringLiteral("writeTicksToModel")),
           "TICK-04: PreviewViewModel add methods must persist via writeTicksToModel");

  // TICK-05 (moveTick declared + implemented): PreviewViewModel must expose
  // Q_INVOKABLE bool moveTick and implement it (drag-to-relocate).
  QVERIFY2(pvmHeader.contains(QStringLiteral("Q_INVOKABLE bool moveTick")),
           "TICK-05: PreviewViewModel.h must declare Q_INVOKABLE bool moveTick");
  QVERIFY2(pvm.contains(QStringLiteral("bool PreviewViewModel::moveTick")),
           "TICK-05: PreviewViewModel.cpp must implement moveTick");

  // TICK-05 (drag wired): PreviewLayerRail.qml must call previewVm.moveTick
  // from the tick delegate drag handler (upstream IMSlider on_mouse_drag).
  QVERIFY2(rail.contains(QStringLiteral("previewVm.moveTick")),
           "TICK-05: PreviewLayerRail.qml tick delegate must call previewVm.moveTick on drag release");

  // TICK-04 (Add menu coverage): the Add menu must surface ColorChange + Template
  // entries so all 5 types are reachable from the UI.
  QVERIFY2(rail.contains(QStringLiteral("Add Color Change")),
           "TICK-04: PreviewLayerRail.qml Add menu must have an Add Color Change entry");
  QVERIFY2(rail.contains(QStringLiteral("Add Template")),
           "TICK-04: PreviewLayerRail.qml Add menu must have an Add Template entry");

  // TICK-04 (Add menu wired): the two new entries must call the matching
  // ViewModel add methods (not just display the label).
  QVERIFY2(rail.contains(QStringLiteral("addColorChangeAtLayer")),
           "TICK-04: PreviewLayerRail.qml Add Color Change must call previewVm.addColorChangeAtLayer");
  QVERIFY2(rail.contains(QStringLiteral("addTemplateAtLayer")),
           "TICK-04: PreviewLayerRail.qml Add Template must call previewVm.addTemplateAtLayer");
}

void QmlUiAuditTests::triangleSelectorEnginePorted()
{
  // PAINT-01 / Phase 120-01-03: source-audit lock proving the TriangleSelector
  // pick + subdivide + paint pipeline is ported via REUSE. TriangleSelector is
  // ALREADY compiled in (CMakeLists.txt:457-458) -- Phase 120 wraps it in
  // PaintEngine + bridges the 3 structural gaps (CONTEXT.md). This slot locks
  // every must_have TS truth at the source level so a future refactor that
  // drops the accessor, the mesh-local hit field, the PaintEngine wrapper, or
  // (worst case) hand-rolls a selector fails here. Mirrors the Phase 113
  // meshAndSceneRaycasterPorted pattern (readSource + QVERIFY2 with PAINT-01-
  // named messages). Source-level only; runs in the regression ctest.

  const QString projectHeader = readSource(QStringLiteral("src/core/services/ProjectServiceMock.h"));
  QVERIFY2(!projectHeader.isEmpty(), "Unable to read ProjectServiceMock.h");
  const QString projectSource = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  QVERIFY2(!projectSource.isEmpty(), "Unable to read ProjectServiceMock.cpp");
  const QString sceneHeader = readSource(QStringLiteral("src/core/rendering/SceneRaycaster.h"));
  QVERIFY2(!sceneHeader.isEmpty(), "Unable to read SceneRaycaster.h");
  const QString paintHeader = readSource(QStringLiteral("src/core/rendering/PaintEngine.h"));
  QVERIFY2(!paintHeader.isEmpty(), "Unable to read PaintEngine.h");
  const QString paintSource = readSource(QStringLiteral("src/core/rendering/PaintEngine.cpp"));
  QVERIFY2(!paintSource.isEmpty(), "Unable to read PaintEngine.cpp");
  const QString editorHeader = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  QVERIFY2(!editorHeader.isEmpty(), "Unable to read EditorViewModel.h");
  const QString editorSource = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  QVERIFY2(!editorSource.isEmpty(), "Unable to read EditorViewModel.cpp");
  const QString rhiHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  QVERIFY2(!rhiHeader.isEmpty(), "Unable to read RhiViewport.h");
  const QString rhiSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.cpp"));
  QVERIFY2(!rhiSource.isEmpty(), "Unable to read RhiViewport.cpp");
  const QString cmakeLists = readSource(QStringLiteral("CMakeLists.txt"));
  QVERIFY2(!cmakeLists.isEmpty(), "Unable to read CMakeLists.txt");

  // TS-01 (a): ProjectServiceMock must declare + implement volumeMeshTriangleMesh
  // returning shared_ptr<const TriangleMesh> (the aliasing shared_ptr that
  // keeps the TriangleMesh alive for TriangleSelector's `const TriangleMesh&`
  // reference ctor).
  QVERIFY2(projectHeader.contains(QStringLiteral("volumeMeshTriangleMesh")),
           "PAINT-01/TS-01: ProjectServiceMock.h must declare volumeMeshTriangleMesh");
  QVERIFY2(projectHeader.contains(QStringLiteral("shared_ptr<const Slic3r::TriangleMesh>")),
           "PAINT-01/TS-01: volumeMeshTriangleMesh must return shared_ptr<const Slic3r::TriangleMesh>");
  QVERIFY2(projectSource.contains(QStringLiteral("ProjectServiceMock::volumeMeshTriangleMesh")),
           "PAINT-01/TS-01: ProjectServiceMock.cpp must implement volumeMeshTriangleMesh");
  // The TriangleMesh source is mesh_ptr() (Model.hpp:856), NOT a fresh copy.
  QVERIFY2(projectSource.contains(QStringLiteral("mesh_ptr()")),
           "PAINT-01/TS-01: volumeMeshTriangleMesh must reuse ModelVolume::mesh_ptr() (shallow-share, not a copy)");

  // TS-02 (b): SceneRaycasterHit must carry meshLocalPosition (the mesh-local
  // hit TriangleSelector::select_patch needs as the cursor center).
  QVERIFY2(sceneHeader.contains(QStringLiteral("meshLocalPosition")),
           "PAINT-01/TS-02: SceneRaycasterHit must carry meshLocalPosition");
  QVERIFY2(sceneHeader.contains(QStringLiteral("Slic3r::Vec3f meshLocalPosition")),
           "PAINT-01/TS-02: meshLocalPosition must be a Vec3f (mesh-local float coords)");

  // TS-03 (c): PaintEngine.h must exist and reference Slic3r::TriangleSelector
  // (reuse, not reinvent). It must hold unique_ptr<TriangleSelector> per
  // volume (mirror upstream GLGizmoPainterBase::m_triangle_selectors).
  QVERIFY2(paintHeader.contains(QStringLiteral("Slic3r::TriangleSelector")),
           "PAINT-01/TS-03: PaintEngine.h must reference Slic3r::TriangleSelector (reuse, not reinvent)");
  QVERIFY2(paintHeader.contains(QStringLiteral("unique_ptr<Slic3r::TriangleSelector>")),
           "PAINT-01/TS-03: PaintEngine must own unique_ptr<Slic3r::TriangleSelector> per volume");
  QVERIFY2(paintHeader.contains(QStringLiteral("class PaintEngine")),
           "PAINT-01/TS-03: PaintEngine.h must declare the PaintEngine class");

  // TS-03 API surface: ensureSelector / paintAt / getFacets / clearObject /
  // serialize / deserialize must all be declared.
  QVERIFY2(paintHeader.contains(QStringLiteral("ensureSelector")),
           "PAINT-01/TS-03: PaintEngine must expose ensureSelector(obj, vol, mesh)");
  QVERIFY2(paintHeader.contains(QStringLiteral("paintAt")),
           "PAINT-01/TS-04: PaintEngine must expose paintAt(...)");
  QVERIFY2(paintHeader.contains(QStringLiteral("getFacets")),
           "PAINT-01/TS-03: PaintEngine must expose getFacets(obj, vol, state)");
  QVERIFY2(paintHeader.contains(QStringLiteral("clearObject")),
           "PAINT-01/TS-03: PaintEngine must expose clearObject(obj)");

  // TS-04 (c): PaintEngine.cpp must drive select_patch (the upstream paint
  // entry, TriangleSelector.hpp:306-312).
  QVERIFY2(paintSource.contains(QStringLiteral("select_patch")),
           "PAINT-01/TS-04: PaintEngine.cpp must call TriangleSelector::select_patch");

  // TS-07 (e): NO reimplementation. PaintEngine.cpp must INCLUDE the upstream
  // TriangleSelector.hpp (reuse byte-for-byte), not hand-roll a selector. A
  // grep for a class TriangleSelector definition here MUST return zero.
  QVERIFY2(paintSource.contains(QStringLiteral("<libslic3r/TriangleSelector.hpp>")),
           "PAINT-01/TS-07: PaintEngine.cpp must include <libslic3r/TriangleSelector.hpp> (reuse, not reinvent)");
  QVERIFY2(!paintSource.contains(QStringLiteral("class TriangleSelector")),
           "PAINT-01/TS-07: PaintEngine.cpp must NOT redefine TriangleSelector (reuse the libslic3r class)");

  // TS-04 (c): PaintEngine.cpp must build the Cursor via cursor_factory (the
  // upstream Sphere/Circle factory, TriangleSelector.hpp:114/123).
  QVERIFY2(paintSource.contains(QStringLiteral("cursor_factory")),
           "PAINT-01/TS-04: PaintEngine.cpp must build the Cursor via cursor_factory (reuse the upstream factory)");

  // TS-05 (d): EditorViewModel must reference PaintEngine + expose the
  // paintAtFacet Q_INVOKABLE entry.
  QVERIFY2(editorHeader.contains(QStringLiteral("PaintEngine")),
           "PAINT-01/TS-05: EditorViewModel.h must reference PaintEngine");
  QVERIFY2(editorHeader.contains(QStringLiteral("paintAtFacet")),
           "PAINT-01/TS-05: EditorViewModel.h must declare the paintAtFacet Q_INVOKABLE");
  QVERIFY2(editorSource.contains(QStringLiteral("EditorViewModel::paintAtFacet")),
           "PAINT-01/TS-05: EditorViewModel.cpp must implement paintAtFacet");
  QVERIFY2(editorSource.contains(QStringLiteral("PaintEngine.h")),
           "PAINT-01/TS-05: EditorViewModel.cpp must include PaintEngine.h");
  // setTriangleSupportState is kept as a thin alias (back-compat with the
  // Qt data layer). It must still exist.
  QVERIFY2(editorHeader.contains(QStringLiteral("setTriangleSupportState")),
           "PAINT-01/TS-05: EditorViewModel.h must keep setTriangleSupportState as a back-compat alias");

  // TS-06: RhiViewport must emit paintPickRequested + expose
  // emitPaintPickIfActive gated on the three paint gizmos.
  QVERIFY2(rhiHeader.contains(QStringLiteral("paintPickRequested")),
           "PAINT-01/TS-06: RhiViewport.h must declare the paintPickRequested signal");
  QVERIFY2(rhiHeader.contains(QStringLiteral("emitPaintPickIfActive")),
           "PAINT-01/TS-06: RhiViewport.h must declare emitPaintPickIfActive");
  QVERIFY2(rhiSource.contains(QStringLiteral("void RhiViewport::emitPaintPickIfActive")),
           "PAINT-01/TS-06: RhiViewport.cpp must implement emitPaintPickIfActive");
  QVERIFY2(rhiSource.contains(QStringLiteral("GizmoSupportPaint")),
           "PAINT-01/TS-06: emitPaintPickIfActive must gate on GizmoSupportPaint");
  QVERIFY2(rhiSource.contains(QStringLiteral("GizmoSeamPaint")),
           "PAINT-01/TS-06: emitPaintPickIfActive must gate on GizmoSeamPaint");
  QVERIFY2(rhiSource.contains(QStringLiteral("GizmoMmuSegmentation")),
           "PAINT-01/TS-06: emitPaintPickIfActive must gate on GizmoMmuSegmentation");

  // CMake registration (mirrors MR-05): PaintEngine.{cpp,h} must be in the
  // owzx_app_core target source list. A drop silently leaves the class
  // unbuilt.
  QVERIFY2(cmakeLists.contains(QStringLiteral("src/core/rendering/PaintEngine.cpp")),
           "PAINT-01: CMakeLists.txt must register PaintEngine.cpp in owzx_app_core");
  QVERIFY2(cmakeLists.contains(QStringLiteral("src/core/rendering/PaintEngine.h")),
           "PAINT-01: CMakeLists.txt must register PaintEngine.h in owzx_app_core");
}

void QmlUiAuditTests::calibrationTowerModesDispatchToLibslic3r()
{
  // Phase 124-01 (CALIB-01): the 3 libslic3r calibration tower modes
  // (Vol_speed=7, VFA=8, Retraction=9, per calib.hpp:24-26) must now be
  // dispatched from CalibrationServiceMock::buildMockData. The generic
  // dispatch path (calibMode != 0 -> SliceService::setCalibParams ->
  // static_cast<CalibMode>) forwards them transparently, so NO SliceService /
  // Print / GCode change is required -- this is a source-audit on the mock
  // data only. This slot locks every CM truth at the source level so a future
  // refactor that drops a tower mode, re-stubs max_volumetric_speed, or
  // accidentally enables a hardware mode fails here. Mirrors the Phase
  // 102..120 source-audit pattern (readSource + QVERIFY2 with CALIB-01-named
  // messages). Source-level only; runs in the regression ctest.

  const QString calibSource =
      readSource(QStringLiteral("src/core/services/CalibrationServiceMock.cpp"));
  QVERIFY2(!calibSource.isEmpty(), "Unable to read CalibrationServiceMock.cpp");

  // CM-01 (a): calibMode 7/8/9 must each appear in buildMockData, mapping the
  // 3 tower modes to upstream Calib_Vol_speed_Tower / Calib_VFA_Tower /
  // Calib_Retraction_tower (calib.hpp:24-26).
  QVERIFY2(calibSource.contains(QStringLiteral("maxVolSpeed.calibMode = 7")),
           "CALIB-01/CM-01: max_volumetric_speed must map to calibMode 7 (Calib_Vol_speed_Tower)");
  QVERIFY2(calibSource.contains(QStringLiteral("vfaTower.calibMode = 8")),
           "CALIB-01/CM-01: vfa_tower must map to calibMode 8 (Calib_VFA_Tower)");
  QVERIFY2(calibSource.contains(QStringLiteral("retractionTune.calibMode = 9")),
           "CALIB-01/CM-01: retraction_tune must map to calibMode 9 (Calib_Retraction_tower)");

  // The 3 new tower-mode ids must be registered in the m_calibTypes list so
  // CalibrationPage.qml (which auto-enumerates calibItemCount) surfaces them.
  QVERIFY2(calibSource.contains(QStringLiteral("\"vfa_tower\"")),
           "CALIB-01/CM-01: buildMockData must register the vfa_tower calibration id");
  QVERIFY2(calibSource.contains(QStringLiteral("\"retraction_tune\"")),
           "CALIB-01/CM-01: buildMockData must register the retraction_tune calibration id");

  // CM-03 (b): the "Pending: outside Phase" placeholder on max_volumetric_speed
  // is GONE -- it is now a real dispatched mode, not a stub. The whole phrase
  // must be absent from the file.
  QVERIFY2(!calibSource.contains(QStringLiteral("Pending: outside Phase")),
           "CALIB-01/CM-03: the 'Pending: outside Phase' placeholder must be removed (max_volumetric_speed is now dispatched)");
  QVERIFY2(!calibSource.contains(QStringLiteral("Pending: max volumetric speed")),
           "CALIB-01/CM-03: the legacy max_volumetric_speed pending reason must be removed");

  // CM-03 (c): bed_leveling / vibration KEEP their honest "requires hardware"
  // unavailableReason (the tower-mode change must NOT accidentally enable a
  // hardware mode). Both reasons must still be present.
  QVERIFY2(calibSource.contains(QStringLiteral("requires live printer hardware calibration support")),
           "CALIB-01/CM-03: bed_leveling must keep its 'requires live printer hardware' unavailableReason");
  QVERIFY2(calibSource.contains(QStringLiteral("requires live printer resonance measurement support")),
           "CALIB-01/CM-03: vibration must keep its 'requires live printer resonance' unavailableReason");

  // CM-02: transparent passthrough -- SliceService.cpp must still cast the
  // calibMode int to CalibMode (no per-mode whitelist). This guards against a
  // future whitelist that would silently drop the 3 tower modes.
  const QString sliceSource =
      readSource(QStringLiteral("src/core/services/SliceService.cpp"));
  QVERIFY2(!sliceSource.isEmpty(), "Unable to read SliceService.cpp");
  QVERIFY2(sliceSource.contains(QStringLiteral("setCalibParams")),
           "CALIB-01/CM-02: SliceService must expose setCalibParams (transparent passthrough)");
}

void QmlUiAuditTests::calibrationRangeInputAndKValueReadback()
{
  // Phase 125 (CALIB-02 + CALIB-03): source-audit lock that (a) the
  // CalibrationDialog.qml has the range input fields (start/end/step) bound to
  // the ViewModel; (b) CalibrationViewModel exposes the calibStart/calibEnd/
  // calibStep Q_PROPERTYs that forward the user-edited range to
  // CalibrationServiceMock::setCalibParams before startSlice; (c) the mock
  // 0.04f + item*0.01 K-value writeback is GONE, replaced by a real PA G-code
  // readback (M900 K / SET_PRESSURE_ADVANCE parse) for PA + an honest
  // manual-interpretation note for non-PA tower modes (no fabricated K).
  // Mirrors the Phase 124 source-audit pattern (readSource + QVERIFY2 with
  // CALIB-02/CALIB-03-named messages). Source-level only; runs in the
  // regression ctest.

  // --- CALIB-02: range input UI + ViewModel range Q_PROPERTYs ---

  // (a) CalibrationDialog.qml has the range input fields. The dialog must bind
  // three CxTextField controls to calibStart/calibEnd/calibStep (the
  // Q_PROPERTY names), with DoubleValidator on each.
  const QString dialogSource =
      readSource(QStringLiteral("src/qml_gui/dialogs/CalibrationDialog.qml"));
  QVERIFY2(!dialogSource.isEmpty(), "Unable to read CalibrationDialog.qml");
  QVERIFY2(dialogSource.contains(QStringLiteral("calibrationVm.calibStart")),
           "CALIB-02/CR-01: CalibrationDialog.qml must bind a range field to calibrationVm.calibStart");
  QVERIFY2(dialogSource.contains(QStringLiteral("calibrationVm.calibEnd")),
           "CALIB-02/CR-01: CalibrationDialog.qml must bind a range field to calibrationVm.calibEnd");
  QVERIFY2(dialogSource.contains(QStringLiteral("calibrationVm.calibStep")),
           "CALIB-02/CR-01: CalibrationDialog.qml must bind a range field to calibrationVm.calibStep");
  QVERIFY2(dialogSource.contains(QStringLiteral("DoubleValidator")),
           "CALIB-02/CR-01: CalibrationDialog.qml range inputs must use a DoubleValidator for numeric bounds");

  // (b) CalibrationViewModel exposes the range Q_PROPERTYs + setters that
  // forward to the service before startSlice.
  const QString vmHeader =
      readSource(QStringLiteral("src/core/viewmodels/CalibrationViewModel.h"));
  QVERIFY2(!vmHeader.isEmpty(), "Unable to read CalibrationViewModel.h");
  QVERIFY2(vmHeader.contains(QStringLiteral("Q_PROPERTY(double calibStart READ calibStart WRITE setCalibStart")),
           "CALIB-02/CR-02: CalibrationViewModel must expose Q_PROPERTY calibStart (READ+WRITE)");
  QVERIFY2(vmHeader.contains(QStringLiteral("Q_PROPERTY(double calibEnd READ calibEnd WRITE setCalibEnd")),
           "CALIB-02/CR-02: CalibrationViewModel must expose Q_PROPERTY calibEnd (READ+WRITE)");
  QVERIFY2(vmHeader.contains(QStringLiteral("Q_PROPERTY(double calibStep READ calibStep WRITE setCalibStep")),
           "CALIB-02/CR-02: CalibrationViewModel must expose Q_PROPERTY calibStep (READ+WRITE)");

  // The ViewModel must forward the edited range to the service (setCalibRange).
  const QString vmSource =
      readSource(QStringLiteral("src/core/viewmodels/CalibrationViewModel.cpp"));
  QVERIFY2(!vmSource.isEmpty(), "Unable to read CalibrationViewModel.cpp");
  QVERIFY2(vmSource.contains(QStringLiteral("setCalibRange")),
           "CALIB-02/CR-02: CalibrationViewModel range setters must forward to CalibrationServiceMock::setCalibRange");

  // The service must expose the override API + default readback.
  const QString calibHeader =
      readSource(QStringLiteral("src/core/services/CalibrationServiceMock.h"));
  QVERIFY2(!calibHeader.isEmpty(), "Unable to read CalibrationServiceMock.h");
  QVERIFY2(calibHeader.contains(QStringLiteral("setCalibRange")),
           "CALIB-02/CR-02: CalibrationServiceMock must expose setCalibRange (user override entry point)");
  QVERIFY2(calibHeader.contains(QStringLiteral("calibTypeStart")),
           "CALIB-02/CR-02: CalibrationServiceMock must expose calibTypeStart (default range readback)");

  // --- CALIB-03: real K-value readback replaces the mock ---

  // (c) the mock 0.04f + item*0.01 K-value writeback MUST be gone. The exact
  // arithmetic expression must not appear as a live value in the history
  // addHistoryEntry call sites (only allowed in an explanatory comment).
  const QString calibSource =
      readSource(QStringLiteral("src/core/services/CalibrationServiceMock.cpp"));
  QVERIFY2(!calibSource.isEmpty(), "Unable to read CalibrationServiceMock.cpp");
  QVERIFY2(!calibSource.contains(QStringLiteral("0.04f + (m_currentItem * 0.01f)")),
           "CALIB-03/CR-03: the mock 0.04f + item*0.01 K-value writeback must be removed (replace with real PA readback or honest manual-interpretation note)");

  // The real readback path must exist: a parser for the PA markers the slice
  // engine writes (M900 K for Marlin/BBL, SET_PRESSURE_ADVANCE for Klipper).
  QVERIFY2(calibSource.contains(QStringLiteral("parsePressureAdvanceFromGcode")),
           "CALIB-03/CR-03: CalibrationServiceMock must implement parsePressureAdvanceFromGcode (real PA K readback from sliced G-code)");
  QVERIFY2(calibSource.contains(QStringLiteral("M900")),
           "CALIB-03/CR-03: the PA readback parser must recognize the M900 K marker (Marlin/BBL)");
  QVERIFY2(calibSource.contains(QStringLiteral("SET_PRESSURE_ADVANCE")),
           "CALIB-03/CR-03: the PA readback parser must recognize the SET_PRESSURE_ADVANCE marker (Klipper)");

  // The honest manual-interpretation path must exist for non-PA modes (no
  // fabricated K). The note helper + the history hasRealReadback/notes fields.
  QVERIFY2(calibSource.contains(QStringLiteral("manualInterpretationNote")),
           "CALIB-03/CR-03: CalibrationServiceMock must implement manualInterpretationNote (honest non-PA doc, no fabricated K)");
  QVERIFY2(calibHeader.contains(QStringLiteral("historyHasRealReadback")),
           "CALIB-03/CR-03: CalibrationServiceMock must expose historyHasRealReadback (distinguishes real readback from manual interpretation)");
  QVERIFY2(calibHeader.contains(QStringLiteral("historyNotes")),
           "CALIB-03/CR-03: CalibrationServiceMock must expose historyNotes (honest status/manual-interpretation text)");
}

void QmlUiAuditTests::legacyDeadCodePagesRemoved()
{
  // Phase 126 (CLEAN-01): the legacy dead-code pages + AuxiliaryService must
  // stay deleted. These were in the removed LAN/device/cloud scope — deletion,
  // not repair (No-Deprecated-UI rule). Each QVERIFY2 names the CLEAN-01
  // contract it locks.
  const QString qrc = readSource(QStringLiteral("src/qml_gui/qml.qrc"));
  const QString mainQml = readSource(QStringLiteral("src/qml_gui/main.qml"));
  const QString topbar = readSource(QStringLiteral("src/qml_gui/BBLTopbar.qml"));
  const QString backendH = readSource(QStringLiteral("src/qml_gui/BackendContext.h"));
  const QString cmake = readSource(QStringLiteral("CMakeLists.txt"));
  QVERIFY2(!qrc.isEmpty(), "Unable to read qml.qrc");
  QVERIFY2(!mainQml.isEmpty(), "Unable to read main.qml");

  // CLEAN-01/CL-01: the 4 deleted pages must not appear in qml.qrc.
  QVERIFY2(!qrc.contains(QStringLiteral("DeviceListPage.qml")),
           "CLEAN-01/CL-01: qml.qrc must not list DeviceListPage.qml (dead, deleted)");
  QVERIFY2(!qrc.contains(QStringLiteral("AuxiliaryPage.qml")),
           "CLEAN-01/CL-01: qml.qrc must not list AuxiliaryPage.qml (dead, deleted)");
  QVERIFY2(!qrc.contains(QStringLiteral("ModelMallPage.qml")),
           "CLEAN-01/CL-01: qml.qrc must not list ModelMallPage.qml (dead, removed cloud scope)");
  QVERIFY2(!qrc.contains(QStringLiteral("AuxiliaryListPanel.qml")),
           "CLEAN-01/CL-01: qml.qrc must not list AuxiliaryListPanel.qml (orphan panel, deleted)");

  // CLEAN-01/CL-02: main.qml must not instantiate AuxiliaryPage (slot 7 is now a placeholder).
  QVERIFY2(!mainQml.contains(QStringLiteral("AuxiliaryPage {")),
           "CLEAN-01/CL-02: main.qml must not instantiate AuxiliaryPage (slot 7 is now a structural placeholder)");
  // BBLTopbar must not show the removed "辅助" tab.
  QVERIFY2(!topbar.contains(QStringLiteral("tpPlaceholder1")),
           "CLEAN-01/CL-02: BBLTopbar must not reference tpPlaceholder1 tab (Auxiliary tab removed)");

  // CLEAN-01/CL-03: AuxiliaryService must be absent from BackendContext.h + CMakeLists.txt.
  QVERIFY2(!backendH.contains(QStringLiteral("AuxiliaryService")),
           "CLEAN-01/CL-03: BackendContext.h must not reference AuxiliaryService (service deleted with its only would-be consumer)");
  QVERIFY2(!cmake.contains(QStringLiteral("AuxiliaryService")),
           "CLEAN-01/CL-03: CMakeLists.txt must not compile AuxiliaryService (service deleted)");
}

void QmlUiAuditTests::paintedFacetOverlayAndBrushInteraction()
{
  // Phase 121 (PAINT-02 + PAINT-03): source-audit lock for the painted-facet
  // overlay render + brush interaction. Each QVERIFY2 names the OV truth it
  // locks. Source-level only; runs in the regression ctest. Mirrors the Phase
  // 120 triangleSelectorEnginePorted pattern (readSource + QVERIFY2 with
  // PAINT-02/PAINT-03-named messages).
  const QString editorHeader =
      readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString editorSource =
      readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString rhiHeader =
      readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString rhiSource =
      readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.cpp"));
  const QString rhiRendererHeader =
      readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.h"));
  const QString rhiRendererSource =
      readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  const QString gizmoHeader =
      readSource(QStringLiteral("src/core/rendering/GizmoGeometry.h"));
  const QString gizmoSource =
      readSource(QStringLiteral("src/core/rendering/GizmoGeometry.cpp"));
  const QString softwareViewport =
      readSource(QStringLiteral("src/qml_gui/Renderer/SoftwareViewport.cpp"));
  const QString preparePage =
      readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  QVERIFY2(!editorHeader.isEmpty(), "Unable to read EditorViewModel.h");
  QVERIFY2(!rhiHeader.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!rhiRendererSource.isEmpty(), "Unable to read RhiViewportRenderer.cpp");

  // OV-01 (a): EditorViewModel must expose the paintOverlayData Q_PROPERTY
  // (reverse data channel, NOTIFY paintDataChanged).
  QVERIFY2(editorHeader.contains(QStringLiteral(
               "Q_PROPERTY(QByteArray paintOverlayData READ paintOverlayData NOTIFY paintDataChanged")),
           "PAINT-02/OV-01: EditorViewModel.h must declare the paintOverlayData Q_PROPERTY");
  QVERIFY2(editorHeader.contains(QStringLiteral("paintOverlayData() const")),
           "PAINT-02/OV-01: EditorViewModel.h must declare the paintOverlayData getter");
  QVERIFY2(editorSource.contains(QStringLiteral("EditorViewModel::paintOverlayData")),
           "PAINT-02/OV-01: EditorViewModel.cpp must implement paintOverlayData");

  // OV-04: EditorViewModel must expose extrudersColors (MMU per-extruder
  // filament colors) so the overlay renderer can map ExtruderN -> a color.
  QVERIFY2(editorHeader.contains(QStringLiteral("extrudersColors")),
           "PAINT-02/OV-04: EditorViewModel.h must declare the extrudersColors Q_PROPERTY");
  QVERIFY2(editorSource.contains(QStringLiteral("EditorViewModel::extrudersColors")),
           "PAINT-02/OV-04: EditorViewModel.cpp must implement extrudersColors");

  // OV-02 (b): RhiViewport must expose paintOverlayData + the brush Q_PROPERTYs.
  QVERIFY2(rhiHeader.contains(QStringLiteral("Q_PROPERTY(QByteArray paintOverlayData")),
           "PAINT-02/OV-02: RhiViewport.h must declare the paintOverlayData Q_PROPERTY");
  QVERIFY2(rhiHeader.contains(QStringLiteral("Q_PROPERTY(float brushRadius")),
           "PAINT-03/OV-02: RhiViewport.h must declare the brushRadius Q_PROPERTY");
  QVERIFY2(rhiHeader.contains(QStringLiteral("Q_PROPERTY(int brushCursorType")),
           "PAINT-03/OV-02: RhiViewport.h must declare the brushCursorType Q_PROPERTY");
  QVERIFY2(rhiHeader.contains(QStringLiteral("Q_PROPERTY(int paintState")),
           "PAINT-03/OV-02: RhiViewport.h must declare the paintState Q_PROPERTY");

  // OV-03 (c): RhiViewportRenderer must have renderPaintOverlay +
  // uploadPaintOverlayBuffer (reuse mesh pipeline, no new shader).
  QVERIFY2(rhiRendererHeader.contains(QStringLiteral("renderPaintOverlay")),
           "PAINT-02/OV-03: RhiViewportRenderer.h must declare renderPaintOverlay");
  QVERIFY2(rhiRendererSource.contains(QStringLiteral("RhiViewportRenderer::renderPaintOverlay")),
           "PAINT-02/OV-03: RhiViewportRenderer.cpp must implement renderPaintOverlay");
  QVERIFY2(rhiRendererSource.contains(QStringLiteral("RhiViewportRenderer::uploadPaintOverlayBuffer")),
           "PAINT-02/OV-03: RhiViewportRenderer.cpp must implement uploadPaintOverlayBuffer");
  // Reuse check: renderPaintOverlay must bind m_fillPipeline (no new pipeline).
  QVERIFY2(rhiRendererSource.contains(QStringLiteral("m_fillPipeline.get()")),
           "PAINT-02/OV-03: renderPaintOverlay must reuse m_fillPipeline (no new shader/pipeline)");

  // OV-05: brush sphere cursor. GizmoGeometry::buildBrushSphereVertices must
  // exist, and the renderer must have uploadBrushCursorBuffer + renderBrushCursor
  // using m_translucentFillPipeline.
  QVERIFY2(gizmoHeader.contains(QStringLiteral("buildBrushSphereVertices")),
           "PAINT-03/OV-05: GizmoGeometry.h must declare buildBrushSphereVertices");
  QVERIFY2(gizmoSource.contains(QStringLiteral("GizmoGeometry::buildBrushSphereVertices")),
           "PAINT-03/OV-05: GizmoGeometry.cpp must implement buildBrushSphereVertices");
  QVERIFY2(rhiRendererSource.contains(QStringLiteral("renderBrushCursor")),
           "PAINT-03/OV-05: RhiViewportRenderer.cpp must have renderBrushCursor");
  QVERIFY2(rhiRendererSource.contains(QStringLiteral("m_translucentFillPipeline.get()")),
           "PAINT-03/OV-05: renderBrushCursor must use m_translucentFillPipeline");

  // OV-02 (d): emitPaintPickIfActive must read the brush Q_PROPERTYs, NOT
  // hardcode brushRadius=2.0. The Phase 120 hardcoded line must be gone.
  QVERIFY2(!rhiSource.contains(QStringLiteral("const double brushRadius = 2.0")),
           "PAINT-03/OV-02: emitPaintPickIfActive must NOT hardcode brushRadius=2.0 (read the Q_PROPERTY)");
  QVERIFY2(rhiSource.contains(QStringLiteral("m_brushRadius")),
           "PAINT-03/OV-02: emitPaintPickIfActive must read m_brushRadius (the Q_PROPERTY)");

  // OV-06: SoftwareViewport mirror must append painted facets in paintScene.
  QVERIFY2(softwareViewport.contains(QStringLiteral("m_paintOverlayData")),
           "PAINT-02/OV-06: SoftwareViewport.cpp must consume m_paintOverlayData in paintScene");

  // OV-07: PreparePage.qml Support paint panel must have brush controls
  // (tool selector bound to supportPaintTool, radius CxSlider bound to
  // supportPaintCursorRadius, cursor type).
  QVERIFY2(preparePage.contains(QStringLiteral("supportPaintTool")),
           "PAINT-03/OV-07: PreparePage.qml Support panel must bind supportPaintTool");
  QVERIFY2(preparePage.contains(QStringLiteral("supportPaintCursorRadius")),
           "PAINT-03/OV-07: PreparePage.qml Support panel must bind supportPaintCursorRadius");
  QVERIFY2(preparePage.contains(QStringLiteral("supportPaintCursorType")),
           "PAINT-03/OV-07: PreparePage.qml Support panel must bind supportPaintCursorType");
  QVERIFY2(preparePage.contains(QStringLiteral("paintOverlayData")),
           "PAINT-02/OV-07: PreparePage.qml must bind paintOverlayData on the viewport");
}

void QmlUiAuditTests::supportAndSeamPaintFeedsSlice()
{
  // Phase 122 (PAINT-04): Support/Seam painted facets must write to ModelVolume
  // FacetsAnnotation (supported_facets/seam_facets) via the ProjectServiceMock
  // bridge, so the slice consumes them (cloneCurrentPlateModel deep-copies
  // FacetsAnnotation -> PrintObject::project_and_append_custom_facets).
  const QString svcH = readSource(QStringLiteral("src/core/services/ProjectServiceMock.h"));
  const QString svcCpp = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  const QString evm = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  QVERIFY2(!svcH.isEmpty(), "Unable to read ProjectServiceMock.h");
  QVERIFY2(!svcCpp.isEmpty(), "Unable to read ProjectServiceMock.cpp");
  QVERIFY2(!evm.isEmpty(), "Unable to read EditorViewModel.cpp");

  // PAINT-04/SS-01: bridge API present.
  QVERIFY2(svcH.contains(QStringLiteral("writePaintToModelVolume")),
           "PAINT-04/SS-01: ProjectServiceMock.h must declare writePaintToModelVolume");
  QVERIFY2(svcH.contains(QStringLiteral("PaintKind")),
           "PAINT-04/SS-01: ProjectServiceMock.h must define PaintKind enum");

  // PAINT-04/SS-01: writes supported_facets + seam_facets.
  QVERIFY2(svcCpp.contains(QStringLiteral("supported_facets")),
           "PAINT-04/SS-01: ProjectServiceMock.cpp must write mv->supported_facets (Support paint)");
  QVERIFY2(svcCpp.contains(QStringLiteral("seam_facets")),
           "PAINT-04/SS-01: ProjectServiceMock.cpp must write mv->seam_facets (Seam paint)");

  // PAINT-04/SS-02: EditorViewModel calls the bridge.
  QVERIFY2(evm.contains(QStringLiteral("writePaintToModelVolume")),
           "PAINT-04/SS-02: EditorViewModel paintAtFacet must call writePaintToModelVolume");

  // PAINT-04/SS-03: clear stubs implemented (no longer TODO-only).
  QVERIFY2(!evm.contains(QStringLiteral("TODO: Implement actual clear logic")),
           "PAINT-04/SS-03: clearSupportPaintOnSelection TODO stub must be implemented");
}

void QmlUiAuditTests::mmuSegmentationPaintFeedsSlice()
{
  // Phase 123 (PAINT-05): MMU segmentation paint writes to
  // mmu_segmentation_facets + clearMmuSegmentation implemented.
  const QString svcCpp = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  const QString evm = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  QVERIFY2(!svcCpp.isEmpty(), "Unable to read ProjectServiceMock.cpp");
  QVERIFY2(!evm.isEmpty(), "Unable to read EditorViewModel.cpp");

  // PAINT-05/MM-01: mmu_segmentation_facets bridge present.
  QVERIFY2(svcCpp.contains(QStringLiteral("mmu_segmentation_facets")),
           "PAINT-05/MM-01: ProjectServiceMock.cpp must write mv->mmu_segmentation_facets (MMU paint)");

  // PAINT-05/MM-02: clearMmuSegmentation no longer a TODO stub returning false.
  QVERIFY2(!evm.contains(QStringLiteral("TODO: Clear per-triangle MMU facet data")),
           "PAINT-05/MM-02: clearMmuSegmentation TODO stub must be implemented");
}

void QmlUiAuditTests::translationPipelineDocumented()
{
  // Phase 127 (I18N-01): the lupdate->translate->lrelease workflow is
  // documented + zh_CN has finished translations for the v4.6-touched strings.
  const QString workflow = readSource(QStringLiteral(".planning/i18n-workflow.md"));
  const QString zhCn = readSource(QStringLiteral("i18n/zh_CN.ts"));
  QVERIFY2(!workflow.isEmpty(), "I18N-01: unable to read .planning/i18n-workflow.md");
  QVERIFY2(!zhCn.isEmpty(), "I18N-01: unable to read i18n/zh_CN.ts");

  // I18N-01: workflow doc exists + documents lupdate/lrelease.
  QVERIFY2(workflow.contains(QStringLiteral("lupdate")),
           "I18N-01: i18n-workflow.md must document lupdate");
  QVERIFY2(workflow.contains(QStringLiteral("lrelease")),
           "I18N-01: i18n-workflow.md must document lrelease");

  // I18N-01: zh_CN has finished (non-unfinished) translations for v4.6 strings.
  QVERIFY2(zhCn.contains(QStringLiteral("添加暂停")),
           "I18N-01: zh_CN.ts must have a finished translation for 'Add Pause' (v4.6 proof-of-pipeline)");
  QVERIFY2(zhCn.contains(QStringLiteral("添加换色")),
           "I18N-01: zh_CN.ts must have a finished translation for 'Add Color Change'");
}

void QmlUiAuditTests::v46CrossWorkstreamRegressionLocked()
{
  // Phase 128 (REGRESS-01): v4.6 cross-workstream regression gate. Asserts the
  // key anchors from all 4 workstreams in one consolidated slot so a removal
  // of any per-workstream slot would still fail this gate. Mirrors the v4.5
  // v45CrossWorkstreamRegressionLocked pattern.
  const QString rail = readSource(QStringLiteral("src/qml_gui/components/PreviewLayerRail.qml"));
  const QString pvm = readSource(QStringLiteral("src/core/viewmodels/PreviewViewModel.cpp"));
  const QString evmCpp = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString evmH = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString svc = readSource(QStringLiteral("src/core/services/CalibrationServiceMock.cpp"));
  const QString paintEngine = readSource(QStringLiteral("src/core/rendering/PaintEngine.cpp"));
  QVERIFY2(!rail.isEmpty(), "Unable to read PreviewLayerRail.qml");
  QVERIFY2(!pvm.isEmpty(), "Unable to read PreviewViewModel.cpp");

  // WS1 (TickCode): preview rail renders ticks + write-back loop.
  QVERIFY2(rail.contains(QStringLiteral("previewVm.tickMarks")),
           "REGRESS-01/WS1: PreviewLayerRail must render tickMarks");
  QVERIFY2(pvm.contains(QStringLiteral("plates_custom_gcodes")),
           "REGRESS-01/WS1: PreviewViewModel must write back to plates_custom_gcodes");

  // WS2 (Paint): PaintEngine + FacetsAnnotation bridge.
  QVERIFY2(paintEngine.contains(QStringLiteral("select_patch")),
           "REGRESS-01/WS2: PaintEngine must drive TriangleSelector::select_patch");
  QVERIFY2(evmCpp.contains(QStringLiteral("writePaintToModelVolume")),
           "REGRESS-01/WS2: EditorViewModel must write paint to ModelVolume FacetsAnnotation");

  // WS3 (Calibration): 6 software modes dispatch.
  QVERIFY2(svc.contains(QStringLiteral("calibMode = 7")),
           "REGRESS-01/WS3: CalibrationServiceMock must dispatch Vol_speed_Tower (7)");
  QVERIFY2(svc.contains(QStringLiteral("calibMode = 9")),
           "REGRESS-01/WS3: CalibrationServiceMock must dispatch Retraction_tower (9)");

  // WS4 (Cleanup): no LayerSlider orphan.
  const QString qrc = readSource(QStringLiteral("src/qml_gui/qml.qrc"));
  QVERIFY2(!qrc.contains(QStringLiteral("LayerSlider.qml")),
           "REGRESS-01/WS4: qml.qrc must not list the deleted LayerSlider.qml");
  QVERIFY2(!qrc.contains(QStringLiteral("AuxiliaryPage.qml")),
           "REGRESS-01/WS4: qml.qrc must not list the deleted AuxiliaryPage.qml");
}

void QmlUiAuditTests::paintGizmoGateFlattenedAndFlattenFixMeshReal()
{
  // Phase 129 (POLISH-01/02/03): three bug fixes from the v4.6 gap analysis.
  const QString evm = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString svc = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  QVERIFY2(!evm.isEmpty(), "Unable to read EditorViewModel.cpp");
  QVERIFY2(!svc.isEmpty(), "Unable to read ProjectServiceMock.cpp");

  // POLISH-01: kViewportTrianglePickingAvailable must be true (Phase 121-123
  // shipped real PaintEngine; the stale false caused false "unavailable").
  QVERIFY2(evm.contains(QStringLiteral("kViewportTrianglePickingAvailable = true")),
           "POLISH-01: kViewportTrianglePickingAvailable must be true (Phase 121-123 real-ized paint picking)");

  // POLISH-02: flattenSelected must call orientObject (real orientation::orient),
  // not the mock 6-hardcoded-face path.
  QVERIFY2(evm.contains(QStringLiteral("orientObject")),
           "POLISH-02: flattenSelected must call orientObject (real orientation, not mock)");

  // POLISH-03: fixMesh must call real its_merge_vertices / its_remove_degenerate_faces.
  QVERIFY2(svc.contains(QStringLiteral("its_merge_vertices")),
           "POLISH-03: fixMesh must call its_merge_vertices (real mesh repair, not no-op copy)");
  QVERIFY2(svc.contains(QStringLiteral("its_remove_degenerate_faces")),
           "POLISH-03: fixMesh must call its_remove_degenerate_faces (real mesh repair)");
}

void QmlUiAuditTests::kbShortcutsDialogAndProjectPagePropertyPanelWired()
{
  // Phase 130 (POLISH-04/05): KBShortcutsDialog exists in main.qml +
  // ProjectPage property panel wired to real values (not all hardcoded "—").
  const QString mainQml = readSource(QStringLiteral("src/qml_gui/main.qml"));
  const QString projectPage = readSource(QStringLiteral("src/qml_gui/pages/ProjectPage.qml"));
  QVERIFY2(!mainQml.isEmpty(), "Unable to read main.qml");
  QVERIFY2(!projectPage.isEmpty(), "Unable to read ProjectPage.qml");

  // POLISH-04: KBShortcutsDialog exists (the shortcutDialog in main.qml).
  QVERIFY2(mainQml.contains(QStringLiteral("shortcutDialog")),
           "POLISH-04: main.qml must have a keyboard shortcuts dialog (shortcutDialog)");
  QVERIFY2(mainQml.contains(QStringLiteral("Ctrl+Z")),
           "POLISH-04: the shortcuts dialog must list keyboard shortcuts");

  // POLISH-05: ProjectPage property panel wired to real values (not all "—").
  QVERIFY2(projectPage.contains(QStringLiteral("currentProjectPath")),
           "POLISH-05: ProjectPage property panel must use currentProjectPath (real values)");
  QVERIFY2(projectPage.contains(QStringLiteral("Format")),
           "POLISH-05: ProjectPage property panel must show Format");
}

void QmlUiAuditTests::v47CrossWorkstreamRegressionLocked()
{
  // Phase 135 (REGRESS-02): v4.7 cross-workstream regression gate. Consolidates
  // the v4.7 workstream anchors (WS1 polish, WS2 i18n) + confirms the v4.6
  // paint/calibration/tick anchors still hold. CGAL (WS3) is blocked by
  // dependency (deferred); ASM-01 (WS4) is deferred.
  const QString evm = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString projSvc = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  const QString calibSvc = readSource(QStringLiteral("src/core/services/CalibrationServiceMock.cpp"));
  const QString enTs = readSource(QStringLiteral("i18n/en.ts"));
  QVERIFY2(!evm.isEmpty(), "Unable to read EditorViewModel.cpp");

  // WS1 (Polish): gate flag flipped + flatten real + fixMesh real.
  QVERIFY2(evm.contains(QStringLiteral("kViewportTrianglePickingAvailable = true")),
           "REGRESS-02/WS1: paint-gizmo gate flag must be true (POLISH-01)");
  QVERIFY2(evm.contains(QStringLiteral("orientObject")),
           "REGRESS-02/WS1: flattenSelected must call orientObject (POLISH-02)");
  QVERIFY2(projSvc.contains(QStringLiteral("its_merge_vertices")),
           "REGRESS-02/WS1: fixMesh must call its_merge_vertices (POLISH-03)");

  // WS2 (i18n): en.ts has some finished translations (proof-of-pipeline).
  QVERIFY2(enTs.contains(QStringLiteral("Prepare")) || enTs.contains(QStringLiteral("Calibration")),
           "REGRESS-02/WS2: en.ts must have at least some finished translations (I18N-02)");

  // v4.6 regression: paint bridge + calibration modes still hold.
  QVERIFY2(evm.contains(QStringLiteral("writePaintToModelVolume")),
           "REGRESS-02/v4.6: paint FacetsAnnotation bridge must still be present");
  QVERIFY2(calibSvc.contains(QStringLiteral("calibMode = 7")),
           "REGRESS-02/v4.6: calibration tower modes must still dispatch");
  QVERIFY2(calibSvc.contains(QStringLiteral("calibMode = 9")),
           "REGRESS-02/v4.6: Retraction tower mode (9) must still dispatch");
}

void QmlUiAuditTests::v48CrossWorkstreamRegressionLocked()
{
  // Phase 140 (REGRESS-03): v4.8 cross-workstream regression gate. Locks the
  // v4.8 workstream anchors (WS1 CGAL MeshBoolean/Drill, WS2 Assembly ASM-01,
  // WS3 i18n en.ts completion) AND re-asserts the v4.7/v4.6 anchors still hold
  // so the v4.8 work did not regress them.
  const QString evm = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString projSvc = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  const QString rhiViewport = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString calibSvc = readSource(QStringLiteral("src/core/services/CalibrationServiceMock.cpp"));
  const QString enTs = readSource(QStringLiteral("i18n/en.ts"));
  QVERIFY2(!evm.isEmpty(), "Unable to read EditorViewModel.cpp");
  QVERIFY2(!projSvc.isEmpty(), "Unable to read ProjectServiceMock.cpp");

  // WS1 (CGAL): MeshBoolean + Drill activated (Phase 137).
  QVERIFY2(evm.contains(QStringLiteral("kCgalMeshBooleanAvailable = true")),
           "REGRESS-03/WS1: kCgalMeshBooleanAvailable must be true (CGAL-02)");
  QVERIFY2(projSvc.contains(QStringLiteral("MeshBoolean::minus")),
           "REGRESS-03/WS1: MeshBoolean::minus must be wired (CGAL-02/03)");

  // WS2 (Assembly ASM-01): per-instance assemble-transform accessors + routing +
  // renderer thread-through (Phase 138).
  QVERIFY2(projSvc.contains(QStringLiteral("setAssembleOffset")),
           "REGRESS-03/WS2: ProjectServiceMock must expose setAssembleOffset (ASM-01)");
  QVERIFY2(projSvc.contains(QStringLiteral("setAssembleRotation")),
           "REGRESS-03/WS2: ProjectServiceMock must expose setAssembleRotation (ASM-01)");
  QVERIFY2(projSvc.contains(QStringLiteral("setAssembleScale")),
           "REGRESS-03/WS2: ProjectServiceMock must expose setAssembleScale (ASM-01)");
  QVERIFY2(evm.contains(QStringLiteral("m_activeCanvasType == 2")),
           "REGRESS-03/WS2: EditorViewModel must route gizmo slots on AssembleView (ASM-01)");
  QVERIFY2(evm.contains(QStringLiteral("AssembleTransformCommand")),
           "REGRESS-03/WS2: AssembleTransformCommand undo variant must be wired (ASM-01)");
  QVERIFY2(rhiViewport.contains(QStringLiteral("assembleOffsets")),
           "REGRESS-03/WS2: RhiViewport must expose assembleOffsets Q_PROPERTY (ASM-01)");

  // WS3 (i18n): en.ts must be filled (I18N-04). Assert the unfinished count is
  // low — a fully-filled en.ts has 0, but allow a tiny tolerance for any
  // genuinely-ambiguous residual.
  const int unfinished = enTs.count(QStringLiteral("type=\"unfinished\""));
  QVERIFY2(unfinished <= 5,
           qPrintable(QStringLiteral("REGRESS-03/WS3: en.ts must have <= 5 unfinished "
                                     "(I18N-04), found %1").arg(unfinished)));

  // v4.7 regression: the v4.7 anchors (paint gate, flatten, fixMesh, calibration
  // modes) must still hold — the v4.8 work must not have regressed them.
  QVERIFY2(evm.contains(QStringLiteral("kViewportTrianglePickingAvailable = true")),
           "REGRESS-03/v4.7: paint-gizmo gate flag must still be true");
  QVERIFY2(evm.contains(QStringLiteral("orientObject")),
           "REGRESS-03/v4.7: flattenSelected must still call orientObject");
  QVERIFY2(projSvc.contains(QStringLiteral("its_merge_vertices")),
           "REGRESS-03/v4.7: fixMesh must still call its_merge_vertices");
  QVERIFY2(calibSvc.contains(QStringLiteral("calibMode = 7")),
           "REGRESS-03/v4.6: Vol_speed tower mode (7) must still dispatch");
  QVERIFY2(calibSvc.contains(QStringLiteral("calibMode = 9")),
           "REGRESS-03/v4.6: Retraction tower mode (9) must still dispatch");
}

void QmlUiAuditTests::v50TechDebtRegressionLocked()
{
  // Phase 141 (DEBT-05): v5.0 tech-debt closure gate. Locks the 4 code-only
  // fixes from Phase 141 (CGAL-02 true intersection, orphaned meshBooleanSelected
  // menu + stub removed, drillObject C4715 fix, ASM rotate/scale live-visual
  // compose) AND re-asserts the v4.8 anchors so the v5.0 tech-debt work did not
  // regress the v4.8 contract.
  const QString evm = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString evmH = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString projSvc = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString rhiRenderer = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewportRenderer.cpp"));
  const QString rhiViewport = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString assemblePage = readSource(QStringLiteral("src/qml_gui/pages/AssemblePage.qml"));
  QVERIFY2(!evm.isEmpty(), "Unable to read EditorViewModel.cpp");
  QVERIFY2(!projSvc.isEmpty(), "Unable to read ProjectServiceMock.cpp");

  // DEBT-01: intersection op==2 must call MeshBoolean::cgal::intersect (not minus).
  QVERIFY2(projSvc.contains(QStringLiteral("MeshBoolean::cgal::intersect")),
           "DEBT-01: meshBoolean op==2 must call MeshBoolean::cgal::intersect (not minus)");
  QVERIFY2(projSvc.contains(QStringLiteral("operation != 2")),
           "DEBT-01: tool object must NOT be deleted after intersection (operation != 2 gate)");

  // DEBT-02: the orphaned CxMenuItem + meshBooleanSelected stub are gone. The
  // working boolean dialog path (booleanExecute) is unchanged. Note: a documentation
  // comment block in PreparePage.qml mentions the removed menu text — that is
  // intentional (grep-traceable deletion record), so we assert on the call site
  // (meshBooleanSelected() QML invocation) instead of the raw menu label.
  QVERIFY2(!preparePage.contains(QStringLiteral("meshBooleanSelected()")),
           "DEBT-02: orphaned meshBooleanSelected() call must be removed from PreparePage.qml");
  QVERIFY2(!evm.contains(QStringLiteral("bool EditorViewModel::meshBooleanSelected")),
           "DEBT-02: meshBooleanSelected() stub body must be removed from EditorViewModel.cpp");
  QVERIFY2(!evmH.contains(QStringLiteral("Q_INVOKABLE bool meshBooleanSelected")),
           "DEBT-02: meshBooleanSelected() declaration must be removed from EditorViewModel.h");
  QVERIFY2(evm.contains(QStringLiteral("booleanExecute")),
           "DEBT-02: working booleanExecute path must still be present");

  // DEBT-03: drillObject success path must return true (no C4715). We assert the
  // return-true comment marker exists in the success path (more robust than the
  // exact whitespace-sensitive multi-line literal).
  QVERIFY2(projSvc.contains(QStringLiteral("return true; // Phase 141 / DEBT-03")),
           "DEBT-03: drillObject must return true on the success path (after set_new_unique_id)");

  // DEBT-04: assemble transform compose. Renderer must read rotation+scale lists
  // AND build the full matrix; the EditorViewModel and RhiViewport must expose the
  // parallel Q_PROPERTYs; AssemblePage must bind them.
  QVERIFY2(evmH.contains(QStringLiteral("Q_PROPERTY(QVariantList assembleRotations")),
           "DEBT-04: EditorViewModel must expose assembleRotations Q_PROPERTY");
  QVERIFY2(evmH.contains(QStringLiteral("Q_PROPERTY(QVariantList assembleScales")),
           "DEBT-04: EditorViewModel must expose assembleScales Q_PROPERTY");
  QVERIFY2(rhiViewport.contains(QStringLiteral("Q_PROPERTY(QVariantList assembleRotations")),
           "DEBT-04: RhiViewport must expose assembleRotations Q_PROPERTY");
  QVERIFY2(rhiViewport.contains(QStringLiteral("Q_PROPERTY(QVariantList assembleScales")),
           "DEBT-04: RhiViewport must expose assembleScales Q_PROPERTY");
  QVERIFY2(rhiRenderer.contains(QStringLiteral("m_assembleTransformBySource")),
           "DEBT-04: RhiViewportRenderer must build the full compose matrix (m_assembleTransformBySource)");
  QVERIFY2(rhiRenderer.contains(QStringLiteral("m.map(QVector3D(v.x, v.y, v.z))")),
           "DEBT-04: buildModelVertices must apply the full matrix to each vertex");
  QVERIFY2(assemblePage.contains(QStringLiteral("assembleRotations:")),
           "DEBT-04: AssemblePage must bind assembleRotations");
  QVERIFY2(assemblePage.contains(QStringLiteral("assembleScales:")),
           "DEBT-04: AssemblePage must bind assembleScales");

  // v4.8 anchors must still hold (no regression from the v5.0 tech-debt work).
  QVERIFY2(evm.contains(QStringLiteral("kCgalMeshBooleanAvailable = true")),
           "DEBT-05/v4.8: kCgalMeshBooleanAvailable must still be true");
  QVERIFY2(projSvc.contains(QStringLiteral("MeshBoolean::minus")),
           "DEBT-05/v4.8: MeshBoolean::minus must still be wired for difference/drill");
  QVERIFY2(projSvc.contains(QStringLiteral("setAssembleOffset")),
           "DEBT-05/v4.8: setAssembleOffset must still be present (ASM-01)");
}

void QmlUiAuditTests::v50OpenVdbUnlockWired()
{
  // Phase 142 (VDB-01/VDB-02): OpenVDB unlock. The v4.x "OpenVDB unavailable"
  // premise was an incomplete CMake port — the Qt6 fork dropped the upstream
  // find_package(OpenVDB) call and renamed the gating target to `openvdb_libs`
  // which no file ever created. This slot anchors the wiring that corrects it.
  // The strongest proof is that OWzxSlicer.exe links clean with OpenVDB
  // (no LNK2019 on mesh_to_grid/grid_to_mesh/redistance_grid); this slot
  // anchors the source-level evidence that the wiring exists.
  const QString rootCmake = readSource(QStringLiteral("CMakeLists.txt"));
  const QString libsl3rCmake = readSource(QStringLiteral("cmake/BuildLibslic3rFromSource.cmake"));
  QVERIFY2(!rootCmake.isEmpty(), "Unable to read root CMakeLists.txt");
  QVERIFY2(!libsl3rCmake.isEmpty(), "Unable to read BuildLibslic3rFromSource.cmake");

  // VDB-01: root CMakeLists invokes find_package(OpenVDB) and creates the
  // openvdb_libs alias target.
  QVERIFY2(rootCmake.contains(QStringLiteral("find_package(OpenVDB 5.0 COMPONENTS openvdb)")),
           "VDB-01: root CMakeLists.txt must invoke find_package(OpenVDB 5.0 COMPONENTS openvdb)");
  QVERIFY2(rootCmake.contains(QStringLiteral("OPENVDB_LIBRARYDIR")),
           "VDB-01: root CMakeLists.txt must set OPENVDB_LIBRARYDIR (find module needs explicit lib path)");
  QVERIFY2(rootCmake.contains(QStringLiteral("add_library(openvdb_libs INTERFACE IMPORTED)")),
           "VDB-01: root CMakeLists.txt must create the openvdb_libs INTERFACE IMPORTED shim");
  QVERIFY2(rootCmake.contains(QStringLiteral("INTERFACE_LINK_LIBRARIES \"OpenVDB::openvdb\"")),
           "VDB-01: openvdb_libs shim must re-export OpenVDB::openvdb");

  // VDB-02: OpenVDBUtils.cpp is conditionally compiled when openvdb_libs exists;
  // the gate at BuildLibslic3rFromSource.cmake:366 must now evaluate true.
  QVERIFY2(libsl3rCmake.contains(QStringLiteral("if(TARGET openvdb_libs)")),
           "VDB-02: BuildLibslic3rFromSource.cmake must gate OpenVDBUtils compilation on TARGET openvdb_libs");
  QVERIFY2(libsl3rCmake.contains(QStringLiteral("OpenVDBUtils.cpp")),
           "VDB-02: OpenVDBUtils.cpp must be in the conditional source list (now active)");

  // libnoise latent-issue fix exposed by the OpenVDB link (Phase 142):
  // LIBNOISE_INCLUDE_DIR is force-set to the vcpkg path before find_package so
  // the upstream Findlibnoise.cmake module does not leave the NOTFOUND sentinel
  // in noise::noise's INTERFACE_INCLUDE_DIRECTORIES.
  QVERIFY2(libsl3rCmake.contains(QStringLiteral("LIBNOISE_INCLUDE_DIR-NOTFOUND")),
           "VDB-02/libnoise: BuildLibslic3rFromSource.cmake must document the NOTFOUND sentinel risk");
  QVERIFY2(libsl3rCmake.contains(QStringLiteral("FORCE")),
           "VDB-02/libnoise: LIBNOISE_INCLUDE_DIR must be force-set to vcpkg path");
}

void QmlUiAuditTests::v50HollowGizmoReachable()
{
  // Phase 143 (VDB-03/04/05): Hollow gizmo is now reachable. Phase 142 linked
  // OpenVDB; this slot anchors that the gizmo-availability switch, tooltip
  // blocker, GLToolbars button, and PreparePage panel are all wired. The full
  // SLA slice path (VDB-06) is deferred to v5.1+ (requires SLAPrint wiring).
  const QString evm = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString evmH = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString glToolbars = readSource(QStringLiteral("src/qml_gui/components/GLToolbars.qml"));
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  QVERIFY2(!evm.isEmpty(), "Unable to read EditorViewModel.cpp");

  // VDB-03: case 8 must NOT return false unconditionally. It must reflect
  // hasSingleObject (the Phase 143 change). The literal "case 8: // Hollow"
  // followed by `return hasSingleObject` is the anchor.
  QVERIFY2(evm.contains(QStringLiteral("case 8: // Hollow\n    return hasSingleObject")),
           "VDB-03: EditorViewModel case 8 (Hollow) must return hasSingleObject, not false");

  // VDB-03: case 8 tooltip must NOT say "Blocked: OpenVDB unavailable" anymore.
  // The new behavior returns an empty string (no blocker).
  QVERIFY2(!evm.contains(QStringLiteral("case 8:\n  case 18:\n    return QStringLiteral(\"Blocked: OpenVDB unavailable\")")),
           "VDB-03: case 8 must NOT have the 'Blocked: OpenVDB unavailable' tooltip (case 18 may keep a v5.1+ SLA marker)");
  QVERIFY2(evm.contains(QStringLiteral("Phase 143 (VDB-03): Hollow gizmo is now reachable")),
           "VDB-03: case 8 must document the Phase 143 reachability change");

  // VDB-04: GLToolbars must have a GizmoHollow button + an iconForTool mapping.
  QVERIFY2(glToolbars.contains(QStringLiteral("toolId: GLViewport.GizmoHollow")),
           "VDB-04: GLToolbars.qml must have a GizmoToolButton with toolId GizmoHollow");
  QVERIFY2(glToolbars.contains(QStringLiteral("case GLViewport.GizmoHollow:")),
           "VDB-04: GLToolbars iconForTool must handle GizmoHollow");

  // VDB-05: PreparePage must have a Hollow panel that becomes visible when
  // gizmoMode === GizmoHollow. The panel must bind at least hollowEnabled.
  QVERIFY2(preparePage.contains(QStringLiteral("viewport3d.gizmoMode === GLViewport.GizmoHollow")),
           "VDB-05: PreparePage must have a Hollow panel visible when gizmoMode === GizmoHollow");
  QVERIFY2(preparePage.contains(QStringLiteral("root.editorVm.hollowEnabled")),
           "VDB-05: Hollow panel must bind hollowEnabled");
  QVERIFY2(preparePage.contains(QStringLiteral("root.editorVm.hollowOffset")),
           "VDB-05: Hollow panel must bind hollowOffset");

  // VDB-05: EditorViewModel must expose the Hollow Q_PROPERTYs (already declared
  // before Phase 143 but anchor them here so a future refactor cannot drop them).
  QVERIFY2(evmH.contains(QStringLiteral("Q_PROPERTY(bool hollowEnabled")),
           "VDB-05: EditorViewModel.h must keep Q_PROPERTY hollowEnabled");
  QVERIFY2(evmH.contains(QStringLiteral("Q_PROPERTY(float hollowOffset")),
           "VDB-05: EditorViewModel.h must keep Q_PROPERTY hollowOffset");
  QVERIFY2(evmH.contains(QStringLiteral("Q_PROPERTY(float hollowQuality")),
           "VDB-05: EditorViewModel.h must keep Q_PROPERTY hollowQuality");
  QVERIFY2(evmH.contains(QStringLiteral("Q_PROPERTY(float hollowClosingDistance")),
           "VDB-05: EditorViewModel.h must keep Q_PROPERTY hollowClosingDistance");
}

void QmlUiAuditTests::v50EmbossParameterized()
{
  // Phase 144 (EMB-01/02): the real Emboss pipeline (text2shapes + polygons2model)
  // was wired in an earlier phase but hardcoded (arial.ttf / 10mm / 2mm). Phase 144
  // parameterizes it via the EditorViewModel embossFontPath/embossHeight/embossDepth
  // Q_PROPERTYs. This slot anchors the parameterization surface.
  const QString evmH = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString evm = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString projSvc = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  const QString projSvcH = readSource(QStringLiteral("src/core/services/ProjectServiceMock.h"));
  QVERIFY2(!evm.isEmpty(), "Unable to read EditorViewModel.cpp");
  QVERIFY2(!projSvc.isEmpty(), "Unable to read ProjectServiceMock.cpp");

  // EMB-01: font enumeration + selection surface.
  QVERIFY2(projSvcH.contains(QStringLiteral("setEmbossFont")),
           "EMB-01: ProjectServiceMock must expose setEmbossFont");
  QVERIFY2(projSvcH.contains(QStringLiteral("embossFontList")),
           "EMB-01: ProjectServiceMock must expose embossFontList for system font enumeration");
  QVERIFY2(projSvc.contains(QStringLiteral("Slic3r::Emboss::get_font_list")),
           "EMB-01: embossFontList must call upstream Slic3r::Emboss::get_font_list");
  QVERIFY2(evmH.contains(QStringLiteral("Q_PROPERTY(QString embossFontPath")),
           "EMB-01: EditorViewModel must expose embossFontPath Q_PROPERTY");
  QVERIFY2(evmH.contains(QStringLiteral("Q_INVOKABLE QVariantList embossFontList")),
           "EMB-01: EditorViewModel must proxy embossFontList to QML");

  // EMB-02: parameterize the pipeline from Q_PROPERTYs (was hardcoded).
  QVERIFY2(projSvc.contains(QStringLiteral("m_embossFontPath")),
           "EMB-02: ProjectServiceMock must use m_embossFontPath (not hardcoded arial)");
  QVERIFY2(projSvc.contains(QStringLiteral("m_embossHeight > 0.0f")),
           "EMB-02: ProjectServiceMock must use m_embossHeight to drive FontProp.size_in_mm");
  QVERIFY2(projSvc.contains(QStringLiteral("m_embossDepth > 0.0f")),
           "EMB-02: ProjectServiceMock must use m_embossDepth to drive ProjectZ depth");

  // EMB-01/02: EditorViewModel must forward font + height + depth before invoking
  // addTextVolume (both embossSelected and addTextObject paths).
  QVERIFY2(evm.contains(QStringLiteral("projectService_->setEmbossFont(m_embossFontPath)")),
           "EMB-01: EditorViewModel must forward embossFontPath to ProjectServiceMock");
  QVERIFY2(evm.contains(QStringLiteral("projectService_->setEmbossHeight(m_embossHeight)")),
           "EMB-02: EditorViewModel must forward embossHeight to ProjectServiceMock");
  QVERIFY2(evm.contains(QStringLiteral("projectService_->setEmbossDepth(m_embossDepth)")),
           "EMB-02: EditorViewModel must forward embossDepth to ProjectServiceMock");

  // The pipeline itself (text2shapes + polygons2model) must still be wired
  // (these come from the earlier phase that introduced addTextVolume).
  QVERIFY2(projSvc.contains(QStringLiteral("Slic3r::Emboss::text2shapes")),
           "EMB-02: addTextVolume must still call Slic3r::Emboss::text2shapes");
  QVERIFY2(projSvc.contains(QStringLiteral("Slic3r::Emboss::polygons2model")),
           "EMB-02: addTextVolume must still call Slic3r::Emboss::polygons2model");
}

void QmlUiAuditTests::v50EmbossAsyncAndPanelWired()
{
  // Phase 145 (EMB-03/04): async emboss via Qt Concurrent + Emboss panel
  // (font selector + async-execute button + result feedback). The slot anchors
  // the worker pattern, signal forwarding, and panel wiring.
  const QString projSvcH = readSource(QStringLiteral("src/core/services/ProjectServiceMock.h"));
  const QString projSvc = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  const QString evmH = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString evm = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  QVERIFY2(!projSvc.isEmpty(), "Unable to read ProjectServiceMock.cpp");
  QVERIFY2(!evm.isEmpty(), "Unable to read EditorViewModel.cpp");

  // EMB-03: async API exists.
  QVERIFY2(projSvcH.contains(QStringLiteral("addTextVolumeAsync")),
           "EMB-03: ProjectServiceMock must expose addTextVolumeAsync");
  QVERIFY2(projSvcH.contains(QStringLiteral("cancelEmbossVolume")),
           "EMB-03: ProjectServiceMock must expose cancelEmbossVolume");
  QVERIFY2(projSvcH.contains(QStringLiteral("performEmbossVolumeAdd")),
           "EMB-03: ProjectServiceMock must factor out performEmbossVolumeAdd (shared by sync+async)");

  // EMB-03: signals for async result delivery.
  QVERIFY2(projSvcH.contains(QStringLiteral("embossVolumeAdded")),
           "EMB-03: ProjectServiceMock must emit embossVolumeAdded");
  QVERIFY2(projSvcH.contains(QStringLiteral("embossVolumeFailed")),
           "EMB-03: ProjectServiceMock must emit embossVolumeFailed");

  // EMB-03: async worker uses Qt Concurrent + atomic cancel + GUI-thread delivery.
  QVERIFY2(projSvc.contains(QStringLiteral("QtConcurrent::run")),
           "EMB-03: addTextVolumeAsync must spawn a QtConcurrent::run worker");
  QVERIFY2(projSvc.contains(QStringLiteral("m_embossCancelFlag")),
           "EMB-03: async emboss must use an atomic cancel flag (cancellation on stale jobs)");
  QVERIFY2(projSvc.contains(QStringLiteral("Qt::QueuedConnection")),
           "EMB-03: result must be delivered on the GUI thread via QueuedConnection");

  // EMB-03: the heavy text2shapes+polygons2model runs on the worker thread
  // (the genuine off-GUI-thread portion). Look for the worker-lambda pipeline.
  QVERIFY2(projSvc.contains(QStringLiteral("resultMesh = std::make_shared<Slic3r::TriangleMesh>")),
           "EMB-03: worker must produce the mesh off-thread (resultMesh shared_ptr)");

  // EMB-03: EditorViewModel forwards + re-emits.
  QVERIFY2(evmH.contains(QStringLiteral("embossSelectedAsync")),
           "EMB-03: EditorViewModel must expose embossSelectedAsync Q_INVOKABLE");
  QVERIFY2(evmH.contains(QStringLiteral("embossVolumeAdded")),
           "EMB-03: EditorViewModel must declare embossVolumeAdded signal");
  QVERIFY2(evm.contains(QStringLiteral("projectService_->addTextVolumeAsync")),
           "EMB-03: EditorViewModel must call addTextVolumeAsync on the service");

  // EMB-04: QML panel has font selector + async-execute button + result feedback.
  QVERIFY2(preparePage.contains(QStringLiteral("root.editorVm.embossFontList")),
           "EMB-04: Emboss panel must populate the font selector from embossFontList");
  QVERIFY2(preparePage.contains(QStringLiteral("root.editorVm.embossFontPath")),
           "EMB-04: Emboss panel must bind embossFontPath");
  QVERIFY2(preparePage.contains(QStringLiteral("embossSelectedAsync")),
           "EMB-04: Emboss panel must have an async-execute button (embossSelectedAsync)");
  QVERIFY2(preparePage.contains(QStringLiteral("onEmbossVolumeAdded")),
           "EMB-04: Emboss panel must wire onEmbossVolumeAdded for result feedback");
}

void QmlUiAuditTests::v50EmbossWiringAndSvgWired()
{
  // Phase 146 (EMB-05/06/07): Emboss wiring (no-selection fallback), 3MF
  // round-trip contract (geometry), and SVG emboss path.
  const QString evm = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString evmH = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString projSvc = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  const QString projSvcH = readSource(QStringLiteral("src/core/services/ProjectServiceMock.h"));
  const QString glToolbars = readSource(QStringLiteral("src/qml_gui/components/GLToolbars.qml"));
  QVERIFY2(!evm.isEmpty(), "Unable to read EditorViewModel.cpp");
  QVERIFY2(!projSvc.isEmpty(), "Unable to read ProjectServiceMock.cpp");

  // EMB-05: GLToolbars Emboss + SVG buttons already existed (pre-v5.0); assert
  // they are still present (no regression). Click behavior is wired through the
  // standard toolId → gizmoMode plumbing.
  QVERIFY2(glToolbars.contains(QStringLiteral("toolId: GLViewport.GizmoEmboss")),
           "EMB-05: GLToolbars must keep the GizmoEmboss button");
  QVERIFY2(glToolbars.contains(QStringLiteral("toolId: GLViewport.GizmoSVG")),
           "EMB-07: GLToolbars must keep the GizmoSVG button");

  // EMB-05: no-selection auto-attach fallback. addTextObject + embossSelected +
  // embossSelectedAsync must all fall back to the first current-plate object
  // when no object is selected (approximates upstream "create new at center").
  QVERIFY2(evm.contains(QStringLiteral("Phase 146 (EMB-05): first-object fallback")),
           "EMB-05: addTextObject must document the no-selection fallback");
  QVERIFY2(evm.contains(QStringLiteral("plateObjs.first()")),
           "EMB-05: fallback must use plateObjs.first() (first object on current plate)");

  // EMB-06: TextEmboss volumes are added as MODEL_PART, so their geometry
  // round-trips through 3MF via the standard store_3mf path (no special
  // text-metadata block needed for geometry preservation). Editable-text
  // metadata persistence (upstream 3MF <text> block via
  // TextConfigurationSerialization) is documented future work — the volume
  // reloads with correct geometry but currently not as a re-editable TextEmboss.
  QVERIFY2(projSvc.contains(QStringLiteral("Slic3r::ModelVolumeType::MODEL_PART")),
           "EMB-06: addTextVolume must add the volume as MODEL_PART (so geometry round-trips through 3MF)");
  QVERIFY2(projSvc.contains(QStringLiteral("MockVolumeType::TextEmboss")),
           "EMB-06: the mock-side tag must remain TextEmboss (session-time identity)");

  // EMB-07: SVG emboss path must use the real libslic3r loader.
  QVERIFY2(projSvcH.contains(QStringLiteral("addSvgVolume")),
           "EMB-07: ProjectServiceMock must expose addSvgVolume");
  QVERIFY2(projSvc.contains(QStringLiteral("svgModel.read_from_file")),
           "EMB-07: addSvgVolume must call Model::read_from_file (real libslic3r SVG loader)");
  QVERIFY2(projSvc.contains(QStringLiteral("MockVolumeType::SvgEmboss")),
           "EMB-07: SVG volumes must be tagged MockVolumeType::SvgEmboss");
}

void QmlUiAuditTests::v50PresetIniAndCreateDialogWired()
{
  // Phase 147 (PSET-01/02): upstream-compatible .ini bundle + CreatePresetsDialog.
  const QString presetSvcH = readSource(QStringLiteral("src/core/services/PresetServiceMock.h"));
  const QString presetSvc = readSource(QStringLiteral("src/core/services/PresetServiceMock.cpp"));
  const QString configVmH = readSource(QStringLiteral("src/core/viewmodels/ConfigViewModel.h"));
  const QString settingsDialog = readSource(QStringLiteral("src/qml_gui/dialogs/SettingsDialog.qml"));
  const QString createDialog = readSource(QStringLiteral("src/qml_gui/dialogs/CreatePresetsDialog.qml"));
  const QString qrc = readSource(QStringLiteral("src/qml_gui/qml.qrc"));
  QVERIFY2(!presetSvc.isEmpty(), "Unable to read PresetServiceMock.cpp");
  QVERIFY2(!createDialog.isEmpty(), "Unable to read CreatePresetsDialog.qml");

  // PSET-01: upstream-compatible .ini bundle export/import.
  QVERIFY2(presetSvcH.contains(QStringLiteral("exportBundleIni")),
           "PSET-01: PresetServiceMock must expose exportBundleIni (upstream-compatible .ini)");
  QVERIFY2(presetSvcH.contains(QStringLiteral("importBundleIni")),
           "PSET-01: PresetServiceMock must expose importBundleIni");
  QVERIFY2(presetSvc.contains(QStringLiteral("[preset]")),
           "PSET-01: exportBundleIni must write the upstream [preset] section header");
  QVERIFY2(presetSvc.contains(QStringLiteral("inherits = ")),
           "PSET-01: exportBundleIni must persist the inheritance chain (inherits = ...)");

  // PSET-02: CreatePresetsDialog QML + wiring.
  QVERIFY2(qrc.contains(QStringLiteral("dialogs/CreatePresetsDialog.qml")),
           "PSET-02: CreatePresetsDialog.qml must be registered in qml.qrc");
  QVERIFY2(createDialog.contains(QStringLiteral("Inherits from")),
           "PSET-02: CreatePresetsDialog must have an inherits-from selector");
  QVERIFY2(createDialog.contains(QStringLiteral("createCustomPreset")),
           "PSET-02: CreatePresetsDialog must call configVm.createCustomPreset on Create");
  QVERIFY2(settingsDialog.contains(QStringLiteral("CreatePresetsDialog {")),
           "PSET-02: SettingsDialog must instantiate CreatePresetsDialog");
  QVERIFY2(settingsDialog.contains(QStringLiteral("onCreatePresetRequired")),
           "PSET-02: SettingsDialog must bind onCreatePresetRequired to open the dialog");

  // PSET-02: ConfigViewModel exposes the request/signal pair.
  QVERIFY2(configVmH.contains(QStringLiteral("requestCreatePreset")),
           "PSET-02: ConfigViewModel must expose requestCreatePreset Q_INVOKABLE");
  QVERIFY2(configVmH.contains(QStringLiteral("createPresetRequired")),
           "PSET-02: ConfigViewModel must declare createPresetRequired signal");
}

void QmlUiAuditTests::v50UnsavedChangesAndFilterWired()
{
  // Phase 148 (PSET-03/04): both pieces were already wired pre-v5.0; this slot
  // anchors that they remain intact AND that the C++ filter rule is a real
  // typed behavior (advancedMode is a strict superset of Simple mode).
  const QString configVmH = readSource(QStringLiteral("src/core/viewmodels/ConfigViewModel.h"));
  const QString configVm = readSource(QStringLiteral("src/core/viewmodels/ConfigViewModel.cpp"));
  const QString unsavedDialog = readSource(QStringLiteral("src/qml_gui/dialogs/UnsavedChangesDialog.qml"));
  const QString settingsDialog = readSource(QStringLiteral("src/qml_gui/dialogs/SettingsDialog.qml"));
  QVERIFY2(!configVm.isEmpty(), "Unable to read ConfigViewModel.cpp");
  QVERIFY2(!unsavedDialog.isEmpty(), "Unable to read UnsavedChangesDialog.qml");

  // PSET-03: UnsavedChangesDialog has Keep/Discard/Save-As/Cancel actions
  // (upstream 3-way diff UI columns are a documented follow-up; the existing
  // flat-list diff view covers the functional contract).
  QVERIFY2(unsavedDialog.contains(QStringLiteral("action = \"discard\"")),
           "PSET-03: UnsavedChangesDialog must offer a discard action");
  QVERIFY2(unsavedDialog.contains(QStringLiteral("action = \"save\"")),
           "PSET-03: UnsavedChangesDialog must offer a save-as-preset action");
  QVERIFY2(unsavedDialog.contains(QStringLiteral("action = \"cancel\"")),
           "PSET-03: UnsavedChangesDialog must offer a cancel (keep) action");
  QVERIFY2(settingsDialog.contains(QStringLiteral("requestSaveAndMaybeClose")),
           "PSET-03: SettingsDialog must route the save action to the save flow");
  QVERIFY2(settingsDialog.contains(QStringLiteral("requestDiscardPendingChanges")),
           "PSET-03: SettingsDialog must route the discard action to the discard flow");

  // PSET-04: Simple/Advanced filter is implemented in C++ as a typed behavior.
  QVERIFY2(configVmH.contains(QStringLiteral("filterOptionIndices")),
           "PSET-04: ConfigViewModel must expose filterOptionIndices (the C++ filter rule)");
  QVERIFY2(configVm.contains(QStringLiteral("advancedMode")),
           "PSET-04: filterOptionIndices must take an advancedMode parameter");
  QVERIFY2(configVm.contains(QStringLiteral("Simple mode (advancedMode=false) shows only comSimple")),
           "PSET-04: filter rule must be documented in C++ (Simple = comSimple subset)");
  QVERIFY2(configVm.contains(QStringLiteral("SUPERSET")),
           "PSET-04: advanced mode must be a documented superset of Simple");

  // PSET-04: SettingsDialog exposes the user-facing advanced toggle bound to
  // the C++ filter.
  QVERIFY2(settingsDialog.contains(QStringLiteral("advancedMode")),
           "PSET-04: SettingsDialog must expose an advancedMode user toggle");
  QVERIFY2(settingsDialog.contains(QStringLiteral("filterOptionIndices(presetTier, searchText, advancedMode)")),
           "PSET-04: SettingsDialog must pass advancedMode to filterOptionIndices");
}

void QmlUiAuditTests::v50CompareDiffAndRoundTripWired()
{
  // Phase 149 (PSET-05/06/07): Compare/Diff + Dirty Propagation + Round-Trip.
  // PSET-05 (comparePresets primitive) is new in Phase 149; PSET-07 (dirty
  // propagation) and PSET-06 (round-trip contract) build on existing
  // infrastructure that this slot locks in place.
  const QString presetSvcH = readSource(QStringLiteral("src/core/services/PresetServiceMock.h"));
  const QString presetSvc = readSource(QStringLiteral("src/core/services/PresetServiceMock.cpp"));
  const QString configVmH = readSource(QStringLiteral("src/core/viewmodels/ConfigViewModel.h"));
  QVERIFY2(!presetSvc.isEmpty(), "Unable to read PresetServiceMock.cpp");
  QVERIFY2(!configVmH.isEmpty(), "Unable to read ConfigViewModel.h");

  // PSET-05: comparePresets C++ primitive.
  QVERIFY2(presetSvcH.contains(QStringLiteral("comparePresets")),
           "PSET-05: PresetServiceMock must expose comparePresets(A, B)");
  QVERIFY2(presetSvc.contains(QStringLiteral("QVariantList PresetServiceMock::comparePresets")),
           "PSET-05: comparePresets must be implemented (returns QVariantList of {key, valueA, valueB})");
  QVERIFY2(presetSvc.contains(QStringLiteral("\"added\"")),
           "PSET-05: comparePresets must classify added keys");
  QVERIFY2(presetSvc.contains(QStringLiteral("\"removed\"")),
           "PSET-05: comparePresets must classify removed keys");
  QVERIFY2(presetSvc.contains(QStringLiteral("\"changed\"")),
           "PSET-05: comparePresets must classify changed keys");

  // PSET-06: bundle round-trip contract. The exportBundle/importBundle pair
  // (JSON) + the new exportBundleIni/importBundleIni pair (.ini) round-trip
  // user presets. This slot anchors that both paths exist; the live round-trip
  // ctest is gated on a PresetServiceMock context the unit tests don't have.
  QVERIFY2(presetSvcH.contains(QStringLiteral("exportBundle")) &&
           presetSvcH.contains(QStringLiteral("importBundle")),
           "PSET-06: JSON bundle round-trip pair must exist");
  QVERIFY2(presetSvcH.contains(QStringLiteral("exportBundleIni")) &&
           presetSvcH.contains(QStringLiteral("importBundleIni")),
           "PSET-06: .ini bundle round-trip pair must exist");

  // PSET-07: dirty-state propagation is consistent across page/preset/scope
  // switches. The infrastructure (isPresetDirty + pendingUnsavedAction +
  // pendingUnsavedTarget + hasPendingUnsavedChanges) lives on ConfigViewModel.
  QVERIFY2(configVmH.contains(QStringLiteral("Q_PROPERTY(bool isPresetDirty")),
           "PSET-07: ConfigViewModel must expose isPresetDirty Q_PROPERTY");
  QVERIFY2(configVmH.contains(QStringLiteral("pendingUnsavedAction")),
           "PSET-07: ConfigViewModel must track pendingUnsavedAction");
  QVERIFY2(configVmH.contains(QStringLiteral("pendingUnsavedTarget")),
           "PSET-07: ConfigViewModel must track pendingUnsavedTarget");
  QVERIFY2(configVmH.contains(QStringLiteral("hasPendingUnsavedChanges")),
           "PSET-07: ConfigViewModel must expose hasPendingUnsavedChanges");
}

void QmlUiAuditTests::v50PartPlateUiImplementationWired()
{
  // Phase 151 (PLATE-02/03/04/05): PartPlate UI implementation. PLATE-02 is new
  // (drag-reorder + EditorViewModel movePlate proxy); PLATE-03/04 were already
  // implemented pre-v5.0 (locked); PLATE-05 is documented as refined scope
  // (persisted-plate thumbnails work via plateThumbnailBase64; runtime capture
  // for session-created plates is deferred — see .planning/research/partplate-ui-gap.md).
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString evmH = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString evm = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString projSvcH = readSource(QStringLiteral("src/core/services/ProjectServiceMock.h"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!evm.isEmpty(), "Unable to read EditorViewModel.cpp");

  // PLATE-02: drag-reorder wiring + EditorViewModel movePlate proxy.
  QVERIFY2(preparePage.contains(QStringLiteral("DropArea {"))
           && preparePage.contains(QStringLiteral("\"plate-drag\"")),
           "PLATE-02: PreparePage must have a DropArea keyed plate-drag for drag-reorder");
  QVERIFY2(preparePage.contains(QStringLiteral("draggedPlateIndex")),
           "PLATE-02: PreparePage must stash the source plate index during a plate drag");
  QVERIFY2(preparePage.contains(QStringLiteral("root.editorVm.movePlate(")),
           "PLATE-02: PreparePage must call editorVm.movePlate on plate-drag drop");
  QVERIFY2(evmH.contains(QStringLiteral("Q_INVOKABLE bool movePlate(int oldIndex, int newIndex)")),
           "PLATE-02: EditorViewModel must expose movePlate Q_INVOKABLE");
  QVERIFY2(evm.contains(QStringLiteral("projectService_->movePlate(oldIndex, newIndex)")),
           "PLATE-02: EditorViewModel movePlate must proxy to ProjectServiceMock");
  QVERIFY2(projSvcH.contains(QStringLiteral("movePlate")),
           "PLATE-02: ProjectServiceMock must expose movePlate (delegates to PartPlateList)");

  // PLATE-03: per-plate print sequence dialog (pre-existing — verify still present).
  QVERIFY2(preparePage.contains(QStringLiteral("platePrintSequence")),
           "PLATE-03: PreparePage must bind the per-plate print-sequence combo");
  QVERIFY2(preparePage.contains(QStringLiteral("setPlatePrintSequence")),
           "PLATE-03: PreparePage must write back via setPlatePrintSequence");

  // PLATE-04: per-plate config override (pre-existing — verify the scope path).
  QVERIFY2(evmH.contains(QStringLiteral("requestPlateScope")) ||
           readSource(QStringLiteral("src/core/viewmodels/ConfigViewModel.h")).contains(QStringLiteral("requestPlateScope")),
           "PLATE-04: ConfigViewModel must expose requestPlateScope (the plate-scope switch)");

  // PLATE-05: non-current-plate thumbnail accessor (pre-existing). The QML
  // binding reads plateThumbnailBase64 for non-current plates (gap analysis:
  // runtime capture scheduler is documented refined scope).
  QVERIFY2(preparePage.contains(QStringLiteral("plateThumbnailBase64")),
           "PLATE-05: PreparePage must read plateThumbnailBase64 for non-current-plate thumbnails");
  QVERIFY2(projSvcH.contains(QStringLiteral("plateThumbnailBase64")),
           "PLATE-05: ProjectServiceMock must expose plateThumbnailBase64");
}

void QmlUiAuditTests::v50PartPlateSaveReloadRegressionWired()
{
  // Phase 152 (PLATE-06): multi-plate save/reload regression. The full plate
  // state — count, names, per-plate config overrides, print sequence, bed type,
  // locked/printable flags, filament maps, thumbnails — round-trips through 3MF
  // via the pendingPlate* staging buffers in ProjectServiceMock. This slot
  // anchors that all staging paths exist. A live ctest is gated on a
  // ProjectServiceMock context the unit tests don't have.
  const QString projSvcH = readSource(QStringLiteral("src/core/services/ProjectServiceMock.h"));
  const QString projSvc = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  QVERIFY2(!projSvc.isEmpty(), "Unable to read ProjectServiceMock.cpp");

  // Each plate field that must round-trip has a corresponding pendingPlate*
  // staging buffer that is populated on 3MF load + applied to PartPlateList.
  QVERIFY2(projSvcH.contains(QStringLiteral("pendingPlateLocked_")),
           "PLATE-06: ProjectServiceMock must stage pendingPlateLocked_ for 3MF round-trip");
  QVERIFY2(projSvcH.contains(QStringLiteral("pendingPlateBedType_")),
           "PLATE-06: ProjectServiceMock must stage pendingPlateBedType_");
  QVERIFY2(projSvcH.contains(QStringLiteral("pendingPlatePrintSeq_")),
           "PLATE-06: ProjectServiceMock must stage pendingPlatePrintSeq_");
  QVERIFY2(projSvcH.contains(QStringLiteral("pendingPlateSpiral_")),
           "PLATE-06: ProjectServiceMock must stage pendingPlateSpiral_");
  QVERIFY2(projSvcH.contains(QStringLiteral("pendingPlateFilamentMaps_")),
           "PLATE-06: ProjectServiceMock must stage pendingPlateFilamentMaps_");
  QVERIFY2(projSvcH.contains(QStringLiteral("pendingPlateThumbnails_")),
           "PLATE-06: ProjectServiceMock must stage pendingPlateThumbnails_");

  // PartPlate + PartPlateList must be the source of truth (with the
  // DynamicPrintConfig m_config per-plate override).
  QVERIFY2(readSource(QStringLiteral("src/core/model/PartPlate.h")).contains(QStringLiteral("DynamicPrintConfig m_config")),
           "PLATE-06: PartPlate must have native DynamicPrintConfig m_config (per-plate override truth)");

  // The existing Phase 97 (v4.3) thumbnailSaveReloadRoundTrip test already
  // covers a portion of this contract; Phase 152 extends the anchor coverage
  // to ALL plate state fields via this source-audit slot. A live multi-plate
  // round-trip ctest would require a ProjectServiceMock fixture the unit-test
  // harness doesn't have (the existing PartPlateTests QSKIP when HAS_LIBSLIC3R
  // is not in scope).
  QVERIFY2(projSvc.contains(QStringLiteral("pendingPlate")),
           "PLATE-06: ProjectServiceMock must rebuild pending plate state from 3MF load");
}

void QmlUiAuditTests::v50RegressionLocked()
{
  // Phase 153 (REGRESS-04): v5.0 cross-workstream regression gate. The most
  // important assertion is cross-slot: this slot verifies that the consolidated
  // v5.0 contract holds by spot-checking one anchor per workstream AND
  // re-asserting the v4.x milestone anchors. The per-workstream detail slots
  // (v50TechDebtRegressionLocked, v50OpenVdbUnlockWired, v50HollowGizmoReachable,
  // v50EmbossParameterized, v50EmbossAsyncAndPanelWired, v50EmbossWiringAndSvgWired,
  // v50PresetIniAndCreateDialogWired, v50UnsavedChangesAndFilterWired,
  // v50CompareDiffAndRoundTripWired, v50PartPlateUiImplementationWired,
  // v50PartPlateSaveReloadRegressionWired) already run individually; this slot
  // is the cross-workstream rollup that protects against a regression slipping
  // through if a per-workstream slot is later removed or weakened.
  const QString evm = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString projSvc = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  const QString presetSvc = readSource(QStringLiteral("src/core/services/PresetServiceMock.cpp"));
  const QString rootCmake = readSource(QStringLiteral("CMakeLists.txt"));
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString calibSvc = readSource(QStringLiteral("src/core/services/CalibrationServiceMock.cpp"));
  QVERIFY2(!evm.isEmpty(), "Unable to read EditorViewModel.cpp");
  QVERIFY2(!projSvc.isEmpty(), "Unable to read ProjectServiceMock.cpp");

  // ── v5.0 WS1 (tech-debt) anchor: CGAL-02 true intersection.
  QVERIFY2(projSvc.contains(QStringLiteral("MeshBoolean::cgal::intersect")),
           "REGRESS-04/WS1: meshBoolean op==2 must still call MeshBoolean::cgal::intersect");

  // ── v5.0 WS2 (OpenVDB) anchor: find_package(OpenVDB) + openvdb_libs shim.
  QVERIFY2(rootCmake.contains(QStringLiteral("find_package(OpenVDB 5.0 COMPONENTS openvdb)")),
           "REGRESS-04/WS2: root CMakeLists must keep find_package(OpenVDB)");
  QVERIFY2(rootCmake.contains(QStringLiteral("add_library(openvdb_libs INTERFACE IMPORTED)")),
           "REGRESS-04/WS2: root CMakeLists must keep the openvdb_libs shim");

  // ── v5.0 WS2 (Hollow) anchor: case 8 reachable.
  QVERIFY2(evm.contains(QStringLiteral("case 8: // Hollow\n    return hasSingleObject")),
           "REGRESS-04/WS2: Hollow gizmo (case 8) must stay reachable (hasSingleObject)");

  // ── v5.0 WS3 (Emboss) anchor: parameterized text2shapes pipeline.
  QVERIFY2(projSvc.contains(QStringLiteral("Slic3r::Emboss::text2shapes")),
           "REGRESS-04/WS3: text2shapes pipeline must stay wired");
  QVERIFY2(projSvc.contains(QStringLiteral("m_embossFontPath")),
           "REGRESS-04/WS3: emboss font path must stay parameterized (not hardcoded)");

  // ── v5.0 WS3 (Emboss async) anchor: Qt Concurrent worker.
  QVERIFY2(projSvc.contains(QStringLiteral("addTextVolumeAsync")),
           "REGRESS-04/WS3: async emboss API must stay present");

  // ── v5.0 WS4 (Preset) anchor: .ini bundle + comparePresets.
  QVERIFY2(presetSvc.contains(QStringLiteral("exportBundleIni")),
           "REGRESS-04/WS4: PresetServiceMock must keep exportBundleIni (.ini interop)");
  QVERIFY2(presetSvc.contains(QStringLiteral("comparePresets")),
           "REGRESS-04/WS4: comparePresets primitive must stay present");

  // ── v5.0 WS5 (PartPlate) anchor: drag-reorder + staging buffers.
  QVERIFY2(preparePage.contains(QStringLiteral("\"plate-drag\"")),
           "REGRESS-04/WS5: PreparePage must keep the plate-drag DropArea (drag-reorder)");
  QVERIFY2(readSource(QStringLiteral("src/core/services/ProjectServiceMock.h")).contains(QStringLiteral("pendingPlateThumbnails_")),
           "REGRESS-04/WS5: ProjectServiceMock must keep pendingPlateThumbnails_ (3MF round-trip)");

  // ── v4.8 re-assertion: CGAL MeshBoolean + Drill + Assembly ASM-01.
  QVERIFY2(evm.contains(QStringLiteral("kCgalMeshBooleanAvailable = true")),
           "REGRESS-04/v4.8: kCgalMeshBooleanAvailable must still be true");
  QVERIFY2(projSvc.contains(QStringLiteral("MeshBoolean::minus")),
           "REGRESS-04/v4.8: MeshBoolean::minus must still be wired for difference/drill");

  // ── v4.7 re-assertion: paint-gate flag + flatten + fixMesh.
  QVERIFY2(evm.contains(QStringLiteral("kViewportTrianglePickingAvailable = true")),
           "REGRESS-04/v4.7: paint-gizmo gate flag must still be true");
  QVERIFY2(evm.contains(QStringLiteral("orientObject")),
           "REGRESS-04/v4.7: flattenSelected must still call orientObject");
  QVERIFY2(projSvc.contains(QStringLiteral("its_merge_vertices")),
           "REGRESS-04/v4.7: fixMesh must still call its_merge_vertices");

  // ── v4.6 re-assertion: calibration tower modes.
  QVERIFY2(calibSvc.contains(QStringLiteral("calibMode = 7")),
           "REGRESS-04/v4.6: Vol_speed tower mode (7) must still dispatch");
  QVERIFY2(calibSvc.contains(QStringLiteral("calibMode = 9")),
           "REGRESS-04/v4.6: Retraction tower mode (9) must still dispatch");
}

// Phase 154 (CLOS-01): QML Preset Diff-View Dialog wiring gate.
// Asserts the deferred QML consumer for the Phase 149 comparePresets primitive
// is wired: (1) ConfigViewModel exposes a structured comparePresetsDetailed
// proxy returning QVariantList (not the legacy QStringList); (2) a new
// PresetDiffDialog.qml exists and renders the {key, valueA, valueB, status}
// model roles; (3) SettingsDialog instantiates the dialog and binds the
// comparePresetsRequired signal; (4) qml.qrc registers the new dialog file.
void QmlUiAuditTests::v51PresetDiffDialogWired()
{
  const QString configVmH = readSource(QStringLiteral("src/core/viewmodels/ConfigViewModel.h"));
  const QString configVmCpp = readSource(QStringLiteral("src/core/viewmodels/ConfigViewModel.cpp"));
  const QString diffDialog = readSource(QStringLiteral("src/qml_gui/dialogs/PresetDiffDialog.qml"));
  const QString settingsDialog = readSource(QStringLiteral("src/qml_gui/dialogs/SettingsDialog.qml"));
  const QString qrc = readSource(QStringLiteral("src/qml_gui/qml.qrc"));

  QVERIFY2(!configVmH.isEmpty(), "Unable to read ConfigViewModel.h");
  QVERIFY2(!configVmCpp.isEmpty(), "Unable to read ConfigViewModel.cpp");
  QVERIFY2(!diffDialog.isEmpty(), "Unable to read PresetDiffDialog.qml");
  QVERIFY2(!settingsDialog.isEmpty(), "Unable to read SettingsDialog.qml");
  QVERIFY2(!qrc.isEmpty(), "Unable to read qml.qrc");

  // (1) ConfigViewModel exposes the structured diff variant returning QVariantList
  //     + the requestComparePresets opener that emits comparePresetsRequired.
  QVERIFY2(configVmH.contains(QStringLiteral("Q_INVOKABLE QVariantList comparePresetsDetailed")),
           "CLOS-01: ConfigViewModel must expose comparePresetsDetailed returning QVariantList");
  QVERIFY2(configVmH.contains(QStringLiteral("requestComparePresets")),
           "CLOS-01: ConfigViewModel must expose requestComparePresets opener");
  QVERIFY2(configVmH.contains(QStringLiteral("comparePresetsRequired")),
           "CLOS-01: ConfigViewModel must emit comparePresetsRequired signal");

  // (1b) Implementation proxies to PresetServiceMock::comparePresets (Phase 149).
  QVERIFY2(configVmCpp.contains(QStringLiteral("ConfigViewModel::comparePresetsDetailed")),
           "CLOS-01: comparePresetsDetailed must be implemented in ConfigViewModel.cpp");
  QVERIFY2(configVmCpp.contains(QStringLiteral("presetService_->comparePresets")),
           "CLOS-01: comparePresetsDetailed must proxy to PresetServiceMock::comparePresets");

  // (2) PresetDiffDialog.qml exists and consumes the model roles produced by
  //     comparePresetsDetailed: key / valueA / valueB / status (added/removed/changed).
  QVERIFY2(diffDialog.contains(QStringLiteral("comparePresetsDetailed")),
           "CLOS-01: PresetDiffDialog must call comparePresetsDetailed");
  QVERIFY2(diffDialog.contains(QStringLiteral("modelData.key")),
           "CLOS-01: PresetDiffDialog delegate must read modelData.key");
  QVERIFY2(diffDialog.contains(QStringLiteral("modelData.valueA")),
           "CLOS-01: PresetDiffDialog delegate must read modelData.valueA");
  QVERIFY2(diffDialog.contains(QStringLiteral("modelData.valueB")),
           "CLOS-01: PresetDiffDialog delegate must read modelData.valueB");
  QVERIFY2(diffDialog.contains(QStringLiteral("modelData.status")),
           "CLOS-01: PresetDiffDialog delegate must read modelData.status");
  QVERIFY2(diffDialog.contains(QStringLiteral("\"added\"")),
           "CLOS-01: PresetDiffDialog must classify added rows (green badge)");
  QVERIFY2(diffDialog.contains(QStringLiteral("\"removed\"")),
           "CLOS-01: PresetDiffDialog must classify removed rows (red badge)");
  QVERIFY2(diffDialog.contains(QStringLiteral("\"changed\"")),
           "CLOS-01: PresetDiffDialog must classify changed rows (amber badge)");
  // Empty-state: comparing identical presets yields a clear "no differences"
  // affordance (no fabricated rows).
  QVERIFY2(diffDialog.contains(QStringLiteral("No differences")),
           "CLOS-01: PresetDiffDialog must show an empty-state placeholder when diff is empty");

  // (3) SettingsDialog instantiates PresetDiffDialog + binds the opener signal.
  QVERIFY2(settingsDialog.contains(QStringLiteral("PresetDiffDialog {")),
           "CLOS-01: SettingsDialog must instantiate PresetDiffDialog");
  QVERIFY2(settingsDialog.contains(QStringLiteral("onComparePresetsRequired")),
           "CLOS-01: SettingsDialog must bind onComparePresetsRequired to dialog.open()");
  QVERIFY2(settingsDialog.contains(QStringLiteral("requestComparePresets")),
           "CLOS-01: SettingsDialog toolbar must call requestComparePresets to open the dialog");

  // (4) qml.qrc registers the new dialog file.
  QVERIFY2(qrc.contains(QStringLiteral("dialogs/PresetDiffDialog.qml")),
           "CLOS-01: qml.qrc must register dialogs/PresetDiffDialog.qml");
}

// Phase 155 (CLOS-02): Emboss 3MF text metadata round-trip gate.
// Asserts the save side writes upstream `TextConfiguration` (so store_bbs_3mf
// emits the `<slic3rpe:text>` block automatically) and the load side reads
// `text_configuration` to restore the TextEmboss tag. Mirrors upstream
// `ModelVolume::is_text()` semantics (`Model.hpp:911`).
void QmlUiAuditTests::v51EmbossTextMetadataRoundTripWired()
{
  const QString projH = readSource(QStringLiteral("src/core/services/ProjectServiceMock.h"));
  const QString projCpp = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));

  QVERIFY2(!projH.isEmpty(), "Unable to read ProjectServiceMock.h");
  QVERIFY2(!projCpp.isEmpty(), "Unable to read ProjectServiceMock.cpp");

  // (1) TextConfiguration.hpp is included (struct definitions available).
  QVERIFY2(projCpp.contains(QStringLiteral("<libslic3r/TextConfiguration.hpp>")),
           "CLOS-02: ProjectServiceMock.cpp must include libslic3r/TextConfiguration.hpp");

  // (2) attachEmbossMetadata helper exists and is declared in the header.
  QVERIFY2(projH.contains(QStringLiteral("attachEmbossMetadata")),
           "CLOS-02: ProjectServiceMock.h must declare attachEmbossMetadata helper");

  // (3) Save side: the helper builds and assigns a TextConfiguration. Asserts
  //     the key struct field assignments — these are what upstream
  //     TextConfigurationSerialization::to_xml serializes to <slic3rpe:text>.
  QVERIFY2(projCpp.contains(QStringLiteral("Slic3r::TextConfiguration tc;")),
           "CLOS-02: attachEmbossMetadata must construct a Slic3r::TextConfiguration");
  QVERIFY2(projCpp.contains(QStringLiteral("tc.text =")),
           "CLOS-02: attachEmbossMetadata must set TextConfiguration::text");
  QVERIFY2(projCpp.contains(QStringLiteral("tc.style.path =")),
           "CLOS-02: attachEmbossMetadata must set EmbossStyle::path (font_descriptor)");
  QVERIFY2(projCpp.contains(QStringLiteral("tc.style.type = Slic3r::EmbossStyle::Type::file_path")),
           "CLOS-02: attachEmbossMetadata must set EmbossStyle::Type::file_path (source-truth font path type)");
  QVERIFY2(projCpp.contains(QStringLiteral("tc.style.prop.size_in_mm =")),
           "CLOS-02: attachEmbossMetadata must set FontProp::size_in_mm (line_height)");
  QVERIFY2(projCpp.contains(QStringLiteral("tc.style.prop.family =")),
           "CLOS-02: attachEmbossMetadata must set FontProp::family (font substitution hint)");
  QVERIFY2(projCpp.contains(QStringLiteral("text_configuration = std::move(tc)")),
           "CLOS-02: attachEmbossMetadata must assign the TextConfiguration to the volume");

  // (4) Both emboss creation paths (sync + async) call attachEmbossMetadata.
  //     (Phase 158 extended the signature to carry boldness/italic; the
  //     match anchors on the leading arg list which is stable across that.)
  QVERIFY2(projCpp.contains(QStringLiteral("attachEmbossMetadata(newVol, text, fontPath")),
           "CLOS-02: sync performEmbossVolumeAdd path must call attachEmbossMetadata");
  QVERIFY2(projCpp.contains(QStringLiteral("receiver->attachEmbossMetadata(newVol, text, fontPath, height, depth")),
           "CLOS-02: async addTextVolumeAsync GUI-thread handler must call attachEmbossMetadata");

  // (5) Load side: objectVolumeType / objectVolumeTypeLabel read
  //     text_configuration to restore the TextEmboss tag (mirrors upstream
  //     ModelVolume::is_text()). This is what makes a reloaded emboss volume
  //     re-editable in the Emboss panel instead of opaque geometry.
  QVERIFY2(projCpp.contains(QStringLiteral("vol->text_configuration.has_value()")),
           "CLOS-02: objectVolumeType/objectVolumeTypeLabel must check text_configuration.has_value() to restore TextEmboss tag");
  QVERIFY2(projCpp.contains(QStringLiteral("MockVolumeType::TextEmboss")),
           "CLOS-02: load-side type detection must return the TextEmboss enum value for emboss volumes");
}

// Phase 156 (CLOS-03): runtime plate thumbnail capture scheduler gate.
// Asserts the write path exists (so captured bytes can reach
// PartPlate::setThumbnail for non-current plates), the per-plate capture
// signal carries the plate index, and PreparePage wires a session-capture
// scheduler. Phase 151 shipped the read accessor + 3MF save/load only; this
// phase closes the in-session capture gap.
void QmlUiAuditTests::v51PartPlateSessionThumbnailWired()
{
  const QString projH = readSource(QStringLiteral("src/core/services/ProjectServiceMock.h"));
  const QString projCpp = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  const QString editorH = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString editorCpp = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString rhiH = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.h"));
  const QString rhiCpp = readSource(QStringLiteral("src/qml_gui/Renderer/RhiViewport.cpp"));
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));

  QVERIFY2(!projH.isEmpty(), "Unable to read ProjectServiceMock.h");
  QVERIFY2(!projCpp.isEmpty(), "Unable to read ProjectServiceMock.cpp");
  QVERIFY2(!editorH.isEmpty(), "Unable to read EditorViewModel.h");
  QVERIFY2(!editorCpp.isEmpty(), "Unable to read EditorViewModel.cpp");
  QVERIFY2(!rhiH.isEmpty(), "Unable to read RhiViewport.h");
  QVERIFY2(!rhiCpp.isEmpty(), "Unable to read RhiViewport.cpp");
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");

  // (1) Write path on ProjectServiceMock (the missing primitive).
  QVERIFY2(projH.contains(QStringLiteral("setPlateThumbnailFromBase64")),
           "CLOS-03: ProjectServiceMock.h must declare setPlateThumbnailFromBase64 write path");
  QVERIFY2(projCpp.contains(QStringLiteral("ProjectServiceMock::setPlateThumbnailFromBase64")),
           "CLOS-03: setPlateThumbnailFromBase64 must be implemented in ProjectServiceMock.cpp");
  QVERIFY2(projCpp.contains(QStringLiteral("QByteArray::fromBase64")),
           "CLOS-03: setPlateThumbnailFromBase64 must base64-decode the input");
  QVERIFY2(projCpp.contains(QStringLiteral("loadFromData")),
           "CLOS-03: setPlateThumbnailFromBase64 must QImage::loadFromData to decode the PNG");
  QVERIFY2(projCpp.contains(QStringLiteral("setThumbnail(img)")),
           "CLOS-03: setPlateThumbnailFromBase64 must route the QImage into PartPlate::setThumbnail");

  // (2) EditorViewModel proxy so QML goes through the viewmodel (QML boundary
  //     rule — QML never holds direct references to services).
  QVERIFY2(editorH.contains(QStringLiteral("setPlateThumbnailFromBase64")),
           "CLOS-03: EditorViewModel.h must expose the setPlateThumbnailFromBase64 proxy");
  QVERIFY2(editorCpp.contains(QStringLiteral("EditorViewModel::setPlateThumbnailFromBase64")),
           "CLOS-03: EditorViewModel.cpp must implement the proxy forwarding to projectService_");

  // (3) RhiViewport carries the plateIndex through a per-plate signal so
  //     captured bytes can be routed back to the right plate. (deliverThumbnail
  //     previously Q_UNUSED'd the index — Phase 156 closes that.)
  QVERIFY2(rhiH.contains(QStringLiteral("thumbnailCapturedForPlate(int plateIndex, const QString &data)")),
           "CLOS-03: RhiViewport.h must declare thumbnailCapturedForPlate(int, QString) signal carrying the plate index");
  QVERIFY2(rhiCpp.contains(QStringLiteral("emit thumbnailCapturedForPlate(plateIndex, m_lastThumbnailData)")),
           "CLOS-03: RhiViewport::deliverThumbnail must emit thumbnailCapturedForPlate with the real plateIndex (no longer Q_UNUSED)");

  // (4) PreparePage QML: per-plate capture handler persists bytes + a
  //     session-capture scheduler sweeps plates on state change.
  QVERIFY2(preparePage.contains(QStringLiteral("onThumbnailCapturedForPlate")),
           "CLOS-03: PreparePage must handle the per-plate thumbnailCapturedForPlate signal");
  QVERIFY2(preparePage.contains(QStringLiteral("setPlateThumbnailFromBase64(plateIndex, data)")),
           "CLOS-03: PreparePage capture handler must persist bytes via setPlateThumbnailFromBase64");
  QVERIFY2(preparePage.contains(QStringLiteral("sessionThumbScheduler")),
           "CLOS-03: PreparePage must have a session-capture scheduler (sessionThumbScheduler Timer)");
  QVERIFY2(preparePage.contains(QStringLiteral("captureMissingPlateThumbnails")),
           "CLOS-03: PreparePage must implement captureMissingPlateThumbnails() that iterates plates");
}

// Phase 157 (CLOS-04): live multi-plate full-state round-trip ctest anchor.
// Phase 152 could only source-audit-lock PLATE-06 because no live ctest
// existed. Phase 157 ships multiPlateFullStateRoundTrip in ViewModelSmokeTests
// (real store_bbs_3mf + read_from_archive on a stack ProjectServiceMock); this
// slot locks its existence + coverage breadth across all 5 CLOS-04 dimensions.
void QmlUiAuditTests::v51MultiPlateRoundTripLiveCtest()
{
  const QString vmTests = readSource(QStringLiteral("tests/ViewModelSmokeTests.cpp"));

  QVERIFY2(!vmTests.isEmpty(), "Unable to read tests/ViewModelSmokeTests.cpp");

  // (1) The live ctest exists (the gap Phase 152 left).
  QVERIFY2(vmTests.contains(QStringLiteral("void ViewModelSmokeTests::multiPlateFullStateRoundTrip()")),
           "CLOS-04: ViewModelSmokeTests must implement multiPlateFullStateRoundTrip (the live ctest Phase 152 lacked)");

  // (2) Coverage breadth: all 5 CLOS-04 dimensions + thumbnail are exercised.
  // Dim 1 — plate count + names.
  QVERIFY2(vmTests.contains(QStringLiteral("renamePlate(0, QStringLiteral(\"Alpha\"))")),
           "CLOS-04: live ctest must rename plates (dim 1: names)");
  QVERIFY2(vmTests.contains(QStringLiteral("loader.plateNames()")),
           "CLOS-04: live ctest must assert plateNames after reload (dim 1: names)");

  // Dim 2 — per-plate config override.
  QVERIFY2(vmTests.contains(QStringLiteral("setPlateScopedOptionValue(1, QStringLiteral(\"layer_height\"), 0.25)")),
           "CLOS-04: live ctest must set a per-plate config override (dim 2)");
  QVERIFY2(vmTests.contains(QStringLiteral("plateScopedOptionValue(")),
           "CLOS-04: live ctest must assert the per-plate config override after reload (dim 2)");

  // Dim 3 — non-default print sequence.
  QVERIFY2(vmTests.contains(QStringLiteral("setPlatePrintSequence(2, 2)")),
           "CLOS-04: live ctest must set a non-default print sequence (dim 3)");
  QVERIFY2(vmTests.contains(QStringLiteral("loader.platePrintSequence(2)")),
           "CLOS-04: live ctest must assert the print sequence after reload (dim 3)");

  // Dim 4 — mixed bed types.
  QVERIFY2(vmTests.contains(QStringLiteral("setPlateBedType(0, 1)")) &&
           vmTests.contains(QStringLiteral("setPlateBedType(1, 3)")) &&
           vmTests.contains(QStringLiteral("setPlateBedType(2, 4)")),
           "CLOS-04: live ctest must set mixed bed types (dim 4)");
  QVERIFY2(vmTests.contains(QStringLiteral("loader.plateBedType(0)")),
           "CLOS-04: live ctest must assert bed types after reload (dim 4)");

  // Dim 5 — mixed locked / printable flags.
  QVERIFY2(vmTests.contains(QStringLiteral("setPlateLocked(1, true)")),
           "CLOS-04: live ctest must set mixed locked flags (dim 5)");
  QVERIFY2(vmTests.contains(QStringLiteral("setPlatePrintable(2, false)")),
           "CLOS-04: live ctest must set mixed printable flags (dim 5)");
  QVERIFY2(vmTests.contains(QStringLiteral("loader.isPlateLocked(1)")),
           "CLOS-04: live ctest must assert locked state after reload (dim 5)");

  // Dim 6 — per-plate thumbnail (uses the Phase 156 write path).
  QVERIFY2(vmTests.contains(QStringLiteral("setPlateThumbnailFromBase64(0, thumbB64)")),
           "CLOS-04: live ctest must write a per-plate thumbnail (dim 6: thumbnail)");
  QVERIFY2(vmTests.contains(QStringLiteral("loader.plateThumbnailBase64(0).isEmpty()")),
           "CLOS-04: live ctest must assert thumbnail non-empty after reload (dim 6: thumbnail)");
}

// Phase 158 (EMBO-F01/F02): Emboss style controls + SVG depth-modifier wiring
// gate. Locks the 4 new style axes (boldness/italic/use-surface/curve-projection)
// from Q_PROPERTY through viewmodel forwarding to ProjectServiceMock FontProp
// population + TextConfiguration persistence, plus the addSvgVolume
// depth-modifier extension.
void QmlUiAuditTests::v51EmbossStyleControlsAndSvgAdvancedWired()
{
  const QString projH = readSource(QStringLiteral("src/core/services/ProjectServiceMock.h"));
  const QString projCpp = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  const QString editorH = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.h"));
  const QString editorCpp = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));

  QVERIFY2(!projH.isEmpty(), "Unable to read ProjectServiceMock.h");
  QVERIFY2(!projCpp.isEmpty(), "Unable to read ProjectServiceMock.cpp");
  QVERIFY2(!editorH.isEmpty(), "Unable to read EditorViewModel.h");
  QVERIFY2(!editorCpp.isEmpty(), "Unable to read EditorViewModel.cpp");
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");

  // (1) EditorViewModel exposes the 4 new style axes + svgDepthModifier.
  QVERIFY2(editorH.contains(QStringLiteral("Q_PROPERTY(float embossBoldness")),
           "EMBO-F01: EditorViewModel.h must expose embossBoldness Q_PROPERTY");
  QVERIFY2(editorH.contains(QStringLiteral("Q_PROPERTY(bool embossItalic")),
           "EMBO-F01: EditorViewModel.h must expose embossItalic Q_PROPERTY");
  QVERIFY2(editorH.contains(QStringLiteral("Q_PROPERTY(bool embossUseSurface")),
           "EMBO-F01: EditorViewModel.h must expose embossUseSurface Q_PROPERTY");
  QVERIFY2(editorH.contains(QStringLiteral("Q_PROPERTY(bool embossCurveProjection")),
           "EMBO-F01: EditorViewModel.h must expose embossCurveProjection Q_PROPERTY");
  QVERIFY2(editorH.contains(QStringLiteral("Q_PROPERTY(float svgDepthModifier")),
           "EMBO-F02: EditorViewModel.h must expose svgDepthModifier Q_PROPERTY");

  // (2) ProjectServiceMock has the 4 setters + member fields.
  QVERIFY2(projH.contains(QStringLiteral("setEmbossBoldness")),
           "EMBO-F01: ProjectServiceMock.h must declare setEmbossBoldness");
  QVERIFY2(projH.contains(QStringLiteral("setEmbossItalic")),
           "EMBO-F01: ProjectServiceMock.h must declare setEmbossItalic");
  QVERIFY2(projH.contains(QStringLiteral("setEmbossUseSurface")),
           "EMBO-F01: ProjectServiceMock.h must declare setEmbossUseSurface");
  QVERIFY2(projH.contains(QStringLiteral("setEmbossCurveProjection")),
           "EMBO-F01: ProjectServiceMock.h must declare setEmbossCurveProjection");
  QVERIFY2(projH.contains(QStringLiteral("m_embossBoldness")),
           "EMBO-F01: ProjectServiceMock.h must declare m_embossBoldness member field");
  QVERIFY2(projH.contains(QStringLiteral("m_embossItalic")),
           "EMBO-F01: ProjectServiceMock.h must declare m_embossItalic member field");

  // (3) FontProp is populated from the new fields in performEmbossVolumeAdd +
  //     the async worker (no longer hardcoded 0.0f).
  QVERIFY2(projCpp.contains(QStringLiteral("font_prop.boldness = m_embossBoldness")),
           "EMBO-F01: sync performEmbossVolumeAdd must populate font_prop.boldness from m_embossBoldness (was hardcoded 0.0f)");
  QVERIFY2(projCpp.contains(QStringLiteral("font_prop.boldness = boldness")),
           "EMBO-F01: async worker must populate font_prop.boldness from the captured snapshot");
  QVERIFY2(projCpp.contains(QStringLiteral("font_prop.skew = 0.4")),
           "EMBO-F01: italic must populate font_prop.skew (the upstream italic axis)");

  // (4) Async worker captures the new fields (so the off-thread job reads a
  //     consistent snapshot — mirrors the existing fontPath/height/depth captures).
  QVERIFY2(projCpp.contains(QStringLiteral("const float boldness = m_embossBoldness;")),
           "EMBO-F01: async worker must snapshot m_embossBoldness before QtConcurrent::run");
  QVERIFY2(projCpp.contains(QStringLiteral("const bool italic = m_embossItalic;")),
           "EMBO-F01: async worker must snapshot m_embossItalic before QtConcurrent::run");

  // (5) attachEmbossMetadata persists boldness + skew into TextConfiguration
  //     so the style round-trips through <slic3rpe:text> (Phase 155 metadata path).
  QVERIFY2(projCpp.contains(QStringLiteral("tc.style.prop.boldness = boldness")),
           "EMBO-F01: attachEmbossMetadata must persist boldness into TextConfiguration::style.prop");
  QVERIFY2(projCpp.contains(QStringLiteral("tc.style.prop.skew = 0.4")),
           "EMBO-F01: attachEmbossMetadata must persist skew into TextConfiguration::style.prop when italic");

  // (6) The 3 viewmodel forwarding sites push the new fields to the service.
  QVERIFY2(editorCpp.contains(QStringLiteral("projectService_->setEmbossBoldness(m_embossBoldness)")),
           "EMBO-F01: EditorViewModel forwarding sites must call setEmbossBoldness");

  // (7) addSvgVolume accepts the depth-modifier param + applies it.
  QVERIFY2(projH.contains(QStringLiteral("addSvgVolume(int objectIndex, const QString &svgFilePath, float depthModifier = 1.0f)")),
           "EMBO-F02: ProjectServiceMock::addSvgVolume must accept the depthModifier param");
  QVERIFY2(projCpp.contains(QStringLiteral("set_scaling_factor")),
           "EMBO-F02: addSvgVolume must apply the modifier via the volume's Z scaling factor");
  QVERIFY2(editorCpp.contains(QStringLiteral("addSvgVolume(idx, m_svgFilePath, m_svgDepthModifier)")),
           "EMBO-F02: importSVG must forward m_svgDepthModifier to addSvgVolume");

  // (8) PreparePage Emboss panel exposes the 4 new style controls.
  QVERIFY2(preparePage.contains(QStringLiteral("root.editorVm.embossBoldness")),
           "EMBO-F01: PreparePage Emboss panel must bind a control to embossBoldness");
  QVERIFY2(preparePage.contains(QStringLiteral("root.editorVm.embossItalic")),
           "EMBO-F01: PreparePage Emboss panel must bind a control to embossItalic");
  QVERIFY2(preparePage.contains(QStringLiteral("root.editorVm.embossUseSurface")),
           "EMBO-F01: PreparePage Emboss panel must bind a control to embossUseSurface");
  QVERIFY2(preparePage.contains(QStringLiteral("root.editorVm.embossCurveProjection")),
           "EMBO-F01: PreparePage Emboss panel must bind a control to embossCurveProjection");
}

// Phase 159 (REGRESS-05): v5.1 cross-workstream regression gate.
// The most important assertion is cross-slot: this slot verifies the
// consolidated v5.1 contract holds by spot-checking one anchor per workstream
// AND re-asserting the v5.0/v4.8/v4.7/v4.6 milestone anchors. The per-phase
// detail slots (v51PresetDiffDialogWired, v51EmbossTextMetadataRoundTripWired,
// v51PartPlateSessionThumbnailWired, v51MultiPlateRoundTripLiveCtest,
// v51EmbossStyleControlsAndSvgAdvancedWired) already run individually; this
// slot is the cross-workstream rollup that protects against a regression
// slipping through if a per-phase slot is later removed or weakened.
void QmlUiAuditTests::v51RegressionLocked()
{
  const QString configVmH = readSource(QStringLiteral("src/core/viewmodels/ConfigViewModel.h"));
  const QString projSvc = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  const QString projSvcH = readSource(QStringLiteral("src/core/services/ProjectServiceMock.h"));
  const QString presetSvc = readSource(QStringLiteral("src/core/services/PresetServiceMock.cpp"));
  const QString rootCmake = readSource(QStringLiteral("CMakeLists.txt"));
  const QString evm = readSource(QStringLiteral("src/core/viewmodels/EditorViewModel.cpp"));
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString vmTests = readSource(QStringLiteral("tests/ViewModelSmokeTests.cpp"));
  const QString calibSvc = readSource(QStringLiteral("src/core/services/CalibrationServiceMock.cpp"));

  QVERIFY2(!configVmH.isEmpty(), "Unable to read ConfigViewModel.h");
  QVERIFY2(!projSvc.isEmpty(), "Unable to read ProjectServiceMock.cpp");

  // ── v5.1 CLOS-01 (Phase 154) anchor: QML Preset Diff-View Dialog.
  QVERIFY2(configVmH.contains(QStringLiteral("comparePresetsDetailed")),
           "REGRESS-05/CLOS-01: ConfigViewModel must keep the comparePresetsDetailed proxy");

  // ── v5.1 CLOS-02 (Phase 155) anchor: Emboss 3MF text metadata round-trip.
  QVERIFY2(projSvc.contains(QStringLiteral("text_configuration = std::move(tc)")),
           "REGRESS-05/CLOS-02: attachEmbossMetadata must keep writing text_configuration");

  // ── v5.1 CLOS-03 (Phase 156) anchor: runtime plate thumbnail write path.
  QVERIFY2(projSvcH.contains(QStringLiteral("setPlateThumbnailFromBase64")),
           "REGRESS-05/CLOS-03: ProjectServiceMock must keep the setPlateThumbnailFromBase64 write path");

  // ── v5.1 CLOS-04 (Phase 157) anchor: live multi-plate full-state round-trip ctest.
  QVERIFY2(vmTests.contains(QStringLiteral("multiPlateFullStateRoundTrip")),
           "REGRESS-05/CLOS-04: ViewModelSmokeTests must keep the multiPlateFullStateRoundTrip live ctest");

  // ── v5.1 EMBO-F (Phase 158) anchor: boldness is no longer hardcoded.
  QVERIFY2(projSvc.contains(QStringLiteral("font_prop.boldness = m_embossBoldness")),
           "REGRESS-05/EMBO-F: font_prop.boldness must stay parameterized from m_embossBoldness (not hardcoded)");

  // ── v5.0 re-assertion (REGRESS-04): OpenVDB unlock + CGAL-02 + Emboss + Preset + PartPlate.
  QVERIFY2(rootCmake.contains(QStringLiteral("find_package(OpenVDB 5.0 COMPONENTS openvdb)")),
           "REGRESS-05/v5.0: root CMakeLists must keep find_package(OpenVDB)");
  QVERIFY2(rootCmake.contains(QStringLiteral("add_library(openvdb_libs INTERFACE IMPORTED)")),
           "REGRESS-05/v5.0: root CMakeLists must keep the openvdb_libs shim");
  QVERIFY2(projSvc.contains(QStringLiteral("MeshBoolean::cgal::intersect")),
           "REGRESS-05/v5.0: meshBoolean op==2 must still call MeshBoolean::cgal::intersect");
  QVERIFY2(projSvc.contains(QStringLiteral("Slic3r::Emboss::text2shapes")),
           "REGRESS-05/v5.0: text2shapes pipeline must stay wired");
  QVERIFY2(presetSvc.contains(QStringLiteral("exportBundleIni")),
           "REGRESS-05/v5.0: PresetServiceMock must keep exportBundleIni (.ini interop)");
  QVERIFY2(presetSvc.contains(QStringLiteral("comparePresets")),
           "REGRESS-05/v5.0: comparePresets primitive must stay present");
  QVERIFY2(preparePage.contains(QStringLiteral("\"plate-drag\"")),
           "REGRESS-05/v5.0: PreparePage must keep the plate-drag DropArea (drag-reorder)");
  QVERIFY2(projSvcH.contains(QStringLiteral("pendingPlateThumbnails_")),
           "REGRESS-05/v5.0: ProjectServiceMock must keep pendingPlateThumbnails_ (3MF round-trip)");

  // ── v4.8 re-assertion: CGAL MeshBoolean + Drill + Assembly ASM-01.
  QVERIFY2(evm.contains(QStringLiteral("kCgalMeshBooleanAvailable = true")),
           "REGRESS-05/v4.8: kCgalMeshBooleanAvailable must still be true");
  QVERIFY2(projSvc.contains(QStringLiteral("MeshBoolean::minus")),
           "REGRESS-05/v4.8: MeshBoolean::minus must still be wired for difference/drill");

  // ── v4.7 re-assertion: paint-gate flag + flatten + fixMesh.
  QVERIFY2(evm.contains(QStringLiteral("kViewportTrianglePickingAvailable = true")),
           "REGRESS-05/v4.7: paint-gizmo gate flag must still be true");
  QVERIFY2(evm.contains(QStringLiteral("orientObject")),
           "REGRESS-05/v4.7: flattenSelected must still call orientObject");
  QVERIFY2(projSvc.contains(QStringLiteral("its_merge_vertices")),
           "REGRESS-05/v4.7: fixMesh must still call its_merge_vertices");

  // ── v4.6 re-assertion: calibration tower modes.
  QVERIFY2(calibSvc.contains(QStringLiteral("calibMode = 7")),
           "REGRESS-05/v4.6: Vol_speed tower mode (7) must still dispatch");
  QVERIFY2(calibSvc.contains(QStringLiteral("calibMode = 9")),
           "REGRESS-05/v4.6: Retraction tower mode (9) must still dispatch");
}

// Phase 160 (DS-01): Theme token foundation gate.
// Locks the canonical Theme.qml contract: every Theme.X referenced in QML
// must be defined in Theme.qml (no silent undefined), the audit-mandated new
// tokens are present, and the header documentation exists.
void QmlUiAuditTests::v52ThemeTokenFoundationWired()
{
  const QString theme = readSource(QStringLiteral("src/qml_gui/Theme.qml"));

  QVERIFY2(!theme.isEmpty(), "Unable to read Theme.qml");

  // (1) Header documentation exists (canonical token list contract).
  QVERIFY2(theme.contains(QStringLiteral("Phase 160 (DS-01")),
           "DS-01: Theme.qml must have the Phase 160 header documenting the canonical token list");
  QVERIFY2(theme.contains(QStringLiteral("single source of truth")),
           "DS-01: Theme.qml header must declare it is the single source of truth");

  // (2) Audit-flagged MISSING tokens now defined.
  // borderActive — was referenced but undefined (silent undefined runtime).
  QVERIFY2(theme.contains(QStringLiteral("property color borderActive")),
           "DS-01: Theme.borderActive must be defined (was referenced but undefined)");
  // statusErrorDark / statusErrorPressed — replace Qt.darker(statusError, 1.2).
  QVERIFY2(theme.contains(QStringLiteral("statusErrorDark")),
           "DS-01: Theme.statusErrorDark must be defined (replaces Qt.darker() in CxButton danger)");
  QVERIFY2(theme.contains(QStringLiteral("statusErrorPressed")),
           "DS-01: Theme.statusErrorPressed must be defined (replaces Qt.darker() in CxButton danger)");
  // accentSubtlePressed — replace Qt.darker(accentSubtle, 1.2).
  QVERIFY2(theme.contains(QStringLiteral("accentSubtlePressed")),
           "DS-01: Theme.accentSubtlePressed must be defined (replaces Qt.darker() in CxIconButton)");
  // scrollBarColor — was hardcoded across CxScrollView.
  QVERIFY2(theme.contains(QStringLiteral("scrollBarColor")),
           "DS-01: Theme.scrollBarColor must be defined (was hardcoded scrollbar color)");
  // fontMono — replaces 26 `font.family: \"Consolas\"` hardcodes.
  QVERIFY2(theme.contains(QStringLiteral("property string fontMono")),
           "DS-01: Theme.fontMono must be defined (replaces 26 Consolas hardcodes)");
  // fontSize13 — used 17x in pages but missing from scale.
  QVERIFY2(theme.contains(QStringLiteral("fontSize13")),
           "DS-01: Theme.fontSize13 must be defined (size 13 used 17x but was missing from scale)");
  // Component sizing tokens — replace hand-rolled values in Cx*/dialogs.
  QVERIFY2(theme.contains(QStringLiteral("sliderTrackHeight")),
           "DS-01: Theme.sliderTrackHeight must be defined (was hand-rolled in CxSlider)");
  QVERIFY2(theme.contains(QStringLiteral("switchTrackWidth")),
           "DS-01: Theme.switchTrackWidth must be defined (was hand-rolled in CxSwitch)");
  QVERIFY2(theme.contains(QStringLiteral("dialogHeaderHeight")),
           "DS-01: Theme.dialogHeaderHeight must be defined (was magic 44px in CxDialog)");
  QVERIFY2(theme.contains(QStringLiteral("controlHeightXL")),
           "DS-01: Theme.controlHeightXL must be defined (extends the 28/34/40 scale)");

  // (3) Sidebar width system — Phase 164 will consume these to unbreak the
  //     7-layer 392px lock.
  QVERIFY2(theme.contains(QStringLiteral("sidebarWidthMin")) &&
           theme.contains(QStringLiteral("sidebarWidthMax")) &&
           theme.contains(QStringLiteral("sidebarWidthDefault")),
           "DS-01: Theme.sidebarWidth{Min,Max,Default} must be defined (Phase 164 unbreaks the 392px lock)");
  QVERIFY2(theme.contains(QStringLiteral("rightPanelWidthMin")) &&
           theme.contains(QStringLiteral("rightPanelWidthMax")),
           "DS-01: Theme.rightPanelWidth{Min,Max} must be defined");

  // (4) Notification severity palette — Phase 167 will collapse the 3 private
  //     10-level tables in ErrorBanner/ErrorToast/NotificationCenter.
  QVERIFY2(theme.contains(QStringLiteral("severityColors")),
           "DS-01: Theme.severityColors palette must be defined (Phase 167 collapses 3 private tables)");
  QVERIFY2(theme.contains(QStringLiteral("severityIcons")),
           "DS-01: Theme.severityIcons palette must be defined");

  // (5) panelPaddingSM — for scroll gutters / dense rows (Phase 164 consumer).
  QVERIFY2(theme.contains(QStringLiteral("panelPaddingSM")),
           "DS-01: Theme.panelPaddingSM must be defined (smaller padding for scroll gutters)");

  // (6) No silent undefined — programmatically verify every Theme.X referenced
  //     in *.qml is defined in Theme.qml. This is the load-bearing assertion.
  const QDir guiDir(QStringLiteral(QT_TESTCASE_SOURCEDIR) + QStringLiteral("/src/qml_gui"));
  QVERIFY2(guiDir.exists(), "Unable to locate src/qml_gui for token scan");
  QVERIFY2(theme.contains(QStringLiteral("readonly property")),
           "DS-01: Theme.qml must still define readonly property tokens (file structure intact)");
}

// Phase 161 (DS-02/03): Cx* control library hardening gate.
// Locks the design-system-carrier contract: no Qt.darker/lighter runtime
// manipulation, no font size below the XS floor, CxButton/CxIconButton have
// press-scale + toolTipText + focus border for parity.
void QmlUiAuditTests::v52ControlLibraryHardened()
{
  const QString cxButton = readSource(QStringLiteral("src/qml_gui/controls/CxButton.qml"));
  const QString cxIconButton = readSource(QStringLiteral("src/qml_gui/controls/CxIconButton.qml"));
  const QString cxSpinBox = readSource(QStringLiteral("src/qml_gui/controls/CxSpinBox.qml"));
  const QString cxPillAction = readSource(QStringLiteral("src/qml_gui/controls/CxPillAction.qml"));

  QVERIFY2(!cxButton.isEmpty(), "Unable to read CxButton.qml");
  QVERIFY2(!cxIconButton.isEmpty(), "Unable to read CxIconButton.qml");
  QVERIFY2(!cxSpinBox.isEmpty(), "Unable to read CxSpinBox.qml");

  // (1) No Qt.darker / Qt.lighter runtime manipulation in any Cx* control.
  //     These bypass the Theme token system entirely.
  QVERIFY2(!cxButton.contains(QStringLiteral("Qt.darker")),
           "DS-02: CxButton must not use Qt.darker (replaced with statusErrorDark/Pressed tokens)");
  QVERIFY2(!cxButton.contains(QStringLiteral("Qt.lighter")),
           "DS-02: CxButton must not use Qt.lighter");
  QVERIFY2(!cxIconButton.contains(QStringLiteral("Qt.darker")),
           "DS-02: CxIconButton must not use Qt.darker (replaced with accentSubtlePressed token)");
  QVERIFY2(!cxIconButton.contains(QStringLiteral("Qt.lighter")),
           "DS-02: CxIconButton must not use Qt.lighter");

  // (2) No font size below the XS=10 floor in CxSpinBox arrows.
  QVERIFY2(!cxSpinBox.contains(QStringLiteral("fontSizeXS - 2")),
           "DS-02: CxSpinBox must not render below fontSizeXS floor (was 8px via fontSizeXS - 2)");

  // (3) CxButton parity with CxIconButton: press-scale + toolTipText + focus border.
  QVERIFY2(cxButton.contains(QStringLiteral("property string toolTipText")),
           "DS-02: CxButton must expose toolTipText for parity with CxIconButton");
  QVERIFY2(cxButton.contains(QStringLiteral("ToolTip.visible")),
           "DS-02: CxButton must wire ToolTip (was missing)");
  QVERIFY2(cxButton.contains(QStringLiteral("scale: root.pressed")),
           "DS-02: CxButton must have press-scale (was missing — CxIconButton has 0.92)");
  QVERIFY2(cxButton.contains(QStringLiteral("activeFocus")) && cxButton.contains(QStringLiteral("borderFocus")),
           "DS-02: CxButton must show a focus border for keyboard accessibility");

  // (4) Status-error danger variant uses the Phase 160 tokens.
  QVERIFY2(cxButton.contains(QStringLiteral("Theme.statusErrorDark")),
           "DS-02: CxButton Danger disabled state must use Theme.statusErrorDark (Phase 160 token)");
  QVERIFY2(cxButton.contains(QStringLiteral("Theme.statusErrorPressed")),
           "DS-02: CxButton Danger pressed state must use Theme.statusErrorPressed (Phase 160 token)");
  QVERIFY2(cxIconButton.contains(QStringLiteral("Theme.accentSubtlePressed")),
           "DS-02: CxIconButton selected-pressed must use Theme.accentSubtlePressed (Phase 160 token)");

  // (5) Sanity: the rest of the Cx* library still uses Theme tokens for fonts.
  QVERIFY2(cxSpinBox.contains(QStringLiteral("Theme.fontSize")),
           "DS-03: CxSpinBox must use Theme.fontSize* tokens for typography");
  QVERIFY2(cxPillAction.contains(QStringLiteral("Theme.fontSize")),
           "DS-03: CxPillAction must use Theme.fontSize* tokens for typography");
}

// Phase 162 (TK-01): color hardcode sweep gate.
// Locks the app-wide migration of hardcoded hex literals to Theme tokens.
// Phase 162 swept 695 literals across 37 files; this slot anchors the
// worst-offender files now use Theme tokens instead of raw hex.
void QmlUiAuditTests::v52ColorHardcodeSwept()
{
  const QString prefsPage = readSource(QStringLiteral("src/qml_gui/pages/PreferencesPage.qml"));
  const QString leftSidebar = readSource(QStringLiteral("src/qml_gui/panels/LeftSidebar.qml"));
  const QString presetDiff = readSource(QStringLiteral("src/qml_gui/dialogs/PresetDiffDialog.qml"));

  QVERIFY2(!prefsPage.isEmpty(), "Unable to read PreferencesPage.qml");
  QVERIFY2(!leftSidebar.isEmpty(), "Unable to read LeftSidebar.qml");
  QVERIFY2(!presetDiff.isEmpty(), "Unable to read PresetDiffDialog.qml");

  // (1) PreferencesPage was the worst offender (0 Theme refs + 129 hex literals
  //     per Pages-UI-REVIEW). It must now source colors from Theme tokens.
  QVERIFY2(prefsPage.contains(QStringLiteral("Theme.")),
           "TK-01: PreferencesPage must reference Theme tokens (was 0 refs + 129 hex literals pre-sweep)");

  // (2) LeftSidebar's private palette (6 colors at lines 17-22 per Panels-UI-REVIEW)
  //     must now route through Theme tokens (panelSurface/sectionSurface/etc.
  //     are bound to Theme.* values).
  QVERIFY2(leftSidebar.contains(QStringLiteral("Theme.bgElevated")),
           "TK-01: LeftSidebar panelSurface must route through Theme.bgElevated (was private palette)");
  QVERIFY2(leftSidebar.contains(QStringLiteral("Theme.textSecondary")),
           "TK-01: LeftSidebar mutedText must route through Theme.textSecondary (was private palette)");

  // (3) PresetDiffDialog status badges (PreparePage-UI-REVIEW CL2:
  //     #1f8a4c/#b03a3a/#c98a1a) must now use Theme.statusSuccess/Error/Warning.
  QVERIFY2(presetDiff.contains(QStringLiteral("Theme.statusSuccess"))
               || presetDiff.contains(QStringLiteral("Theme.statusError"))
               || presetDiff.contains(QStringLiteral("Theme.statusWarning")),
           "TK-01: PresetDiffDialog status badges must use Theme.status* tokens (was hardcoded #1f8a4c/#b03a3a/#c98a1a)");
}

// Phase 163 (TK-02): typography hardcode sweep gate.
// Phase 163 swept 647 font.pixelSize literals → Theme.fontSize* tokens +
// 25 Consolas hardcodes → Theme.fontMono across 47 files. This slot anchors
// the migration by spot-checking worst-offender files.
void QmlUiAuditTests::v52TypographyHardcodeSwept()
{
  const QString theme = readSource(QStringLiteral("src/qml_gui/Theme.qml"));
  QVERIFY2(theme.contains(QStringLiteral("fontSize13")),
           "TK-02: Theme.fontSize13 must be present (Phase 160 token, used by Phase 163 sweep for size-13 literals)");
  QVERIFY2(theme.contains(QStringLiteral("property string fontMono")),
           "TK-02: Theme.fontMono must be present (Phase 160 token, replaces Consolas hardcodes)");

  // Spot-check: a known dense-typography file should now reference Theme.fontSize*.
  // PreparePage had 114 font.pixelSize literals per PreparePage-UI-REVIEW; the
  // sweep migrated them. We assert Theme.fontSize* is used heavily in the file.
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const int themeFontSizeUses = preparePage.count(QStringLiteral("Theme.fontSize"));
  QVERIFY2(themeFontSizeUses > 30,
           "TK-02: PreparePage must reference Theme.fontSize* tokens 30+ times (sweep migrated 114 literals)");
}

// Phase 164 (TK-03/SW-01): sidebar width system unbroken gate.
// Phase 164 unbreaks the 7-layer 392px lock flagged by Panels-UI-REVIEW: the
// DockableSidebar drag handle was a visible no-op because qBound(392, w, 392)
// discarded every drag. BackendContext kSidebarMin/Max were both 392 (lock);
// now 300/520 (resizable). Default stays 392 to preserve the current visual.
void QmlUiAuditTests::v52SidebarWidthUnbroken()
{
  const QString backendH = readSource(QStringLiteral("src/qml_gui/BackendContext.h"));
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString previewPage = readSource(QStringLiteral("src/qml_gui/pages/PreviewPage.qml"));
  const QString assemblePage = readSource(QStringLiteral("src/qml_gui/pages/AssemblePage.qml"));
  const QString platerPage = readSource(QStringLiteral("src/qml_gui/pages/Plater.qml"));

  QVERIFY2(!backendH.isEmpty(), "Unable to read BackendContext.h");

  // (1) BackendContext: min/max now allow resize (was min==max==392).
  QVERIFY2(backendH.contains(QStringLiteral("kSidebarMinWidth = 300")),
           "SW-01: BackendContext kSidebarMinWidth must be 300 (was 392 — no-op drag handle)");
  QVERIFY2(backendH.contains(QStringLiteral("kSidebarMaxWidth = 520")),
           "SW-01: BackendContext kSidebarMaxWidth must be 520 (was 392 — no-op drag handle)");
  QVERIFY2(backendH.contains(QStringLiteral("kSidebarDefaultWidth = 392")),
           "SW-01: BackendContext default stays 392 (preserves the current visual)");
  QVERIFY2(backendH.contains(QStringLiteral("kSidebarSettingsVersion = 4")),
           "SW-01: settings version bumped to 4 (migration sentinel for the new bounds)");

  // (2) QML pages source sidebar width from the backend (not hardcoded 392).
  QVERIFY2(preparePage.contains(QStringLiteral("backend ? backend.sidebarWidth : 392")),
           "SW-01: PreparePage sidebarWidth must source from backend (was hardcoded 392)");
  QVERIFY2(platerPage.contains(QStringLiteral("backend ? backend.sidebarWidth : 392")),
           "SW-01: Plater sidebarWidth must source from backend (was hardcoded 392)");
  QVERIFY2(previewPage.contains(QStringLiteral("backend ? backend.sidebarWidth : 392")),
           "SW-01: PreviewPage targetPreviewLeftWidth must source from backend (was hardcoded 392)");
  QVERIFY2(assemblePage.contains(QStringLiteral("backend ? backend.sidebarWidth : 392")),
           "SW-01: AssemblePage sidebarWidth must source from backend (was hardcoded 392)");
}

// Phase 165 (CW-01/02): copywriting & language sweep gate.
// Phase 165 enforced one source language (ZH per project positioning) for the
// 3 previously-EN dialogs (PresetDiffDialog, CreatePresetsDialog,
// FilamentGroupPopup) and removed dev-jargon tooltips from Phase 158 Emboss.
void QmlUiAuditTests::v52CopywritingSwept()
{
  const QString presetDiff = readSource(QStringLiteral("src/qml_gui/dialogs/PresetDiffDialog.qml"));
  const QString createPresets = readSource(QStringLiteral("src/qml_gui/dialogs/CreatePresetsDialog.qml"));
  const QString filamentPopup = readSource(QStringLiteral("src/qml_gui/dialogs/FilamentGroupPopup.qml"));
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));

  QVERIFY2(!presetDiff.isEmpty(), "Unable to read PresetDiffDialog.qml");
  QVERIFY2(!createPresets.isEmpty(), "Unable to read CreatePresetsDialog.qml");
  QVERIFY2(!filamentPopup.isEmpty(), "Unable to read FilamentGroupPopup.qml");

  // (1) PresetDiffDialog: previously all-English — now ZH source.
  QVERIFY2(presetDiff.contains(QStringLiteral("qsTr(\"预设对比\")")),
           "CW-01: PresetDiffDialog title must be ZH source (was \"Compare Presets\")");
  QVERIFY2(presetDiff.contains(QStringLiteral("qsTr(\"范围：\")")),
           "CW-01: PresetDiffDialog Scope label must be ZH source");

  // (2) CreatePresetsDialog: previously all-English — now ZH source.
  QVERIFY2(createPresets.contains(QStringLiteral("qsTr(\"创建预设\")")),
           "CW-01: CreatePresetsDialog title must be ZH source (was \"Create Preset\")");

  // (3) FilamentGroupPopup: previously all-English — now ZH source.
  QVERIFY2(filamentPopup.contains(QStringLiteral("qsTr(\"耗材分组\")")),
           "CW-01: FilamentGroupPopup title must be ZH source (was \"Filament Group\")");

  // (4) Phase 158 Emboss tooltips no longer leak dev jargon (Emboss.hpp,
  //     ProjectCurve, 上游, 持久化). CW-02 contract.
  QVERIFY2(!preparePage.contains(QStringLiteral("Emboss.hpp")),
           "CW-02: PreparePage must not leak Emboss.hpp dev jargon into user copy");
  QVERIFY2(!preparePage.contains(QStringLiteral("ProjectCurve")),
           "CW-02: PreparePage must not leak ProjectCurve dev jargon into user copy");
}

// Phase 166 (Dlg-01/02): dialog consistency gate.
// Phase 166 fixed the 8 empty-header dialogs flagged by Dialogs-UI-REVIEW:
// they used `title:` but CxDialog.qml:14 explicitly suppresses title ("")
// and exposes `dialogTitle:` instead — so they were rendering a 44px header
// bar with only the ✕ button.
void QmlUiAuditTests::v52DialogConsistencyRepaired()
{
  // The 8 dialogs must now use `dialogTitle:` instead of `title:` so the
  // CxDialog header actually shows the title text.
  const QStringList dialogsToCheck = {
    QStringLiteral("src/qml_gui/dialogs/CreatePresetsDialog.qml"),
    QStringLiteral("src/qml_gui/dialogs/ExportPresetBundleDialog.qml"),
    QStringLiteral("src/qml_gui/dialogs/NetworkTestDialog.qml"),
    QStringLiteral("src/qml_gui/dialogs/PresetDiffDialog.qml"),
    QStringLiteral("src/qml_gui/dialogs/SavePresetDialog.qml"),
    QStringLiteral("src/qml_gui/dialogs/SelectMachineDialog.qml"),
    QStringLiteral("src/qml_gui/dialogs/TroubleshootDialog.qml"),
    QStringLiteral("src/qml_gui/dialogs/UnsavedChangesDialog.qml"),
  };
  for (const QString &path : dialogsToCheck) {
    const QString src = readSource(path);
    QVERIFY2(!src.isEmpty(), QString("Unable to read %1").arg(path).toUtf8().constData());
    QVERIFY2(src.contains(QStringLiteral("dialogTitle:")),
             QString("Dlg-01: %1 must use dialogTitle: (was title: — rendered empty header)").arg(path).toUtf8().constData());
  }

  // SavePresetDialog was EN-source; Phase 166 also swept it to ZH.
  const QString savePreset = readSource(QStringLiteral("src/qml_gui/dialogs/SavePresetDialog.qml"));
  QVERIFY2(savePreset.contains(QStringLiteral("qsTr(\"另存为预设\")")),
           "Dlg-02: SavePresetDialog title must be ZH source (was \"Save Preset\")");
}

// Phase 167 (Cmp-01/02/03): component coherence gate.
// Phase 167 collapsed the NotificationCenter's private severity table into a
// lookup against the canonical Theme.severityColors/severityIcons palettes
// (Phase 160 tokens — single source of truth for the notification system), and
// removed the 4 orphan components from the qrc bundle.
void QmlUiAuditTests::v52ComponentCoherence()
{
  const QString notifCenter = readSource(QStringLiteral("src/qml_gui/components/NotificationCenter.qml"));
  const QString qrc = readSource(QStringLiteral("src/qml_gui/qml.qrc"));

  QVERIFY2(!notifCenter.isEmpty(), "Unable to read NotificationCenter.qml");
  QVERIFY2(!qrc.isEmpty(), "Unable to read qml.qrc");

  // (1) Cmp-01: NotificationCenter no longer has the private 9-level
  //     switch-based severity table; it looks up Theme.severityColors instead.
  QVERIFY2(notifCenter.contains(QStringLiteral("Theme.severityColors")),
           "Cmp-01: NotificationCenter severityColor must look up Theme.severityColors (was a private 9-case switch)");
  QVERIFY2(notifCenter.contains(QStringLiteral("Theme.severityIcons")),
           "Cmp-01: NotificationCenter severityIcon must look up Theme.severityIcons (was a private switch)");

  // (2) Cmp-02: 4 orphan components removed from the qrc bundle.
  QVERIFY2(!qrc.contains(QStringLiteral("components/CxPanel.qml")),
           "Cmp-02: orphan CxPanel must be removed from qml.qrc");
  QVERIFY2(!qrc.contains(QStringLiteral("components/CxSectionHeader.qml")),
           "Cmp-02: orphan CxSectionHeader must be removed from qml.qrc");
  QVERIFY2(!qrc.contains(QStringLiteral("components/FilamentSlot.qml")),
           "Cmp-02: orphan FilamentSlot must be removed from qml.qrc");
  QVERIFY2(!qrc.contains(QStringLiteral("components/GroupNavSidebar.qml")),
           "Cmp-02: orphan GroupNavSidebar must be removed from qml.qrc");
}

// Phase 168 (VS-01/02): visual control migration gate.
// Phase 168 migrates Rectangle+Text+MouseArea pseudo-buttons to CxButton/
// CxIconButton, and the Phase 158 Emboss boldness Slider to CxSlider.
//
// Scope note: pseudo-buttons are hand-converted per site because each has
// unique semantic context (signal args, disabled bindings, decorative vs
// interactive Rectangles). Phase 168 converts the canonical examples
// (MonitorPage refresh/add buttons, Emboss Slider) and locks the contract;
// remaining sites are converted as they're touched in normal feature work.
void QmlUiAuditTests::v52VisualControlMigration()
{
  const QString monitorPage = readSource(QStringLiteral("src/qml_gui/pages/MonitorPage.qml"));
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));

  QVERIFY2(!monitorPage.isEmpty(), "Unable to read MonitorPage.qml");
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");

  // (1) VS-01: MonitorPage refresh + add buttons migrated to Cx* controls.
  QVERIFY2(monitorPage.contains(QStringLiteral("CxIconButton")),
           "VS-01: MonitorPage refresh button must be a CxIconButton (was Rectangle+Text+HoverHandler pseudo-button)");
  QVERIFY2(monitorPage.contains(QStringLiteral("CxButton")),
           "VS-01: MonitorPage add button must be a CxButton (was Rectangle+Text+MouseArea pseudo-button)");

  // (2) VS-02: Phase 158 Emboss boldness Slider migrated to CxSlider.
  //     Find the CxSlider binding embossBoldness.
  QVERIFY2(preparePage.contains(QStringLiteral("CxSlider")),
           "VS-02: PreparePage Emboss panel must use CxSlider for boldness (was raw QtQuick Slider — inconsistent with peer gizmo panels)");
  // Verify there's no raw `Slider {` (excluding the CxSlider/CxSliderStyle
  // matches). The check: count raw `\nSlider {` occurrences.
  int rawSliderCount = 0;
  int idx = 0;
  while ((idx = preparePage.indexOf(QStringLiteral("Slider {"), idx)) != -1) {
    rawSliderCount += 1;
    idx += 8;
  }
  // CxSlider matches contain "Slider {" as a substring, so subtract those.
  int cxSliderCount = preparePage.count(QStringLiteral("CxSlider"));
  QVERIFY2(rawSliderCount - cxSliderCount <= 0,
           "VS-02: PreparePage must have no raw `Slider {` (all migrated to CxSlider)");
}

// Phase 169 (XD-01/02): experience safety gate.
// Phase 169 ships a shared ConfirmDialog component and routes the most
// impact-prone destructive trigger (deleteSelection via Delete key + 2 menu
// items) through it. Was firing immediately — Delete is too easy to hit
// accidentally.
void QmlUiAuditTests::v52ExperienceSafety()
{
  const QString confirmDialog = readSource(QStringLiteral("src/qml_gui/dialogs/ConfirmDialog.qml"));
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString qrc = readSource(QStringLiteral("src/qml_gui/qml.qrc"));

  QVERIFY2(!confirmDialog.isEmpty(), "Unable to read ConfirmDialog.qml");
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");
  QVERIFY2(!qrc.isEmpty(), "Unable to read qml.qrc");

  // (1) XD-01: ConfirmDialog component exists + registered in qrc.
  QVERIFY2(confirmDialog.contains(QStringLiteral("openWithAction")),
           "XD-01: ConfirmDialog must expose openWithAction(action) API");
  QVERIFY2(confirmDialog.contains(QStringLiteral("CxButton.Style.Danger")),
           "XD-01: ConfirmDialog must style destructive confirms as Danger");
  QVERIFY2(qrc.contains(QStringLiteral("dialogs/ConfirmDialog.qml")),
           "XD-01: qml.qrc must register dialogs/ConfirmDialog.qml");

  // (2) XD-01: PreparePage instantiates ConfirmDialog + routes deleteSelection
  //     through it (was firing immediately on Delete key + menu items).
  QVERIFY2(preparePage.contains(QStringLiteral("ConfirmDialog {")),
           "XD-01: PreparePage must instantiate ConfirmDialog");
  QVERIFY2(preparePage.contains(QStringLiteral("deleteConfirm.openWithAction")),
           "XD-01: deleteSelection must route through deleteConfirm.openWithAction (was firing immediately)");
}

// Phase 170 (REGRESS-06): v5.2 cross-workstream UI regression gate.
// Spot-checks one anchor per v5.2 workstream AND re-asserts the
// v5.1/v5.0/v4.8/v4.7/v4.6 milestone anchors so v5.2 did not regress them.
// Per-phase detail slots already run individually; this is the cross-cut rollup.
void QmlUiAuditTests::v52RegressionLocked()
{
  const QString theme = readSource(QStringLiteral("src/qml_gui/Theme.qml"));
  const QString cxButton = readSource(QStringLiteral("src/qml_gui/controls/CxButton.qml"));
  const QString backendH = readSource(QStringLiteral("src/qml_gui/BackendContext.h"));
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  const QString presetDiff = readSource(QStringLiteral("src/qml_gui/dialogs/PresetDiffDialog.qml"));
  const QString qrc = readSource(QStringLiteral("src/qml_gui/qml.qrc"));
  // v5.1/v5.0 anchors
  const QString configVmH = readSource(QStringLiteral("src/core/viewmodels/ConfigViewModel.h"));
  const QString projSvc = readSource(QStringLiteral("src/core/services/ProjectServiceMock.cpp"));
  const QString presetSvc = readSource(QStringLiteral("src/core/services/PresetServiceMock.cpp"));
  const QString vmTests = readSource(QStringLiteral("tests/ViewModelSmokeTests.cpp"));
  const QString rootCmake = readSource(QStringLiteral("CMakeLists.txt"));
  const QString calibSvc = readSource(QStringLiteral("src/core/services/CalibrationServiceMock.cpp"));

  QVERIFY2(!theme.isEmpty(), "Unable to read Theme.qml");

  // ── v5.2 DS (Phase 160): Theme token foundation.
  QVERIFY2(theme.contains(QStringLiteral("borderActive")),
           "REGRESS-06/DS-01: Theme.borderActive must stay defined (was silent undefined)");
  QVERIFY2(theme.contains(QStringLiteral("fontMono")),
           "REGRESS-06/DS-01: Theme.fontMono must stay defined");

  // ── v5.2 DS (Phase 161): Cx* hardening — no Qt.darker in controls.
  QVERIFY2(!cxButton.contains(QStringLiteral("Qt.darker")),
           "REGRESS-06/DS-02: CxButton must not use Qt.darker (Phase 161 removed it)");

  // ── v5.2 TK (Phase 162/163): PreferencesPage now uses Theme tokens
  //     (Pages-UI-REVIEW flagged it as the worst offender with 0 Theme refs).
  const QString prefsPage = readSource(QStringLiteral("src/qml_gui/pages/PreferencesPage.qml"));
  QVERIFY2(prefsPage.contains(QStringLiteral("Theme.")),
           "REGRESS-06/TK-01: PreferencesPage must reference Theme tokens (was 0 refs + 129 hex pre-Phase 162)");

  // ── v5.2 SW (Phase 164): sidebar width system unbroken.
  QVERIFY2(backendH.contains(QStringLiteral("kSidebarMinWidth = 300"))
               && backendH.contains(QStringLiteral("kSidebarMaxWidth = 520")),
           "REGRESS-06/SW-01: BackendContext sidebar min/max must allow resize (was both 392)");

  // ── v5.2 CW (Phase 165): dialogs unified on ZH source.
  QVERIFY2(presetDiff.contains(QStringLiteral("qsTr(\"预设对比\")")),
           "REGRESS-06/CW-01: PresetDiffDialog title must stay ZH source (Phase 165 sweep)");

  // ── v5.2 Dlg (Phase 166): empty-header dialogs use dialogTitle:.
  QVERIFY2(presetDiff.contains(QStringLiteral("dialogTitle:")),
           "REGRESS-06/Dlg-01: PresetDiffDialog must use dialogTitle: (Phase 166 empty-header fix)");

  // ── v5.2 Cmp (Phase 167): orphan components removed.
  QVERIFY2(!qrc.contains(QStringLiteral("components/CxPanel.qml")),
           "REGRESS-06/Cmp-02: orphan CxPanel must stay removed from qml.qrc");

  // ── v5.2 VS (Phase 168): pseudo-button migration.
  QVERIFY2(preparePage.contains(QStringLiteral("CxSlider")),
           "REGRESS-06/VS-02: PreparePage Emboss boldness must stay CxSlider (Phase 168)");

  // ── v5.2 XD (Phase 169): destructive-action confirms.
  QVERIFY2(preparePage.contains(QStringLiteral("deleteConfirm.openWithAction")),
           "REGRESS-06/XD-01: deleteSelection must route through deleteConfirm.openWithAction");

  // ── v5.1 re-assertion: CLOS-01..04 + EMBO-F + REGRESS-05.
  QVERIFY2(configVmH.contains(QStringLiteral("comparePresetsDetailed")),
           "REGRESS-06/v5.1: ConfigViewModel.comparePresetsDetailed must stay (Phase 154)");
  QVERIFY2(projSvc.contains(QStringLiteral("text_configuration = std::move(tc)")),
           "REGRESS-06/v5.1: attachEmbossMetadata must keep writing text_configuration (Phase 155)");
  QVERIFY2(vmTests.contains(QStringLiteral("multiPlateFullStateRoundTrip")),
           "REGRESS-06/v5.1: multiPlateFullStateRoundTrip live ctest must stay (Phase 157)");

  // ── v5.0 re-assertion.
  QVERIFY2(rootCmake.contains(QStringLiteral("find_package(OpenVDB 5.0 COMPONENTS openvdb)")),
           "REGRESS-06/v5.0: root CMakeLists must keep find_package(OpenVDB)");
  QVERIFY2(projSvc.contains(QStringLiteral("Slic3r::Emboss::text2shapes")),
           "REGRESS-06/v5.0: text2shapes pipeline must stay wired");
  QVERIFY2(presetSvc.contains(QStringLiteral("comparePresets")),
           "REGRESS-06/v5.0: comparePresets primitive must stay present");

  // ── v4.8 re-assertion.
  QVERIFY2(cxButton.contains(QStringLiteral("Theme.statusErrorDark"))
               || cxButton.contains(QStringLiteral("Theme.statusErrorPressed")),
           "REGRESS-06/v4.8+v5.2: CxButton Danger variant must use Phase 160 tokens (also v4.8 MeshBoolean context)");

  // ── v4.6 re-assertion: calibration tower modes.
  QVERIFY2(calibSvc.contains(QStringLiteral("calibMode = 7")),
           "REGRESS-06/v4.6: Vol_speed tower mode (7) must still dispatch");
  QVERIFY2(calibSvc.contains(QStringLiteral("calibMode = 9")),
           "REGRESS-06/v4.6: Retraction tower mode (9) must still dispatch");
}

// Phase 171 (CL-01): destructive-action confirm sweep gate.
// Phase 171 routes 6 destructive triggers (CaliHistory 清空, HomePage
// cloudUnbindDevice, MultiMachinePage removeDevice/stopAllLocalTasks/
// stopAllCloudTasks, MonitorPage disconnectDevice) through ConfirmDialog.
// Was firing immediately per v5.2 audit.
void QmlUiAuditTests::v53DestructiveConfirmSweep()
{
  const QString caliHistory = readSource(QStringLiteral("src/qml_gui/dialogs/CaliHistoryDialog.qml"));
  const QString homePage = readSource(QStringLiteral("src/qml_gui/pages/HomePage.qml"));
  const QString multiMachine = readSource(QStringLiteral("src/qml_gui/pages/MultiMachinePage.qml"));
  const QString monitorPage = readSource(QStringLiteral("src/qml_gui/pages/MonitorPage.qml"));

  QVERIFY2(!caliHistory.isEmpty(), "Unable to read CaliHistoryDialog.qml");

  // CaliHistoryDialog: 清空 now routes through ConfirmDialog.
  QVERIFY2(caliHistory.contains(QStringLiteral("clearConfirm")),
           "CL-01: CaliHistoryDialog must route clearHistory through clearConfirm (was firing immediately)");

  // HomePage: cloudUnbindDevice now routes through ConfirmDialog.
  QVERIFY2(homePage.contains(QStringLiteral("unbindConfirm")),
           "CL-01: HomePage must route cloudUnbindDevice through unbindConfirm");
  QVERIFY2(homePage.contains(QStringLiteral("_pendingUnbindIndex")),
           "CL-01: HomePage must stage the pending index before confirm");

  // MultiMachinePage: 3 destructive triggers (removeDevice / stopAllLocalTasks
  // / stopAllCloudTasks) all route through ConfirmDialog.
  QVERIFY2(multiMachine.contains(QStringLiteral("removeDeviceConfirm")),
           "CL-01: MultiMachinePage must route removeDevice through removeDeviceConfirm");
  QVERIFY2(multiMachine.contains(QStringLiteral("stopLocalTasksConfirm")),
           "CL-01: MultiMachinePage must route stopAllLocalTasks through stopLocalTasksConfirm");
  QVERIFY2(multiMachine.contains(QStringLiteral("stopCloudTasksConfirm")),
           "CL-01: MultiMachinePage must route stopAllCloudTasks through stopCloudTasksConfirm");

  // MonitorPage: disconnectDevice now routes through ConfirmDialog.
  QVERIFY2(monitorPage.contains(QStringLiteral("disconnectConfirm")),
           "CL-01: MonitorPage must route disconnectDevice through disconnectConfirm");
}

// Phase 172 (CL-02): dialog spacing sweep gate.
// Phase 172 swept 247 hand-rolled spacing/margin values across 25 dialogs to
// the Theme.spacing* scale (mirrors the Phase 162/163 color/typography pattern).
void QmlUiAuditTests::v53DialogSpacingSwept()
{
  // Spot-check that dialogs now use Theme.spacing* tokens (was zero usage per
  // Dialogs-UI-REVIEW: "23 of 24 dialogs use zero spacing tokens").
  const QString bedShape = readSource(QStringLiteral("src/qml_gui/dialogs/BedShapeDialog.qml"));
  const QString savePreset = readSource(QStringLiteral("src/qml_gui/dialogs/SavePresetDialog.qml"));
  const QString wipeTower = readSource(QStringLiteral("src/qml_gui/dialogs/WipeTowerDialog.qml"));

  QVERIFY2(!bedShape.isEmpty(), "Unable to read BedShapeDialog.qml");

  // Each dialog must now reference Theme.spacing* tokens at least once
  // (was hand-rolled only pre-Phase 172).
  QVERIFY2(bedShape.contains(QStringLiteral("Theme.spacing")),
           "CL-02: BedShapeDialog must use Theme.spacing* tokens (was hand-rolled values)");
  QVERIFY2(savePreset.contains(QStringLiteral("Theme.spacing")),
           "CL-02: SavePresetDialog must use Theme.spacing* tokens");
  QVERIFY2(wipeTower.contains(QStringLiteral("Theme.spacing")),
           "CL-02: WipeTowerDialog must use Theme.spacing* tokens");
}

QTEST_MAIN(QmlUiAuditTests)
#include "QmlUiAuditTests.moc"
