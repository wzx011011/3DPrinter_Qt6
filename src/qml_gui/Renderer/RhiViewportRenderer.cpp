#include "RhiViewportRenderer.h"
#include "RhiViewport.h"
#include "core/rendering/AssemblyMeasureGeometry.h"
#include "core/rendering/GizmoCenter.h"
#include "core/rendering/GizmoGeometry.h"

#include <QDebug>
#include <QFile>
#include <QHash>

#include <algorithm>
#include <cstddef>

RhiViewportRenderer::RhiViewportRenderer() = default;

RhiViewportRenderer::~RhiViewportRenderer()
{
  releaseResources();
}

void RhiViewportRenderer::initialize(QRhiCommandBuffer *cb)
{
  Q_UNUSED(cb);
  // BUG FIX: do NOT call releaseResources() here unconditionally.
  // QQuickRhiItemRenderer::initialize() is called on every swapchain rebuild
  // (window resize, visibility change, etc). Clearing all GPU buffers here
  // forces a full re-upload every frame, which makes the bed grid flash or
  // disappear entirely (the original "blank viewport" symptom).
  // Only reset the per-frame upload flag so the next render re-uploads scene
  // buffers; the pipelines and buffers themselves are reused.
  m_pipelineFailed = false;
  m_sceneBuffersUploaded = false;
}

void RhiViewportRenderer::synchronize(QQuickRhiItem *item)
{
  auto *viewport = qobject_cast<RhiViewport *>(item);
  if (viewport == nullptr)
    return;

  m_canvasType = viewport->m_canvasType;
  // Phase 91 (ASMEXPLODE-02): mirror upstream m_explosion_ratio
  // (GLCanvas3D.hpp:596). If the ratio changed since the last synchronize,
  // force a model re-upload so buildModelVertices re-applies the per-volume
  // offset on the CanvasAssembleView branch. The offset is gated to
  // CanvasAssembleView in buildModelVertices, so Prepare/Preview are unaffected.
  m_explosionRatio = viewport->m_explosionRatio;
  if (!qFuzzyCompare(m_explosionRatio, m_lastExplosionRatio))
  {
    m_lastExplosionRatio = m_explosionRatio;
    m_modelVertexBufferUploaded = false;
    // Connector guide lines appear/disappear when the ratio crosses 1.0.
    m_assemblyConnectorBufferUploaded = false;
  }
  m_meshBytes = viewport->m_meshData.size();
  m_previewBytes = viewport->m_previewData.size();
  QList<int> activeObjectIndices;
  activeObjectIndices.reserve(viewport->m_activePlateObjectIndices.size());
  for (const QVariant &value : viewport->m_activePlateObjectIndices)
    activeObjectIndices.append(value.toInt());
  QList<int> batchSourceObjectIndices;
  batchSourceObjectIndices.reserve(viewport->m_meshBatchSourceObjectIndices.size());
  for (const QVariant &value : viewport->m_meshBatchSourceObjectIndices)
    batchSourceObjectIndices.append(value.toInt());

  // Phase 138 (ASM-01): build the sourceObjectIndex -> assemble offset map by
  // zipping m_assembleOffsets with the parallel meshBatchSourceObjectIndices
  // list. Skip zero offsets (they translate nothing). Used by buildModelVertices
  // on the CanvasAssembleView path.
  if (m_assembleOffsets != viewport->m_assembleOffsets)
  {
    m_assembleOffsets = viewport->m_assembleOffsets;
    m_modelVertexBufferUploaded = false;
  }
  // Phase 141 (DEBT-04): mirror the parallel rotation/scale lists. Any change to
  // any of the three lists forces a vertex re-upload so the composed transform is
  // reflected in the live CanvasAssembleView render (v4.8 tech-debt closure).
  if (m_assembleRotations != viewport->m_assembleRotations)
  {
    m_assembleRotations = viewport->m_assembleRotations;
    m_modelVertexBufferUploaded = false;
  }
  if (m_assembleScales != viewport->m_assembleScales)
  {
    m_assembleScales = viewport->m_assembleScales;
    m_modelVertexBufferUploaded = false;
  }
  m_assembleOffsetBySource.clear();
  m_assembleOffsetBySource.reserve(m_assembleOffsets.size() * 2);
  m_assembleTransformBySource.clear();
  m_assembleTransformBySource.reserve(m_assembleOffsets.size() * 2);
  {
    const int offsetCount = int(m_assembleOffsets.size());
    const int rotCount = int(m_assembleRotations.size());
    const int scaleCount = int(m_assembleScales.size());
    const int idxCount = batchSourceObjectIndices.size();
    const int pairCount = std::min({offsetCount, idxCount});
    for (int i = 0; i < pairCount; ++i)
    {
      const QVector3D off = m_assembleOffsets[i].value<QVector3D>();
      const bool hasRot = i < rotCount;
      const bool hasScale = i < scaleCount;
      const QVector3D rot = hasRot ? m_assembleRotations[i].value<QVector3D>() : QVector3D();
      const QVector3D scl = hasScale ? m_assembleScales[i].value<QVector3D>() : QVector3D(1.0f, 1.0f, 1.0f);
      const bool offZero = (off.x() == 0.0f && off.y() == 0.0f && off.z() == 0.0f);
      const bool rotZero = (rot.x() == 0.0f && rot.y() == 0.0f && rot.z() == 0.0f);
      const bool sclIdentity = (scl.x() == 1.0f && scl.y() == 1.0f && scl.z() == 1.0f);
      // Keep the legacy offset-only map populated (other readers + the regression
      // slot anchor on its presence); the composed transform is the source of truth
      // for rendering once any non-translate component is present.
      if (!offZero)
        m_assembleOffsetBySource.insert(batchSourceObjectIndices[i], off);
      if (offZero && rotZero && sclIdentity)
        continue; // identity — no compose needed
      // Build translate * rotateZ * rotateY * rotateX * scale (matches the
      // gizmo Euler XYZ convention used by EditorViewModel assembleRotation).
      QMatrix4x4 m;
      m.translate(off);
      if (!rotZero)
      {
        m.rotate(rot.z(), QVector3D(0.0f, 0.0f, 1.0f));
        m.rotate(rot.y(), QVector3D(0.0f, 1.0f, 0.0f));
        m.rotate(rot.x(), QVector3D(1.0f, 0.0f, 0.0f));
      }
      if (!sclIdentity)
        m.scale(scl);
      m_assembleTransformBySource.insert(batchSourceObjectIndices[i], m);
    }
  }
  const bool modelGenerationChanged = m_modelGeneration != viewport->m_modelGeneration;
  if (m_sceneGeneration != viewport->m_sceneGeneration) {
    m_sceneGeneration = viewport->m_sceneGeneration;
    m_prepareScene.setBed(viewport->m_bedWidth,
                          viewport->m_bedDepth,
                          viewport->m_bedOriginX,
                          viewport->m_bedOriginY,
                          viewport->m_bedShapeType,
                          viewport->m_bedDiameter);
    m_prepareScene.setShowBed(viewport->m_showBed);
  }
  if (m_modelGeneration != viewport->m_modelGeneration) {
    m_modelGeneration = viewport->m_modelGeneration;
    m_prepareScene.setPlateContext(viewport->m_currentPlateIndex,
                                   viewport->m_plateCount,
                                   activeObjectIndices);
    m_prepareScene.setModelMeshData(viewport->m_meshData,
                                    batchSourceObjectIndices,
                                    activeObjectIndices);
  }
  const int prevSelectedSourceObjectIndex = m_prepareScene.selectedSourceObjectIndex();
  m_prepareScene.setSelectedSourceObjectIndex(viewport->m_selectedSourceObjectIndex);
  m_prepareScene.setHoveredSourceObjectIndex(viewport->m_hoveredSourceObjectIndex);
  const QSize pixelSize = renderTarget() ? renderTarget()->pixelSize() : QSize(int(viewport->width()), int(viewport->height()));
  const float aspect = pixelSize.height() > 0
      ? float(std::max(1, pixelSize.width())) / float(std::max(1, pixelSize.height()))
      : 1.0f;
  m_cameraMvp = viewport->cameraMvp(aspect);
  if (viewport->m_cameraDirty) {
    m_prepareScene.markCameraDirty();
    viewport->m_cameraDirty = false;
  }
  m_clearColor = (m_canvasType == RhiViewport::CanvasPreview)
      ? QColor(8, 12, 20)
      : QColor(86, 87, 93);

  // ── Phase 26: Preview segment pipeline — store preview data + control props ──
  if (m_previewData != viewport->m_previewData) {
    m_previewData = viewport->m_previewData;
    resetPreviewGpuState(false);
    parsePreviewSegments();
    qInfo("[RHI] preview payload bytes=%d vertices=%u spans=%d",
          int(m_previewData.size()),
          m_previewSegmentVertexCount,
          int(m_previewDrawSpans.size()));
  }
  m_layerMin = viewport->m_layerMin;
  m_layerMax = viewport->m_layerMax;
  m_moveEnd = viewport->m_moveEnd;
  m_showTravelMoves = viewport->m_showTravelMoves;
  m_gcodeViewMode = viewport->m_gcodeViewMode;

  // ── Phase 67: Gizmo state pipeline ──
  // Read gizmoMode/cutAxis/cutPosition from the viewport item and compute
  // gizmoCenter from the selected object's AABB. setGizmoMode already calls
  // update() on the item (RhiViewport.cpp:321), so this synchronize runs on
  // the next frame after any QML state change. The diagnostic log fires only
  // on actual state deltas to avoid spamming once per frame.
  const int prevGizmoMode = m_gizmoMode;
  const int prevCutAxis = m_cutAxis;
  const float prevCutPosition = m_cutPosition;
  const QVector3D prevGizmoCenter = m_gizmoCenter;
  m_gizmoMode = viewport->m_gizmoMode;
  m_cutAxis = viewport->m_cutAxis;
  m_cutPosition = viewport->m_cutPosition;
  m_gizmoCenter = computeGizmoCenter();
  m_cameraEye = viewport->m_camera.eye();
  if (m_gizmoMode != prevGizmoMode || m_cutAxis != prevCutAxis ||
      !qFuzzyCompare(m_cutPosition, prevCutPosition) ||
      m_gizmoCenter != prevGizmoCenter)
  {
    qInfo("[RHI] gizmo state: mode=%d cutAxis=%d cutPos=%.3f center=(%.2f,%.2f,%.2f)",
          m_gizmoMode, m_cutAxis, double(m_cutPosition),
          double(m_gizmoCenter.x()), double(m_gizmoCenter.y()), double(m_gizmoCenter.z()));
  }
  if (modelGenerationChanged ||
      m_gizmoMode != prevGizmoMode || m_cutAxis != prevCutAxis ||
      !qFuzzyCompare(m_cutPosition, prevCutPosition) ||
      prevSelectedSourceObjectIndex != m_prepareScene.selectedSourceObjectIndex() ||
      m_gizmoCenter != prevGizmoCenter)
  {
    m_cutPlaneDirty = true;
    m_cutPlaneFillBufferUploaded = false;
    m_cutPlaneOutlineBufferUploaded = false;
  }

  // Phase 92 (ASMMEASURE-02): mirror the two Assembly-measure selection indices
  // and force an overlay re-upload when they change OR the gizmo mode flips
  // to/from GizmoAssemblyMeasure (19). This closes the re-render loop:
  // selection change -> viewmodel stateChanged -> QML binding ->
  // RhiViewport setter -> update() -> synchronize() copies here ->
  // uploadAssemblyMeasureBuffers rebuilds the overlay.
  m_assemblyMeasureSelectedA = viewport->m_assemblyMeasureSelectedA;
  m_assemblyMeasureSelectedB = viewport->m_assemblyMeasureSelectedB;
  if (m_assemblyMeasureSelectedA != m_assemblyMeasureLastSelectedA
      || m_assemblyMeasureSelectedB != m_assemblyMeasureLastSelectedB
      || m_gizmoMode != prevGizmoMode)
  {
    m_assemblyMeasureLastSelectedA = m_assemblyMeasureSelectedA;
    m_assemblyMeasureLastSelectedB = m_assemblyMeasureSelectedB;
    m_assemblyMeasureLineBufferUploaded = false;
    m_assemblyMeasureTriBufferUploaded = false;
    m_assemblyMeasureValueBufferUploaded = false;
  }

  const bool prevShowWipeTower = m_showWipeTower;
  const float prevWipeTowerWidth = m_wipeTowerWidth;
  const float prevWipeTowerDepth = m_wipeTowerDepth;
  const float prevWipeTowerHeight = m_wipeTowerHeight;
  const float prevWipeTowerX = m_wipeTowerX;
  const float prevWipeTowerZ = m_wipeTowerZ;
  const bool prevWipeTowerHasRealMesh = m_wipeTowerHasRealMesh;
  const std::vector<float> prevWipeTowerMeshVertices = m_wipeTowerMeshVertices;
  m_showWipeTower = viewport->m_showWipeTower;
  m_wipeTowerWidth = viewport->m_wipeTowerWidth;
  m_wipeTowerDepth = viewport->m_wipeTowerDepth;
  m_wipeTowerHeight = viewport->m_wipeTowerHeight;
  m_wipeTowerX = viewport->m_wipeTowerX;
  m_wipeTowerZ = viewport->m_wipeTowerZ;
  // Phase 109 (WTMESH-05): pull the Option B real-mesh state from the viewport
  // item. The mesh vertices are flattened XYZ triples (libslic3r world frame);
  // the builder applies the upstream Y -> Qt Z transform. The dirty-flag
  // comparison uses size + content equality so a re-slice that produces the
  // same hull does NOT trigger a rebuild, but a real change does.
  m_wipeTowerHasRealMesh = viewport->m_wipeTowerHasRealMesh;
  m_wipeTowerMeshVertices = viewport->m_wipeTowerMeshVertices;
  if (m_showWipeTower != prevShowWipeTower ||
      !qFuzzyCompare(m_wipeTowerWidth, prevWipeTowerWidth) ||
      !qFuzzyCompare(m_wipeTowerDepth, prevWipeTowerDepth) ||
      !qFuzzyCompare(m_wipeTowerHeight, prevWipeTowerHeight) ||
      !qFuzzyCompare(m_wipeTowerX, prevWipeTowerX) ||
      !qFuzzyCompare(m_wipeTowerZ, prevWipeTowerZ) ||
      m_wipeTowerHasRealMesh != prevWipeTowerHasRealMesh ||
      m_wipeTowerMeshVertices != prevWipeTowerMeshVertices)
  {
    m_wipeTowerDirty = true;
    m_wipeTowerBufferUploaded = false;
  }

  // Phase 121 (PAINT-02/PAINT-03): mirror the paint overlay payload + brush
  // params from the viewport item. The payload is the EditorViewModel byte
  // stream (world-transformed painted facets); brush fields drive the sphere
  // cursor. A change to the payload forces an overlay re-upload; the brush
  // cursor re-uploads when its inputs change (handled in renderBrushCursor).
  const QByteArray prevPaintOverlay = m_paintOverlayData;
  m_paintOverlayData = viewport->m_paintOverlayData;
  m_extrudersColors = viewport->m_extrudersColors;
  m_brushRadius = viewport->m_brushRadius;
  m_brushCursorType = viewport->m_brushCursorType;
  m_paintState = viewport->m_paintState;
  m_brushMouseScreenX = viewport->m_brushMouseScreenX;
  m_brushMouseScreenY = viewport->m_brushMouseScreenY;
  m_brushButtonState = viewport->m_brushButtonState;
  if (m_paintOverlayData != prevPaintOverlay)
    m_paintOverlayBufferUploaded = false;

  // Render-side per-role visibility mask (no repack). The viewport carries a
  // 20-element QVariantList of bools indexed by canonical libvgcode role; convert
  // to QVector<bool> for the draw-range skip check. Missing entries default visible.
  if (viewport->m_roleVisibility.size() >= 20)
  {
    m_roleVisibility.resize(20);
    for (int i = 0; i < 20; ++i)
      m_roleVisibility[i] = viewport->m_roleVisibility.at(i).toBool();
  }
  else
  {
    m_roleVisibility.clear();
  }

  // Phase 95 (THUMBCAP-03): mirror the thumbnail capture request from the item
  // (GUI thread) to the renderer. Mirrors the m_fitRequestCount/m_viewPreset
  // pattern: requestThumbnailCapture sets the flag + plateIndex/size + update()
  // on the GUI thread; synchronize() copies them here and clears the item-side
  // flag so the request does not re-fire every frame. The request is now
  // "owned" by the renderer until the readback completes.
  if (viewport->m_thumbnailRequestPending) {
    m_thumbnailRequestPending = true;
    m_thumbnailPlateIndex = viewport->m_thumbnailPlateIndex;
    m_thumbnailSize = viewport->m_thumbnailSize;
    viewport->m_thumbnailRequestPending = false;
  }
  // Cache the item pointer for the queued QImage callback + follow-up update().
  // QPointer survives item recreation and nulls itself if the item is destroyed
  // before the readback completes.
  m_viewportItem = viewport;
}

