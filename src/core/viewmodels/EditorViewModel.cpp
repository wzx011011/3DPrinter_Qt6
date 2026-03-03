#include "EditorViewModel.h"

#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceServiceMock.h"

EditorViewModel::EditorViewModel(ProjectServiceMock *projectService, SliceServiceMock *sliceService, QObject *parent)
    : QObject(parent), projectService_(projectService), sliceService_(sliceService)
{
  // Pre-load two mock objects for UI demo
  m_objects = {
      {QStringLiteral("Benchy.3mf"), true},
      {QStringLiteral("Support_plate.stl"), true},
  };

  connect(projectService_, &ProjectServiceMock::projectChanged, this, [this]()
          {
        statusText_ = QStringLiteral("已更新项目对象");
        emit stateChanged(); });

  connect(sliceService_, &SliceServiceMock::slicingChanged, this, [this]()
          {
        statusText_ = sliceService_->slicing() ? QStringLiteral("切片中...") : QStringLiteral("切片完成");
        emit stateChanged(); });

  connect(sliceService_, &SliceServiceMock::progressChanged, this, &EditorViewModel::stateChanged);
}

QString EditorViewModel::projectName() const { return projectService_->projectName(); }
int EditorViewModel::modelCount() const { return projectService_->modelCount(); }
QString EditorViewModel::statusText() const { return statusText_; }

// ---------- object list ----------
int EditorViewModel::objectCount() const { return m_objects.size(); }
int EditorViewModel::selectedObjectIndex() const { return m_selectedObjectIndex; }

QString EditorViewModel::objectName(int i) const
{
  return (i >= 0 && i < m_objects.size()) ? m_objects[i].name : QString{};
}

bool EditorViewModel::objectVisible(int i) const
{
  return (i >= 0 && i < m_objects.size()) && m_objects[i].visible;
}

void EditorViewModel::setObjectVisible(int i, bool visible)
{
  if (i >= 0 && i < m_objects.size())
  {
    m_objects[i].visible = visible;
    emit stateChanged();
  }
}

void EditorViewModel::deleteObject(int i)
{
  if (i >= 0 && i < m_objects.size())
  {
    m_objects.removeAt(i);
    if (m_selectedObjectIndex >= m_objects.size())
      m_selectedObjectIndex = m_objects.isEmpty() ? -1 : m_objects.size() - 1;
    emit stateChanged();
  }
}

void EditorViewModel::selectObject(int i)
{
  if (m_selectedObjectIndex != i)
  {
    m_selectedObjectIndex = i;
    emit stateChanged();
  }
}

// ---------- slice bridge ----------
int EditorViewModel::sliceProgress() const { return sliceService_->progress(); }
bool EditorViewModel::isSlicing() const { return sliceService_->slicing(); }

// ---------- actions ----------
void EditorViewModel::importMockModel()
{
  projectService_->importMockModel();
  m_objects.append({QString("Model_%1.3mf").arg(m_objects.size() + 1), true});
  emit stateChanged();
}

void EditorViewModel::requestSlice()
{
  sliceService_->startSlice(projectService_->projectName());
}
