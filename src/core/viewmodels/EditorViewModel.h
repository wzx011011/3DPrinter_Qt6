#pragma once

#include <QObject>
#include <QList>
#include <QSet>
#include <QString>
#include <QByteArray>
#include <QVariant>
#include <QUrl>
#include <QVector3D>
#include <QVector4D>
#include <QHash>
#include <memory>

#include "core/rendering/SupportPaintTypes.h"
// Phase 93 (ASMROUTE-02): AssembleView data pool — caches per-object info for
// the AssembleView gizmos. Pure data (no libslic3r, no QRhi). See
// src/core/rendering/AssembleViewDataPool.h for the upstream mirror + the
// ModelObjectsClipper deferral.
#include "core/rendering/AssembleViewDataPool.h"
// PrepareSceneData.h is lightweight (Qt Core only — no QRhi, no libslic3r);
// included so the Assembly-measure bounds helper can return ModelBounds values
// (mirrors the GizmoCenter.h include pattern).
#include "qml_gui/Renderer/PrepareSceneData.h"
#include "qml_gui/Renderer/ViewportContextHit.h"
// Phase 100 (WTREAD-01): SliceService.h defines the WipeTowerGeometry POD
// struct captured by value in the worker; included here so the
// onWipeTowerGeometryReady slot signature has the complete type.
#include "core/services/SliceService.h"
// P15.11: drag-time sequential clearance preview. The header declares only
// plain-value structs (no libslic3r types) -- safe to include unconditionally.
#include "core/services/SequentialClearanceCompute.h"

class ProjectServiceMock;
class UndoRedoManager;
class ConfigViewModel;
class QTimer;
namespace OWzx {
class MeasureEngine;
class SceneRaycaster;  // Phase 115 (MEASURE-04): two-stage pick stage-2.
class PaintEngine;     // Phase 120 (PAINT-01): per-volume TriangleSelector owner.
} // namespace OWzx

class EditorViewModel final : public QObject
{
  Q_OBJECT
  // Phase 93 (ASMROUTE-02): test access to private pool/members for the
  // assembleViewDataPoolIsolatedFromPrepareAndPreview isolation slot.
  friend class ViewModelSmokeTests;
  Q_PROPERTY(QString projectName READ projectName NOTIFY stateChanged)
  Q_PROPERTY(int modelCount READ modelCount NOTIFY stateChanged)
  Q_PROPERTY(int plateCount READ plateCount NOTIFY stateChanged)
  Q_PROPERTY(int maxPlateCount READ maxPlateCount CONSTANT)
  Q_PROPERTY(bool canAddPlate READ canAddPlate NOTIFY stateChanged)
  Q_PROPERTY(int currentPlateIndex READ currentPlateIndex NOTIFY stateChanged)
  Q_PROPERTY(QVariantList activePlateObjectIndices READ activePlateObjectIndices NOTIFY stateChanged)
  Q_PROPERTY(QVariantList meshBatchSourceObjectIndices READ meshBatchSourceObjectIndices NOTIFY stateChanged)
  Q_PROPERTY(QVariantList meshBatchVolumeIndices READ meshBatchVolumeIndices NOTIFY stateChanged)
  Q_PROPERTY(QVariantList meshBatchInstanceIndices READ meshBatchInstanceIndices NOTIFY stateChanged)
  // P15.1/15.2 (COLOR): per-batch render-channel metadata paralleling
  // meshBatchSourceObjectIndices. Drives the upstream model coloring
  // (filament_colour by extruder, translucent modifier volumes, unprintable
  // black) in PrepareSceneData.
  Q_PROPERTY(QVariantList meshBatchVolumeTypes READ meshBatchVolumeTypes NOTIFY stateChanged)
  Q_PROPERTY(QVariantList meshBatchExtruderIds READ meshBatchExtruderIds NOTIFY stateChanged)
  Q_PROPERTY(QVariantList meshBatchPrintableFlags READ meshBatchPrintableFlags NOTIFY stateChanged)
  // Phase 138 (ASM-01): per-source-object assemble offset (GL X,Y,Z), one entry
  // per source object index, paralleling meshBatchSourceObjectIndices. Consumed
  // by RhiViewport on the CanvasAssembleView path so assembled volumes render at
  // their moved pose. Source: ModelInstance::m_assemble_transformation offset
  // (Model.hpp:1289) via ProjectServiceMock::assembleOffset.
  Q_PROPERTY(QVariantList assembleOffsets READ assembleOffsets NOTIFY stateChanged)
  // Phase 141 (DEBT-04): per-source-object assemble rotation (Euler XYZ, radians)
  // and scale (XYZ), parallel to assembleOffsets. Read via ProjectServiceMock
  // assembleRotation/assembleScale (ModelInstance::m_assemble_transformation).
  // Bound to RhiViewport on CanvasAssembleView; the renderer composes the full
  // transform matrix (translate * rotate * scale) in buildModelVertices so
  // rotate/scale drags reflect in the live render (v4.8 tech debt: translate-only).
  Q_PROPERTY(QVariantList assembleRotations READ assembleRotations NOTIFY stateChanged)
  Q_PROPERTY(QVariantList assembleScales READ assembleScales NOTIFY stateChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
  Q_PROPERTY(int loadProgress READ loadProgress NOTIFY stateChanged)
  Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
  Q_PROPERTY(bool showAllObjects READ showAllObjects NOTIFY stateChanged)
  Q_PROPERTY(int objectOrganizeMode READ objectOrganizeMode NOTIFY stateChanged)
  Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY stateChanged)
  Q_PROPERTY(bool hasSelectedVolume READ hasSelectedVolume NOTIFY stateChanged)
  Q_PROPERTY(int selectedVolumeCount READ selectedVolumeCount NOTIFY stateChanged)
  Q_PROPERTY(bool canOpenSelectionSettings READ canOpenSelectionSettings NOTIFY stateChanged)
  Q_PROPERTY(bool canRenameSelectedObject READ canRenameSelectedObject NOTIFY stateChanged)
  Q_PROPERTY(bool canDuplicateSelectedObjects READ canDuplicateSelectedObjects NOTIFY stateChanged)
  Q_PROPERTY(bool canDeleteSelection READ canDeleteSelection NOTIFY stateChanged)
  Q_PROPERTY(bool canSetSelectionPrintable READ canSetSelectionPrintable NOTIFY stateChanged)
  Q_PROPERTY(bool canTransformSelection READ canTransformSelection NOTIFY stateChanged)
  Q_PROPERTY(bool canArrangeObjects READ canArrangeObjects NOTIFY stateChanged)
  Q_PROPERTY(int availableGizmoMask READ availableGizmoMask NOTIFY stateChanged)
  Q_PROPERTY(QString settingsTargetType READ settingsTargetType NOTIFY stateChanged)
  Q_PROPERTY(QString settingsTargetName READ settingsTargetName NOTIFY stateChanged)
  Q_PROPERTY(int settingsTargetObjectIndex READ settingsTargetObjectIndex NOTIFY stateChanged)
  Q_PROPERTY(int settingsTargetVolumeIndex READ settingsTargetVolumeIndex NOTIFY stateChanged)
  // Object-list panel support
  Q_PROPERTY(int objectCount READ objectCount NOTIFY stateChanged)
  // Phase 236 (DLG-03): RecenterDialog data — entries are
  // { "index": <source object index>, "name": <display name> } maps for every
  // object whose estimated footprint leaves the printable area. Refreshed by
  // checkObjectsOutsideBed() via stateChanged (upstream RecenterDialog input).
  Q_PROPERTY(QVariantList objectsOutsideBed READ objectsOutsideBed NOTIFY stateChanged)
  // Phase 236 (DLG-03): ObjColorDialog data — "#RRGGBB" strings parsed from
  // the .mtl next to a freshly imported .obj (upstream
  // ObjImportColorDialog). Non-empty only between load completion and
  // applyPendingObjColors/dismiss; objColorMappingRequested announces it.
  Q_PROPERTY(QVariantList pendingObjColors READ pendingObjColors NOTIFY stateChanged)
  Q_PROPERTY(int pendingObjObjectIndex READ pendingObjObjectIndex NOTIFY stateChanged)
  Q_PROPERTY(int selectedObjectIndex READ selectedObjectIndex NOTIFY stateChanged)
  Q_PROPERTY(int selectedSourceObjectIndex READ selectedSourceObjectIndex NOTIFY stateChanged)
  Q_PROPERTY(int selectedObjectCount READ selectedObjectCount NOTIFY stateChanged)
  // Upstream GLCanvas3D.cpp:4309-4327: the constrained camera orbit pivots
  // on the selection bbox center, else the whole-model bbox center (GL world
  // coords, via ProjectServiceMock::selectionWorldBoundingBox). Null when no
  // objects exist -- the orbit then pivots on the camera target like the
  // upstream rotate_on_sphere fallback.
  Q_PROPERTY(QVariant rotationCenter READ rotationCenter NOTIFY stateChanged)
  /// Phase 198 (PHASE198): currently selected single volume index for the
  /// per-object settings / layer-range dialogs. Exposed so QML dialogs bound
  /// to `editorVm.selectedVolumeIndex` resolve (previously undefined).
  Q_PROPERTY(int selectedVolumeIndex READ selectedVolumeIndex NOTIFY stateChanged)
  /// P15.11 (SIDEBAR-LOCAL-AXES): orientation of the sidebar transform hint
  /// arrows on the selected object's local axes (upstream Selection.cpp
  /// orient_matrix: instance rotation, composed with the volume rotation for
  /// a single-volume selection). {rx, ry, rz} Euler degrees in the slic3r
  /// frame; {0, 0, 0} when there is no selection (world axes preserved).
  Q_PROPERTY(QVariantList selectedHintLocalRotation READ selectedHintLocalRotation NOTIFY stateChanged)
  Q_PROPERTY(int contextMenuFamily READ contextMenuFamily NOTIFY stateChanged)
  Q_PROPERTY(int contextSourceObjectIndex READ contextSourceObjectIndex NOTIFY stateChanged)
  Q_PROPERTY(int contextVolumeIndex READ contextVolumeIndex NOTIFY stateChanged)
  Q_PROPERTY(int contextInstanceIndex READ contextInstanceIndex NOTIFY stateChanged)
  Q_PROPERTY(int contextPlateIndex READ contextPlateIndex NOTIFY stateChanged)
  /// 模型网格数据（TLV 格式，用于 GLViewport 渲染）
  Q_PROPERTY(QByteArray meshData READ meshData NOTIFY stateChanged)
  // P15.11: post-validate clearance streams, packed as raw float XYZ triples.
  Q_PROPERTY(QByteArray sequentialClearanceOutline READ sequentialClearanceOutline NOTIFY sequentialClearanceChanged)
  Q_PROPERTY(QByteArray sequentialClearanceFill READ sequentialClearanceFill NOTIFY sequentialClearanceChanged)
  Q_PROPERTY(QByteArray sequentialHeightFill READ sequentialHeightFill NOTIFY sequentialClearanceChanged)
  // P15.11: true while the sequentialClearance* streams hold the DRAG-TIME
  // preview geometry (upstream update_sequential_clearance, render_fill=false
  // gray hulls) instead of the post-validate failure payload. Bound to
  // RhiViewport so the renderer switches to the NO_FILL gray color and keeps
  // drawing during the gizmo drag (upstream resets the overlay at mouse-up).
  Q_PROPERTY(bool sequentialClearancePreviewMode READ sequentialClearancePreviewMode NOTIFY sequentialClearanceChanged)
  /// 加载完成后的相机适应提示: (cx, cy, cz, radius)，全零表示无效
  Q_PROPERTY(QVector4D fitHint READ fitHint NOTIFY stateChanged)
  /// 选中对象边界框尺寸 (dx, dy, dz, volume)，全零表示无选中
  Q_PROPERTY(QVector4D measureDimensions READ measureDimensions NOTIFY stateChanged)
  /// 选中对象详细信息文本（对齐上游 Plater::show_object_info）
  Q_PROPERTY(QString selectedObjectInfoText READ selectedObjectInfoText NOTIFY stateChanged)
  /// 选中对象三角面数
  Q_PROPERTY(int selectedObjectTriangles READ selectedObjectTriangles NOTIFY stateChanged)
  /// 选中对象未修复的非流形边数（对齐上游 TriangleMeshStats::open_edges）
  Q_PROPERTY(int selectedObjectOpenEdges READ selectedObjectOpenEdges NOTIFY stateChanged)
  /// 选中对象已修复错误总数（对齐上游 ModelObject::get_repaired_errors_count）
  Q_PROPERTY(int selectedObjectRepairedErrors READ selectedObjectRepairedErrors NOTIFY stateChanged)
  /// 选中对象是否为流形网格（对齐上游 TriangleMeshStats::manifold）
  Q_PROPERTY(bool selectedObjectIsManifold READ selectedObjectIsManifold NOTIFY stateChanged)
  /// 对象变换（对齐上游 ObjectManipulation）
  Q_PROPERTY(float objectPosX READ objectPosX WRITE setObjectPosX NOTIFY stateChanged)
  Q_PROPERTY(float objectPosY READ objectPosY WRITE setObjectPosY NOTIFY stateChanged)
  Q_PROPERTY(float objectPosZ READ objectPosZ WRITE setObjectPosZ NOTIFY stateChanged)
  Q_PROPERTY(float objectRotX READ objectRotX WRITE setObjectRotX NOTIFY stateChanged)
  Q_PROPERTY(float objectRotY READ objectRotY WRITE setObjectRotY NOTIFY stateChanged)
  Q_PROPERTY(float objectRotZ READ objectRotZ WRITE setObjectRotZ NOTIFY stateChanged)
  Q_PROPERTY(float objectScaleX READ objectScaleX WRITE setObjectScaleX NOTIFY stateChanged)
  Q_PROPERTY(float objectScaleY READ objectScaleY WRITE setObjectScaleY NOTIFY stateChanged)
  Q_PROPERTY(float objectScaleZ READ objectScaleZ WRITE setObjectScaleZ NOTIFY stateChanged)
  Q_PROPERTY(bool uniformScale READ uniformScale WRITE setUniformScale NOTIFY stateChanged)
  Q_PROPERTY(bool hasObjectManipSelection READ hasObjectManipSelection NOTIFY stateChanged)
  /// Undo/Redo (对齐上游 UndoRedo 框架)
  Q_PROPERTY(bool canUndo READ canUndo NOTIFY stateChanged)
  Q_PROPERTY(bool canRedo READ canRedo NOTIFY stateChanged)
  Q_PROPERTY(bool showLabels READ showLabels WRITE setShowLabels NOTIFY stateChanged)
  /// Phase 91 (ASMEXPLODE-01): explosion ratio for AssembleView multi-part
  /// separation. Mirrors upstream m_explosion_ratio (default 1.0,
  /// GLCanvas3D.hpp:596). NOTIFY stateChanged drives the QML slider readout AND
  /// the RhiViewport re-render (the offset pass keys on this value).
  Q_PROPERTY(float explosionRatio READ explosionRatio WRITE setExplosionRatio NOTIFY stateChanged)
  // Phase 92 (ASMMEASURE-01/02): Assembly measurement gizmo state. The gizmo
  // is activable only on AssembleView with explosion ratio ~= 1.0 AND >=2
  // volumes selected (mirrors GLGizmoAssembly::on_is_activable,
  // GLGizmoAssembly.cpp:53-68). The distance/angle/plane accessors expose the
  // current measurement between the first two selected volumes; they are
  // computed via AssemblyMeasureGeometry (AABB-center distance + longest-axis
  // angle — documented Phase 92 simplification of the full feature-picking
  // engine, which needs ITS + raycaster + data pool from Phase 93).
  Q_PROPERTY(bool assemblyMeasureGizmoActive READ assemblyMeasureGizmoActive NOTIFY stateChanged)
  Q_PROPERTY(QString assemblyMeasureDistanceText READ assemblyMeasureDistanceText NOTIFY stateChanged)
  Q_PROPERTY(QString assemblyMeasureAngleText READ assemblyMeasureAngleText NOTIFY stateChanged)
  Q_PROPERTY(QVector3D assemblyMeasureDistanceXyz READ assemblyMeasureDistanceXyz NOTIFY stateChanged)
  Q_PROPERTY(QString assemblyMeasurePlaneText READ assemblyMeasurePlaneText NOTIFY stateChanged)
  // Phase 114 (MEASURE-03): real feature-picking measurement readouts. The
  // Assembly Q_PROPERTYs above are the COARSE AABB-center multi-volume
  // fallback (AssemblyMeasureGeometry, Phase 92). These are the PRECISE
  // single-feature readouts from MeasureEngine (Measure::Measuring on the
  // per-volume ITS). The Phase 115 snap UX binds the measure*Text readouts
  // directly; the raw float/vector Q_PROPERTYs are exposed for debug + the
  // Phase 115 point/edge/circle/plane overlay. measureReadoutValid is the
  // gate (mirrors the WTREAD-02 / FMAP-01 valid-flag pattern): false until a
  // valid measurement is computed, so no stale readout leaks to QML.
  //
  // Mirrors upstream GLGizmoMeasure.cpp:1990-2048 readout table: angle
  // (AngleAndEdges.angle), perpendicular distance (distance_infinite),
  // direct distance (distance_strict), distance XYZ (distance_xyz).
  Q_PROPERTY(bool measureReadoutValid READ measureReadoutValid NOTIFY measureReadoutChanged)
  Q_PROPERTY(QString measureAngleText READ measureAngleText NOTIFY measureReadoutChanged)
  Q_PROPERTY(QString measurePerpendicularDistanceText READ measurePerpendicularDistanceText NOTIFY measureReadoutChanged)
  Q_PROPERTY(QString measureDirectDistanceText READ measureDirectDistanceText NOTIFY measureReadoutChanged)
  Q_PROPERTY(QString measureDistanceXyzText READ measureDistanceXyzText NOTIFY measureReadoutChanged)
  Q_PROPERTY(float measureAngleDeg READ measureAngleDeg NOTIFY measureReadoutChanged)
  Q_PROPERTY(float measurePerpendicularDistance READ measurePerpendicularDistance NOTIFY measureReadoutChanged)
  Q_PROPERTY(float measureDirectDistance READ measureDirectDistance NOTIFY measureReadoutChanged)
  Q_PROPERTY(QVector3D measureDistanceXyz READ measureDistanceXyz NOTIFY measureReadoutChanged)

public:
  enum ContextMenuFamily
  {
    ContextMenuNone = -1,
    ContextMenuDefault = 0,
    ContextMenuObject = 1,
    ContextMenuPart = 2,
    ContextMenuMulti = 3,
    ContextMenuPlate = 4,
    ContextMenuText = 5,
    ContextMenuSvg = 6
  };
  Q_ENUM(ContextMenuFamily)