void RhiViewportRenderer::render(QRhiCommandBuffer *cb)
{
  if (cb == nullptr || rhi() == nullptr || renderTarget() == nullptr) {
    return;
  }

  // Phase 95 (THUMBCAP-01/03): poll the async thumbnail readback at the START
  // of render(), before the on-screen pass. readBackTexture completes on a
  // later frame; when m_thumbnailReadbackResult.data becomes non-empty the
  // QImage is ready and is posted back to the GUI thread. Since Phase 95 REVIEW
  // W-2 the thumbnail pass uses a dedicated uniform buffer, so the on-screen
  // camera UBO is never touched by a capture — no MVP restore is needed here.
  if (m_thumbnailReadbackInFlight && !m_thumbnailReadbackResult.data.isEmpty()) {
    m_thumbnailReadbackInFlight = false;
    deliverCompletedThumbnail();
  }

  QRhiResourceUpdateBatch *updates = nullptr;
  quint32 dirtyFlags = m_prepareScene.peekDirtyFlags();
  // Phase 90: CanvasAssembleView reuses the View3D mesh-render path (basic
  // mesh render proves the canvas host). Guard widened to != CanvasPreview so
  // both CanvasView3D and CanvasAssembleView upload scene/gizmo buffers;
  // CanvasPreview keeps its own segment upload block below. Mirrors the
  // upstream CanvasAssembleView render branch on selection change
  // (Plater.cpp:7322). Explosion-driven separation rendering is Phase 91.
  if (m_canvasType != RhiViewport::CanvasPreview) {
    const bool sceneDirty = (dirtyFlags & (PrepareSceneData::DirtyBed
                                           | PrepareSceneData::DirtyPlate
                                           | PrepareSceneData::DirtyMesh
                                           | PrepareSceneData::DirtySelection
                                           | PrepareSceneData::DirtyCamera
                                           | PrepareSceneData::DirtyGpu)) != 0;
    if ((sceneDirty || !m_sceneBuffersUploaded) && !m_pipelineFailed) {
      updates = rhi()->nextResourceUpdateBatch();
      if (!uploadSceneBuffers(updates, dirtyFlags)) {
        delete updates;
        updates = nullptr;
      } else {
        // Phase 68: upload the gizmo vertex buffer in the same batch.
        uploadGizmoBuffer(updates);
        m_prepareScene.takeDirtyFlags();
      }
    }
    else if (!m_pipelineFailed && (m_cutPlaneDirty || m_wipeTowerDirty ||
                                   !m_cutPlaneFillBufferUploaded ||
                                   !m_wipeTowerBufferUploaded ||
                                   !m_paintOverlayBufferUploaded ||
                                   !m_brushCursorBufferUploaded)) {
      updates = rhi()->nextResourceUpdateBatch();
      if (!uploadCutPlaneBuffers(updates, dirtyFlags) ||
          !uploadWipeTowerBuffer(updates))
      {
        delete updates;
        updates = nullptr;
      }
      else
      {
        // Phase 121 (PAINT-02/PAINT-03): paint overlay + brush cursor uploads.
        // These reuse the same batch; failure of either is non-fatal (the draw
        // is simply skipped). uploadPaintOverlayBuffer parses the byte stream;
        // uploadBrushCursorBuffer rebuilds the sphere when inputs change.
        uploadPaintOverlayBuffer(updates);
        uploadBrushCursorBuffer(updates);
      }
    }
  }

  // ── Phase 26/28: Preview segment buffer + camera uniform upload (before beginPass) ──
  // BUG-V31-1 fix: camera uniform MUST be uploaded before beginPass, not after.
  // beginPass-after-resourceUpdate is undefined in QRhi; D3D12 strictly enforces
  // command buffer ordering and segfaults on this pattern (root cause of the
  // D3D12 crash that was worked around with D3D11-first in RhiBackendSelector).
  if (m_canvasType == RhiViewport::CanvasPreview) {
    if (!updates)
      updates = rhi()->nextResourceUpdateBatch();
    // Segment buffer upload
    if (!m_previewSegmentBufferUploaded && !m_previewVertices.isEmpty()) {
      if (uploadPreviewSegmentBuffer(updates))
        m_previewSegmentBufferUploaded = true;
    }
    // Camera uniform upload (merged into the same pre-beginPass batch)
    uploadCameraUniform(updates, PrepareSceneData::DirtyCamera);
  }

  cb->beginPass(renderTarget(), m_clearColor, {1.0f, 0}, updates);
  // Phase 90: CanvasAssembleView shares the View3D mesh draw block (bed +
  // model vertex buffer) so the AssembleView canvas is not empty at runtime.
  // Guard widened to != CanvasPreview; the CanvasPreview draw block below
  // stays strictly == CanvasPreview. Mirrors Plater.cpp:7322.
  if (m_canvasType != RhiViewport::CanvasPreview && ensurePipelines()) {
    cb->setViewport(QRhiViewport(0, 0, float(renderTarget()->pixelSize().width()),
                                 float(renderTarget()->pixelSize().height())));
    cb->setShaderResources(m_srb.get());
    if (m_prepareScene.showBed() && m_bedFillBuffer && m_bedFillVertexCount > 0) {
      cb->setGraphicsPipeline(m_fillPipeline.get());
      const QRhiCommandBuffer::VertexInput fillBinding(m_bedFillBuffer.get(), 0);
      cb->setVertexInput(0, 1, &fillBinding);
      cb->draw(m_bedFillVertexCount);
    }
    if (m_prepareScene.showBed() && m_bedLineBuffer && m_bedLineVertexCount > 0) {
      cb->setGraphicsPipeline(m_linePipeline.get());
      const QRhiCommandBuffer::VertexInput lineBinding(m_bedLineBuffer.get(), 0);
      cb->setVertexInput(0, 1, &lineBinding);
      cb->draw(m_bedLineVertexCount);
    }
    if (m_modelVertexBuffer && m_modelVertexCount > 0) {
      cb->setGraphicsPipeline(m_fillPipeline.get());
      const QRhiCommandBuffer::VertexInput modelBinding(m_modelVertexBuffer.get(), 0);
      cb->setVertexInput(0, 1, &modelBinding);
      cb->draw(m_modelVertexCount);
    }
    // Phase 121 (PAINT-02/OV-03): render the painted-facet overlay after the
    // model mesh, before highlight. Reuses m_fillPipeline (opaque vertex-color
    // fill). Gated to the three paint gizmos (Support=6, Seam=7, MMU=10).
    renderPaintOverlay(cb);
  if (m_highlightVertexBuffer && m_highlightVertexCount > 0) {
      // Highlight is translucent: test depth but do not write it, so it does
      // not occlude opaque geometry drawn in subsequent frames/passes.
      cb->setGraphicsPipeline(m_translucentFillPipeline.get());
      const QRhiCommandBuffer::VertexInput highlightBinding(m_highlightVertexBuffer.get(), 0);
      cb->setVertexInput(0, 1, &highlightBinding);
      cb->draw(m_highlightVertexCount);
    }
    renderWipeTower(cb);
    renderCutPlane(cb);
    // Phase 91 (ASMEXPLODE-02): yellow dashed connector guide lines on
    // AssembleView only, when ratio > 1.0 (matches 装配页_爆炸.png).
    if (m_canvasType == RhiViewport::CanvasAssembleView
        && m_explosionRatio > 1.0f + std::numeric_limits<float>::epsilon()
        && m_assemblyConnectorBuffer && m_assemblyConnectorVertexCount > 0)
    {
      renderAssemblyConnectors(cb);
    }
    // Phase 92 (ASMMEASURE-02): Assembly measurement overlay (white dashed
    // dimension line + arrowheads + teal value box) on AssembleView only, when
    // the Assembly measure gizmo is active (mode 19). Matches 装配页_测量.png.
    if (m_canvasType == RhiViewport::CanvasAssembleView
        && m_gizmoMode == 19 /*GizmoAssemblyMeasure*/)
    {
      renderAssemblyMeasureOverlay(cb);
    }
    // Phase 68: render the move gizmo (X/Y/Z arrows) when gizmoMode == Move.
    // Drawn after meshes/highlight so it sits on top via no-depth-write.
    renderMoveGizmo(cb);
    renderRotateGizmo(cb);
    renderScaleGizmo(cb);
    // Phase 121 (PAINT-03/OV-05): translucent brush sphere cursor. Drawn last
    // so it sits on top via no-depth-write (m_translucentFillPipeline). Gated
    // to the three paint gizmos; hidden when brushButtonState < 0.
    renderBrushCursor(cb);
  }

  // ── Phase 26/27: Preview segment rendering + timing (CanvasPreview branch) ──
  if (m_canvasType == RhiViewport::CanvasPreview && ensurePipelines()
      && m_previewSegmentBuffer && m_previewSegmentVertexCount > 0) {
    m_previewFrameTimer.start();
    // Camera uniform was already uploaded before beginPass (BUG-V31-1 fix).
    cb->setViewport(QRhiViewport(0, 0, float(renderTarget()->pixelSize().width()),
                                 float(renderTarget()->pixelSize().height())));
    cb->setShaderResources(m_srb.get());
    cb->setGraphicsPipeline(m_linePipeline.get());

    const QVector<PreviewDrawRange> drawRanges = computePreviewDrawRanges();
    if (!drawRanges.isEmpty()) {
      const QRhiCommandBuffer::VertexInput segBinding(m_previewSegmentBuffer.get(), 0);
      cb->setVertexInput(0, 1, &segBinding);
      for (const PreviewDrawRange &range : drawRanges) {
        if (range.vertexCount > 0)
          cb->draw(range.vertexCount, 1, range.firstVertex);
      }
    }

    // Phase 27 (PERF-01): capture Preview frame timing.
    m_previewLastFrameMs = m_previewFrameTimer.elapsed();
    if (!m_previewFirstFrameDone) {
      m_previewFirstFrameMs = m_previewLastFrameMs;
      m_previewFirstFrameDone = true;
    }
    qInfo("[RHI-PERF] preview frame=%lldms first=%lldms upload=%lldms segments=%u",
          m_previewLastFrameMs, m_previewFirstFrameMs,
          m_previewLastUploadMs, m_previewSegmentVertexCount / 2);
  }

  cb->endPass();

  // Phase 95 (THUMBCAP-01/02/03): offscreen thumbnail pass runs AFTER the
  // on-screen pass completes, as a second beginPass/endPass pair on the same
  // command buffer. Thumbnail capture is a View3D/AssembleView feature
  // (Preview thumbnails are out of scope). The readback is issued on the
  // offscreen pass's resource-update batch and completes on a later frame.
  if (m_thumbnailRequestPending && !m_thumbnailReadbackInFlight
      && m_canvasType != RhiViewport::CanvasPreview
      && m_cameraUniformBuffer != nullptr && m_srb != nullptr
      && m_thumbnailSize > 0
      && ensureThumbnailRenderTarget(m_thumbnailSize))
  {
    renderThumbnailPass(cb);
    // Issue the readback on a fresh batch merged into the next beginPass of
    // the on-screen RT — but since the offscreen pass just ended, use a
    // resource-update batch that the QRhi processes as part of this frame's
    // command stream. The readback result lands in m_thumbnailReadbackResult
    // on a subsequent frame.
    QRhiResourceUpdateBatch *readbackUpdates = rhi()->nextResourceUpdateBatch();
    issueThumbnailReadback(readbackUpdates);
    // Merge the readback into the on-screen RT by ending the frame with it
    // pending: QRhi processes resource-update batches via
    // cb->resourceUpdate() — issue it directly here so the readback is
    // scheduled before the frame ends.
    cb->resourceUpdate(readbackUpdates);
    m_thumbnailReadbackInFlight = true;
    m_thumbnailResultPlateIndex = m_thumbnailPlateIndex;
    m_thumbnailResultSize = m_thumbnailSize;
    m_thumbnailRequestPending = false;
    // Request a follow-up frame so the async readback result is polled on the
    // next render(). Without this, a single capture on an otherwise-idle
    // scene would never deliver the QImage.
    if (m_viewportItem != nullptr) {
      QMetaObject::invokeMethod(m_viewportItem.data(),
                                "update",
                                Qt::QueuedConnection);
    }
  }
}

void RhiViewportRenderer::releaseResources()
{
  // Phase 95 (THUMBCAP-01): release offscreen thumbnail RT + pipelines +
  // pending readback state before the on-screen resources are torn down.
  releaseThumbnailResources();
  m_thumbnailRequestPending = false;
  m_linePipeline.reset();
  m_fillPipeline.reset();
  m_translucentFillPipeline.reset();
  m_translucentLinePipeline.reset();
  // Phase 68: gizmo pipelines + vertex buffer.
  m_gizmoLinePipeline.reset();
  m_gizmoTriPipeline.reset();
  m_gizmoVertexBuffer.reset();
  m_cutPlaneFillBuffer.reset();
  m_cutPlaneOutlineBuffer.reset();
  m_wipeTowerBuffer.reset();
  m_paintOverlayBuffer.reset();   // Phase 121 (PAINT-02)
  m_brushCursorBuffer.reset();    // Phase 121 (PAINT-03)
  m_assemblyConnectorBuffer.reset();  // Phase 91
  m_srb.reset();
  m_cameraUniformBuffer.reset();
  m_highlightVertexBuffer.reset();
  m_modelVertexBuffer.reset();
  m_bedLineBuffer.reset();
  m_bedFillBuffer.reset();
  resetPreviewGpuState(true);
  m_renderPassDescriptor = nullptr;
  m_sceneBuffersUploaded = false;
  m_modelVertexBufferUploaded = false;
  m_highlightVertexBufferUploaded = false;
  m_cameraUniformBufferUploaded = false;
  m_gizmoVertexBufferUploaded = false;       // Phase 68
  m_cutPlaneFillBufferUploaded = false;
  m_cutPlaneOutlineBufferUploaded = false;
  m_wipeTowerBufferUploaded = false;
  m_paintOverlayBufferUploaded = false;   // Phase 121 (PAINT-02)
  m_brushCursorBufferUploaded = false;    // Phase 121 (PAINT-03)
  m_assemblyConnectorBufferUploaded = false;  // Phase 91
  m_gizmoPipelineCreated = false;            // Phase 68
  m_bedFillBufferBytes = 0;
  m_bedLineBufferBytes = 0;
  m_modelVertexBufferBytes = 0;
  m_highlightVertexBufferBytes = 0;
  m_cameraUniformBufferBytes = 0;
  m_gizmoVertexBufferBytes = 0;              // Phase 68
  m_cutPlaneFillBufferBytes = 0;
  m_cutPlaneOutlineBufferBytes = 0;
  m_wipeTowerBufferBytes = 0;
  m_paintOverlayBufferBytes = 0;   // Phase 121 (PAINT-02)
  m_brushCursorBufferBytes = 0;    // Phase 121 (PAINT-03)
  m_moveGizmoOffsets = {};
  m_rotateGizmoOffsets = {};
  m_scaleGizmoOffsets = {};
  m_bedFillVertexCount = 0;
  m_bedLineVertexCount = 0;
  m_modelVertexCount = 0;
  m_highlightVertexCount = 0;
  m_cutPlaneFillVertexCount = 0;
  m_cutPlaneOutlineVertexCount = 0;
  m_wipeTowerVertexCount = 0;
  m_paintOverlayVertexCount = 0;   // Phase 121 (PAINT-02)
  m_brushCursorVertexCount = 0;    // Phase 121 (PAINT-03)
  m_sceneGeneration = 0;
  m_modelGeneration = 0;
  m_cutPlaneDirty = true;
  m_wipeTowerDirty = true;
}

