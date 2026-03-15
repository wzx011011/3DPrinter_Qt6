#include "MultiMachineViewModel.h"
#include <algorithm>
#include <QSharedPointer>

// Upstream state maps (from MultiMachine.cpp DeviceItem::sync_state / get_state_device)
static const char *deviceStatusText(int state) {
  switch (state) {
    case 0: return "Idle";             // IDLE
    case 1: return "Printing Finish";  // FINISH
    case 2: return "Printing Failed";  // FAILED
    case 3: return "Printing";         // RUNNING
    case 4: return "Printing Pause";   // PAUSE
    case 5: return "Prepare";          // PREPARE
    case 6: return "Slicing";          // SLICING
    default: return "Syncing";         // unknown
  }
}

// Upstream local task states (from MultiMachine.cpp DeviceItem::get_local_state_task)
static const char *localTaskStatusText(int state) {
  switch (state) {
    case 0: return "Pending";
    case 1: return "Sending";
    case 2: return "Sending Finish";
    case 3: return "Sending Cancel";
    case 4: return "Sending Failed";
    case 5: return "Printing";
    case 6: return "Print Success";
    case 7: return "Print Failed";
    case 8: return "Removed";
    case 9: return "Idle";
    default: return "Unknown";
  }
}

// Upstream cloud task states (from MultiMachine.cpp DeviceItem::get_cloud_state_task)
static const char *cloudTaskStatusText(int state) {
  switch (state) {
    case 0: return "Printing";
    case 1: return "Printing Finish";
    case 2: return "Printing Failed";
    default: return "Unknown";
  }
}

// ──────────────────────────────────────────────
// Construction
// ──────────────────────────────────────────────
MultiMachineViewModel::MultiMachineViewModel(QObject *parent)
    : QObject(parent)
{
  buildMockData();
}

void MultiMachineViewModel::buildMockData()
{
  // 5 mock devices (upstream max is PICK_DEVICE_MAX=6)
  // Status codes align with upstream state_device enum
  m_machineEntries = {
      {"K1C-001",          "K1C",             "Printing",        "01:23:00", "192.168.1.101", "Benchy.gcode",  "",        true,  3, 67},
      {"K1-Max-002",       "K1 Max",          "Printing Finish", "--",       "192.168.1.102", "",                "",        true,  1, 100},
      {"Ender-3S1-Pro",    "Ender-3 S1 Pro",  "Idle",            "--",       "192.168.1.103", "",                "",        true,  0, 0},
      {"K2-Plus-004",      "K2 Plus",         "Prepare",         "--",       "192.168.1.104", "Cube.stl",       "",        true,  5, 0},
      {"CR-10-Smart-005",  "CR-10 Smart Pro", "",                "--",       "--",             "",                "",        false, 7, 0},
  };

  // 3 mock local tasks (aligns with upstream LocalTaskManagerPage / MultiTaskItem)
  m_localTasks = {
      {"Benchy.gcode",   "K1C-001",  "2026-03-14 09:30", "01:23:00", 5, 67,  0,  false},  // printing
      {"Cube.stl",       "K2-Plus-004", "2026-03-14 09:15", "--",        0, 0,   0,  false},  // pending
      {"Bracket_v2.3mf", "K1-Max-002", "2026-03-14 08:00", "--",        6, 100, 0,  false},  // success
  };

  // 3 mock cloud tasks (aligns with upstream CloudTaskManagerPage)
  m_cloudTasks = {
      {"CalibrationCube.gcode", "K1C-001",    "2026-03-13 14:00", "--",        1, 100, false},
      {"Gear_Assembly.gcode",   "K1-Max-002", "2026-03-13 16:30", "02:15:30", 0, 42,  false},
      {"Phone_Stand.stl",       "K2-Plus-004","2026-03-13 10:00", "--",        2, 88,  false},
  };
}