  enum SliceResultStatus {
    SliceResultMissing = 0,
    SliceResultValid = 1,
    SliceResultStale = 2
  };
  Q_ENUM(SliceResultStatus)

  explicit EditorViewModel(ProjectServiceMock *projectService, SliceService *sliceService, QObject *parent = nullptr);
  // Phase 114 (MEASURE-03): destructor defined out-of-line in the .cpp so the
  // unique_ptr<MeasureEngine> member (forward-declared here) can be destroyed
  // where MeasureEngine is a complete type.
  ~EditorViewModel();

  QString projectName() const;
  int modelCount() const;
  int plateCount() const;
  int maxPlateCount() const;
  int currentPlateIndex() const;
  QVariantList activePlateObjectIndices() const;
  QVariantList meshBatchSourceObjectIndices() const;
  QVariantList meshBatchVolumeIndices() const;
  QVariantList meshBatchInstanceIndices() const;
  // P15.1/15.2 (COLOR): per-batch render-channel metadata.
  QVariantList meshBatchVolumeTypes() const;
  QVariantList meshBatchExtruderIds() const;
  QVariantList meshBatchPrintableFlags() const;
  // Phase 138 (ASM-01): per-source-object assemble offset list (one QVector3D
  // per source object index). Mirrors meshBatchSourceObjectIndices indexing.
  QVariantList assembleOffsets() const;
  // Phase 141 (DEBT-04): per-source-object assemble rotation/scale lists.
  QVariantList assembleRotations() const;
  QVariantList assembleScales() const;
  QString statusText() const;
  int loadProgress() const;
  bool loading() const;
  bool showAllObjects() const;
  bool showLabels() const;
  int objectOrganizeMode() const;
  bool hasSelection() const;
  bool hasSelectedVolume() const;
  int selectedVolumeCount() const;
  bool canOpenSelectionSettings() const;
  bool canRenameSelectedObject() const;
  bool canDuplicateSelectedObjects() const;
  bool canDeleteSelection() const;
  bool canSetSelectionPrintable() const;
  bool canTransformSelection() const;
  bool canArrangeObjects() const;
  int availableGizmoMask() const;
  QString settingsTargetType() const;
  QString settingsTargetName() const;
  int settingsTargetObjectIndex() const;
  int settingsTargetVolumeIndex() const;
  QByteArray meshData() const;
  QByteArray sequentialClearanceOutline() const { return m_sequentialClearanceOutline; }
  QByteArray sequentialClearanceFill() const { return m_sequentialClearanceFill; }
  QByteArray sequentialHeightFill() const { return m_sequentialHeightFill; }
  // P15.11: drag-time preview flag (see the Q_PROPERTY comment).
  bool sequentialClearancePreviewMode() const { return m_sequentialClearancePreviewMode; }
  QVector4D fitHint() const { return m_fitHint; }
  QVector4D measureDimensions() const { return m_measureDimensions; }

  // Object info (对齐上游 Plater::show_object_info)
  QString selectedObjectInfoText() const;
  int selectedObjectTriangles() const;
  int selectedObjectOpenEdges() const;
  int selectedObjectRepairedErrors() const;
  bool selectedObjectIsManifold() const;

  // Object manipulation (对齐上游 ObjectManipulation)
  float objectPosX() const;
  float objectPosY() const;
  float objectPosZ() const;
  float objectRotX() const;
  float objectRotY() const;
  float objectRotZ() const;
  float objectScaleX() const;
  float objectScaleY() const;
  float objectScaleZ() const;
  bool uniformScale() const;
  bool hasObjectManipSelection() const;

  // Flatten/Cut (对齐上游 GLGizmoFlatten / GLGizmoCut)
  int flattenFaceCount() const { return m_flattenFaceCount; }
  int cutAxis() const { return m_cutAxis; }
  void setCutAxis(int axis);
  qreal cutPosition() const { return m_cutPosition; }
  void setCutPosition(qreal pos);
  int cutKeepMode() const { return m_cutKeepMode; }
  void setCutKeepMode(int mode);
  // Cut connector settings (对齐上游 GLGizmoCut connector)
  int cutMode() const;
  void setCutMode(int mode);
  int connectorType() const;
  void setConnectorType(int v);
  int connectorStyle() const;
  void setConnectorStyle(int v);
  int connectorShape() const;
  void setConnectorShape(int v);
  float connectorSize() const;
  /// v5.12 gap-closure: independent groove parameters (对齐上游 Cut::Groove).
  /// When set, these override the connectorSize-derived defaults in advCutSelected.
  float grooveDepth() const;
  float grooveWidth() const;
  float grooveFlapsAngle() const;  // radians
  void setConnectorSize(float v);
  void setGrooveDepth(float v);
  void setGrooveWidth(float v);
  void setGrooveFlapsAngle(float v);
  float connectorDepth() const;
  void setConnectorDepth(float v);
  // Measure selection (对齐上游 GLGizmoMeasure)
  int measureSelectionMode() const;
  void setMeasureSelectionMode(int mode);
  // Support painting (对齐上游 GLGizmoFdmSupports)
  int supportPaintTool() const;
  void setSupportPaintTool(int tool);
  int supportPaintCursorType() const;
  void setSupportPaintCursorType(int type);
  int supportPaintToolType() const;
  void setSupportPaintToolType(int type);
  float supportPaintCursorRadius() const;
  void setSupportPaintCursorRadius(float radius);
  int activePaintKind() const;
  void setActivePaintKind(int kind);
  float supportPaintAngleThreshold() const;
  void setSupportPaintAngleThreshold(float angle);
  float supportPaintSmartFillAngle() const;
  void setSupportPaintSmartFillAngle(float angle);
  float supportPaintGapArea() const;
  void setSupportPaintGapArea(float area);
  bool supportPaintOnOverhangsOnly() const;
  void setSupportPaintOnOverhangsOnly(bool on);
  /// Phase 240 (GIZ-02): overhang filter angle in degrees (upstream
  /// m_highlight_by_angle_threshold_deg, GLGizmoPainterBase.hpp:280; sourced
  /// upstream from the object's support threshold config via
  /// get_selection_support_threshold_angle, GLGizmoFdmSupports.cpp:223-227 —
  /// Qt6 delta: exposed as a panel property until per-object config plumbing
  /// reaches the paint gizmo).
  float supportPaintOverhangAngle() const;
  void setSupportPaintOverhangAngle(float angle);
  bool supportEnable() const;
  void setSupportEnable(bool enable);
  int supportType() const;
  void setSupportType(int type);
  /// Set support paint tool from QML (tool: 0=enforcer, 1=blocker, 2=erase)
  Q_INVOKABLE void setSupportPaintToolFromQml(int tool);
  /// Clear all support painting on selected volumes (aligns with upstream "Erase all painting")
  Q_INVOKABLE void clearSupportPaintOnSelection();
  /// 设置三角形支撑状态（对齐上游 TriangleSelector select_triangle）
  Q_INVOKABLE void setTriangleSupportState(int objectIndex, int triangleIndex, int paintState);
  /// 清除所有绘制数据（对齐上游 TriangleSelector reset）
  Q_INVOKABLE void clearAllPaintData();
  // Phase 120 (PAINT-01 / TS-05): the paint-pick entry point. Mirrors the
  // upstream GLGizmoPainterBase mouse-down flow:
  //   1. Stage-1 (RhiViewport::pickSourceObjectAt) already narrowed the scene
  //      to pickedSourceIndex (cheap ray->AABB prefilter).
  //   2. Stage-2 (HERE -- SceneRaycaster::hitTest) runs the per-triangle ITS
  //      raycast over the candidate volume(s) and returns facetIdx + the
  //      mesh-local hit point (TS-02 meshLocalPosition).
  //   3. PaintEngine::paintAt drives TriangleSelector::select_patch with a
  //      cursor built from brushRadius/cursorType (TS-04).
  //
  // rayOrigin/rayDir are the world-space pick ray (QVector3D from QML, opaque
  // -- no ray math in QML). pickedSourceIndex is the stage-1 survivor.
  // hit{X,Y,Z} are the world-space hit (carried through for back-compat /
  // future overlay anchor). state is the EnforcerBlockerType int
  // (0=None,1=Enforcer,2=Blocker, 3..N=ExtruderN for MMU). brushRadius is in
  // world mm. cursorType is PaintCursorType (0=Circle,1=Sphere).
  //
  // Returns true when a facet was hit and the selector was painted.
  Q_INVOKABLE bool paintAtFacet(int obj, int vol, int facetIdx,
                                double hitX, double hitY, double hitZ,
                                int state, double brushRadius, int cursorType,
                                int pickedSourceIndex,
                                QVector3D rayOrigin, QVector3D rayDir,
                                QVector3D cameraPosition);
  /// Phase 240 (GIZ-02): smart (seed) fill pick entry. Same two-stage pick
  /// contract as paintAtFacet, but instead of the brush cursor it drives
  /// TriangleSelector::seed_fill_select_triangles + seed_fill_apply_on_
  /// triangles (upstream SMART_FILL tool click, GLGizmoPainterBase.cpp:
  /// 773-781). seedFillAngle is the region-growth threshold bound to the
  /// supportPaintSmartFillAngle Q_PROPERTY; overhangsOnly threads the
  /// overhang filter angle (upstream m_paint_on_overhangs_only ->
  /// m_highlight_by_angle_threshold_deg). Returns true when a facet was hit
  /// and the seed fill committed.
  Q_INVOKABLE bool smartFillAtFacet(int state,
                                    double seedFillAngle,
                                    bool overhangsOnly,
                                    double overhangAngle,
                                    int pickedSourceIndex,
                                    QVector3D rayOrigin, QVector3D rayDir);
  /// Phase 120 (PAINT-01): drop all PaintEngine selectors for one object.
  /// Called on gizmo-exit cleanup (mirrors GLGizmoPainterBase on_exit).
  Q_INVOKABLE void clearPaintOnObject(int objectIndex);
  /// v5.16 (UNDO-04): re-sync the PaintEngine's cached TriangleSelector for
  /// (objectIndex, volumeIndex) from the ModelVolume FacetsAnnotation of the
  /// given paint kind (0=Support, 1=Seam, 2=Mmu). Called by PaintCommand after
  /// undo/redo restored the annotation — the PaintEngine cache does NOT
  /// auto-reload from the ModelVolume, so without this the overlay would keep
  /// rendering the pre-undo paint (mirrors upstream GLGizmoPainterBase
  /// data_changed -> update_model_object round-trip).
  Q_INVOKABLE void resyncPaintSelector(int objectIndex, int volumeIndex, int kind);
  int enforcedSupportCount() const;
  int blockedSupportCount() const;
  int totalPaintedTriangleCount() const;
  // Phase 121 (PAINT-02): reverse-channel getter. Flattens the selected
  // object's painted facets (state set chosen by the current gizmo) into a
  // world-transformed byte stream for the RHI overlay renderer + Software
  // mirror. See the Q_PROPERTY comment above for the wire format.
  QByteArray paintOverlayData() const;
  // Phase 121 (PAINT-02/OV-04): MMU per-extruder filament colors as hex strings
  // (matches PreviewViewModel::extruderColor). Used by the overlay renderer to
  // color ExtruderN facets.
  QVariantList extrudersColors() const;
  // Seam painting (对齐上游 GLGizmoSeam)
  int seamPaintTool() const;
  void setSeamPaintTool(int tool);
  float seamPaintCursorRadius() const;
  void setSeamPaintCursorRadius(float radius);
  bool seamPaintOnOverhangsOnly() const;
  void setSeamPaintOnOverhangsOnly(bool on);
  /// Clear all seam painting on selected volumes
  Q_INVOKABLE void clearSeamPaintOnSelection();
  // Hollow gizmo (对齐上游 GLGizmoHollow)
  bool hollowEnabled() const;
  void setHollowEnabled(bool on);
  float hollowHoleRadius() const;
  void setHollowHoleRadius(float r);
  float hollowHoleHeight() const;
  void setHollowHoleHeight(float h);
  float hollowOffset() const;
  void setHollowOffset(float v);
  float hollowQuality() const;
  void setHollowQuality(float v);
  float hollowClosingDistance() const;
  void setHollowClosingDistance(float v);
  int hollowHoleCount() const;
  QByteArray hollowMarkerData() const;
  Q_INVOKABLE void deleteSelectedHollowPoints();
  /// Phase 220 (HOLLOW-REFRESH): rebuild the hollow marker stream on demand.
  /// Called from QML on gizmo-mode change (entering GizmoHollow) so loaded
  /// drain holes render without a place/delete first.
  Q_INVOKABLE void refreshHollowMarkers();
  /// Place a drain hole at the mesh surface hit by the given world ray
  /// (对齐上游 GLGizmoHollow::on_mouse → unproject_on_mesh). Reuses the
  /// shared SceneRaycaster to find the mesh-local intersection, then appends
  /// a sla::DrainHole to ModelObject::sla_drain_holes via ProjectServiceMock.
  /// Returns true if a hole was placed.
  Q_INVOKABLE bool placeHollowPoint(int pickedSourceIndex, QVector3D rayOrigin, QVector3D rayDir);
  // Simplify gizmo (对齐上游 GLGizmoSimplify)
  int simplifyWantedCount() const;
  void setSimplifyWantedCount(int count);
  float simplifyMaxError() const;
  void setSimplifyMaxError(float error);
  Q_INVOKABLE bool simplifySelected();
  /// Phase 240 (GIZ-06): upstream GLGizmoSimplify three-stage flow (start /
  /// preview / apply + cancel, GLGizmoSimplify.cpp process / close / apply).
  /// simplifyPreviewStart runs the decimation on a mesh COPY off the model
  /// (no mutation), exposing the resulting facet count via
  /// simplifyPreviewTriangleCount. simplifyPreviewApply commits through the
  /// existing simplifySelected() undo path; simplifyPreviewCancel discards.
  Q_INVOKABLE void simplifyPreviewStart();
  Q_INVOKABLE void simplifyPreviewApply();
  Q_INVOKABLE void simplifyPreviewCancel();
  /// True while the preview decimation job is in flight.
  bool simplifyPreviewRunning() const { return m_simplifyPreviewRunning; }
  /// Facet count after the last preview decimation (-1 = no preview).
  int simplifyPreviewTriangleCount() const { return m_simplifyPreviewTriangleCount; }
  /// True when a preview result is staged and Apply/Cancel is offered.
  bool simplifyPreviewValid() const { return m_simplifyPreviewTriangleCount >= 0; }
  Q_INVOKABLE int selectedObjectTriangleCount() const;
  // MMU segmentation gizmo (对齐上游 GLGizmoMmuSegmentation)
  int mmuSelectedExtruder() const;
  void setMmuSelectedExtruder(int idx);
  int mmuExtruderCount() const;
  QStringList mmuExtruderColors() const;
  Q_INVOKABLE bool clearMmuSegmentation();

  /// ── Drill gizmo properties (对齐上游 GLGizmoDrill) ──
  float drillRadius() const;
  void setDrillRadius(float r);
  float drillDepth() const;
  void setDrillDepth(float d);
  int drillShape() const;
  void setDrillShape(int s);
  int drillDirection() const;
  void setDrillDirection(int d);
  bool drillOneLayerOnly() const;
  void setDrillOneLayerOnly(bool v);
  Q_INVOKABLE bool drillSelected();

  /// ── Emboss gizmo properties (对齐上游 GLGizmoEmboss) ──
  QString embossText() const;
  void setEmbossText(const QString &t);
  float embossHeight() const;
  void setEmbossHeight(float h);
  float embossDepth() const;
  void setEmbossDepth(float d);
  /// Phase 144 (EMB-01): user-selected font path (empty = default arial.ttf).
  QString embossFontPath() const;
  void setEmbossFontPath(const QString &path);
  /// Phase 158 (EMBO-F01): style axes. boldness + italic map to upstream
  /// FontProp; use-surface + curve-projection are projection concepts persisted
  /// for round-trip (geometry deformation deferred — see header note above).
  float embossBoldness() const;
  void setEmbossBoldness(float b);
  bool embossItalic() const;
  void setEmbossItalic(bool i);
  bool embossUseSurface() const;
  void setEmbossUseSurface(bool u);
  bool embossCurveProjection() const;
  void setEmbossCurveProjection(bool c);
  /// Phase 158 (EMBO-F02): SVG depth-modifier (Z-scale; 1.0 = no change).
  float svgDepthModifier() const;
  void setSvgDepthModifier(float m);
  /// Phase 144 (EMB-01): enumerate system fonts (proxies ProjectServiceMock).
  Q_INVOKABLE QVariantList embossFontList() const;
  Q_INVOKABLE bool embossSelected();
  /// Phase 145 (EMB-03): async variant of embossSelected. Forwards inputs to
  /// ProjectServiceMock::addTextVolumeAsync. The result arrives via the
  /// embossVolumeAdded / embossVolumeFailed signals (which QML binds to).
  /// A second invocation auto-cancels the prior job.
  Q_INVOKABLE void embossSelectedAsync();
  Q_INVOKABLE void cancelEmboss();
  /// Phase 196 (FEAT-01): true while an async emboss job is in flight.
  bool embossRunning() const;
  /// Phase 240 (GIZ-06): in-place emboss editing (upstream GLGizmoEmboss
  /// edits the SELECTED text volume's TextConfiguration and re-generates
  /// its mesh in place instead of adding a new volume). Reads the selected
  /// volume's stored text configuration into the emboss* Q_PROPERTYs so the
  /// panel populates on selection. Returns true when the selected volume is
  /// a re-editable text volume and the configuration was loaded.
  Q_INVOKABLE bool loadEmbossFromSelectedVolume();
  /// Phase 240 (GIZ-06): true when the currently selected volume carries a
  /// TextConfiguration (panel switches to re-generate mode).
  bool embossEditingExistingVolume() const;
  /// Phase 240 (GIZ-06): re-generate the SELECTED text volume's mesh from
  /// the current emboss* Q_PROPERTYs (upstream GLGizmoEmboss in-place
  /// editing). Returns true when the volume was regenerated. When the
  /// selected volume is NOT a text volume this falls back to embossSelected
  /// (create mode) so the panel button keeps working in both modes.
  Q_INVOKABLE bool updateEmbossVolume();

  /// ── MeshBoolean gizmo properties (对齐上游 GLGizmoMeshBoolean) ──
  int booleanOperation() const; // 0=union, 1=diff, 2=intersect
  void setBooleanOperation(int op);
  Q_INVOKABLE bool booleanExecute();

