#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QQuickRhiItem>
#include <QVector>

#include <limits>
#include <memory>

#include "PrepareSceneData.h"

#include <rhi/qrhi.h>
#include <rhi/qshader.h>

class RhiViewportRenderer : public QQuickRhiItemRenderer
{
public:
  struct Vertex
  {
    float x;
    float y;
    float z;
    float r;
    float g;
    float b;
    float a;
  };
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
                      QRhiGraphicsPipeline::Topology topology);
  bool uploadSceneBuffers(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadBedBuffers(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadModelBuffer(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadHighlightBuffer(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadCameraUniform(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool ensureBuffer(std::unique_ptr<QRhiBuffer> &buffer,
                    quint32 byteSize,
                    quint32 &storedSize,
                    QRhiBuffer::UsageFlags usage);

  // ── Phase 26: Preview segment pipeline (D-26-01..04) ──
  void parsePreviewSegments();
  bool uploadPreviewSegmentBuffer(QRhiResourceUpdateBatch *updates);
  QVector<PreviewDrawRange> computePreviewDrawRanges() const;

  QVector<Vertex> buildSceneVertices(const QList<PrepareSceneData::Vertex> &source) const;
  QVector<Vertex> buildModelVertices(const QList<PrepareSceneData::ModelVertex> &source) const;
  QVector<Vertex> buildHighlightVertices() const;
  QShader loadShader(const QString &path) const;

  std::unique_ptr<QRhiBuffer> m_bedFillBuffer;
  std::unique_ptr<QRhiBuffer> m_bedLineBuffer;
  std::unique_ptr<QRhiBuffer> m_modelVertexBuffer;
  std::unique_ptr<QRhiBuffer> m_highlightVertexBuffer;
  std::unique_ptr<QRhiBuffer> m_cameraUniformBuffer;
  std::unique_ptr<QRhiShaderResourceBindings> m_srb;
  std::unique_ptr<QRhiGraphicsPipeline> m_fillPipeline;
  std::unique_ptr<QRhiGraphicsPipeline> m_linePipeline;
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