// ──────────────────────────────────────────────
// Device accessors
// ──────────────────────────────────────────────
int MultiMachineViewModel::machineCount() const {
  // Return filtered count for current page (aligns with upstream pagination)
  int start = m_currentPage * m_pageSize;
  int count = 0;
  int shown = 0;
  for (const auto &m : m_machineEntries) {
    if (!m.filteredIn) continue;
    ++count;
    if (shown >= start) ++shown;
  }
  // Simpler: count filtered items in current page range
  int filteredSoFar = 0;
  int pageStart = m_currentPage * m_pageSize;
  for (const auto &m : m_machineEntries) {
    if (!m.filteredIn) continue;
    if (filteredSoFar >= pageStart && filteredSoFar < pageStart + m_pageSize)
      ++shown;
    ++filteredSoFar;
  }
  return shown;
}

int MultiMachineViewModel::filteredMachineCount() const {
  int count = 0;
  for (const auto &m : m_machineEntries)
    if (m.filteredIn) ++count;
  return count;
}

QString MultiMachineViewModel::searchText() const { return m_searchText; }

void MultiMachineViewModel::setSearchText(const QString &text) {
  if (text == m_searchText) return;
  m_searchText = text;
  // Update filter state for all entries (aligns with upstream selected_machines)
  const QString filter = text.trimmed().toLower();
  for (auto &m : m_machineEntries) {
    if (filter.isEmpty()) {
      m.filteredIn = true;
    } else {
      m.filteredIn = m.name.toLower().contains(filter)
                     || m.model.toLower().contains(filter)
                     || m.status.toLower().contains(filter)
                     || m.ip.toLower().contains(filter);
    }
  }
  m_currentPage = 0;
  recomputePagination();
  emit machinesChanged();
}

int MultiMachineViewModel::machinePageCount() const { return m_machineEntries.size(); }
bool MultiMachineViewModel::hasDevices() const { return !m_machineEntries.isEmpty(); }

// Helper: get the i-th filtered machine entry (within current page)
const MultiMachineViewModel::MachineEntry *MultiMachineViewModel::filteredMachineAt(
    const QList<MultiMachineViewModel::MachineEntry> &list, int page, int pageSize, int i) {
  int pageStart = page * pageSize;
  int filteredIdx = 0;
  for (const auto &m : list) {
    if (!m.filteredIn) continue;
    if (filteredIdx == pageStart + i) return &m;
    ++filteredIdx;
  }
  return nullptr;
}

QString MultiMachineViewModel::machineName(int i) const {
  if (auto *e = filteredMachineAt(m_machineEntries, m_currentPage, m_pageSize, i)) return e->name;
  return {};
}
QString MultiMachineViewModel::machineModel(int i) const {
  if (auto *e = filteredMachineAt(m_machineEntries, m_currentPage, m_pageSize, i)) return e->model;
  return {};
}
QString MultiMachineViewModel::machineStatus(int i) const {
  if (auto *e = filteredMachineAt(m_machineEntries, m_currentPage, m_pageSize, i))
    return QString(deviceStatusText(e->statusInt));
  return {};
}
int MultiMachineViewModel::machineStatusInt(int i) const {
  if (auto *e = filteredMachineAt(m_machineEntries, m_currentPage, m_pageSize, i)) return e->statusInt;
  return 7;
}
bool MultiMachineViewModel::machineOnline(int i) const {
  if (auto *e = filteredMachineAt(m_machineEntries, m_currentPage, m_pageSize, i)) return e->online;
  return false;
}
bool MultiMachineViewModel::machineSelected(int i) const { Q_UNUSED(i); return false; }
int MultiMachineViewModel::machineProgress(int i) const {
  if (auto *e = filteredMachineAt(m_machineEntries, m_currentPage, m_pageSize, i)) return e->progress;
  return 0;
}
QString MultiMachineViewModel::machineRemaining(int i) const {
  if (auto *e = filteredMachineAt(m_machineEntries, m_currentPage, m_pageSize, i)) return e->remaining;
  return {};
}
QString MultiMachineViewModel::machineIp(int i) const {
  if (auto *e = filteredMachineAt(m_machineEntries, m_currentPage, m_pageSize, i)) return e->ip;
  return {};
}
QString MultiMachineViewModel::machineTaskName(int i) const {
  if (auto *e = filteredMachineAt(m_machineEntries, m_currentPage, m_pageSize, i))
    return e->taskName.isEmpty() ? QStringLiteral("No task") : e->taskName;
  return QStringLiteral("No task");
}