  /// ── AdvancedCut gizmo properties (对齐上游 GLGizmoAdvancedCut) ──
  int advCutAxis() const; // 0=X, 1=Y, 2=Z
  void setAdvCutAxis(int a);
  float advCutPosition() const;
  void setAdvCutPosition(float p);
  bool advCutKeepBoth() const;
  void setAdvCutKeepBoth(bool v);
  bool advCutConnectors() const;
  void setAdvCutConnectors(bool v);
  Q_INVOKABLE bool advCutSelected();
  /// v5.13 gap-closure: place a connector pin on the mesh surface hit by the
  /// given world ray（对齐上游 GLGizmoAdvancedCut add_connector）。The pin's
  /// axis aligns to the current adv-cut axis; radius/height/type/style/shape
  /// come from the connector* properties. Returns true when placed.
  Q_INVOKABLE bool placeAdvancedCutConnector(int pickedSourceIndex,
                                             QVector3D rayOrigin, QVector3D rayDir);
  /// v5.13: placed connector count on the selected object（面板显示）。
  Q_INVOKABLE int advancedCutConnectorCount() const;
  QByteArray advancedCutMarkerData() const;
  /// v5.13: rebuild pin markers on gizmo entry (called from QML
  /// onGizmoModeChanged), mirroring refreshHollowMarkers.
  Q_INVOKABLE void refreshAdvancedCutConnectors();
  /// v5.13: clear the selected object's connector pins.
  Q_INVOKABLE bool clearAdvancedCutConnectors();

  /// ── FaceDetector gizmo properties (对齐上游 GLGizmoFaceDetector) ──
  float faceDetectorAngle() const;
  int faceDetectorResultCount() const;
  void setFaceDetectorAngle(float a);
  Q_INVOKABLE bool detectFlatFaces();

  /// ── Text gizmo properties (对齐上游 GLGizmoText) ──
  QString textContent() const;
  void setTextContent(const QString &t);
  float textSize() const;
  void setTextSize(float s);
  Q_INVOKABLE bool addTextObject();

  /// ── SVG gizmo properties (对齐上游 GLGizmoSVG) ──
  QString svgFilePath() const;
  void setSvgFilePath(const QString &p);
  float svgScale() const;
  void setSvgScale(float s);
  Q_INVOKABLE bool importSVG();
  /// Phase 240 (GIZ-06): SVG drop workbench completion (minimal port of the
  /// upstream GLGizmoSVG workbench). A dropped .svg creates a NEW object
  /// whose SVG volume is placed at the given bed point (the QML side
  /// resolves the drop position to a bed-plane point via the viewport; the
  /// upstream exact ray-hit placement is approximated by the bed-plane
  /// intersection). Move/rotate then go through the existing transform
  /// panel / gizmos. Returns the new object index (>=0) or -1.
  Q_INVOKABLE int placeSvgAtBedPoint(const QString &svgPath, float bedX, float bedY);

  void setObjectPosX(float v);
  void setObjectPosY(float v);
  void setObjectPosZ(float v);
  // Phase 69: begin/end bracket a move-gizmo drag so undo sees one
  // TransformCommand for the whole drag, not one command per frame.
  Q_INVOKABLE void beginGizmoMoveDrag();
  Q_INVOKABLE void applyGizmoMoveDelta(float dx, float dy, float dz);
  Q_INVOKABLE void endGizmoMoveDrag();
  Q_INVOKABLE void beginGizmoRotateDrag();
  Q_INVOKABLE void applyGizmoRotateDelta(int axis, float radians);
  Q_INVOKABLE void endGizmoRotateDrag();
  Q_INVOKABLE void beginGizmoScaleDrag();
  Q_INVOKABLE void applyGizmoScaleFactor(int axis, float factor);
  Q_INVOKABLE void endGizmoScaleDrag();
  void setObjectRotX(float v);
  void setObjectRotY(float v);
  void setObjectRotZ(float v);
  void setObjectScaleX(float v);
  void setObjectScaleY(float v);
  void setObjectScaleZ(float v);
  void setUniformScale(bool v);
  Q_INVOKABLE void resetObjectTransform();
  Q_INVOKABLE void applyScaleFactor(float factor);

  // Object list accessors (safe Q_INVOKABLE — no QVariantList)
  int objectCount() const;
  /// Phase 236 (DLG-03): backing store of the objectsOutsideBed Q_PROPERTY
  /// (populated by checkObjectsOutsideBed()).
  QVariantList objectsOutsideBed() const { return m_objectsOutsideBed; }
  QVariantList pendingObjColors() const { return m_pendingObjColors; }
  int pendingObjObjectIndex() const { return m_pendingObjObjectIndex; }
  int selectedObjectIndex() const;
  int selectedSourceObjectIndex() const;
  int selectedObjectCount() const;
  QVariant rotationCenter() const;
  /// Phase 198 (PHASE198): getter backing the selectedVolumeIndex Q_PROPERTY.
  int selectedVolumeIndex() const;
  /// P15.11 (SIDEBAR-LOCAL-AXES): getter backing the selectedHintLocalRotation
  /// Q_PROPERTY. Returns {rx, ry, rz} degrees (slic3r frame) for the primary
  /// selection -- instance rotation, or instance * volume rotation when a
  /// single volume is selected; {0, 0, 0} when nothing is selected.
  QVariantList selectedHintLocalRotation() const;
  int contextMenuFamily() const;
  int contextSourceObjectIndex() const;
  int contextVolumeIndex() const;
  int contextInstanceIndex() const;
  int contextPlateIndex() const;
  Q_INVOKABLE int synchronizeViewportContext(int targetKind,
                                             int sourceObjectIndex,
                                             int volumeIndex,
                                             int instanceIndex,
                                             int plateIndex);
  Q_INVOKABLE bool contextActionAvailable(const QString &action) const;
  Q_INVOKABLE QString objectName(int i) const;
  Q_INVOKABLE QString objectModuleName(int i) const;
  Q_INVOKABLE QString objectGroupLabel(int i) const;
  Q_INVOKABLE int objectGroupCount(int i) const;
  Q_INVOKABLE bool objectGroupExpanded(int i) const;
  Q_INVOKABLE void toggleObjectGroupExpanded(int i);
  Q_INVOKABLE bool objectExpanded(int i) const;
  Q_INVOKABLE void toggleObjectExpanded(int i);
  Q_INVOKABLE int objectVolumeCount(int i) const;
  Q_INVOKABLE int objectInstanceCount(int i) const;
  Q_INVOKABLE QString objectVolumeName(int i, int volumeIndex) const;
  Q_INVOKABLE QString objectVolumeTypeLabel(int i, int volumeIndex) const;
  Q_INVOKABLE bool isVolumeSelected(int i, int volumeIndex) const;
  Q_INVOKABLE void selectVolume(int i, int volumeIndex);
  Q_INVOKABLE void toggleVolumeSelection(int i, int volumeIndex);
  Q_INVOKABLE void deleteVolume(int i, int volumeIndex);
  /// Add a volume to the selected object (对齐上游 GUI_ObjectList::load_subobject / load_generic_subobject)
  /// volumeType: 0=ModelPart, 1=NegativeVolume, 2=ParameterModifier, 3=SupportBlocker, 4=SupportEnforcer
  Q_INVOKABLE bool addVolumeToObject(int volumeType);
  /// Change volume type (对齐上游 GUI_ObjectList::load_generic_subobject type conversion)
  Q_INVOKABLE bool changeVolumeType(int newVolumeType);
  /// Volume-level extruder assignment (对齐上游 ModelVolume::extruder_id)
  /// Returns -1 if inherit from object, 0+ for specific extruder
  Q_INVOKABLE int volumeExtruderId(int objectIndex, int volumeIndex) const;
  /// Set volume extruder. -1 = inherit, 0+ = specific extruder
  Q_INVOKABLE bool setVolumeExtruderId(int objectIndex, int volumeIndex, int extruderId);
  /// 从外部文件导入 volume（对齐上游 GUI_ObjectList::load_generic_subobject 文件加载）
  Q_INVOKABLE bool addVolumeFromFile(int objectIndex, const QString &filePath, int volumeType);
  /// 添加原始体 volume（对齐上游 create_mesh + add_volume）
  Q_INVOKABLE bool addPrimitive(int objectIndex, int primitiveType, int volumeType = 0);
  /// 添加文字浮雕 volume（对齐上游 GLGizmoText）
  Q_INVOKABLE bool addTextVolume(int objectIndex, const QString &text);
  /// 添加 SVG 浮雕 volume（对齐上游 GLGizmoSVG）
  Q_INVOKABLE bool addSvgVolume(int objectIndex, const QString &svgFilePath);
  Q_INVOKABLE bool isObjectSelected(int i) const;
  Q_INVOKABLE bool objectPrintable(int i) const;
  Q_INVOKABLE void setObjectPrintable(int i, bool printable);
  Q_INVOKABLE void deleteObject(int i);
  Q_INVOKABLE void deleteSelectedObjects();
  Q_INVOKABLE void selectObject(int i);
  Q_INVOKABLE bool selectSourceObject(int sourceIndex);
  Q_INVOKABLE void toggleObjectSelection(int i);
  Q_INVOKABLE void clearObjectSelection();
  Q_INVOKABLE void selectAllVisibleObjects();
  Q_INVOKABLE void setSelectedObjectsPrintable(bool printable);
  Q_INVOKABLE void deleteSelection();
  /// 复制选中对象（对齐上游 Plater::clone_selection）
  Q_INVOKABLE void duplicateSelectedObjects();
  /// 剪贴板操作（对齐上游 Selection::copy_to_clipboard / paste_objects_from_clipboard）
  Q_INVOKABLE void copySelectedObjects();
  Q_INVOKABLE void pasteObjects();
  Q_INVOKABLE void cutSelectedObjects();
  /// 剪贴板是否有内容
  Q_PROPERTY(bool hasClipboardContent READ hasClipboardContent NOTIFY stateChanged)
  bool hasClipboardContent() const;
  /// 切换选中对象可见性（对齐上游 Plater::set_selected_visible）
  Q_INVOKABLE void toggleSelectedObjectsVisibility();
  /// 镜像选中对象（对齐上游 ModelInstance::set_mirror，同步真实模型）
  Q_INVOKABLE void mirrorSelectedObjects(int axis);
  /// 自动摆放（对齐上游 Plater::priv::on_arrange）— 委托给 GLViewport::arrangeSelected()
  /// 自动朝向（对齐上游 Plater::orient() / AutoOrienter）

