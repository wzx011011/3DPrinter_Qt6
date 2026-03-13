#pragma once

#include <QObject>
#include <QList>
#include <QSet>
#include <QString>
#include <QByteArray>
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
  Q_INVOKABLE void requestSelectionSettings();
  Q_INVOKABLE QString plateName(int i) const;
  Q_INVOKABLE int plateObjectCount(int i) const;
  Q_INVOKABLE int objectPlateIndex(int i) const;
  Q_INVOKABLE QString objectPlateName(int i) const;
  Q_INVOKABLE bool setCurrentPlateIndex(int i);
  Q_INVOKABLE void setShowAllObjects(bool showAll);
  Q_INVOKABLE void setObjectOrganizeMode(int mode);

  // Slice-progress bridge (delegates to SliceService)
  Q_PROPERTY(QString sliceStatusLabel READ sliceStatusLabel NOTIFY stateChanged)
  Q_PROPERTY(QString sliceEstimatedTime READ sliceEstimatedTime NOTIFY stateChanged)
  Q_PROPERTY(QString sliceOutputPath READ sliceOutputPath NOTIFY stateChanged)
  Q_PROPERTY(QString sliceResultWeight READ sliceResultWeight NOTIFY stateChanged)
  Q_PROPERTY(QString sliceResultPlateLabel READ sliceResultPlateLabel NOTIFY stateChanged)
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
  bool hasSliceResult() const;
  bool canRequestSlice() const;
  QString sliceActionLabel() const;
  QString sliceActionHint() const;

  Q_INVOKABLE void requestSlice();
  Q_INVOKABLE void requestSliceAll();
  Q_INVOKABLE void cancelSlice();
  Q_INVOKABLE bool requestExportGCode(const QString &targetPath);
  /// 加载模型文件 (3MF/STL/OBJ)
  Q_INVOKABLE bool loadFile(const QString &filePath);
  /// 清空当前场景与项目状态（用于顶部工具栏新建）
  Q_INVOKABLE void clearWorkspace();

signals:
  void stateChanged();
  void selectionSettingsRequested();

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
  int m_primarySelectedSourceIndex = -1;
  int m_selectedVolumeObjectSourceIndex = -1;
  QSet<int> m_selectedVolumeIndices;
  int m_selectedVolumeIndex = -1;
  bool m_showAllObjects = false;
  int m_objectOrganizeMode = 0;
  QString m_sliceEstimatedTime;
  int m_sliceResultPlateIndex = -1;
  QVector4D m_fitHint; ///< (cx, cy, cz, radius) in GL coords; zero = invalid
  QByteArray m_cachedMeshData;
  QList<int> m_sliceAllQueue; ///< plate indices queued for Slice All
  bool m_slicingAll = false;
};
