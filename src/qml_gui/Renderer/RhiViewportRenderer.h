#pragma once

#include <QColor>
#include <QQuickRhiItem>

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
  bool ensurePipeline();
  bool uploadVertices();
  QShader loadShader(const QString &path) const;

  std::unique_ptr<QRhiBuffer> m_vertexBuffer;
  std::unique_ptr<QRhiShaderResourceBindings> m_srb;
  std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;
  QRhiRenderPassDescriptor *m_renderPassDescriptor = nullptr;
  bool m_verticesUploaded = false;
  bool m_pipelineFailed = false;
  int m_canvasType = 0;
  int m_meshBytes = 0;
  int m_previewBytes = 0;
  PrepareSceneData m_prepareScene;
  QColor m_clearColor = QColor(14, 20, 28);
};
