#pragma once

#include <QObject>
#include <QVariant>
#include <QStringList>
#include <QList>
#include <QByteArray>
#include <QVector3D>
#include <atomic>
#include <memory>

#include "core/model/PartPlateList.h"

#ifdef HAS_LIBSLIC3R
// Phase 112 (MEASURE-01): indexed_triangle_set lives at GLOBAL scope in
// libslic3r (forward-declared before `namespace Slic3r` at Measure.hpp:9 and
// used as a public value member of TriangleMesh at TriangleMesh.hpp:158). It
// is forward-declared here (not fully included) so the header stays free of
// the TriangleMesh.hpp / Point.hpp / admesh pollution; the complete type is
// only required in the .cpp where the accessor is implemented.
struct indexed_triangle_set;
namespace Slic3r
{
  class Model;
  class TriangleSelector;
}
#endif

/// Volume type enum (对齐上游 ModelVolumeType, ModelCommon.hpp)
enum class MockVolumeType {
  ModelPart = 0,
  NegativeVolume = 1,
  ParameterModifier = 2,
  SupportBlocker = 3,
  SupportEnforcer = 4,
  TextEmboss = 5,     ///< 对齐上游 GLGizmoText — 文字浮雕体积
  SvgEmboss = 6       ///< 对齐上游 GLGizmoSVG — SVG 浮雕体积
};

/// Paint kind for the Phase 122/123 TriangleSelector -> FacetsAnnotation bridge.
/// Mirrors upstream update_model_object (GLGizmoFdmSupports.cpp:577,
/// GLGizmoSeam.cpp:315, GLGizmoMmuSegmentation.cpp:700) which writes the
/// TriangleSelector into one of three ModelVolume FacetsAnnotation members.
enum class PaintKind { Support, Seam, Mmu };

/// Mock volume data entry
struct MockVolumeEntry {
  QString name;
  MockVolumeType type = MockVolumeType::ModelPart;
  int extruderId = -1;  // -1 = inherit from object/parent, 0+ = specific extruder (对齐上游 ModelVolume::extruder_id)
};

/// Layer range entry (对齐上游 t_layer_height_range → ModelConfig mapping)
struct MockLayerRange {
  double minZ = 0.0;
  double maxZ = 0.0;
  QHash<QString, QVariant> overrides;
};

class ProjectServiceMock final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString projectName READ projectName NOTIFY projectChanged)
  Q_PROPERTY(int modelCount READ modelCount NOTIFY projectChanged)
  Q_PROPERTY(int plateCount READ plateCount NOTIFY plateDataLoaded)
  Q_PROPERTY(int currentPlateIndex READ currentPlateIndex NOTIFY plateSelectionChanged)
  // v3.2 Phase 29 D-29-15: read-only plate-grid geometry for debug/QML.
  // cols/stride derive from plateCount, so NOTIFY reuses plateDataLoaded.
  Q_PROPERTY(int plateCols READ plateCols NOTIFY plateDataLoaded)
  Q_PROPERTY(double plateStrideX READ plateStrideX NOTIFY plateDataLoaded)
  Q_PROPERTY(double plateStrideY READ plateStrideY NOTIFY plateDataLoaded)
  Q_PROPERTY(QString lastError READ lastError NOTIFY projectChanged)
  Q_PROPERTY(int loadProgress READ loadProgress NOTIFY loadProgressChanged)
  Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

public:
  explicit ProjectServiceMock(QObject *parent = nullptr);
  ~ProjectServiceMock();

  QString projectName() const;
  int modelCount() const;
  int plateCount() const;
  int currentPlateIndex() const;
  // v3.2 Phase 29 D-29-15: plate-grid geometry (read-only, derived from
  // PartPlateList). Null-guarded for safety before m_plateList is constructed.
  int plateCols() const { return m_plateList ? m_plateList->plateCols() : 0; }
  double plateStrideX() const { return m_plateList ? m_plateList->plateStrideX() : 0.0; }
  double plateStrideY() const { return m_plateList ? m_plateList->plateStrideY() : 0.0; }

  // v3.2 Phase 30: read-only access to the plate list (for tests / inspection).
  const OWzx::PartPlateList *plateListConst() const { return m_plateList.get(); }
  // v3.2 Phase 30: mutable access for tests (cache manipulation). Production
  // code should go through the Q_INVOKABLE API; this is a test seam.
  OWzx::PartPlateList *plateListMut() { return m_plateList.get(); }

  /// Sets plate width/depth/height (mm) and refreshes origins. Test seam.
  void setPlateSize(int width, int depth, int height) {
    if (m_plateList) m_plateList->setPlateSize(width, depth, height);
  }

  /// Reconstruct per-plate instance membership from current model world offsets
  /// (v3.2 Phase 29, ARRANGE-02). Public test/load-path hook: useful for
  /// 3MF-loaded projects that already span multiple plates (their instance
  /// offsets were parsed from the archive), and for deterministic tests.
  /// exceptLocked=true preserves locked-plate membership.
  Q_INVOKABLE void rebuildPlateMembership(bool exceptLocked = true) {
    if (!m_plateList) return;
#ifdef HAS_LIBSLIC3R
    m_plateList->setModel(model_);
#endif
    m_plateList->rebuildPlatesAfterArrangement(exceptLocked, /*recyclePlates=*/false);
    emit plateDataLoaded(m_plateList->plateCount());
  }
  QString lastError() const;
  int loadProgress() const;
  bool loading() const;
  QString sourceFilePath() const;