// ===========================================================================
// Phase 95 (THUMBCAP-01/02/03): offscreen thumbnail capture infrastructure.
// Replaces the RhiViewport::requestThumbnailCapture solid-color stub with a
// real QRhi texture readback. The thumbnail RT is single-sample (frozen
// decision 2: no MSAA resolve), sized to the requested thumbnail dimensions
// (frozen decision 1: Option A offscreen RT), and rendered AFTER the on-screen
// pass completes. The readback is async (completes on a later frame), so the
// renderer polls the result at the start of render() and delivers the QImage
// back to the item via a queued callback (frozen decision 3: synchronize()
// queue pattern + queued signal).
// ===========================================================================
void RhiViewportRenderer::releaseThumbnailResources()
{
  m_thumbnailFillPipeline.reset();
  m_thumbnailLinePipeline.reset();
  m_thumbnailSrb.reset();
  m_thumbnailUniformBuffer.reset();
  m_thumbnailUniformBufferBytes = 0;
  if (m_thumbnailRenderPassDescriptor != nullptr) {
    m_thumbnailRenderPassDescriptor->deleteLater();
    m_thumbnailRenderPassDescriptor = nullptr;
  }
  m_thumbnailRenderTarget.reset();
  m_thumbnailTexture.reset();
  m_thumbnailReadbackInFlight = false;
  m_thumbnailLastBuiltSize = 0;
}

bool RhiViewportRenderer::ensureThumbnailRenderTarget(int size)
{
  if (rhi() == nullptr)
    return false;
  // Already built for this size: reuse.
  if (m_thumbnailRenderTarget && m_thumbnailLastBuiltSize == size)
    return true;

  // Tear down any previous-size RT before rebuilding.
  releaseThumbnailResources();

  // Phase 95 (THUMBCAP-02): single-sample (sample count 1) offscreen texture.
  // No multisample flag, no resolve step. RGBA8 matches the QImage the
  // readback produces.
  m_thumbnailTexture.reset(rhi()->newTexture(QRhiTexture::RGBA8,
                                             QSize(size, size),
                                             /*sampleCount=*/1,
                                             QRhiTexture::RenderTarget));
  if (!m_thumbnailTexture || !m_thumbnailTexture->create()) {
    releaseThumbnailResources();
    return false;
  }

  // Offscreen RT needs its OWN QRhiRenderPassDescriptor (the on-screen
  // renderTarget()->renderPassDescriptor() is not compatible with the
  // thumbnail texture format).
  QRhiTextureRenderTargetDescription desc(QRhiColorAttachment(m_thumbnailTexture.get()));
  m_thumbnailRenderTarget.reset(rhi()->newTextureRenderTarget(desc));
  m_thumbnailRenderPassDescriptor = m_thumbnailRenderTarget->newCompatibleRenderPassDescriptor();
  m_thumbnailRenderTarget->setRenderPassDescriptor(m_thumbnailRenderPassDescriptor);
  if (!m_thumbnailRenderTarget->create()) {
    releaseThumbnailResources();
    return false;
  }

  // Build the thumbnail pipelines reusing the SAME .qsb shaders and vertex
  // input layout as the on-screen m_fillPipeline/m_linePipeline, but bound to
  // the thumbnail RPD. They are separate instances because the render-pass
  // descriptors differ (cannot reuse on-screen pipelines for the offscreen RT).
  QShader vertexShader = loadShader(QStringLiteral(":/rhi_viewport/shaders/rhi_viewport.vert.qsb"));
  QShader fragmentShader = loadShader(QStringLiteral(":/rhi_viewport/shaders/rhi_viewport.frag.qsb"));
  if (!vertexShader.isValid() || !fragmentShader.isValid()) {
    releaseThumbnailResources();
    return false;
  }

  QRhiVertexInputLayout inputLayout;
  inputLayout.setBindings({QRhiVertexInputBinding(sizeof(Vertex))});
  inputLayout.setAttributes({
      QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, offsetof(Vertex, x)),
      QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float4, offsetof(Vertex, r)),
  });

  // Dedicated uniform buffer + SRB for the thumbnail pass (Phase 95 REVIEW W-2).
  // Same 256-byte aligned Dynamic layout as the on-screen m_cameraUniformBuffer,
  // but a separate instance so the thumbnail pass never overwrites the on-screen
  // camera UBO. The thumbnail pipelines bind this SRB, not m_srb.
  if (!ensureBuffer(m_thumbnailUniformBuffer,
                    256,  // 256-byte aligned for D3D12 cbuffer (matches on-screen)
                    m_thumbnailUniformBufferBytes,
                    QRhiBuffer::UniformBuffer)) {
    releaseThumbnailResources();
    return false;
  }
  m_thumbnailSrb.reset(rhi()->newShaderResourceBindings());
  m_thumbnailSrb->setBindings({
      QRhiShaderResourceBinding::uniformBuffer(0,
                                               QRhiShaderResourceBinding::VertexStage,
                                               m_thumbnailUniformBuffer.get())
  });
  if (!m_thumbnailSrb->create()) {
    releaseThumbnailResources();
    return false;
  }

  auto buildOne = [&](std::unique_ptr<QRhiGraphicsPipeline> &pipe,
                      QRhiGraphicsPipeline::Topology topology) -> bool {
    pipe.reset(rhi()->newGraphicsPipeline());
    pipe->setTopology(topology);
    pipe->setShaderStages({
        QRhiShaderStage(QRhiShaderStage::Vertex, vertexShader),
        QRhiShaderStage(QRhiShaderStage::Fragment, fragmentShader),
    });
    // Bind the dedicated thumbnail SRB (Phase 95 REVIEW W-2): the thumbnail
    // pipelines read the thumbnail-aspect MVP from m_thumbnailUniformBuffer,
    // not the shared on-screen camera UBO.
    pipe->setShaderResourceBindings(m_thumbnailSrb.get());
    pipe->setVertexInputLayout(inputLayout);
    pipe->setRenderPassDescriptor(m_thumbnailRenderPassDescriptor);
    pipe->setDepthTest(true);
    pipe->setDepthWrite(true);
    pipe->setTargetBlends({});
    if (!pipe->create()) {
      pipe.reset();
      return false;
    }
    return true;
  };

  if (!buildOne(m_thumbnailFillPipeline, QRhiGraphicsPipeline::Triangles)
      || !buildOne(m_thumbnailLinePipeline, QRhiGraphicsPipeline::Lines)) {
    releaseThumbnailResources();
    return false;
  }

  m_thumbnailLastBuiltSize = size;
  return true;
}

void RhiViewportRenderer::renderThumbnailPass(QRhiCommandBuffer *cb)
{
  // Phase 95 (THUMBCAP-01): render the SAME scene (bed fill + grid lines +
  // model mesh) as the on-screen View3D/AssembleView pass, but into the
  // offscreen thumbnail RT. Reuses the on-screen vertex buffers + SRB.
  // Thumbnail capture is a View3D/AssembleView feature; Preview thumbnails are
  // out of scope for this phase (gated by the m_canvasType != CanvasPreview
  // check in the caller).
  if (cb == nullptr || m_thumbnailRenderTarget == nullptr)
    return;

  // Re-upload the camera uniform with the thumbnail aspect ratio. The
  // thumbnail is square so aspect = 1.0 (no distortion). Use the item's
  // cameraMvp(1.0f) so the offscreen pass does not inherit the on-screen
  // viewport's wide aspect ratio.
  const QMatrix4x4 thumbnailMvp = (m_viewportItem != nullptr)
      ? m_viewportItem->cameraMvp(1.0f)
      : m_cameraMvp;

  QRhiResourceUpdateBatch *thumbUpdates = rhi()->nextResourceUpdateBatch();
  // Upload the thumbnail-aspect MVP into the DEDICATED thumbnail uniform buffer
  // (Phase 95 REVIEW W-2). This leaves the on-screen m_cameraUniformBuffer
  // untouched, so no next-frame refresh is needed to restore the on-screen MVP.
  // Only the 64-byte MVP is written; the tail is zero-initialized/irrelevant
  // for the thumbnail (no gizmos drawn).
  const QMatrix4x4 corrected = rhi()->clipSpaceCorrMatrix() * thumbnailMvp;
  thumbUpdates->updateDynamicBuffer(m_thumbnailUniformBuffer.get(), 0, 64,
                                    corrected.constData());

  cb->beginPass(m_thumbnailRenderTarget.get(), m_clearColor, {1.0f, 0}, thumbUpdates);
  cb->setViewport(QRhiViewport(0, 0, float(m_thumbnailLastBuiltSize),
                               float(m_thumbnailLastBuiltSize)));
  cb->setShaderResources(m_thumbnailSrb.get());

  // Bed fill (triangles).
  if (m_prepareScene.showBed() && m_bedFillBuffer && m_bedFillVertexCount > 0) {
    cb->setGraphicsPipeline(m_thumbnailFillPipeline.get());
    const QRhiCommandBuffer::VertexInput fillBinding(m_bedFillBuffer.get(), 0);
    cb->setVertexInput(0, 1, &fillBinding);
    cb->draw(m_bedFillVertexCount);
  }
  // Bed grid lines.
  if (m_prepareScene.showBed() && m_bedLineBuffer && m_bedLineVertexCount > 0) {
    cb->setGraphicsPipeline(m_thumbnailLinePipeline.get());
    const QRhiCommandBuffer::VertexInput lineBinding(m_bedLineBuffer.get(), 0);
    cb->setVertexInput(0, 1, &lineBinding);
    cb->draw(m_bedLineVertexCount);
  }
  // Model mesh (triangles) — the actual rendered scene, not just a clear color.
  if (m_modelVertexBuffer && m_modelVertexCount > 0) {
    cb->setGraphicsPipeline(m_thumbnailFillPipeline.get());
    const QRhiCommandBuffer::VertexInput modelBinding(m_modelVertexBuffer.get(), 0);
    cb->setVertexInput(0, 1, &modelBinding);
    cb->draw(m_modelVertexCount);
  }

  cb->endPass();
}

void RhiViewportRenderer::issueThumbnailReadback(QRhiResourceUpdateBatch *updates)
{
  // Phase 95 (THUMBCAP-01): issue the async readback. readBackTexture completes
  // on a later frame; the result is polled in render() via
  // m_thumbnailReadbackResult.data becoming non-empty.
  if (updates == nullptr || m_thumbnailTexture == nullptr)
    return;
  m_thumbnailReadbackResult = {};
  QRhiReadbackDescription rb(m_thumbnailTexture.get());
  updates->readBackTexture(rb, &m_thumbnailReadbackResult);
}

void RhiViewportRenderer::deliverCompletedThumbnail()
{
  // Phase 95 (THUMBCAP-01/03): construct the QImage from the completed
  // readback result and post it to the GUI thread via a queued
  // QMetaObject::invokeMethod to RhiViewport::deliverThumbnail. The QRhi
  // readback produces raw RGBA8 pixel bytes in m_thumbnailReadbackResult.data
  // (format == QRhiTexture::RGBA8); wrap them into a QImage.
  if (m_viewportItem == nullptr)
    return;

  const QSize size(m_thumbnailResultSize, m_thumbnailResultSize);
  if (m_thumbnailReadbackResult.data.isEmpty() || size.isEmpty())
    return;

  // QRhiReadbackResult.format is QRhiTexture::Format. The thumbnail RT is
  // RGBA8, so the bytes are 4 bytes/pixel in R,G,B,A order. QImage's
  // Format_RGBA8888 matches that byte order exactly (no swizzle needed).
  // The QImage is constructed as a non-owning view over the readback buffer,
  // then copy()'d into a deep copy with its own pixel buffer in the SAME
  // expression. This guarantees the detached copy is the only thing handed
  // to the queued invokeMethod, so a future readback (or any mutation of
  // m_thumbnailReadbackResult.data) can never reach the delivered image.
  // (Phase 95 REVIEW W-1 hardening.)
  QImage image = QImage(reinterpret_cast<const uchar *>(m_thumbnailReadbackResult.data.constData()),
                        size.width(), size.height(), size.width() * 4,
                        QImage::Format_RGBA8888).copy();

  const int plateIndex = m_thumbnailResultPlateIndex;
  QMetaObject::invokeMethod(m_viewportItem.data(),
                            "deliverThumbnail",
                            Qt::QueuedConnection,
                            Q_ARG(QImage, image),
                            Q_ARG(int, plateIndex));
}

void RhiViewportRenderer::resetPreviewGpuState(bool keepCpuStaging)
{
  m_previewSegmentBuffer.reset();
  m_previewSegmentBufferBytes = 0;
  m_previewSegmentBufferUploaded = false;
  m_previewLastUploadMs = -1;
  m_previewLastFrameMs = -1;
  m_previewFirstFrameMs = -1;
  m_previewFirstFrameDone = false;

  if (!keepCpuStaging) {
    m_previewVertices.clear();
    m_previewDrawSpans.clear();
    m_previewSegmentVertexCount = 0;
    m_previewRangeCacheKey = 0;
    m_cachedPreviewRanges.clear();
  }
}

bool RhiViewportRenderer::ensurePipelines()
{
  if (m_fillPipeline && m_linePipeline && m_translucentFillPipeline
      && m_translucentLinePipeline)
    return true;
  if (m_pipelineFailed || rhi() == nullptr || renderTarget() == nullptr)
    return false;

  if (!m_cameraUniformBuffer
      && !ensureBuffer(m_cameraUniformBuffer,
                       256,  // 256-byte aligned for D3D12 cbuffer (see uploadCameraUniform)
                       m_cameraUniformBufferBytes,
                       QRhiBuffer::UniformBuffer)) {
    m_cameraUniformBufferBytes = 0;
    m_pipelineFailed = true;
    return false;
  }

  if (!m_srb)
  {
    m_srb.reset(rhi()->newShaderResourceBindings());
    m_srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0,
                                                 QRhiShaderResourceBinding::VertexStage,
                                                 m_cameraUniformBuffer.get())
    });
    if (!m_srb->create()) {
      m_pipelineFailed = true;
      return false;
    }
  }

  return ensurePipeline(m_fillPipeline, QRhiGraphicsPipeline::Triangles)
      && ensurePipeline(m_linePipeline, QRhiGraphicsPipeline::Lines)
      && ensurePipeline(m_translucentFillPipeline, QRhiGraphicsPipeline::Triangles,
                        /*enableDepthWrite=*/false,
                        /*enableBlending=*/true)
      && ensurePipeline(m_translucentLinePipeline, QRhiGraphicsPipeline::Lines,
                        /*enableDepthWrite=*/false,
                        /*enableBlending=*/true);
}

