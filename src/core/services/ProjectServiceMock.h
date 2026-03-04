#pragma once

#include <QObject>
#include <QStringList>

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
  Q_PROPERTY(QString lastError READ lastError NOTIFY projectChanged)

public:
  explicit ProjectServiceMock(QObject *parent = nullptr);
  ~ProjectServiceMock();

  QString projectName() const;
  int modelCount() const;
  QString lastError() const;

  /// 加载 3MF/STL/OBJ 等模型文件（真正调用 libslic3r）
  Q_INVOKABLE bool loadFile(const QString &filePath);

  /// 返回已加载的模型对象名称列表
  Q_INVOKABLE QStringList objectNames() const;

  /// Mock 导入（保留向后兼容）
  Q_INVOKABLE void importMockModel();

signals:
  void projectChanged();

private:
  QString projectName_ = tr("未命名项目");
  int modelCount_ = 0;
  QString lastError_;
  QStringList objectNames_;

#ifdef HAS_LIBSLIC3R
  Slic3r::Model *model_ = nullptr;
#endif
};
