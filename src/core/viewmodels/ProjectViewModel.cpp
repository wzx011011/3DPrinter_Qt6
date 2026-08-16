#include "ProjectViewModel.h"
#include "core/services/ProjectServiceMock.h"

#include <QFileInfo>
#include <QSettings>

namespace
{
  void prependRecent(QStringList &items, const QString &path)
  {
    if (path.isEmpty())
      return;
    items.removeAll(path);
    items.prepend(path);
    constexpr int kMaxRecent = 12;
    while (items.size() > kMaxRecent)
      items.removeLast();
  }
} // namespace

ProjectViewModel::ProjectViewModel(QObject *parent) : QObject(parent)
{
  // Phase 241 (PAGE-01): recent projects load from QSettings persistence
  // (upstream app_config "recent_projects", wxFrame recent-files menu). The
  // three hardcoded mock paths are gone — an empty list renders the honest
  // HomePage empty state until a project is opened.
  m_recentProjects = QSettings()
                         .value(QStringLiteral("recentProjects"))
                         .toStringList();
}

void ProjectViewModel::setProjectService(ProjectServiceMock *projectService)
{
  m_projectService = projectService;
  refreshFileTree();
}

void ProjectViewModel::persistRecentProjects()
{
  // Phase 241 (PAGE-01): persist under the app QSettings tree so the recent
  // list survives restarts and is shared by the topbar Recent submenu and the
  // HomePage cards (single source of truth, upstream keeps one list too).
  QSettings settings;
  settings.setValue(QStringLiteral("recentProjects"), m_recentProjects);
  settings.sync();
}

QVariantList ProjectViewModel::fileTree() const
{
  QVariantList result;
  result.reserve(m_fileEntries.size());
  for (const auto &e : m_fileEntries)
  {
    result.append(QVariantMap{{"name", e.name}, {"isDir", e.isDir}, {"depth", e.depth}});
  }
  return result;
}

int ProjectViewModel::fileTreeCount() const { return m_fileEntries.size(); }
QString ProjectViewModel::fileTreeName(int i) const { return (i >= 0 && i < m_fileEntries.size()) ? m_fileEntries[i].name : QString{}; }
bool ProjectViewModel::fileTreeIsDir(int i) const { return (i >= 0 && i < m_fileEntries.size()) ? m_fileEntries[i].isDir : false; }
int ProjectViewModel::fileTreeDepth(int i) const { return (i >= 0 && i < m_fileEntries.size()) ? m_fileEntries[i].depth : 0; }

QString ProjectViewModel::projectFileSizeText() const
{
  // Phase 241 (PAGE-02): real QFileInfo size for the loaded project path
  // (replaces the always-"—" placeholder). Human-readable KB/MB formatting.
  if (m_currentProjectPath.isEmpty())
    return QString{};
  const QFileInfo fi(m_currentProjectPath);
  if (!fi.exists())
    return QString{};
  const qint64 bytes = fi.size();
  if (bytes >= 1024 * 1024)
    return QStringLiteral("%1 MB").arg(QString::number(bytes / (1024.0 * 1024.0), 'f', 2));
  if (bytes >= 1024)
    return QStringLiteral("%1 KB").arg(QString::number(bytes / 1024.0, 'f', 1));
  return QStringLiteral("%1 B").arg(bytes);
}

QString ProjectViewModel::projectLastModifiedText() const
{
  // Phase 241 (PAGE-02): real last-modified stamp from the file system.
  if (m_currentProjectPath.isEmpty())
    return QString{};
  const QFileInfo fi(m_currentProjectPath);
  if (!fi.exists())
    return QString{};
  return fi.lastModified().toString(QStringLiteral("yyyy-MM-dd hh:mm"));
}

void ProjectViewModel::refreshFileTree()
{
  // Phase 241 (PAGE-02): the "project resources" tree is derived from the
  // REAL loaded state instead of a hardcoded mock list:
  //   [0] the project file itself (depth 0, only when a path is set)
  //   [n] one "Plate k" group per plate (depth 1, folder-like)
  //       with the plate's object source modules underneath (depth 2).
  // With nothing loaded the tree is empty and the QML page shows its honest
  // empty state. Imported-but-unsaved models still show their plates.
  // Plate thumbnail images are NOT rendered here — they would need a base64
  // image provider (documented follow-up).
  m_fileEntries.clear();
  if (!m_currentProjectPath.isEmpty())
  {
    const QFileInfo fi(m_currentProjectPath);
    m_fileEntries.append({fi.fileName().isEmpty() ? m_currentProjectPath : fi.fileName(),
                          false, 0});
  }
  const bool hasContent = m_projectService
                          && (!m_currentProjectPath.isEmpty()
                              || m_projectService->modelCount() > 0);
  if (hasContent)
  {
    const QStringList allObjectNames = m_projectService->objectNames();
    const int plateCount = m_projectService->plateCount();
    for (int p = 0; p < plateCount; ++p)
    {
      m_fileEntries.append({QObject::tr("Plate %1").arg(p + 1), true, 1});
      const QList<int> objects = m_projectService->plateObjectIndices(p);
      for (int objectIndex : objects)
      {
        QString moduleName = m_projectService->objectModuleName(objectIndex);
        if (moduleName.isEmpty())
          moduleName = allObjectNames.value(objectIndex);
        if (!moduleName.isEmpty())
          m_fileEntries.append({moduleName, false, 2});
      }
    }
  }
  emit projectChanged();
}

void ProjectViewModel::newProject()
{
  m_currentProjectPath = "";
  m_isDirty = false;
  refreshFileTree();
  emit dirtyChanged();
}

void ProjectViewModel::openProject(const QString &path)
{
  m_currentProjectPath = path;
  m_isDirty = false;
  refreshFileTree();
  const QStringList before = m_recentProjects;
  prependRecent(m_recentProjects, path);
  if (before != m_recentProjects)
    persistRecentProjects();
  emit dirtyChanged();
  if (before != m_recentProjects)
    emit recentChanged();
}

void ProjectViewModel::saveProject()
{
  m_isDirty = false;
  const QStringList before = m_recentProjects;
  prependRecent(m_recentProjects, m_currentProjectPath);
  if (before != m_recentProjects)
    persistRecentProjects();
  emit dirtyChanged();
  if (before != m_recentProjects)
    emit recentChanged();
}

void ProjectViewModel::saveProjectAs(const QString &path)
{
  m_currentProjectPath = path;
  m_isDirty = false;
  refreshFileTree();
  const QStringList before = m_recentProjects;
  prependRecent(m_recentProjects, path);
  if (before != m_recentProjects)
    persistRecentProjects();
  emit dirtyChanged();
  if (before != m_recentProjects)
    emit recentChanged();
}

void ProjectViewModel::importModel(const QStringList &paths)
{
  Q_UNUSED(paths)
  m_isDirty = true;
  emit dirtyChanged();
}

void ProjectViewModel::selectFile(const QString &path)
{
  if (m_selectedFile != path)
  {
    m_selectedFile = path;
    emit selectionChanged();
  }
}

void ProjectViewModel::clearRecentProjects()
{
  if (m_recentProjects.isEmpty())
    return;
  m_recentProjects.clear();
  persistRecentProjects();
  emit recentChanged();
}
