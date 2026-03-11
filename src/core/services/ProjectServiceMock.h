#pragma once

#include <QObject>
#include <QVariant>
#include <QStringList>
#include <QList>
#include <QByteArray>
#include <atomic>
#include <memory>

#ifdef HAS_LIBSLIC3R
namespace Slic3r
{
  class Model;
}
#endif

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
  Q_INVOKABLE QList<int> currentPlateObjectIndices() const;
  Q_INVOKABLE int plateObjectCount(int index) const;
  Q_INVOKABLE int plateIndexForObject(int objectIndex) const;
  Q_INVOKABLE bool objectPrintable(int index) const;
  Q_INVOKABLE bool setObjectPrintable(int index, bool printable);
  Q_INVOKABLE int objectVolumeCount(int index) const;
  Q_INVOKABLE QString objectVolumeName(int objectIndex, int volumeIndex) const;
  Q_INVOKABLE QString objectVolumeTypeLabel(int objectIndex, int volumeIndex) const;
  QVariant scopedOptionValue(int objectIndex, int volumeIndex, const QString &key, const QVariant &fallbackValue = QVariant()) const;
  bool setScopedOptionValue(int objectIndex, int volumeIndex, const QString &key, const QVariant &value);
  Q_INVOKABLE bool deleteObjectVolume(int objectIndex, int volumeIndex);
  Q_INVOKABLE bool deleteObject(int index);

  /**
   * 提取所有三角面顶点为紧凑 float 数组 (x,y,z per vertex, 3 verts per face)。
   * 坐标系转换：libslic3r Z-up → GL Y-up：GL(x,z,y)
   */
  QByteArray meshData() const;

  /// 清空当前项目（新建项目/重置场景）
  Q_INVOKABLE void clearProject();

signals:
  void projectChanged();
  void plateDataLoaded(int plateCount);
  void plateSelectionChanged();
  void loadingChanged();
  void loadProgressChanged();
  void loadProgressUpdated(int progress, const QString &stageText);
  void loadFinished(bool success, const QString &message);

private:
  QString projectName_ = tr("未命名项目");
  int modelCount_ = 0;
  int plateCount_ = 0;
  int currentPlateIndex_ = -1;
  QString lastError_;
  QString sourceFilePath_;
  QStringList objectNames_;
  QStringList objectModuleNames_;
  QList<bool> objectPrintableStates_;
  QStringList plateNames_;
  QList<QList<int>> plateObjectIndices_;
  int loadProgress_ = 0;
  bool loading_ = false;

  std::shared_ptr<std::atomic_bool> activeCancelFlag_;

#ifdef HAS_LIBSLIC3R
  Slic3r::Model *model_ = nullptr;
#endif
};
