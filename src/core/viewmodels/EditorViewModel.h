#pragma once

#include <QObject>
#include <QList>
#include <QSet>
#include <QString>
#include <QByteArray>
#include <QVector3D>
#include <QVector4D>

#include "core/rendering/SupportPaintTypes.h"

class ProjectServiceMock;
class SliceService;
class UndoRedoManager;

class EditorViewModel final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString projectName READ projectName NOTIFY stateChanged)
  Q_PROPERTY(int modelCount READ modelCount NOTIFY stateChanged)
  Q_PROPERTY(int plateCount READ plateCount NOTIFY stateChanged)
  Q_PROPERTY(int currentPlateIndex READ currentPlateIndex NOTIFY stateChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
  Q_PROPERTY(int loadProgress READ loadProgress NOTIFY stateChanged)
  Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
  Q_PROPERTY(bool showAllObjects READ showAllObjects NOTIFY stateChanged)
  Q_PROPERTY(int objectOrganizeMode READ objectOrganizeMode NOTIFY stateChanged)
  Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY stateChanged)
  Q_PROPERTY(bool hasSelectedVolume READ hasSelectedVolume NOTIFY stateChanged)
  Q_PROPERTY(int selectedVolumeCount READ selectedVolumeCount NOTIFY stateChanged)
  Q_PROPERTY(bool canOpenSelectionSettings READ canOpenSelectionSettings NOTIFY stateChanged)
  Q_PROPERTY(QString settingsTargetType READ settingsTargetType NOTIFY stateChanged)
  Q_PROPERTY(QString settingsTargetName READ settingsTargetName NOTIFY stateChanged)
  Q_PROPERTY(int settingsTargetObjectIndex READ settingsTargetObjectIndex NOTIFY stateChanged)
  Q_PROPERTY(int settingsTargetVolumeIndex READ settingsTargetVolumeIndex NOTIFY stateChanged)
  // Object-list panel support
  Q_PROPERTY(int objectCount READ objectCount NOTIFY stateChanged)
  Q_PROPERTY(int selectedObjectIndex READ selectedObjectIndex NOTIFY stateChanged)
  Q_PROPERTY(int selectedObjectCount READ selectedObjectCount NOTIFY stateChanged)
  /// 模型网格数据（TLV 格式，用于 GLViewport 渲染）
  Q_PROPERTY(QByteArray meshData READ meshData NOTIFY stateChanged)
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

