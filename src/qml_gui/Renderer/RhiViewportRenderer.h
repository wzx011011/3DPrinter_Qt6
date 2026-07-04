#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QQuickRhiItem>
#include <QVector>
#include <QVector3D>

#include <limits>
#include <memory>

#include "PrepareSceneData.h"
#include "core/rendering/GizmoVertex.h"

#include <rhi/qrhi.h>
#include <rhi/qshader.h>

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
                      bool enableDepthWrite = true);
  bool uploadSceneBuffers(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadBedBuffers(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadModelBuffer(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadHighlightBuffer(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadCameraUniform(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool ensureGizmoPipeline();                                  // Phase 68
  bool uploadGizmoBuffer(QRhiResourceUpdateBatch *updates);   // Phase 68
  void renderMoveGizmo(QRhiCommandBuffer *cb);                // Phase 68
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
  // Highlight is translucent (alpha < 1.0): it tests depth (so it occludes
  // behind opaque geometry) but does not WRITE depth (so it does not block
  // geometry drawn after it from passing the depth test).
  std::unique_ptr<QRhiGraphicsPipeline> m_fillPipelineNoDepthWrite;
  // Phase 68: gizmo pipelines. Separate from m_fill/m_line because the gizmo
  // shader applies position*scale+center displacement. Lines for shafts,
  // triangles for cones/rings/boxes. No depth write so the gizmo stays
  // visible through objects (matches GL glClear(GL_DEPTH_BUFFER_BIT) before
  // each gizmo render).
  std::unique_ptr<QRhiGraphicsPipeline> m_gizmoLinePipeline;
  std::unique_ptr<QRhiGraphicsPipeline> m_gizmoTriPipeline;
  std::unique_ptr<QRhiBuffer> m_gizmoVertexBuffer;
  bool m_gizmoVertexBufferUploaded = false;
  bool m_gizmoPipelineCreated = false;
  quint32 m_gizmoVertexBufferBytes = 0;
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
  int m_canvasType = 0;
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
};