#ifdef HAS_LIBSLIC3R
  std::unique_ptr<Slic3r::Model> cloneCurrentPlateModel() const;
  Slic3r::Model *rawModel() const { return model_; }

  // Phase 122/123 (PAINT-04/05): write the painted TriangleSelector into the
  // ModelVolume FacetsAnnotation member for the given paint kind. Mirrors
  // upstream update_model_object (GLGizmoFdmSupports.cpp:577 for Support,
  // GLGizmoSeam.cpp:315 for Seam, GLGizmoMmuSegmentation.cpp:700 for Mmu).
  // FacetsAnnotation::set (Model.cpp:3524) serializes + touch() (timestamp bump
  // -> re-slice). cloneCurrentPlateModel deep-copies FacetsAnnotation, so the
  // slice consumes it automatically; 3MF persistence is automatic via
  // bbs_3mf paint_supports/paint_seam/paint_color attributes.
  bool writePaintToModelVolume(int objectIndex, int volumeIndex,
                               PaintKind kind,
                               const Slic3r::TriangleSelector &selector);
  // Reset the FacetsAnnotation member for the given paint kind (clear paint).
  bool clearPaintOnModelVolume(int objectIndex, int volumeIndex, PaintKind kind);
#endif

  /// 加载 3MF/STL/OBJ 等模型文件（真正调用 libslic3r）
  Q_INVOKABLE bool loadFile(const QString &filePath);
  Q_INVOKABLE void cancelLoad();

  // v2.4 IO-01: 项目保存（调用 libslic3r store_3mf 真实导出 .3mf）
  Q_INVOKABLE bool saveProjectAs(const QString &filePath);
  // v2.4 IO-02: 导出模型（STL/3MF/OBJ 格式）
  Q_INVOKABLE bool exportModel(const QString &filePath, const QString &format);
  /// v2.4: 当前项目路径（saveProjectAs 后更新）
  QString currentProjectPath() const { return currentProjectPath_; }

  /// 返回已加载的模型对象名称列表
  Q_INVOKABLE QStringList objectNames() const;
  Q_INVOKABLE QStringList plateNames() const;
  Q_INVOKABLE QString objectModuleName(int index) const;
  Q_INVOKABLE bool setCurrentPlateIndex(int index);
  /// 添加新平板（对齐上游 Plater::PartPlateList）
  Q_INVOKABLE bool addPlate();
  /// 删除指定平板（至少保留 1 个平板）
  Q_INVOKABLE bool deletePlate(int plateIndex);
  /// 重命名指定平板
  Q_INVOKABLE bool renamePlate(int plateIndex, const QString &newName);
  /// 删除指定平板上的所有对象
  Q_INVOKABLE bool removeAllOnPlate(int plateIndex);
  /// 平板锁定状态
  Q_INVOKABLE bool isPlateLocked(int plateIndex) const;
  Q_INVOKABLE bool setPlateLocked(int plateIndex, bool locked);
  // ── v3.0 Phase 17: plate lifecycle completion (PLATE-03/04/05) ──
  /// Deep-copy a plate including its ModelObjects (D-06, upstream duplicate_plate).
  Q_INVOKABLE bool clonePlate(int sourceIndex);
  /// Reorder plate oldIndex to newIndex (D-07).
  Q_INVOKABLE bool movePlate(int oldIndex, int newIndex);
  Q_INVOKABLE bool setPlatePrintable(int plateIndex, bool printable);
  Q_INVOKABLE bool isPlatePrintable(int plateIndex) const;

  // v3.2 Phase 31 (FMAP-03, Manual mode) + v4.5 Phase 107 (FMAP-02): per-plate
  // filament->extruder mapping. `mode` is the widened 4-value FilamentMapMode
  // (OWzx::FilamentMapMode): fmmAutoForFlush=0, fmmAutoForMatch=1, fmmManual=2,
  // fmmDefault=3 (per-plate "inherit from global" sentinel; the UI does NOT
  // expose fmmDefault as a 4th radio button -- anti-feature per FEATURES.md).
  // maps[i] = the extruder index that filament i maps to (1-based, matching
  // upstream PlateData::filament_maps at bbs_3mf.hpp:98).
  Q_INVOKABLE bool setPlateFilamentMap(int plateIndex, int mode, const QList<int>& maps);
  // Phase 110 (FMAP-03): mode-only write path for the FilamentGroupPopup. The
  // full setPlateFilamentMap above takes mode + maps together; the popup only
  // changes the mode and leaves the per-extruder maps untouched, so this
  // thin variant reuses the existing plate-write plumbing (reads the current
  // maps then delegates to setPlateFilamentMap). PartPlate::setFilamentMapMode
  // (Phase 110 R-02 / FP-04) clamps out-of-range ints at the Q_INVOKABLE
  // boundary, so a QML caller passing mode=5 cannot store an invalid enum.
  Q_INVOKABLE bool setPlateFilamentMapMode(int plateIndex, int mode);
  Q_INVOKABLE int plateFilamentMapMode(int plateIndex) const;
  Q_INVOKABLE QList<int> plateFilamentMaps(int plateIndex) const;
#ifdef HAS_LIBSLIC3R
  // v3.0 Phase 19 (D-15): per-plate DynamicPrintConfig for slice config merge.
  // Returns Slic3r::DynamicPrintConfig* (forward-declared in the HAS_LIBSLIC3R block).
  const ::Slic3r::DynamicPrintConfig *plateDynamicConfig(int plateIndex) const;
