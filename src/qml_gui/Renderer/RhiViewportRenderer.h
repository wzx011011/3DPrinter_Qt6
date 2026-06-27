#pragma once

#include <QColor>
#include <QMatrix4x4>
#include <QQuickRhiItem>
#include <QVector>

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

  RhiViewportRenderer();
  ~RhiViewportRenderer() override;

protected:
  void initialize(QRhiCommandBuffer *cb) override;
  void synchronize(QQuickRhiItem *item) override;
  void render(QRhiCommandBuffer *cb) override;

private:
  void releaseResources();
  bool ensurePipelines();
  bool ensurePipeline(std::unique_ptr<QRhiGraphicsPipeline> &pipeline,
                      QRhiGraphicsPipeline::Topology topology);
  bool uploadSceneBuffers(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadBedBuffers(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadModelBuffer(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadCameraUniform(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool ensureBuffer(std::unique_ptr<QRhiBuffer> &buffer,
                    quint32 byteSize,
                    quint32 &storedSize,
                    QRhiBuffer::UsageFlags usage);
  QVector<Vertex> buildSceneVertices(const QList<PrepareSceneData::Vertex> &source) const;
  QVector<Vertex> buildModelVertices(const QList<PrepareSceneData::ModelVertex> &source) const;
  QShader loadShader(const QString &path) const;

  std::unique_ptr<QRhiBuffer> m_bedFillBuffer;
  std::unique_ptr<QRhiBuffer> m_bedLineBuffer;
  std::unique_ptr<QRhiBuffer> m_modelVertexBuffer;
  std::unique_ptr<QRhiBuffer> m_cameraUniformBuffer;
  std::unique_ptr<QRhiShaderResourceBindings> m_srb;
  std::unique_ptr<QRhiGraphicsPipeline> m_fillPipeline;
  std::unique_ptr<QRhiGraphicsPipeline> m_linePipeline;
  QRhiRenderPassDescriptor *m_renderPassDescriptor = nullptr;
  bool m_sceneBuffersUploaded = false;
  bool m_modelVertexBufferUploaded = false;
  bool m_cameraUniformBufferUploaded = false;
  bool m_pipelineFailed = false;
  quint32 m_bedFillBufferBytes = 0;
  quint32 m_bedLineBufferBytes = 0;
  quint32 m_modelVertexBufferBytes = 0;
  quint32 m_cameraUniformBufferBytes = 0;
  quint32 m_bedFillVertexCount = 0;
  quint32 m_bedLineVertexCount = 0;
  quint32 m_modelVertexCount = 0;
  int m_canvasType = 0;
  int m_meshBytes = 0;
  int m_previewBytes = 0;
  qint64 m_sceneGeneration = 0;
  qint64 m_modelGeneration = 0;
  PrepareSceneData m_prepareScene;
  QMatrix4x4 m_cameraMvp;
  QColor m_clearColor = QColor(14, 20, 28);
};
