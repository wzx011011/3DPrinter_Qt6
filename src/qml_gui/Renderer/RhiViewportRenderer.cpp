#include "RhiViewportRenderer.h"
#include "RhiViewport.h"
#include "core/rendering/GizmoCenter.h"
#include "core/rendering/GizmoGeometry.h"

#include <QDebug>
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
        // Phase 68: upload the gizmo vertex buffer in the same batch.
        uploadGizmoBuffer(updates);
        m_prepareScene.takeDirtyFlags();
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
      // Highlight is translucent: test depth but do not write it, so it does
      // not occlude opaque geometry drawn in subsequent frames/passes.
      cb->setGraphicsPipeline(m_fillPipelineNoDepthWrite.get());
      const QRhiCommandBuffer::VertexInput highlightBinding(m_highlightVertexBuffer.get(), 0);
      cb->setVertexInput(0, 1, &highlightBinding);
      cb->draw(m_highlightVertexCount);
    }
    // Phase 68: render the move gizmo (X/Y/Z arrows) when gizmoMode == Move.
    // Drawn after meshes/highlight so it sits on top via no-depth-write.
    renderMoveGizmo(cb);
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
}

void RhiViewportRenderer::releaseResources()
{
  m_linePipeline.reset();
  m_fillPipeline.reset();
  m_fillPipelineNoDepthWrite.reset();
  // Phase 68: gizmo pipelines + vertex buffer.
  m_gizmoLinePipeline.reset();
  m_gizmoTriPipeline.reset();
  m_gizmoVertexBuffer.reset();
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
  m_gizmoPipelineCreated = false;            // Phase 68
  m_bedFillBufferBytes = 0;
  m_bedLineBufferBytes = 0;
  m_modelVertexBufferBytes = 0;
  m_highlightVertexBufferBytes = 0;
  m_cameraUniformBufferBytes = 0;
  m_gizmoVertexBufferBytes = 0;              // Phase 68
  m_bedFillVertexCount = 0;
  m_bedLineVertexCount = 0;
  m_modelVertexCount = 0;
  m_highlightVertexCount = 0;
  m_sceneGeneration = 0;
  m_modelGeneration = 0;
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
  if (m_fillPipeline && m_linePipeline)
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
      && ensurePipeline(m_linePipeline, QRhiGraphicsPipeline::Lines)
      && ensurePipeline(m_fillPipelineNoDepthWrite, QRhiGraphicsPipeline::Triangles,
                        /*enableDepthWrite=*/false);
}

bool RhiViewportRenderer::ensurePipeline(std::unique_ptr<QRhiGraphicsPipeline> &pipeline,
                                         QRhiGraphicsPipeline::Topology topology,
                                         bool enableDepthWrite)
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
  // Standard color blend (no blending for opaque scene geometry).
  pipeline->setTargetBlends({});
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

  // Build the move gizmo vertices (shaft lines + cone triangles, per-axis
  // colored). Phase 66 GizmoGeometry produces the same layout meshes use.
  // Per-axis draw offsets are hardcoded in renderMoveGizmo (kShaftBase/kConeBase).
  QVector<GizmoVertex> verts = GizmoGeometry::buildMoveGizmoVertices();

  const quint32 byteSize = quint32(verts.size() * sizeof(GizmoVertex));
  if (!ensureBuffer(m_gizmoVertexBuffer, byteSize, m_gizmoVertexBufferBytes,
                    QRhiBuffer::VertexBuffer))
    return false;

  updates->uploadStaticBuffer(m_gizmoVertexBuffer.get(), 0, byteSize, verts.constData());
  m_gizmoVertexBufferUploaded = true;
  qInfo("[RHI] gizmo vertex buffer uploaded: %u move verts", quint32(verts.size()));
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
    // Gizmo tests depth (occludes behind itself correctly) but does NOT write
    // depth, so it stays visible through objects drawn after it. Matches the
    // GL path's glClear(GL_DEPTH_BUFFER_BIT) before each gizmo render.
    pipe->setDepthTest(true);
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
  qInfo("[RHI] gizmo pipelines created (lines + triangles, no-depth-write)");
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

  // The move gizmo layout from GizmoGeometry (3 axes x 38 verts each):
  //   axis 0 (X): shaft [0,1], cone [2..37]
  //   axis 1 (Y): shaft [38,39], cone [40..75]
  //   axis 2 (Z): shaft [76,77], cone [78..113]
  // Shafts use Lines topology (pair verts); cones use Triangles. The verts
  // are NOT contiguous across axes, so issue per-axis draw calls (matches
  // the GL path's glDrawArrays-per-axis loop).
  static const int kShaftBase[3] = {0, 38, 76};
  static const int kConeBase[3] = {2, 40, 78};
  const int kShaftVertsPerAxis = 2;
  const int kConeVertsPerAxis = 36;

  // Shafts (GL_LINES).
  if (m_gizmoLinePipeline)
  {
    cb->setGraphicsPipeline(m_gizmoLinePipeline.get());
    cb->setVertexInput(0, 1, &gizmoBinding);
    for (int ax = 0; ax < 3; ++ax)
      cb->draw(kShaftVertsPerAxis, 1, kShaftBase[ax]);
  }
  // Cones (GL_TRIANGLES).
  if (m_gizmoTriPipeline)
  {
    cb->setGraphicsPipeline(m_gizmoTriPipeline.get());
    cb->setVertexInput(0, 1, &gizmoBinding);
    for (int ax = 0; ax < 3; ++ax)
      cb->draw(kConeVertsPerAxis, 1, kConeBase[ax]);
  }
}

// ── Phase 26: Preview segment pipeline ──────────────────────────────────────
// GCV1 wire format from PreviewViewModel: "GCV1" magic + int count +
// count * PackedSegment (76 bytes each). Each segment → 2 Line vertices
// (start xyz + end xyz, sharing RGBA), with GCode y↔z axis swap to GL space.
// Layer ranges are indexed per-layer for GPU draw-range filtering (D-26-02).
// Color is CPU-pre-baked by PreviewViewModel (D-26-03); renderer is opaque RGBA.

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
    // Axis swap: GCode (x, y, z) → GL (x, z, y) — mirror GCodeRenderer.cpp:527-528.
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

