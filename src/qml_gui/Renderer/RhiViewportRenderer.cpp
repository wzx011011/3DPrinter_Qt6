#include "RhiViewportRenderer.h"
#include "RhiViewport.h"
#include "core/rendering/AssemblyMeasureGeometry.h"
#include "core/rendering/GizmoCenter.h"
#include "core/rendering/GizmoGeometry.h"

#include <QDebug>
#include <QFile>
#include <QHash>
#include <QPainter>
#include <QSvgRenderer>

#include <algorithm>
#include <atomic>
#include <cstddef>

// Phase 207 (REPRO-01): env-gated structured render diagnostics. Enable with
// OWZX_RHI_TRACE=1 to emit a tagged log line per render() milestone so the
// D3D12 0xC0000005 crash frame can be located without a native debugger. The
// gate is read once and cached; release builds with the env unset emit zero
// overhead.
namespace {
bool rhiTraceEnabled()
{
  static const bool enabled = qEnvironmentVariableIsSet("OWZX_RHI_TRACE");
  return enabled;
}
void rhiTrace(const char *milestone)
{
  if (rhiTraceEnabled())
    qInfo("[RHI-TRACE] %s", milestone);
}
} // namespace

// Phase 209 (MIT-02 / Seam B): POD mirror of the GLSL std140 CameraBlock
// declared by the gizmo shader (mesh shader's CameraBlock declares only the
// mat4 mvp and ignores the trailing bytes). Packing all three sub-ranges into
// one struct lets uploadCameraUniform issue a single updateDynamicBuffer
// instead of three separate sub-range writes, which is more efficient and
// avoids the D3D12 multi-record coalescing path (a plausible 0xC0000005
// source when binding the SRB against a buffer with three pending uploads).
//
// std140 layout (matches the existing inline comment in uploadCameraUniform):
//   offset 0:  mat4 mvp        (64 bytes; QMatrix4x4 is 16 contiguous floats)
//   offset 64: vec3 gizmoCenter (12 bytes; QVector3D is 3 contiguous floats)
//   offset 76: float gizmoScale (4 bytes; packs into the vec3's std140 tail)
//   offset 80: mat4 view        (64 bytes; v5.15 MODELLIT eye-space lighting)
// Total = 144 bytes. The first 80 bytes are bit-identical to the mesh/gizmo
// std140 CameraBlock; the v5.15 lit/bed shaders extend the block with the
// raw view matrix at offset 80 (declared as vec4 gizmoCenter + mat4 view,
// which reads the same bytes at [64,80)). The 256-byte backing buffer is
// allocated elsewhere (D3D12 cbuffer alignment); only the first 144 bytes
// are written.
struct CameraBlockPacked {
  QMatrix4x4 mvp;        // offset 0,  64 bytes
  QVector3D gizmoCenter; // offset 64, 12 bytes
  float gizmoScale;      // offset 76, 4 bytes
  QMatrix4x4 view;       // offset 80, 64 bytes (v5.15 MODELLIT)
};
// QMatrix4x4 (float[16]) and QVector3D (float[3]) may carry ABI alignment
// padding on some compilers, so the C++ struct can be larger than the std140
// layout. Only the first 144 bytes are uploaded; the assert checks the lower
// bound.
static_assert(sizeof(CameraBlockPacked) >= 144,
              "CameraBlockPacked must be at least 144 bytes to hold the GLSL "
              "std140 CameraBlock layout (mat4 + vec4 + mat4).");

RhiViewportRenderer::RhiViewportRenderer() = default;

RhiViewportRenderer::~RhiViewportRenderer()
{
  releaseResources();
}

