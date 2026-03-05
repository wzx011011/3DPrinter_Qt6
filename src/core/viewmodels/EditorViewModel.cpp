#include "EditorViewModel.h"

#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceService.h"
#include <QUrl>
#include <QVector4D>
#include <cstring> // memcpy
#include <cmath>

EditorViewModel::EditorViewModel(ProjectServiceMock *projectService, SliceService *sliceService, QObject *parent)
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

  connect(sliceService_, &SliceService::slicingChanged, this, [this]()
          {
        statusText_ = sliceService_->slicing() ? QStringLiteral("切片中...") : QStringLiteral("切片完成");
        emit stateChanged(); });

  connect(sliceService_, &SliceService::progressChanged, this, &EditorViewModel::stateChanged);
  connect(sliceService_, &SliceService::progressUpdated, this, [this](int percent, const QString &label)
          {
        statusText_ = QStringLiteral("%1... %2%").arg(label).arg(percent);
        emit stateChanged(); });
  connect(sliceService_, &SliceService::sliceFailed, this, [this](const QString &message)
          {
        statusText_ = message;
        emit stateChanged(); });
  connect(projectService_, &ProjectServiceMock::loadProgressUpdated, this, [this](int progress, const QString &stageText)
          {
    if (projectService_->loading())
      statusText_ = QStringLiteral("%1... %2%").arg(stageText).arg(progress);
    emit stateChanged(); });
  connect(projectService_, &ProjectServiceMock::loadingChanged, this, [this]()
          {
    if (!projectService_->loading())
      emit stateChanged(); });
  connect(projectService_, &ProjectServiceMock::loadFinished, this, [this](bool success, const QString &message)
          {
    if (success)
    {
      m_objects.clear();
      const QStringList names = projectService_->objectNames();
      for (const auto &name : names)
      {
        m_objects.append({name, true});
      }
      m_selectedObjectIndex = m_objects.isEmpty() ? -1 : 0;
      statusText_ = QStringLiteral("已加载 %1 个模型，%2 个平板").arg(m_objects.size()).arg(projectService_->plateCount());

      const QByteArray md = projectService_->meshData();
      m_fitHint = QVector4D();
      constexpr int kBboxBytes = 6 * int(sizeof(float));
      if (md.size() >= kBboxBytes)
      {
        float bbox[6];
        memcpy(bbox, md.constData() + md.size() - kBboxBytes, kBboxBytes);
        const float cx = (bbox[0] + bbox[3]) * 0.5f;
        const float cy = (bbox[1] + bbox[4]) * 0.5f;
        const float cz = (bbox[2] + bbox[5]) * 0.5f;
        const float dx = bbox[3] - bbox[0];
        const float dy = bbox[4] - bbox[1];
        const float dz = bbox[5] - bbox[2];
        const float radius = std::sqrt(dx * dx + dy * dy + dz * dz) * 0.5f;
        m_fitHint = QVector4D(cx, cy, cz, std::max(radius, 10.f));
      }
    }
    else
    {
      statusText_ = message;
      m_fitHint = QVector4D();
    }
    emit stateChanged(); });
}

QString EditorViewModel::projectName() const { return projectService_->projectName(); }
int EditorViewModel::modelCount() const { return projectService_->modelCount(); }
int EditorViewModel::plateCount() const { return projectService_->plateCount(); }
QString EditorViewModel::statusText() const { return statusText_; }
int EditorViewModel::loadProgress() const { return projectService_->loadProgress(); }
bool EditorViewModel::loading() const { return projectService_->loading(); }
QByteArray EditorViewModel::meshData() const { return projectService_->meshData(); }

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

void EditorViewModel::cancelSlice()
{
  sliceService_->cancelSlice();
}

bool EditorViewModel::loadFile(const QString &filePath)
{
  // Accept both local paths and file:// URLs (from QML FileDialog)
  QUrl url(filePath);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : filePath;
  const bool started = projectService_->loadFile(localPath);
  if (started)
  {
    statusText_ = QStringLiteral("正在加载...");
    m_fitHint = QVector4D();
  }
  else
  {
    statusText_ = projectService_->lastError();
  }
  emit stateChanged();
  return started;
}

void EditorViewModel::clearWorkspace()
{
  if (projectService_)
    projectService_->clearProject();

  m_objects.clear();
  m_selectedObjectIndex = -1;
  m_fitHint = QVector4D();
  statusText_ = QStringLiteral("已新建项目");
  emit stateChanged();
}