bool RhiViewportRenderer::ensurePipeline(std::unique_ptr<QRhiGraphicsPipeline> &pipeline,
                                         QRhiGraphicsPipeline::Topology topology,
                                         bool enableDepthWrite,
                                         bool enableBlending)
{
  if (pipeline)
    return true;
  if (m_pipelineFailed || rhi() == nullptr || renderTarget() == nullptr || m_srb == nullptr)
    return false;

  QShader vertexShader = loadShader(QStringLiteral(":/rhi_viewport/shaders/rhi_viewport.vert.qsb"));
  QShader fragmentShader = loadShader(QStringLiteral(":/rhi_viewport/shaders/rhi_viewport.frag.qsb"));
  if (!vertexShader.isValid() || !fragmentShader.isValid()) {
    m_pipelineFailed = true;
    return false;
  }

  QRhiVertexInputLayout inputLayout;
  inputLayout.setBindings({QRhiVertexInputBinding(sizeof(Vertex))});
  inputLayout.setAttributes({
      QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, offsetof(Vertex, x)),
      QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float4, offsetof(Vertex, r)),
  });

  m_renderPassDescriptor = renderTarget()->renderPassDescriptor();
  pipeline.reset(rhi()->newGraphicsPipeline());
  pipeline->setTopology(topology);
  pipeline->setShaderStages({
      QRhiShaderStage(QRhiShaderStage::Vertex, vertexShader),
      QRhiShaderStage(QRhiShaderStage::Fragment, fragmentShader),
  });
  pipeline->setShaderResourceBindings(m_srb.get());
  pipeline->setVertexInputLayout(inputLayout);
  pipeline->setRenderPassDescriptor(m_renderPassDescriptor);
  // Depth test enabled so overlapping geometry occludes correctly instead of
  // relying purely on draw order. Requires the depth-stencil buffer that
  // QQuickRhiItem creates when sampleCount > 1 (set in RhiViewport ctor).
  // Qt 6.10 QRhi API: setDepthTest + setDepthWrite (compare op hardcoded Less).
  pipeline->setDepthTest(true);
  pipeline->setDepthWrite(enableDepthWrite);
  if (enableBlending)
  {
    QRhiGraphicsPipeline::TargetBlend enable;
    enable.enable = true;
    enable.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    enable.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    enable.srcAlpha = QRhiGraphicsPipeline::One;
    enable.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    pipeline->setTargetBlends({enable});
  }
  else
  {
    // Standard color blend (no blending for opaque scene geometry).
    pipeline->setTargetBlends({});
  }
  if (!pipeline->create()) {
    pipeline.reset();
    m_pipelineFailed = true;
    return false;
  }

  return true;
}

bool RhiViewportRenderer::uploadSceneBuffers(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags)
{
  if (updates == nullptr || rhi() == nullptr)
    return false;

  if (!uploadCameraUniform(updates, dirtyFlags))
    return false;
  if (!uploadBedBuffers(updates, dirtyFlags))
    return false;
  if (!uploadModelBuffer(updates, dirtyFlags))
    return false;
  if (!uploadHighlightBuffer(updates, dirtyFlags))
    return false;
  if (!uploadCutPlaneBuffers(updates, dirtyFlags))
    return false;
  if (!uploadWipeTowerBuffer(updates))
    return false;
  // Phase 91 (ASMEXPLODE-02): connector guide lines (AssembleView, ratio > 1.0).
  if (!uploadAssemblyConnectorBuffer(updates, dirtyFlags))
    return false;
  // Phase 92 (ASMMEASURE-02): Assembly measurement overlay (AssembleView,
  // gizmo mode 19). Upload is a no-op (empty buffers) outside that gate.
  if (!uploadAssemblyMeasureBuffers(updates, dirtyFlags))
    return false;

  m_sceneBuffersUploaded = true;
  return true;
}

bool RhiViewportRenderer::uploadBedBuffers(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags)
{
  if (updates == nullptr || rhi() == nullptr)
    return false;

  const bool uploadScene = !m_sceneBuffersUploaded
      || (dirtyFlags & (PrepareSceneData::DirtyBed
                        | PrepareSceneData::DirtyPlate
                        | PrepareSceneData::DirtyGpu)) != 0;
  if (!uploadScene)
    return true;

  const QVector<Vertex> fillVertices = buildSceneVertices(m_prepareScene.bedFillVertices());
  const QVector<Vertex> lineVertices = buildSceneVertices(m_prepareScene.bedLineVertices());
  const quint32 fillBytes = quint32(fillVertices.size() * int(sizeof(Vertex)));
  const quint32 lineBytes = quint32(lineVertices.size() * int(sizeof(Vertex)));

  if (!ensureBuffer(m_bedFillBuffer, fillBytes, m_bedFillBufferBytes, QRhiBuffer::VertexBuffer)
      || !ensureBuffer(m_bedLineBuffer, lineBytes, m_bedLineBufferBytes, QRhiBuffer::VertexBuffer))
    return false;

  m_bedFillVertexCount = quint32(fillVertices.size());
  m_bedLineVertexCount = quint32(lineVertices.size());
  if (m_bedFillBuffer && fillBytes > 0) {
    updates->uploadStaticBuffer(m_bedFillBuffer.get(),
                                0,
                                fillBytes,
                                fillVertices.constData());
  }
  if (m_bedLineBuffer && lineBytes > 0) {
    updates->uploadStaticBuffer(m_bedLineBuffer.get(),
                                0,
                                lineBytes,
                                lineVertices.constData());
  }

  return true;
}

bool RhiViewportRenderer::uploadModelBuffer(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags)
{
  if (updates == nullptr || rhi() == nullptr)
    return false;

  const bool uploadModel = !m_modelVertexBufferUploaded
      || (dirtyFlags & (PrepareSceneData::DirtyMesh
                        | PrepareSceneData::DirtyPlate
                        | PrepareSceneData::DirtyVisibility
                        | PrepareSceneData::DirtyGpu)) != 0;
  if (!uploadModel)
    return true;

  const QVector<Vertex> modelVertices = buildModelVertices(m_prepareScene.modelVertices());
  const quint32 modelBytes = quint32(modelVertices.size() * int(sizeof(Vertex)));
  if (!ensureBuffer(m_modelVertexBuffer, modelBytes, m_modelVertexBufferBytes, QRhiBuffer::VertexBuffer))
    return false;

  m_modelVertexCount = quint32(modelVertices.size());
  if (m_modelVertexBuffer && modelBytes > 0) {
    updates->uploadStaticBuffer(m_modelVertexBuffer.get(),
                                0,
                                modelBytes,
                                modelVertices.constData());
  }
  m_modelVertexBufferUploaded = true;
  return true;
}

bool RhiViewportRenderer::uploadHighlightBuffer(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags)
{
  if (updates == nullptr || rhi() == nullptr)
    return false;

  const bool uploadHighlight = !m_highlightVertexBufferUploaded
      || (dirtyFlags & (PrepareSceneData::DirtySelection
                        | PrepareSceneData::DirtyMesh
                        | PrepareSceneData::DirtyPlate
                        | PrepareSceneData::DirtyGpu)) != 0;
  if (!uploadHighlight)
    return true;

  const QVector<Vertex> highlightVertices = buildHighlightVertices();
  const quint32 highlightBytes = quint32(highlightVertices.size() * int(sizeof(Vertex)));
  if (!ensureBuffer(m_highlightVertexBuffer, highlightBytes, m_highlightVertexBufferBytes, QRhiBuffer::VertexBuffer))
    return false;

  m_highlightVertexCount = quint32(highlightVertices.size());
  if (m_highlightVertexBuffer && highlightBytes > 0) {
    updates->uploadStaticBuffer(m_highlightVertexBuffer.get(),
                                0,
                                highlightBytes,
                                highlightVertices.constData());
  }
  m_highlightVertexBufferUploaded = true;
  return true;
}

bool RhiViewportRenderer::uploadCutPlaneBuffers(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags)
{
  if (updates == nullptr || rhi() == nullptr)
    return false;

  const bool uploadCut = m_cutPlaneDirty
      || !m_cutPlaneFillBufferUploaded
      || !m_cutPlaneOutlineBufferUploaded
      || (dirtyFlags & (PrepareSceneData::DirtyMesh
                        | PrepareSceneData::DirtySelection
                        | PrepareSceneData::DirtyGpu)) != 0;
  if (!uploadCut)
    return true;

  QVector<Vertex> fillVertices;
  QVector<Vertex> outlineVertices;
  const int selectedSourceObjectIndex = m_prepareScene.selectedSourceObjectIndex();
  if ((m_gizmoMode == 5 || m_gizmoMode == 14) && selectedSourceObjectIndex >= 0)
  {
    bool found = false;
    PrepareSceneData::ModelBounds selectedBounds;
    for (const PrepareSceneData::ModelBatch &batch : m_prepareScene.modelBatches())
    {
      if (batch.sourceObjectIndex != selectedSourceObjectIndex)
        continue;

      if (!found)
      {
        selectedBounds = batch.bounds;
        found = true;
        continue;
      }

      selectedBounds.minX = std::min(selectedBounds.minX, batch.bounds.minX);
      selectedBounds.minY = std::min(selectedBounds.minY, batch.bounds.minY);
      selectedBounds.minZ = std::min(selectedBounds.minZ, batch.bounds.minZ);
      selectedBounds.maxX = std::max(selectedBounds.maxX, batch.bounds.maxX);
      selectedBounds.maxY = std::max(selectedBounds.maxY, batch.bounds.maxY);
      selectedBounds.maxZ = std::max(selectedBounds.maxZ, batch.bounds.maxZ);
    }

    if (found)
    {
      const QVector3D boundsMin(selectedBounds.minX, selectedBounds.minY, selectedBounds.minZ);
      const QVector3D boundsMax(selectedBounds.maxX, selectedBounds.maxY, selectedBounds.maxZ);
      fillVertices = GizmoGeometry::buildCutPlaneVertices(boundsMin, boundsMax,
                                                          m_cutAxis, m_cutPosition);
      outlineVertices = GizmoGeometry::buildCutPlaneOutlineVertices(boundsMin, boundsMax,
                                                                     m_cutAxis, m_cutPosition);
    }
  }

  const quint32 fillBytes = quint32(fillVertices.size() * int(sizeof(Vertex)));
  const quint32 outlineBytes = quint32(outlineVertices.size() * int(sizeof(Vertex)));
  if (!ensureBuffer(m_cutPlaneFillBuffer, fillBytes, m_cutPlaneFillBufferBytes,
                    QRhiBuffer::VertexBuffer)
      || !ensureBuffer(m_cutPlaneOutlineBuffer, outlineBytes, m_cutPlaneOutlineBufferBytes,
                       QRhiBuffer::VertexBuffer))
    return false;

  m_cutPlaneFillVertexCount = quint32(fillVertices.size());
  m_cutPlaneOutlineVertexCount = quint32(outlineVertices.size());
  if (m_cutPlaneFillBuffer && fillBytes > 0)
  {
    updates->uploadStaticBuffer(m_cutPlaneFillBuffer.get(), 0, fillBytes,
                                fillVertices.constData());
  }
  if (m_cutPlaneOutlineBuffer && outlineBytes > 0)
  {
    updates->uploadStaticBuffer(m_cutPlaneOutlineBuffer.get(), 0, outlineBytes,
                                outlineVertices.constData());
  }

  m_cutPlaneFillBufferUploaded = true;
  m_cutPlaneOutlineBufferUploaded = true;
  m_cutPlaneDirty = false;
  return true;
}

bool RhiViewportRenderer::uploadWipeTowerBuffer(QRhiResourceUpdateBatch *updates)
{
  if (updates == nullptr || rhi() == nullptr)
    return false;

  if (!m_wipeTowerDirty && m_wipeTowerBufferUploaded)
    return true;

  QVector<Vertex> vertices;
  if (m_showWipeTower)
  {
    // Phase 109 (WTMESH-04/WM-04): Option B real-mesh branch. When
    // m_wipeTowerHasRealMesh is true, the worker captured the convex hull of
    // the merged real_wipe_tower_mesh + real_brim_mesh; build the vertex
    // buffer from that mesh. Otherwise fall back to the Option A dimensioned
    // box (Phase 99 Frozen Decision 2 baseline -- UNCHANGED). The dynamic-size
    // ensureBuffer + uploadStaticBuffer path below already handles variable
    // vertex counts, so no buffer resize changes are needed here.
    if (m_wipeTowerHasRealMesh && !m_wipeTowerMeshVertices.empty())
    {
      vertices = GizmoGeometry::buildWipeTowerMeshVertices(
          m_wipeTowerMeshVertices, m_wipeTowerX, m_wipeTowerZ);
      // Defensive: if the builder returned empty (malformed capture), fall
      // back to Option A so the wipe tower still renders. The capture
      // invariant guarantees non-empty, but this guards against a future
      // regression without silently dropping the tower.
      if (vertices.isEmpty())
      {
        vertices = GizmoGeometry::buildWipeTowerVertices(m_wipeTowerX,
                                                         m_wipeTowerZ,
                                                         m_wipeTowerWidth,
                                                         m_wipeTowerDepth,
                                                         m_wipeTowerHeight);
      }
    }
    else
    {
      // Option A (Phase 99 Frozen Decision 2 baseline): dimensioned box from
      // the real sliced width/depth/height/position. This is the v4.4 path,
      // unchanged. Single-material / pre-slice / mock paths land here because
      // wipe_tower_mesh_data is std::nullopt (hasRealMesh=false).
      vertices = GizmoGeometry::buildWipeTowerVertices(m_wipeTowerX,
                                                       m_wipeTowerZ,
                                                       m_wipeTowerWidth,
                                                       m_wipeTowerDepth,
                                                       m_wipeTowerHeight);
    }
  }

  const quint32 byteSize = quint32(vertices.size() * int(sizeof(Vertex)));
  if (!ensureBuffer(m_wipeTowerBuffer, byteSize, m_wipeTowerBufferBytes,
                    QRhiBuffer::VertexBuffer))
    return false;

  m_wipeTowerVertexCount = quint32(vertices.size());
  if (m_wipeTowerBuffer && byteSize > 0)
  {
    updates->uploadStaticBuffer(m_wipeTowerBuffer.get(), 0, byteSize,
                                vertices.constData());
  }

  m_wipeTowerBufferUploaded = true;
  m_wipeTowerDirty = false;
  return true;
}

