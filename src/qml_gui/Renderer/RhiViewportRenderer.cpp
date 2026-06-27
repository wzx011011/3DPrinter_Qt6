#include "RhiViewportRenderer.h"
#include "RhiViewport.h"

#include <QFile>

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
  releaseResources();
  m_pipelineFailed = false;
  m_sceneBuffersUploaded = false;
}

void RhiViewportRenderer::synchronize(QQuickRhiItem *item)
{
  auto *viewport = qobject_cast<RhiViewport *>(item);
  if (viewport == nullptr)
    return;

  m_canvasType = viewport->m_canvasType;
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
      : QColor(13, 18, 24);
}

void RhiViewportRenderer::render(QRhiCommandBuffer *cb)
{
  if (cb == nullptr || rhi() == nullptr || renderTarget() == nullptr) {
    return;
  }

  QRhiResourceUpdateBatch *updates = nullptr;
  quint32 dirtyFlags = m_prepareScene.peekDirtyFlags();
  if (m_canvasType == RhiViewport::CanvasView3D) {
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
        m_prepareScene.takeDirtyFlags();
      }
    }
  }

  cb->beginPass(renderTarget(), m_clearColor, {1.0f, 0}, updates);
  if (m_canvasType == RhiViewport::CanvasView3D && ensurePipelines()) {
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
    if (m_highlightVertexBuffer && m_highlightVertexCount > 0) {
      cb->setGraphicsPipeline(m_fillPipeline.get());
      const QRhiCommandBuffer::VertexInput highlightBinding(m_highlightVertexBuffer.get(), 0);
      cb->setVertexInput(0, 1, &highlightBinding);
      cb->draw(m_highlightVertexCount);
    }
  }
  cb->endPass();
}

void RhiViewportRenderer::releaseResources()
{
  m_linePipeline.reset();
  m_fillPipeline.reset();
  m_srb.reset();
  m_cameraUniformBuffer.reset();
  m_highlightVertexBuffer.reset();
  m_modelVertexBuffer.reset();
  m_bedLineBuffer.reset();
  m_bedFillBuffer.reset();
  m_renderPassDescriptor = nullptr;
  m_sceneBuffersUploaded = false;
  m_modelVertexBufferUploaded = false;
  m_highlightVertexBufferUploaded = false;
  m_cameraUniformBufferUploaded = false;
  m_bedFillBufferBytes = 0;
  m_bedLineBufferBytes = 0;
  m_modelVertexBufferBytes = 0;
  m_highlightVertexBufferBytes = 0;
  m_cameraUniformBufferBytes = 0;
  m_bedFillVertexCount = 0;
  m_bedLineVertexCount = 0;
  m_modelVertexCount = 0;
  m_highlightVertexCount = 0;
  m_sceneGeneration = 0;
  m_modelGeneration = 0;
}

bool RhiViewportRenderer::ensurePipelines()
{
  if (m_fillPipeline && m_linePipeline)
    return true;
  if (m_pipelineFailed || rhi() == nullptr || renderTarget() == nullptr)
    return false;

  if (!m_cameraUniformBuffer
      && !ensureBuffer(m_cameraUniformBuffer,
                       64,
                       m_cameraUniformBufferBytes,
                       QRhiBuffer::UniformBuffer)) {
    m_cameraUniformBufferBytes = 0;
    m_pipelineFailed = true;
    return false;
  }

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

  return ensurePipeline(m_fillPipeline, QRhiGraphicsPipeline::Triangles)
      && ensurePipeline(m_linePipeline, QRhiGraphicsPipeline::Lines);
}

bool RhiViewportRenderer::ensurePipeline(std::unique_ptr<QRhiGraphicsPipeline> &pipeline,
                                         QRhiGraphicsPipeline::Topology topology)
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

bool RhiViewportRenderer::uploadCameraUniform(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags)
{
  if (updates == nullptr || rhi() == nullptr)
    return false;

  const bool uploadCamera = !m_cameraUniformBufferUploaded
      || (dirtyFlags & (PrepareSceneData::DirtyCamera | PrepareSceneData::DirtyGpu)) != 0;
  if (!uploadCamera)
    return true;

  if (!ensureBuffer(m_cameraUniformBuffer, 64, m_cameraUniformBufferBytes, QRhiBuffer::UniformBuffer))
    return false;

  const QMatrix4x4 corrected = rhi()->clipSpaceCorrMatrix() * m_cameraMvp;
  updates->updateDynamicBuffer(m_cameraUniformBuffer.get(), 0, 64, corrected.constData());
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
