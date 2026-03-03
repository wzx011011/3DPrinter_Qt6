#pragma once

#include <QObject>

class ProjectServiceMock final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString projectName READ projectName NOTIFY projectChanged)
  Q_PROPERTY(int modelCount READ modelCount NOTIFY projectChanged)

public:
  explicit ProjectServiceMock(QObject *parent = nullptr);

  QString projectName() const;
  int modelCount() const;

  Q_INVOKABLE void importMockModel();

signals:
  void projectChanged();

private:
  QString projectName_ = tr("未命名项目");
  int modelCount_ = 0;
};
