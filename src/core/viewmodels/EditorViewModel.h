#pragma once

#include <QObject>
#include <QList>
#include <QSet>
#include <QString>
#include <QByteArray>
#include <QVector3D>
#include <QVector4D>

class ProjectServiceMock;
class SliceService;

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
  /// 挤出机耗材用量（对齐上游 SliceInfoPanel per-extruder filament breakdown）
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

signals:
  void stateChanged();
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
  void continueSliceAllQueue();

  struct ObjectEntry
  {
    QString name;
    bool printable = true;
  };

  ProjectServiceMock *projectService_ = nullptr;
  SliceService *sliceService_ = nullptr;
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
  QByteArray m_cachedMeshData;
  QList<int> m_sliceAllQueue; ///< plate indices queued for Slice All
  bool m_slicingAll = false;
  QSet<int> m_slicedPlateIndices; ///< tracks which plates have valid slice results
};