  Q_INVOKABLE void autoOrientSelected();
  /// 拆分选中对象为独立对象（对齐上游 Plater::priv::split_object）
  Q_INVOKABLE void splitSelectedObject();
  /// 修复选中对象网格（对齐上游 MeshRepairDialog / fix_mesh）
  Q_INVOKABLE bool fixMeshSelected();
  /// 简化选中对象网格（对齐上游 GLGizmoSimplify — stub, needs simplify dialog）
  Q_INVOKABLE bool simplifyMeshSelected();
  // Phase 141 / DEBT-02: meshBooleanSelected() removed — was a no-op stub called only
  // by the orphaned "网格布尔运算" CxMenuItem. The real boolean path is the boolean
  // dialog → booleanExecute → ProjectServiceMock::meshBoolean.
  /// 按显式索引修改 volume 类型（对齐上游 GUI_ObjectList::load_generic_subobject）
  Q_INVOKABLE bool changeVolumeTypeByIndex(int objIdx, int volIdx, int newType);
  /// 重新从磁盘加载选中对象源文件（对齐上游 Plater::reload_from_disk）
  Q_INVOKABLE bool reloadSelectedFromDisk();
  /// 用 STL 文件替换选中 volume 的网格（对齐上游 GUI_ObjectList::load_subobject）
  Q_INVOKABLE bool replaceWithStl(const QString &path);
  /// 重新加载当前平板所有对象（对齐上游 Plater::reload_all_from_disk）
  Q_INVOKABLE bool reloadAllOnPlate(int plateIndex);
  /// 合并选中对象为单一多部件对象（对齐上游 GUI_ObjectList::assemble）
  Q_INVOKABLE bool assembleSelectedObjects();
  /// P16.2: merge all parts of the selected object into one mesh object
  /// (对齐上游 ObjectList::merge(false), GUI_ObjectList.cpp:2792-2810).
  Q_INVOKABLE bool mergeSelectedPartsToSingleObject();
  /// P16.4: Change Filament submenu (upstream MenuFactory::
  /// append_menu_item_change_filament GUI_Factories.cpp:1879-1961 +
  /// ObjectList::set_extruder_for_selected_items GUI_ObjectList.cpp:5622).
  /// extruder 0 = "Default"; 1..n = Filament n.
  Q_INVOKABLE int configFilamentCount() const;
  Q_INVOKABLE int objectExtruderId(int objectIndex) const;
  Q_INVOKABLE void setExtruderForSelectedItems(int extruder);
  /// P16.9: Flush Options submenu (upstream append_menu_items_flush_options,
  /// GUI_Factories.cpp:937-1028). optionIndex: 0=infill, 1=object, 2=support.
  Q_INVOKABLE bool flushOptionValue(int optionIndex) const;
  Q_INVOKABLE bool toggleFlushOption(int optionIndex);
  /// P16.9: Invalidate cut info (upstream has_selected_cut_object /
  /// invalidate_cut_info_for_selection, GUI_ObjectList.cpp:3033-3076).
  Q_INVOKABLE bool anySelectedObjectHasCutInfo() const;
  Q_INVOKABLE bool invalidateSelectedCutInfo();
  /// P16.11: per-instance printable (upstream itInstance toggle,
  /// ObjectList::toggle_printable_state GUI_ObjectList.cpp:5817-5866).
  Q_INVOKABLE bool instancePrintable(int objectIndex, int instanceIndex) const;
  Q_INVOKABLE void toggleInstancePrintable(int objectIndex, int instanceIndex);
  /// 将指定实例复制为独立对象（对齐上游 GUI_ObjectList::instance_to_object）
  Q_INVOKABLE bool instanceToObject(int instIdx);
  Q_INVOKABLE bool setSelectedInstanceCount(int count);
  Q_INVOKABLE bool addSelectedInstance();
  Q_INVOKABLE bool removeSelectedInstance();
  Q_INVOKABLE bool fillBedWithInstances();
  /// v5.11 gap-closure: rectangular array of the selected object (对齐上游
  /// instance-array). rows*cols instances on a grid with the given spacing.
  Q_INVOKABLE bool arraySelectedObject(int rows, int cols, float spacingX, float spacingY);
  Q_INVOKABLE bool splitSelectedToObjects();
  Q_INVOKABLE bool splitSelectedToParts();
  Q_INVOKABLE bool exportSelectedObjects(const QString &outputPath,
                                         bool separateFiles = false,
                                         bool drcFormat = false);
  Q_INVOKABLE bool addFilesToContextPlate(const QStringList &filePaths);
  /// Phase 237 (VIEW-03): synchronous batch import into the CURRENT plate
  /// (upstream PlaterDropTarget::OnDropFiles -> Plater::load_files,
  /// Plater.cpp:2738-2767). Thin sibling of addFilesToContextPlate that
  /// targets currentPlateIndex instead of the context-menu plate.
  Q_INVOKABLE bool addFilesToCurrentPlate(const QStringList &filePaths);
  Q_INVOKABLE bool addHandyModelToContextPlate(const QString &modelId);
  Q_INVOKABLE bool replaceAllOnContextPlate(const QStringList &filePaths);
  Q_INVOKABLE bool dropSelectedObjectsToBed();
  Q_INVOKABLE bool subdivideSelectedMesh();
  Q_INVOKABLE bool convertSelectedObjectUnits(int conversionType);
  Q_INVOKABLE bool copyContextProcessSettings();
  Q_INVOKABLE bool pasteContextProcessSettings();
  Q_INVOKABLE bool hasContextProcessSettingsClipboard() const;
  Q_INVOKABLE bool addPrimitiveToContextPlate(int type);
  Q_INVOKABLE bool pasteToContextPlate();
  Q_INVOKABLE bool autoOrientContextPlate();
  /// 获取选中 volume 的类型枚举（对齐上游 ModelVolumeType）
  Q_INVOKABLE int getSelectedVolumeType() const;
  /// 添加原始几何体到当前平板（对齐上游 create_mesh + add_volume）
  Q_INVOKABLE bool addPrimitiveToPlate(int type);
  /// 删除指定耗材槽位（对齐上游 filament slot management）
  Q_INVOKABLE bool deleteFilamentSlot(int index);
  /// 合并两个耗材槽位的预设（对齐上游 filament slot merge）
  Q_INVOKABLE bool mergeFilamentSlots(int from, int to);
  Q_INVOKABLE void fixMeshForObject(int i);
  Q_INVOKABLE void exportObjectAsStl(int i);
  /// 扁平平放（对齐上游 GLGizmoFlatten）— 将选中对象最大面朝下
  Q_INVOKABLE void flattenSelected();
  /// Phase 240 (GIZ-03): flatten face pick. Runs the stage-2 SceneRaycaster
  /// pick over the stage-1 survivor, then rotates the object so the picked
  /// facet's world normal faces DOWN (upstream GLGizmoFlatten::on_mouse
  /// LeftDown -> Selection::flattening_rotate, GLGizmoFlatten.cpp:22-38 +
  /// Selection.cpp:1293-1310). Offset is preserved (upstream keeps the
  /// offset matrix). Pushed as one TransformCommand (undoable, mirrors the
  /// upstream do_rotate "Gizmo-Place on Face" snapshot). Returns true when a
  /// facet was hit and the rotation applied.
  Q_INVOKABLE bool flattenPickFace(int pickedSourceIndex,
                                   QVector3D worldOrigin, QVector3D worldDirection);
  /// Phase 240 (GIZ-03): flatten hover pick. Same stage-2 pick, but instead
  /// of rotating it refreshes the hovered-facet highlight stream
  /// (flattenHoverData) that the renderer paints (upstream m_hover_id plane
  /// recolor, GLGizmoFlatten.cpp:107). No-op highlight clear on miss.
  Q_INVOKABLE void flattenHoverPick(int pickedSourceIndex,
                                    QVector3D worldOrigin, QVector3D worldDirection);
  /// Phase 240 (GIZ-03): hovered-facet highlight byte stream for the RHI
  /// overlay renderer. Wire format mirrors hollowMarkerData: header
  /// [int32 vertexCount] then vertexCount * [float x,y,z] world-space
  /// triangle soup (9 floats per highlighted facet). Empty when no hover.
  /// NOTIFY stateChanged (refreshed by flattenHoverPick).
  QByteArray flattenHoverData() const;
  /// Phase 240 (GIZ-03): pure math helper for the flatten rotation (unit
  /// testable headless). Given the object's current Euler XYZ rotation
  /// (degrees) and a WORLD-space facet normal, returns the new Euler XYZ
  /// rotation (degrees) that rotates the object so the normal faces down
  /// (-Z). Upstream math: tnormal = world normal (Selection.cpp:1299-1301);
  /// R_new = Quaterniond().setFromTwoVectors(tnormal, -UnitZ) * R_old
  /// (Selection.cpp:1303-1305). Implemented with QMatrix4x4/QQuaternion so
  /// it compiles without HAS_LIBSLIC3R.
  static QVector3D flattenRotationForNormal(const QVector3D &oldRotationDeg,
                                            const QVector3D &worldNormal);
  /// 切割选中对象（对齐上游 GLGizmoCut）— 沿指定轴/位置切割
  Q_INVOKABLE void cutSelected(int axis, double position);
  /// Phase 240 (GIZ-04): cut execution carrying the interactive plane
  /// rotation (upstream GLGizmoCut3D m_rotation -> get_cut_matrix,
  /// GLGizmoCut.cpp:3234-3253). cutRotationX/Y/Z tilt the cut plane normal
  /// around the world X/Y axes (degrees); 0/0/0 keeps the legacy axis-
  /// aligned cut byte-for-byte.
  Q_INVOKABLE bool cutSelectedWithRotation();
  /// Phase 240 (GIZ-04): interactive cut-plane tilt (upstream GLGizmoCut3D
  /// rotation grabbers, GLGizmoCut.cpp:1635-1644). Euler XYZ degrees applied
  /// on top of the base axis alignment when the plane is rendered + cut.
  Q_PROPERTY(float cutRotationX READ cutRotationX WRITE setCutRotationX NOTIFY stateChanged)
  Q_PROPERTY(float cutRotationY READ cutRotationY WRITE setCutRotationY NOTIFY stateChanged)
  Q_PROPERTY(float cutRotationZ READ cutRotationZ WRITE setCutRotationZ NOTIFY stateChanged)
  float cutRotationX() const { return m_cutRotationX; }
  float cutRotationY() const { return m_cutRotationY; }
  float cutRotationZ() const { return m_cutRotationZ; }
  void setCutRotationX(float v) { if (!qFuzzyCompare(m_cutRotationX, v)) { m_cutRotationX = v; emit stateChanged(); } }
  void setCutRotationY(float v) { if (!qFuzzyCompare(m_cutRotationY, v)) { m_cutRotationY = v; emit stateChanged(); } }
  void setCutRotationZ(float v) { if (!qFuzzyCompare(m_cutRotationZ, v)) { m_cutRotationZ = v; emit stateChanged(); } }
  /// Phase 240 (GIZ-04): same as cutRotationX/Y/Z but for the AdvancedCut
  /// gizmo's plane (advCutPosition's sibling state).
  Q_PROPERTY(float advCutRotationX READ advCutRotationX WRITE setAdvCutRotationX NOTIFY stateChanged)
  Q_PROPERTY(float advCutRotationY READ advCutRotationY WRITE setAdvCutRotationY NOTIFY stateChanged)
  Q_PROPERTY(float advCutRotationZ READ advCutRotationZ WRITE setAdvCutRotationZ NOTIFY stateChanged)
  float advCutRotationX() const { return m_advCutRotationX; }
  float advCutRotationY() const { return m_advCutRotationY; }
  float advCutRotationZ() const { return m_advCutRotationZ; }
  void setAdvCutRotationX(float v) { if (!qFuzzyCompare(m_advCutRotationX, v)) { m_advCutRotationX = v; emit stateChanged(); } }
  void setAdvCutRotationY(float v) { if (!qFuzzyCompare(m_advCutRotationY, v)) { m_advCutRotationY = v; emit stateChanged(); } }
  void setAdvCutRotationZ(float v) { if (!qFuzzyCompare(m_advCutRotationZ, v)) { m_advCutRotationZ = v; emit stateChanged(); } }
  /// 切割平面属性（对齐上游 GLGizmoCut 控制面板）
  Q_PROPERTY(int cutAxis READ cutAxis WRITE setCutAxis NOTIFY stateChanged)
  Q_PROPERTY(qreal cutPosition READ cutPosition WRITE setCutPosition NOTIFY stateChanged)
  Q_PROPERTY(int cutKeepMode READ cutKeepMode WRITE setCutKeepMode NOTIFY stateChanged)
  /// 切割连接器设置（对齐上游 GLGizmoCut connector type/shape/size）
  Q_PROPERTY(int cutMode READ cutMode WRITE setCutMode NOTIFY stateChanged)
  Q_PROPERTY(int connectorType READ connectorType WRITE setConnectorType NOTIFY stateChanged)
  Q_PROPERTY(int connectorStyle READ connectorStyle WRITE setConnectorStyle NOTIFY stateChanged)
  Q_PROPERTY(int connectorShape READ connectorShape WRITE setConnectorShape NOTIFY stateChanged)
  Q_PROPERTY(float connectorSize READ connectorSize WRITE setConnectorSize NOTIFY stateChanged)
  /// v5.12 gap-closure: independent groove params (override connectorSize defaults)
  Q_PROPERTY(float grooveDepth READ grooveDepth WRITE setGrooveDepth NOTIFY stateChanged)
  Q_PROPERTY(float grooveWidth READ grooveWidth WRITE setGrooveWidth NOTIFY stateChanged)
  Q_PROPERTY(float grooveFlapsAngle READ grooveFlapsAngle WRITE setGrooveFlapsAngle NOTIFY stateChanged)
  Q_PROPERTY(float connectorDepth READ connectorDepth WRITE setConnectorDepth NOTIFY stateChanged)
  /// 测量拾取模式（对齐上游 GLGizmoMeasure feature/point selection）
  Q_PROPERTY(int measureSelectionMode READ measureSelectionMode WRITE setMeasureSelectionMode NOTIFY stateChanged)
  /// 支撑绘制设置（对齐上游 GLGizmoFdmSupports）
  // Phase 122/123 (PAINT-04/05): which paint kind is active so paintAtFacet
  // can route the write-back to the correct ModelVolume FacetsAnnotation member
  // (0=Support->supported_facets, 1=Seam->seam_facets, 2=Mmu->mmu_segmentation_facets).
  // QML sets this when switching gizmo (GizmoSupportPaint=6->0, GizmoSeamPaint=7->1, GizmoMmuSegmentation=10->2).
  Q_PROPERTY(int activePaintKind READ activePaintKind WRITE setActivePaintKind NOTIFY stateChanged)
  Q_PROPERTY(int supportPaintTool READ supportPaintTool WRITE setSupportPaintTool NOTIFY stateChanged)
  Q_PROPERTY(int supportPaintCursorType READ supportPaintCursorType WRITE setSupportPaintCursorType NOTIFY stateChanged)
  Q_PROPERTY(int supportPaintToolType READ supportPaintToolType WRITE setSupportPaintToolType NOTIFY stateChanged)
  Q_PROPERTY(float supportPaintCursorRadius READ supportPaintCursorRadius WRITE setSupportPaintCursorRadius NOTIFY stateChanged)
  Q_PROPERTY(float supportPaintAngleThreshold READ supportPaintAngleThreshold WRITE setSupportPaintAngleThreshold NOTIFY stateChanged)
  Q_PROPERTY(float supportPaintSmartFillAngle READ supportPaintSmartFillAngle WRITE setSupportPaintSmartFillAngle NOTIFY stateChanged)
  Q_PROPERTY(float supportPaintGapArea READ supportPaintGapArea WRITE setSupportPaintGapArea NOTIFY stateChanged)
  Q_PROPERTY(bool supportPaintOnOverhangsOnly READ supportPaintOnOverhangsOnly WRITE setSupportPaintOnOverhangsOnly NOTIFY stateChanged)
  /// Phase 240 (GIZ-02): overhang filter angle (see supportPaintOverhangAngle).
  Q_PROPERTY(float supportPaintOverhangAngle READ supportPaintOverhangAngle WRITE setSupportPaintOverhangAngle NOTIFY stateChanged)
  Q_PROPERTY(bool supportEnable READ supportEnable WRITE setSupportEnable NOTIFY stateChanged)
  Q_PROPERTY(int supportType READ supportType WRITE setSupportType NOTIFY stateChanged)
  /// 支撑绘制数据（对齐上游 GLGizmoFdmSupports paint state）
  Q_PROPERTY(int enforcedSupportCount READ enforcedSupportCount NOTIFY paintDataChanged)
  Q_PROPERTY(int blockedSupportCount READ blockedSupportCount NOTIFY paintDataChanged)
  Q_PROPERTY(int totalPaintedTriangleCount READ totalPaintedTriangleCount NOTIFY paintDataChanged)
  // Phase 121 (PAINT-02): reverse data channel for the painted-facet overlay.
  // paintOverlayData flattens the selected object's painted facets (Enforcer +
  // Blocker; MMU: Extruder1..N) into a byte stream the RHI renderer + Software
  // mirror consume. Wire format: a header (int32 paintGizmoMode, int32 triangle
  // count) followed by triangleCount records of [int32 state, float vx,vy,vz x3]
  // (3 verts per triangle, already world-transformed via rebuildWorldTransform).
  // Mirrors the meshData Q_PROPERTY pattern (QByteArray over QML). The getter
  // drives PaintEngine::getFacets per (object, volume) for the active gizmo's
  // state set so the renderer reuses the mesh pipeline (GizmoVertex fill).
  Q_PROPERTY(QByteArray paintOverlayData READ paintOverlayData NOTIFY paintDataChanged)
  // Phase 121 (PAINT-02/OV-04): MMU per-extruder filament colors so the overlay
  // renderer can map ExtruderN -> a real color (mirrors PreviewViewModel::
  // extruderColor, upstream 8-color cycle). Exposed as a hex-string list so QML
  // + the renderer both decode with QColor(hex). NOTIFY paintDataChanged keeps
  // it in lockstep with the overlay stream.
  Q_PROPERTY(QVariantList extrudersColors READ extrudersColors NOTIFY paintDataChanged)
  /// 缝线绘制设置（对齐上游 GLGizmoSeam）
  Q_PROPERTY(int seamPaintTool READ seamPaintTool WRITE setSeamPaintTool NOTIFY stateChanged)
  Q_PROPERTY(float seamPaintCursorRadius READ seamPaintCursorRadius WRITE setSeamPaintCursorRadius NOTIFY stateChanged)
  Q_PROPERTY(bool seamPaintOnOverhangsOnly READ seamPaintOnOverhangsOnly WRITE setSeamPaintOnOverhangsOnly NOTIFY stateChanged)
  /// SLA 空洞标记设置（对齐上游 GLGizmoHollow）
  Q_PROPERTY(bool hollowEnabled READ hollowEnabled WRITE setHollowEnabled NOTIFY stateChanged)
  Q_PROPERTY(float hollowHoleRadius READ hollowHoleRadius WRITE setHollowHoleRadius NOTIFY stateChanged)
  Q_PROPERTY(float hollowHoleHeight READ hollowHoleHeight WRITE setHollowHoleHeight NOTIFY stateChanged)
  Q_PROPERTY(float hollowOffset READ hollowOffset WRITE setHollowOffset NOTIFY stateChanged)
  Q_PROPERTY(float hollowQuality READ hollowQuality WRITE setHollowQuality NOTIFY stateChanged)
  Q_PROPERTY(float hollowClosingDistance READ hollowClosingDistance WRITE setHollowClosingDistance NOTIFY stateChanged)
  Q_PROPERTY(int hollowHoleCount READ hollowHoleCount NOTIFY stateChanged)
  /// Drain-hole marker geometry for the renderer (对齐上游 GLGizmoHollow point
  /// rendering). A packed vertex byte stream (world-space small-disc per hole)
  /// consumed by RhiViewportRenderer. Refreshed whenever holes change.
  Q_PROPERTY(QByteArray hollowMarkerData READ hollowMarkerData NOTIFY hollowDataChanged)
  /// 网格简化设置（对齐上游 GLGizmoSimplify）
  Q_PROPERTY(int simplifyWantedCount READ simplifyWantedCount WRITE setSimplifyWantedCount NOTIFY stateChanged)
  Q_PROPERTY(float simplifyMaxError READ simplifyMaxError WRITE setSimplifyMaxError NOTIFY stateChanged)
  /// MMU 多耗材分段设置（对齐上游 GLGizmoMmuSegmentation）
  Q_PROPERTY(int mmuSelectedExtruder READ mmuSelectedExtruder WRITE setMmuSelectedExtruder NOTIFY stateChanged)
  Q_PROPERTY(int mmuExtruderCount READ mmuExtruderCount NOTIFY stateChanged)
  /// Configured filament colours for the MMU swatches (对齐上游
  /// m_extruders_colors). Bound from ProjectServiceMock::plateFilamentColours
  /// instead of hard-coded Theme constants.
  Q_PROPERTY(QStringList mmuExtruderColors READ mmuExtruderColors NOTIFY stateChanged)
  /// Drill 设置（对齐上游 GLGizmoDrill）
  Q_PROPERTY(float drillRadius READ drillRadius WRITE setDrillRadius NOTIFY stateChanged)
  Q_PROPERTY(float drillDepth READ drillDepth WRITE setDrillDepth NOTIFY stateChanged)
  Q_PROPERTY(int drillShape READ drillShape WRITE setDrillShape NOTIFY stateChanged)
  Q_PROPERTY(int drillDirection READ drillDirection WRITE setDrillDirection NOTIFY stateChanged)
  Q_PROPERTY(bool drillOneLayerOnly READ drillOneLayerOnly WRITE setDrillOneLayerOnly NOTIFY stateChanged)
  /// Emboss 设置（对齐上游 GLGizmoEmboss）
  Q_PROPERTY(QString embossText READ embossText WRITE setEmbossText NOTIFY stateChanged)
  Q_PROPERTY(float embossHeight READ embossHeight WRITE setEmbossHeight NOTIFY stateChanged)
  Q_PROPERTY(float embossDepth READ embossDepth WRITE setEmbossDepth NOTIFY stateChanged)
  /// Phase 144 (EMB-01): user-selected font path (empty = default system font).
  Q_PROPERTY(QString embossFontPath READ embossFontPath WRITE setEmbossFontPath NOTIFY stateChanged)
  /// Phase 158 (EMBO-F01): style axes. boldness + italic map to upstream
  /// FontProp fields and reach text2shapes; use-surface + curve-projection are
  /// projection concepts persisted into TextConfiguration for round-trip
  /// (geometry deformation deferred — upstream Emboss.hpp has no ProjectCurve).
  Q_PROPERTY(float embossBoldness READ embossBoldness WRITE setEmbossBoldness NOTIFY stateChanged)
  Q_PROPERTY(bool embossItalic READ embossItalic WRITE setEmbossItalic NOTIFY stateChanged)
  Q_PROPERTY(bool embossUseSurface READ embossUseSurface WRITE setEmbossUseSurface NOTIFY stateChanged)
  Q_PROPERTY(bool embossCurveProjection READ embossCurveProjection WRITE setEmbossCurveProjection NOTIFY stateChanged)
  /// Phase 196 (FEAT-01): true while an async emboss job is in flight.
  /// QML binds this to a spinner (CxBusyIndicator) so the user sees feedback
  /// during async text generation. Set true on embossSelectedAsync() start,
  /// false on embossVolumeAdded/embossVolumeFailed.
  Q_PROPERTY(bool embossRunning READ embossRunning NOTIFY embossRunningChanged)
  /// Phase 158 (EMBO-F02): SVG depth-modifier (Z-scale on the imported mesh;
  /// 1.0 = no change). Forwarded via importSVG() → addSvgVolume(idx, path, dz).
  Q_PROPERTY(float svgDepthModifier READ svgDepthModifier WRITE setSvgDepthModifier NOTIFY stateChanged)
  /// MeshBoolean 设置（对齐上游 GLGizmoMeshBoolean）
  Q_PROPERTY(int booleanOperation READ booleanOperation WRITE setBooleanOperation NOTIFY stateChanged)
  /// AdvancedCut 设置（对齐上游 GLGizmoAdvancedCut）
  Q_PROPERTY(int advCutAxis READ advCutAxis WRITE setAdvCutAxis NOTIFY stateChanged)
  Q_PROPERTY(float advCutPosition READ advCutPosition WRITE setAdvCutPosition NOTIFY stateChanged)
  Q_PROPERTY(bool advCutKeepBoth READ advCutKeepBoth WRITE setAdvCutKeepBoth NOTIFY stateChanged)
  Q_PROPERTY(bool advCutConnectors READ advCutConnectors WRITE setAdvCutConnectors NOTIFY stateChanged)
  /// v5.13: placed connector pin count on the selected object.
  Q_PROPERTY(int advancedCutConnectorCount READ advancedCutConnectorCount NOTIFY stateChanged)
  /// v5.13: connector-pin marker geometry for the renderer (same packed
  /// [uint32 count][GizmoVertex...] format as hollowMarkerData; one
  /// axis-aligned cylinder per pin, world space).
  Q_PROPERTY(QByteArray advancedCutMarkerData READ advancedCutMarkerData NOTIFY advancedCutConnectorDataChanged)
  /// FaceDetector 设置（对齐上游 GLGizmoFaceDetector）
  Q_PROPERTY(float faceDetectorAngle READ faceDetectorAngle WRITE setFaceDetectorAngle NOTIFY stateChanged)
  /// v5.12 gap-closure: number of flat faces found by detectFlatFaces.
  Q_PROPERTY(int faceDetectorResultCount READ faceDetectorResultCount NOTIFY stateChanged)
  /// Text 设置（对齐上游 GLGizmoText）
  Q_PROPERTY(QString textContent READ textContent WRITE setTextContent NOTIFY stateChanged)
  Q_PROPERTY(float textSize READ textSize WRITE setTextSize NOTIFY stateChanged)
  /// SVG 设置（对齐上游 GLGizmoSVG）
  Q_PROPERTY(QString svgFilePath READ svgFilePath WRITE setSvgFilePath NOTIFY stateChanged)
  Q_PROPERTY(float svgScale READ svgScale WRITE setSvgScale NOTIFY stateChanged)
  /// 扁平可用面数（对齐上游 GLGizmoFlatten 面）
  Q_PROPERTY(int flattenFaceCount READ flattenFaceCount NOTIFY stateChanged)
  /// Phase 240 (GIZ-03): hovered-facet highlight stream for the renderer.
  Q_PROPERTY(QByteArray flattenHoverData READ flattenHoverData NOTIFY stateChanged)
  /// Phase 240 (GIZ-06): simplify preview state (upstream GLGizmoSimplify
  /// three-stage start/preview/apply).
  Q_PROPERTY(bool simplifyPreviewRunning READ simplifyPreviewRunning NOTIFY stateChanged)
  Q_PROPERTY(int simplifyPreviewTriangleCount READ simplifyPreviewTriangleCount NOTIFY stateChanged)
  Q_PROPERTY(bool simplifyPreviewValid READ simplifyPreviewValid NOTIFY stateChanged)
  /// Phase 240 (GIZ-06): true when the selected volume carries a
  /// TextConfiguration (emboss panel switches to in-place re-generate mode).
  Q_PROPERTY(bool embossEditingExistingVolume READ embossEditingExistingVolume NOTIFY stateChanged)
  /// Phase 240 (GIZ-05): in-scene measure overlay line stream + projected
  /// 2D annotation anchors.
  Q_PROPERTY(QByteArray measureOverlayData READ measureOverlayData NOTIFY measureReadoutChanged)
  Q_PROPERTY(QVariantList measureOverlayLabels READ measureOverlayLabels NOTIFY measureReadoutChanged)
  /// 翻转切割平面（对齐上游 GLGizmoCut::flip_cut_plane）
  Q_INVOKABLE void flipCutPlane();
  /// 居中切割位置到选中对象中心
  Q_INVOKABLE void centerCutPlane();
  Q_INVOKABLE void requestSelectionSettings();
  /// Phase 174 (FEAT-01): per-object/volume scoped option API proxies.
  /// These forward to ProjectServiceMock so QML (SelectionSettingsDialog) can
  /// read/write per-object print/filament parameter overrides without holding
  /// a direct service reference (QML boundary rule).
  Q_INVOKABLE QVariant scopedOptionValue(int objectIndex, int volumeIndex, const QString &key, const QVariant &fallback = QVariant()) const;
  Q_INVOKABLE bool setScopedOptionValue(int objectIndex, int volumeIndex, const QString &key, const QVariant &value);
  Q_INVOKABLE int scopedOverrideCount(int objectIndex, int volumeIndex) const;
  Q_INVOKABLE QString scopedOverriddenKey(int objectIndex, int volumeIndex, int index) const;
  Q_INVOKABLE bool resetScopedOptionValue(int objectIndex, int volumeIndex, const QString &key);
  /// Phase 175 (FEAT-02): per-object layer-range editor API proxies.
  /// Forward to ProjectServiceMock so the ObjectLayersDialog QML can manage
  /// per-layer-height ranges (add/remove/set-value/get-value).
  Q_INVOKABLE int objectLayerRangeCount(int objectIndex) const;
  Q_INVOKABLE double layerRangeMinZ(int objectIndex, int rangeIndex) const;
  Q_INVOKABLE double layerRangeMaxZ(int objectIndex, int rangeIndex) const;
  Q_INVOKABLE QVariant layerRangeValue(int objectIndex, int rangeIndex, const QString &key, const QVariant &fallback = QVariant()) const;
  Q_INVOKABLE bool addObjectLayerRange(int objectIndex, double minZ, double maxZ);
  Q_INVOKABLE bool removeObjectLayerRange(int objectIndex, int rangeIndex);
  Q_INVOKABLE bool setLayerRangeValue(int objectIndex, int rangeIndex, const QString &key, const QVariant &value);
  /// Phase 198 (PHASE198): request opening the per-object layer-range editor
  /// dialog for the currently selected object. Mirrors requestSelectionSettings
  /// -- Q_INVOKABLE emits a signal the page observes, rather than the list
  /// holding a dialog reference (对齐上游 GUI_ObjectList right-click "Layers").
  Q_INVOKABLE void requestObjectLayerRanges();
  Q_INVOKABLE QString plateName(int i) const;
  Q_INVOKABLE int plateObjectCount(int i) const;
  Q_INVOKABLE int objectPlateIndex(int i) const;
  Q_INVOKABLE QString objectPlateName(int i) const;
  Q_INVOKABLE bool setCurrentPlateIndex(int i);
  Q_INVOKABLE void setShowAllObjects(bool showAll);
  void setShowLabels(bool v);
  Q_INVOKABLE void setObjectOrganizeMode(int mode);
  /// 平板管理（对齐上游 PartPlateList）
  Q_INVOKABLE bool addPlate();
  Q_INVOKABLE bool canAddPlate() const;
  Q_INVOKABLE bool deletePlate(int plateIndex);
  Q_INVOKABLE bool canDeletePlate(int plateIndex) const;
  /// 平板级操作（对齐上游 create_plate_menu）
  Q_INVOKABLE void selectAllOnPlate(int plateIndex);
  Q_INVOKABLE void removeAllOnPlate(int plateIndex);
  Q_INVOKABLE bool renamePlate(int plateIndex, const QString &newName);
  Q_INVOKABLE bool isPlateLocked(int plateIndex) const;
  Q_INVOKABLE void togglePlateLocked(int plateIndex);
  // ── v3.0 Phase 17: plate lifecycle completion (PLATE-03/04/05) ──
  /// 克隆平板（深拷贝含 ModelObject，对齐上游 duplicate_plate）
  Q_INVOKABLE bool clonePlate(int sourceIndex);
  /// 重排平板（oldIndex → newIndex）
  Q_INVOKABLE bool movePlate(int oldIndex, int newIndex);
  Q_INVOKABLE bool setPlatePrintable(int plateIndex, bool printable);
  Q_INVOKABLE bool isPlatePrintable(int plateIndex) const;
  // Phase 110 (FMAP-03): mode-only write path for the FilamentGroupPopup.
  // Delegates to ProjectServiceMock::setPlateFilamentMapMode, which reuses the
  // existing plate-write plumbing and is guarded by the R-02 / FP-04 clamp at
  // PartPlate::setFilamentMapMode(int). Emits stateChanged so the preview
  // surface refreshes; the popup reads the Phase 108 auto* Q_PROPERTYs for the
  // auto-recommended preview.
  Q_INVOKABLE bool setPlateFilamentMapMode(int plateIndex, int mode);
  Q_INVOKABLE int plateFilamentMapMode(int plateIndex) const;
  /// 查询指定平板是否有有效切片结果
  Q_INVOKABLE bool isPlateSliced(int plateIndex) const;
  /// 移动选中对象到指定平板（对齐上游 Plater::priv::on_arrange 跨平板拖拽）
  Q_INVOKABLE bool canMoveSelectionToPlate(int targetPlateIndex) const;
  Q_INVOKABLE bool moveSelectedObjectToPlate(int targetPlateIndex);
  /// 平板缩略图颜色（Mock 模式：基于平板对象数生成独特颜色，对齐上游 PartPlate thumbnail）
  Q_INVOKABLE QString plateThumbnailColor(int plateIndex) const;
  // Phase 98 (THUMBVERIFY-01): persisted-thumbnail accessor for QML. Empty
  // when no thumbnail is cached (plate-card renders nothing, not a mock).
  Q_INVOKABLE QString plateThumbnailBase64(int plateIndex) const;
  /// Phase 156 (CLOS-03): runtime thumbnail WRITE proxy — forwards to
  /// ProjectServiceMock::setPlateThumbnailFromBase64. QML calls this from the
  /// per-plate capture handler so non-current plates get real thumbnails.
  Q_INVOKABLE bool setPlateThumbnailFromBase64(int plateIndex, const QString &base64);
  /// 重命名对象（对齐上游 Plater::rename_object）
  Q_INVOKABLE bool renameObject(int index, const QString &newName);
  /// 移动对象排序位置（对齐上游 GUI_ObjectList 拖拽重排序）
  Q_INVOKABLE bool moveObject(int fromIndex, int toIndex);
  /// 平板设置（对齐上游 PlateSettingsDialog）
  Q_INVOKABLE int plateBedType(int plateIndex) const;
  Q_INVOKABLE bool setPlateBedType(int plateIndex, int bedType);
  Q_INVOKABLE int platePrintSequence(int plateIndex) const;
  /// Resolve a plate's ByDefault sequence against the effective global print
  /// config; returns the UI sequence index (0=ByDefault, 1=ByLayer, 2=ByObject).
  Q_INVOKABLE int resolvedPlatePrintSequence(int plateIndex) const;
  Q_INVOKABLE bool setPlatePrintSequence(int plateIndex, int seq);
  Q_INVOKABLE int plateSpiralMode(int plateIndex) const;
  Q_INVOKABLE bool setPlateSpiralMode(int plateIndex, int mode);
  /// 首层耗材顺序（对齐上游 first_layer_print_sequence）
  Q_INVOKABLE int plateFirstLayerSeqChoice(int plateIndex) const;
  Q_INVOKABLE bool setPlateFirstLayerSeqChoice(int plateIndex, int choice);
  Q_INVOKABLE QVariantList plateFirstLayerSeqOrder(int plateIndex) const;
  Q_INVOKABLE bool setPlateFirstLayerSeqOrder(int plateIndex, const QVariantList &order);
  /// 其他层耗材顺序（对齐上游 other_layers_print_sequence）
  Q_INVOKABLE int plateOtherLayersSeqChoice(int plateIndex) const;
  Q_INVOKABLE bool setPlateOtherLayersSeqChoice(int plateIndex, int choice);
  Q_INVOKABLE int plateOtherLayersSeqCount(int plateIndex) const;
  Q_INVOKABLE int plateOtherLayersSeqBegin(int plateIndex, int entryIndex) const;
  Q_INVOKABLE int plateOtherLayersSeqEnd(int plateIndex, int entryIndex) const;
  Q_INVOKABLE QVariantList plateOtherLayersSeqOrder(int plateIndex, int entryIndex) const;
  Q_INVOKABLE bool addPlateOtherLayersSeqEntry(int plateIndex, int beginLayer, int endLayer);
  Q_INVOKABLE bool removePlateOtherLayersSeqEntry(int plateIndex, int entryIndex);
  Q_INVOKABLE bool setPlateOtherLayersSeqRange(int plateIndex, int entryIndex, int beginLayer, int endLayer);
  Q_INVOKABLE bool setPlateOtherLayersSeqOrder(int plateIndex, int entryIndex, const QVariantList &order);
  /// 当前平板使用的耗材数（Mock 模式）
  Q_INVOKABLE int plateExtruderCount(int plateIndex) const;
  /// 居中选中对象到热床（对齐上游 Plater::priv::on_center / ModelObject::center_instances）
  Q_INVOKABLE void centerSelectedObjects();
  /// 铺满热床副本（对齐上游 Plater::priv::on_fill_bed）
  Q_INVOKABLE void fillBedWithCopies();
  /// 导出选中对象为 STL（对齐上游 Plater::priv::on_export_stl）
  Q_INVOKABLE void exportSelectedAsStl();