// ──────────────────────────────────────────────
// Pagination
// ──────────────────────────────────────────────
int MultiMachineViewModel::currentPage() const { return m_currentPage; }
void MultiMachineViewModel::setCurrentPage(int page) {
  if (page < 0) page = 0;
  if (page >= totalPages()) page = std::max(0, totalPages() - 1);
  if (page != m_currentPage) {
    m_currentPage = page;
    emit paginationChanged();
    emit machinesChanged();
  }
}
int MultiMachineViewModel::totalPages() const {
  int filtered = filteredMachineCount();
  return (filtered == 0) ? 1 : (int)std::ceil((double)filtered / m_pageSize);
}
int MultiMachineViewModel::pageSize() const { return m_pageSize; }
QString MultiMachineViewModel::selectedCountText() const {
  // Upstream: "Select Connected Printers (X/6)"
  return QString("Select Connected Printers (%1/%2)").arg(filteredMachineCount()).arg(6);
}

// ──────────────────────────────────────────────
// Local Task accessors
// ──────────────────────────────────────────────
int MultiMachineViewModel::localTaskCount() const { return m_localTasks.size(); }
bool MultiMachineViewModel::hasLocalTasks() const { return !m_localTasks.isEmpty(); }
int MultiMachineViewModel::localSelectedCount() const {
  int c = 0;
  for (const auto &t : m_localTasks) if (t.selected) ++c;
  return c;
}

QString MultiMachineViewModel::localTaskProjectName(int i) const {
  return (i >= 0 && i < m_localTasks.size()) ? m_localTasks[i].projectName : QString{};
}
QString MultiMachineViewModel::localTaskDevName(int i) const {
  return (i >= 0 && i < m_localTasks.size()) ? m_localTasks[i].devName : QString{};
}
int MultiMachineViewModel::localTaskStatus(int i) const {
  return (i >= 0 && i < m_localTasks.size()) ? m_localTasks[i].status : 0;
}
QString MultiMachineViewModel::localTaskStatusText(int i) const {
  return (i >= 0 && i < m_localTasks.size()) ? QString(localTaskStatusText(m_localTasks[i].status)) : QString{};
}
int MultiMachineViewModel::localTaskProgress(int i) const {
  return (i >= 0 && i < m_localTasks.size()) ? m_localTasks[i].progress : 0;
}
QString MultiMachineViewModel::localTaskSendTime(int i) const {
  return (i >= 0 && i < m_localTasks.size()) ? m_localTasks[i].sendTime : QString{};
}
QString MultiMachineViewModel::localTaskRemaining(int i) const {
  return (i >= 0 && i < m_localTasks.size()) ? m_localTasks[i].remaining : QString{"--"};
}
int MultiMachineViewModel::localTaskSendingPercent(int i) const {
  return (i >= 0 && i < m_localTasks.size()) ? m_localTasks[i].sendingPercent : 0;
}
bool MultiMachineViewModel::localTaskSelected(int i) const {
  return (i >= 0 && i < m_localTasks.size()) ? m_localTasks[i].selected : false;
}

// ──────────────────────────────────────────────
// Cloud Task accessors
// ──────────────────────────────────────────────
int MultiMachineViewModel::cloudTaskCount() const { return m_cloudTasks.size(); }
bool MultiMachineViewModel::hasCloudTasks() const { return !m_cloudTasks.isEmpty(); }

