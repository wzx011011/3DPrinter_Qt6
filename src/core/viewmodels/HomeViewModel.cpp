#include "HomeViewModel.h"

#include "core/services/CloudServiceMock.h"
#include "core/viewmodels/ProjectViewModel.h"
#include <QFileInfo>

HomeViewModel::HomeViewModel(CloudServiceMock *cloudService, QObject *parent)
    : QObject(parent), cloudService_(cloudService)
{
  // Phase 241 (PAGE-01): the hardcoded mock recent list is gone. Entries now
  // mirror ProjectViewModel's persisted recent-projects list (upstream
  // app_config "recent_projects", wxFrame recent-files menu). Empty until a
  // source viewmodel is injected via setProjectViewModel.

  if (cloudService_) {
    connect(cloudService_, &CloudServiceMock::loginStateChanged,
            this, &HomeViewModel::cloudStateChanged);
    connect(cloudService_, &CloudServiceMock::devicesChanged,
            this, &HomeViewModel::cloudStateChanged);
    connect(cloudService_, &CloudServiceMock::syncStateChanged,
            this, &HomeViewModel::cloudStateChanged);
    connect(cloudService_, &CloudServiceMock::loginFailed,
            this, [this](const QString &err) { emit cloudLoginFailed(err); });
  }
}

void HomeViewModel::setProjectViewModel(ProjectViewModel *projectViewModel)
{
  // Drop any previous source wiring before attaching the new one.
  if (projectViewModel_)
    disconnect(projectViewModel_, &ProjectViewModel::recentChanged,
               this, &HomeViewModel::refreshRecentProjects);
  projectViewModel_ = projectViewModel;
  if (projectViewModel_)
  {
    // Keep the HomePage cards live whenever the persisted recent list
    // changes (open/save/clear), independent of the composition root.
    connect(projectViewModel_, &ProjectViewModel::recentChanged,
            this, &HomeViewModel::refreshRecentProjects);
  }
  refreshRecentProjects();
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

void HomeViewModel::openProject(const QString &path)
{
  // Phase 241 (PAGE-01): routes through BackendContext::topbarOpenProject
  // (upstream Plater::load_file) — never a silent no-op.
  if (!path.isEmpty())
    emit openProjectRequested(path);
}

void HomeViewModel::openRecentProject(int index)
{
  if (index >= 0 && index < m_entries.size())
    emit openProjectRequested(m_entries[index].path);
}

void HomeViewModel::refreshRecentProjects()
{
  // Phase 241 (PAGE-01): rebuild from the persisted source list. name/date
  // derive from the real file (QFileInfo) instead of stored strings — matches
  // upstream, which shows the file name and last-modified stamp.
  m_entries.clear();
  if (projectViewModel_)
  {
    const QStringList paths = projectViewModel_->recentProjects();
    m_entries.reserve(paths.size());
    for (const QString &path : paths)
    {
      const QFileInfo fi(path);
      ProjectEntry entry;
      entry.path = path;
      entry.name = fi.fileName().isEmpty() ? path : fi.fileName();
      entry.date = fi.exists()
                       ? fi.lastModified().toString(QStringLiteral("yyyy-MM-dd"))
                       : QString{};
      m_entries.append(entry);
    }
  }
  emit recentProjectsChanged();
}

// ── Cloud account ────────────────────────────────────────────

bool HomeViewModel::cloudLoggedIn() const { return cloudService_ ? cloudService_->loggedIn() : false; }
QString HomeViewModel::cloudUserName() const { return cloudService_ ? cloudService_->userName() : QString(); }
QString HomeViewModel::cloudUserEmail() const { return cloudService_ ? cloudService_->userEmail() : QString(); }
int HomeViewModel::cloudBoundDeviceCount() const { return cloudService_ ? cloudService_->boundDeviceCount() : 0; }
bool HomeViewModel::cloudSyncing() const { return cloudService_ ? cloudService_->syncing() : false; }
QString HomeViewModel::cloudLastSyncTime() const { return cloudService_ ? cloudService_->lastSyncTime() : QString(); }

void HomeViewModel::cloudLogin(const QString &user, const QString &password)
{
  if (cloudService_) cloudService_->login(user, password);
}

void HomeViewModel::cloudLogout()
{
  if (cloudService_) cloudService_->logout();
}

void HomeViewModel::cloudBindDevice(const QString &name, const QString &pin)
{
  if (cloudService_) cloudService_->bindDevice(name, pin);
}

void HomeViewModel::cloudUnbindDevice(int index)
{
  if (cloudService_) cloudService_->unbindDevice(index);
}

QVariantMap HomeViewModel::cloudBoundDeviceAt(int index) const
{
  return cloudService_ ? cloudService_->boundDeviceAt(index) : QVariantMap();
}

void HomeViewModel::cloudSyncPresets()
{
  if (cloudService_) cloudService_->syncPresets();
}
