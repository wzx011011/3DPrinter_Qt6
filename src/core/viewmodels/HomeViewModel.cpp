#include "HomeViewModel.h"

HomeViewModel::HomeViewModel(QObject *parent) : QObject(parent)
{
  // Mock recent projects stored as plain structs (no QVariantList member - prevents V4 GC destructor crash)
  m_entries = {
      {"benchy.3mf", "2026-03-02", "C:/projects/benchy.3mf"},
      {"phone_stand.3mf", "2026-03-01", "C:/projects/phone_stand.3mf"},
      {"bracket.stl", "2026-02-28", "C:/projects/bracket.stl"},
      {"miniature_figure.3mf", "2026-02-25", "C:/projects/figure.3mf"},
      {"cable_clip.stl", "2026-02-20", "C:/projects/cable_clip.stl"},
  };
}

QVariantList HomeViewModel::recentProjects() const
{
  QVariantList result;
  result.reserve(m_entries.size());
  for (const auto &e : m_entries)
  {
    result.append(QVariantMap{{"name", e.name}, {"date", e.date}, {"path", e.path}});
  }
  return result;
}

int HomeViewModel::recentProjectCount() const { return m_entries.size(); }
QString HomeViewModel::recentProjectName(int i) const { return (i >= 0 && i < m_entries.size()) ? m_entries[i].name : QString{}; }
QString HomeViewModel::recentProjectDate(int i) const { return (i >= 0 && i < m_entries.size()) ? m_entries[i].date : QString{}; }
QString HomeViewModel::recentProjectPath(int i) const { return (i >= 0 && i < m_entries.size()) ? m_entries[i].path : QString{}; }

void HomeViewModel::openProject(const QString &path) { Q_UNUSED(path) }
void HomeViewModel::openRecentProject(int index) { Q_UNUSED(index) }
void HomeViewModel::refreshRecentProjects() { emit recentProjectsChanged(); }
