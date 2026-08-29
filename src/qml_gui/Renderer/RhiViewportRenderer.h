#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QHash>
#include <QImage>
#include <QMatrix4x4>
#include <QPointer>
#include <QQuickRhiItem>
#include <QVariant>
#include <QVariantList>
#include <QVector>
#include <QVector3D>

#include <limits>
#include <memory>
#include <vector>

#include "PrepareSceneData.h"
#include "core/rendering/GizmoGeometry.h"
#include "core/rendering/GizmoVertex.h"
#include "core/rendering/NavigatorCube.h"

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
    // P17.2: true = extrusion prism spans (triangle topology), false = lines.
    bool triangles = false;
  };

  RhiViewportRenderer();
  ~RhiViewportRenderer() override;

protected:
  void initialize(QRhiCommandBuffer *cb) override;
  void synchronize(QQuickRhiItem *item) override;
  void render(QRhiCommandBuffer *cb) override;

private:
  void releaseResources();
  void releaseRenderPassDependentResources();
  void resetPreviewGpuState(bool keepCpuStaging);
  bool ensurePipelines();
  bool ensurePipeline(std::unique_ptr<QRhiGraphicsPipeline> &pipeline,
                      QRhiGraphicsPipeline::Topology topology,
                      bool enableDepthWrite = true,
                      bool enableBlending = false,
                      QRhiShaderResourceBindings *srb = nullptr,
                      bool enableDepthTest = true);
  bool uploadSceneBuffers(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadBedBuffers(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadModelBuffer(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadHighlightBuffer(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadCutPlaneBuffers(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags);
  bool uploadWipeTowerBuffer(QRhiResourceUpdateBatch *updates);
  // Phase 121 (PAINT-02/OV-03): parse m_paintOverlayData bytes -> GizmoVertex
  // (per-state color) -> ensureBuffer + uploadStaticBuffer. Reuses the mesh
  // pipeline (m_fillPipeline); no new shader/pipeline.
  bool uploadPaintOverlayBuffer(QRhiResourceUpdateBatch *updates);
  void renderPaintOverlay(QRhiCommandBuffer *cb);
  // Phase HOLLOW: drain-hole marker disc upload + render (translucent fill).
  bool uploadHollowMarkerBuffer(QRhiResourceUpdateBatch *updates);
  void renderHollowMarkers(QRhiCommandBuffer *cb);
  // v5.13: connector-pin marker upload + render (AdvancedCut gizmo).
  bool uploadAdvancedCutMarkerBuffer(QRhiResourceUpdateBatch *updates);
  void renderAdvancedCutMarkers(QRhiCommandBuffer *cb);
  // Phase 240 (GIZ-05): measure overlay line upload + render (line
  // pipeline, GizmoMeasure only). Parses m_measureOverlayData
  // ([int32 segmentCount][x,y,z pairs]) into GizmoVertex line soup.
  bool uploadMeasureOverlayBuffer(QRhiResourceUpdateBatch *updates);
  void renderMeasureOverlay(QRhiCommandBuffer *cb);
  // Phase 240 (GIZ-03): flatten hovered-facet highlight upload + render
  // (translucent fill pipeline, GizmoFlatten only). Parses
  // m_flattenHoverData ([int32 vertexCount][x,y,z triples]).
  bool uploadFlattenHoverBuffer(QRhiResourceUpdateBatch *updates);
  void renderFlattenHover(QRhiCommandBuffer *cb);
  // Phase 121 (PAINT-03/OV-05): translucent sphere cursor that follows the
  // mouse while a paint gizmo is active. Built from buildBrushSphereVertices.
  bool uploadBrushCursorBuffer(QRhiResourceUpdateBatch *updates);
  void renderBrushCursor(QRhiCommandBuffer *cb);
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
  // v5.15 (BEDTEX/MODELLIT): bed texture upload (image + quad) and the
  // per-face model normal upload for the lit pipeline.
  bool ensureBedTexturePipeline();
  void uploadBedTexture(QRhiResourceUpdateBatch *updates);
  void renderBedTexture(QRhiCommandBuffer *cb);
  bool ensureModelLitPipeline();
  // P15.2 (COLOR): blended clone of the lit pipeline for the translucent
  // model pass (blend on, depth write off).
  bool ensureModelLitBlendPipeline();
  void renderModelTranslucentPass(QRhiCommandBuffer *cb);
  // v5.16 (BEDMODEL/BEDTYPE-TEX)
  void uploadBedModelMesh(QRhiResourceUpdateBatch *updates);
  void renderBedModel(QRhiCommandBuffer *cb);
  // P15.7 (AXES): origin arrows through the lit pipeline.
  void renderAxes(QRhiCommandBuffer *cb);
  // P15.5 (SINK): sinking-contour bands; hovered/displaced ranges draw
  // depth-unconditional (upstream glDepthFunc(GL_ALWAYS), 3DScene.cpp:1018).
  void renderSinkingContours(QRhiCommandBuffer *cb);
  void releaseBedTypeParts();
  void prepareBedTypeParts(QRhiResourceUpdateBatch *updates);
  void renderBedTypeParts(QRhiCommandBuffer *cb);
  void renderMoveGizmo(QRhiCommandBuffer *cb);                // Phase 68
  void renderRotateGizmo(QRhiCommandBuffer *cb);              // Phase 70
  void renderScaleGizmo(QRhiCommandBuffer *cb);               // Phase 70
  void renderCutPlane(QRhiCommandBuffer *cb);                  // Phase 71
  void renderWipeTower(QRhiCommandBuffer *cb);                 // Phase 71
  bool ensureBuffer(std::unique_ptr<QRhiBuffer> &buffer,
                    quint32 byteSize,
                    quint32 &storedSize,
                    QRhiBuffer::UsageFlags usage);
  // v5.16 (NAVIGATOR): bottom-left 3D navigator cube (upstream
  // GLCanvas3D::_render_3d_navigator + ImGuizmo::ViewManipulate). Drawn in
  // overlay pixel space with a dedicated ortho UBO (no depth, blended).
  bool uploadNavigatorBuffer(QRhiResourceUpdateBatch *updates);
  void renderNavigator(QRhiCommandBuffer *cb);

  // ── Phase 26: Preview segment pipeline (D-26-01..04) ──
  void parsePreviewSegments();
  bool uploadPreviewSegmentBuffer(QRhiResourceUpdateBatch *updates);
  void rebuildPreviewCap();  // P17.7: sequential-range cap quad
  QVector<PreviewDrawRange> computePreviewDrawRanges() const;
  quint64 computePreviewRangeCacheKey() const;

  // ── Phase 238 (PREV-01): ghost shells ──
  // Semi-transparent object meshes drawn behind the toolpaths in the preview
  // pass (upstream ALWAYS loads + renders shells in preview:
  // GCodeViewer.cpp:3076 load_shells / :4023 render_shells, drawn before
  // render_toolpaths). Reuses the prepare-pass mesh staging
  // (m_prepareScene.modelVertices) with a ghost alpha override, uploaded to a
  // dedicated buffer and drawn with m_translucentFillPipeline (blended, depth
  // test but no depth write -- mirrors upstream glDepthMask(GL_FALSE)).
  bool uploadGhostShellBuffer(QRhiResourceUpdateBatch *updates);
  // ── Phase 238 (PREV-02): tool marker ──
  // The 3D tool-position arrow (upstream GCodeViewer::SequentialView::Marker,
  // Marker::render GCodeViewer.cpp:306-330) rendered at markerX/Y/Z while
  // showMarker is true. The properties already existed on RhiViewport but the
  // renderer never consumed them; this closes the gap.
  bool uploadToolMarkerBuffer(QRhiResourceUpdateBatch *updates);

  QVector<Vertex> buildSceneVertices(const QList<PrepareSceneData::Vertex> &source) const;
  QVector<Vertex> buildModelVertices(const QList<PrepareSceneData::ModelVertex> &source) const;
  QVector<Vertex> buildHighlightVertices() const;
  QShader loadShader(const QString &path) const;
  // v5.16 (BEDBOTTOM): upstream bed/plate bottom gate. GLCanvas3D passes
  // bottom = !camera.is_looking_downward() into _render_bed/_render_platelist
  // (GLCanvas3D.cpp:1912/1922); when the camera looks from below (or exactly
  // horizontal) the plate background, exclude areas, logo texture and bed
  // model are skipped so objects stay visible (PartPlate::render `if (!bottom)`,
  // Bed3D::render_system `if (!bottom)`). Only the grid remains, in
  // LINE_BOTTOM_COLOR (render_grid(true)).
  bool cameraLookingDown() const;

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
  // v5.16 (BEDBOTTOM): grid-only lines for below-horizon camera views
  // (upstream PartPlate::render_grid(true) LINE_BOTTOM_COLOR).
  std::unique_ptr<QRhiBuffer> m_bedBottomLineBuffer;
  // P15.8 (BOLDGRID): every-5th grid line as ~2px flat quads (upstream
  // render_grid second bolder draw, PartPlate.cpp:909-911).
  std::unique_ptr<QRhiBuffer> m_bedBoldFillBuffer;
  // P15.5 (SINK): white bed-plane contour bands of sinking batches
  // (upstream GLVolume::SinkingContours, 3DScene.cpp:108-161).
  std::unique_ptr<QRhiBuffer> m_sinkingContourBuffer;
  // P15.7 (AXES): origin arrows (upstream Bed3D::Axes::render,
  // 3DBed.cpp:183-245) drawn with the lit pipeline + face normals.
  std::unique_ptr<QRhiBuffer> m_bedAxesVertexBuffer;
  std::unique_ptr<QRhiBuffer> m_bedAxesNormalBuffer;
  // P15.5 (SINK): depth-test-off clone of the fill pipeline for the
  // hovered/displaced contour ranges (glDepthFunc(GL_ALWAYS),
  // 3DScene.cpp:1018-1034).
  std::unique_ptr<QRhiGraphicsPipeline> m_sinkingAlwaysPipeline;
  // v5.16 (NAVIGATOR): overlay cube buffer. The cube is projected to world
  // space on the CPU and drawn with the verified scene translucent pipeline
  // (dedicated overlay SRB/UBO attempts produced no visible output on the
  // D3D11 runtime; see P13.6 in the migration tracker).
  std::unique_ptr<QRhiBuffer> m_navigatorFillBuffer;
  // v5.16 (HTLIMIT): ByObject clearance rings (upstream render_height_limit).
  std::unique_ptr<QRhiBuffer> m_bedLimitBuffer;
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
  // v5.15 (BEDTEX): textured print-bed quad. Mirrors upstream PartPlate
  // render_logo_texture — the printer profile's bed_texture image drawn over
  // the plate background/grid, blended, depth test+write off. Separate SRB
  // because binding 1 carries the sampled texture.
  std::unique_ptr<QRhiGraphicsPipeline> m_bedTexturePipeline;
  std::unique_ptr<QRhiShaderResourceBindings> m_bedTextureSrb;
  std::unique_ptr<QRhiTexture> m_bedTexture;
  std::unique_ptr<QRhiSampler> m_bedTextureSampler;
  std::unique_ptr<QRhiBuffer> m_bedTextureVertexBuffer;
  QImage m_bedTextureImage;
  QString m_bedTexturePath;
  QString m_pendingBedTexturePath;
  bool m_bedTextureDirty = false;
  bool m_bedTextureQuadDirty = true;
  quint32 m_bedTextureVertexBytes = 0;
  quint32 m_bedTextureVertexCount = 0;
  // v5.15 (MODELLIT): two-light gouraud pipeline for model meshes (upstream
  // gouraud.vs constants). Shares m_srb (camera UBO only) and reads a
  // parallel per-face-normal buffer alongside m_modelVertexBuffer.
  std::unique_ptr<QRhiGraphicsPipeline> m_modelLitPipeline;
  std::unique_ptr<QRhiBuffer> m_modelNormalBuffer;
  quint32 m_modelNormalBufferBytes = 0;
  bool m_modelLitEnabled = true;
  // P15.2 (COLOR): upstream _render_objects(Transparent) — translucent
  // modifier/negative volumes drawn after the opaque pass and the bed/plate,
  // depth-tested without depth writes, far-to-near by view-space depth
  // (3DScene.cpp:852-887). Same lit shaders; blend enabled, depth write off.
  std::unique_ptr<QRhiGraphicsPipeline> m_modelLitBlendPipeline;
  std::unique_ptr<QRhiBuffer> m_modelTranslucentBuffer;
  std::unique_ptr<QRhiBuffer> m_modelTranslucentNormalBuffer;
  quint32 m_modelTranslucentBufferBytes = 0;
  quint32 m_modelTranslucentNormalBufferBytes = 0;
  quint32 m_modelTranslucentVertexCount = 0;
  struct TranslucentBatchRange
  {
    quint32 firstVertex = 0;
    quint32 vertexCount = 0;
    QVector3D center;
  };
  QVector<TranslucentBatchRange> m_modelTranslucentBatches;
  QString m_extrudersColorsSignature;
  // Parsed filament colours (rgba 0..1, index 0 = extruder 1) mirrored from
  // RhiViewport::m_extrudersColors; consumed by the P15.1 tint and the
  // P17.5 ghost-shell tint.
  QList<QVector4D> m_extrudersColorsParsed;
  // v5.16 (BEDMODEL): printer bed_model STL drawn with the lit pipeline in
  // DEFAULT_MODEL_COLOR_DARK (upstream Bed3D::render_model).
  std::unique_ptr<QRhiBuffer> m_bedModelVertexBuffer;
  std::unique_ptr<QRhiBuffer> m_bedModelNormalBuffer;
  quint32 m_bedModelVertexBytes = 0;
  quint32 m_bedModelNormalBytes = 0;
  quint32 m_bedModelVertexCount = 0;
  QByteArray m_bedModelMeshBytes;
  bool m_bedModelUploaded = false;
  // v5.16 (BEDTYPE-TEX): BBL bed-type texture parts (upstream PartPlateList
  // bed_texture_info). One texture+pipeline per part (2-3 visible).
  struct BedTypePartGpu {
    std::unique_ptr<QRhiTexture> texture;
    std::unique_ptr<QRhiSampler> sampler;
    std::unique_ptr<QRhiShaderResourceBindings> srb;
    std::unique_ptr<QRhiGraphicsPipeline> pipeline;
    std::unique_ptr<QRhiBuffer> vertexBuffer;
    quint32 vertexBytes = 0;
    quint32 vertexCount = 0;
  };
  QHash<QString, BedTypePartGpu *> m_bedTypePartGpu;
  bool m_bedTypeActive = false;
  bool m_bedCaliActive = false;
  QString m_bedTypeImagesDir;
  int m_bedTypeIndex = -1;
  float m_bedTypePlateOffsetX = 0.0f;
  float m_bedTypePlateOffsetZ = 0.0f;
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
  // Phase 121 (PAINT-02/OV-03): painted-facet overlay buffer. Reuses the mesh
  // pipeline (m_fillPipeline + GizmoVertex). Uploaded from m_paintOverlayData.
  std::unique_ptr<QRhiBuffer> m_paintOverlayBuffer;
  // Phase HOLLOW: drain-hole marker disc buffer (translucent fill pipeline).
  std::unique_ptr<QRhiBuffer> m_hollowMarkerBuffer;
  std::unique_ptr<QRhiBuffer> m_advancedCutMarkerBuffer;  // v5.13
  // Phase 240 (GIZ-05): measure overlay line buffer (line pipeline).
  std::unique_ptr<QRhiBuffer> m_measureOverlayBuffer;
  // Phase 240 (GIZ-03): flatten hovered-facet highlight buffer (translucent
  // fill pipeline).
  std::unique_ptr<QRhiBuffer> m_flattenHoverBuffer;
  // Phase 121 (PAINT-03/OV-05): brush sphere cursor buffer (translucent).
  std::unique_ptr<QRhiBuffer> m_brushCursorBuffer;
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
  // Phase 121 (PAINT-02/OV-03): overlay + brush-cursor upload flags.
  bool m_paintOverlayBufferUploaded = false;
  bool m_brushCursorBufferUploaded = false;
  // Phase HOLLOW: drain-hole marker disc upload flag.
  bool m_hollowMarkerBufferUploaded = false;
  bool m_advancedCutMarkerBufferUploaded = false;  // v5.13
  // Phase 240 (GIZ-05/GIZ-03): overlay upload flags.
  bool m_measureOverlayBufferUploaded = false;
  bool m_flattenHoverBufferUploaded = false;
  bool m_assemblyConnectorBufferUploaded = false;
  bool m_assemblyMeasureLineBufferUploaded = false;
  bool m_assemblyMeasureTriBufferUploaded = false;
  bool m_assemblyMeasureValueBufferUploaded = false;
  bool m_gizmoPipelineCreated = false;
  quint32 m_gizmoVertexBufferBytes = 0;
  quint32 m_cutPlaneFillBufferBytes = 0;
  quint32 m_cutPlaneOutlineBufferBytes = 0;
  quint32 m_wipeTowerBufferBytes = 0;
  // Phase 121 (PAINT-02/OV-03): overlay + brush-cursor buffer byte sizes.
  quint32 m_paintOverlayBufferBytes = 0;
  quint32 m_hollowMarkerBufferBytes = 0;
  quint32 m_advancedCutMarkerBufferBytes = 0;  // v5.13
  quint32 m_measureOverlayBufferBytes = 0;   // Phase 240 (GIZ-05)
  quint32 m_flattenHoverBufferBytes = 0;     // Phase 240 (GIZ-03)
  quint32 m_brushCursorBufferBytes = 0;
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
  // Phase 210 (MIT-03 / Seam C): frame counter for the first-N-frames force
  // window. After a swapchain rebuild (initialize()) the counter resets to 0
  // and the first 3 frames unconditionally upload the camera UBO (and all
  // scene buffers, via the *Uploaded flag resets) so the first SRB bind never
  // sees uninitialized GPU memory under D3D12. Incremented at the start of
  // render(); the value seen during upload logic is 1-based on frame 1, so
  // the force window is m_frameCount <= 3 (frames 1..3).
  int m_frameCount = 0;
  quint32 m_bedFillBufferBytes = 0;
  quint32 m_bedLineBufferBytes = 0;
  quint32 m_bedBottomLineBufferBytes = 0;
  // P15.8 (BOLDGRID)/P15.5 (SINK)/P15.7 (AXES): derived bed geometry buffers.
  quint32 m_bedBoldFillBufferBytes = 0;
  quint32 m_sinkingContourBufferBytes = 0;
  quint32 m_bedAxesVertexBufferBytes = 0;
  quint32 m_bedAxesNormalBufferBytes = 0;
  quint32 m_navigatorFillBufferBytes = 0;
  quint32 m_bedLimitBufferBytes = 0;
  quint32 m_modelVertexBufferBytes = 0;
  quint32 m_highlightVertexBufferBytes = 0;
  quint32 m_cameraUniformBufferBytes = 0;
  quint32 m_bedFillVertexCount = 0;
  quint32 m_bedLineVertexCount = 0;
  quint32 m_bedBottomLineVertexCount = 0;
  quint32 m_bedBoldFillVertexCount = 0;
  quint32 m_sinkingContourVertexCount = 0;
  quint32 m_bedAxesVertexCount = 0;
  // P15.5 (SINK): upload gates for the derived bed buffers.
  bool m_bedBoldFillBufferUploaded = false;
  bool m_sinkingContourBufferUploaded = false;
  bool m_bedAxesBuffersUploaded = false;
  // P15.4 (SELBOX): gizmo-drag state mirrored from RhiViewport in
  // synchronize(); upstream suppresses the selection outline while a gizmo
  // runs (if (!m_gizmos.is_running()), GLCanvas3D.cpp:7367).
  bool m_gizmoDragging = false;
  quint32 m_navigatorFillVertexCount = 0;
  // v5.16 (NAVIGATOR): cube state mirrored from RhiViewport in synchronize().
  bool m_navigatorEnabled = true;
  bool m_navigatorBufferUploaded = false;
  NavigatorCube::RectF m_navigatorRect;
  int m_navigatorHoverBox = -1;
  QVector3D m_navigatorHoverFaceNormal;
  QMatrix4x4 m_navigatorLastView;   // geometry rebuild cache key
  QMatrix4x4 m_navigatorLastMvp;
  int m_navigatorLastHoverBox = -2;
  QSize m_navigatorLastPixelSize;
  quint32 m_bedLimitVertexCount = 0;
  quint32 m_modelVertexCount = 0;
  quint32 m_highlightVertexCount = 0;
  quint32 m_cutPlaneFillVertexCount = 0;
  quint32 m_cutPlaneOutlineVertexCount = 0;
  quint32 m_wipeTowerVertexCount = 0;
  // Phase 121 (PAINT-02/OV-03): overlay + brush-cursor vertex counts.
  quint32 m_paintOverlayVertexCount = 0;
  quint32 m_hollowMarkerVertexCount = 0;
  quint32 m_advancedCutMarkerVertexCount = 0;  // v5.13
  quint32 m_measureOverlayVertexCount = 0;   // Phase 240 (GIZ-05)
  quint32 m_flattenHoverVertexCount = 0;     // Phase 240 (GIZ-03)
  quint32 m_brushCursorVertexCount = 0;
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
  // Phase 138 (ASM-01): per-source-object assemble offset (GL X,Y,Z), mirrored
  // from RhiViewport::m_assembleOffsets in synchronize(). Applied as a per-object
  // translation in buildModelVertices on the CanvasAssembleView path.
  QVariantList m_assembleOffsets;
  // Phase 141 (DEBT-04): per-source-object assemble rotation (Euler XYZ, radians)
  // and scale (XYZ), mirrored from RhiViewport. Composed with the offset into the
  // per-source transform matrix below (v4.8 tech-debt closure: translate-only → full).
  QVariantList m_assembleRotations;
  QVariantList m_assembleScales;
  // Phase 138 (ASM-01): offset map keyed by sourceObjectIndex, built in
  // synchronize() by zipping m_assembleOffsets with the viewport's parallel
  // meshBatchSourceObjectIndices list. Consumed by buildModelVertices.
  QHash<int, QVector3D> m_assembleOffsetBySource;
  // Phase 141 (DEBT-04): full per-source assemble transform (translate * rotate *
  // scale), built in synchronize() from the three parallel lists. Empty entry =
  // identity (no compose). Applied to each vertex in buildModelVertices on the
  // CanvasAssembleView branch.
  QHash<int, QMatrix4x4> m_assembleTransformBySource;
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
  QMatrix4x4 m_cameraView;      // v5.15 (MODELLIT): world->eye for gouraud lighting
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
  // Phase 240 (GIZ-04): interactive cut-plane tilt (Euler XYZ degrees,
  // upstream GLGizmoCut3D m_rotation). Applied to the plane quad around its
  // on-axis center in uploadCutPlaneBuffers.
  float m_cutRotationX = 0.f;
  float m_cutRotationY = 0.f;
  float m_cutRotationZ = 0.f;
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
  // Phase 109 (WTMESH-03/WM-04/WM-05): Option B real-mesh state mirrored from
  // RhiViewport in synchronize(). When m_wipeTowerHasRealMesh is true,
  // uploadWipeTowerBuffer builds the vertex buffer from m_wipeTowerMeshVertices
  // via buildWipeTowerMeshVertices (Option B); otherwise it falls back to
  // buildWipeTowerVertices (Option A, Phase 99 Frozen Decision 2 baseline).
  // The mesh vertices are flattened XYZ triples (libslic3r world frame); the
  // builder applies the upstream Y -> Qt Z transform. No libslic3r types cross
  // into the renderer.
  bool m_wipeTowerHasRealMesh = false;
  std::vector<float> m_wipeTowerMeshVertices;

  // ── Phase 121 (PAINT-02/PAINT-03): paint overlay + brush cursor state ──
  // Mirrored from RhiViewport in synchronize(). m_paintOverlayData is the byte
  // stream from EditorViewModel::paintOverlayData (world-transformed facets).
  // m_extrudersColors carries the MMU hex strings for ExtruderN coloring.
  // Brush fields drive the sphere cursor (position/color/radius).
  QByteArray m_paintOverlayData;
  // Phase HOLLOW: drain-hole marker byte stream from
  // EditorViewModel::hollowMarkerData (world-space disc-fan GizmoVertex).
  QByteArray m_hollowMarkerData;
  // v5.13: connector-pin marker byte stream from
  // EditorViewModel::advancedCutMarkerData.
  QByteArray m_advancedCutMarkerData;
  // Phase 240 (GIZ-05): measure overlay line stream from
  // EditorViewModel::measureOverlayData.
  QByteArray m_measureOverlayData;
  // Phase 240 (GIZ-03): flatten hovered-facet stream from
  // EditorViewModel::flattenHoverData.
  QByteArray m_flattenHoverData;
  QVariantList m_extrudersColors;
  float m_brushRadius = 2.0f;
  int m_brushCursorType = 1;     // 1=Sphere
  int m_paintState = 1;          // EnforcerBlockerType int
  float m_brushMouseScreenX = 0.f;
  float m_brushMouseScreenY = 0.f;
  int m_brushButtonState = 0;    // 0=hover, 1=left, 2=right, -1=hide
  // Cached cursor center (world space) + last-used button state for the brush
  // sphere. Re-computed in renderBrushCursor from the screen position via a
  // ray-mesh pick; stored here so uploadBrushCursorBuffer can rebuild on change.
  QVector3D m_brushCursorWorldCenter;
  int m_brushCursorLastButtonState = -2; // sentinel != any valid state
  float m_brushCursorLastScreenX = std::numeric_limits<float>::lowest();
  float m_brushCursorLastScreenY = std::numeric_limits<float>::lowest();
  float m_brushCursorLastRadius = -1.f;

  // ── Phase 26: Preview segment pipeline state ──
  QByteArray m_previewData;              // GCV1 blob from RhiViewport
  int m_layerMin = 0;
  int m_layerMax = 0;
  int m_moveEnd = 0;
  bool m_showTravelMoves = true;
  int m_gcodeViewMode = 0;
  QVector<bool> m_roleVisibility;  ///< Per-role extrusion mask from RhiViewport (render-side skip).
  QVector<Vertex> m_previewVertices;     // expanded preview vertices (CPU staging)
  struct PreviewDrawSpan {
    int layer;
    int move;
    quint32 vertexOffset;
    quint32 vertexCount;
    int role;  ///< Canonical libvgcode EGCodeExtrusionRole index for render-side filtering.
    // P17.2: true for extrusion segments expanded into the solid prism
    // (triangle topology, translucent fill pipeline), false for the 2-vertex
    // line segments (travel/retract/marker, line pipeline).
    bool triangles = false;
  };
  QVector<PreviewDrawSpan> m_previewDrawSpans;
  // P17.7: sequential-range cap (upstream SequentialRangeCap, GCodeViewer.hpp:
  // 509-520 / :3618-3690) — a quad capping the toolpath end at the current
  // move position while dragging. Rebuilt when moveEnd changes.
  QVector<Vertex> m_previewCapVertices;
  int m_previewCapMoveEnd = -1;
  std::unique_ptr<QRhiBuffer> m_previewCapBuffer;
  quint32 m_previewCapBufferBytes = 0;
  quint32 m_previewCapVertexCount = 0;
  bool m_previewCapUploadPending = false;
  std::unique_ptr<QRhiBuffer> m_previewSegmentBuffer;
  quint32 m_previewSegmentBufferBytes = 0;
  quint32 m_previewSegmentVertexCount = 0;
  bool m_previewSegmentBufferUploaded = false;

  // ── Phase 238 (PREV-01/02): preview ghost-shell + tool-marker GPU state ──
  std::unique_ptr<QRhiBuffer> m_ghostShellBuffer;
  std::unique_ptr<QRhiBuffer> m_toolMarkerBuffer;
  quint32 m_ghostShellBufferBytes = 0;
  quint32 m_toolMarkerBufferBytes = 0;
  quint32 m_ghostShellVertexCount = 0;
  quint32 m_toolMarkerVertexCount = 0;
  bool m_ghostShellBufferUploaded = false;
  bool m_toolMarkerBufferUploaded = false;
  /// Model generation at the last ghost-shell upload (rebuild trigger).
  qint64 m_ghostShellModelGeneration = 0;
  // Marker state mirrored from RhiViewport in synchronize() (PREV-02). The
  // last* fields detect position changes so the small marker buffer is only
  // rebuilt when the tool actually moves (playback) or toggles.
  float m_markerX = 0.f;
  float m_markerY = 0.f;
  float m_markerZ = 0.f;
  bool m_showMarker = true;
  float m_lastMarkerX = std::numeric_limits<float>::lowest();
  float m_lastMarkerY = std::numeric_limits<float>::lowest();
  float m_lastMarkerZ = std::numeric_limits<float>::lowest();

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
  // Phase 208 (MIT-01 / Seam A): thumbnail readback batch deferred from the
  // frame in which the offscreen capture pass ran to the NEXT frame's on-
  // screen beginPass. The previous code issued cb->resourceUpdate() directly
  // on the bare command buffer after the thumbnail pass ended - the only
  // pass-external resourceUpdate in this file, and a plausible D3D12 0xC0000005
  // contributor. Holding the batch for one frame and merging it into the next
  // beginPass's 4th-argument batch keeps all resource updates folded into a
  // pass, matching QRhi's documented usage. Render-thread owned; QRhi does not
  // take ownership (the batch is returned to the pool via delete after merge).
  QRhiResourceUpdateBatch *m_pendingReadbackUpdates = nullptr;
  // Item pointer for the queued callback (QPointer survives item recreation
  // and nulls itself if the item is destroyed before the readback completes).
  QPointer<RhiViewport> m_viewportItem;
  // Camera snapshot copied during synchronize(); render() must not call into
  // the GUI-owned RhiViewport or its CameraController.
  QMatrix4x4 m_thumbnailCameraMvp;
  bool m_thumbnailCameraMvpValid = false;
};