void RhiViewportRenderer::initialize(QRhiCommandBuffer *cb)
{
  Q_UNUSED(cb);
  rhiTrace("initialize-enter");
  static std::atomic<int> s_initCount{0};
  const int n = ++s_initCount;
  if (rhiTraceEnabled())
    qInfo("[RHI-TRACE] initialize-call#%d canvasType=%d backend=%s", n, int(m_canvasType),
          rhi() ? rhi()->backendName() : "(null)");
  QRhiRenderPassDescriptor *currentRenderPass = renderTarget()
      ? renderTarget()->renderPassDescriptor() : nullptr;
  if (m_renderPassDescriptor != nullptr
      && currentRenderPass != m_renderPassDescriptor)
  {
    // QRhi graphics pipelines are render-pass compatible objects. A swapchain
    // rebuild may replace the descriptor even when the QRhi and buffers survive,
    // so every on-screen dependent pipeline must be recreated before drawing.
    releaseRenderPassDependentResources();
  }
  m_renderPassDescriptor = currentRenderPass;

  // BUG FIX: do NOT call releaseResources() here unconditionally.
  // QQuickRhiItemRenderer::initialize() is called on every swapchain rebuild
  // (window resize, visibility change, etc). Clearing all GPU buffers here
  // forces a full re-upload every frame, which makes the bed grid flash or
  // disappear entirely (the original "blank viewport" symptom).
  // The pipelines and buffers themselves are reused; only the per-buffer
  // upload flags are reset so the next render re-uploads their contents.
  m_pipelineFailed = false;
  // Phase 210 (MIT-03 / Seam C): reset ALL buffer-uploaded flags on every
  // swapchain rebuild. D3D12 may invalidate the GPU-side backing memory of
  // buffers tied to the previous swapchain's device-context state; without
  // this reset, the first render() after a rebuild would skip re-uploading
  // camera/model/highlight/gizmo/cutplane/wipetower/paint/assembly buffers
  // and bind stale or invalid memory. Re-uploading once per rebuild is cheap
  // and backend-agnostic (D3D11/Vulkan/Metal tolerate it as a no-op cost).
  // This mirrors what releaseResources() does for the full-destroy path.
  m_sceneBuffersUploaded = false;
  m_modelVertexBufferUploaded = false;
  m_highlightVertexBufferUploaded = false;
  m_cameraUniformBufferUploaded = false;
  m_gizmoVertexBufferUploaded = false;
  m_cutPlaneFillBufferUploaded = false;
  m_cutPlaneOutlineBufferUploaded = false;
  m_wipeTowerBufferUploaded = false;
  m_paintOverlayBufferUploaded = false;
  m_hollowMarkerBufferUploaded = false;
  m_advancedCutMarkerBufferUploaded = false;  // v5.13
  m_brushCursorBufferUploaded = false;
  m_assemblyConnectorBufferUploaded = false;
  m_assemblyMeasureLineBufferUploaded = false;
  m_assemblyMeasureTriBufferUploaded = false;
  m_assemblyMeasureValueBufferUploaded = false;
  m_previewSegmentBufferUploaded = false;
  // Phase 238 (PREV-01/02): ghost shells + tool marker re-upload on rebuild.
  m_ghostShellBufferUploaded = false;
  m_toolMarkerBufferUploaded = false;
  // Reset the first-N-frames force window so the new swapchain gets a
  // guaranteed camera UBO upload on its first render() (see render()).
  m_frameCount = 0;
  rhiTrace("seamC-initialize-reset");
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
  QList<int> batchVolumeIndices;
  batchVolumeIndices.reserve(viewport->m_meshBatchVolumeIndices.size());
  for (const QVariant &value : viewport->m_meshBatchVolumeIndices)
    batchVolumeIndices.append(value.toInt());
  QList<int> batchInstanceIndices;
  batchInstanceIndices.reserve(viewport->m_meshBatchInstanceIndices.size());
  for (const QVariant &value : viewport->m_meshBatchInstanceIndices)
    batchInstanceIndices.append(value.toInt());
  if (viewport->m_canvasType == RhiViewport::CanvasAssembleView
      && batchVolumeIndices.isEmpty() && batchInstanceIndices.isEmpty()) {
    batchVolumeIndices = QList<int>(batchSourceObjectIndices.size(), 0);
    batchInstanceIndices = QList<int>(batchSourceObjectIndices.size(), 0);
  }

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
    m_prepareScene.setBedExcludeAreas(viewport->m_bedExcludeAreas);
    m_prepareScene.setShowBed(viewport->m_showBed);
  }
  // v5.16 (HTLIMIT): diff-checked like the bedtype gates (no scene churn).
  m_prepareScene.setHeightLimit(viewport->m_bedHeightLimitActive
                                    && viewport->m_bedHeightToRod > 0.0f,
                                viewport->m_bedHeightToRod,
                                viewport->m_bedHeightToLid);
  if (m_modelGeneration != viewport->m_modelGeneration) {
    m_modelGeneration = viewport->m_modelGeneration;
    m_prepareScene.setPlateContext(viewport->m_currentPlateIndex,
                                   viewport->m_plateCount,
                                   activeObjectIndices);
    m_prepareScene.setModelMeshData(viewport->m_meshData,
                                    batchSourceObjectIndices,
                                    batchVolumeIndices,
                                    batchInstanceIndices,
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
  // v5.15 (MODELLIT): raw world->eye matrix for the gouraud lighting in
  // model_lit.vert (no clip-space correction — lighting is view-relative).
  m_cameraView = viewport->m_camera.viewMatrix();
  // v5.15 (BEDTEX): pick up a pending bed texture path change.
  if (viewport->m_bedTextureDirty) {
    viewport->m_bedTextureDirty = false;
    const QString path = viewport->m_bedTextureUrl.toLocalFile();
    if (path != m_pendingBedTexturePath) {
      m_pendingBedTexturePath = path;
      m_bedTextureDirty = true;
    }
  }
  // v5.16 (BEDMODEL/BEDTYPE-TEX): bed_model stream + bed-type gates. The
  // current plate's grid offset (upstream compute_shape_position) is baked
  // into both the bed model vertices and the bed-type part quads.
  {
    const int plateCount = viewport->m_plateCount;
    const int cols = PrepareSceneData::computePlateColumns(plateCount > 0 ? plateCount : 1);
    const int row = plateCount > 0 ? viewport->m_currentPlateIndex / cols : 0;
    const int col = plateCount > 0 ? viewport->m_currentPlateIndex % cols : 0;
    const float strideX = viewport->m_bedWidth * 1.2f;
    const float strideD = viewport->m_bedDepth * 1.2f;
    m_bedTypePlateOffsetX = float(col) * strideX;
    m_bedTypePlateOffsetZ = float(row) * strideD;

    static float s_lastBedModelOffX = 1e9f;
    static float s_lastBedModelOffZ = 1e9f;
    if (viewport->m_bedModelMeshData != m_bedModelMeshBytes
        || m_bedTypePlateOffsetX != s_lastBedModelOffX
        || m_bedTypePlateOffsetZ != s_lastBedModelOffZ) {
      m_bedModelMeshBytes = viewport->m_bedModelMeshData;
      m_bedModelUploaded = false;
      s_lastBedModelOffX = m_bedTypePlateOffsetX;
      s_lastBedModelOffZ = m_bedTypePlateOffsetZ;
    }
    const bool typeActive = viewport->m_bedTypeTexturesActive;
    if (typeActive != m_bedTypeActive
        || viewport->m_bedCaliLinesActive != m_bedCaliActive
        || viewport->m_bedTypeImagesDir != m_bedTypeImagesDir) {
      m_bedTypeActive = typeActive;
      m_bedCaliActive = viewport->m_bedCaliLinesActive;
      m_bedTypeImagesDir = viewport->m_bedTypeImagesDir;
      releaseBedTypeParts();
    }
    if (viewport->m_currentPlateBedType != m_bedTypeIndex) {
      m_bedTypeIndex = viewport->m_currentPlateBedType;
      // Different bed type -> different part set; drop the stale quads.
      releaseBedTypeParts();
    }
  }
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

  // ── Phase 238 (PREV-02): tool-marker state mirrored from RhiViewport.
  // markerX/Y/Z + showMarker already existed as Q_PROPERTYs (bound to
  // previewVm.toolX/Y/Z in PreviewPage.qml, updated on every playback tick by
  // PreviewViewModel::updateToolPositionData) but were never consumed. A
  // position/visibility change forces a marker buffer rebuild.
  m_markerX = viewport->m_markerX;
  m_markerY = viewport->m_markerY;
  m_markerZ = viewport->m_markerZ;
  m_showMarker = viewport->m_showMarker;
  if (m_markerX != m_lastMarkerX || m_markerY != m_lastMarkerY
      || m_markerZ != m_lastMarkerZ)
  {
    m_lastMarkerX = m_markerX;
    m_lastMarkerY = m_markerY;
    m_lastMarkerZ = m_markerZ;
    m_toolMarkerBufferUploaded = false;
  }

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

  // Phase HOLLOW: drain-hole marker byte stream (same pipe as paint overlay).
  const QByteArray prevHollowMarker = m_hollowMarkerData;
  m_hollowMarkerData = viewport->m_hollowMarkerData;
  if (m_hollowMarkerData != prevHollowMarker)
    m_hollowMarkerBufferUploaded = false;

  // v5.13: connector-pin marker byte stream (same pipe pattern).
  const QByteArray prevAdvCutMarker = m_advancedCutMarkerData;
  m_advancedCutMarkerData = viewport->m_advancedCutMarkerData;
  if (m_advancedCutMarkerData != prevAdvCutMarker)
    m_advancedCutMarkerBufferUploaded = false;

  // Phase 240 (GIZ-05): measure overlay line stream (same pipe pattern).
  const QByteArray prevMeasureOverlay = m_measureOverlayData;
  m_measureOverlayData = viewport->m_measureOverlayData;
  if (m_measureOverlayData != prevMeasureOverlay)
    m_measureOverlayBufferUploaded = false;

  // Phase 240 (GIZ-03): flatten hover facet stream (same pipe pattern).
  const QByteArray prevFlattenHover = m_flattenHoverData;
  m_flattenHoverData = viewport->m_flattenHoverData;
  if (m_flattenHoverData != prevFlattenHover)
    m_flattenHoverBufferUploaded = false;

  // Phase 240 (GIZ-04): cut-plane tilt + gizmo-mode rotation state. A change
  // to either rotation or the plane grab state marks the cut plane dirty so
  // the rotated quad re-uploads.
  const float prevCutRotX = m_cutRotationX;
  const float prevCutRotY = m_cutRotationY;
  const float prevCutRotZ = m_cutRotationZ;
  m_cutRotationX = viewport->m_cutRotationX;
  m_cutRotationY = viewport->m_cutRotationY;
  m_cutRotationZ = viewport->m_cutRotationZ;
  if (!qFuzzyCompare(prevCutRotX, m_cutRotationX)
      || !qFuzzyCompare(prevCutRotY, m_cutRotationY)
      || !qFuzzyCompare(prevCutRotZ, m_cutRotationZ))
    m_cutPlaneDirty = true;

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
  rhiTrace("render-enter");

  // Phase 210 (MIT-03 / Seam C): bump the frame counter first so the value
  // seen by the upload logic below is 1-based. The first 3 frames after every
  // initialize() fall in the force window (m_frameCount <= 3) and get an
  // unconditional camera UBO upload so the first SRB bind never reads
  // uninitialized GPU memory (a D3D12 0xC0000005 candidate).
  ++m_frameCount;

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
                                   !m_brushCursorBufferUploaded ||
                                   !m_hollowMarkerBufferUploaded ||
                                   !m_advancedCutMarkerBufferUploaded ||
                                   !m_measureOverlayBufferUploaded ||
                                   !m_flattenHoverBufferUploaded)) {
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
        uploadHollowMarkerBuffer(updates);
        uploadAdvancedCutMarkerBuffer(updates);
        // Phase 240 (GIZ-05/GIZ-03): measure overlay + flatten hover streams.
        uploadMeasureOverlayBuffer(updates);
        uploadFlattenHoverBuffer(updates);
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
    // Phase 238 (PREV-01): ghost-shell mesh upload (upstream always loads
    // shells for preview, GCodeViewer.cpp:3076; the buffer rebuilds when the
    // model generation changes).
    uploadGhostShellBuffer(updates);
    // Phase 238 (PREV-02): tool-marker upload (rebuilds when the marker
    // position or visibility changed in synchronize()).
    uploadToolMarkerBuffer(updates);
    // Camera uniform upload (merged into the same pre-beginPass batch).
    // Phase 210 (MIT-03 / Seam C): in the first-N-frames force window OR the
    // DirtyGpu/DirtySelection case, pass DirtyGpu in addition to DirtyCamera
    // so the upload is guaranteed even if m_cameraUniformBufferUploaded was
    // left stale by a swapchain rebuild (the flag is also reset in
    // initialize(), but DirtyGpu is belt-and-suspenders for the very first
    // bind).
    const quint32 camFlags = (m_frameCount <= 3)
        ? (PrepareSceneData::DirtyCamera | PrepareSceneData::DirtyGpu)
        : PrepareSceneData::DirtyCamera;
    uploadCameraUniform(updates, camFlags);
  }
  else if (!m_pipelineFailed && m_frameCount <= 3
           && m_cameraUniformBuffer != nullptr) {
    // Phase 210 (MIT-03 / Seam C): View3D/AssembleView first-N-frames force
    // upload of the camera UBO. When the scene is not dirty and the buffers
    // are already flagged uploaded (the common steady-state path), the
    // scene-dirty guard above does not allocate an updates batch and the
    // camera UBO is not re-issued. After a swapchain rebuild that leaves the
    // GPU-side buffer contents invalid under D3D12, the first SRB bind would
    // then read uninitialized memory. Force a camera upload here for the
    // first 3 frames regardless of dirty state. uploadSceneBuffers (taken on
    // the scene-dirty branch above) already uploads the camera, so this arm
    // only fires on the steady-state path.
    if (updates == nullptr)
      updates = rhi()->nextResourceUpdateBatch();
    uploadCameraUniform(updates,
                        PrepareSceneData::DirtyCamera | PrepareSceneData::DirtyGpu);
    rhiTrace("seamC-force-frame");
  }

  // Phase 208 (MIT-01 / Seam A): fold the deferred thumbnail readback batch
  // into this frame's on-screen beginPass. The previous frame's capture
  // stashed its readback batch here instead of issuing it on the bare command
  // buffer (the only pass-external resourceUpdate in this file). QRhi's merge()
  // copies the queued readBackTexture op into the current batch; the pending
  // batch becomes inert and is returned to the pool via delete. This is one
  // frame later than the original issue, but the readback is already async
  // (m_thumbnailReadbackResult is polled on a later render anyway), so the
  // extra frame of latency is invisible. The readback still targets
  // m_thumbnailTexture, which lives until releaseThumbnailResources(), so the
  // one-frame deferral is safe.
  if (m_pendingReadbackUpdates != nullptr) {
    if (updates == nullptr)
      updates = rhi()->nextResourceUpdateBatch();
    updates->merge(m_pendingReadbackUpdates);
    delete m_pendingReadbackUpdates;
    m_pendingReadbackUpdates = nullptr;
    rhiTrace("seamA-folded");
  }

  cb->beginPass(renderTarget(), m_clearColor, {1.0f, 0}, updates);
  rhiTrace("beginPass-done");
  // Phase 90: CanvasAssembleView shares the View3D mesh draw block (bed +
  // model vertex buffer) so the AssembleView canvas is not empty at runtime.
  // Guard widened to != CanvasPreview; the CanvasPreview draw block below
  // stays strictly == CanvasPreview. Mirrors Plater.cpp:7322.
  if (m_canvasType != RhiViewport::CanvasPreview && ensurePipelines()) {
    cb->setViewport(QRhiViewport(0, 0, float(renderTarget()->pixelSize().width()),
                                 float(renderTarget()->pixelSize().height())));
    cb->setShaderResources(m_srb.get());
    rhiTrace("setShaderResources-done");
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
    // v5.16 (HTLIMIT): ByObject clearance rings over the grid (upstream
    // render_height_limit runs inside PartPlate::render; preview's
    // GCodeViewer::_render_bed does not draw them).
    if (m_prepareScene.showBed() && m_bedLimitBuffer && m_bedLimitVertexCount > 0) {
      cb->setGraphicsPipeline(m_linePipeline.get());
      const QRhiCommandBuffer::VertexInput limitBinding(m_bedLimitBuffer.get(), 0);
      cb->setVertexInput(0, 1, &limitBinding);
      cb->draw(m_bedLimitVertexCount);
    }
    // v5.15 (BEDTEX): printer bed texture image drawn over the background +
    // grid, layered exactly like upstream render_logo (blended, no depth
    // writes). Drawn before the model so the mesh (which does depth-test)
    // still occludes it from below-camera angles. For BBL vendors the
    // bed-type overlay parts replace the single image (upstream
    // PartPlate::render_logo bedtype branch).
    if (m_prepareScene.showBed()) {
      if (m_bedTypeActive)
        renderBedTypeParts(cb);
      else
        renderBedTexture(cb);
    }
    // v5.16 (BEDMODEL): printer frame STL under the working mesh.
    renderBedModel(cb);
    if (m_modelVertexBuffer && m_modelVertexCount > 0) {
      // v5.15 (MODELLIT): lit draw path (two-light gouraud) with the parallel
      // per-face normal buffer. Falls back to the flat vertex-color pipeline
      // when the lit pipeline or normal buffer is unavailable.
      if (m_modelLitEnabled && ensureModelLitPipeline() && m_modelNormalBuffer) {
        cb->setGraphicsPipeline(m_modelLitPipeline.get());
        const QRhiCommandBuffer::VertexInput modelBindings[2] = {
            QRhiCommandBuffer::VertexInput(m_modelVertexBuffer.get(), 0),
            QRhiCommandBuffer::VertexInput(m_modelNormalBuffer.get(), 1),
        };
        cb->setVertexInput(0, 2, modelBindings);
      } else {
        cb->setGraphicsPipeline(m_fillPipeline.get());
        const QRhiCommandBuffer::VertexInput modelBinding(m_modelVertexBuffer.get(), 0);
        cb->setVertexInput(0, 1, &modelBinding);
      }
      cb->draw(m_modelVertexCount);
    }
    // Phase 121 (PAINT-02/OV-03): render the painted-facet overlay after the
    // model mesh, before highlight. Reuses m_fillPipeline (opaque vertex-color
    // fill). Gated to the three paint gizmos (Support=6, Seam=7, MMU=10).
    renderPaintOverlay(cb);
    // Phase HOLLOW: render drain-hole marker discs (translucent fill).
    // Gated to GizmoHollow (8); drawn after paint overlay, before highlight.
    renderHollowMarkers(cb);
    renderAdvancedCutMarkers(cb);
    // Phase 240 (GIZ-05/GIZ-03): measure dimension lines (GizmoMeasure=3)
    // and the flatten hovered-facet highlight (GizmoFlatten=4). Both draw
    // after the mesh with their own gizmo gating inside the render helpers.
    renderMeasureOverlay(cb);
    renderFlattenHover(cb);
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
  // Phase 238 (PREV-01/02): the outer gate no longer requires a non-empty
  // segment buffer -- ghost shells and the tool marker render even while the
  // segment payload is being replaced. Segment drawing keeps its own
  // non-empty guard below.
  if (m_canvasType == RhiViewport::CanvasPreview && ensurePipelines()) {
    m_previewFrameTimer.start();
    // Camera uniform was already uploaded before beginPass (BUG-V31-1 fix).
    cb->setViewport(QRhiViewport(0, 0, float(renderTarget()->pixelSize().width()),
                                 float(renderTarget()->pixelSize().height())));
    cb->setShaderResources(m_srb.get());
    // v5.15 (BEDTEX): upstream Preview also renders the print bed
    // (GCodeViewer -> _render_bed) behind the toolpath segments, including
    // the bed texture image.
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
    if (m_prepareScene.showBed()) {
      if (m_bedTypeActive)
        renderBedTypeParts(cb);
      else
        renderBedTexture(cb);
    }
    renderBedModel(cb);

    // Phase 238 (PREV-01): ghost object shells BEHIND the toolpaths
    // (upstream render_shells, GCodeViewer.cpp:4023, drawn before
    // render_toolpaths in the render() order :1247; BBS always loads shells
    // for preview, :983-988). Translucent fill pipeline = blended, depth test
    // but no depth write -- mirrors the upstream glDepthMask(GL_FALSE).
    if (m_ghostShellBuffer && m_ghostShellVertexCount > 0) {
      cb->setGraphicsPipeline(m_translucentFillPipeline.get());
      const QRhiCommandBuffer::VertexInput ghostBinding(m_ghostShellBuffer.get(), 0);
      cb->setVertexInput(0, 1, &ghostBinding);
      cb->draw(m_ghostShellVertexCount);
    }

    cb->setGraphicsPipeline(m_linePipeline.get());

    if (m_previewSegmentBuffer && m_previewSegmentVertexCount > 0) {
      const QVector<PreviewDrawRange> drawRanges = computePreviewDrawRanges();
      if (!drawRanges.isEmpty()) {
        const QRhiCommandBuffer::VertexInput segBinding(m_previewSegmentBuffer.get(), 0);
        cb->setVertexInput(0, 1, &segBinding);
        for (const PreviewDrawRange &range : drawRanges) {
          if (range.vertexCount > 0)
            cb->draw(range.vertexCount, 1, range.firstVertex);
        }
      }
    }

    // Phase 238 (PREV-02): 3D tool-position marker, drawn after the segments
    // so it stays visible over toolpaths via the no-depth-write translucent
    // pipeline (upstream Marker::render, GCodeViewer.cpp:306-330: blended
    // white 0.5-alpha model at the current sequential-view position). The
    // G-code axis convention (x,y,z) maps to scene (x,z,y), matching the
    // segment axis swap in parsePreviewSegments.
    if (m_showMarker && m_toolMarkerBuffer && m_toolMarkerVertexCount > 0) {
      cb->setGraphicsPipeline(m_translucentFillPipeline.get());
      const QRhiCommandBuffer::VertexInput markerBinding(m_toolMarkerBuffer.get(), 0);
      cb->setVertexInput(0, 1, &markerBinding);
      cb->draw(m_toolMarkerVertexCount);
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
  rhiTrace("endPass-done");

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
    rhiTrace("thumbnail-pass-begin");
    renderThumbnailPass(cb);
    rhiTrace("thumbnail-pass-done");
    // Issue the readback on a fresh batch merged into the next beginPass of
    // the on-screen RT — but since the offscreen pass just ended, use a
    // resource-update batch that the QRhi processes as part of this frame's
    // command stream. The readback result lands in m_thumbnailReadbackResult
    // on a subsequent frame.
    QRhiResourceUpdateBatch *readbackUpdates = rhi()->nextResourceUpdateBatch();
    issueThumbnailReadback(readbackUpdates);
    // Phase 208 (MIT-01 / Seam A): stash the readback batch for the NEXT
    // frame's on-screen beginPass instead of issuing it directly on the bare
    // command buffer here. cb->resourceUpdate() between two passes (the
    // thumbnail pass just ended) is the only pass-external resourceUpdate in
    // this file and a plausible D3D12 0xC0000005 contributor (BUG-V31-1
    // pattern: resource updates must be folded into beginPass's 4th arg).
    // The next render() merges m_pendingReadbackUpdates into its updates batch
    // before beginPass. m_thumbnailReadbackInFlight is still set now so no
    // duplicate capture is attempted during the one-frame deferral window.
    m_pendingReadbackUpdates = readbackUpdates;
    rhiTrace("seamA-deferred");
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
  rhiTrace("render-exit");
}

void RhiViewportRenderer::releaseRenderPassDependentResources()
{
  m_linePipeline.reset();
  m_fillPipeline.reset();
  m_translucentFillPipeline.reset();
  m_translucentLinePipeline.reset();
  m_bedTexturePipeline.reset();
  m_modelLitPipeline.reset();
  m_gizmoLinePipeline.reset();
  m_gizmoTriPipeline.reset();
  m_gizmoPipelineCreated = false;
  // Bed-type parts own per-part pipelines bound to the old descriptor. Rebuild
  // their complete GPU bundles lazily so SRB/texture ownership stays simple.
  releaseBedTypeParts();
  m_pipelineFailed = false;
}

void RhiViewportRenderer::releaseResources()
{
  // Phase 95 (THUMBCAP-01): release offscreen thumbnail RT + pipelines +
  // pending readback state before the on-screen resources are torn down.
  releaseThumbnailResources();
  // Phase 208 (MIT-01 / Seam A): drop a deferred readback batch that never
  // got folded into a beginPass (e.g. the capture frame was the last frame
  // before teardown). delete returns it to QRhi's pool.
  if (m_pendingReadbackUpdates != nullptr) {
    delete m_pendingReadbackUpdates;
    m_pendingReadbackUpdates = nullptr;
  }
  m_thumbnailRequestPending = false;
  m_linePipeline.reset();
  m_fillPipeline.reset();
  m_translucentFillPipeline.reset();
  m_translucentLinePipeline.reset();
  // v5.15 (BEDTEX/MODELLIT): bed texture + lit model resources.
  m_bedTexturePipeline.reset();
  m_bedTextureSrb.reset();
  m_bedTexture.reset();
  m_bedTextureSampler.reset();
  m_bedTextureVertexBuffer.reset();
  m_bedTextureImage = QImage();
  m_bedTexturePath.clear();
  m_pendingBedTexturePath.clear();
  m_bedTextureDirty = false;
  m_bedTextureQuadDirty = true;
  m_bedTextureVertexBytes = 0;
  m_bedTextureVertexCount = 0;
  m_modelLitPipeline.reset();
  m_modelNormalBuffer.reset();
  m_modelNormalBufferBytes = 0;
  // v5.16 (BEDMODEL/BEDTYPE-TEX)
  m_bedModelVertexBuffer.reset();
  m_bedModelNormalBuffer.reset();
  m_bedModelVertexBytes = 0;
  m_bedModelNormalBytes = 0;
  m_bedModelVertexCount = 0;
  m_bedModelMeshBytes.clear();
  m_bedModelUploaded = false;
  releaseBedTypeParts();
  // Phase 68: gizmo pipelines + vertex buffer.
  m_gizmoLinePipeline.reset();
  m_gizmoTriPipeline.reset();
  m_gizmoVertexBuffer.reset();
  m_cutPlaneFillBuffer.reset();
  m_cutPlaneOutlineBuffer.reset();
  m_wipeTowerBuffer.reset();
  m_paintOverlayBuffer.reset();   // Phase 121 (PAINT-02)
  m_hollowMarkerBuffer.reset();   // Phase HOLLOW
  m_advancedCutMarkerBuffer.reset();  // v5.13
  m_brushCursorBuffer.reset();    // Phase 121 (PAINT-03)
  m_assemblyConnectorBuffer.reset();  // Phase 91
  m_srb.reset();
  m_cameraUniformBuffer.reset();
  m_highlightVertexBuffer.reset();
  m_modelVertexBuffer.reset();
  m_bedLineBuffer.reset();
  m_bedLimitBuffer.reset();
  m_bedFillBuffer.reset();
  resetPreviewGpuState(true);
  // Phase 238 (PREV-01/02): drop the preview ghost-shell + tool-marker GPU
  // buffers with the rest of the scene resources.
  m_ghostShellBuffer.reset();
  m_ghostShellBufferBytes = 0;
  m_ghostShellVertexCount = 0;
  m_ghostShellBufferUploaded = false;
  m_ghostShellModelGeneration = 0;
  m_toolMarkerBuffer.reset();
  m_toolMarkerBufferBytes = 0;
  m_toolMarkerVertexCount = 0;
  m_toolMarkerBufferUploaded = false;
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
  m_hollowMarkerBufferUploaded = false;   // Phase HOLLOW
  m_advancedCutMarkerBufferUploaded = false;   // v5.13
  m_brushCursorBufferUploaded = false;    // Phase 121 (PAINT-03)
  m_assemblyConnectorBufferUploaded = false;  // Phase 91
  m_gizmoPipelineCreated = false;            // Phase 68
  m_bedFillBufferBytes = 0;
  m_bedLineBufferBytes = 0;
  m_bedLimitBufferBytes = 0;
  m_modelVertexBufferBytes = 0;
  m_highlightVertexBufferBytes = 0;
  m_cameraUniformBufferBytes = 0;
  m_gizmoVertexBufferBytes = 0;              // Phase 68
  m_cutPlaneFillBufferBytes = 0;
  m_cutPlaneOutlineBufferBytes = 0;
  m_wipeTowerBufferBytes = 0;
  m_paintOverlayBufferBytes = 0;   // Phase 121 (PAINT-02)
  m_hollowMarkerBufferBytes = 0;   // Phase HOLLOW
  m_advancedCutMarkerBufferBytes = 0;   // v5.13
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
  const QPointer<RhiViewport> viewport = m_viewportItem;
  if (viewport == nullptr)
    return;
  QMetaObject::invokeMethod(
      viewport.data(),
      [viewport, image, plateIndex]() {
        // The queued callback runs on the GUI thread. Recheck the guarded item
        // there because it may have been destroyed after readback completion.
        if (viewport != nullptr)
          viewport->deliverThumbnail(image, plateIndex);
      },
      Qt::QueuedConnection);
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
  rhiTrace("ensurePipelines-enter");
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

// ── v5.16 (BEDTYPE-TEX): BBL bed-type texture part tables ───────────────────
// Ported verbatim from upstream PartPlateList::init_bed_type_info /
// init_cali_texture_info (PartPlate.cpp:5505-5575): per-bed-type overlay
// strips positioned in mm relative to the plate origin. BedType indices
// follow libslic3r PrintConfig.hpp:255 (btDefault=0, SuperTack=1, PC=2,
// EP=3, PEI=4, PTE=5, PCT=6).
namespace {
struct BedTypePartDef {
  const char *file;
  float x, y, w, h;
};
const BedTypePartDef kBedTypeParts[][3] = {
    /* btDefault */ {},
    /* btSuperTack */ {{"bbl_bed_st_left.svg", 9, 70, 12.5f, 170},
                        {"bbl_bed_st_bottom.svg", 74, -10, 148, 12}, {}},
    /* btPC */ {{"bbl_bed_pc_left.svg", 10, 130, 10, 110},
                 {"bbl_bed_pc_bottom.svg", 74, -10, 148, 12}, {}},
    /* btEP */ {{"bbl_bed_ep_left.svg", 7.5f, 90, 12.5f, 150},
                 {"bbl_bed_ep_bottom.svg", 74, -10, 148, 12}, {}},
    /* btPEI */ {{"bbl_bed_pei_left.svg", 7.5f, 50, 12.5f, 190},
                  {"bbl_bed_pei_bottom.svg", 74, -10, 148, 12}, {}},
    /* btPTE */ {{"bbl_bed_pte_left.svg", 10, 80, 10, 160},
                  {"bbl_bed_pte_bottom.svg", 74, -10, 148, 12}, {}},
    /* btPCT */ {{"orca_bed_pct_left.svg", 10, 130, 10, 110},
                  {"bbl_bed_pc_bottom.svg", 74, -10, 148, 12}, {}},
};
const BedTypePartDef kCaliPart = {"bbl_cali_lines.svg", 18, 2, 224, 16};
} // namespace

// ── v5.15 (BEDTEX): textured print-bed quad ────────────────────────────────
// Upstream contract: PartPlate::render_logo / render_logo_texture load the
// printer profile's bed_texture image (PNG directly, SVG rasterized, capped
// at 2048px) and draw it blended over the plate background + grid with depth
// test/write off (see PartPlate.cpp:736-878).
bool RhiViewportRenderer::ensureBedTexturePipeline()
{
  if (m_bedTexturePipeline)
    return true;
  if (m_pipelineFailed || rhi() == nullptr || renderTarget() == nullptr)
    return false;
  // The SRB references the texture + sampler, so creation is deferred until
  // a texture has actually been uploaded.
  if (!m_bedTexture || !m_bedTextureSampler || !m_cameraUniformBuffer)
    return false;

  if (!m_bedTextureSrb) {
    m_bedTextureSrb.reset(rhi()->newShaderResourceBindings());
    m_bedTextureSrb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage, m_cameraUniformBuffer.get()),
        QRhiShaderResourceBinding::sampledTexture(
            1, QRhiShaderResourceBinding::FragmentStage,
            m_bedTexture.get(), m_bedTextureSampler.get()),
    });
    if (!m_bedTextureSrb->create())
      return false;
  }

  const QShader vertexShader = loadShader(
      QStringLiteral(":/rhi_viewport/shaders/bed_texture.vert.qsb"));
  const QShader fragmentShader = loadShader(
      QStringLiteral(":/rhi_viewport/shaders/bed_texture.frag.qsb"));
  if (!vertexShader.isValid() || !fragmentShader.isValid())
    return false;

  // 5 floats: pos.xyz + uv.xy
  struct BedTextureVertex { float x, y, z, u, v; };
  QRhiVertexInputLayout inputLayout;
  inputLayout.setBindings({QRhiVertexInputBinding(sizeof(BedTextureVertex))});
  inputLayout.setAttributes({
      QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, 0),
      QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float)),
  });

  m_bedTexturePipeline.reset(rhi()->newGraphicsPipeline());
  m_bedTexturePipeline->setTopology(QRhiGraphicsPipeline::Triangles);
  m_bedTexturePipeline->setShaderStages({
      QRhiShaderStage(QRhiShaderStage::Vertex, vertexShader),
      QRhiShaderStage(QRhiShaderStage::Fragment, fragmentShader),
  });
  m_bedTexturePipeline->setShaderResourceBindings(m_bedTextureSrb.get());
  m_bedTexturePipeline->setVertexInputLayout(inputLayout);
  m_bedTexturePipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
  // Upstream draws the logo texture with depth writes off and (effectively)
  // no depth test, layered over the already-drawn background + grid.
  m_bedTexturePipeline->setDepthTest(false);
  m_bedTexturePipeline->setDepthWrite(false);
  QRhiGraphicsPipeline::TargetBlend blend;
  blend.enable = true;
  blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
  blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
  blend.srcAlpha = QRhiGraphicsPipeline::One;
  blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
  m_bedTexturePipeline->setTargetBlends({blend});
  if (!m_bedTexturePipeline->create()) {
    m_bedTexturePipeline.reset();
    m_bedTextureSrb.reset();
    return false;
  }
  return true;
}