public:
  explicit EditorViewModel(ProjectServiceMock *projectService, SliceService *sliceService, QObject *parent = nullptr);

  QString projectName() const;
  int modelCount() const;
  int plateCount() const;
  int currentPlateIndex() const;
  QString statusText() const;
  int loadProgress() const;
  bool loading() const;
  bool showAllObjects() const;
  int objectOrganizeMode() const;
  bool hasSelection() const;
  bool hasSelectedVolume() const;
  int selectedVolumeCount() const;
  bool canOpenSelectionSettings() const;
  QString settingsTargetType() const;
  QString settingsTargetName() const;
  int settingsTargetObjectIndex() const;
  int settingsTargetVolumeIndex() const;
  QByteArray meshData() const;
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
  void setConnectorSize(float v);
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
  float supportPaintAngleThreshold() const;
  void setSupportPaintAngleThreshold(float angle);
  float supportPaintSmartFillAngle() const;
  void setSupportPaintSmartFillAngle(float angle);
  float supportPaintGapArea() const;
  void setSupportPaintGapArea(float area);
  bool supportPaintOnOverhangsOnly() const;
  void setSupportPaintOnOverhangsOnly(bool on);
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
  int enforcedSupportCount() const;
  int blockedSupportCount() const;
  int totalPaintedTriangleCount() const;
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
  int hollowSelectedHoleCount() const;
  Q_INVOKABLE void deleteSelectedHollowPoints();
  // Simplify gizmo (对齐上游 GLGizmoSimplify)
  int simplifyWantedCount() const;
  void setSimplifyWantedCount(int count);
  float simplifyMaxError() const;
  void setSimplifyMaxError(float error);
  Q_INVOKABLE bool simplifySelected();
  Q_INVOKABLE int selectedObjectTriangleCount() const;
  // MMU segmentation gizmo (对齐上游 GLGizmoMmuSegmentation)
  int mmuSelectedExtruder() const;
  void setMmuSelectedExtruder(int idx);
  int mmuExtruderCount() const;
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
  Q_INVOKABLE bool embossSelected();

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

  /// ── FaceDetector gizmo properties (对齐上游 GLGizmoFaceDetector) ──
  float faceDetectorAngle() const;
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

  void setObjectPosX(float v);
  void setObjectPosY(float v);
  void setObjectPosZ(float v);
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
  int selectedObjectIndex() const;
  int selectedObjectCount() const;
  Q_INVOKABLE QString objectName(int i) const;
  Q_INVOKABLE QString objectModuleName(int i) const;
  Q_INVOKABLE QString objectGroupLabel(int i) const;
  Q_INVOKABLE int objectGroupCount(int i) const;
  Q_INVOKABLE bool objectGroupExpanded(int i) const;
  Q_INVOKABLE void toggleObjectGroupExpanded(int i);
  Q_INVOKABLE bool objectExpanded(int i) const;
  Q_INVOKABLE void toggleObjectExpanded(int i);
  Q_INVOKABLE int objectVolumeCount(int i) const;
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
  Q_INVOKABLE bool addPrimitive(int objectIndex, int primitiveType);
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
  /// 扁平平放（对齐上游 GLGizmoFlatten）— 将选中对象最大面朝下
  Q_INVOKABLE void flattenSelected();
  /// 切割选中对象（对齐上游 GLGizmoCut）— 沿指定轴/位置切割
  Q_INVOKABLE void cutSelected(int axis, double position);
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
  Q_PROPERTY(float connectorDepth READ connectorDepth WRITE setConnectorDepth NOTIFY stateChanged)
  /// 测量拾取模式（对齐上游 GLGizmoMeasure feature/point selection）
  Q_PROPERTY(int measureSelectionMode READ measureSelectionMode WRITE setMeasureSelectionMode NOTIFY stateChanged)
  /// 支撑绘制设置（对齐上游 GLGizmoFdmSupports）
  Q_PROPERTY(int supportPaintTool READ supportPaintTool WRITE setSupportPaintTool NOTIFY stateChanged)
  Q_PROPERTY(int supportPaintCursorType READ supportPaintCursorType WRITE setSupportPaintCursorType NOTIFY stateChanged)
  Q_PROPERTY(int supportPaintToolType READ supportPaintToolType WRITE setSupportPaintToolType NOTIFY stateChanged)
  Q_PROPERTY(float supportPaintCursorRadius READ supportPaintCursorRadius WRITE setSupportPaintCursorRadius NOTIFY stateChanged)
  Q_PROPERTY(float supportPaintAngleThreshold READ supportPaintAngleThreshold WRITE setSupportPaintAngleThreshold NOTIFY stateChanged)
  Q_PROPERTY(float supportPaintSmartFillAngle READ supportPaintSmartFillAngle WRITE setSupportPaintSmartFillAngle NOTIFY stateChanged)
  Q_PROPERTY(float supportPaintGapArea READ supportPaintGapArea WRITE setSupportPaintGapArea NOTIFY stateChanged)
  Q_PROPERTY(bool supportPaintOnOverhangsOnly READ supportPaintOnOverhangsOnly WRITE setSupportPaintOnOverhangsOnly NOTIFY stateChanged)
  Q_PROPERTY(bool supportEnable READ supportEnable WRITE setSupportEnable NOTIFY stateChanged)
  Q_PROPERTY(int supportType READ supportType WRITE setSupportType NOTIFY stateChanged)
  /// 支撑绘制数据（对齐上游 GLGizmoFdmSupports paint state）
  Q_PROPERTY(int enforcedSupportCount READ enforcedSupportCount NOTIFY paintDataChanged)
  Q_PROPERTY(int blockedSupportCount READ blockedSupportCount NOTIFY paintDataChanged)
  Q_PROPERTY(int totalPaintedTriangleCount READ totalPaintedTriangleCount NOTIFY paintDataChanged)
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
  Q_PROPERTY(int hollowSelectedHoleCount READ hollowSelectedHoleCount NOTIFY stateChanged)
  /// 网格简化设置（对齐上游 GLGizmoSimplify）
  Q_PROPERTY(int simplifyWantedCount READ simplifyWantedCount WRITE setSimplifyWantedCount NOTIFY stateChanged)
  Q_PROPERTY(float simplifyMaxError READ simplifyMaxError WRITE setSimplifyMaxError NOTIFY stateChanged)
  /// MMU 多耗材分段设置（对齐上游 GLGizmoMmuSegmentation）
  Q_PROPERTY(int mmuSelectedExtruder READ mmuSelectedExtruder WRITE setMmuSelectedExtruder NOTIFY stateChanged)
  Q_PROPERTY(int mmuExtruderCount READ mmuExtruderCount NOTIFY stateChanged)
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
  /// MeshBoolean 设置（对齐上游 GLGizmoMeshBoolean）
  Q_PROPERTY(int booleanOperation READ booleanOperation WRITE setBooleanOperation NOTIFY stateChanged)
  /// AdvancedCut 设置（对齐上游 GLGizmoAdvancedCut）
  Q_PROPERTY(int advCutAxis READ advCutAxis WRITE setAdvCutAxis NOTIFY stateChanged)
  Q_PROPERTY(float advCutPosition READ advCutPosition WRITE setAdvCutPosition NOTIFY stateChanged)
  Q_PROPERTY(bool advCutKeepBoth READ advCutKeepBoth WRITE setAdvCutKeepBoth NOTIFY stateChanged)
  Q_PROPERTY(bool advCutConnectors READ advCutConnectors WRITE setAdvCutConnectors NOTIFY stateChanged)
  /// FaceDetector 设置（对齐上游 GLGizmoFaceDetector）
  Q_PROPERTY(float faceDetectorAngle READ faceDetectorAngle WRITE setFaceDetectorAngle NOTIFY stateChanged)
  /// Text 设置（对齐上游 GLGizmoText）
  Q_PROPERTY(QString textContent READ textContent WRITE setTextContent NOTIFY stateChanged)
  Q_PROPERTY(float textSize READ textSize WRITE setTextSize NOTIFY stateChanged)
  /// SVG 设置（对齐上游 GLGizmoSVG）
  Q_PROPERTY(QString svgFilePath READ svgFilePath WRITE setSvgFilePath NOTIFY stateChanged)
  Q_PROPERTY(float svgScale READ svgScale WRITE setSvgScale NOTIFY stateChanged)
  /// 扁平可用面数（对齐上游 GLGizmoFlatten 面）
  Q_PROPERTY(int flattenFaceCount READ flattenFaceCount NOTIFY stateChanged)
  /// 翻转切割平面（对齐上游 GLGizmoCut::flip_cut_plane）
  Q_INVOKABLE void flipCutPlane();
  /// 居中切割位置到选中对象中心
  Q_INVOKABLE void centerCutPlane();
  Q_INVOKABLE void requestSelectionSettings();
  Q_INVOKABLE QString plateName(int i) const;
  Q_INVOKABLE int plateObjectCount(int i) const;
  Q_INVOKABLE int objectPlateIndex(int i) const;
  Q_INVOKABLE QString objectPlateName(int i) const;
  Q_INVOKABLE bool setCurrentPlateIndex(int i);
  Q_INVOKABLE void setShowAllObjects(bool showAll);
  Q_INVOKABLE void setObjectOrganizeMode(int mode);
  /// 平板管理（对齐上游 PartPlateList）
  Q_INVOKABLE bool addPlate();
  Q_INVOKABLE bool deletePlate(int plateIndex);
  /// 平板级操作（对齐上游 create_plate_menu）
  Q_INVOKABLE void selectAllOnPlate(int plateIndex);
  Q_INVOKABLE void removeAllOnPlate(int plateIndex);
  Q_INVOKABLE bool renamePlate(int plateIndex, const QString &newName);
  Q_INVOKABLE bool isPlateLocked(int plateIndex) const;
  Q_INVOKABLE void togglePlateLocked(int plateIndex);
  /// 查询指定平板是否有有效切片结果
  Q_INVOKABLE bool isPlateSliced(int plateIndex) const;
  /// 移动选中对象到指定平板（对齐上游 Plater::priv::on_arrange 跨平板拖拽）
  Q_INVOKABLE bool moveSelectedObjectToPlate(int targetPlateIndex);
  /// 平板缩略图颜色（Mock 模式：基于平板对象数生成独特颜色，对齐上游 PartPlate thumbnail）
  Q_INVOKABLE QString plateThumbnailColor(int plateIndex) const;
  /// 重命名对象（对齐上游 Plater::rename_object）
  Q_INVOKABLE bool renameObject(int index, const QString &newName);
  /// 移动对象排序位置（对齐上游 GUI_ObjectList 拖拽重排序）
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
  /// 生成平板缩略图（对齐上游 PartPlate::thumbnail_data）
  /// 返回 base64 PNG 图片供 QML Image 组件使用
  Q_INVOKABLE QString generatePlateThumbnail(int plateIndex, int size = 64);
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
  Q_PROPERTY(int extruderCount READ extruderCount NOTIFY stateChanged)
  Q_PROPERTY(bool hasSliceResult READ hasSliceResult NOTIFY stateChanged)
  Q_PROPERTY(bool canRequestSlice READ canRequestSlice NOTIFY stateChanged)
  Q_PROPERTY(QString sliceActionLabel READ sliceActionLabel NOTIFY stateChanged)
  Q_PROPERTY(QString sliceActionHint READ sliceActionHint NOTIFY stateChanged)
  Q_INVOKABLE int sliceProgress() const;
  Q_INVOKABLE bool isSlicing() const;
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

  bool allPlatesSliced() const;
  int extruderCount() const;
  Q_INVOKABLE QString extruderUsedLength(int extruderId) const;
  Q_INVOKABLE QString extruderUsedWeight(int extruderId) const;
  bool hasSliceResult() const;
  bool canRequestSlice() const;
  QString sliceActionLabel() const;
  QString sliceActionHint() const;

  Q_INVOKABLE void requestSlice();
  Q_INVOKABLE void requestSliceAll();
  Q_INVOKABLE void cancelSlice();
  Q_INVOKABLE bool requestExportGCode(const QString &targetPath);
  /// 请求切换到预览页面（对齐上游 Plater::priv::on_preview）
  Q_INVOKABLE void switchToPreview();
  /// 加载模型文件 (3MF/STL/OBJ)
  Q_INVOKABLE bool loadFile(const QString &filePath);
  /// 清空当前场景与项目状态（用于顶部工具栏新建）
  Q_INVOKABLE void clearWorkspace();
  /// JSON 项目加载后刷新 UI 状态（供 BackendContext 调用）
  void refreshAfterLoad();

  /// Undo/Redo integration (对齐上游 UndoRedo 框架)
  void setUndoRedoManager(UndoRedoManager *manager);
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

