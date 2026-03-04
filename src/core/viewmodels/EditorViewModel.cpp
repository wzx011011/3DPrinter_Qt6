#include "EditorViewModel.h"

#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceServiceMock.h"
#include <QUrl>
#include <QVector4D>
#include <cstring> // memcpy
#include <cmath>

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

bool EditorViewModel::loadFile(const QString &filePath)
{
  // Accept both local paths and file:// URLs (from QML FileDialog)
  QUrl url(filePath);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : filePath;
  bool ok = projectService_->loadFile(localPath);
  if (ok)
  {
    // Sync object list from ProjectService
    m_objects.clear();
    const QStringList names = projectService_->objectNames();
    for (const auto &name : names)
    {
      m_objects.append({name, true});
    }
    m_selectedObjectIndex = m_objects.isEmpty() ? -1 : 0;
    statusText_ = QStringLiteral("已加载 %1 个模型").arg(m_objects.size());

    // ── 从 TLV 尾部 bbox 计算相机 fitHint ─────────────────────────────
    const QByteArray md = projectService_->meshData();
    m_fitHint = QVector4D(); // reset
    // TLV trailer 最后 6 个 float = bbox
    constexpr int kBboxBytes = 6 * int(sizeof(float));
    if (md.size() >= kBboxBytes)
    {
      float bbox[6];
      memcpy(bbox, md.constData() + md.size() - kBboxBytes, kBboxBytes);
      // bbox: [minX, minY, minZ, maxX, maxY, maxZ] in GL coords
      const float cx = (bbox[0] + bbox[3]) * 0.5f;
      const float cy = (bbox[1] + bbox[4]) * 0.5f;
      const float cz = (bbox[2] + bbox[5]) * 0.5f;
      const float dx = bbox[3] - bbox[0];
      const float dy = bbox[4] - bbox[1];
      const float dz = bbox[5] - bbox[2];
      const float radius = std::sqrt(dx * dx + dy * dy + dz * dz) * 0.5f;
      m_fitHint = QVector4D(cx, cy, cz, std::max(radius, 10.f));
      qInfo("[EditorViewModel] fitHint: center=(%.1f, %.1f, %.1f) radius=%.1f",
            cx, cy, cz, m_fitHint.w());
    }
  }
  else
  {
    statusText_ = projectService_->lastError();
  }
  emit stateChanged();
  return ok;
}
