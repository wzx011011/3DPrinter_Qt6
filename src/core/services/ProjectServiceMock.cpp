#include "ProjectServiceMock.h"

ProjectServiceMock::ProjectServiceMock(QObject *parent)
    : QObject(parent)
{
}

QString ProjectServiceMock::projectName() const
{
  return projectName_;
}

int ProjectServiceMock::modelCount() const
{
  return modelCount_;
}

void ProjectServiceMock::importMockModel()
{
  modelCount_++;
  emit projectChanged();
}