signals:
  void stateChanged();
  void paintDataChanged();
  void bedShapeChanged();
  void selectionSettingsRequested();
  /// 请求切换到预览页面（对齐上游 Plater::priv::on_preview）
  void previewRequested();

private:
  void refreshMeshCacheAndFitHint();
  void rebuildObjectEntriesFromService();
  bool deleteSelectedVolumesBySource();
  void ensureValidObjectSelection(bool preferFirstVisible);
  QList<int> baseVisibleObjectIndices() const;
  QString sourceObjectGroupLabel(int sourceIndex) const;
  QString sourceObjectGroupKey(int sourceIndex) const;
  int mapFilteredToSourceIndex(int filteredIndex) const;
  QList<int> visibleObjectIndices() const;
  bool currentPlateHasPrintableObjects() const;
  void checkViewportWarnings();
  void continueSliceAllQueue();

  struct ObjectEntry
  {
    QString name;
    bool printable = true;
  };

  ProjectServiceMock *projectService_ = nullptr;
  SliceService *sliceService_ = nullptr;
  UndoRedoManager *m_undoManager = nullptr;
  QString statusText_ = tr("就绪");
  QList<ObjectEntry> m_objects;
  QSet<int> m_selectedSourceIndices;
  QSet<QString> m_collapsedGroupKeys;
  QSet<int> m_collapsedObjectSourceIndices;
  QList<ObjectEntry> m_clipboard; ///< 剪贴板缓冲区
  int m_primarySelectedSourceIndex = -1;
  int m_selectedVolumeObjectSourceIndex = -1;
  QSet<int> m_selectedVolumeIndices;
  int m_selectedVolumeIndex = -1;
  bool m_showAllObjects = false;
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
  // Cut connector (对齐上游 GLGizmoCut connector settings)
  int m_cutMode = 0;              ///< 0=Planar, 1=TongueAndGroove
  int m_connectorType = 0;        ///< 0=Plug, 1=Dowel, 2=Snap
  int m_connectorStyle = 0;       ///< 0=Prism, 1=Frustum
  int m_connectorShape = 3;       ///< 0=Triangle, 1=Square, 2=Hexagon, 3=Circle
  float m_connectorSize = 5.0f;   ///< 连接器尺寸 mm
  float m_connectorDepth = 0.5f;  ///< 深度比 0-1
  // Measure selection (对齐上游 GLGizmoMeasure)
  int m_measureSelectionMode = 0; ///< 0=Default point, 1=Feature selection
  QByteArray m_cachedMeshData;
  QList<int> m_sliceAllQueue; ///< plate indices queued for Slice All
  bool m_slicingAll = false;
  QSet<int> m_slicedPlateIndices; ///< tracks which plates have valid slice results
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
  int m_supportPaintTool = 1;              ///< 0=None, 1=Enforcer, 2=Blocker
  int m_supportPaintCursorType = 0;        ///< 0=Circle, 1=Sphere, 2=Pointer, 3=HeightRange, 4=GapFill
  int m_supportPaintToolType = 0;          ///< 0=Brush, 1=BucketFill, 2=SmartFill, 3=GapFill
  float m_supportPaintCursorRadius = 5.0f; ///< Brush radius in mm
  float m_supportPaintAngleThreshold = 45.0f; ///< Overhang highlight angle
  float m_supportPaintSmartFillAngle = 30.0f; ///< Smart fill angle threshold
  float m_supportPaintGapArea = 1.0f;      ///< Gap fill area threshold
  bool m_supportPaintOnOverhangsOnly = false; ///< Restrict painting to overhangs
  bool m_supportEnable = false;            ///< Support enabled flag
  int m_supportType = 0;                   ///< 0=normal, 1=tree
  QList<Crality3D::ObjectPaintData> m_paintData;  ///< 对齐上游 per-volume paint data
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
  int m_hollowSelectedHoleCount = 0;      ///< selected drain holes count
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
  // MeshBoolean (对齐上游 GLGizmoMeshBoolean)
  int m_booleanOperation = 1;             ///< 0=union, 1=diff, 2=intersect
  // AdvancedCut (对齐上游 GLGizmoAdvancedCut)
  int m_advCutAxis = 2;                  ///< 0=X, 1=Y, 2=Z
  float m_advCutPosition = 0.0f;
  bool m_advCutKeepBoth = true;
  bool m_advCutConnectors = false;
  // FaceDetector (对齐上游 GLGizmoFaceDetector)
  float m_faceDetectorAngle = 5.0f;
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
};
