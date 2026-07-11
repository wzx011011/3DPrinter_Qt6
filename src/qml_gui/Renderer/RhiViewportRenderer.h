#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QImage>
#include <QMatrix4x4>
#include <QPointer>
#include <QQuickRhiItem>
#include <QVector>
#include <QVector3D>

#include <limits>
#include <memory>

#include "PrepareSceneData.h"
#include "core/rendering/GizmoGeometry.h"
#include "core/rendering/GizmoVertex.h"

#include <rhi/qrhi.h>
#include <rhi/qshader.h>

class RhiViewport;
class RhiViewportRenderer : public QQuickRhiItemRenderer
{
public:
  // Vertex layout is defined in core/rendering/GizmoVertex.h (shared with
  // the gizmo geometry builders so they can produce vertices without pulling
  // in this heavy header). Alias kept for call-site compatibility.
  using Vertex = GizmoVertex;
  struct PreviewDrawRange
  {
    quint32 firstVertex = 0;
    quint32 vertexCount = 0;
  };

  RhiViewportRenderer();
  ~RhiViewportRenderer() override;

protected:
  void initialize(QRhiCommandBuffer *cb) override;
  void synchronize(QQuickRhiItem *item) override;
  void render(QRhiCommandBuffer *cb) override;

private:
  void releaseResources();
  void resetPreviewGpuState(bool keepCpuStaging);
  bool ensurePipelines();
  bool ensurePipeline(std::unique_ptr<QRhiGraphicsPipeline> &pipeline,
                      QRhiGraphicsPipeline::Topology topology,
                      bool enableDepthWrite = true,
                      bool enableBlending = false);
  bool uploadSceneBuffers(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadBedBuffers(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadModelBuffer(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadHighlightBuffer(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadCutPlaneBuffers(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadWipeTowerBuffer(QRhiResourceUpdateBatch *updates);
  // Phase 91 (ASMEXPLODE-02): yellow dashed connector guide lines between
  // originally-adjacent volumes of the same object on AssembleView when
  // ratio > 1.0 (matches shotScreen/装配页_爆炸.png).
  bool uploadAssemblyConnectorBuffer(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  void renderAssemblyConnectors(QRhiCommandBuffer *cb);
  // Phase 92 (ASMMEASURE-02): Assembly measurement overlay (white dashed
  // dimension line + arrowheads + teal value box) between the two selected
  // volumes. Gated to m_gizmoMode == GizmoAssemblyMeasure on CanvasAssembleView.
  bool uploadAssemblyMeasureBuffers(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  void renderAssemblyMeasureOverlay(QRhiCommandBuffer *cb);
  bool uploadCameraUniform(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool ensureGizmoPipeline();                                  // Phase 68
  bool uploadGizmoBuffer(QRhiResourceUpdateBatch *updates);   // Phase 68/70
  void renderMoveGizmo(QRhiCommandBuffer *cb);                // Phase 68
  void renderRotateGizmo(QRhiCommandBuffer *cb);              // Phase 70
  void renderScaleGizmo(QRhiCommandBuffer *cb);               // Phase 70
  void renderCutPlane(QRhiCommandBuffer *cb);                  // Phase 71
  void renderWipeTower(QRhiCommandBuffer *cb);                 // Phase 71
  bool ensureBuffer(std::unique_ptr<QRhiBuffer> &buffer,
                    quint32 byteSize,
                    quint32 &storedSize,
                    QRhiBuffer::UsageFlags usage);

  // ── Phase 26: Preview segment pipeline (D-26-01..04) ──
  void parsePreviewSegments();
  bool uploadPreviewSegmentBuffer(QRhiResourceUpdateBatch *updates);
  QVector<PreviewDrawRange> computePreviewDrawRanges() const;
  quint64 computePreviewRangeCacheKey() const;

  QVector<Vertex> buildSceneVertices(const QList<PrepareSceneData::Vertex> &source) const;
  QVector<Vertex> buildModelVertices(const QList<PrepareSceneData::ModelVertex> &source) const;
  QVector<Vertex> buildHighlightVertices() const;
  QShader loadShader(const QString &path) const;

  // ── Phase 95 (THUMBCAP-01/02/03): offscreen thumbnail capture ──
  // Real QRhi texture readback replacing the solid-color stub. The thumbnail
  // is rendered into a separate single-sample offscreen QRhiTexture RT at the
  // requested size, then read back via QRhiResourceUpdateBatch::readBackTexture.
  // The request crosses GUI->render via synchronize() (mirroring the
  // m_fitRequestCount pattern); the QImage crosses back via a queued
  // QMetaObject::invokeMethod to RhiViewport::deliverThumbnail.
  bool ensureThumbnailRenderTarget(int size);
  void releaseThumbnailResources();
  void renderThumbnailPass(QRhiCommandBuffer *cb);
  void issueThumbnailReadback(QRhiResourceUpdateBatch *updates);
  void deliverCompletedThumbnail();
  // Phase 67: instance helper forwarding to the static testable one.
  QVector3D computeGizmoCenter() const;

  std::unique_ptr<QRhiBuffer> m_bedFillBuffer;
  std::unique_ptr<QRhiBuffer> m_bedLineBuffer;
  std::unique_ptr<QRhiBuffer> m_modelVertexBuffer;
  std::unique_ptr<QRhiBuffer> m_highlightVertexBuffer;
  std::unique_ptr<QRhiBuffer> m_cameraUniformBuffer;
  std::unique_ptr<QRhiShaderResourceBindings> m_srb;
  std::unique_ptr<QRhiGraphicsPipeline> m_fillPipeline;
  std::unique_ptr<QRhiGraphicsPipeline> m_linePipeline;
  // Translucent cut/wipe/highlight pipelines test depth but do not write it,
  // and enable source-alpha blending so baked vertex alpha is visible.
  std::unique_ptr<QRhiGraphicsPipeline> m_translucentFillPipeline;
  std::unique_ptr<QRhiGraphicsPipeline> m_translucentLinePipeline;
  // Phase 68: gizmo pipelines. Separate from m_fill/m_line because the gizmo
  // shader applies position*scale+center displacement. Lines for shafts,
  // triangles for cones/rings/boxes. No depth write so the gizmo stays
  // visible through objects (matches GL glClear(GL_DEPTH_BUFFER_BIT) before
  // each gizmo render).
  std::unique_ptr<QRhiGraphicsPipeline> m_gizmoLinePipeline;
  std::unique_ptr<QRhiGraphicsPipeline> m_gizmoTriPipeline;
  std::unique_ptr<QRhiBuffer> m_gizmoVertexBuffer;
  std::unique_ptr<QRhiBuffer> m_cutPlaneFillBuffer;
  std::unique_ptr<QRhiBuffer> m_cutPlaneOutlineBuffer;
  std::unique_ptr<QRhiBuffer> m_wipeTowerBuffer;
  // Phase 91 (ASMEXPLODE-02): assembly connector guide-line buffer.
  std::unique_ptr<QRhiBuffer> m_assemblyConnectorBuffer;
  // Phase 92 (ASMMEASURE-02): Assembly measurement overlay buffers. The
  // dimension line uses the line pipeline (white GL_LINES dashes); the
  // arrowheads + teal value-box quad use the triangle pipelines (gizmo tri
  // for white arrowheads, translucent fill for the teal box). Drawn only when
  // m_gizmoMode == GizmoAssemblyMeasure on CanvasAssembleView.
  std::unique_ptr<QRhiBuffer> m_assemblyMeasureLineBuffer;
  std::unique_ptr<QRhiBuffer> m_assemblyMeasureTriBuffer;
  std::unique_ptr<QRhiBuffer> m_assemblyMeasureValueBuffer;
  bool m_gizmoVertexBufferUploaded = false;
  bool m_cutPlaneFillBufferUploaded = false;
  bool m_cutPlaneOutlineBufferUploaded = false;
  bool m_wipeTowerBufferUploaded = false;
  bool m_assemblyConnectorBufferUploaded = false;
  bool m_assemblyMeasureLineBufferUploaded = false;
  bool m_assemblyMeasureTriBufferUploaded = false;
  bool m_assemblyMeasureValueBufferUploaded = false;
  bool m_gizmoPipelineCreated = false;
  quint32 m_gizmoVertexBufferBytes = 0;
  quint32 m_cutPlaneFillBufferBytes = 0;
  quint32 m_cutPlaneOutlineBufferBytes = 0;
  quint32 m_wipeTowerBufferBytes = 0;
  quint32 m_assemblyConnectorBufferBytes = 0;
  quint32 m_assemblyMeasureLineBufferBytes = 0;
  quint32 m_assemblyMeasureTriBufferBytes = 0;
  quint32 m_assemblyMeasureValueBufferBytes = 0;
  GizmoGeometryOffsets m_moveGizmoOffsets;
  GizmoGeometryOffsets m_rotateGizmoOffsets;
  GizmoGeometryOffsets m_scaleGizmoOffsets;
  QRhiRenderPassDescriptor *m_renderPassDescriptor = nullptr;
  bool m_sceneBuffersUploaded = false;
  bool m_modelVertexBufferUploaded = false;
  bool m_highlightVertexBufferUploaded = false;
  bool m_cameraUniformBufferUploaded = false;
  bool m_pipelineFailed = false;
  quint32 m_bedFillBufferBytes = 0;
  quint32 m_bedLineBufferBytes = 0;
  quint32 m_modelVertexBufferBytes = 0;
  quint32 m_highlightVertexBufferBytes = 0;
  quint32 m_cameraUniformBufferBytes = 0;
  quint32 m_bedFillVertexCount = 0;
  quint32 m_bedLineVertexCount = 0;
  quint32 m_modelVertexCount = 0;
  quint32 m_highlightVertexCount = 0;
  quint32 m_cutPlaneFillVertexCount = 0;
  quint32 m_cutPlaneOutlineVertexCount = 0;
  quint32 m_wipeTowerVertexCount = 0;
  quint32 m_assemblyConnectorVertexCount = 0;
  quint32 m_assemblyMeasureLineVertexCount = 0;
  quint32 m_assemblyMeasureTriVertexCount = 0;
  quint32 m_assemblyMeasureValueVertexCount = 0;
  int m_canvasType = 0;
  // Phase 91 (ASMEXPLODE-02): explosion ratio mirrored from RhiViewport in
  // synchronize(). Drives the per-volume offset in buildModelVertices when the
  // active canvas is CanvasAssembleView. m_lastExplosionRatio forces a model
  // re-upload when the ratio changes (default 1.0 == no offset).
  float m_explosionRatio = 1.0f;
  float m_lastExplosionRatio = 1.0f;
  // Phase 92 (ASMMEASURE-02): the two selected source indices the overlay
  // annotates. Mirrored from RhiViewport in synchronize(); a change forces an
  // overlay re-upload. Default -1 = not set (nothing drawn).
  int m_assemblyMeasureSelectedA = -1;
  int m_assemblyMeasureSelectedB = -1;
  int m_assemblyMeasureLastSelectedA = -1;
  int m_assemblyMeasureLastSelectedB = -1;
  int m_meshBytes = 0;
  int m_previewBytes = 0;
  qint64 m_sceneGeneration = 0;
  qint64 m_modelGeneration = 0;
  PrepareSceneData m_prepareScene;
  QMatrix4x4 m_cameraMvp;
  QColor m_clearColor = QColor(14, 20, 28);

  // ── Phase 67: Gizmo state read from RhiViewport in synchronize() ──
  // The viewport item owns gizmoMode/cutAxis/cutPosition as Q_PROPERTY values;
  // the renderer mirrors them here so render() (Phase 68+) can branch on them.
  // gizmoCenter is computed from the selected object's AABB midpoint via the
  // free function GizmoCenter::fromSelectedBatch (src/core/rendering/GizmoCenter.h),
  // which is unit-tested independently.
  int m_gizmoMode = 0;          // RhiViewport::GizmoMode (0=Move, 1=Rotate, 2=Scale, 5=Cut, ...)
  int m_cutAxis = 2;            // 0=X, 1=Y, 2=Z (default Z)
  float m_cutPosition = 0.f;    // cut-plane offset along cutAxis (mm)
  QVector3D m_gizmoCenter;      // midpoint of the selected batch's bounds; origin if no selection
  QVector3D m_cameraEye;        // Phase 68: camera position for gizmoScale computation
  bool m_cutPlaneDirty = true;
  bool m_showWipeTower = false;
  float m_wipeTowerWidth = 10.f;
  float m_wipeTowerDepth = 10.f;
  float m_wipeTowerHeight = 50.f;
  float m_wipeTowerX = 100.f;
  float m_wipeTowerZ = 25.f;
  bool m_wipeTowerDirty = true;

  // ── Phase 26: Preview segment pipeline state ──
  QByteArray m_previewData;              // GCV1 blob from RhiViewport
  int m_layerMin = 0;
  int m_layerMax = 0;
  int m_moveEnd = 0;
  bool m_showTravelMoves = true;
  int m_gcodeViewMode = 0;
  QVector<bool> m_roleVisibility;  ///< Per-role extrusion mask from RhiViewport (render-side skip).
  QVector<Vertex> m_previewVertices;     // expanded Line vertices (CPU staging)
  struct PreviewDrawSpan {
    int layer;
    int move;
    quint32 vertexOffset;
    quint32 vertexCount;
    int role;  ///< Canonical libvgcode EGCodeExtrusionRole index for render-side filtering.
  };
  QVector<PreviewDrawSpan> m_previewDrawSpans;
  std::unique_ptr<QRhiBuffer> m_previewSegmentBuffer;
  quint32 m_previewSegmentBufferBytes = 0;
  quint32 m_previewSegmentVertexCount = 0;
  bool m_previewSegmentBufferUploaded = false;

  // Cache for computePreviewDrawRanges — the function is called every render
  // frame but its inputs (layer range, moveEnd, role visibility, span set)
  // change rarely. Without this, each idle frame re-traverses the full span
  // vector (can be millions of spans for large G-code). The cache is keyed by
  // a hash of all inputs; m_previewDrawSpans rebuild (parsePreviewSegments)
  // and span-set mutations invalidate it by resetting the key to 0.
  // Members are mutable because computePreviewDrawRanges is const.
  mutable quint64 m_previewRangeCacheKey = 0;
  mutable QVector<PreviewDrawRange> m_cachedPreviewRanges;
  mutable quint32 m_lastLoggedPreviewFirstVertex = std::numeric_limits<quint32>::max();
  mutable quint32 m_lastLoggedPreviewVertexCount = std::numeric_limits<quint32>::max();
  mutable int m_lastLoggedPreviewLayerLow = std::numeric_limits<int>::min();
  mutable int m_lastLoggedPreviewLayerHigh = std::numeric_limits<int>::min();
  mutable int m_lastLoggedPreviewMoveEnd = std::numeric_limits<int>::min();

  // ── Phase 27: Preview performance instrumentation (PERF-01) ──
  QElapsedTimer m_previewFrameTimer;
  qint64 m_previewLastUploadMs = -1;
  qint64 m_previewLastFrameMs = -1;
  qint64 m_previewFirstFrameMs = -1;
  bool m_previewFirstFrameDone = false;

  // ── Phase 95 (THUMBCAP-01/02/03): offscreen thumbnail capture state ──
  // The offscreen RT is single-sample (sample count 1) so NO MSAA resolve is
  // needed at readback (frozen decision 2). It has its own render-pass
  // descriptor because the thumbnail pipelines cannot share the on-screen
  // renderTarget()'s RPD. The thumbnail pipelines reuse the same .qsb shaders
  // and vertex layout as m_fillPipeline/m_linePipeline but are separate
  // instances bound to the thumbnail RPD.
  std::unique_ptr<QRhiTexture> m_thumbnailTexture;
  std::unique_ptr<QRhiTextureRenderTarget> m_thumbnailRenderTarget;
  QRhiRenderPassDescriptor *m_thumbnailRenderPassDescriptor = nullptr;
  std::unique_ptr<QRhiGraphicsPipeline> m_thumbnailFillPipeline;
  std::unique_ptr<QRhiGraphicsPipeline> m_thumbnailLinePipeline;
  // Dedicated uniform buffer + SRB for the thumbnail pass (Phase 95 REVIEW W-2).
  // The thumbnail pass previously overwrote the shared m_cameraUniformBuffer's
  // MVP and relied on a next-frame refresh to restore the on-screen value. A
  // dedicated buffer + SRB removes that modify-restore coupling entirely: the
  // thumbnail pipelines bind m_thumbnailSrb (pointing at m_thumbnailUniformBuffer)
  // so the on-screen camera UBO is never touched by a capture.
  std::unique_ptr<QRhiBuffer> m_thumbnailUniformBuffer;
  std::unique_ptr<QRhiShaderResourceBindings> m_thumbnailSrb;
  quint32 m_thumbnailUniformBufferBytes = 0;
  // Request mirror (copied from the item in synchronize()).
  bool m_thumbnailRequestPending = false;
  int m_thumbnailPlateIndex = 0;
  int m_thumbnailSize = 128;
  int m_thumbnailLastBuiltSize = 0;
  // Async readback state: readBackTexture completes on a later frame, so the
  // renderer polls m_thumbnailReadbackResult at the start of render() and
  // delivers the QImage only when data is populated.
  bool m_thumbnailReadbackInFlight = false;
  QRhiReadbackResult m_thumbnailReadbackResult;
  int m_thumbnailResultPlateIndex = 0;
  int m_thumbnailResultSize = 0;
  // Item pointer for the queued callback (QPointer survives item recreation
  // and nulls itself if the item is destroyed before the readback completes).
  QPointer<RhiViewport> m_viewportItem;
};