bool RhiViewportRenderer::uploadAssemblyConnectorBuffer(QRhiResourceUpdateBatch *updates,
                                                        quint32 dirtyFlags)
{
  // Phase 91 (ASMEXPLODE-02): yellow dashed connector guide lines between
  // originally-adjacent volumes of the same object, visible only when ratio > 1.0
  // on AssembleView (matches shotScreen/装配页_爆炸.png; CONTEXT.md decision 7).
  // Connectors join the ORIGINAL (pre-offset) volume centers — they stay anchored
  // where the volumes were touching and bridge the gap created by the explosion.
  if (updates == nullptr || rhi() == nullptr)
    return false;

  const bool reupload = !m_assemblyConnectorBufferUploaded
      || (dirtyFlags & (PrepareSceneData::DirtyMesh
                        | PrepareSceneData::DirtyPlate
                        | PrepareSceneData::DirtyGpu)) != 0;
  if (!reupload)
    return true;

  QVector<Vertex> vertices;
  // Connectors are meaningful only on AssembleView with ratio > 1.0. On any
  // other canvas or at ratio <= 1.0 the buffer is left empty (nothing drawn).
  if (m_canvasType == RhiViewport::CanvasAssembleView
      && m_explosionRatio > 1.0f + std::numeric_limits<float>::epsilon())
  {
    // Group volume batches by their parent object (sourceObjectIndex), keeping
    // the original modelBatches() order so "adjacent" = consecutive batches of
    // the same object (the simplest faithful definition of originally-touching
    // volumes — upstream connects volumes that were adjacent pre-explosion).
    struct VolumeCenter { int sourceObjectIndex; QVector3D center; };
    QList<VolumeCenter> centers;
    for (const PrepareSceneData::ModelBatch &batch : m_prepareScene.modelBatches())
    {
      if (batch.sourceObjectIndex < 0 || batch.vertexCount <= 0)
        continue;
      VolumeCenter vc;
      vc.sourceObjectIndex = batch.sourceObjectIndex;
      vc.center = QVector3D((batch.bounds.minX + batch.bounds.maxX) * 0.5f,
                            (batch.bounds.minY + batch.bounds.maxY) * 0.5f,
                            (batch.bounds.minZ + batch.bounds.maxZ) * 0.5f);
      centers.append(vc);
    }

    // Yellow RGBA (matches 装配页_爆炸.png). Dash effect: emit short alternating
    // dash/gap segments along each connector (avoids a new stipple shader while
    // matching the dashed visual). 6 dash segments per connector at this density.
    const float r = 1.0f;
    const float g = 0.85f;
    const float b = 0.0f;
    const float a = 1.0f;
    const int kDashCount = 6;
    for (int i = 1; i < centers.size(); ++i)
    {
      if (centers[i].sourceObjectIndex != centers[i - 1].sourceObjectIndex)
        continue;
      const QVector3D &p0 = centers[i - 1].center;
      const QVector3D &p1 = centers[i].center;
      for (int d = 0; d < kDashCount; ++d)
      {
        const float t0 = float(d) / float(kDashCount);
        const float t1 = float(d + 0.5f) / float(kDashCount);
        const QVector3D a0 = p0 + (p1 - p0) * t0;
        const QVector3D a1 = p0 + (p1 - p0) * t1;
        // Two vertices = one line segment (GL_LINES via m_linePipeline).
        vertices.append(Vertex{a0.x(), a0.y(), a0.z(), r, g, b, a});
        vertices.append(Vertex{a1.x(), a1.y(), a1.z(), r, g, b, a});
      }
    }
  }

  const quint32 byteSize = quint32(vertices.size() * int(sizeof(Vertex)));
  if (!ensureBuffer(m_assemblyConnectorBuffer, byteSize, m_assemblyConnectorBufferBytes,
                    QRhiBuffer::VertexBuffer))
    return false;

  m_assemblyConnectorVertexCount = quint32(vertices.size());
  if (m_assemblyConnectorBuffer && byteSize > 0)
  {
    updates->uploadStaticBuffer(m_assemblyConnectorBuffer.get(), 0, byteSize,
                                vertices.constData());
  }
  m_assemblyConnectorBufferUploaded = true;
  return true;
}

void RhiViewportRenderer::renderAssemblyConnectors(QRhiCommandBuffer *cb)
{
  // Phase 91 (ASMEXPLODE-02): draw the connector guide-line buffer with the
  // shared line pipeline (GL_LINES). Drawn after the mesh so connectors sit on
  // top of separated volumes (matches 装配页_爆炸.png).
  if (cb == nullptr || m_assemblyConnectorBuffer == nullptr
      || m_assemblyConnectorVertexCount == 0 || m_linePipeline == nullptr)
    return;

  cb->setShaderResources();
  cb->setGraphicsPipeline(m_linePipeline.get());
  const QRhiCommandBuffer::VertexInput binding(m_assemblyConnectorBuffer.get(), 0);
  cb->setVertexInput(0, 1, &binding);
  cb->draw(m_assemblyConnectorVertexCount);
}

bool RhiViewportRenderer::uploadAssemblyMeasureBuffers(QRhiResourceUpdateBatch *updates,
                                                       quint32 dirtyFlags)
{
  // Phase 92 (ASMMEASURE-02): Assembly measurement overlay (white dashed
  // dimension line + arrowheads + teal value box) between the two selected
  // volumes. Matches shotScreen/装配页_测量.png. The overlay is meaningful only
  // on AssembleView with the Assembly measure gizmo active (mode 19); on any
  // other canvas/gizmo the buffers are left empty (nothing drawn). Selection
  // deltas are handled by the dirty flags set in synchronize() (which clear
  // the *Uploaded flags); mesh/plate/gpu dirty also force a rebuild.
  if (updates == nullptr || rhi() == nullptr)
    return false;

  const bool reupload = !m_assemblyMeasureLineBufferUploaded
      || !m_assemblyMeasureTriBufferUploaded
      || !m_assemblyMeasureValueBufferUploaded
      || (dirtyFlags & (PrepareSceneData::DirtyMesh
                        | PrepareSceneData::DirtyPlate
                        | PrepareSceneData::DirtySelection
                        | PrepareSceneData::DirtyGpu)) != 0;
  if (!reupload)
    return true;

  // Start empty; only populate on AssembleView + gizmo mode 19 with two valid
  // selected volumes.
  QVector<Vertex> lineVerts;     // dimension line (GL_LINES)
  QVector<Vertex> triVerts;      // arrowheads (GL_TRIANGLES)
  QVector<Vertex> valueVerts;    // teal value-box quad (GL_TRIANGLES)

  const bool active = m_canvasType == RhiViewport::CanvasAssembleView
      && m_gizmoMode == 19 /*GizmoAssemblyMeasure*/;
  if (active)
  {
    // Locate the two selected volumes' first batches (mirrors uploadHighlightBuffer
    // / renderAssemblyConnectors). Use the first batch per sourceObjectIndex.
    bool foundA = false, foundB = false;
    PrepareSceneData::ModelBounds boundsA{}, boundsB{};
    for (const PrepareSceneData::ModelBatch &batch : m_prepareScene.modelBatches())
    {
      if (batch.vertexCount <= 0 || batch.sourceObjectIndex < 0)
        continue;
      if (!foundA && batch.sourceObjectIndex == m_assemblyMeasureSelectedA)
      {
        boundsA = batch.bounds;
        foundA = true;
        continue;
      }
      if (!foundB && batch.sourceObjectIndex == m_assemblyMeasureSelectedB)
      {
        boundsB = batch.bounds;
        foundB = true;
        continue;
      }
      if (foundA && foundB)
        break;
    }

    if (foundA && foundB)
    {
      const AssemblyMeasureResult result =
          AssemblyMeasureGeometry::measure(boundsA, boundsB);
      if (result.valid)
      {
        const QVector3D &p0 = result.centerA;
        const QVector3D &p1 = result.centerB;
        // ── Dimension line: white dashed segments (reuse the Phase 91 dash
        // technique — N alternating dash/gap GL_LINES segments, white RGBA).
        const float wr = 1.0f, wg = 1.0f, wb = 1.0f, wa = 1.0f;
        const int kDashCount = 8;
        for (int d = 0; d < kDashCount; ++d)
        {
          const float t0 = float(d) / float(kDashCount);
          const float t1 = float(d + 0.5f) / float(kDashCount);
          const QVector3D a0 = p0 + (p1 - p0) * t0;
          const QVector3D a1 = p0 + (p1 - p0) * t1;
          lineVerts.append(Vertex{a0.x(), a0.y(), a0.z(), wr, wg, wb, wa});
          lineVerts.append(Vertex{a1.x(), a1.y(), a1.z(), wr, wg, wb, wa});
        }

        // ── Arrowheads: a small white triangle at each endpoint pointing along
        // the line. World-space approximation (a faithful screen-space arrow
        // like upstream render_dimensioning needs the MVP; this approximation
        // is documented in the plan and is acceptable for Phase 92). Build two
        // side vertices perpendicular to the line in world space.
        QVector3D dir = (p1 - p0);
        const float len = dir.length();
        if (len > 1e-5f)
        {
          dir /= len;
          // Pick a vector not parallel to dir for the perpendicular basis.
          QVector3D up = (std::abs(dir.y()) < 0.9f) ? QVector3D(0, 1, 0) : QVector3D(1, 0, 0);
          QVector3D perp = QVector3D::crossProduct(dir, up).normalized();
          const float head = std::clamp(len * 0.08f, 1.0f, 8.0f);  // arrow size (mm)
          const float half = head * 0.5f;
          // Tip at p0 - dir*head (points outward from A along the line toward B
          // reversed), base two side vertices. Symmetric for B.
          auto appendArrow = [&](const QVector3D &tip, const QVector3D &lineDir) {
            const QVector3D base = tip + lineDir * head;
            const QVector3D s1 = base + perp * half;
            const QVector3D s2 = base - perp * half;
            triVerts.append(Vertex{tip.x(), tip.y(), tip.z(), wr, wg, wb, wa});
            triVerts.append(Vertex{s1.x(), s1.y(), s1.z(), wr, wg, wb, wa});
            triVerts.append(Vertex{s2.x(), s2.y(), s2.z(), wr, wg, wb, wa});
          };
          appendArrow(p0, dir);    // arrow at A pointing toward B
          appendArrow(p1, -dir);   // arrow at B pointing toward A
        }

        // ── Teal value box: a small translucent teal quad at the midpoint,
        // drawn behind the value text (the value text itself renders in the
        // QML panel — the box is a visual anchor per the screenshot). Teal
        // #0fb-family (0.0, 0.73, 0.73) with alpha 0.85.
        const QVector3D mid = (p0 + p1) * 0.5f;
        const float tr = 0.0f, tg = 0.73f, tb = 0.73f, ta = 0.85f;
        // Size the box in world mm (a few mm each side). Orient facing the
        // camera by building a billboard-ish quad in the plane spanned by `dir`
        // and `up` — good enough for the screenshot visual.
        const float boxHalf = 2.5f;
        QVector3D boxUp = (std::abs(dir.y()) < 0.9f) ? QVector3D(0, 1, 0) : QVector3D(1, 0, 0);
        QVector3D boxPerp = QVector3D::crossProduct(dir, boxUp).normalized();
        const QVector3D c0 = mid - boxPerp * boxHalf - boxUp * boxHalf;
        const QVector3D c1 = mid + boxPerp * boxHalf - boxUp * boxHalf;
        const QVector3D c2 = mid + boxPerp * boxHalf + boxUp * boxHalf;
        const QVector3D c3 = mid - boxPerp * boxHalf + boxUp * boxHalf;
        valueVerts.append(Vertex{c0.x(), c0.y(), c0.z(), tr, tg, tb, ta});
        valueVerts.append(Vertex{c1.x(), c1.y(), c1.z(), tr, tg, tb, ta});
        valueVerts.append(Vertex{c2.x(), c2.y(), c2.z(), tr, tg, tb, ta});
        valueVerts.append(Vertex{c0.x(), c0.y(), c0.z(), tr, tg, tb, ta});
        valueVerts.append(Vertex{c2.x(), c2.y(), c2.z(), tr, tg, tb, ta});
        valueVerts.append(Vertex{c3.x(), c3.y(), c3.z(), tr, tg, tb, ta});
      }
    }
  }

  // Upload the three buffers (empty uploads are fine — vertex count stays 0).
  const quint32 lineBytes = quint32(lineVerts.size() * int(sizeof(Vertex)));
  const quint32 triBytes = quint32(triVerts.size() * int(sizeof(Vertex)));
  const quint32 valueBytes = quint32(valueVerts.size() * int(sizeof(Vertex)));
  if (!ensureBuffer(m_assemblyMeasureLineBuffer, lineBytes,
                    m_assemblyMeasureLineBufferBytes, QRhiBuffer::VertexBuffer)
      || !ensureBuffer(m_assemblyMeasureTriBuffer, triBytes,
                       m_assemblyMeasureTriBufferBytes, QRhiBuffer::VertexBuffer)
      || !ensureBuffer(m_assemblyMeasureValueBuffer, valueBytes,
                       m_assemblyMeasureValueBufferBytes, QRhiBuffer::VertexBuffer))
  {
    return false;
  }

  m_assemblyMeasureLineVertexCount = quint32(lineVerts.size());
  m_assemblyMeasureTriVertexCount = quint32(triVerts.size());
  m_assemblyMeasureValueVertexCount = quint32(valueVerts.size());
  if (m_assemblyMeasureLineBuffer && lineBytes > 0)
    updates->uploadStaticBuffer(m_assemblyMeasureLineBuffer.get(), 0, lineBytes,
                                lineVerts.constData());
  if (m_assemblyMeasureTriBuffer && triBytes > 0)
    updates->uploadStaticBuffer(m_assemblyMeasureTriBuffer.get(), 0, triBytes,
                                triVerts.constData());
  if (m_assemblyMeasureValueBuffer && valueBytes > 0)
    updates->uploadStaticBuffer(m_assemblyMeasureValueBuffer.get(), 0, valueBytes,
                                valueVerts.constData());
  m_assemblyMeasureLineBufferUploaded = true;
  m_assemblyMeasureTriBufferUploaded = true;
  m_assemblyMeasureValueBufferUploaded = true;
  return true;
}

void RhiViewportRenderer::renderAssemblyMeasureOverlay(QRhiCommandBuffer *cb)
{
  // Phase 92 (ASMMEASURE-02): draw the three overlay buffers. The dimension
  // line uses the shared line pipeline (GL_LINES, white dashes); the arrowheads
  // use the gizmo triangle pipeline (white, no depth write so they stay
  // visible); the teal value box uses the translucent fill pipeline (source-
  // alpha blend, no depth write). Drawn after the mesh + connectors so the
  // overlay sits on top (matches 装配页_测量.png).
  if (cb == nullptr)
    return;

  cb->setShaderResources();
  // Dimension line (white dashes).
  if (m_assemblyMeasureLineBuffer && m_assemblyMeasureLineVertexCount > 0
      && m_linePipeline)
  {
    cb->setGraphicsPipeline(m_linePipeline.get());
    const QRhiCommandBuffer::VertexInput binding(m_assemblyMeasureLineBuffer.get(), 0);
    cb->setVertexInput(0, 1, &binding);
    cb->draw(m_assemblyMeasureLineVertexCount);
  }
  // Arrowheads (white triangles). Uses m_fillPipeline (raw world-space
  // triangles) — NOT m_gizmoTriPipeline, which applies a gizmoCenter+scale
  // displacement in its vertex shader and would offset the arrowheads.
  if (m_assemblyMeasureTriBuffer && m_assemblyMeasureTriVertexCount > 0
      && m_fillPipeline)
  {
    cb->setGraphicsPipeline(m_fillPipeline.get());
    const QRhiCommandBuffer::VertexInput binding(m_assemblyMeasureTriBuffer.get(), 0);
    cb->setVertexInput(0, 1, &binding);
    cb->draw(m_assemblyMeasureTriVertexCount);
  }
  // Teal value box (translucent fill).
  if (m_assemblyMeasureValueBuffer && m_assemblyMeasureValueVertexCount > 0
      && m_translucentFillPipeline)
  {
    cb->setGraphicsPipeline(m_translucentFillPipeline.get());
    const QRhiCommandBuffer::VertexInput binding(m_assemblyMeasureValueBuffer.get(), 0);
    cb->setVertexInput(0, 1, &binding);
    cb->draw(m_assemblyMeasureValueVertexCount);
  }
}