  // Slice-progress bridge (delegates to SliceService)
  Q_PROPERTY(QString sliceStatusLabel READ sliceStatusLabel NOTIFY stateChanged)
  Q_PROPERTY(QString sliceEstimatedTime READ sliceEstimatedTime NOTIFY stateChanged)
  Q_PROPERTY(QString sliceOutputPath READ sliceOutputPath NOTIFY stateChanged)
  Q_PROPERTY(QString sliceResultWeight READ sliceResultWeight NOTIFY stateChanged)
  Q_PROPERTY(QString sliceResultPlateLabel READ sliceResultPlateLabel NOTIFY stateChanged)
  Q_PROPERTY(QString sliceResultFilament READ sliceResultFilament NOTIFY stateChanged)
  Q_PROPERTY(QString sliceResultCost READ sliceResultCost NOTIFY stateChanged)
  Q_PROPERTY(int sliceResultLayerCount READ sliceResultLayerCount NOTIFY stateChanged)
  /// 模型尺寸文本（对齐上游 SliceInfoPanel 模型信息）
  Q_PROPERTY(QString modelSizeText READ modelSizeText NOTIFY stateChanged)
  /// 平均打印速度（对齐上游 PrintEstimatedStatistics）
  Q_PROPERTY(QString avgPrintSpeed READ avgPrintSpeed NOTIFY stateChanged)
  /// 预估打印时间（对齐上游 PrintEstimatedStatistics）
  Q_PROPERTY(QString estimatedPrintTime READ estimatedPrintTime NOTIFY stateChanged)
  /// 视口告警类型（对齐上游 EWarning）
  Q_PROPERTY(int viewportWarning READ viewportWarning NOTIFY stateChanged)
  Q_PROPERTY(QString viewportWarningMessage READ viewportWarningMessage NOTIFY stateChanged)
  Q_PROPERTY(bool hasViewportWarning READ hasViewportWarning NOTIFY stateChanged)
  /// 排列设置（对齐上游 ArrangeSettings）
  Q_PROPERTY(float arrangeDistance READ arrangeDistance WRITE setArrangeDistance NOTIFY stateChanged)
  Q_PROPERTY(bool arrangeRotation READ arrangeRotation WRITE setArrangeRotation NOTIFY stateChanged)
  Q_PROPERTY(bool arrangeAlignY READ arrangeAlignY WRITE setArrangeAlignY NOTIFY stateChanged)
  Q_PROPERTY(bool arrangeMultiMaterial READ arrangeMultiMaterial WRITE setArrangeMultiMaterial NOTIFY stateChanged)
  Q_PROPERTY(bool arrangeAvoidCalibration READ arrangeAvoidCalibration WRITE setArrangeAvoidCalibration NOTIFY stateChanged)
  /// 全部平板已切片标记（对齐上游 SliceAll 完成状态）
  Q_PROPERTY(bool allPlatesSliced READ allPlatesSliced NOTIFY stateChanged)
  /// 挤出机耗材用量（对齐上游 SliceInfoPanel per-extruder filament breakdown）
  /// 热床尺寸（对齐上游 BedShapeDialog / bed_shape config）
  Q_PROPERTY(float bedWidth READ bedWidth WRITE setBedWidth NOTIFY bedShapeChanged)
  Q_PROPERTY(float bedDepth READ bedDepth WRITE setBedDepth NOTIFY bedShapeChanged)
  Q_PROPERTY(float bedMaxHeight READ bedMaxHeight WRITE setBedMaxHeight NOTIFY bedShapeChanged)
  Q_PROPERTY(float bedOriginX READ bedOriginX WRITE setBedOriginX NOTIFY bedShapeChanged)
  Q_PROPERTY(float bedOriginY READ bedOriginY WRITE setBedOriginY NOTIFY bedShapeChanged)
  Q_PROPERTY(int bedShapeType READ bedShapeType WRITE setBedShapeType NOTIFY bedShapeChanged)
  Q_PROPERTY(float bedDiameter READ bedDiameter WRITE setBedDiameter NOTIFY bedShapeChanged)
  /// v5.15 (BEDTEX): printer-profile bed texture image (upstream PartPlate
  /// m_logo_texture_filename). READ-only + NOTIFY: the path flows from the
  /// selected printer preset via ConfigViewModel/PresetServiceMock.
  Q_PROPERTY(QUrl bedTextureUrl READ bedTextureUrl NOTIFY bedShapeChanged)
  /// v5.16 (EXCLAREA): bed_exclude_area polygons parsed from the printer
  /// preset (upstream coPoints; Plater.cpp:8169 feeds them to PartPlate).
  /// Each entry is a flat [x1,y1,x2,y2,...] point list for one polygon.
  Q_PROPERTY(QVariantList bedExcludeAreas READ bedExcludeAreas NOTIFY bedShapeChanged)
  /// v5.16 (HTLIMIT): extruder_clearance_height_to_rod/_to_lid from the
  /// printer preset (Plater.cpp:8151-8152). The ByObject gate is applied via
  /// resolvedPlatePrintSequence(currentPlateIndex) == 2.
  Q_PROPERTY(float bedHeightToRod READ bedHeightToRod NOTIFY bedShapeChanged)
  Q_PROPERTY(float bedHeightToLid READ bedHeightToLid NOTIFY bedShapeChanged)
  /// v5.16 (BEDMODEL): the printer's bed_model STL packed as a triangle
  /// stream ([uint32 vertexCount][float x,y,z scene-space] * count).
  Q_PROPERTY(QByteArray bedModelMeshData READ bedModelMeshData NOTIFY bedShapeChanged)
  /// v5.16 (BEDTYPE-TEX): BBL-only bed-type texture system gates + assets.
  Q_PROPERTY(bool bedTypeTexturesActive READ bedTypeTexturesActive NOTIFY bedShapeChanged)
  Q_PROPERTY(bool bedCaliLinesActive READ bedCaliLinesActive NOTIFY bedShapeChanged)
  Q_PROPERTY(QString bedTypeImagesDir READ bedTypeImagesDir NOTIFY bedShapeChanged)
  /// v5.16: current plate's bed type (Slic3r BedType index) driving the
  /// bed-type texture parts.
  Q_PROPERTY(int currentPlateBedType READ currentPlateBedType NOTIFY stateChanged)
  /// Phase 100 (WTREAD-01): post-slice wipe-tower geometry read back from
  /// Print::wipe_tower_data() via SliceService. READ-only + NOTIFY because the
  /// dims flow from SliceService (libslic3r worker), not from QML. showWipeTower
  /// is the WTREAD-02 gate (has_wipe_tower()): false on single-material slices
  /// so no placeholder box leaks. Defaults match RhiViewport.h:304-309
  /// (show=false, 10/10/50/100/25) so the pre-slice renderer is unchanged.
  Q_PROPERTY(bool showWipeTower READ showWipeTower NOTIFY wipeTowerGeometryChanged)
  Q_PROPERTY(float wipeTowerWidth READ wipeTowerWidth NOTIFY wipeTowerGeometryChanged)
  Q_PROPERTY(float wipeTowerDepth READ wipeTowerDepth NOTIFY wipeTowerGeometryChanged)
  Q_PROPERTY(float wipeTowerHeight READ wipeTowerHeight NOTIFY wipeTowerGeometryChanged)
  Q_PROPERTY(float wipeTowerX READ wipeTowerX NOTIFY wipeTowerGeometryChanged)
  Q_PROPERTY(float wipeTowerZ READ wipeTowerZ NOTIFY wipeTowerGeometryChanged)
  /// Phase 109 (WTMESH-01/02): Option B real-mesh readback. hasRealMesh gates
  /// the renderer branch (true -> buildWipeTowerMeshVertices, false -> Option A
  /// buildWipeTowerVertices, Phase 99 Frozen Decision 2 baseline). meshVertices
  /// carries the flattened XYZ triples (libslic3r world frame) as a
  /// QVariantList so it crosses the QML boundary cleanly. Both are READ-only +
  /// NOTIFY (wipeTowerGeometryChanged) because they flow from SliceService
  /// (libslic3r worker), not from QML -- same one-way flow as the dim
  /// Q_PROPERTYs above. When hasRealMesh is false, meshVertices is empty and
  /// PreparePage.qml binds the renderer to the Option A fallback.
  Q_PROPERTY(bool wipeTowerHasRealMesh READ wipeTowerHasRealMesh NOTIFY wipeTowerGeometryChanged)
  Q_PROPERTY(QVariantList wipeTowerMeshVertices READ wipeTowerMeshVertices NOTIFY wipeTowerGeometryChanged)
  /// Phase 108 (FMAP-01): post-slice filament-map auto-recommendation read
  /// back from Print::get_filament_maps() via SliceService. READ-only + NOTIFY
  /// because the map flows from SliceService (libslic3r worker), not from QML
  /// -- one-way libslic3r -> GUI flow, mirroring the wipe-tower Q_PROPERTYs.
  /// hasAutoFilamentMap is the gate (mirrors the v4.4 WTREAD-02 showWipeTower
  /// gate): false until a valid auto recommendation arrives (the engine only
  /// computes an auto-map when mode < fmmManual). autoFilamentMapMode exposes
  /// the resolved OWzx::FilamentMapMode as an int for Phase 110 UI binding.
  /// autoFilamentMaps exposes the 1-based per-extruder group ids. Phase 110
  /// builds the FilamentGroupPopup UI on top of these bindings.
  Q_PROPERTY(bool hasAutoFilamentMap READ hasAutoFilamentMap NOTIFY filamentMapChanged)
  Q_PROPERTY(int autoFilamentMapMode READ autoFilamentMapMode NOTIFY filamentMapChanged)
  Q_PROPERTY(QVariantList autoFilamentMaps READ autoFilamentMaps NOTIFY filamentMapChanged)
  Q_PROPERTY(int extruderCount READ extruderCount NOTIFY stateChanged)
  Q_PROPERTY(bool hasSliceResult READ hasSliceResult NOTIFY stateChanged)
  /// Phase 52 PREPSB-05: staleness exposed to QML so Preview/Export can show a
  /// "stale -- reslice" indicator and disable export of stale results.
  Q_PROPERTY(QVariantList stalePlateIndices READ stalePlateIndices NOTIFY stateChanged)
  Q_PROPERTY(bool hasStaleSliceResults READ hasStaleSliceResults NOTIFY stateChanged)
  Q_PROPERTY(bool canRequestSlice READ canRequestSlice NOTIFY stateChanged)
  Q_PROPERTY(bool canPreview READ canPreview NOTIFY stateChanged)
  /// Phase 239 (ENGN-01): true while a switch-to-preview request is waiting
  /// for the auto-reslice (or the previous-G-code reuse parse) to finish. The
  /// page switch completes on sliceFinished (upstream Plater.cpp:6165-6234
  /// do_reslice keeps the user on the plater until the background process
  /// completes, then reload_print lands in preview).
  Q_PROPERTY(bool pendingPreviewAfterSlice READ pendingPreviewAfterSlice NOTIFY stateChanged)
  /// Phase 239 (ENGN-02): true when the current preview entry reused the
  /// plate's previous G-code instead of reslicing (upstream
  /// export_gcode_from_previous_file, BackgroundSlicingProcess.cpp:199-221).
  /// Cleared by the same invalidation set that marks slice results stale.
  Q_PROPERTY(bool previewReusedPreviousGcode READ previewReusedPreviousGcode NOTIFY stateChanged)
  Q_PROPERTY(bool canExportGCode READ canExportGCode NOTIFY stateChanged)
  Q_PROPERTY(QString sliceReadinessReason READ sliceReadinessReason NOTIFY stateChanged)
  Q_PROPERTY(QString previewActionHint READ previewActionHint NOTIFY stateChanged)
  Q_PROPERTY(QString exportActionHint READ exportActionHint NOTIFY stateChanged)
  Q_PROPERTY(QString sliceActionLabel READ sliceActionLabel NOTIFY stateChanged)
  Q_PROPERTY(QString sliceActionHint READ sliceActionHint NOTIFY stateChanged)
  Q_INVOKABLE int sliceProgress() const;
  Q_INVOKABLE bool isSlicing() const;
  /// Phase 196 (FEAT-01): returns the SliceService::State enum value so QML
  /// (SliceProgress.qml) can render Cancelled/Error-specific UI. Values map to
  /// SliceService::State (Idle=0, Slicing=1, Exporting=2, Completed=3,
  /// Cancelled=4, Error=5).
  Q_INVOKABLE int sliceState() const;
  QString sliceStatusLabel() const;
  QString sliceEstimatedTime() const;
  QString sliceOutputPath() const;
  QString sliceResultWeight() const;
  QString sliceResultPlateLabel() const;
  QString sliceResultFilament() const;
  QString sliceResultCost() const;
  int sliceResultLayerCount() const;
  QString modelSizeText() const;
  QString avgPrintSpeed() const;
  QString estimatedPrintTime() const;
  Q_INVOKABLE QString estimatePrintTimeForObject(int objectIndex) const;
  int viewportWarning() const;
  QString viewportWarningMessage() const;
  bool hasViewportWarning() const;
  // Arrange settings (对齐上游 ArrangeSettings)
  float arrangeDistance() const;
  void setArrangeDistance(float v);
  bool arrangeRotation() const;
  void setArrangeRotation(bool v);
  bool arrangeAlignY() const;
  void setArrangeAlignY(bool v);
  bool arrangeMultiMaterial() const;
  void setArrangeMultiMaterial(bool v);
  /// 自动排列全部对象（对齐上游 Plater::priv::on_arrange → arrange_objects）
  Q_INVOKABLE void arrangeAllObjects();
  Q_INVOKABLE bool arrangePlate(int plateIndex);
  bool arrangeAvoidCalibration() const;
  void setArrangeAvoidCalibration(bool v);
  /// 重置排列设置到默认值（对齐上游 ArrangeSettings Reset）
  Q_INVOKABLE void resetArrangeSettings();
  // Bed shape (对齐上游 BedShapeDialog)
  float bedWidth() const;
  void setBedWidth(float v);
  float bedDepth() const;
  void setBedDepth(float v);
  float bedMaxHeight() const;
  void setBedMaxHeight(float v);
  float bedOriginX() const;
  void setBedOriginX(float v);
  float bedOriginY() const;
  void setBedOriginY(float v);
  int bedShapeType() const;
  void setBedShapeType(int v);
  float bedDiameter() const;
  void setBedDiameter(float v);
  // v5.15 (BEDTEX): bed texture path from the selected printer preset.
  QUrl bedTextureUrl() const;
  // v5.16 (BEDMODEL/BEDTYPE-TEX): bed model mesh + bed-type texture state
  // (accessors for the Q_PROPERTYs above; definitions live with the
  // syncBedFromPrinterPreset body).
  QByteArray bedModelMeshData() const { return m_bedModelMeshData; }
  bool bedTypeTexturesActive() const { return m_bedTypeTexturesActive; }
  bool bedCaliLinesActive() const { return m_bedCaliLinesActive; }
  QString bedTypeImagesDir() const { return m_bedTypeImagesDir; }
  QVariantList bedExcludeAreas() const { return m_bedExcludeAreas; }
  float bedHeightToRod() const { return m_bedHeightToRod; }
  float bedHeightToLid() const { return m_bedHeightToLid; }
  /// v5.16: current plate's bed type (Slic3r BedType index).
  int currentPlateBedType() const;
  /// v5.15: pull printable_area + bed_texture from the selected printer
  /// preset into the viewport bed state (mirrors upstream set_bed_shape).
  void syncBedFromPrinterPreset();
  /// v5.16 (BEDMODEL): load the printer's bed_model STL into the triangle
  /// stream consumed by the viewport (defined in the .cpp).
  QByteArray loadBedModelMesh(const QString &stlPath) const;
  // Phase 100 (WTREAD-01): wipe-tower geometry getters (read-only; set from
  // the SliceService-received WipeTowerGeometry struct, not from QML).
  bool showWipeTower() const;
  float wipeTowerWidth() const;
  float wipeTowerDepth() const;
  float wipeTowerHeight() const;
  float wipeTowerX() const;
  float wipeTowerZ() const;
  // Phase 109 (WTMESH-01/02): Option B real-mesh getters (read-only; set from
  // the SliceService-received WipeTowerGeometry struct, not from QML).
  bool wipeTowerHasRealMesh() const;
  QVariantList wipeTowerMeshVertices() const;
  // Phase 108 (FMAP-01): filament-map auto-recommendation getters (read-only;
  // set from the SliceService-received FilamentMapResult struct, not from QML).
  bool hasAutoFilamentMap() const;
  int autoFilamentMapMode() const;
  QVariantList autoFilamentMaps() const;