void RhiViewportRenderer::uploadBedTexture(QRhiResourceUpdateBatch *updates)
{
  if (updates == nullptr || rhi() == nullptr)
    return;

  // 1. (Re)load the source image when the pending path changed. Mirrors
  // upstream update_logo_texture_filename: only .png/.svg are considered;
  // anything else (or a missing file) clears the texture.
  if (m_bedTextureDirty) {
    m_bedTextureDirty = false;
    if (m_pendingBedTexturePath != m_bedTexturePath) {
      m_bedTexturePath = m_pendingBedTexturePath;
      QImage image;
      const bool isSvg = m_bedTexturePath.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive);
      const bool isPng = m_bedTexturePath.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive);
      if (isSvg) {
        QSvgRenderer svg(m_bedTexturePath);
        if (svg.isValid()) {
          QSize sz = svg.defaultSize();
          if (sz.isEmpty())
            sz = QSize(1024, 1024);
          // Upstream caps the rasterized logo texture at 2048px
          // (logo_tex_size in PartPlate::render_logo).
          const int kMaxTex = 2048;
          if (sz.width() > kMaxTex || sz.height() > kMaxTex) {
            const float s = float(kMaxTex) / float(std::max(sz.width(), sz.height()));
            sz = QSize(int(sz.width() * s), int(sz.height() * s));
          }
          QImage raster(sz, QImage::Format_RGBA8888);
          raster.fill(Qt::transparent);
          QPainter painter(&raster);
          svg.render(&painter);
          painter.end();
          image = raster;
        }
      } else if (isPng) {
        image = QImage(m_bedTexturePath);
        if (!image.isNull())
          image = image.convertToFormat(QImage::Format_RGBA8888);
      }
      if (image.isNull()) {
        if (!m_bedTexturePath.isEmpty())
          qWarning("[RHI] bed texture load failed: %s", qPrintable(m_bedTexturePath));
        m_bedTextureImage = QImage();
      } else {
        m_bedTextureImage = image;
      }
      // GPU state is rebuilt from scratch on any path change (SRB references
      // the texture object, so the pipeline must be recreated too).
      m_bedTexture.reset();
      m_bedTextureSampler.reset();
      m_bedTextureSrb.reset();
      m_bedTexturePipeline.reset();
      m_bedTextureQuadDirty = true;
    }
  }

  if (m_bedTextureImage.isNull())
    return;

  // 2. Create + upload the GPU texture.
  if (!m_bedTexture) {
    m_bedTexture.reset(rhi()->newTexture(QRhiTexture::RGBA8, m_bedTextureImage.size()));
    if (!m_bedTexture->create()) {
      m_bedTexture.reset();
      return;
    }
    m_bedTextureSampler.reset(rhi()->newSampler(
        QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    if (!m_bedTextureSampler->create()) {
      m_bedTextureSampler.reset();
      m_bedTexture.reset();
      return;
    }
    updates->uploadTexture(m_bedTexture.get(), m_bedTextureImage);
  }

  // 3. Rebuild the bed quad when the bed rect changed. The quad uses the
  // same scene mapping as buildSceneVertices: (x, 0, z=y_mm).
  struct BedTextureVertex { float x, y, z, u, v; };
  const float left = m_prepareScene.bedOriginX();
  const float top = m_prepareScene.bedOriginY();
  const float right = left + m_prepareScene.bedWidth();
  const float bottom = top + m_prepareScene.bedDepth();
  if (m_bedTextureQuadDirty) {
    m_bedTextureQuadDirty = false;
    QVector<BedTextureVertex> quad;
    quad.reserve(6);
    // QRhi texture uploads keep the image's top row at v=0, so v grows with
    // the bed's +depth axis (image top edge maps to the bed's top edge).
    const auto append = [&](float x, float z, float u, float v) {
      quad.append(BedTextureVertex{x, 0.0f, z, u, v});
    };
    append(left,  top,    0.f, 0.f);
    append(right, top,    1.f, 0.f);
    append(right, bottom, 1.f, 1.f);
    append(left,  top,    0.f, 0.f);
    append(right, bottom, 1.f, 1.f);
    append(left,  bottom, 0.f, 1.f);
    const quint32 bytes = quint32(quad.size() * int(sizeof(BedTextureVertex)));
    if (ensureBuffer(m_bedTextureVertexBuffer, bytes, m_bedTextureVertexBytes, QRhiBuffer::VertexBuffer)
        && m_bedTextureVertexBuffer && bytes > 0) {
      updates->uploadStaticBuffer(m_bedTextureVertexBuffer.get(), 0, bytes, quad.constData());
      m_bedTextureVertexCount = quint32(quad.size());
    } else {
      m_bedTextureVertexCount = 0;
    }
  }
}

void RhiViewportRenderer::renderBedTexture(QRhiCommandBuffer *cb)
{
  if (cb == nullptr || !m_prepareScene.showBed())
    return;
  if (!m_bedTexturePipeline || !m_bedTextureVertexBuffer || m_bedTextureVertexCount == 0)
    return;
  if (!ensureBedTexturePipeline())
    return;
  cb->setGraphicsPipeline(m_bedTexturePipeline.get());
  cb->setShaderResources(m_bedTextureSrb.get());
  const QRhiCommandBuffer::VertexInput binding(m_bedTextureVertexBuffer.get(), 0);
  cb->setVertexInput(0, 1, &binding);
  cb->draw(m_bedTextureVertexCount);
  // Restore the shared camera-only SRB for the draws that follow.
  cb->setShaderResources(m_srb.get());
}

// ── v5.15 (MODELLIT): two-light gouraud pipeline for model meshes ──────────
// Upstream contract: resources/shaders/140/gouraud.vs — ambient 0.3, top
// light (dir -0.457,0.457,0.762 / diffuse 0.8 / specular 0.125 / shininess
// 20) + front light (dir 0.699,0.140,0.699 / diffuse 0.3), evaluated in eye
// space. Shares m_srb (camera UBO); reads the per-face normal buffer at
// vertex-input binding 1 alongside the shared position/color buffer.
// ── v5.16 (BEDMODEL): printer frame STL (upstream Bed3D::render_model) ─────
void RhiViewportRenderer::uploadBedModelMesh(QRhiResourceUpdateBatch *updates)
{
  if (updates == nullptr || rhi() == nullptr)
    return;
  if (!m_bedModelUploaded && m_bedModelMeshBytes.isEmpty())
    return;

  // Parse the stream: [uint32 vertexCount][float x,y,z] * count.
  QVector<Vertex> vertices;
  if (m_bedModelMeshBytes.size() >= 4) {
    quint32 count = 0;
    std::memcpy(&count, m_bedModelMeshBytes.constData(), 4);
    const qint64 needed = 4 + qint64(count) * 12;
    if (count > 0 && m_bedModelMeshBytes.size() >= needed) {
      vertices.reserve(int(count));
      const float *src = reinterpret_cast<const float *>(m_bedModelMeshBytes.constData() + 4);
      // Upstream Bed3D::update_model_offset: the model is CENTERED on the
      // bed (assemble_transform at the extended bounding-box center) and
      // sits 0.41 + GROUND_Z below the bed plane. The vendor STLs are
      // authored around their own center (e.g. Ender-3 S1 spans
      // [-121.5, 112.5]), so re-center onto the current plate's middle
      // instead of using the raw coordinates (raw placement drew the plate
      // slab across the viewport corner).
      float rawMinX = std::numeric_limits<float>::max();
      float rawMaxX = std::numeric_limits<float>::lowest();
      float rawMinZ = std::numeric_limits<float>::max();
      float rawMaxZ = std::numeric_limits<float>::lowest();
      for (quint32 i = 0; i < count; ++i) {
        rawMinX = std::min(rawMinX, src[0]);
        rawMaxX = std::max(rawMaxX, src[0]);
        rawMinZ = std::min(rawMinZ, src[2]);
        rawMaxZ = std::max(rawMaxZ, src[2]);
        src += 3;
      }
      const float plateCenterX = m_prepareScene.bedOriginX()
          + m_prepareScene.bedWidth() * 0.5f + m_bedTypePlateOffsetX;
      const float plateCenterZ = m_prepareScene.bedOriginY()
          + m_prepareScene.bedDepth() * 0.5f + m_bedTypePlateOffsetZ;
      const float offsetX = plateCenterX - (rawMinX + rawMaxX) * 0.5f;
      const float offsetZ = plateCenterZ - (rawMinZ + rawMaxZ) * 0.5f;
      const float offsetY = -0.44f;
      src = reinterpret_cast<const float *>(m_bedModelMeshBytes.constData() + 4);
      for (quint32 i = 0; i < count; ++i) {
        vertices.append(Vertex{src[0] + offsetX, src[1] + offsetY, src[2] + offsetZ,
                                0.255f, 0.255f, 0.283f, 1.0f});
        src += 3;
      }
    }
  }

  const quint32 bytes = quint32(vertices.size() * int(sizeof(Vertex)));
  if (!ensureBuffer(m_bedModelVertexBuffer, bytes, m_bedModelVertexBytes, QRhiBuffer::VertexBuffer))
    return;
  if (m_bedModelVertexBuffer && bytes > 0)
    updates->uploadStaticBuffer(m_bedModelVertexBuffer.get(), 0, bytes, vertices.constData());

  // Parallel per-face normals for the lit pipeline (same layout as the model
  // mesh path).
  {
    QVector<float> normals;
    normals.reserve(vertices.size() * 3);
    const int triCount = vertices.size() / 3;
    const Vertex *v = vertices.constData();
    for (int t = 0; t < triCount; ++t) {
      const Vertex &a = v[t * 3 + 0];
      const Vertex &b = v[t * 3 + 1];
      const Vertex &c = v[t * 3 + 2];
      const float ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z;
      const float wx = c.x - a.x, wy = c.y - a.y, wz = c.z - a.z;
      float nx = uy * wz - uz * wy;
      float ny = uz * wx - ux * wz;
      float nz = ux * wy - uy * wx;
      const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
      if (len > 1e-9f) {
        nx /= len; ny /= len; nz /= len;
      } else {
        nx = 0.f; ny = 1.f; nz = 0.f;
      }
      for (int k = 0; k < 3; ++k) {
        normals.append(nx);
        normals.append(ny);
        normals.append(nz);
      }
    }
    const quint32 normalBytes = quint32(normals.size() * int(sizeof(float)));
    if (ensureBuffer(m_bedModelNormalBuffer, normalBytes, m_bedModelNormalBytes, QRhiBuffer::VertexBuffer)
        && m_bedModelNormalBuffer && normalBytes > 0)
      updates->uploadStaticBuffer(m_bedModelNormalBuffer.get(), 0, normalBytes, normals.constData());
  }
  m_bedModelVertexCount = quint32(vertices.size());
  m_bedModelUploaded = true;
}

void RhiViewportRenderer::renderBedModel(QRhiCommandBuffer *cb)
{
  if (cb == nullptr || !m_bedModelVertexBuffer || m_bedModelVertexCount == 0)
    return;
  if (!m_modelLitEnabled || !ensureModelLitPipeline() || !m_bedModelNormalBuffer)
    return;
  // Upstream Bed3D::update_model_offset: the frame sits slightly below the
  // bed plane (-0.41 + GROUND_Z). The stream positions are authored in bed
  // coordinates, so the y offset is baked at upload via a stream rewrite in
  // synchronize (see the bed-model sync block); here it is a plain lit draw.
  cb->setGraphicsPipeline(m_modelLitPipeline.get());
  const QRhiCommandBuffer::VertexInput bindings[2] = {
      QRhiCommandBuffer::VertexInput(m_bedModelVertexBuffer.get(), 0),
      QRhiCommandBuffer::VertexInput(m_bedModelNormalBuffer.get(), 1),
  };
  cb->setVertexInput(0, 2, bindings);
  cb->draw(m_bedModelVertexCount);
  cb->setShaderResources(m_srb.get());
}

// ── v5.16 (BEDTYPE-TEX): BBL bed-type overlay parts ────────────────────────
void RhiViewportRenderer::releaseBedTypeParts()
{
  qDeleteAll(m_bedTypePartGpu);
  m_bedTypePartGpu.clear();
}

void RhiViewportRenderer::prepareBedTypeParts(QRhiResourceUpdateBatch *updates)
{
  if (updates == nullptr || rhi() == nullptr || renderTarget() == nullptr)
    return;
  if (m_bedTypeImagesDir.isEmpty() || m_bedTypeIndex <= 0
      || m_bedTypeIndex >= int(sizeof(kBedTypeParts) / sizeof(kBedTypeParts[0])))
    return;

  const BedTypePartDef *parts[4] = {nullptr, nullptr, nullptr, nullptr};
  int partCount = 0;
  for (const BedTypePartDef &def : kBedTypeParts[m_bedTypeIndex]) {
    if (def.file == nullptr || def.w <= 0.f)
      continue;
    parts[partCount++] = &def;
  }
  if (m_bedCaliActive)
    parts[partCount++] = &kCaliPart;
  if (partCount == 0)
    return;

  // Plate origin in scene coords; upstream part rects are measured from the
  // plate's lower-left corner (y grows up), the bed quad maps depth growing
  // downward from the origin, so a part row y maps to depth (bedDepth - y).
  const float plateLeft = m_prepareScene.bedOriginX() + m_bedTypePlateOffsetX;
  const float plateDepth = m_prepareScene.bedDepth();
  const float plateTopMm = m_prepareScene.bedOriginY() + m_bedTypePlateOffsetZ;

  for (int i = 0; i < partCount; ++i) {
    const BedTypePartDef &def = *parts[i];
    const QString path = m_bedTypeImagesDir + QLatin1Char('/') + QLatin1String(def.file);
    BedTypePartGpu *gpu = m_bedTypePartGpu.value(path, nullptr);
    if (gpu == nullptr) {
      // Lazily rasterize the SVG (upstream GLTexture::load_from_svg_file,
      // 2048 cap) and create the texture + pipeline trio.
      QSvgRenderer svg(path);
      if (!svg.isValid())
        continue;
      QSize sz = svg.defaultSize();
      if (sz.isEmpty())
        sz = QSize(256, 256);
      const int kMaxTex = 2048;
      if (sz.width() > kMaxTex || sz.height() > kMaxTex) {
        const float scale = float(kMaxTex) / float(std::max(sz.width(), sz.height()));
        sz = QSize(int(sz.width() * scale), int(sz.height() * scale));
      }
      QImage image(sz, QImage::Format_RGBA8888);
      image.fill(Qt::transparent);
      QPainter painter(&image);
      svg.render(&painter);
      painter.end();

      gpu = new BedTypePartGpu;
      gpu->texture.reset(rhi()->newTexture(QRhiTexture::RGBA8, image.size()));
      gpu->sampler.reset(rhi()->newSampler(
          QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
          QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
      if (!gpu->texture->create() || !gpu->sampler->create()) {
        delete gpu;
        continue;
      }
      gpu->srb.reset(rhi()->newShaderResourceBindings());
      gpu->srb->setBindings({
          QRhiShaderResourceBinding::uniformBuffer(
              0, QRhiShaderResourceBinding::VertexStage, m_cameraUniformBuffer.get()),
          QRhiShaderResourceBinding::sampledTexture(
              1, QRhiShaderResourceBinding::FragmentStage,
              gpu->texture.get(), gpu->sampler.get()),
      });
      if (!gpu->srb->create()) {
        delete gpu;
        continue;
      }
      QShader vs = loadShader(QStringLiteral(":/rhi_viewport/shaders/bed_texture.vert.qsb"));
      QShader fs = loadShader(QStringLiteral(":/rhi_viewport/shaders/bed_texture.frag.qsb"));
      if (!vs.isValid() || !fs.isValid()) {
        delete gpu;
        continue;
      }
      struct BedTextureVertex { float x, y, z, u, v; };
      QRhiVertexInputLayout inputLayout;
      inputLayout.setBindings({QRhiVertexInputBinding(sizeof(BedTextureVertex))});
      inputLayout.setAttributes({
          QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, 0),
          QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float)),
      });
      gpu->pipeline.reset(rhi()->newGraphicsPipeline());
      gpu->pipeline->setTopology(QRhiGraphicsPipeline::Triangles);
      gpu->pipeline->setShaderStages({
          QRhiShaderStage(QRhiShaderStage::Vertex, vs),
          QRhiShaderStage(QRhiShaderStage::Fragment, fs),
      });
      gpu->pipeline->setShaderResourceBindings(gpu->srb.get());
      gpu->pipeline->setVertexInputLayout(inputLayout);
      gpu->pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
      gpu->pipeline->setDepthTest(false);
      gpu->pipeline->setDepthWrite(false);
      QRhiGraphicsPipeline::TargetBlend blend;
      blend.enable = true;
      blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
      blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
      blend.srcAlpha = QRhiGraphicsPipeline::One;
      blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
      gpu->pipeline->setTargetBlends({blend});
      if (!gpu->pipeline->create()) {
        delete gpu;
        continue;
      }
      updates->uploadTexture(gpu->texture.get(), image);
      m_bedTypePartGpu.insert(path, gpu);
    }

    if (gpu->vertexCount == 0) {
      // Quad for this part (y = 0.02, upstream GROUND_Z + 0.02).
      struct BedTextureVertex { float x, y, z, u, v; };
      const float leftX = plateLeft + def.x;
      const float rightX = leftX + def.w;
      const float topZ = plateTopMm + (plateDepth - def.y - def.h);
      const float bottomZ = plateTopMm + (plateDepth - def.y);
      QVector<BedTextureVertex> quad;
      quad.reserve(6);
      const auto append = [&quad](float x, float z, float u, float v) {
        quad.append(BedTextureVertex{x, 0.02f, z, u, v});
      };
      append(leftX, topZ, 0.f, 0.f);
      append(rightX, topZ, 1.f, 0.f);
      append(rightX, bottomZ, 1.f, 1.f);
      append(leftX, topZ, 0.f, 0.f);
      append(rightX, bottomZ, 1.f, 1.f);
      append(leftX, bottomZ, 0.f, 1.f);
      const quint32 bytes = quint32(quad.size() * int(sizeof(BedTextureVertex)));
      if (ensureBuffer(gpu->vertexBuffer, bytes, gpu->vertexBytes, QRhiBuffer::VertexBuffer)
          && gpu->vertexBuffer && bytes > 0) {
        updates->uploadStaticBuffer(gpu->vertexBuffer.get(), 0, bytes, quad.constData());
        gpu->vertexCount = quint32(quad.size());
      }
    }
  }
}