bool RhiViewportRenderer::uploadCameraUniform(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags)
{
  if (updates == nullptr || rhi() == nullptr)
    return false;

  const bool uploadCamera = !m_cameraUniformBufferUploaded
      || (dirtyFlags & (PrepareSceneData::DirtyCamera
                        | PrepareSceneData::DirtyGpu
                        | PrepareSceneData::DirtySelection)) != 0;
  // Phase 68: DirtySelection is included because gizmoCenter (packed into
  // this uniform buffer) tracks the selected object's AABB. Without this, a
  // selection change would update m_gizmoCenter in synchronize() but the GPU
  // uniform would keep the stale value.
  if (!uploadCamera)
    return true;

  // D3D12 requires uniform buffers to be 256-byte aligned (HLSL cbuffer
  // minimum size). The MVP matrix is 64 bytes, but the backing buffer must be
  // padded to a 256-byte boundary or D3D12's setShaderResources reads past the
  // buffer end and segfaults. D3D11/Vulkan/Metal tolerate the smaller size,
  // so this only manifests under D3D12. We allocate 256 bytes, upload only
  // the 64-byte MVP, and leave the rest as padding.
  if (!ensureBuffer(m_cameraUniformBuffer, 256, m_cameraUniformBufferBytes, QRhiBuffer::UniformBuffer))
    return false;

  const QMatrix4x4 corrected = rhi()->clipSpaceCorrMatrix() * m_cameraMvp;
  updates->updateDynamicBuffer(m_cameraUniformBuffer.get(), 0, 64, corrected.constData());

  // Phase 68: pack gizmoCenter (vec3 at offset 64) + gizmoScale (float at
  // offset 76) into the same uniform buffer. The mesh shader's CameraBlock
  // only declares mat4 mvp (64 bytes) so it ignores the extra data; the gizmo
  // shader's CameraBlock declares the full { mat4; vec3; float; } and reads it.
  // std140 padding: vec3 is followed by 4 bytes of pad to reach the float at
  // offset 76 (= 64 + 12 + 4? no: std140 vec3 takes 16 bytes, but we pack the
  // float in the vec3's tail padding). Match the GLSL std140 layout exactly:
  //   offset 0:  mat4 mvp       (64 bytes)
  //   offset 64: vec3 gizmoCenter (12 bytes) + float gizmoScale (4 bytes) = 16 bytes
  // Total CameraBlock = 80 bytes, well within the 256-byte buffer.
  const float gizmoScale = std::max((m_gizmoCenter - m_cameraEye).length() * 0.15f, 5.f);
  // QVector3D has no data() method; take the address of the first component.
  // The three floats (x,y,z) are contiguous in memory per the Qt GUI ABI.
  updates->updateDynamicBuffer(m_cameraUniformBuffer.get(), 64, 12,
                               &m_gizmoCenter[0]);
  updates->updateDynamicBuffer(m_cameraUniformBuffer.get(), 76, 4,
                               &gizmoScale);

  m_cameraUniformBufferUploaded = true;
  return true;
}

bool RhiViewportRenderer::ensureBuffer(std::unique_ptr<QRhiBuffer> &buffer,
                                       quint32 byteSize,
                                       quint32 &storedSize,
                                       QRhiBuffer::UsageFlags usage)
{
  if (byteSize == 0) {
    buffer.reset();
    storedSize = 0;
    return true;
  }

  if (buffer && storedSize == byteSize)
    return true;

  const QRhiBuffer::Type type = (usage & QRhiBuffer::UniformBuffer) != 0
      ? QRhiBuffer::Dynamic
      : QRhiBuffer::Static;
  buffer.reset(rhi()->newBuffer(type, usage, byteSize));
  if (!buffer->create()) {
    buffer.reset();
    storedSize = 0;
    m_pipelineFailed = true;
    return false;
  }

  storedSize = byteSize;
  return true;
}

QVector<RhiViewportRenderer::Vertex> RhiViewportRenderer::buildSceneVertices(const QList<PrepareSceneData::Vertex> &source) const
{
  QVector<Vertex> vertices;
  vertices.reserve(source.size());

  for (const PrepareSceneData::Vertex &sourceVertex : source) {
    vertices.append(Vertex{sourceVertex.x,
                           0.0f,
                           sourceVertex.y,
                           sourceVertex.r,
                           sourceVertex.g,
                           sourceVertex.b,
                           sourceVertex.a});
  }

  return vertices;
}

QVector<RhiViewportRenderer::Vertex> RhiViewportRenderer::buildModelVertices(const QList<PrepareSceneData::ModelVertex> &source) const
{
  QVector<Vertex> vertices;
  vertices.reserve(source.size());

  // Functional source-truth mapping: upstream GLCanvas3D draws transformed
  // PartPlate-filtered model triangles in world coordinates. The QRhi path keeps
  // the same source-space contract and only changes the GPU transport.
  for (const PrepareSceneData::ModelVertex &sourceVertex : source) {
    vertices.append(Vertex{sourceVertex.x,
                           sourceVertex.y,
                           sourceVertex.z,
                           sourceVertex.r,
                           sourceVertex.g,
                           sourceVertex.b,
                           sourceVertex.a});
  }

  // Phase 91 (ASMEXPLODE-02): per-volume explosion offset on AssembleView only.
  // Mirrors upstream m_explosion_ratio separation (GLCanvas3D.hpp:596): each
  // volume is pushed radially away from its parent object's center by
  // (volumeCenter - objectCenter) * (ratio - 1.0). objectCenter is the midpoint
  // of the union of all sibling volume batches sharing a sourceObjectIndex.
  // Strictly gated to CanvasAssembleView and to ratio != 1.0, so Prepare
  // (CanvasView3D) and Preview (CanvasPreview) rendering is byte-for-byte
  // unaffected (CONTEXT.md decision 4 offset formula).
  if (m_canvasType == RhiViewport::CanvasAssembleView
      && !qFuzzyIsNull(m_explosionRatio - 1.0f)
      && !vertices.isEmpty())
  {
    const QList<PrepareSceneData::ModelBatch> &batches = m_prepareScene.modelBatches();

    // Precompute per-object unioned bounds midpoint (objectCenter).
    QHash<int, PrepareSceneData::ModelBounds> objectBoundsBySource;
    objectBoundsBySource.reserve(batches.size() * 2);
    for (const PrepareSceneData::ModelBatch &batch : batches)
    {
      if (batch.sourceObjectIndex < 0 || batch.vertexCount <= 0)
        continue;
      auto it = objectBoundsBySource.find(batch.sourceObjectIndex);
      if (it == objectBoundsBySource.end())
      {
        objectBoundsBySource.insert(batch.sourceObjectIndex, batch.bounds);
      }
      else
      {
        PrepareSceneData::ModelBounds &ub = it.value();
        ub.minX = std::min(ub.minX, batch.bounds.minX);
        ub.minY = std::min(ub.minY, batch.bounds.minY);
        ub.minZ = std::min(ub.minZ, batch.bounds.minZ);
        ub.maxX = std::max(ub.maxX, batch.bounds.maxX);
        ub.maxY = std::max(ub.maxY, batch.bounds.maxY);
        ub.maxZ = std::max(ub.maxZ, batch.bounds.maxZ);
      }
    }

    const float t = m_explosionRatio - 1.0f;
    for (const PrepareSceneData::ModelBatch &batch : batches)
    {
      if (batch.sourceObjectIndex < 0 || batch.vertexCount <= 0)
        continue;
      const auto it = objectBoundsBySource.constFind(batch.sourceObjectIndex);
      if (it == objectBoundsBySource.constEnd())
        continue;

      const PrepareSceneData::ModelBounds &objectBounds = it.value();
      const float objectCenterX = (objectBounds.minX + objectBounds.maxX) * 0.5f;
      const float objectCenterY = (objectBounds.minY + objectBounds.maxY) * 0.5f;
      const float objectCenterZ = (objectBounds.minZ + objectBounds.maxZ) * 0.5f;
      // batchCenter = midpoint of this volume's own bounds.
      const float batchCenterX = (batch.bounds.minX + batch.bounds.maxX) * 0.5f;
      const float batchCenterY = (batch.bounds.minY + batch.bounds.maxY) * 0.5f;
      const float batchCenterZ = (batch.bounds.minZ + batch.bounds.maxZ) * 0.5f;
      const float offX = (batchCenterX - objectCenterX) * t;
      const float offY = (batchCenterY - objectCenterY) * t;
      const float offZ = (batchCenterZ - objectCenterZ) * t;

      const int endVertex = std::min(batch.firstVertex + batch.vertexCount, int(vertices.size()));
      for (int i = std::max(0, batch.firstVertex); i < endVertex; ++i)
      {
        Vertex &v = vertices[i];
        v.x += offX;
        v.y += offY;
        v.z += offZ;
      }
    }
  }

  // Phase 138 (ASM-01): per-object assemble-offset translation on AssembleView.
  // The mesh blob above already bakes the ordinary per-object m_transformation
  // (offset/rotation/scale) into world space, but NOT the per-instance assemble
  // transform (ModelInstance::m_assemble_transformation, Model.hpp:1280-1294)
  // which upstream stores separately. On the assembly canvas, Move/Rotate/Scale
  // gizmo edits write the assemble transform (Plan 138-02 routing); the ViewModel
  // exposes per-source-object offsets (assembleOffsets Q_PROPERTY) which the
  // viewport forwards here. We apply the assemble translation (offset) to every
  // vertex of each batch keyed by sourceObjectIndex. Rotate/scale of the assemble
  // pose are persisted (Plan 01/02) and round-trip (Plan 04) but are not yet
  // reflected in the live render — translate-only rendering is the ASM-01 minimum
  // (Move is the primary interaction); full-matrix compose is a render-fidelity
  // follow-up. Gated to CanvasAssembleView; Prepare/Preview unaffected.
  // m_assembleOffsetBySource is built in synchronize() by zipping the offsets
  // with the parallel meshBatchSourceObjectIndices list.
  //
  // Phase 141 (DEBT-04): the loop now prefers the full composed transform
  // (m_assembleTransformBySource, populated when any rotate/scale is non-identity)
  // and falls back to the legacy translate-only offset map otherwise. This closes
  // the v4.8 tech debt where Rotate/Scale gizmo drags persisted + round-tripped
  // but were not reflected in the live CanvasAssembleView render.
  if (m_canvasType == RhiViewport::CanvasAssembleView
      && !m_assembleOffsetBySource.isEmpty()
      && !vertices.isEmpty())
  {
    const QList<PrepareSceneData::ModelBatch> &batches = m_prepareScene.modelBatches();
    for (const PrepareSceneData::ModelBatch &batch : batches)
    {
      if (batch.sourceObjectIndex < 0 || batch.vertexCount <= 0)
        continue;
      const auto tIt = m_assembleTransformBySource.constFind(batch.sourceObjectIndex);
      const bool hasFull = (tIt != m_assembleTransformBySource.constEnd());
      const auto oIt = m_assembleOffsetBySource.constFind(batch.sourceObjectIndex);
      const bool hasOff = (oIt != m_assembleOffsetBySource.constEnd());
      if (!hasFull && !hasOff)
        continue;
      const int endVertex = std::min(batch.firstVertex + batch.vertexCount, int(vertices.size()));
      if (hasFull)
      {
        const QMatrix4x4 &m = tIt.value();
        for (int i = std::max(0, batch.firstVertex); i < endVertex; ++i)
        {
          Vertex &v = vertices[i];
          const QVector3D p = m.map(QVector3D(v.x, v.y, v.z));
          v.x = p.x();
          v.y = p.y();
          v.z = p.z();
        }
      }
      else
      {
        const QVector3D &off = oIt.value();
        for (int i = std::max(0, batch.firstVertex); i < endVertex; ++i)
        {
          Vertex &v = vertices[i];
          v.x += off.x();
          v.y += off.y();
          v.z += off.z();
        }
      }
    }
  }

  return vertices;
}

QVector<RhiViewportRenderer::Vertex> RhiViewportRenderer::buildHighlightVertices() const
{
  QVector<Vertex> vertices;
  const int selectedSourceObjectIndex = m_prepareScene.selectedSourceObjectIndex();
  const int hoveredSourceObjectIndex = m_prepareScene.hoveredSourceObjectIndex();
  if (selectedSourceObjectIndex < 0 && hoveredSourceObjectIndex < 0)
    return vertices;

  const QList<PrepareSceneData::ModelVertex> &source = m_prepareScene.modelVertices();
  for (const PrepareSceneData::ModelBatch &batch : m_prepareScene.modelBatches()) {
    const bool selected = batch.sourceObjectIndex == selectedSourceObjectIndex;
    const bool hovered = batch.sourceObjectIndex == hoveredSourceObjectIndex;
    if (!selected && !hovered)
      continue;

    const float r = selected ? 1.0f : 0.35f;
    const float g = selected ? 0.78f : 0.75f;
    const float b = selected ? 0.22f : 1.0f;
    const float a = selected ? 0.62f : 0.38f;
    const int endVertex = std::min(batch.firstVertex + batch.vertexCount, int(source.size()));
    for (int i = std::max(0, batch.firstVertex); i < endVertex; ++i) {
      const PrepareSceneData::ModelVertex &sourceVertex = source.at(i);
      vertices.append(Vertex{sourceVertex.x,
                             sourceVertex.y,
                             sourceVertex.z,
                             r,
                             g,
                             b,
                             a});
    }
  }

  return vertices;
}

QShader RhiViewportRenderer::loadShader(const QString &path) const
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return {};
  return QShader::fromSerialized(file.readAll());
}

// ===========================================================================
// Phase 67: Gizmo center computation (delegates to the free function so it
// can be unit-tested without linking the full renderer).
// ===========================================================================
QVector3D RhiViewportRenderer::computeGizmoCenter() const
{
  return GizmoCenter::fromSelectedBatch(
      m_prepareScene.selectedSourceObjectIndex(),
      m_prepareScene.modelBatches());
}

