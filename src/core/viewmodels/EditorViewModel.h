#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QByteArray>
#include <QVector4D>

class ProjectServiceMock;
class SliceServiceMock;

class EditorViewModel final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString projectName READ projectName NOTIFY stateChanged)
  Q_PROPERTY(int modelCount READ modelCount NOTIFY stateChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
  // Object-list panel support
  Q_PROPERTY(int objectCount READ objectCount NOTIFY stateChanged)
  Q_PROPERTY(int selectedObjectIndex READ selectedObjectIndex NOTIFY stateChanged)
  /// 模型网格数据（TLV 格式，用于 GLViewport 渲染）
  Q_PROPERTY(QByteArray meshData READ meshData NOTIFY stateChanged)
  /// 加载完成后的相机适应提示: (cx, cy, cz, radius)，全零表示无效
  Q_PROPERTY(QVector4D fitHint READ fitHint NOTIFY stateChanged)

public:
  explicit EditorViewModel(ProjectServiceMock *projectService, SliceServiceMock *sliceService, QObject *parent = nullptr);

  QString projectName() const;
  int modelCount() const;
  QString statusText() const;
  QByteArray meshData() const;
  QVector4D fitHint() const { return m_fitHint; }

  // Object list accessors (safe Q_INVOKABLE — no QVariantList)
  int objectCount() const;
  int selectedObjectIndex() const;
  Q_INVOKABLE QString objectName(int i) const;
  Q_INVOKABLE bool objectVisible(int i) const;
  Q_INVOKABLE void setObjectVisible(int i, bool visible);
  Q_INVOKABLE void deleteObject(int i);
  Q_INVOKABLE void selectObject(int i);

  // Slice-progress bridge (delegates to SliceServiceMock)
  Q_INVOKABLE int sliceProgress() const;
  Q_INVOKABLE bool isSlicing() const;

  Q_INVOKABLE void importMockModel();
  Q_INVOKABLE void requestSlice();
  /// 加载模型文件 (3MF/STL/OBJ)
  Q_INVOKABLE bool loadFile(const QString &filePath);

signals:
  void stateChanged();

private:
  struct ObjectEntry
  {
    QString name;
    bool visible = true;
  };

  ProjectServiceMock *projectService_ = nullptr;
  SliceServiceMock *sliceService_ = nullptr;
  QString statusText_ = tr("就绪");
  QList<ObjectEntry> m_objects;
  int m_selectedObjectIndex = -1;
  QVector4D m_fitHint; ///< (cx, cy, cz, radius) in GL coords; zero = invalid
};