void RhiViewportRenderer::renderBedTypeParts(QRhiCommandBuffer *cb)
{
  if (cb == nullptr || !m_bedTypeActive)
    return;
  for (auto it = m_bedTypePartGpu.cbegin(); it != m_bedTypePartGpu.cend(); ++it) {
    BedTypePartGpu *gpu = it.value();
    if (gpu == nullptr || gpu->vertexCount == 0 || !gpu->pipeline)
      continue;
    cb->setGraphicsPipeline(gpu->pipeline.get());
    cb->setShaderResources(gpu->srb.get());
    const QRhiCommandBuffer::VertexInput binding(gpu->vertexBuffer.get(), 0);
    cb->setVertexInput(0, 1, &binding);
    cb->draw(gpu->vertexCount);
  }
  cb->setShaderResources(m_srb.get());
}

bool RhiViewportRenderer::ensureModelLitPipeline()
{
  if (m_modelLitPipeline)
    return true;
  if (m_pipelineFailed || rhi() == nullptr || renderTarget() == nullptr || m_srb == nullptr)
    return false;

  const QShader vertexShader = loadShader(
      QStringLiteral(":/rhi_viewport/shaders/model_lit.vert.qsb"));
  const QShader fragmentShader = loadShader(
      QStringLiteral(":/rhi_viewport/shaders/model_lit.frag.qsb"));
  if (!vertexShader.isValid() || !fragmentShader.isValid())
    return false;

  QRhiVertexInputLayout inputLayout;
  inputLayout.setBindings({
      QRhiVertexInputBinding(sizeof(Vertex)),          // pos3 + color4
      QRhiVertexInputBinding(3 * sizeof(float)),       // normal3 (parallel)
  });
  inputLayout.setAttributes({
      QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, offsetof(Vertex, x)),
      QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float4, offsetof(Vertex, r)),
      QRhiVertexInputAttribute(1, 2, QRhiVertexInputAttribute::Float3, 0),
  });

  m_renderPassDescriptor = renderTarget()->renderPassDescriptor();
  m_modelLitPipeline.reset(rhi()->newGraphicsPipeline());
  m_modelLitPipeline->setTopology(QRhiGraphicsPipeline::Triangles);
  m_modelLitPipeline->setShaderStages({
      QRhiShaderStage(QRhiShaderStage::Vertex, vertexShader),
      QRhiShaderStage(QRhiShaderStage::Fragment, fragmentShader),
  });
  m_modelLitPipeline->setShaderResourceBindings(m_srb.get());
  m_modelLitPipeline->setVertexInputLayout(inputLayout);
  m_modelLitPipeline->setRenderPassDescriptor(m_renderPassDescriptor);
  m_modelLitPipeline->setDepthTest(true);
  m_modelLitPipeline->setDepthWrite(true);
  m_modelLitPipeline->setTargetBlends({});
  if (!m_modelLitPipeline->create()) {
    m_modelLitPipeline.reset();
    return false;
  }
  return true;
}