QString MultiMachineViewModel::cloudTaskProjectName(int i) const {
  return (i >= 0 && i < m_cloudTasks.size()) ? m_cloudTasks[i].projectName : QString{};
}
QString MultiMachineViewModel::cloudTaskDevName(int i) const {
  return (i >= 0 && i < m_cloudTasks.size()) ? m_cloudTasks[i].devName : QString{};
}
int MultiMachineViewModel::cloudTaskStatus(int i) const {
  return (i >= 0 && i < m_cloudTasks.size()) ? m_cloudTasks[i].status : 0;
}
QString MultiMachineViewModel::cloudTaskStatusText(int i) const {
  return (i >= 0 && i < m_cloudTasks.size()) ? QString(cloudTaskStatusText(m_cloudTasks[i].status)) : QString{};
}
int MultiMachineViewModel::cloudTaskProgress(int i) const {
  return (i >= 0 && i < m_cloudTasks.size()) ? m_cloudTasks[i].progress : 0;
}
QString MultiMachineViewModel::cloudTaskSendTime(int i) const {
  return (i >= 0 && i < m_cloudTasks.size()) ? m_cloudTasks[i].sendTime : QString{};
}
QString MultiMachineViewModel::cloudTaskRemaining(int i) const {
  return (i >= 0 && i < m_cloudTasks.size()) ? m_cloudTasks[i].remaining : QString{"--"};
}

// ──────────────────────────────────────────────
// Actions
// ──────────────────────────────────────────────
void MultiMachineViewModel::viewMachine(int index)
{
  // Aligns with upstream EVT_MULTI_DEVICE_VIEW -> jump_to_monitor
  int idx = m_currentPage * m_pageSize + index;
  if (idx >= 0 && idx < m_machineEntries.size()) {
    emit messageRequested(tr("View device: %1").arg(m_machineEntries[idx].name));
  }
}

void MultiMachineViewModel::selectLocalTask(int index)
{
  // Upstream: only pending (0) and sending (1) tasks are selectable (checkbox enabled)
  if (index >= 0 && index < m_localTasks.size()) {
    if (m_localTasks[index].status <= 1) {
      m_localTasks[index].selected = !m_localTasks[index].selected;
    }
    emit localTasksChanged();
  }
}

void MultiMachineViewModel::selectAllLocalTasks()
{
  // Aligns with upstream m_select_checkbox toggle: select all where status <= 1
  for (auto &t : m_localTasks) {
    if (t.status <= 1) t.selected = true;
  }
  emit localTasksChanged();
}

void MultiMachineViewModel::unselectAllLocalTasks()
{
  for (auto &t : m_localTasks) {
    t.selected = false;
  }
  emit localTasksChanged();
}

void MultiMachineViewModel::cancelLocalTask(int index)
{
  // Aligns with upstream MultiTaskItem::onCancel: cancel via task_obj->cancel() + command_task_cancel
  if (index >= 0 && index < m_localTasks.size()) {
    if (m_localTasks[index].status <= 1) {
      m_localTasks[index].status = 3; // Sending Cancel
      m_localTasks[index].sendingPercent = 0;
      emit localTasksChanged();
      emit messageRequested(tr("Task cancelled: %1").arg(m_localTasks[index].projectName));
    }
  }
}

void MultiMachineViewModel::stopAllLocalTasks()
{
  // Aligns with upstream LocalTaskManagerPage::cancel_all: cancel selected tasks where button is shown
  for (auto &t : m_localTasks) {
    if (t.selected && t.status <= 1) {
      t.status = 3; // cancel
      t.sendingPercent = 0;
    }
  }
  emit localTasksChanged();
  emit messageRequested(tr("All selected tasks cancelled"));
}

void MultiMachineViewModel::pauseCloudTask(int index)
{
  // Aligns with upstream MultiTaskItem::onPause: command_task_pause
  if (index >= 0 && index < m_cloudTasks.size() && m_cloudTasks[index].status == 0) {
    emit messageRequested(tr("Pause cloud task: %1").arg(m_cloudTasks[index].projectName));
    emit cloudTasksChanged();
  }
}

void MultiMachineViewModel::resumeCloudTask(int index)
{
  // Aligns with upstream MultiTaskItem::onResume: command_task_resume
  if (index >= 0 && index < m_cloudTasks.size() && m_cloudTasks[index].status == 0) {
    emit messageRequested(tr("Resume cloud task: %1").arg(m_cloudTasks[index].projectName));
    emit cloudTasksChanged();
  }
}

void MultiMachineViewModel::stopCloudTask(int index)
{
  // Aligns with upstream MultiTaskItem::onStop: command_task_abort
  if (index >= 0 && index < m_cloudTasks.size()) {
    if (m_cloudTasks[index].status == 0) {
      m_cloudTasks[index].status = 2; // failed
      emit cloudTasksChanged();
      emit messageRequested(tr("Stop cloud task: %1").arg(m_cloudTasks[index].projectName));
    }
  }
}