  bool allPlatesSliced() const;
  int extruderCount() const;
  Q_INVOKABLE QString extruderUsedLength(int extruderId) const;
  Q_INVOKABLE QString extruderUsedWeight(int extruderId) const;
  bool hasSliceResult() const;
  bool pendingPreviewAfterSlice() const;
  bool previewReusedPreviousGcode() const;
  // Phase 52 PREPSB-05: staleness surface for Preview/Export gating
  QVariantList stalePlateIndices() const;
  bool hasStaleSliceResults() const;
  bool canRequestSlice() const;
  bool canPreview() const;
  bool canExportGCode() const;
  QString sliceReadinessReason() const;
  QString previewActionHint() const;
  QString exportActionHint() const;
  QString sliceActionLabel() const;
  QString sliceActionHint() const;
  Q_INVOKABLE int plateSliceResultStatus(int plateIndex) const;

  Q_INVOKABLE void requestSlice();
  Q_INVOKABLE void requestSliceAll();
  // v2.4 IO: 项目保存/导出转发（EditorVM 持有 projectService_）
  Q_INVOKABLE bool saveProjectAs(const QString &filePath) const;
  Q_INVOKABLE bool exportModel(const QString &filePath, const QString &format) const;
  Q_INVOKABLE void cancelSlice();
  Q_INVOKABLE QString defaultExportGCodeFileName(int plateIndex = -1) const;
  Q_INVOKABLE bool requestExportGCode(const QString &targetPath);
  Q_INVOKABLE bool requestExportAllGCode(const QString &directoryPath, const QString &baseName = QString());
  /// 请求切换到预览页面（对齐上游 Plater::priv::on_preview）
  Q_INVOKABLE void switchToPreview();
  Q_INVOKABLE bool canActivateGizmo(int gizmoMode) const;
  Q_INVOKABLE QString gizmoStatusText(int gizmoMode) const;
  /// 加载模型文件 (3MF/STL/OBJ)
  Q_INVOKABLE bool loadFile(const QString &filePath);
  /// Phase 237 (VIEW-04): apply a unit conversion to one object (upstream
  /// Plater.cpp:4237-4253 offers model.convert_from_meters(true) /
  /// convert_from_imperial_units(true) after the looks_likeSavedInMeters /
  /// looks_like_imperial_units checks). conversionType is the upstream
  /// Slic3r::ConversionType int (1 = CONV_FROM_INCH x25.4,
  /// 3 = CONV_FROM_METER x1000) forwarded to
  /// ProjectServiceMock::convertObjectUnits -> ModelObject::convert_units.
  Q_INVOKABLE bool applyUnitConversion(int objectIndex, int conversionType);
  /// Phase 237 (VIEW-04): unit-inference hint for one object (0 = none,
  /// 1 = meters, 2 = imperial); proxies
  /// ProjectServiceMock::loadedObjectUnitHint.
  Q_INVOKABLE int loadedObjectUnitHint(int objectIndex) const;
  /// Phase 237 (VIEW-06): uniform-scale the current selection so it fits the
  /// printable bed bbox, preserving proportions (port of upstream
  /// priv::scale_selection_to_fit_print_volume, Plater.cpp:5135 ->
  /// Selection::scale_to_fit_print_volume, Selection.cpp:1419-1508 rectangle
  /// branch). Returns the applied scale factor (0 = no-op / nothing
  /// selected).
  Q_INVOKABLE double scaleSelectionToFitBed();
  /// Phase 237 (VIEW-06): export the current plate's sliced G-code + plate
  /// 3MF block as *.gcode.3mf (upstream Plater::export_gcode_3mf,
  /// Plater.cpp:11499-11573). targetPath gets the .gcode.3mf suffix appended
  /// when missing (upstream appends .3mf the same way, Plater.cpp:11546
  /// -11547). Returns false when no valid slice result exists.
  Q_INVOKABLE bool requestExportGcode3mf(const QString &targetPath);
  // Phase 236 (DLG-03): RecenterDialog plumbing (upstream
  // Plater::priv::update_undo_redo_gui / RecenterDialog). Re-runs the
  // outside-bed detection against the real bed Q_PROPERTYs and returns the
  // number of offending objects; objectsOutsideBed refreshes via stateChanged.
  Q_INVOKABLE int checkObjectsOutsideBed();
  /// Phase 236 (DLG-03): move every outside-bed object back into the
  /// printable area (clamp to the bed rectangle, keep the current height).
  /// Returns the number of moved objects; invalidates slice results.
  Q_INVOKABLE int recenterObjectsOutsideBed();
  /// Phase 236 (DLG-03): list the importable model entries of a zip archive
  /// (FileArchiveDialog data source; proxies ProjectServiceMock).
  Q_INVOKABLE QStringList listArchiveEntries(const QString &archivePath) const;
  /// Phase 236 (DLG-03): extract the selected entries of a zip archive into a
  /// per-import temp directory and load each through the normal loadFile()
  /// path (FileArchiveDialog "import selected"). Returns the number of
  /// successfully started imports.
  Q_INVOKABLE int importArchiveEntries(const QString &archivePath, const QStringList &entries);
  /// Phase 236 (DLG-03): apply the user's ObjColorDialog mapping — sets every
  /// volume of the pending object to the chosen extruder (upstream
  /// ObjImportColorDialog on_confirm sets the object extruder per color).
  Q_INVOKABLE bool applyPendingObjColors(int extruderId);
  /// 清空当前场景与项目状态（用于顶部工具栏新建）
  Q_INVOKABLE void clearWorkspace();
  /// JSON 项目加载后刷新 UI 状态（供 BackendContext 调用）
  void refreshAfterLoad();
  /// AI/automation refresh hook (OWzx-only, docs/ai-control.md): rebuild the
  /// VM mirrors (object entries, mesh cache, slice-result staleness) after a
  /// scene mutation performed directly on ProjectServiceMock by an external
  /// actor (AI tools / scripts). Mirrors the refresh sequence the in-VM
  /// mutation flows run (e.g. duplicateSelectedObjects).
  void refreshAfterExternalSceneChange();
  /// Phase 52 PREPSB-05: invalidate slice/preview/export results for ALL
  /// plates (a preset/scope/option change affects every plate's result). Made
  /// public so the composition root (BackendContext) can call it on
  /// ConfigViewModel::stateChanged. Callers must re-emit stateChanged() so the
  /// stalePlateIndices / hasStaleSliceResults Q_PROPERTYs refresh in QML.
  void invalidateAllSliceResults();