#endif
  Q_INVOKABLE QList<int> plateObjectIndices(int plateIndex) const;
  Q_INVOKABLE QList<int> currentPlateObjectIndices() const;
  Q_INVOKABLE int plateObjectCount(int index) const;
  Q_INVOKABLE int plateIndexForObject(int objectIndex) const;
  /// 移动对象到指定平板（对齐上游跨平板拖拽）
  void setObjectPlateForIndex(int objectIndex, int plateIndex);
  Q_INVOKABLE bool objectPrintable(int index) const;
  Q_INVOKABLE bool setObjectPrintable(int index, bool printable);
  Q_INVOKABLE bool objectVisible(int index) const;
  Q_INVOKABLE bool setObjectVisible(int index, bool visible);
  Q_INVOKABLE int objectVolumeCount(int index) const;
  /// 每个对象的实例数量（对齐上游 ModelObject::instances.size()）
  Q_INVOKABLE int objectInstanceCount(int index) const;
  Q_INVOKABLE QString objectVolumeName(int objectIndex, int volumeIndex) const;
  Q_INVOKABLE QString objectVolumeTypeLabel(int objectIndex, int volumeIndex) const;
  /// Get volume type as integer (0=ModelPart, 1=NegativeVolume, etc.) for the given volume
  Q_INVOKABLE int objectVolumeType(int objectIndex, int volumeIndex) const;
  QVariant scopedOptionValue(int objectIndex, int volumeIndex, const QString &key, const QVariant &fallbackValue = QVariant()) const;
  bool setScopedOptionValue(int objectIndex, int volumeIndex, const QString &key, const QVariant &value);
  /// Plate-level scoped overrides (对齐上游 PartPlate config override)
  QVariant plateScopedOptionValue(int plateIndex, const QString &key, const QVariant &fallbackValue = QVariant()) const;
  bool setPlateScopedOptionValue(int plateIndex, const QString &key, const QVariant &value);
  /// Count overridden keys for per-scope diff panel
  int scopedOverrideCount(int objectIndex, int volumeIndex) const;
  QString scopedOverriddenKey(int objectIndex, int volumeIndex, int index) const;
  bool resetScopedOptionValue(int objectIndex, int volumeIndex, const QString &key);
  int plateScopedOverrideCount(int plateIndex) const;
  QString plateScopedOverriddenKey(int plateIndex, int index) const;
  bool resetPlateScopedOptionValue(int plateIndex, const QString &key);
  Q_INVOKABLE bool deleteObjectVolume(int objectIndex, int volumeIndex);
  /// Add a new volume to the specified object (对齐上游 GUI_ObjectList::load_subobject / load_generic_subobject)
  /// volumeType: 0=ModelPart, 1=NegativeVolume, 2=ParameterModifier, 3=SupportBlocker, 4=SupportEnforcer
  Q_INVOKABLE bool addVolume(int objectIndex, int volumeType);
  /// Change volume type (对齐上游 GUI_ObjectList::load_generic_subobject type conversion)
  Q_INVOKABLE bool changeVolumeType(int objectIndex, int volumeIndex, int newVolumeType);
  /// Volume-level extruder assignment (对齐上游 ModelVolume::extruder_id)
  /// Returns -1 if inherit from object/parent, 0+ for specific extruder
  Q_INVOKABLE int volumeExtruderId(int objectIndex, int volumeIndex) const;
  /// Set volume extruder assignment. -1 = inherit, 0+ = specific extruder (1-based in UI, 0-based internally)
  Q_INVOKABLE bool setVolumeExtruderId(int objectIndex, int volumeIndex, int extruderId);
  /// 从外部文件导入 volume（对齐上游 GUI_ObjectList::load_generic_subobject 文件加载）
  /// Mock 模式：创建 volume 条目并命名
  Q_INVOKABLE bool addVolumeFromFile(int objectIndex, const QString &filePath, int volumeType);
  /// 添加原始体 volume（对齐上游 create_mesh + add_volume）
  /// primitiveType: 0=立方体, 1=球体, 2=圆柱体, 3=圆环
  Q_INVOKABLE bool addPrimitive(int objectIndex, int primitiveType);
  /// 添加文字浮雕 volume（对齐上游 GLGizmoText）
  /// Mock 模式：创建 TextEmboss 类型 volume
  Q_INVOKABLE bool addTextVolume(int objectIndex, const QString &text);
  /// Phase 145 (EMB-03): async variant of addTextVolume. Spawns a Qt Concurrent
  /// worker that runs the text2shapes + polygons2model pipeline off the GUI
  /// thread; result is delivered via the embossVolumeAdded signal on the GUI
  /// thread. Caller should connect to embossVolumeAdded + embossVolumeFailed
  /// before invoking. cancelEmbossVolume() cancels the in-flight job (a second
  /// invocation auto-cancels the prior — typing fast does not pile up stale
  /// jobs). Mirrors the codebase's QtConcurrent::run + atomic-cancel pattern
  /// used by loadModelFromPath (ProjectServiceMock.cpp:540+) and SliceService.
  Q_INVOKABLE void addTextVolumeAsync(int objectIndex, const QString &text);
  Q_INVOKABLE void cancelEmbossVolume();
  /// Synchronous helper (worker-thread side) that performs the actual
  /// text2shapes+polygons2model pipeline and writes the resulting volume to the
  /// model. Returns false on failure + fills `errMsg`. Pre-Phase-145 this was
  /// the body of addTextVolume; the synchronous Q_INVOKABLE now forwards to it,
  /// and addTextVolumeAsync spawns it via QtConcurrent::run.
  bool performEmbossVolumeAdd(int objectIndex, const QString &text, QString &errMsg);
  /// Phase 144 (EMB-01): font path + height/depth setters. Used by addTextVolume
  /// to parameterize the Emboss pipeline (was hardcoded arial.ttf / 10mm / 2mm).
  /// The viewmodel forwards the user-selected font + embossHeight/embossDepth
  /// Q_PROPERTY values here before invoking addTextVolume.
  void setEmbossFont(const QString &fontPath);
  void setEmbossHeight(float mm);
  void setEmbossDepth(float mm);
  /// Phase 144 (EMB-01): enumerate system fonts via upstream
  /// Emboss::get_font_list_by_enumeration (Windows) or get_font_list_by_folder.
  /// Returns a list of {family name, font file path} pairs for QML.
  QVariantList embossFontList() const;
  /// 添加 SVG 浮雕 volume（对齐上游 GLGizmoSVG）
  /// Mock 模式：创建 SvgEmboss 类型 volume
  Q_INVOKABLE bool addSvgVolume(int objectIndex, const QString &svgFilePath);
  /// 简化对象网格（对齐上游 GLGizmoSimplify → its_quadric_edge_collapse）
  /// wantedCount: 目标三角面数 (0=不限制), maxError: 最大误差 (0=不限制)
  Q_INVOKABLE bool simplifyObject(int objectIndex, int wantedCount, float maxError);
  /// 自动朝向指定对象（对齐上游 Slic3r::orientation::orient(ModelObject*)）
  /// HAS_LIBSLIC3R: 调用真实 orient API；Mock: 返回 false
  Q_INVOKABLE bool orientObject(int objectIndex);
  /// 自动排列全部对象（对齐上游 Plater::priv::on_arrange → Slic3r::arrange_objects）
  /// HAS_LIBSLIC3R: 调用真实 arrange_objects API；Mock: 返回 false
  Q_INVOKABLE bool arrangeObjects(float spacing, bool allowRotation, bool alignY,
                                    const QString &printableArea = QString());
  /// 切割对象（对齐上游 GLGizmoCut::perform_cut → cut_mesh）
  /// axis: 0=X, 1=Y, 2=Z; position: 切割平面位置(mm); keepMode: 0=all, 1=upper, 2=lower
  /// Returns the index of the new object created (≥0 on success, -1 on failure)
  Q_INVOKABLE int cutObject(int objectIndex, int axis, double position, int keepMode);
  /// 榫卯切割（对齐上游 GLGizmoCut::perform_cut → Cut::perform_with_groove）
  /// axis: 0=X, 1=Y, 2=Z; position: 切割平面位置(mm); keepMode: 0=all, 1=upper, 2=lower
  /// grooveDepth/width/flapsAngle/angle: 榫卯参数（对齐上游 Cut::Groove）
  /// Returns the index of the new object created (≥0 on success, -1 on failure)
  Q_INVOKABLE int cutObjectWithGroove(int objectIndex, int axis, double position, int keepMode,
                                       float grooveDepth, float grooveWidth, float grooveFlapsAngle, float grooveAngle);
  /// 镜像对象（对齐上游 ModelInstance::set_mirror）
  Q_INVOKABLE bool mirrorObject(int objectIndex, int axis);
  /// 布尔运算（对齐上游 GLGizmoMeshBoolean::execute_mesh_boolean）
  /// operation: 0=union(A+B), 1=difference(A-B), 2=intersection(A∩B)
  /// Replaces srcObject mesh with the result, deletes toolObject
  Q_INVOKABLE bool meshBoolean(int srcObjectIndex, int toolObjectIndex, int operation);
  /// 钻孔操作（对齐上游 GLGizmoDrill → sla::hollow_mesh_and_drill / MeshBoolean::cgal::minus）
  /// shape: 0=Circle(cylinder), 1=Triangle(triPrism), 2=Square(cube)
  /// direction: 0=Normal(top-down), 1=Bottom-up, 2=Both
  /// Simplified: drills from top of bounding box along -Z at object center
  Q_INVOKABLE bool drillObject(int objectIndex, float radius, float depth, int shape, int direction, bool oneLayerOnly);
  /// 从真实模型同步变换到 Mock 数组（orient/split/arrange/cut 等 API 变更模型后调用）
  void syncTransformsFromModel();

  /// ── Mesh snapshot for undo/redo (used by BooleanCommand / DrillCommand) ──
  /// Serializes the first model_part volume mesh of the given object to a byte array.
  /// Returns empty QByteArray if the object does not exist or has no model_part volume.
  QByteArray captureObjectMeshSnapshot(int objectIndex) const;
  /// Restores the first model_part volume mesh of the given object from a previously captured snapshot.
  /// Returns true on success.
  bool restoreObjectMeshSnapshot(int objectIndex, const QByteArray &snapshot);

  /// ── Layer range support (对齐上游 ModelObject::layer_config_ranges) ──
  /// Get layer ranges for an object
  QList<MockLayerRange> objectLayerRanges(int objectIndex) const;
  /// Add a new layer range to an object
  Q_INVOKABLE bool addObjectLayerRange(int objectIndex, double minZ, double maxZ);
  /// Remove a layer range by index
  Q_INVOKABLE bool removeObjectLayerRange(int objectIndex, int rangeIndex);
  /// Set an override value for a specific layer range
  Q_INVOKABLE bool setLayerRangeValue(int objectIndex, int rangeIndex, const QString &key, const QVariant &value);
  /// Get override value for a specific layer range
  QVariant layerRangeValue(int objectIndex, int rangeIndex, const QString &key, const QVariant &fallback = QVariant()) const;

  Q_INVOKABLE bool deleteObject(int index);

  /**
   * 提取所有三角面顶点为紧凑 float 数组 (x,y,z per vertex, 3 verts per face)。
   * 坐标系转换：libslic3r Z-up → GL Y-up：GL(x,z,y)
   */
  QByteArray meshData() const;
  QList<int> meshBatchSourceObjectIndices() const;

  /// 复制指定对象（对齐上游 Plater::clone_selection 行为）
  /// 返回新对象的索引，失败返回 -1
  Q_INVOKABLE int duplicateObject(int sourceIndex);
  /// 拆分指定对象（对齐上游 ModelObject::split）
  /// HAS_LIBSLIC3R: 调用真实 split API 返回新对象索引列表；Mock: 返回空列表
  Q_INVOKABLE QList<int> splitObject(int objectIndex);
  /// 修复网格（对齐上游 MeshRepairDialog / fix_mesh）
  void fixMeshForObject(int objectIndex);
  /// 修复网格并返回结果（对齐上游 MeshRepairDialog / fix_mesh）
  bool fixMesh(int objectIndex);
  /// 从磁盘重新加载对象源文件（对齐上游 Plater::reload_from_disk）
  bool reloadFromDisk(int objectIndex);
  /// 用 STL 文件替换指定 volume 的网格（对齐上游 GUI_ObjectList::load_subobject）
  bool replaceVolume(int objectIndex, int volumeIndex, const QString &stlPath);
  /// 合并多个对象为一个多部件对象（对齐上游 GUI_ObjectList::assemble）
  bool assembleObjects(const QList<int> &objIndices);
  /// 将指定实例复制为独立对象（对齐上游 GUI_ObjectList::instance_to_object）
  bool duplicateInstanceAsObject(int objectIndex, int instanceIndex);
  /// 创建原始几何体到当前平板（对齐上游 create_mesh + add_volume）
  /// type: 0=Cube, 1=Sphere, 2=Cylinder, 3=Cone, 4=TruncatedCone, 5=Torus, 6=Disc
  Q_INVOKABLE int addPrimitiveToPlate(int type);
  /// 重命名对象（对齐上游 Plater::rename_object）
  Q_INVOKABLE bool renameObject(int index, const QString &newName);
  /// 移动对象位置（对齐上游 GUI_ObjectList 拖拽重排序）
  Q_INVOKABLE bool moveObject(int fromIndex, int toIndex);

  /// 平板设置（对齐上游 PlateSettingsDialog）
  Q_INVOKABLE int plateBedType(int plateIndex) const;
  Q_INVOKABLE bool setPlateBedType(int plateIndex, int bedType);
  Q_INVOKABLE int platePrintSequence(int plateIndex) const;
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
  /// 其他层序列条目数
  Q_INVOKABLE int plateOtherLayersSeqCount(int plateIndex) const;
  /// 获取指定条目的 begin_layer
  Q_INVOKABLE int plateOtherLayersSeqBegin(int plateIndex, int entryIndex) const;
  /// 获取指定条目的 end_layer
  Q_INVOKABLE int plateOtherLayersSeqEnd(int plateIndex, int entryIndex) const;
  /// 获取指定条目的耗材顺序
  Q_INVOKABLE QVariantList plateOtherLayersSeqOrder(int plateIndex, int entryIndex) const;
  /// 添加其他层序列条目
  Q_INVOKABLE bool addPlateOtherLayersSeqEntry(int plateIndex, int beginLayer, int endLayer);
  /// 删除其他层序列条目
  Q_INVOKABLE bool removePlateOtherLayersSeqEntry(int plateIndex, int entryIndex);
  /// 修改其他层序列条目的层范围
  Q_INVOKABLE bool setPlateOtherLayersSeqRange(int plateIndex, int entryIndex, int beginLayer, int endLayer);
  /// 修改其他层序列条目的耗材顺序
  Q_INVOKABLE bool setPlateOtherLayersSeqOrder(int plateIndex, int entryIndex, const QVariantList &order);
  /// 获取当前平板使用的耗材数（Mock 模式基于对象数推断）
  Q_INVOKABLE int plateExtruderCount(int plateIndex) const;
  // Phase 98 (THUMBVERIFY-01): real persisted-thumbnail accessor for the UI
  // plate-card fallback. Returns the cached PartPlate::thumbnail() as base64
  // PNG (empty when no thumbnail is cached -- never fabricates a placeholder).
  // Replaces the removed mock QPainter fallback so non-current plates display
  // their saved/reloaded thumbnail instead of a fake.
  Q_INVOKABLE QString plateThumbnailBase64(int plateIndex) const;
  /// Phase 156 (CLOS-03): runtime thumbnail WRITE path. Decodes the base64
  /// PNG (with or without the `data:image/png;base64,` prefix) into a QImage
  /// and routes it into PartPlate::setThumbnail for the given plate. This is
  /// the missing piece that lets non-current plates show real thumbnails
  /// captured in-session (Phase 151 shipped the read accessor + 3MF save/load
  /// round-trip only). Returns true on successful decode + store.
  Q_INVOKABLE bool setPlateThumbnailFromBase64(int plateIndex, const QString &base64);

  /// 添加新对象到当前平板（对齐上游 Plater 粘贴剪贴板行为）
  /// 返回新对象的索引，失败返回 -1
  Q_INVOKABLE int addObject(const QString &name);

  /// 获取/设置对象变换（对齐上游 ModelObject::get_transformation / set_transformation）
  QVector3D objectPosition(int index) const;
  QVector3D objectRotation(int index) const;  // degrees
  QVector3D objectScale(int index) const;
  bool setObjectPosition(int index, float x, float y, float z);
  bool setObjectRotation(int index, float x, float y, float z);
  bool setObjectScale(int index, float x, float y, float z);
  bool setObjectScaleUniform(int index, float s);

  // ASM-01 (Phase 138): per-instance assemble-transform accessors. In assembly
  // mode the Move/Rotate/Scale gizmos target ModelInstance::m_assemble_transformation
  // (upstream Model.hpp:1253-1298), NOT m_transformation. These mirror the ordinary
  // objectPosition/Rotation/Scale dual-path convention (proxy to ModelInstance under
  // HAS_LIBSLIC3R, else fall back to parallel mock stores) and apply the same
  // slic3r(X,Y,Z) <-> GL(X,Z,Y) Y/Z swap for offset and deg<->rad for rotation.
  QVector3D assembleOffset(int index) const;
  QVector3D assembleRotation(int index) const;  // degrees
  QVector3D assembleScale(int index) const;
  bool setAssembleOffset(int index, float x, float y, float z);
  bool setAssembleRotation(int index, float x, float y, float z);  // degrees in
  bool setAssembleScale(int index, float x, float y, float z);
  // Non-const: upstream ModelInstance::is_assemble_initialized() (Model.hpp:1345)
  // is itself non-const, so we cannot expose this as a const query.
  bool isAssembleInitialized(int index);

  /// 清空当前项目（新建项目/重置场景）
  Q_INVOKABLE void clearProject();
  /// 加载项目元数据（对齐上游 Plater::load_project，Mock 模式从 JSON 恢复）
  Q_INVOKABLE bool loadProject(const QString &filePath);
  /// 保存项目元数据（对齐上游 MainFrame::do_save / Plater::save_project）
  /// Mock 模式保存为 JSON；真实模式调用 3MF writer
  Q_INVOKABLE bool saveProject(const QString &filePath);