void MultiMachineViewModel::stopAllCloudTasks()
{
  // Aligns with upstream CloudTaskManagerPage::stop_all
  for (auto &t : m_cloudTasks) {
    if (t.selected && t.status == 0) {
      t.status = 2; // failed
    }
  }
  emit cloudTasksChanged();
  emit messageRequested(tr("All selected cloud tasks stopped"));
}

void MultiMachineViewModel::pauseAllCloudTasks()
{
  // Aligns with upstream CloudTaskManagerPage::pause_all
  for (auto &t : m_cloudTasks) {
    if (t.selected && t.status == 0) {
      emit messageRequested(tr("Pause cloud task: %1").arg(t.projectName));
    }
  }
  emit cloudTasksChanged();
}

void MultiMachineViewModel::resumeAllCloudTasks()
{
  // Aligns with upstream CloudTaskManagerPage::resume_all
  for (auto &t : m_cloudTasks) {
    if (t.selected && t.status == 0) {
      emit messageRequested(tr("Resume cloud task: %1").arg(t.projectName));
    }
  }
  emit cloudTasksChanged();
}

void MultiMachineViewModel::editPrinters()
{
  // Aligns with upstream MultiMachinePickPage "Edit Printers" button
  emit messageRequested(tr("Edit Printers dialog (mock)"));
}

void MultiMachineViewModel::addDevice()
{
  // Aligns with upstream MultiMachinePickPage "Add" button
  if (m_machineEntries.size() >= 6) {
    emit messageRequested(tr("Maximum 6 printers allowed"));
    return;
  }
  int n = m_machineEntries.size() + 1;
  m_machineEntries.append({
      QString("New-Printer-%1").arg(n),
      "Unknown",
      "Idle",
      "--",
      "0.0.0.0",
      "",
      "",
      true,
      0,
      0
  });
  recomputePagination();
  emit machinesChanged();
  emit messageRequested(tr("Device added (mock)"));
}

void MultiMachineViewModel::removeDevice(int index)
{
  int idx = m_currentPage * m_pageSize + index;
  if (idx >= 0 && idx < m_machineEntries.size()) {
    QString name = m_machineEntries[idx].name;
    m_machineEntries.removeAt(idx);
    recomputePagination();
    emit machinesChanged();
    emit messageRequested(tr("Device removed: %1").arg(name));
  }
}

void MultiMachineViewModel::refreshMachines()
{
  emit machinesChanged();
  emit localTasksChanged();
  emit cloudTasksChanged();
}

void MultiMachineViewModel::sortDevicesByName()
{
  m_sortNameAsc = !m_sortNameAsc;
  m_sortField = 1;
  std::sort(m_machineEntries.begin(), m_machineEntries.end(),
    [this](const MachineEntry &a, const MachineEntry &b) {
      return m_sortNameAsc ? a.name < b.name : a.name > b.name;
    });
  emit machinesChanged();
}

void MultiMachineViewModel::sortDevicesByStatus()
{
  m_sortStatusAsc = !m_sortStatusAsc;
  m_sortField = 2;
  std::sort(m_machineEntries.begin(), m_machineEntries.end(),
    [this](const MachineEntry &a, const MachineEntry &b) {
      return m_sortStatusAsc ? a.statusInt < b.statusInt : a.statusInt > b.statusInt;
    });
  emit machinesChanged();
}

void MultiMachineViewModel::recomputePagination()
{
  int total = totalPages();
  if (m_currentPage >= total && total > 0) m_currentPage = total - 1;
  emit paginationChanged();
}

// ── Cloud Task selection (aligns with upstream EVT_MULTI_CLOUD_TASK_SELECTED) ──
bool MultiMachineViewModel::cloudTaskSelected(int i) const {
  return (i >= 0 && i < m_cloudTasks.size()) ? m_cloudTasks[i].selected : false;
}