  /// Undo/Redo integration (对齐上游 UndoRedo 框架)
  void setUndoRedoManager(UndoRedoManager *manager);
  /// Phase 90 (ASMROUTE-01): active canvas type injected by BackendContext on
  /// view-mode change. Mirrors upstream
  /// get_current_canvas3D()->get_canvas_type() (GLCanvas3D.hpp:509-513). The
  /// int value equals RhiViewport::CanvasType (0=View3D, 1=Preview,
  /// 2=AssembleView). Drives selection/gizmo/undo-redo routing in
  /// availableGizmoMask() and selectSourceObject() (see Plater.cpp:7322,11601,
  /// 11635). Phase 92 adds the Assembly gizmo to the mask.
  void setActiveCanvasType(int type);
  int activeCanvasType() const { return m_activeCanvasType; }
  /// Phase 91 (ASMEXPLODE-01): explosion ratio for AssembleView, mirroring
  /// upstream m_explosion_ratio (GLCanvas3D.hpp:596). setExplosionRatio drives
  /// the per-volume separation rendering via NOTIFY stateChanged.
  float explosionRatio() const { return m_explosionRatio; }
  void setExplosionRatio(float ratio);
  /// Phase 91 (ASMEXPLODE-01): reset explosion ratio to default 1.0, mirroring
  /// upstream reset_explosion_ratio()
  /// (third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.hpp:770-771).
  Q_INVOKABLE void resetExplosionRatio();
  // Phase 92 (ASMMEASURE-01): activate the Assembly measurement gizmo. Mirrors
  // upstream GLGizmoAssembly (Ctrl+Y, ONLY_ASSEMBLY mode,
  // third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoAssembly.cpp:45-51).
  // Returns true if the gizmo is activable (AssembleView + explosion ~ 1.0 +
  // >=2 volumes selected, GLGizmoAssembly.cpp:53-68) and was activated.
  Q_INVOKABLE bool activateAssemblyMeasureGizmo();
  Q_INVOKABLE void deactivateAssemblyMeasureGizmo();
  // Phase 92 (ASMMEASURE-01): Assembly measurement gizmo state accessors. The
  // text/vector accessors compute lazily from the first two selected volumes'
  // AABBs via AssemblyMeasureGeometry (center-to-center distance + longest-axis
  // angle — documented simplification). Empty/zero when <2 volumes selected.
  bool assemblyMeasureGizmoActive() const { return m_assemblyMeasureGizmoActive; }
  QString assemblyMeasureDistanceText() const;
  QString assemblyMeasureAngleText() const;
  QVector3D assemblyMeasureDistanceXyz() const;
  QString assemblyMeasurePlaneText() const;
  // Phase 114 (MEASURE-03): real feature-picking measurement readout
  // accessors (ME-04). The text variants format to upstream precision
  // (3 decimals + unit/glyph, GLGizmoMeasure.cpp:24 format_double). The
  // raw float/vector variants expose the underlying value for debug + the
  // Phase 115 overlay. measureReadoutValid is the gate.
  bool measureReadoutValid() const { return m_measureReadout.valid; }
  QString measureAngleText() const;
  QString measurePerpendicularDistanceText() const;
  QString measureDirectDistanceText() const;
  QString measureDistanceXyzText() const;
  float measureAngleDeg() const { return m_measureReadout.angleDeg; }
  float measurePerpendicularDistance() const
  { return m_measureReadout.perpendicularDistance; }
  float measureDirectDistance() const { return m_measureReadout.directDistance; }
  QVector3D measureDistanceXyz() const { return m_measureReadout.distanceXyz; }
  /// Phase 240 (GIZ-05): in-scene measure overlay geometry (world-space
  /// line stream for the RHI renderer, gated on GizmoMeasure). Wire format:
  /// header [int32 segmentCount] then segmentCount * 2 * [float x,y,z]
  /// segments. Segments: from-anchor -> current/hover anchor (the dimension
  /// line) + a short cross marker at each anchor (upstream renders 3D lines
  /// + dashed dimension line, GLGizmoMeasure.cpp render dimensioning).
  /// Empty when no valid measurement + no hover anchor.
  QByteArray measureOverlayData() const;
  /// Phase 240 (GIZ-05): 2D annotation anchors projected from the 3D
  /// measurement points. Each element is a QVariantMap {x,y,z, label} in
  /// world space; QML positions the label via RhiViewport::
  /// projectWorldToScreen (2D overlay projected from 3D anchor points is
  /// the accepted upstream delta -- the RHI renderer has no text pipeline).
  QVariantList measureOverlayLabels() const;
  /// Phase 114 (MEASURE-03): drive a measurement readout from a Phase 113
  /// SceneRaycaster hit + an optional second hit. Computes the picked
  /// feature(s) via MeasureEngine (Measure::Measuring on the per-volume ITS,
  /// pitfall-6 scrubbed) and the measurement readouts between them, caches
  /// the result into the measure* Q_PROPERTYs, and emits
  /// measureReadoutChanged(). Returns true if a valid readout was produced.
  ///
  /// The single-hit overload (secondHit=false) resolves the feature at the
  /// hit but produces no measurement (measureReadoutValid stays false until
  /// a second feature is supplied) -- the Phase 115 snap UX uses the first
  /// hit to set the "from" feature, the second to compute the readout.
  /// Mirrors the upstream two-click measure flow
  /// (GLGizmoMeasure.cpp m_selected_features first/second).
  ///
  /// All inputs are world-space. objectIndex/volumeIndex/facetIdx come from
  /// SceneRaycasterHit; worldPoint is the hit position; the volume's world
  /// transform is reconstructed on the C++ side from translation + euler
  /// rotation (radians, XYZ order) + scale (Eigen types cannot cross QML,
  /// so the renderer decomposes the candidate worldTransform it already
  /// holds). onlySelectPlane forces the plane feature
  /// (Measuring::get_feature last arg, Measure.hpp:128).
  Q_INVOKABLE bool computeMeasureReadoutFromHit(int objectIndex,
                                                int volumeIndex,
                                                int facetIdx,
                                                QVector3D worldPoint,
                                                QVector3D volumeTranslation,
                                                QVector3D volumeRotationRad,
                                                QVector3D volumeScale,
                                                bool onlySelectPlane);
  /// Phase 114 (MEASURE-03): clear the cached measurement readout (sends
  /// the measure* Q_PROPERTYs back to their invalid defaults). Call on
  /// cursor-leave / gizmo-deactivate so no stale readout lingers.
  Q_INVOKABLE void clearMeasureReadout();
  /// Phase 115 (MEASURE-04): drive the snap UX from a mouse event. Runs the
  /// two-stage pick (Phase 113 SceneRaycaster over the picked source object's
  /// volumes only -- pitfall 7 mitigation, NO whole-scene loop) and feeds the
  /// closest world-space hit through computeMeasureReadoutFromHit (Phase 114
  /// MeasureEngine::getFeature). The shiftHeld arg implements the upstream
  /// Shift toggle (GLGizmoMeasure.cpp:409-442): shiftHeld==true forces
  /// EMode::PointSelection -- the raw world hit point is used as a Point
  /// feature (no feature snapping). shiftHeld==false keeps the default
  /// FeatureSelection (snap to nearest edge/circle/plane). Returns true when
  /// a feature was resolved and the readout (or the "from" stash) updated.
  ///
  /// pickedSourceIndex is the stage-1 survivor from RhiViewport::pickSourceObjectAt
  /// (the cheap ray->AABB prefilter). Stage 2 narrows to that object's volumes.
  Q_INVOKABLE bool pickMeasureFeatureAt(QVector3D rayOrigin,
                                        QVector3D rayDirection,
                                        int pickedSourceIndex,
                                        bool shiftHeld);
  /// Phase 115 (MEASURE-04): the FeatureKind of the most recently picked
  /// feature (0=Undef, 1=Point, 2=Edge, 4=Circle, 8=Plane -- mirrors
  /// OWzx::FeatureKind / Measure::SurfaceFeatureType). Drives the gizmo
  /// overlay highlight type (point marker / edge line / circle / plane).
  /// 0 when no feature is currently hovered (ray missed / gizmo off).
  Q_PROPERTY(int measureHoverFeatureKind READ measureHoverFeatureKind NOTIFY measureReadoutChanged)
  int measureHoverFeatureKind() const { return m_measureHoverFeatureKind; }
  /// Phase 115 (MEASURE-04): the world-space position of the most recently
  /// picked feature (the snapped point for PointSelection, or the feature's
  /// representative point for FeatureSelection). Drives the gizmo overlay
  /// marker placement. Origin when no feature is hovered.
  Q_PROPERTY(QVector3D measureHoverWorldPosition READ measureHoverWorldPosition NOTIFY measureReadoutChanged)
  QVector3D measureHoverWorldPosition() const { return m_measureHoverWorldPosition; }
  /// Phase 114 (MEASURE-03): invalidate the per-volume Measure::Measuring
  /// cache. Wire to every mutation that changes a volume mesh (load, cut,
  /// boolean, simplify, drill) -- mirrors the upstream rebuild-on-mesh-
  /// change signal (GLGizmoMeasure.cpp m_curr_measuring rebuild, pitfall 6).
  /// Without this, a stale Measuring would serve features from the OLD mesh.
  Q_INVOKABLE void invalidateMeasureEngine();
  /// Rebuild the packed hollow-marker vertex byte stream from the selected
  /// object's drain holes (mesh-local → world). Emits hollowDataChanged via
  /// callers; this only mutates m_hollowMarkerData.
  void rebuildHollowMarkerData();
  /// v5.13: rebuild the connector-pin marker byte stream from the selected
  /// object's cut_connectors（同 hollow 标记打包格式）。
  void rebuildAdvancedCutConnectorMarkers();
  /// Phase 222 (FIL-COLOUR): sync the active filament colours from
  /// PresetServiceMock (via ConfigViewModel) into ProjectServiceMock so
  /// plateFilamentColours()/filamentCount() reflect the configured filaments.
  void syncFilamentColours();
  /// Phase 114 (MEASURE-03): test accessor exposing the cached Measuring
  /// count for the measureEngineInstantiatedPerVolume smoke assertion.
  /// Returns 0 when no Measuring is cached (or HAS_LIBSLIC3R is off).
  int measureEngineCachedCount() const;
  // Phase 92: first-two selected source indices, for the RhiViewport overlay
  // selection bindings (AssemblePage forwards them to the renderer).
  QList<int> assemblyMeasureSelectedSourceIndices() const;
  /// Config preset injection (对齐上游 PresetBundle::full_fff_config → BackgroundSlicingProcess)
  void setConfigViewModel(ConfigViewModel *vm);
  Q_INVOKABLE void undo();
  Q_INVOKABLE void redo();
  Q_INVOKABLE void clearUndoStack();
  bool canUndo() const;
  bool canRedo() const;
  /// Restore selection state from undo/redo commands (internal use)
  Q_INVOKABLE void restoreSelection(const QSet<int> &sourceIndices, int primaryIndex);
  /// Rebuild object list and notify (called by undo/redo commands after model changes)
  Q_INVOKABLE void rebuildAndNotify();
  UndoRedoManager *undoRedoManager() const { return m_undoManager; }

private slots:
  /// Phase 100 (WTREAD-01): receives the captured-by-value wipe-tower geometry
  /// delivered by SliceService::wipeTowerGeometryReady on the GUI thread.
  /// Applies the has_wipe_tower() gate (WTREAD-02): when geometry.valid is
  /// false, m_showWipeTower is forced to false and the dim members are left
  /// untouched (no placeholder pushed as "real" geometry). Always emits
  /// wipeTowerGeometryChanged() so the QML bindings refresh.
  void onWipeTowerGeometryReady(const WipeTowerGeometry &geometry);
  void onSequentialPrintClearanceReady(const SequentialPrintClearance &clearance);
  /// Phase 108 (FMAP-01): receives the captured-by-value filament-map auto-
  /// recommendation delivered by SliceService::filamentMapReady on the GUI
  /// thread. Applies the valid gate: when result.valid is false (user picked
  /// Manual, so the engine computed no auto-map), m_hasAutoFilamentMap is
  /// forced to false and the maps/mode members are left untouched (no stale
  /// map leaks to the Phase 110 UI). Always emits filamentMapChanged() so the
  /// QML bindings refresh.
  void onFilamentMapReady(const FilamentMapResult &result);

signals:
  void stateChanged();
  void paintDataChanged();
  void hollowDataChanged();
  void advancedCutConnectorDataChanged();
  void bedShapeChanged();
  void sequentialClearanceChanged();
  /// Phase 236 (DLG-03): an .obj with a multi-color .mtl finished loading —
  /// pendingObjColors holds the colors, the ObjColorDialog should open.
  void objColorMappingRequested(const QString &objectName);
  /// Phase 237 (VIEW-04): a freshly imported object tripped the upstream
  /// saved-unit heuristic (Plater.cpp:4237-4253) — the shell should offer the
  /// scale-to-millimeters confirm. unitHint: 1 = meters (x1000),
  /// 2 = imperial (x25.4).
  void unitConversionPromptRequested(int objectIndex, int unitHint, const QString &objectName);
  /// Phase 237 (VIEW-04): zero-volume objects were auto-removed after an
  /// import (upstream Model::removed_objects_with_zero_volume +
  /// Plater.cpp:4231-4233 info dialog; OWzx surfaces a notification).
  void zeroVolumeObjectsRemoved(int removedCount);
  /// Phase 145 (EMB-03): async emboss result delivery. Re-emitted from
  /// ProjectServiceMock's embossVolumeAdded/Failed so QML binds to a single
  /// EditorViewModel signal source.
  void embossVolumeAdded(int objectIndex, const QString &volumeName);
  void embossVolumeFailed(const QString &reason);
  /// Phase 196 (FEAT-01): emitted when m_embossRunning changes.
  void embossRunningChanged();
  /// Phase 100 (WTREAD-01): emitted whenever the wipe-tower geometry is
  /// refreshed from a SliceService readback (valid or invalid). Drives the
  /// six wipe-tower Q_PROPERTYs above.
  void wipeTowerGeometryChanged();
  /// Phase 108 (FMAP-01): emitted whenever the filament-map auto-recommendation
  /// is refreshed from a SliceService readback (valid or invalid). Drives the
  /// three auto* Q_PROPERTYs above.
  void filamentMapChanged();
  /// Phase 114 (MEASURE-03): emitted whenever the feature-picking
  /// measurement readout is refreshed (valid or cleared). Drives the nine
  /// measure* Q_PROPERTYs above.
  void measureReadoutChanged();
  void selectionSettingsRequested();
  /// Phase 198 (PHASE198): emitted by requestObjectLayerRanges() so the host
  /// page opens the per-object layer-range editor (对齐上游 GUI_ObjectLayers).
  void objectLayerRangeRequested();
  /// 请求切换到预览页面（对齐上游 Plater::priv::on_preview）
  void previewRequested();

private:
  void refreshMeshCacheAndFitHint();
  void invalidateSliceResultsForCurrentPlate();
  void invalidateSliceResultsForPlate(int plateIndex);
  // P15.11: drag-time sequential clearance preview (upstream
  // GLCanvas3D::update_sequential_clearance + reset_sequential_print_clearance).
  // begin is called by every beginGizmo{Move,Rotate,Scale}Drag (the
  // RhiViewport mouse-handler drag hooks emit gizmoDragBegin/gizmoDragEnd for
  // them); refresh restarts the 150 ms debounce on each applyGizmo*Delta;
  // end cancels stale computes and restores the validation payload. All three
  // gate on the resolved ByObject print sequence (GLCanvas3D.cpp:5193-5195).
  void beginDragSequentialClearance();
  void refreshDragSequentialClearance();
  void endDragSequentialClearance();
  /// P15.11: debounce-timer callback -- collects the CURRENT instance poses
  /// (value only) and runs SequentialClearanceCompute::runCompute on
  /// QtConcurrent with a generation counter; the finished handler drops stale
  /// results and reuses onSequentialPrintClearanceReady for delivery.
  void dispatchDragSequentialClearanceCompute();
  void rebuildObjectEntriesFromService();
  /// Phase 240 (GIZ-02): shared paint commit path. Mirrors the painted
  /// TriangleSelector into the Qt data layer (setTriangleSupportState),
  /// writes the FacetsAnnotation back to the ModelVolume (so the slice
  /// consumes the paint) and pushes a PaintCommand (undoable). Used by both
  /// paintAtFacet (brush) and smartFillAtFacet (seed fill).
  void commitPaintedSelector(int objectIndex, int volumeIndex, int facetIdx, int state);
  bool deleteSelectedVolumesBySource();
  void ensureValidObjectSelection(bool preferFirstVisible);
  void clearContextState();
  QList<int> baseVisibleObjectIndices() const;
  QString sourceObjectGroupLabel(int sourceIndex) const;
  QString sourceObjectGroupKey(int sourceIndex) const;
  int mapFilteredToSourceIndex(int filteredIndex) const;
  QList<int> visibleObjectIndices() const;
  bool currentPlateHasPrintableObjects() const;
  bool hasValidSliceResultForPlate(int plateIndex) const;
  void checkViewportWarnings();
  void continueSliceAllQueue();
  // Phase 92 (ASMMEASURE-01): Assembly measurement gizmo activability. Mirrors
  // upstream GLGizmoAssembly::on_is_activable()
  // (third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoAssembly.cpp:53-68):
  // active canvas is AssembleView (2) AND explosion ratio ~= 1.0 (within 1e-2,
  // matching upstream abs(ratio-1.0f) < 1e-2) AND >=2 volumes selected. Used by
  // availableGizmoMask() AssembleView branch and activateAssemblyMeasureGizmo().
  bool isAssemblyMeasureActivable() const;
  // Phase 92 (ASMMEASURE-02): parse the cached mesh blob to extract per-volume
  // AABBs for the first two selected source indices. Returns the bounds in the
  // same order as assemblyMeasureSelectedSourceIndices(). Empty when fewer than
  // two volumes are selected or the blob is malformed. The blob format mirrors
  // PrepareSceneData::setModelMeshData (objectCount header + per-batch
  // [renderObjectId, triangleCount, verts...]); bounds are derived from the
  // vertices, matching the renderer's batch.bounds derivation.
  QList<PrepareSceneData::ModelBounds> selectedVolumeBoundsForAssemblyMeasure() const;

  // Phase 93 (ASMROUTE-02): refresh the AssembleView data pool's
  // ModelObjectsInfo resource from the same bounds source Phase 92 used (the
  // cached mesh blob + m_cachedMeshBatchSourceObjectIndices). Called ONLY when
  // m_activeCanvasType == 2 (CanvasAssembleView), mirroring upstream
  // GLGizmosManager.cpp:427-431 running in the AssembleView-only gizmo
  // update. Prepare/Preview never call this (isolation constraint).
  void refreshAssembleViewDataPool();
  // Phase 93 (ASMROUTE-02): test accessor exposing the cached object count
  // for the AssembleView data pool. Returns 0 when the pool is not currently
  // valid (i.e. when not on AssembleView, or update() has released the
  // resource) — which is itself the isolation assertion consumed by the
  // assembleViewDataPoolIsolatedFromPrepareAndPreview smoke test.
  int assembleViewDataPoolObjectCountForTest() const;

  struct ObjectEntry
  {
    QString name;
    bool printable = true;
  };