// ===========================================================================
// Phase 68: Move gizmo RHI rendering
// ===========================================================================
bool RhiViewportRenderer::uploadGizmoBuffer(QRhiResourceUpdateBatch *updates)
{
  if (updates == nullptr || rhi() == nullptr)
    return false;
  if (m_gizmoVertexBufferUploaded)
    return true;

  GizmoGeometryOffsets moveOffsets;
  GizmoGeometryOffsets rotateOffsets;
  GizmoGeometryOffsets scaleOffsets;
  QVector<GizmoVertex> moveVerts = GizmoGeometry::buildMoveGizmoVertices(&moveOffsets);
  QVector<GizmoVertex> rotateVerts = GizmoGeometry::buildRotateGizmoVertices(&rotateOffsets);
  QVector<GizmoVertex> scaleVerts = GizmoGeometry::buildScaleGizmoVertices(&scaleOffsets);

  auto adjustOffsets = [](GizmoGeometryOffsets offsets, int base) {
    for (int ax = 0; ax < 3; ++ax)
    {
      if (offsets.shaftVertCount > 0)
        offsets.shaftBase[ax] += base;
      if (offsets.coneVertCount > 0)
        offsets.coneBase[ax] += base;
      if (offsets.boxVertCount > 0)
        offsets.boxBase[ax] += base;
      if (offsets.ringVertCount > 0)
        offsets.ringBase[ax] += base;
    }
    return offsets;
  };

  const int moveBase = 0;
  const int rotateBase = moveVerts.size();
  const int scaleBase = rotateBase + rotateVerts.size();
  m_moveGizmoOffsets = adjustOffsets(moveOffsets, moveBase);
  m_rotateGizmoOffsets = adjustOffsets(rotateOffsets, rotateBase);
  m_scaleGizmoOffsets = adjustOffsets(scaleOffsets, scaleBase);

  QVector<GizmoVertex> verts;
  verts.reserve(moveVerts.size() + rotateVerts.size() + scaleVerts.size());
  verts += moveVerts;
  verts += rotateVerts;
  verts += scaleVerts;

  const quint32 byteSize = quint32(verts.size() * sizeof(GizmoVertex));
  if (!ensureBuffer(m_gizmoVertexBuffer, byteSize, m_gizmoVertexBufferBytes,
                    QRhiBuffer::VertexBuffer))
    return false;

  updates->uploadStaticBuffer(m_gizmoVertexBuffer.get(), 0, byteSize, verts.constData());
  m_gizmoVertexBufferUploaded = true;
  qInfo("[RHI] gizmo vertex buffer uploaded: %u verts (move=%d rotate=%d scale=%d)",
        quint32(verts.size()), moveVerts.size(), rotateVerts.size(), scaleVerts.size());
  return true;
}

bool RhiViewportRenderer::ensureGizmoPipeline()
{
  if (m_gizmoPipelineCreated)
    return true;
  if (m_pipelineFailed || rhi() == nullptr || renderTarget() == nullptr)
    return false;

  // Gizmo shader reads the extended CameraBlock { mat4 mvp; vec3 gizmoCenter;
  // float gizmoScale; } at binding 0 - same binding as the mesh SRB, so reuse
  // m_srb (the uniform buffer now carries gizmoCenter+gizmoScale packed after
  // the MVP, see uploadCameraUniform).
  if (m_srb == nullptr)
    return false;

  QShader vertexShader = loadShader(QStringLiteral(":/rhi_viewport/shaders/rhi_gizmo.vert.qsb"));
  QShader fragmentShader = loadShader(QStringLiteral(":/rhi_viewport/shaders/rhi_gizmo.frag.qsb"));
  if (!vertexShader.isValid() || !fragmentShader.isValid())
  {
    m_pipelineFailed = true;
    return false;
  }

  // Same vertex input layout as meshes: position (float3) + color (float4).
  QRhiVertexInputLayout inputLayout;
  inputLayout.setBindings({QRhiVertexInputBinding(sizeof(GizmoVertex))});
  inputLayout.setAttributes({
      QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, offsetof(GizmoVertex, x)),
      QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float4, offsetof(GizmoVertex, r)),
  });

  m_renderPassDescriptor = renderTarget()->renderPassDescriptor();

  auto buildOne = [&](std::unique_ptr<QRhiGraphicsPipeline> &pipe,
                      QRhiGraphicsPipeline::Topology topology) -> bool
  {
    pipe.reset(rhi()->newGraphicsPipeline());
    pipe->setTopology(topology);
    pipe->setShaderStages({
        QRhiShaderStage(QRhiShaderStage::Vertex, vertexShader),
        QRhiShaderStage(QRhiShaderStage::Fragment, fragmentShader),
    });
    pipe->setShaderResourceBindings(m_srb.get());
    pipe->setVertexInputLayout(inputLayout);
    pipe->setRenderPassDescriptor(m_renderPassDescriptor);
    // Gizmos are overlays. The model has already populated the depth buffer
    // in this pass, so testing depth here would hide axes inside the selected
    // object instead of matching the legacy GL depth-clear behavior.
    pipe->setDepthTest(false);
    pipe->setDepthWrite(false);
    pipe->setTargetBlends({});
    if (!pipe->create())
    {
      pipe.reset();
      m_pipelineFailed = true;
      return false;
    }
    return true;
  };

  if (!buildOne(m_gizmoLinePipeline, QRhiGraphicsPipeline::Lines))
    return false;
  if (!buildOne(m_gizmoTriPipeline, QRhiGraphicsPipeline::Triangles))
    return false;

  m_gizmoPipelineCreated = true;
  qInfo("[RHI] gizmo pipelines created (lines + triangles, depth-independent overlay)");
  return true;
}

void RhiViewportRenderer::renderMoveGizmo(QRhiCommandBuffer *cb)
{
  if (cb == nullptr || m_gizmoVertexBuffer == nullptr)
    return;
  // Only draw when move mode is active AND something is selected.
  if (m_gizmoMode != 0 /*GizmoMove*/ ||
      m_prepareScene.selectedSourceObjectIndex() < 0)
    return;
  if (!ensureGizmoPipeline())
    return;

  const QRhiCommandBuffer::VertexInput gizmoBinding(m_gizmoVertexBuffer.get(), 0);
  cb->setShaderResources();

  // Shafts (GL_LINES).
  if (m_gizmoLinePipeline)
  {
    cb->setGraphicsPipeline(m_gizmoLinePipeline.get());
    cb->setVertexInput(0, 1, &gizmoBinding);
    for (int ax = 0; ax < 3; ++ax)
      cb->draw(m_moveGizmoOffsets.shaftVertCount, 1, m_moveGizmoOffsets.shaftBase[ax]);
  }
  // Cones (GL_TRIANGLES).
  if (m_gizmoTriPipeline)
  {
    cb->setGraphicsPipeline(m_gizmoTriPipeline.get());
    cb->setVertexInput(0, 1, &gizmoBinding);
    for (int ax = 0; ax < 3; ++ax)
      cb->draw(m_moveGizmoOffsets.coneVertCount, 1, m_moveGizmoOffsets.coneBase[ax]);
  }
}

void RhiViewportRenderer::renderRotateGizmo(QRhiCommandBuffer *cb)
{
  if (cb == nullptr || m_gizmoVertexBuffer == nullptr)
    return;
  if (m_gizmoMode != 1 /*GizmoRotate*/ ||
      m_prepareScene.selectedSourceObjectIndex() < 0)
    return;
  if (!ensureGizmoPipeline())
    return;

  if (m_gizmoTriPipeline)
  {
    const QRhiCommandBuffer::VertexInput gizmoBinding(m_gizmoVertexBuffer.get(), 0);
    cb->setShaderResources();
    cb->setGraphicsPipeline(m_gizmoTriPipeline.get());
    cb->setVertexInput(0, 1, &gizmoBinding);
    for (int ax = 0; ax < 3; ++ax)
      cb->draw(m_rotateGizmoOffsets.ringVertCount, 1, m_rotateGizmoOffsets.ringBase[ax]);
  }
}

void RhiViewportRenderer::renderScaleGizmo(QRhiCommandBuffer *cb)
{
  if (cb == nullptr || m_gizmoVertexBuffer == nullptr)
    return;
  if (m_gizmoMode != 2 /*GizmoScale*/ ||
      m_prepareScene.selectedSourceObjectIndex() < 0)
    return;
  if (!ensureGizmoPipeline())
    return;

  const QRhiCommandBuffer::VertexInput gizmoBinding(m_gizmoVertexBuffer.get(), 0);
  cb->setShaderResources();

  if (m_gizmoLinePipeline)
  {
    cb->setGraphicsPipeline(m_gizmoLinePipeline.get());
    cb->setVertexInput(0, 1, &gizmoBinding);
    for (int ax = 0; ax < 3; ++ax)
      cb->draw(m_scaleGizmoOffsets.shaftVertCount, 1, m_scaleGizmoOffsets.shaftBase[ax]);
  }

  if (m_gizmoTriPipeline)
  {
    cb->setGraphicsPipeline(m_gizmoTriPipeline.get());
    cb->setVertexInput(0, 1, &gizmoBinding);
    for (int ax = 0; ax < 3; ++ax)
      cb->draw(m_scaleGizmoOffsets.boxVertCount, 1, m_scaleGizmoOffsets.boxBase[ax]);
  }
}

// ── Phase 26: Preview segment pipeline ──────────────────────────────────────
// GCV1 wire format from PreviewViewModel: "GCV1" magic + int count +
// count * PackedSegment (76 bytes each). Each segment → 2 Line vertices
// (start xyz + end xyz, sharing RGBA), with GCode y↔z axis swap to GL space.
// Layer ranges are indexed per-layer for GPU draw-range filtering (D-26-02).
// Color is CPU-pre-baked by PreviewViewModel (D-26-03); renderer is opaque RGBA.

void RhiViewportRenderer::renderCutPlane(QRhiCommandBuffer *cb)
{
  if (cb == nullptr)
    return;
  if (m_gizmoMode != 5 /*GizmoCut*/ && m_gizmoMode != 14 /*GizmoAdvancedCut*/)
    return;
  if (m_prepareScene.selectedSourceObjectIndex() < 0)
    return;

  if (m_translucentFillPipeline && m_cutPlaneFillBuffer && m_cutPlaneFillVertexCount > 0)
  {
    cb->setShaderResources(m_srb.get());
    cb->setGraphicsPipeline(m_translucentFillPipeline.get());
    const QRhiCommandBuffer::VertexInput fillBinding(m_cutPlaneFillBuffer.get(), 0);
    cb->setVertexInput(0, 1, &fillBinding);
    cb->draw(m_cutPlaneFillVertexCount);
  }

  if (m_translucentLinePipeline && m_cutPlaneOutlineBuffer && m_cutPlaneOutlineVertexCount > 0)
  {
    cb->setShaderResources(m_srb.get());
    cb->setGraphicsPipeline(m_translucentLinePipeline.get());
    const QRhiCommandBuffer::VertexInput outlineBinding(m_cutPlaneOutlineBuffer.get(), 0);
    cb->setVertexInput(0, 1, &outlineBinding);
    cb->draw(m_cutPlaneOutlineVertexCount);
  }
}

void RhiViewportRenderer::renderWipeTower(QRhiCommandBuffer *cb)
{
  if (cb == nullptr)
    return;
  if (!m_prepareScene.showBed() || !m_showWipeTower)
    return;
  if (m_translucentFillPipeline == nullptr || m_wipeTowerBuffer == nullptr ||
      m_wipeTowerVertexCount == 0)
    return;

  cb->setShaderResources(m_srb.get());
  cb->setGraphicsPipeline(m_translucentFillPipeline.get());
  const QRhiCommandBuffer::VertexInput wipeBinding(m_wipeTowerBuffer.get(), 0);
  cb->setVertexInput(0, 1, &wipeBinding);
  cb->draw(m_wipeTowerVertexCount);
}

// ===========================================================================
// Phase 121 (PAINT-02/OV-03): painted-facet overlay upload + render.
//
// uploadPaintOverlayBuffer parses the m_paintOverlayData byte stream (produced
// by EditorViewModel::paintOverlayData) into GizmoVertex records with a
// per-state color, then ensureBuffer + uploadStaticBuffer. The byte stream is
// ALREADY world-transformed (the ViewModel applies rebuildWorldTransform), so
// the renderer just copies the vertices and applies the upstream Y->Qt-Z axis
// swap (the scene uses X/Z as the bed plane, Y up -- same swap as the mesh-data
// path in parsePreviewSegments).
//
// Wire format (packed, little-endian):
//   header: [int32 modeLabel, int32 triangleCount]
//   body:   triangleCount * [int32 state, float vx,vy,vz (3 verts)] (40 B/ea)
//
// Color mapping (OV-04):
//   Support/Seam Enforcer(1) = light green (0.5,1,0.5,1)
//   Support/Seam Blocker(2)  = light red  (1,0.5,0.5,1)
//   MMU ExtruderN(N in 1..16) = extrudersColors[N-1] hex -> QColor
// ===========================================================================
namespace {
struct PaintOverlayHeader
{
  qint32 modeLabel;
  qint32 triangleCount;
};
struct PaintOverlayTri
{
  qint32 state;
  float verts[9]; // 3 verts x (x,y,z)
};
// State -> RGBA. Support/Seam use Enforcer=green/Blocker=red; MMU states
// (3..16) are colored per-extruder by the caller (stateColor function falls
// back to a neutral gray for MMU when no extruder color list is set).
inline void stateColor(int state, const QVariantList &extruderColors,
                       float out[4])
{
  // EnforcerBlockerType: 1=Enforcer, 2=Blocker, 3..16=Extruder3..16
  // (TriangleSelector.hpp:13-38). Extruder1 aliases ENFORCER, Extruder2 aliases
  // BLOCKER, so MMU extruders 1/2 reuse the green/red; extruders 3..16 use the
  // filament-color list.
  if (state == 1) // Enforcer / Extruder1
  {
    out[0] = 0.5f; out[1] = 1.0f; out[2] = 0.5f; out[3] = 1.0f;
    return;
  }
  if (state == 2) // Blocker / Extruder2
  {
    out[0] = 1.0f; out[1] = 0.5f; out[2] = 0.5f; out[3] = 1.0f;
    return;
  }
  // MMU extruder 3..16: index into extruderColors (state-1, clamped). Missing
  // list -> neutral gray so the facet is still visible.
  if (state >= 3 && state <= 16 && !extruderColors.isEmpty())
  {
    const int idx = (state - 1) % extruderColors.size();
    const QColor c = QColor(extruderColors.at(idx).toString());
    if (c.isValid())
    {
      out[0] = float(c.redF());
      out[1] = float(c.greenF());
      out[2] = float(c.blueF());
      out[3] = 1.0f;
      return;
    }
  }
  // Fallback neutral.
  out[0] = 0.7f; out[1] = 0.7f; out[2] = 0.7f; out[3] = 1.0f;
}
} // namespace

bool RhiViewportRenderer::uploadPaintOverlayBuffer(QRhiResourceUpdateBatch *updates)
{
  if (updates == nullptr || rhi() == nullptr)
    return false;
  if (m_paintOverlayBufferUploaded)
    return true;

  QVector<Vertex> vertices;
  if (m_paintOverlayData.size() >= int(sizeof(PaintOverlayHeader)))
  {
    const auto *hdr = reinterpret_cast<const PaintOverlayHeader *>(
        m_paintOverlayData.constData());
    const qint32 triCount = hdr->triangleCount;
    const qint64 expected = qint64(sizeof(PaintOverlayHeader))
                            + qint64(triCount) * qint64(sizeof(PaintOverlayTri));
    if (triCount > 0 && m_paintOverlayData.size() >= expected)
    {
      const auto *tris = reinterpret_cast<const PaintOverlayTri *>(
          m_paintOverlayData.constData() + sizeof(PaintOverlayHeader));
      vertices.reserve(int(triCount) * 3);
      for (qint32 t = 0; t < triCount; ++t)
      {
        float color[4];
        stateColor(int(tris[t].state), m_extrudersColors, color);
        for (int v = 0; v < 3; ++v)
        {
          // Upstream Y -> Qt-Z axis swap (libslic3r world X,Y,Z -> scene X,Z,Y),
          // matching the mesh-data path (parsePreviewSegments). The ViewModel
          // emits libslic3r world coords; the renderer converts to scene coords.
          Vertex gv;
          gv.x = tris[t].verts[v * 3 + 0];
          gv.y = tris[t].verts[v * 3 + 2]; // libslic3r Z -> scene Y (up)
          gv.z = tris[t].verts[v * 3 + 1]; // libslic3r Y -> scene Z (into bed)
          gv.r = color[0];
          gv.g = color[1];
          gv.b = color[2];
          gv.a = color[3];
          vertices.append(gv);
        }
      }
    }
  }

  const quint32 byteSize = quint32(vertices.size() * int(sizeof(Vertex)));
  if (!ensureBuffer(m_paintOverlayBuffer, byteSize, m_paintOverlayBufferBytes,
                    QRhiBuffer::VertexBuffer))
    return false;

  m_paintOverlayVertexCount = quint32(vertices.size());
  if (m_paintOverlayBuffer && byteSize > 0)
  {
    updates->uploadStaticBuffer(m_paintOverlayBuffer.get(), 0, byteSize,
                                vertices.constData());
  }
  m_paintOverlayBufferUploaded = true;
  return true;
}