void MultiMachineViewModel::selectCloudTask(int index)
{
  // Upstream: only cloud tasks with status == 0 (printing) are selectable
  if (index >= 0 && index < m_cloudTasks.size()) {
    if (m_cloudTasks[index].status == 0) {
      m_cloudTasks[index].selected = !m_cloudTasks[index].selected;
    }
    emit cloudTasksChanged();
  }
}

void MultiMachineViewModel::selectAllCloudTasks()
{
  // Aligns with upstream m_select_checkbox toggle: select all printing tasks
  for (auto &t : m_cloudTasks) {
    if (t.status == 0) t.selected = true;
  }
  emit cloudTasksChanged();
}

void MultiMachineViewModel::unselectAllCloudTasks()
{
  for (auto &t : m_cloudTasks) {
    t.selected = false;
  }
  emit cloudTasksChanged();
}

int MultiMachineViewModel::cloudSelectedCount() const {
  int c = 0;
  for (const auto &t : m_cloudTasks) if (t.selected) ++c;
  return c;
}

// ── Cloud Task pagination (aligns with upstream CloudTaskManagerPage m_flipping_panel) ──
int MultiMachineViewModel::cloudCurrentPage() const { return m_cloudCurrentPage; }
void MultiMachineViewModel::setCloudCurrentPage(int page) {
  if (page < 0) page = 0;
  int total = cloudTotalPages();
  if (total > 0 && page >= total) page = total - 1;
  if (page != m_cloudCurrentPage) {
    m_cloudCurrentPage = page;
    emit cloudPaginationChanged();
    emit cloudTasksChanged();
  }
}
int MultiMachineViewModel::cloudTotalPages() const {
  int count = m_cloudTasks.size();
  return (count == 0) ? 1 : (int)std::ceil((double)count / m_cloudPageSize);
}

// ── Send task to device (对齐上游 SendMultiMachinePage) ──

int MultiMachineViewModel::onlineMachineCount() const
{
  int count = 0;
  for (const auto &m : m_machineEntries)
    if (m.online) ++count;
  return count;
}

QString MultiMachineViewModel::onlineMachineName(int i) const
{
  int idx = 0;
  for (const auto &m : m_machineEntries)
  {
    if (!m.online) continue;
    if (idx == i) return m.name;
    ++idx;
  }
  return {};
}

bool MultiMachineViewModel::sendTaskToDevice(int localTaskIndex, int onlineMachineIndex)
{
  // 对齐上游 MultiMachinePickPage::on_ok / SendMultiMachinePage
  if (localTaskIndex < 0 || localTaskIndex >= m_localTasks.size())
    return false;

  const QString devName = onlineMachineName(onlineMachineIndex);
  if (devName.isEmpty())
    return false;

  m_localTasks[localTaskIndex].devName = devName;
  m_localTasks[localTaskIndex].status = 1;  // sending
  m_localTasks[localTaskIndex].progress = 0;
  m_localTasks[localTaskIndex].sendingPercent = 0;
  emit localTasksChanged();
  emit messageRequested(QObject::tr("正在发送任务到 %1...").arg(devName));

  // Mock: simulate sending progress (aligns with upstream m_sending_percent update)
  QTimer *progressTimer = new QTimer(this);
  progressTimer->setInterval(200);
  QSharedPointer<int> step = QSharedPointer<int>::create(0);
  connect(progressTimer, &QTimer::timeout, this, [this, progressTimer, localTaskIndex, devName, step]() {
    if (localTaskIndex >= m_localTasks.size() || *step >= 10) {
      progressTimer->stop();
      progressTimer->deleteLater();
      if (localTaskIndex < m_localTasks.size()) {
        m_localTasks[localTaskIndex].status = 5;  // printing
        m_localTasks[localTaskIndex].progress = 10;
        m_localTasks[localTaskIndex].sendingPercent = 100;
        emit localTasksChanged();
        emit messageRequested(QObject::tr("任务已发送到 %1").arg(devName));
      }
      return;
    }
    ++(*step);
    m_localTasks[localTaskIndex].sendingPercent = (*step) * 10;
    emit localTasksChanged();
  });
  progressTimer->start();

  return true;
}
