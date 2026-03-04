#include "ProjectViewModel.h"

ProjectViewModel::ProjectViewModel(QObject *parent) : QObject(parent)
{
  m_recentProjects = {
      "C:/projects/benchy.3mf",
      "C:/projects/phone_stand.3mf",
      "C:/projects/bracket.stl"};

  // Store as structs to avoid QVariantList member corruption by V4 GC
  m_fileEntries = {
      {"project.3mf", false, 0},
      {"models", true, 0},
      {"benchy.stl", false, 1},
      {"support.stl", false, 1},
      {"config.ini", false, 0},
  };
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

void ProjectViewModel::newProject()
{
  m_currentProjectPath = "";
  m_isDirty = false;
  m_fileEntries.clear();
  emit projectChanged();
  emit dirtyChanged();
}

void ProjectViewModel::openProject(const QString &path)
{
  m_currentProjectPath = path;
  m_isDirty = false;
  emit projectChanged();
  emit dirtyChanged();
}

void ProjectViewModel::saveProject()
{
  m_isDirty = false;
  emit dirtyChanged();
}

void ProjectViewModel::saveProjectAs(const QString &path)
{
  m_currentProjectPath = path;
  m_isDirty = false;
  emit projectChanged();
  emit dirtyChanged();
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
  emit recentChanged();
}
