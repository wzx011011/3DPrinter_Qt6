#include "RhiViewportRenderer.h"
#include "RhiViewport.h"

#include <QFile>

#include <cstring>

namespace {

constexpr RhiViewportRenderer::Vertex kDiagnosticVertices[] = {
    {-0.72f, -0.62f, 0.08f, 0.78f, 0.42f, 1.0f},
    { 0.72f, -0.62f, 0.12f, 0.48f, 0.92f, 1.0f},
    { 0.00f,  0.64f, 0.98f, 0.58f, 0.24f, 1.0f},
};

} // namespace

RhiViewportRenderer::RhiViewportRenderer() = default;

RhiViewportRenderer::~RhiViewportRenderer()
{
  releaseResources();
}

void RhiViewportRenderer::initialize(QRhiCommandBuffer *cb)
{
  Q_UNUSED(cb);
  releaseResources();
  m_pipelineFailed = false;
  m_verticesUploaded = false;
}

void RhiViewportRenderer::synchronize(QQuickRhiItem *item)
{
  const auto *viewport = qobject_cast<RhiViewport *>(item);
  if (viewport == nullptr)
    return;

  m_canvasType = viewport->m_canvasType;
  m_meshBytes = viewport->m_meshData.size();
  m_previewBytes = viewport->m_previewData.size();
  QList<int> activeObjectIndices;
  activeObjectIndices.reserve(viewport->m_activePlateObjectIndices.size());
  for (const QVariant &value : viewport->m_activePlateObjectIndices)
    activeObjectIndices.append(value.toInt());
  m_prepareScene.setBed(viewport->m_bedWidth,
                        viewport->m_bedDepth,
                        viewport->m_bedOriginX,
                        viewport->m_bedOriginY,
                        viewport->m_bedShapeType,
                        viewport->m_bedDiameter);
  m_prepareScene.setShowBed(viewport->m_showBed);
  m_prepareScene.setPlateContext(viewport->m_currentPlateIndex,
                                 viewport->m_plateCount,
                                 activeObjectIndices);
  m_prepareScene.setMeshGeneration(m_meshBytes);
  m_clearColor = (m_canvasType == RhiViewport::CanvasPreview)
      ? QColor(8, 12, 20)
      : QColor(13, 18, 24);
}

void RhiViewportRenderer::render(QRhiCommandBuffer *cb)
{
  if (cb == nullptr || rhi() == nullptr || renderTarget() == nullptr) {
    return;
  }

  QRhiResourceUpdateBatch *updates = nullptr;
  if (!m_verticesUploaded && !m_pipelineFailed) {
    if (uploadVertices())
      updates = rhi()->nextResourceUpdateBatch();
  }

  if (updates != nullptr)
    updates->uploadStaticBuffer(m_vertexBuffer.get(), kDiagnosticVertices);

  cb->beginPass(renderTarget(), m_clearColor, {1.0f, 0}, updates);
  if (ensurePipeline()) {
    cb->setGraphicsPipeline(m_pipeline.get());
    cb->setViewport(QRhiViewport(0, 0, float(renderTarget()->pixelSize().width()),
                                 float(renderTarget()->pixelSize().height())));
    cb->setShaderResources(m_srb.get());
    const QRhiCommandBuffer::VertexInput binding(m_vertexBuffer.get(), 0);
    cb->setVertexInput(0, 1, &binding);
    cb->draw(3);
  }
  cb->endPass();
}

void RhiViewportRenderer::releaseResources()
{
  m_pipeline.reset();
  m_srb.reset();
  m_vertexBuffer.reset();
  m_renderPassDescriptor = nullptr;
  m_verticesUploaded = false;
}

bool RhiViewportRenderer::ensurePipeline()
{
  if (m_pipeline)
    return true;
  if (m_pipelineFailed || rhi() == nullptr || renderTarget() == nullptr)
    return false;

  QShader vertexShader = loadShader(QStringLiteral(":/rhi_viewport/shaders/rhi_viewport.vert.qsb"));
  QShader fragmentShader = loadShader(QStringLiteral(":/rhi_viewport/shaders/rhi_viewport.frag.qsb"));
  if (!vertexShader.isValid() || !fragmentShader.isValid()) {
    m_pipelineFailed = true;
    return false;
  }

  m_srb.reset(rhi()->newShaderResourceBindings());
  m_srb->setBindings({});
  if (!m_srb->create()) {
    m_pipelineFailed = true;
    return false;
  }

  QRhiVertexInputLayout inputLayout;
  inputLayout.setBindings({QRhiVertexInputBinding(sizeof(Vertex))});
  inputLayout.setAttributes({
      QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float2, offsetof(Vertex, x)),
      QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float4, offsetof(Vertex, r)),
  });

  m_renderPassDescriptor = renderTarget()->renderPassDescriptor();
  m_pipeline.reset(rhi()->newGraphicsPipeline());
  m_pipeline->setTopology(QRhiGraphicsPipeline::Triangles);
  m_pipeline->setShaderStages({
      QRhiShaderStage(QRhiShaderStage::Vertex, vertexShader),
      QRhiShaderStage(QRhiShaderStage::Fragment, fragmentShader),
  });
  m_pipeline->setShaderResourceBindings(m_srb.get());
  m_pipeline->setVertexInputLayout(inputLayout);
  m_pipeline->setRenderPassDescriptor(m_renderPassDescriptor);
  if (!m_pipeline->create()) {
    m_pipeline.reset();
    m_pipelineFailed = true;
    return false;
  }

  return true;
}

bool RhiViewportRenderer::uploadVertices()
{
  if (m_vertexBuffer)
  {
    m_verticesUploaded = true;
    return true;
  }

  if (rhi() == nullptr)
    return false;

  m_vertexBuffer.reset(rhi()->newBuffer(QRhiBuffer::Static,
                                        QRhiBuffer::VertexBuffer,
                                        quint32(sizeof(kDiagnosticVertices))));
  if (!m_vertexBuffer->create()) {
    m_vertexBuffer.reset();
    m_pipelineFailed = true;
    return false;
  }

  m_verticesUploaded = true;
  return true;
}

QShader RhiViewportRenderer::loadShader(const QString &path) const
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return {};
  return QShader::fromSerialized(file.readAll());
}