#ifdef HAS_LIBSLIC3R
  /// 获取对象三角面数（HAS_LIBSLIC3R 真实统计）
  Q_INVOKABLE int objectTriangleCount(int objectIndex) const;
  /// 获取对象未修复的非流形边数（对齐上游 TriangleMeshStats::open_edges）
  Q_INVOKABLE int objectOpenEdges(int objectIndex) const;
  /// 获取对象已修复的错误总数（对齐上游 ModelObject::get_repaired_errors_count）
  Q_INVOKABLE int objectRepairedErrors(int objectIndex) const;
  /// 获取对象网格体积 mm^3（对齐上游 TriangleMeshStats::volume，考虑实例变换）
  Q_INVOKABLE float objectVolume(int objectIndex) const;

  // == Phase 112 (MEASURE-01): per-volume ITS accessor ========================
  // Returns the per-volume indexed_triangle_set (ITS) for the ModelVolume at
  // (objectIndex, volumeIndex), as a std::shared_ptr<const indexed_triangle_set>.
  //
  // This is the PUBLIC API that unblocks the cross-workstream dependencies:
  //   - Phase 113 (SceneRaycaster port, one raycaster per volume,
  //     SceneRaycaster.hpp:71-72 m_volumes).
  //   - Phase 114 (Measure::Measuring ctor, Measure.hpp:122
  //     `explicit Measuring(const indexed_triangle_set&)`).
  //   - AssembleViewDataPool ModelObjectsClipper (the reserved
  //     AssembleViewDataID::ModelObjectsClipper = 1 << 4 slot at
  //     AssembleViewDataPool.h:44).
  //
  // == OWNERSHIP CONTRACT (Pitfalls.md pitfall 6 / MI-02) =====================
  // The returned shared_ptr is a SHALLOW-SHARE aliasing pointer: it shares the
  // refcount of the ModelVolume's internal std::shared_ptr<const TriangleMesh>
  // (Model.hpp:856 mesh_ptr(), member m_mesh at Model.hpp:1040) and points at
  // &mesh.its (TriangleMesh.hpp:158 -- `its` is a public value member). Because
  // the refcount is shared, the TriangleMesh (and therefore the ITS) stays
  // alive for as long as ANY caller holds the returned shared_ptr -- even if the
  // ModelVolume itself is removed mid-use (deleteObject / cut / boolean / etc.
  // drop the ModelVolume, but the previous ITS shared_ptr remains valid until
  // the last holder releases it). This closes the use-after-free hazard at the
  // libslic3r->Qt boundary documented in pitfall 6 (the "detach-on-copy" and
  // "rebuild timing" lifetime bugs).
  //
  // == SHALLOW-SHARE DECISION (MI-03) =========================================
  // Shallow-share is SAFE here and is chosen over a copy:
  //   1. The ModelVolume already owns the TriangleMesh via a shared_ptr (NOT a
  //      raw owning pointer), so the ITS reference is stable for the
  //      TriangleMesh's lifetime, and the TriangleMesh is reference-counted.
  //   2. set_mesh / reset_mesh (Model.hpp:857-863) always construct a NEW
  //      shared_ptr (make_shared); they never mutate the existing TriangleMesh
  //      in place. A caller holding a prior ITS therefore never observes a
  //      torn-down or detached ITS -- the worst case is a STALE-but-valid ITS,
  //      which is exactly the contract Measure::Measuring needs (it indexes
  //      the mesh in its ctor and treats it as immutable thereafter).
  //   3. Qt implicit-sharing detach does NOT apply: TriangleMesh / ITS are
  //      plain libslic3r types (no QSharedData), and the shared_ptr<const ...>
  //      constness prevents writes from the Qt side.
  // The aliasing constructor (std::shared_ptr<const indexed_triangle_set>(
  // meshSharedPtr, &meshSharedPtr->its)) gives zero-copy shallow-share while
  // preserving the refcount tie to the TriangleMesh. This is the cleanest
  // approach.
  //
  // == CACHE DECISION (MI-04) =================================================
  // NO separate mesh cache is added on ProjectServiceMock. Rationale: the
  // ModelVolume's shared_ptr<const TriangleMesh> m_mesh IS the cache -- it is
  // the in-memory source of truth, it is already reference-counted, and
  // set_mesh/reset_mesh atomically swap it. A parallel cache keyed by
  // (objectIndex, volumeIndex) would (a) duplicate the libslic3r ownership
  // model, (b) require its own invalidation wiring on every model mutation
  // (load/cut/boolean/simplify/drill/orient/arrange), and (c) risk diverging
  // from the live mesh after a mutation if a cache-flush is missed. The
  // accessor reads the live m_mesh every call (O(1) pointer dereference), so
  // there is no construction cost to amortize. If a future phase proves that
  // hot-path callers (e.g. per-mouse-move raycast in Phase 113) need a stable
  // snapshot independent of mutations, they hold the returned shared_ptr for
  // the duration of their use -- that is the cache, and it is caller-owned.
  //
  // == DEFENSIVE NULL RETURN (MI-05) ==========================================
  // Returns nullptr for: no model loaded; objectIndex out of range; object
  // null; volumeIndex out of range; volume null; volume mesh empty. Callers
  // MUST null-check before dereferencing. No exceptions, no crashes.
  //
  // == SURFACE FEATURE BOUNDARY (MI-06 / pitfall 6, FUTURE Phase 113/114) =====
  // This accessor returns the ITS ONLY. It does NOT return libslic3r
  // SurfaceFeature instances. SurfaceFeature (Measure.hpp:27-99) carries raw
  // `void* volume` (Measure.hpp:95) and `std::vector<int>* plane_indices`
  // (Measure.hpp:96) that point at libslic3r-owned memory. These raw pointers
  // MUST NOT escape into Qt -- Phase 113 (raycaster) and Phase 114 (Measuring)
  // are responsible for scrubbing/repointing them to Qt-owned handles at the
  // boundary (e.g. cast a Qt volume index to void* for SurfaceFeature::volume,
  // never hand through a libslic3r ModelVolume*). This accessor flags that
  // boundary concern in its contract so the downstream phases cannot miss it.
  std::shared_ptr<const indexed_triangle_set> volumeMeshIts(int objectIndex, int volumeIndex) const;

  // == Phase 120 (PAINT-01): per-volume TriangleMesh accessor =================
  // Returns the per-volume TriangleMesh for the ModelVolume at (objectIndex,
  // volumeIndex), as a std::shared_ptr<const Slic3r::TriangleMesh>.
  //
  // TriangleSelector's ctor takes `const TriangleMesh&` (TriangleSelector.hpp:
  // 299) and stores it as a reference for its whole lifetime
  // (TriangleSelector.hpp:477 m_mesh). So the TriangleMesh MUST outlive every
  // TriangleSelector that references it. This accessor returns the SAME
  // shared_ptr<const TriangleMesh> ModelVolume already owns (mesh_ptr(),
  // Model.hpp:856 -- the aliasing target is the TriangleMesh itself, NOT its
  // `its` member like volumeMeshIts above). The shared_ptr refcount therefore
  // keeps the TriangleMesh alive as long as any PaintEngine selector holds it
  // -- even if the ModelVolume is deleted mid-use (load/cut/boolean/etc.
  // drop the volume, but the prior shared_ptr stays valid until released).
  //
  // Mirrors the volumeMeshIts ownership contract (MI-02/MI-03 shallow-share,
  // MI-04 no parallel cache, MI-05 defensive nullptr). The only difference is
  // the aliasing target: &mesh (this accessor) vs &mesh.its (volumeMeshIts).
  // Both share the SAME refcount on ModelVolume::m_mesh.
  //
  // Returns nullptr for: no model loaded; objectIndex/volumeIndex out of
  // range; volume null; mesh empty. Callers MUST null-check.
  std::shared_ptr<const Slic3r::TriangleMesh> volumeMeshTriangleMesh(int objectIndex, int volumeIndex) const;
