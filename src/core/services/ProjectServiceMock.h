#pragma once

#include <QObject>
#include <QVariant>
#include <QStringList>
#include <QList>
#include <QByteArray>
#include <QVector3D>
#include <atomic>
#include <memory>

#ifdef HAS_LIBSLIC3R
namespace Slic3r
{
  class Model;
}
#endif

/// Volume type enum (对齐上游 ModelVolumeType, ModelCommon.hpp)
enum class MockVolumeType {
  ModelPart = 0,
  NegativeVolume = 1,
  ParameterModifier = 2,
  SupportBlocker = 3,
  SupportEnforcer = 4
};

/// Mock volume data entry
struct MockVolumeEntry {
  QString name;
  MockVolumeType type = MockVolumeType::ModelPart;
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
  QString lastError() const;
  int loadProgress() const;
  bool loading() const;
  QString sourceFilePath() const;

#ifdef HAS_LIBSLIC3R
  std::unique_ptr<Slic3r::Model> cloneCurrentPlateModel() const;
#endif

  /// 加载 3MF/STL/OBJ 等模型文件（真正调用 libslic3r）
  Q_INVOKABLE bool loadFile(const QString &filePath);
  Q_INVOKABLE void cancelLoad();

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
  Q_INVOKABLE QString objectVolumeName(int objectIndex, int volumeIndex) const;
  Q_INVOKABLE QString objectVolumeTypeLabel(int objectIndex, int volumeIndex) const;
  QVariant scopedOptionValue(int objectIndex, int volumeIndex, const QString &key, const QVariant &fallbackValue = QVariant()) const;
  bool setScopedOptionValue(int objectIndex, int volumeIndex, const QString &key, const QVariant &value);
  /// Plate-level scoped overrides (对齐上游 PartPlate config override)
  QVariant plateScopedOptionValue(int plateIndex, const QString &key, const QVariant &fallbackValue = QVariant()) const;
  bool setPlateScopedOptionValue(int plateIndex, const QString &key, const QVariant &value);
  Q_INVOKABLE bool deleteObjectVolume(int objectIndex, int volumeIndex);
  /// Add a new volume to the specified object (对齐上游 GUI_ObjectList::load_subobject / load_generic_subobject)
  /// volumeType: 0=ModelPart, 1=NegativeVolume, 2=ParameterModifier, 3=SupportBlocker, 4=SupportEnforcer
  Q_INVOKABLE bool addVolume(int objectIndex, int volumeType);
  /// Change volume type (对齐上游 GUI_ObjectList::load_generic_subobject type conversion)
  Q_INVOKABLE bool changeVolumeType(int objectIndex, int volumeIndex, int newVolumeType);

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

  /// 复制指定对象（对齐上游 Plater::clone_selection 行为）
  /// 返回新对象的索引，失败返回 -1
  Q_INVOKABLE int duplicateObject(int sourceIndex);
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

  /// 清空当前项目（新建项目/重置场景）
  Q_INVOKABLE void clearProject();
  /// 加载项目元数据（对齐上游 Plater::load_project，Mock 模式从 JSON 恢复）
  Q_INVOKABLE bool loadProject(const QString &filePath);
  /// 保存项目元数据（对齐上游 MainFrame::do_save / Plater::save_project）
  /// Mock 模式保存为 JSON；真实模式调用 3MF writer
  Q_INVOKABLE bool saveProject(const QString &filePath);

signals:
  void projectChanged();
  void plateDataLoaded(int plateCount);
  void plateSelectionChanged();
  void loadingChanged();
  void loadProgressChanged();
  void loadProgressUpdated(int progress, const QString &stageText);
  void loadFinished(bool success, const QString &message);

private:
  /// Mock-mode per-object scoped overrides (objectIndex → key-value map)
  QHash<int, QHash<QString, QVariant>> m_mockObjectOverrides;
  /// Mock-mode per-volume scoped overrides ((objectIndex << 16 | volumeIndex) → key-value map)
  QHash<int, QHash<QString, QVariant>> m_mockVolumeOverrides;
  /// Mock-mode per-plate scoped overrides (plateIndex → key-value map)
  QHash<int, QHash<QString, QVariant>> m_mockPlateOverrides;
  /// Mock-mode per-object volume data (objectIndex → list of MockVolumeEntry)
  QHash<int, QList<MockVolumeEntry>> m_mockVolumes;
  /// Mock-mode per-object layer ranges (objectIndex → list of MockLayerRange)
  QHash<int, QList<MockLayerRange>> m_mockLayerRanges;

  QString projectName_ = tr("未命名项目");
  int modelCount_ = 0;
  int plateCount_ = 0;
  int currentPlateIndex_ = -1;
  QString lastError_;
  QString sourceFilePath_;
  QStringList objectNames_;
  QStringList objectModuleNames_;
  QList<bool> objectPrintableStates_;
  QList<bool> objectVisibleStates_;
  QStringList plateNames_;
  QList<QList<int>> plateObjectIndices_;
  QList<bool> plateLockedStates_;
  /// Per-plate settings (对齐上游 PlateSettingsDialog)
  QList<int> plateBedTypes_;       // 0=Default/PEI, 1=EP, 2=PC, 3=PEI, 4=Custom
  QList<int> platePrintSequences_; // 0=ByLayer, 1=ByObject
  QList<int> plateSpiralModes_;    // 0=Default, 1=On, 2=Off
  /// Per-object transforms (Mock mode)
  QList<QVector3D> objectPositions_;   // (x, y, z) in mm
  QList<QVector3D> objectRotations_;   // (x, y, z) in degrees
  QList<QVector3D> objectScales_;      // (x, y, z) factors
  int loadProgress_ = 0;
  bool loading_ = false;

  std::shared_ptr<std::atomic_bool> activeCancelFlag_;

#ifdef HAS_LIBSLIC3R
  Slic3r::Model *model_ = nullptr;
#endif
};