void RhiViewportRenderer::renderPaintOverlay(QRhiCommandBuffer *cb)
{
  if (cb == nullptr)
    return;
  // OV-03: gate to the three paint gizmos (Support=6, Seam=7, MMU=10).
  if (m_gizmoMode != 6 && m_gizmoMode != 7 && m_gizmoMode != 10)
    return;
  if (m_fillPipeline == nullptr || m_paintOverlayBuffer == nullptr ||
      m_paintOverlayVertexCount == 0)
    return;

  cb->setShaderResources(m_srb.get());
  // Reuse m_fillPipeline (opaque vertex-color fill, same as the model mesh).
  cb->setGraphicsPipeline(m_fillPipeline.get());
  const QRhiCommandBuffer::VertexInput binding(m_paintOverlayBuffer.get(), 0);
  cb->setVertexInput(0, 1, &binding);
  cb->draw(m_paintOverlayVertexCount);
}

// ===========================================================================
// Phase 121 (PAINT-03/OV-05): brush sphere cursor.
//
// The cursor is a translucent UV-sphere (buildBrushSphereVertices) centered at
// the world point under the mouse. To avoid duplicating the picking pipeline in
// the renderer (pickSourceObjectAt lives in RhiViewport), the world center is
// approximated by unprojecting the mouse to a world ray and intersecting the
// bed plane (Y=0) -- the same anchor the model sits on. This keeps the cursor
// glued to the bed at the mouse location without a per-frame raycast. When no
// bed intersection exists (ray parallel to bed), the cursor falls back to the
// selected-object gizmo center so it stays visible.
//
// Color (OV-05): left-button blue (0,0,1,0.25), right-button red (1,0,0,0.25),
// hover black (0,0,0,0.25). brushButtonState < 0 hides the cursor entirely.
// ===========================================================================
bool RhiViewportRenderer::uploadBrushCursorBuffer(QRhiResourceUpdateBatch *updates)
{
  if (updates == nullptr || rhi() == nullptr)
    return false;

  // Determine the cursor color from the button state.
  float color[4] = {0.f, 0.f, 0.f, 0.25f}; // hover black (default)
  if (m_brushButtonState == 1)             // left -> blue
  {
    color[2] = 1.0f;
  }
  else if (m_brushButtonState == 2)        // right -> red
  {
    color[0] = 1.0f;
  }

  // Recompute the world center only when inputs change (mouse position, radius,
  // button state, or MVP). The MVP changes with camera moves, so compare the
  // raw screen coords + radius + button; the world center is derived in
  // renderBrushCursor (kept here as a cached value). To keep this simple, we
  // rebuild the sphere whenever any cursor-driving input changed since the last
  // upload -- tracked via m_brushCursorLastButtonState + a screen-position
  // comparison.
  const bool inputsChanged =
      (m_brushButtonState != m_brushCursorLastButtonState) ||
      !qFuzzyCompare(m_brushMouseScreenX, m_brushCursorLastScreenX) ||
      !qFuzzyCompare(m_brushMouseScreenY, m_brushCursorLastScreenY) ||
      !qFuzzyCompare(m_brushRadius, m_brushCursorLastRadius);
  if (!inputsChanged && m_brushCursorBufferUploaded)
    return true;

  m_brushCursorLastButtonState = m_brushButtonState;
  m_brushCursorLastScreenX = m_brushMouseScreenX;
  m_brushCursorLastScreenY = m_brushMouseScreenY;
  m_brushCursorLastRadius = m_brushRadius;

  // Hidden (buttonState < 0): emit zero verts so the draw is skipped.
  QVector<Vertex> vertices;
  if (m_brushButtonState >= 0 && m_brushRadius > 0.f)
  {
    // Unproject the mouse to a world ray and intersect the bed plane (Y=0).
    // m_cameraMvp is the view-projection; invert it to map NDC->world. The
    // mouse is in pixel coords; convert to NDC via the render-target size.
    QVector3D center = m_gizmoCenter; // fallback when no bed hit
    if (renderTarget() != nullptr)
    {
      const QSize pix = renderTarget()->pixelSize();
      if (pix.width() > 0 && pix.height() > 0)
      {
        const float ndcX = (m_brushMouseScreenX / float(pix.width())) * 2.f - 1.f;
        const float ndcY = 1.f - (m_brushMouseScreenY / float(pix.height())) * 2.f;
        const QMatrix4x4 invMvp = m_cameraMvp.inverted();
        const QVector3D nearPt = invMvp.map(QVector3D(ndcX, ndcY, 0.f));
        const QVector3D farPt = invMvp.map(QVector3D(ndcX, ndcY, 1.f));
        const QVector3D dir = (farPt - nearPt).normalized();
        // Intersect with the Y=0 plane (scene bed plane). Solving
        // nearPt.y + t*dir.y = 0 -> t = -nearPt.y / dir.y.
        if (std::abs(dir.y()) > 1e-6f)
        {
          const float t = -nearPt.y() / dir.y();
          if (t >= 0.f) // forward of the camera
            center = nearPt + dir * t;
        }
      }
    }
    m_brushCursorWorldCenter = center;
    vertices = GizmoGeometry::buildBrushSphereVertices(center, m_brushRadius,
                                                       color);
  }

  const quint32 byteSize = quint32(vertices.size() * int(sizeof(Vertex)));
  if (!ensureBuffer(m_brushCursorBuffer, byteSize, m_brushCursorBufferBytes,
                    QRhiBuffer::VertexBuffer))
    return false;

  m_brushCursorVertexCount = quint32(vertices.size());
  if (m_brushCursorBuffer && byteSize > 0)
  {
    updates->uploadStaticBuffer(m_brushCursorBuffer.get(), 0, byteSize,
                                vertices.constData());
  }
  m_brushCursorBufferUploaded = true;
  return true;
}

void RhiViewportRenderer::renderBrushCursor(QRhiCommandBuffer *cb)
{
  if (cb == nullptr)
    return;
  // OV-05: gate to the three paint gizmos; hide when buttonState < 0.
  if (m_gizmoMode != 6 && m_gizmoMode != 7 && m_gizmoMode != 10)
    return;
  if (m_brushButtonState < 0)
    return;
  if (m_translucentFillPipeline == nullptr || m_brushCursorBuffer == nullptr ||
      m_brushCursorVertexCount == 0)
    return;

  cb->setShaderResources(m_srb.get());
  cb->setGraphicsPipeline(m_translucentFillPipeline.get());
  const QRhiCommandBuffer::VertexInput binding(m_brushCursorBuffer.get(), 0);
  cb->setVertexInput(0, 1, &binding);
  cb->draw(m_brushCursorVertexCount);
}

namespace {
struct GcvPackedSegment
{
  float x1, y1, z1, x2, y2, z2;
  float r, g, b;
  float feedrate, fan_speed, temperature, width, layer_time, acceleration;
  int extruder_id, layer, move;
  int role;  // must match PackedSegment layout exactly (canonical libvgcode index).
};
// Wire-format lock-step guard: PackedSegment and GcvPackedSegment carry the
// identical 76-byte layout (16 floats + 4 ints) so the GCV1 blob memcpy is safe.
static_assert(sizeof(GcvPackedSegment) == 76, "GcvPackedSegment must be 76 bytes after adding role");
// PackedSegment is 76 bytes (16 floats + 4 ints, packed); if the platform adds
// padding the parse logic uses sizeof explicitly.
} // namespace

void RhiViewportRenderer::parsePreviewSegments()
{
  resetPreviewGpuState(false);

  if (m_previewData.size() < 8)
    return;
  if (std::memcmp(m_previewData.constData(), "GCV1", 4) != 0)
    return;

  int count = 0;
  std::memcpy(&count, m_previewData.constData() + 4, 4);
  if (count <= 0)
    return;

  const qsizetype payloadSize = qsizetype(count) * sizeof(GcvPackedSegment);
  if (m_previewData.size() < 8 + payloadSize)
    return;

  const auto *seg = reinterpret_cast<const GcvPackedSegment *>(m_previewData.constData() + 8);
  m_previewVertices.reserve(int(count) * 2);
  m_previewDrawSpans.reserve(int(count));

  for (int i = 0; i < count; ++i)
  {
    // Axis swap: GCode (x, y, z) -> scene (x, z, y), matching the legacy
    // preview renderer convention.
    Vertex a;
    a.x = seg[i].x1;
    a.y = seg[i].z1;
    a.z = seg[i].y1;
    a.r = seg[i].r;
    a.g = seg[i].g;
    a.b = seg[i].b;
    a.a = 1.0f;

    Vertex b = a;
    b.x = seg[i].x2;
    b.y = seg[i].z2;
    b.z = seg[i].y2;

    m_previewVertices.append(a);
    m_previewVertices.append(b);

    const quint32 vertexOffset = quint32(m_previewVertices.size() - 2);
    m_previewDrawSpans.append({seg[i].layer, seg[i].move, vertexOffset, 2, seg[i].role});
  }

  m_previewSegmentVertexCount = quint32(m_previewVertices.size());

  // Invalidate the draw-range cache: m_previewDrawSpans was just rebuilt, so
  // any cached ranges from a previous span set are stale. The next
  // computePreviewDrawRanges call will repopulate the cache.
  m_previewRangeCacheKey = 0;
  m_cachedPreviewRanges.clear();
}

bool RhiViewportRenderer::uploadPreviewSegmentBuffer(QRhiResourceUpdateBatch *updates)
{
  if (m_previewVertices.isEmpty())
    return false;

  const quint32 byteSize = m_previewSegmentVertexCount * sizeof(Vertex);
  if (!ensureBuffer(m_previewSegmentBuffer, byteSize, m_previewSegmentBufferBytes,
                    QRhiBuffer::VertexBuffer))
    return false;

  updates->uploadStaticBuffer(m_previewSegmentBuffer.get(), 0, byteSize,
                              m_previewVertices.constData());
  // Phase 27 (PERF-01): capture upload timing.
  m_previewLastUploadMs = 0; // uploadStaticBuffer is deferred; actual timing measured at frame level
  return true;
}

QVector<RhiViewportRenderer::PreviewDrawRange> RhiViewportRenderer::computePreviewDrawRanges() const
{
  // Cache check: this function is called every render frame, but its inputs
  // (layerMin/Max, moveEnd, roleVisibility, span count) change rarely. Reuse
  // the cached result when the input signature matches; the heavy O(N) span
  // traversal below only runs on a real input change.
  const quint64 cacheKey = computePreviewRangeCacheKey();
  if (cacheKey == m_previewRangeCacheKey && !m_cachedPreviewRanges.isEmpty()
      && m_previewRangeCacheKey != 0) {
    return m_cachedPreviewRanges;
  }

  QVector<PreviewDrawRange> ranges;

  const auto logRangeIfChanged = [this](quint32 first, quint32 count, int layerLow, int layerHigh) {
    if (m_lastLoggedPreviewFirstVertex == first
        && m_lastLoggedPreviewVertexCount == count
        && m_lastLoggedPreviewLayerLow == layerLow
        && m_lastLoggedPreviewLayerHigh == layerHigh
        && m_lastLoggedPreviewMoveEnd == m_moveEnd)
      return;
    m_lastLoggedPreviewFirstVertex = first;
    m_lastLoggedPreviewVertexCount = count;
    m_lastLoggedPreviewLayerLow = layerLow;
    m_lastLoggedPreviewLayerHigh = layerHigh;
    m_lastLoggedPreviewMoveEnd = m_moveEnd;
    qInfo("[RHI] preview ranges first=%u visibleCount=%u layers=%d..%d moveEnd=%d",
          first,
          count,
          layerLow,
          layerHigh,
          m_moveEnd);
  };

  const int layerLow = std::min(m_layerMin, m_layerMax);
  const int layerHigh = std::max(m_layerMin, m_layerMax);

  if (m_previewDrawSpans.isEmpty()) {
    logRangeIfChanged(0, 0, layerLow, layerHigh);
    m_previewRangeCacheKey = cacheKey;
    m_cachedPreviewRanges = ranges;
    return ranges;
  }
  if (m_moveEnd <= 0) {
    logRangeIfChanged(0, 0, layerLow, layerHigh);
    m_previewRangeCacheKey = cacheKey;
    m_cachedPreviewRanges = ranges;
    return ranges;
  }

  bool hasOpenRange = false;
  PreviewDrawRange openRange;
  quint32 totalVertexCount = 0;

  const auto flushOpenRange = [&]() {
    if (!hasOpenRange)
      return;
    ranges.append(openRange);
    totalVertexCount += openRange.vertexCount;
    hasOpenRange = false;
    openRange = {};
  };

  for (const auto &span : m_previewDrawSpans) {
    bool visible = true;
    if (span.layer < layerLow || span.layer > layerHigh)
      visible = false;
    if (span.move >= m_moveEnd)
      visible = false;
    if (visible && span.role >= 0 && span.role < m_roleVisibility.size())
      visible = m_roleVisibility[span.role];

    if (!visible) {
      flushOpenRange();
      continue;
    }

    if (!hasOpenRange) {
      openRange.firstVertex = span.vertexOffset;
      openRange.vertexCount = span.vertexCount;
      hasOpenRange = true;
      continue;
    }

    const quint32 expectedNext = openRange.firstVertex + openRange.vertexCount;
    if (span.vertexOffset == expectedNext) {
      openRange.vertexCount += span.vertexCount;
    } else {
      flushOpenRange();
      openRange.firstVertex = span.vertexOffset;
      openRange.vertexCount = span.vertexCount;
      hasOpenRange = true;
    }
  }
  flushOpenRange();

  const quint32 firstVertex = ranges.isEmpty() ? 0 : ranges.first().firstVertex;
  logRangeIfChanged(firstVertex, totalVertexCount, layerLow, layerHigh);

  m_previewRangeCacheKey = cacheKey;
  m_cachedPreviewRanges = ranges;
  return ranges;
}

quint64 RhiViewportRenderer::computePreviewRangeCacheKey() const
{
  // Hash all inputs that affect computePreviewDrawRanges output. A change in
  // any of these must invalidate the cache. Mirrors the visibility predicates
  // in the traversal loop: layer range filter, moveEnd filter, per-role
  // visibility, and the underlying span set itself (count + generation).
  quint64 key = 1469598103934665603ULL;  // FNV-1a 64-bit offset basis
  auto mix = [&key](quint64 v) {
    key ^= v;
    key *= 1099511628211ULL;  // FNV-1a 64-bit prime
  };
  mix(static_cast<quint64>(m_layerMin));
  mix(static_cast<quint64>(m_layerMax));
  mix(static_cast<quint64>(m_moveEnd));
  mix(static_cast<quint64>(m_previewDrawSpans.size()));
  // roleVisibility: pack each bool into the key. QVector<bool> is bit-packed,
  // so iterate explicitly (no direct byte access).
  for (int i = 0; i < m_roleVisibility.size() && i < 32; ++i) {
    if (m_roleVisibility.at(i))
      mix(static_cast<quint64>(1) << i);
  }
  return key;
}