#endif

signals:
  void projectChanged();
  void plateDataLoaded(int plateCount);
  void plateSelectionChanged();
  void loadingChanged();
  void loadProgressChanged();
  void loadProgressUpdated(int progress, const QString &stageText);
  void loadFinished(bool success, const QString &message);
  /// Emitted when a 3MF project loads with embedded config (对齐上游 Plater::priv::load_config_file)
  void projectConfigLoaded(const QHash<QString, QVariant> &config);
  /// Phase 145 (EMB-03): async emboss result delivery. Fires on the GUI thread
  /// after addTextVolumeAsync's worker completes successfully. objectIndex is
  /// the target object; volumeName is the (possibly truncated) text label.
  void embossVolumeAdded(int objectIndex, const QString &volumeName);
  /// Phase 145 (EMB-03): async emboss failure delivery. Fires on the GUI thread
  /// after addTextVolumeAsync's worker fails (font load, empty shapes, mesh
  /// generation, or cancellation).
  void embossVolumeFailed(const QString &reason);

private:
  /// v2.4: 当前项目保存路径（saveProjectAs 后更新）
  QString currentProjectPath_;
  /// Phase 144 (EMB-01/02): emboss font path + height/depth. Set by the
  /// viewmodel from EditorViewModel Q_PROPERTYs before addTextVolume runs.
  /// Empty path = fall back to a known system font. Defaults preserve the
  /// pre-Phase-144 behavior (10mm height, 2mm depth).
  std::string m_embossFontPath;
  float m_embossHeight = 10.0f;
  float m_embossDepth = 2.0f;
  /// Phase 145 (EMB-03): in-flight async emboss cancellation flag. A new
  /// addTextVolumeAsync invocation sets this to true on the prior flag (if any),
  /// then installs a fresh flag for itself. The worker polls via load().
  /// Mirrors the activeCancelFlag_ pattern used for model loading.
  std::shared_ptr<std::atomic_bool> m_embossCancelFlag;

  /// Phase 155 (CLOS-02): attach upstream `TextConfiguration` to a freshly
  /// created emboss volume so store_bbs_3mf writes the `<slic3rpe:text>` block.
  /// Called from both the sync performEmbossVolumeAdd path and the async
  /// addTextVolumeAsync GUI-thread result handler — kept in one place so the
  /// two paths persist identical metadata. `volume` is a ModelVolume* (opaque
  /// in the header; full type only needed in the .cpp). `depth` is recorded
  /// only into the volume name suffix as an in-session affordance — depth is
  /// a projection property upstream, not a FontProp field, so it round-trips
  /// via the mesh Z extent (geometry already persisted as MODEL_PART).
  void attachEmbossMetadata(void *volume, const QString &text,
                            const std::string &fontPath, float height, float depth);
  /// Mock-mode per-object scoped overrides (objectIndex → key-value map)
  QHash<int, QHash<QString, QVariant>> m_mockObjectOverrides;
  /// Mock-mode per-volume scoped overrides ((objectIndex << 16 | volumeIndex) → key-value map)
  QHash<int, QHash<QString, QVariant>> m_mockVolumeOverrides;
  /// QVariant adaptation layer for per-plate scoped overrides (plateIndex → key→value).
  /// The plate config truth (D-04) is the native DynamicPrintConfig on PartPlate; this map
  /// is the QML/QVariant-facing view ProjectServiceMock exposes (setPlateScopedOptionValue etc.).
  /// Phase 18 (3MF persistence) and Phase 19 (slice merge) bridge this with PartPlate::config().
  QHash<int, QHash<QString, QVariant>> m_mockPlateOverrides;
  /// Mock-mode per-object volume data (objectIndex → list of MockVolumeEntry)
  QHash<int, QList<MockVolumeEntry>> m_mockVolumes;
  /// Mock-mode per-object layer ranges (objectIndex → list of MockLayerRange)
  QHash<int, QList<MockLayerRange>> m_mockLayerRanges;

  QString projectName_ = tr("未命名项目");
  int modelCount_ = 0;
  QString lastError_;
  QString sourceFilePath_;
  QStringList objectNames_;
  QStringList objectModuleNames_;
  QList<bool> objectPrintableStates_;
  QList<bool> objectVisibleStates_;
  /// v3.0 Phase 16 (D-05): single source of truth for ALL plate state.
  /// Replaces the previous parallel QList vectors (plateCount_, plateNames_,
  /// plateObjectIndices_, plateLockedStates_, plateBedTypes_, platePrintSequences_,
  /// plateSpiralModes_, plateFirstLayerSeq*, plateOtherLayersSeq*, m_mockPlateOverrides).
  std::unique_ptr<OWzx::PartPlateList> m_plateList;
  /// Source-compat alias for the layer-sequence entry (now defined in PartPlate.h as
  /// OWzx::LayerSeqEntry). Kept so untouched .cpp call sites compile.
  using MockLayerSeqEntry = OWzx::LayerSeqEntry;
  /// Per-object transforms (Mock mode)
  QList<QVector3D> objectPositions_;   // (x, y, z) in mm
  QList<QVector3D> objectRotations_;   // (x, y, z) in degrees
  QList<QVector3D> objectScales_;      // (x, y, z) factors
  // ASM-01 (Phase 138): per-instance assemble transforms (Mock mode). Mirror of the
  // upstream ModelInstance::m_assemble_transformation (Model.hpp:1253). Used only on
  // the non-lib path; under HAS_LIBSLIC3R these are kept in sync with the real Model
  // field for consistency with the ordinary accessor pattern.
  QList<QVector3D> assembleOffsets_;   // (x, y, z) in mm
  QList<QVector3D> assembleRotations_; // (x, y, z) in degrees
  QList<QVector3D> assembleScales_;    // (x, y, z) factors
  int loadProgress_ = 0;
  bool loading_ = false;

  std::shared_ptr<std::atomic_bool> activeCancelFlag_;

  // v3.0 Phase 18 (D-12): staging buffers for per-plate state captured during async
  // 3MF load (PlateData lives in the read lambda; m_plateList is rebuilt in a later
  // lambda). These carry locked/bed-type/print-seq/spiral across the two lambdas so
  // the rebuild restores ALL plate state. Cleared before each load, applied during rebuild.
  QList<bool> pendingPlateLocked_;
  QList<int> pendingPlateBedType_;
  QList<int> pendingPlatePrintSeq_;
  QList<int> pendingPlateSpiral_;
  // v3.2 Phase 31 (FMAP-02): per-plate filament maps + mode extracted from 3MF
  // PlateData during load; applied to PartPlate in the rebuild.
  QList<QList<int>> pendingPlateFilamentMaps_;
  QList<int> pendingPlateFilamentMapMode_;
  // v3.2 Phase 30 (THUMB-02): per-plate thumbnails extracted from 3MF
  // PlateData::plate_thumbnail during load; applied to PartPlate in the rebuild.
  QList<QImage> pendingPlateThumbnails_;

#ifdef HAS_LIBSLIC3R
  // Forward-declared to keep libslic3r/Config.hpp out of the header (header pollution).
  class DynamicPrintConfig;
  Slic3r::Model *model_ = nullptr;
#endif
};