bool RhiViewportRenderer::uploadSceneBuffers(QRhiResourceUpdateBatch *updates, quint32 dirtyFlags)
{
  rhiTrace("uploadSceneBuffers-enter");
  if (updates == nullptr || rhi() == nullptr)
    return false;

  if (!uploadCameraUniform(updates, dirtyFlags))
    return false;
  if (!uploadBedBuffers(updates, dirtyFlags))
    return false;
  // v5.15 (BEDTEX): texture image + bed quad (no-op when no texture path).
  uploadBedTexture(updates);
  // v5.16 (BEDMODEL/BEDTYPE-TEX): printer frame mesh + BBL bed-type parts.
  // The bed-model vertices bake the current plate offset + the upstream
  // -0.41 Z placement at upload time.
  uploadBedModelMesh(updates);
  if (m_bedTypeActive)
    prepareBedTypeParts(updates);
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

  // v5.15 (BEDTEX): the texture quad follows the bed rect, so a bed
  // geometry change must rebuild it too.
  m_bedTextureQuadDirty = true;

  const QVector<Vertex> fillVertices = buildSceneVertices(m_prepareScene.bedFillVertices());
  const QVector<Vertex> lineVertices = buildSceneVertices(m_prepareScene.bedLineVertices());
  // v5.16 (HTLIMIT): ModelVertex and Vertex share the 7-float layout; the
  // limit lines carry their height in y directly.
  const QList<PrepareSceneData::ModelVertex> &limitSource =
      m_prepareScene.bedLimitVertices();
  QVector<Vertex> limitVertices;
  limitVertices.reserve(limitSource.size());
  for (const PrepareSceneData::ModelVertex &v : limitSource)
    limitVertices.append(Vertex{v.x, v.y, v.z, v.r, v.g, v.b, v.a});
  const quint32 fillBytes = quint32(fillVertices.size() * int(sizeof(Vertex)));
  const quint32 lineBytes = quint32(lineVertices.size() * int(sizeof(Vertex)));
  const quint32 limitBytes = quint32(limitVertices.size() * int(sizeof(Vertex)));

  if (!ensureBuffer(m_bedFillBuffer, fillBytes, m_bedFillBufferBytes, QRhiBuffer::VertexBuffer)
      || !ensureBuffer(m_bedLineBuffer, lineBytes, m_bedLineBufferBytes, QRhiBuffer::VertexBuffer)
      || !ensureBuffer(m_bedLimitBuffer, limitBytes, m_bedLimitBufferBytes, QRhiBuffer::VertexBuffer))
    return false;

  m_bedFillVertexCount = quint32(fillVertices.size());
  m_bedLineVertexCount = quint32(lineVertices.size());
  m_bedLimitVertexCount = quint32(limitVertices.size());
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
  if (m_bedLimitBuffer && limitBytes > 0) {
    updates->uploadStaticBuffer(m_bedLimitBuffer.get(),
                                0,
                                limitBytes,
                                limitVertices.constData());
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

  // v5.15 (MODELLIT): parallel per-face normal array (one normal per model
  // vertex, i.e. three identical normals per triangle) for the lit pipeline.
  // Computed here so the normal buffer always matches the position buffer
  // even when the mesh stream rebuilds.
  {
    QVector<float> normals;
    normals.reserve(modelVertices.size() * 3);
    const int triCount = modelVertices.size() / 3;
    const Vertex *v = modelVertices.constData();
    for (int t = 0; t < triCount; ++t) {
      const Vertex &a = v[t * 3 + 0];
      const Vertex &b = v[t * 3 + 1];
      const Vertex &c = v[t * 3 + 2];
      const float ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z;
      const float wx = c.x - a.x, wy = c.y - a.y, wz = c.z - a.z;
      float nx = uy * wz - uz * wy;
      float ny = uz * wx - ux * wz;
      float nz = ux * wy - uy * wx;
      const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
      if (len > 1e-9f) {
        nx /= len; ny /= len; nz /= len;
      } else {
        nx = 0.f; ny = 1.f; nz = 0.f;
      }
      for (int k = 0; k < 3; ++k) {
        normals.append(nx);
        normals.append(ny);
        normals.append(nz);
      }
    }
    const quint32 normalBytes = quint32(normals.size() * int(sizeof(float)));
    if (ensureBuffer(m_modelNormalBuffer, normalBytes, m_modelNormalBufferBytes, QRhiBuffer::VertexBuffer)
        && m_modelNormalBuffer && normalBytes > 0) {
      updates->uploadStaticBuffer(m_modelNormalBuffer.get(), 0, normalBytes, normals.constData());
    }
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

      // Phase 240 (GIZ-04): interactive plane tilt (upstream GLGizmoCut3D
      // m_rotation). Rotate the plane quad around its on-axis center point.
      // The world (libslic3r X,Y,Z) Euler maps to the scene frame as
      // X->X, Y->Z, Z->Y (the renderer applies the same swap per vertex).
      // The quad is pre-expanded by sqrt(2) so the rotated plane still spans
      // the object's bounding box diagonal.
      if (!qFuzzyIsNull(m_cutRotationX) || !qFuzzyIsNull(m_cutRotationY)
          || !qFuzzyIsNull(m_cutRotationZ))
      {
        QVector3D center = (boundsMin + boundsMax) * 0.5f;
        if (m_cutAxis == 0)
          center.setX(m_cutPosition);
        else if (m_cutAxis == 1)
          center.setY(m_cutPosition);
        else
          center.setZ(m_cutPosition);
        QMatrix4x4 rot;
        rot.rotate(m_cutRotationX, 1.f, 0.f, 0.f);   // world X -> scene X
        rot.rotate(m_cutRotationY, 0.f, 0.f, 1.f);   // world Y -> scene Z
        rot.rotate(m_cutRotationZ, 0.f, 1.f, 0.f);   // world Z -> scene Y
        const float expand = 1.41421362f;            // sqrt(2) diagonal cover
        auto tiltVertex = [&, this](Vertex &v) {
          QVector3D p(v.x, v.y, v.z);
          p = center + (p - center) * expand;
          p = center + rot.mapVector(p - center);
          v.x = p.x();
          v.y = p.y();
          v.z = p.z();
        };
        for (Vertex &v : fillVertices)
          tiltVertex(v);
        for (Vertex &v : outlineVertices)
          tiltVertex(v);
      }

      // Phase 240 (GIZ-04): rotation grabbers (upstream GLGizmoCut3D renders
      // rotation grabbers on the plane border, GLGizmoCut.cpp render_grabber).
      // Two small spheres mark the tilt handles (world-X tilt / world-Y tilt);
      // pickCutPlaneAt hit-tests the SAME positions (+/- 14px).
      {
        QVector3D center = (boundsMin + boundsMax) * 0.5f;
        if (m_cutAxis == 0)
          center.setX(m_cutPosition);
        else if (m_cutAxis == 1)
          center.setY(m_cutPosition);
        else
          center.setZ(m_cutPosition);
        QVector3D u, v;
        switch (m_cutAxis) {
        case 0: u = QVector3D(0.f, 1.f, 0.f); v = QVector3D(0.f, 0.f, 1.f); break;
        case 1: u = QVector3D(1.f, 0.f, 0.f); v = QVector3D(0.f, 0.f, 1.f); break;
        default: u = QVector3D(1.f, 0.f, 0.f); v = QVector3D(0.f, 1.f, 0.f); break;
        }
        QMatrix4x4 grabRot;
        grabRot.rotate(m_cutRotationX, 1.f, 0.f, 0.f);
        grabRot.rotate(m_cutRotationY, 0.f, 0.f, 1.f);
        grabRot.rotate(m_cutRotationZ, 0.f, 1.f, 0.f);
        const float grabberColor1[4] = {0.118f, 0.565f, 1.0f, 1.0f};  // X tilt: blue
        const float grabberColor2[4] = {0.2f, 0.8f, 0.4f, 1.0f};      // Y tilt: green
        const float grabRadius = std::max(
            1.2f, (boundsMax - boundsMin).length() * 0.012f);
        const QVector<GizmoVertex> g1 = GizmoGeometry::buildBrushSphereVertices(
            center + grabRot.mapVector(u * 6.f), grabRadius, grabberColor1);
        const QVector<GizmoVertex> g2 = GizmoGeometry::buildBrushSphereVertices(
            center + grabRot.mapVector(v * 6.f), grabRadius, grabberColor2);
        for (const GizmoVertex &gv : g1 + g2) {
          Vertex out;
          out.x = gv.x;
          out.y = gv.y;
          out.z = gv.z;
          out.r = gv.r;
          out.g = gv.g;
          out.b = gv.b;
          out.a = gv.a;
          fillVertices.append(out);
        }
      }
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
  rhiTrace("uploadCameraUniform-enter");
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

  // Phase 209 (MIT-02 / Seam B): pack MVP + gizmoCenter + gizmoScale into a
  // single 80-byte struct and issue ONE updateDynamicBuffer. Previously this
  // was three separate sub-range writes (offsets 0/64/76); each enqueued a
  // distinct upload record on the same buffer, forcing the D3D12 backend to
  // coalesce them and risking an off-by-one during SRB bind (a plausible
  // 0xC0000005 source). The pack matches the GLSL std140 CameraBlock exactly
  // (see CameraBlockPacked above): mesh shader reads only the first 64 bytes
  // (mat4 mvp), gizmo shader reads the full 80. gizmoScale is computed the
  // same way as before (distance-based, clamped to >= 5).
  const float gizmoScale = std::max((m_gizmoCenter - m_cameraEye).length() * 0.15f, 5.f);
  CameraBlockPacked packed;
  packed.mvp = corrected;
  packed.gizmoCenter = m_gizmoCenter;
  packed.gizmoScale = gizmoScale;
  packed.view = m_cameraView;
  updates->updateDynamicBuffer(m_cameraUniformBuffer.get(), 0,
                               144, &packed);
  rhiTrace("seamB-packed");

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
// Phase HOLLOW: drain-hole marker disc upload + render.
//
// EditorViewModel::hollowMarkerData packs: a 4-byte uint32 vertex count
// followed by N GizmoVertex (7 floats: x,y,z,r,g,b,a) in libslic3r world
// space. The renderer swaps Y/Z to scene space (same as paint overlay) and
// uploads into m_hollowMarkerBuffer. Render uses the translucent fill pipeline
// (depth test, no depth write) so the semi-transparent red discs do not
// occlude the mesh beneath. Gated to GizmoHollow (mode 8).
// ===========================================================================
bool RhiViewportRenderer::uploadHollowMarkerBuffer(QRhiResourceUpdateBatch *updates)
{
  if (updates == nullptr || rhi() == nullptr)
    return false;
  if (m_hollowMarkerBufferUploaded)
    return true;

  QVector<Vertex> vertices;
  // Header: uint32 vertex count (little-endian), then raw GizmoVertex stream.
  if (m_hollowMarkerData.size() >= 4) {
    quint32 count = 0;
    std::memcpy(&count, m_hollowMarkerData.constData(), 4);
    const qint64 expected = 4 + qint64(count) * qint64(sizeof(GizmoVertex));
    if (count > 0 && m_hollowMarkerData.size() >= expected) {
      const auto *gv = reinterpret_cast<const GizmoVertex *>(
          m_hollowMarkerData.constData() + 4);
      vertices.reserve(int(count));
      for (quint32 i = 0; i < count; ++i) {
        Vertex v;
        // libslic3r world (X,Y,Z) -> scene (X,Z,Y): Z becomes up, Y goes into bed.
        v.x = gv[i].x;
        v.y = gv[i].z;
        v.z = gv[i].y;
        v.r = gv[i].r;
        v.g = gv[i].g;
        v.b = gv[i].b;
        v.a = gv[i].a;
        vertices.append(v);
      }
    }
  }

  const quint32 byteSize = quint32(vertices.size() * int(sizeof(Vertex)));
  if (!ensureBuffer(m_hollowMarkerBuffer, byteSize, m_hollowMarkerBufferBytes,
                    QRhiBuffer::VertexBuffer))
    return false;

  m_hollowMarkerVertexCount = quint32(vertices.size());
  if (m_hollowMarkerBuffer && byteSize > 0) {
    updates->uploadStaticBuffer(m_hollowMarkerBuffer.get(), 0, byteSize,
                                vertices.constData());
  }
  m_hollowMarkerBufferUploaded = true;
  return true;
}

void RhiViewportRenderer::renderHollowMarkers(QRhiCommandBuffer *cb)
{
  if (cb == nullptr)
    return;
  // Gate to the hollow gizmo only (对齐上游 GLGizmoHollow active render).
  if (m_gizmoMode != 8)
    return;
  if (m_translucentFillPipeline == nullptr || m_hollowMarkerBuffer == nullptr ||
      m_hollowMarkerVertexCount == 0)
    return;

  cb->setShaderResources(m_srb.get());
  // Translucent fill: depth-tested, no depth write (discs don't occlude mesh).
  cb->setGraphicsPipeline(m_translucentFillPipeline.get());
  const QRhiCommandBuffer::VertexInput binding(m_hollowMarkerBuffer.get(), 0);
  cb->setVertexInput(0, 1, &binding);
  cb->draw(m_hollowMarkerVertexCount);
}

// ===========================================================================
// v5.13: AdvancedCut connector-pin marker upload + render.
// Same packed format as the hollow markers ([uint32 count][GizmoVertex...] in
// libslic3r world space; Y/Z swapped to scene here). Gated to GizmoAdvancedCut
// (mode 14); translucent fill so pins don't occlude the mesh.
// ===========================================================================
bool RhiViewportRenderer::uploadAdvancedCutMarkerBuffer(QRhiResourceUpdateBatch *updates)
{
  if (updates == nullptr || rhi() == nullptr)
    return false;
  if (m_advancedCutMarkerBufferUploaded)
    return true;

  QVector<Vertex> vertices;
  if (m_advancedCutMarkerData.size() >= 4) {
    quint32 count = 0;
    std::memcpy(&count, m_advancedCutMarkerData.constData(), 4);
    const qint64 expected = 4 + qint64(count) * qint64(sizeof(GizmoVertex));
    if (count > 0 && m_advancedCutMarkerData.size() >= expected) {
      const auto *gv = reinterpret_cast<const GizmoVertex *>(
          m_advancedCutMarkerData.constData() + 4);
      vertices.reserve(int(count));
      for (quint32 i = 0; i < count; ++i) {
        Vertex v;
        // libslic3r world (X,Y,Z) -> scene (X,Z,Y).
        v.x = gv[i].x;
        v.y = gv[i].z;
        v.z = gv[i].y;
        v.r = gv[i].r;
        v.g = gv[i].g;
        v.b = gv[i].b;
        v.a = gv[i].a;
        vertices.append(v);
      }
    }
  }

  const quint32 byteSize = quint32(vertices.size() * int(sizeof(Vertex)));
  if (!ensureBuffer(m_advancedCutMarkerBuffer, byteSize, m_advancedCutMarkerBufferBytes,
                    QRhiBuffer::VertexBuffer))
    return false;

  m_advancedCutMarkerVertexCount = quint32(vertices.size());
  if (m_advancedCutMarkerBuffer && byteSize > 0) {
    updates->uploadStaticBuffer(m_advancedCutMarkerBuffer.get(), 0, byteSize,
                                vertices.constData());
  }
  m_advancedCutMarkerBufferUploaded = true;
  return true;
}

void RhiViewportRenderer::renderAdvancedCutMarkers(QRhiCommandBuffer *cb)
{
  if (cb == nullptr)
    return;
  // Gate to the AdvancedCut gizmo only (mode 14).
  if (m_gizmoMode != 14)
    return;
  if (m_translucentFillPipeline == nullptr || m_advancedCutMarkerBuffer == nullptr ||
      m_advancedCutMarkerVertexCount == 0)
    return;

  cb->setShaderResources(m_srb.get());
  cb->setGraphicsPipeline(m_translucentFillPipeline.get());
  const QRhiCommandBuffer::VertexInput binding(m_advancedCutMarkerBuffer.get(), 0);
  cb->setVertexInput(0, 1, &binding);
  cb->draw(m_advancedCutMarkerVertexCount);
}

// ===========================================================================
// Phase 240 (GIZ-05): measure overlay lines. Parses the
// [int32 segmentCount][x,y,z pair]* stream from
// EditorViewModel::measureOverlayData into a GizmoVertex line soup (white,
// matching the assembly-measure dimension line color) and renders it with the
// shared line pipeline while gizmoMode == GizmoMeasure (3).
// ===========================================================================
bool RhiViewportRenderer::uploadMeasureOverlayBuffer(QRhiResourceUpdateBatch *updates)
{
  if (updates == nullptr || rhi() == nullptr)
    return false;
  if (m_measureOverlayBufferUploaded)
    return true;

  QVector<Vertex> vertices;
  if (m_measureOverlayData.size() >= 4) {
    qint32 segmentCount = 0;
    std::memcpy(&segmentCount, m_measureOverlayData.constData(), 4);
    const qint64 expected = 4 + qint64(segmentCount) * 6 * qint64(sizeof(float));
    if (segmentCount > 0 && m_measureOverlayData.size() >= expected) {
      const auto *coords = reinterpret_cast<const float *>(
          m_measureOverlayData.constData() + 4);
      vertices.reserve(segmentCount * 2);
      for (qint32 s = 0; s < segmentCount; ++s) {
        for (int e = 0; e < 2; ++e) {
          const float *c = coords + (qint64(s) * 2 + e) * 3;
          Vertex v;
          // libslic3r world (X,Y,Z) -> scene (X,Z,Y).
          v.x = c[0];
          v.y = c[2];
          v.z = c[1];
          v.r = 1.f;
          v.g = 1.f;
          v.b = 1.f;
          v.a = 1.f;
          vertices.append(v);
        }
      }
    }
  }

  const quint32 byteSize = quint32(vertices.size() * int(sizeof(Vertex)));
  if (!ensureBuffer(m_measureOverlayBuffer, byteSize, m_measureOverlayBufferBytes,
                    QRhiBuffer::VertexBuffer))
    return false;

  m_measureOverlayVertexCount = quint32(vertices.size());
  if (m_measureOverlayBuffer && byteSize > 0) {
    updates->uploadStaticBuffer(m_measureOverlayBuffer.get(), 0, byteSize,
                                vertices.constData());
  }
  m_measureOverlayBufferUploaded = true;
  return true;
}

void RhiViewportRenderer::renderMeasureOverlay(QRhiCommandBuffer *cb)
{
  if (cb == nullptr)
    return;
  // Gate to the Measure gizmo only (mode 3).
  if (m_gizmoMode != 3)
    return;
  if (m_linePipeline == nullptr || m_measureOverlayBuffer == nullptr ||
      m_measureOverlayVertexCount == 0)
    return;

  cb->setShaderResources(m_srb.get());
  cb->setGraphicsPipeline(m_linePipeline.get());
  const QRhiCommandBuffer::VertexInput binding(m_measureOverlayBuffer.get(), 0);
  cb->setVertexInput(0, 1, &binding);
  cb->draw(m_measureOverlayVertexCount);
}

// ===========================================================================
// Phase 240 (GIZ-03): flatten hovered-facet highlight. Parses the
// [int32 vertexCount][x,y,z triple]* stream from
// EditorViewModel::flattenHoverData and renders the hovered facet as a
// translucent highlight (upstream recolors the hovered flatten plane,
// GLGizmoFlatten.cpp:107) while gizmoMode == GizmoFlatten (4).
// ===========================================================================
bool RhiViewportRenderer::uploadFlattenHoverBuffer(QRhiResourceUpdateBatch *updates)
{
  if (updates == nullptr || rhi() == nullptr)
    return false;
  if (m_flattenHoverBufferUploaded)
    return true;

  QVector<Vertex> vertices;
  if (m_flattenHoverData.size() >= 4) {
    qint32 vertexCount = 0;
    std::memcpy(&vertexCount, m_flattenHoverData.constData(), 4);
    const qint64 expected = 4 + qint64(vertexCount) * 3 * qint64(sizeof(float));
    if (vertexCount > 0 && m_flattenHoverData.size() >= expected) {
      const auto *coords = reinterpret_cast<const float *>(
          m_flattenHoverData.constData() + 4);
      vertices.reserve(vertexCount);
      for (qint32 i = 0; i < vertexCount; ++i) {
        const float *c = coords + qint64(i) * 3;
        Vertex v;
        // libslic3r world (X,Y,Z) -> scene (X,Z,Y).
        v.x = c[0];
        v.y = c[2];
        v.z = c[1];
        // Hover color: upstream FLATTEN_HOVER_COLOR is a strong cyan/blue.
        v.r = 0.118f;
        v.g = 0.565f;
        v.b = 1.0f;
        v.a = 0.55f;
        vertices.append(v);
      }
    }
  }

  const quint32 byteSize = quint32(vertices.size() * int(sizeof(Vertex)));
  if (!ensureBuffer(m_flattenHoverBuffer, byteSize, m_flattenHoverBufferBytes,
                    QRhiBuffer::VertexBuffer))
    return false;

  m_flattenHoverVertexCount = quint32(vertices.size());
  if (m_flattenHoverBuffer && byteSize > 0) {
    updates->uploadStaticBuffer(m_flattenHoverBuffer.get(), 0, byteSize,
                                vertices.constData());
  }
  m_flattenHoverBufferUploaded = true;
  return true;
}

void RhiViewportRenderer::renderFlattenHover(QRhiCommandBuffer *cb)
{
  if (cb == nullptr)
    return;
  // Gate to the Flatten gizmo only (mode 4).
  if (m_gizmoMode != 4)
    return;
  if (m_translucentFillPipeline == nullptr || m_flattenHoverBuffer == nullptr ||
      m_flattenHoverVertexCount == 0)
    return;

  cb->setShaderResources(m_srb.get());
  cb->setGraphicsPipeline(m_translucentFillPipeline.get());
  const QRhiCommandBuffer::VertexInput binding(m_flattenHoverBuffer.get(), 0);
  cb->setVertexInput(0, 1, &binding);
  cb->draw(m_flattenHoverVertexCount);
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
  float jerk, pressure_advance, actual_speed, actual_flow;  // v5.11: 4 extra fields
  int extruder_id, layer, move;
  int role;  // must match PackedSegment layout exactly (canonical libvgcode index).
};
// Wire-format lock-step guard: PackedSegment and GcvPackedSegment carry the
// identical 92-byte layout (19 floats + 4 ints) so the GCV1 blob memcpy is safe.
static_assert(sizeof(GcvPackedSegment) == 92, "GcvPackedSegment must be 92 bytes (19 floats + 4 ints)");
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

// Phase 238 (PREV-01): ghost shells -- semi-transparent object meshes drawn
// behind the preview toolpaths (upstream GCodeViewer ALWAYS renders the
// shells in preview: load_shells GCodeViewer.cpp:3076 + the always-on call
// site :983-988 "BBS: always load shell at preview", render_shells :4023
// with glDepthMask(GL_FALSE) + Transparent volume render). The vertices come
// from the SAME prepare-pass mesh staging (m_prepareScene.modelVertices --
// fed by PreviewPage.qml's meshData/meshBatch* bindings, mirroring
// PreparePage), with the per-vertex alpha overridden to the ghost alpha.
bool RhiViewportRenderer::uploadGhostShellBuffer(QRhiResourceUpdateBatch *updates)
{
  if (updates == nullptr || rhi() == nullptr)
    return false;
  // Rebuild only when the model generation changed (object edits) or after a
  // swapchain rebuild. Not tied to PrepareSceneData dirty flags: the preview
  // path never takes them, so they can stay set indefinitely.
  if (m_ghostShellBufferUploaded && m_ghostShellModelGeneration == m_modelGeneration)
    return true;

  const QVector<Vertex> source = buildModelVertices(m_prepareScene.modelVertices());
  if (source.isEmpty())
  {
    // No meshes loaded (e.g. empty plate) -- nothing to ghost; still record
    // the generation so empty scenes do not rebuild every frame.
    m_ghostShellModelGeneration = m_modelGeneration;
    m_ghostShellBufferUploaded = true;
    m_ghostShellVertexCount = 0;
    return true;
  }

  // Ghost alpha override (upstream renders the shell volumes in Transparent
  // mode so toolpaths stay readable on top).
  constexpr float kGhostAlpha = 0.35f;
  QVector<Vertex> ghost;
  ghost.resize(source.size());
  for (int i = 0; i < source.size(); ++i)
  {
    const Vertex &v = source[i];
    ghost[i] = Vertex{v.x, v.y, v.z, v.r, v.g, v.b, kGhostAlpha};
  }

  const quint32 byteSize = quint32(ghost.size()) * sizeof(Vertex);
  if (!ensureBuffer(m_ghostShellBuffer, byteSize, m_ghostShellBufferBytes,
                    QRhiBuffer::VertexBuffer))
    return false;
  updates->uploadStaticBuffer(m_ghostShellBuffer.get(), 0, byteSize,
                              ghost.constData());
  m_ghostShellVertexCount = quint32(ghost.size());
  m_ghostShellModelGeneration = m_modelGeneration;
  m_ghostShellBufferUploaded = true;
  return true;
}

// Phase 238 (PREV-02): tool marker -- the stylized-arrow nozzle at the
// current sequential-view position (upstream GCodeViewer::SequentialView::
// Marker::render GCodeViewer.cpp:306-330; geometry from Marker::init
// stilized_arrow(16, 1.5, 3.0, 0.8, 3.0) :285-292, white 0.5 alpha, flipped
// so the tip touches the position :295-299). Position/visibility come from
// the RhiViewport markerX/Y/Z/showMarker properties (driven by
// PreviewViewModel::updateToolPositionData during playback); the G-code
// (x,y,z) convention maps to scene (x,z,y) like the preview segments.
bool RhiViewportRenderer::uploadToolMarkerBuffer(QRhiResourceUpdateBatch *updates)
{
  if (updates == nullptr || rhi() == nullptr)
    return false;
  if (m_toolMarkerBufferUploaded)
    return true;
  if (!m_showMarker)
  {
    // Hidden: record as uploaded so the rebuild waits for the next real
    // position/visibility change (synchronize clears the flag).
    m_toolMarkerBufferUploaded = true;
    m_toolMarkerVertexCount = 0;
    return true;
  }

  const QVector<GizmoVertex> marker =
      GizmoGeometry::buildToolMarkerVertices(
          QVector3D(m_markerX, m_markerZ, m_markerY));
  if (marker.isEmpty())
    return false;

  const quint32 byteSize = quint32(marker.size()) * sizeof(GizmoVertex);
  if (!ensureBuffer(m_toolMarkerBuffer, byteSize, m_toolMarkerBufferBytes,
                    QRhiBuffer::VertexBuffer))
    return false;
  updates->uploadStaticBuffer(m_toolMarkerBuffer.get(), 0, byteSize,
                              marker.constData());
  m_toolMarkerVertexCount = quint32(marker.size());
  m_toolMarkerBufferUploaded = true;
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