  ProjectServiceMock *projectService_ = nullptr;
  SliceService *sliceService_ = nullptr;
  ConfigViewModel *configViewModel_ = nullptr;
  UndoRedoManager *m_undoManager = nullptr;
  // Phase 90 (ASMROUTE-01): active canvas type (0=View3D, 1=Preview,
  // 2=AssembleView). Default View3D. Set by BackendContext::setCurrentViewMode.
  int m_activeCanvasType = 0;
  // Phase 91 (ASMEXPLODE-01): explosion ratio mirroring upstream
  // m_explosion_ratio (GLCanvas3D.hpp:596, default 1.0). Drives the per-volume
  // separation offset in RhiViewportRenderer's CanvasAssembleView branch.
  float m_explosionRatio = 1.0f;
  // Phase 92 (ASMMEASURE-01): Assembly measurement gizmo active flag. Set by
  // activateAssemblyMeasureGizmo() only when isAssemblyMeasureActivable() is
  // true. Mirrors upstream GLGizmoBase::m_state activation.
  bool m_assemblyMeasureGizmoActive = false;
  // Phase 93 (ASMROUTE-02): AssembleView data pool — caches per-object info
  // for the AssembleView gizmos. Updated ONLY when m_activeCanvasType == 2
  // (CanvasAssembleView), mirroring upstream GLGizmosManager.cpp:427-431
  // where update_data() runs the pool update with the full bitmask on
  // AssembleView and with None otherwise (releasing all resources).
  // Prepare/Preview never populate or read it (isolation constraint).
  AssembleViewDataPool m_assembleViewDataPool;
  QString statusText_ = tr("就绪");
  QList<ObjectEntry> m_objects;
  /// Phase 236 (DLG-03): outside-bed detection results (source indices +
  /// names) and the pending ObjColorDialog payload. m_pendingObjCheckPath
  /// carries the .obj path between loadFile() and the loadFinished handler.
  QVariantList m_objectsOutsideBed;
  QVariantList m_pendingObjColors;
  int m_pendingObjObjectIndex = -1;
  QString m_pendingObjCheckPath;
  /// Phase 237 (VIEW-04): set by loadFile and consumed by the loadFinished
  /// handler so the zero-volume / unit-inference checks only run for model
  /// imports (upstream gates them on !is_project_file, Plater.cpp:4229).
  bool m_pendingLoadIsModelImport = false;
  int m_preLoadModelCount = 0;
  QSet<int> m_selectedSourceIndices;
  QSet<QString> m_collapsedGroupKeys;
  QSet<int> m_collapsedObjectSourceIndices;
  QList<ObjectEntry> m_clipboard; ///< 剪贴板缓冲区
  /// v5.16 (UNDO-05): 剪贴板整对象 3MF 快照（与 m_clipboard 条目平行，对齐上游
  /// Selection::copy_to_clipboard 的对象深拷贝语义——粘贴恢复网格/变换/覆盖而非仅名字）
  QList<QByteArray> m_clipboardSnapshots;
  int m_primarySelectedSourceIndex = -1;
  int m_selectedVolumeObjectSourceIndex = -1;
  QSet<int> m_selectedVolumeIndices;
  int m_selectedVolumeIndex = -1;
  bool m_showAllObjects = false;
  bool m_showLabels = false;
  int m_objectOrganizeMode = 0;
  QString m_sliceEstimatedTime;
  int m_sliceResultPlateIndex = -1;
  QVector4D m_fitHint; ///< (cx, cy, cz, radius) in GL coords; zero = invalid
  QVector4D m_measureDimensions; ///< (dx, dy, dz, volume) in mm; zero = no selection
  bool m_uniformScale = true;     ///< uniform scale lock (对齐上游 ObjectManipulation)
  // Flatten/Cut state (对齐上游 GLGizmoFlatten / GLGizmoCut)
  int m_flattenFaceCount = 0;
  int m_cutAxis = 2;              ///< 0=X, 1=Y, 2=Z (默认沿 Z 轴切割)
  qreal m_cutPosition = 0.0;
  int m_cutKeepMode = 0;          ///< 0=全部保留, 1=保留上半, 2=保留下半
  /// Phase 240 (GIZ-03/GIZ-04): flatten hover highlight payload + interactive
  /// cut-plane tilt (upstream GLGizmoCut3D m_rotation).
  QByteArray m_flattenHoverData;
  float m_cutRotationX = 0.0f;    ///< degrees, tilt around world X
  float m_cutRotationY = 0.0f;    ///< degrees, tilt around world Y
  float m_cutRotationZ = 0.0f;    ///< degrees, spin around the plane normal
  // Cut connector (对齐上游 GLGizmoCut connector settings)
  int m_cutMode = 0;              ///< 0=Planar, 1=TongueAndGroove
  int m_connectorType = 0;        ///< 0=Plug, 1=Dowel, 2=Snap
  int m_connectorStyle = 0;       ///< 0=Prism, 1=Frustum
  int m_connectorShape = 3;       ///< 0=Triangle, 1=Square, 2=Hexagon, 3=Circle
  float m_connectorSize = 5.0f;   ///< 连接器尺寸 mm
  /// v5.12 gap-closure: independent groove params (对齐上游 Cut::Groove).
  /// When >0, override the connectorSize-derived defaults. grooveFlapsAngle
  /// is in degrees from QML; advCutSelected converts to radians.
  float m_grooveDepth = 0.0f;         ///< 0 = use connectorSize default
  float m_grooveWidth = 0.0f;         ///< 0 = use connectorSize*4 default
  float m_grooveFlapsAngle = 0.0f;    ///< 0 = use 60° (PI/3) default
  float m_connectorDepth = 0.5f;  ///< 深度比 0-1
  // Measure selection (对齐上游 GLGizmoMeasure)
  int m_measureSelectionMode = 0; ///< 0=Default point, 1=Feature selection
  QByteArray m_cachedMeshData;
  QByteArray m_sequentialClearanceOutline;
  QByteArray m_sequentialClearanceFill;
  QByteArray m_sequentialHeightFill;
  // P15.11: drag-time preview state. m_validationClearance* backs up the
  // post-validate payload while the drag overwrites the live streams; the
  // restore at drag end (upstream reset_sequential_print_clearance at
  // mouse_up, GLCanvas3D.cpp:4698) brings the last validation result back
  // (empty backup = cleared). The generation counter cancels in-flight
  // QtConcurrent computes when the drag ends or a newer tick supersedes them.
  bool m_sequentialClearancePreviewMode = false;
  bool m_dragClearanceActive = false;
  quint64 m_dragClearanceGeneration = 0;
  QTimer *m_dragClearanceTimer = nullptr; ///< 150 ms single-shot move debounce
  SequentialClearanceCompute::HullCache m_dragClearanceHullCache;
  SequentialClearanceCompute::ConfigValues m_dragClearanceConfig;
  QByteArray m_validationClearanceOutline;
  QByteArray m_validationClearanceFill;
  QByteArray m_validationClearanceHeightFill;
  QList<int> m_cachedMeshBatchSourceObjectIndices;
  QList<int> m_cachedMeshBatchVolumeIndices;
  QList<int> m_cachedMeshBatchInstanceIndices;
  // P15.1/15.2 (COLOR): per-batch render-channel metadata caches.
  QList<int> m_cachedMeshBatchVolumeTypes;
  QList<int> m_cachedMeshBatchExtruderIds;
  QList<int> m_cachedMeshBatchPrintableFlags;
  ContextMenuFamily m_contextMenuFamily = ContextMenuNone;
  int m_contextSourceObjectIndex = -1;
  int m_contextVolumeIndex = -1;
  int m_contextInstanceIndex = -1;
  int m_contextPlateIndex = -1;
  QHash<QString, QVariant> m_contextProcessSettingsClipboard;
  bool m_contextProcessSettingsAreVolumeScoped = false;
  QList<int> m_sliceAllQueue; ///< plate indices queued for Slice All
  bool m_slicingAll = false;
  QSet<int> m_slicedPlateIndices; ///< tracks which plates have valid slice results
  QSet<int> m_stalePlateIndices; ///< tracks plates whose previous slice result no longer matches current inputs
  /// Phase 239 (ENGN-01): set by switchToPreview() when the current plate's
  /// result is stale (kick reslice) or valid (kick previous-G-code reuse);
  /// consumed by the sliceFinished handler to complete the page switch.
  bool m_pendingPreviewAfterSlice = false;
  /// Phase 239 (ENGN-02): the current preview entry reused the previous
  /// G-code; cleared by the slice-result invalidation set.
  bool m_previewReusedPreviousGcode = false;
  // Viewport warnings (对齐上游 EWarning)
  int m_viewportWarning = 0;    ///< 0=none, 1=ObjectOutside, 2=ObjectClashed
  QString m_viewportWarningMessage;
  // Arrange settings (对齐上游 ArrangeSettings)
  float m_arrangeDistance = 0.f;       ///< 0 = auto spacing
  bool  m_arrangeRotation = false;    ///< 自动旋转
  bool  m_arrangeAlignY = false;      ///< 对齐 Y 轴
  bool  m_arrangeMultiMaterial = true; ///< 允许多耗材同板
  bool  m_arrangeAvoidCalibration = true; ///< 避免校准区域
  // Support painting (对齐上游 GLGizmoFdmSupports)
  int m_activePaintKind = 0;               ///< 0=Support, 1=Seam, 2=Mmu (Phase 122/123 write-back routing)
  int m_supportPaintTool = 1;              ///< 0=None, 1=Enforcer, 2=Blocker
  int m_supportPaintCursorType = 0;        ///< 0=Circle, 1=Sphere, 2=Pointer, 3=HeightRange, 4=GapFill
  int m_supportPaintToolType = 0;          ///< 0=Brush, 1=BucketFill, 2=SmartFill, 3=GapFill
  float m_supportPaintCursorRadius = 5.0f; ///< Brush radius in mm
  float m_supportPaintAngleThreshold = 45.0f; ///< Overhang highlight angle
  float m_supportPaintSmartFillAngle = 30.0f; ///< Smart fill angle threshold
  float m_supportPaintGapArea = 1.0f;      ///< Gap fill area threshold
  bool m_supportPaintOnOverhangsOnly = false; ///< Restrict painting to overhangs
  float m_supportPaintOverhangAngle = 0.0f;   ///< GIZ-02 overhang filter angle (deg)
  bool m_supportEnable = false;            ///< Support enabled flag
  int m_supportType = 0;                   ///< 0=normal, 1=tree
  QList<OWzx::ObjectPaintData> m_paintData;  ///< 对齐上游 per-volume paint data
  // Seam painting (对齐上游 GLGizmoSeam)
  int m_seamPaintTool = 0;                 ///< 0=None, 1=Enforcer, 2=Blocker
  float m_seamPaintCursorRadius = 2.0f;
  bool m_seamPaintOnOverhangsOnly = false;
  // Hollow gizmo (对齐上游 GLGizmoHollow)
  bool m_hollowEnabled = true;             ///< m_enable_hollowing
  float m_hollowHoleRadius = 2.0f;        ///< m_new_hole_radius
  float m_hollowHoleHeight = 6.0f;        ///< m_new_hole_height
  float m_hollowOffset = 3.0f;            ///< m_offset_stash
  float m_hollowQuality = 0.5f;           ///< m_quality_stash
  float m_hollowClosingDistance = 2.0f;    ///< m_closing_d_stash
  int m_hollowHoleCount = 0;      ///< selected drain holes count
  QByteArray m_hollowMarkerData;         ///< packed world-space disc vertices for renderer
  QByteArray m_advancedCutMarkerData;    ///< v5.13: packed connector-pin marker vertices
  int m_simplifyWantedCount = 0;          ///< target triangle count (0=auto)
  float m_simplifyMaxError = 0.0f;       ///< max quadric error (0=auto)
  int m_mmuSelectedExtruder = 0;          ///< currently selected extruder for MMU painting
  int m_mmuExtruderCount = 4;             ///< number of available extruders
  // Drill (对齐上游 GLGizmoDrill)
  float m_drillRadius = 5.0f;
  float m_drillDepth = 50.0f;
  int m_drillShape = 0;                  ///< 0=Circle, 1=Triangle, etc.
  int m_drillDirection = 0;              ///< 0=Top-down, 1=Bottom-up
  bool m_drillOneLayerOnly = false;
  // Emboss (对齐上游 GLGizmoEmboss)
  QString m_embossText;
  float m_embossHeight = 2.0f;
  float m_embossDepth = 1.0f;
  QString m_embossFontPath; // Phase 144 (EMB-01): empty = default system font
  // Phase 158 (EMBO-F01): style axes. boldness + italic map to FontProp;
  // use-surface + curve-projection are projection concepts (geometry
  // deformation deferred — upstream has no ProjectCurve class).
  float m_embossBoldness = 0.0f;
  bool m_embossItalic = false;
  bool m_embossUseSurface = false;
  bool m_embossCurveProjection = false;
  // Phase 196 (FEAT-01): true while an async emboss job is in flight.
  bool m_embossRunning = false;
  // Phase 158 (EMBO-F02): SVG depth-modifier (Z-scale on the imported mesh).
  float m_svgDepthModifier = 1.0f;
  // MeshBoolean (对齐上游 GLGizmoMeshBoolean)
  int m_booleanOperation = 1;             ///< 0=union, 1=diff, 2=intersect

  // Phase 69: move-gizmo drag coalescing state.
  bool m_gizmoMoveDragActive = false;
  QVector3D m_gizmoMoveDragStartPos;
  int m_gizmoMoveDragSourceIndex = -1;
  bool m_gizmoRotateDragActive = false;
  QVector3D m_gizmoRotateDragStartRot;
  int m_gizmoRotateDragSourceIndex = -1;
  bool m_gizmoScaleDragActive = false;
  QVector3D m_gizmoScaleDragStartScale;
  int m_gizmoScaleDragSourceIndex = -1;
  // AdvancedCut (对齐上游 GLGizmoAdvancedCut)
  int m_advCutAxis = 2;                  ///< 0=X, 1=Y, 2=Z
  float m_advCutPosition = 0.0f;
  bool m_advCutKeepBoth = true;
  bool m_advCutConnectors = false;
  /// Phase 240 (GIZ-04): interactive plane tilt for the AdvancedCut plane.
  float m_advCutRotationX = 0.0f;
  float m_advCutRotationY = 0.0f;
  float m_advCutRotationZ = 0.0f;
  // Phase 240 (GIZ-06): simplify three-stage preview state (upstream
  // GLGizmoSimplify m_state running/success + preview triangle count).
  bool m_simplifyPreviewRunning = false;
  int m_simplifyPreviewTriangleCount = -1;
  quint64 m_simplifyPreviewGeneration = 0;
  int m_simplifyPreviewObjectIndex = -1;
  int m_simplifyPreviewWantedCount = 0;
  float m_simplifyPreviewMaxError = 0.f;
  // FaceDetector (对齐上游 GLGizmoFaceDetector)
  float m_faceDetectorAngle = 5.0f;
  int m_faceDetectorResultCount = 0;  ///< v5.12: faces found by detectFlatFaces
  // Text (对齐上游 GLGizmoText)
  QString m_textContent;
  float m_textSize = 20.0f;
  // SVG (对齐上游 GLGizmoSVG)
  QString m_svgFilePath;
  float m_svgScale = 1.0f;
  // Bed shape state (对齐上游 BedShapeDialog / bed_shape)
  float m_bedWidth = 220.0f;
  float m_bedDepth = 220.0f;
  float m_bedMaxHeight = 300.0f;
  float m_bedOriginX = 0.0f;
  float m_bedOriginY = 0.0f;
  int m_bedShapeType = 0;                 ///< 0=Rectangle, 1=Circle, 2=Custom
  float m_bedDiameter = 220.0f;
  // v5.15 (BEDTEX): bed texture path + last-synced printer preset (guards
  // the stateChanged-driven re-sync so only preset changes re-apply the bed).
  QUrl m_bedTextureUrl;
  QString m_bedSyncedPreset;
  QByteArray m_bedModelMeshData;
  bool m_bedTypeTexturesActive = false;
  bool m_bedCaliLinesActive = false;
  QString m_bedTypeImagesDir;
  QVariantList m_bedExcludeAreas;
  float m_bedHeightToRod = 0.0f;
  float m_bedHeightToLid = 0.0f;
  // Phase 100 (WTREAD-01): wipe-tower geometry mirrored from the SliceService
  // readback (Print::wipe_tower_data() captured by value in the worker).
  // Defaults match RhiViewport.h:304-309 (show=false, 10/10/50/100/25) so the
  // pre-slice renderer is unchanged. m_showWipeTower is the WTREAD-02 gate
  // (Print::has_wipe_tower()): false on single-material slices so no
  // placeholder box leaks.
  bool m_showWipeTower = false;
  float m_wipeTowerWidth = 10.f;
  float m_wipeTowerDepth = 10.f;
  float m_wipeTowerHeight = 50.f;
  float m_wipeTowerX = 100.f;
  float m_wipeTowerZ = 25.f;
  // Phase 109 (WTMESH-01/02): Option B real-mesh readback state. m_hasRealMesh
  // gates the renderer branch; m_meshVertices carries the flattened XYZ triples
  // (libslic3r world frame) as a std::vector<float> for the getter to convert
  // to QVariantList. Defaults keep hasRealMesh=false so the pre-slice and
  // single-material paths take the Option A fallback.
  bool m_wipeTowerHasRealMesh = false;
  std::vector<float> m_wipeTowerMeshVertices;
  // Phase 108 (FMAP-01): filament-map auto-recommendation readback state.
  // m_hasAutoFilamentMap is the valid gate (mirrors WTREAD-02 showWipeTower):
  // false until a valid auto recommendation arrives. m_autoFilamentMapMode is
  // the resolved OWzx::FilamentMapMode; m_autoFilamentMaps is the 1-based
  // per-extruder group id list. Defaults keep the pre-slice UI inert so no
  // stale recommendation leaks before the first real readback.
  bool m_hasAutoFilamentMap = false;
  int m_autoFilamentMapMode = static_cast<int>(OWzx::FilamentMapMode::fmmDefault);
  QVariantList m_autoFilamentMaps;
  // Phase 114 (MEASURE-03): feature-picking measurement readout cache.
  // m_measureReadoutValid is the gate (mirrors WTREAD-02 / FMAP-01 valid-
  // flag pattern): false until a valid readout is computed, so no stale
  // value leaks to the Phase 115 UI. The text getters format on demand;
  // the raw values live here and are surfaced via the Q_PROPERTY getters.
  struct MeasureReadout
  {
    bool valid = false;
    bool hasAngle = false;
    float angleDeg = 0.0f;          // Measure::AngleAndEdges.angle, deg
    bool hasPerpendicularDistance = false;
    float perpendicularDistance = 0.0f; // distance_infinite
    bool hasDirectDistance = false;
    float directDistance = 0.0f;    // distance_strict
    bool hasDistanceXyz = false;
    QVector3D distanceXyz;          // distance_xyz
  };
  MeasureReadout m_measureReadout;
#ifdef HAS_LIBSLIC3R
  // Phase 114 (MEASURE-03): per-volume Measure::Measuring engine. Held via
  // unique_ptr + forward-declared so the MeasureEngine.h (and its libslic3r
  // Point.hpp include) does NOT leak into this header. Lazily constructed
  // on the first computeMeasureReadoutFromHit call, invalidated on mesh
  // change via invalidateMeasureEngine(). Null when HAS_LIBSLIC3R is off.
  std::unique_ptr<OWzx::MeasureEngine> m_measureEngine;
  // Phase 114 (MEASURE-03): the first ("from") feature of the two-click
  // measure flow (mirrors GLGizmoMeasure m_selected_features.first). When
  // set, the next computeMeasureReadoutFromHit measures against it; when
  // empty, the next hit becomes the "from" feature. Held as the hit fields
  // (no libslic3r SurfaceFeature -- pitfall 6). The volume world transform
  // is stored as its translation/rotation/scale decomposition so the C++
  // side can rebuild the Eigen Transform3d (Eigen types cannot be Q_PROPERTY
  // members and cannot cross QML). The valid flag + world point moved
  // outside the guard (Phase 240 GIZ-05 overlay getters).
  int m_measureFromObjectIndex = -1;
  int m_measureFromVolumeIndex = -1;
  QVector3D m_measureFromVolumeTranslation;
  QVector3D m_measureFromVolumeRotationRad;
  QVector3D m_measureFromVolumeScale{1.0f, 1.0f, 1.0f};
  int m_measureFromFacetIdx = -1;
  // Phase 115 (MEASURE-04): two-stage pick stage-2 raycaster. Lazily built on
  // the first pickMeasureFeatureAt call from the SAME ITS source MeasureEngine
  // uses (ProjectServiceMock::volumeMeshIts, Phase 112). invalidateMeasureEngine
  // drops both caches so a mesh change refreshes them together (pitfall 6/7).
  std::unique_ptr<OWzx::SceneRaycaster> m_sceneRaycaster;
  // Phase 120 (PAINT-01): per-volume TriangleSelector owner. Lazily built on
  // the first paintAtFacet call from ProjectServiceMock::volumeMeshTriangleMesh
  // (TS-01 aliasing shared_ptr -- keeps the TriangleMesh alive for the
  // selector's lifetime). Reuses m_sceneRaycaster for the stage-2 hit so the
  // raycaster + paint engine share the same ITS identity + cache invalidation
  // (invalidateMeasureEngine drops both). The m_paintData QList<ObjectPaintData>
  // above is kept only as a back-compat enum mirror; PaintEngine is the real
  // per-volume owner (replaces the flat placeholder, CONTEXT.md gap 3).
  std::unique_ptr<OWzx::PaintEngine> m_paintEngine;
#endif
  // Phase 114 (MEASURE-03) + Phase 240 (GIZ-05): the "from" anchor flag +
  // world point live OUTSIDE the HAS_LIBSLIC3R guard so the overlay getters
  // (measureOverlayData/Labels) compile unconditionally with a defined
  // (false/origin) value when libslic3r is off (same pattern as
  // m_measureHoverWorldPosition below).
  bool m_measureFromFeatureValid = false;
  QVector3D m_measureFromWorldPoint;
  // Phase 115 (MEASURE-04): the most recently picked feature kind + world
  // position (drives the gizmo overlay highlight). Reset to Undef/origin by
  // clearMeasureReadout() (mirrors WTREAD-02 / FMAP-01 always-emit pattern).
  // Kept outside the HAS_LIBSLIC3R guard so the Q_PROPERTY getters always have
  // a defined value (0/origin) when libslic3r is off.
  int m_measureHoverFeatureKind = 0;  // OWzx::FeatureKind::Undef == 0
  QVector3D m_measureHoverWorldPosition;
};
