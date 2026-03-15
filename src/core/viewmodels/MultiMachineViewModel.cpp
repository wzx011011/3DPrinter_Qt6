#include "MultiMachineViewModel.h"
#include <algorithm>

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
static const char *localTaskStatusTextFn(int state) {
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
static const char *cloudTaskStatusTextFn(int state) {
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

  // Timer-based refresh (aligns with upstream m_refresh_timer 2s in MultiMachinePage::Show)
  m_refreshTimer.setInterval(2000);
  connect(&m_refreshTimer, &QTimer::timeout, this, &MultiMachineViewModel::simulateMockStateUpdate);
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

  // 5 mock local tasks (more diverse states to exercise all UI paths)
  // Aligns with upstream LocalTaskManagerPage / MultiTaskItem
  m_localTasks = {
      {"Benchy.gcode",      "K1C-001",      "2026-03-14 09:30", "01:23:00", 5, 67,  false},  // printing
      {"Cube.stl",          "K2-Plus-004",  "2026-03-14 09:15", "--",        0, 0,   false},  // pending
      {"Bracket_v2.3mf",    "K1-Max-002",   "2026-03-14 08:00", "--",        6, 100, false},  // success
      {"CalibrationCube.gcode", "K1C-001",  "2026-03-14 10:00", "--",        1, 45,  false},  // sending (in progress)
      {"Phone_Stand.stl",   "Ender-3S1-Pro","2026-03-14 07:30", "--",        3, 0,   false},  // cancelled
  };

  // 12 mock cloud tasks to exercise pagination (aligns with upstream m_count_page_item=10)
  m_cloudTasks = {
      {"CalibrationCube.gcode",  "K1C-001",      "2026-03-13 14:00", "--",        1, 100},
      {"Gear_Assembly.gcode",    "K1-Max-002",   "2026-03-13 16:30", "02:15:30", 0, 42},
      {"Phone_Stand.stl",        "K2-Plus-004",  "2026-03-13 10:00", "--",        2, 88},
      {"Benchy_Third.gcode",     "K1C-001",      "2026-03-12 09:00", "--",        1, 100},
      {"Support_Test.gcode",     "K1-Max-002",   "2026-03-12 11:30", "00:45:10", 0, 73},
      {"Articulated_Dragon.stl", "K2-Plus-004",  "2026-03-11 15:00", "--",        1, 100},
      {"Miniature_Village.gcode","K1C-001",      "2026-03-11 10:00", "03:20:00", 0, 28},
      {"Flexi_Rex.stl",          "K1-Max-002",   "2026-03-10 14:30", "--",        2, 65},
      {"Hollow_Cube.gcode",      "K2-Plus-004",  "2026-03-10 09:00", "--",        1, 100},
      {"Lithophane_Box.gcode",   "K1C-001",      "2026-03-09 16:00", "--",        1, 100},
      {"Plant_Pot.stl",          "K1-Max-002",   "2026-03-09 11:00", "01:10:00", 0, 55},
      {"Wrench_Set.gcode",       "K2-Plus-004",  "2026-03-08 14:00", "--",        1, 100},
  };
}

// ──────────────────────────────────────────────
// Device accessors
// ──────────────────────────────────────────────
int MultiMachineViewModel::machineCount() const {
  // Return filtered count for current page (aligns with upstream pagination)
  int filteredSoFar = 0;
  int pageStart = m_currentPage * m_pageSize;
  int shown = 0;
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
// Sort (aligns with upstream SortItem)
// ──────────────────────────────────────────────
int MultiMachineViewModel::sortField() const { return m_sortField; }
bool MultiMachineViewModel::sortAsc() const {
  return m_sortField == 1 ? m_sortNameAsc : (m_sortField == 2 ? m_sortStatusAsc : true);
}

void MultiMachineViewModel::sortDevicesByName()
{
  m_sortNameAsc = !m_sortNameAsc;
  m_sortField = 1;
  std::sort(m_machineEntries.begin(), m_machineEntries.end(),
    [this](const MachineEntry &a, const MachineEntry &b) {
      return m_sortNameAsc ? a.name < b.name : a.name > b.name;
    });
  emit sortChanged();
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
  emit sortChanged();
  emit machinesChanged();
}

// ──────────────────────────────────────────────
// Pagination (Device)
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

void MultiMachineViewModel::recomputePagination()
{
  int total = totalPages();
  if (m_currentPage >= total && total > 0) m_currentPage = total - 1;
  emit paginationChanged();
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
  return (i >= 0 && i < m_localTasks.size()) ? QString(localTaskStatusTextFn(m_localTasks[i].status)) : QString{};
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
bool MultiMachineViewModel::localTaskSelected(int i) const {
  return (i >= 0 && i < m_localTasks.size()) ? m_localTasks[i].selected : false;
}

// ──────────────────────────────────────────────
// Cloud Task accessors (all, not page-aware)
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
  return (i >= 0 && i < m_cloudTasks.size()) ? QString(cloudTaskStatusTextFn(m_cloudTasks[i].status)) : QString{};
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
bool MultiMachineViewModel::cloudTaskSelected(int i) const {
  return (i >= 0 && i < m_cloudTasks.size()) ? m_cloudTasks[i].selected : false;
}
int MultiMachineViewModel::cloudSelectedCount() const {
  int c = 0;
  for (const auto &t : m_cloudTasks) if (t.selected) ++c;
  return c;
}

// ──────────────────────────────────────────────
// Cloud Task pagination (aligns with upstream CloudTaskManagerPage)
// ──────────────────────────────────────────────
int MultiMachineViewModel::cloudCurrentPage() const { return m_cloudCurrentPage; }
void MultiMachineViewModel::setCloudCurrentPage(int page) {
  if (page < 0) page = 0;
  if (page >= cloudTotalPages()) page = std::max(0, cloudTotalPages() - 1);
  if (page != m_cloudCurrentPage) {
    m_cloudCurrentPage = page;
    emit cloudPaginationChanged();
    emit cloudTasksChanged();
  }
}
int MultiMachineViewModel::cloudTotalPages() const {
  int total = m_cloudTasks.size();
  return (total == 0) ? 1 : (int)std::ceil((double)total / m_cloudPageSize);
}

void MultiMachineViewModel::recomputeCloudPagination() {
  int total = cloudTotalPages();
  if (m_cloudCurrentPage >= total && total > 0) m_cloudCurrentPage = total - 1;
  emit cloudPaginationChanged();
}

// Paged cloud task accessors (translate page-local index to global)
int MultiMachineViewModel::pagedCloudTaskCount() const {
  int start = m_cloudCurrentPage * m_cloudPageSize;
  int end = std::min(start + m_cloudPageSize, m_cloudTasks.size());
  return std::max(0, end - start);
}

static int pagedCloudIndex(int cloudCurrentPage, int cloudPageSize, int localI) {
  return cloudCurrentPage * cloudPageSize + localI;
}

QString MultiMachineViewModel::pagedCloudTaskProjectName(int i) const {
  int idx = pagedCloudIndex(m_cloudCurrentPage, m_cloudPageSize, i);
  return cloudTaskProjectName(idx);
}
QString MultiMachineViewModel::pagedCloudTaskDevName(int i) const {
  int idx = pagedCloudIndex(m_cloudCurrentPage, m_cloudPageSize, i);
  return cloudTaskDevName(idx);
}
int MultiMachineViewModel::pagedCloudTaskStatus(int i) const {
  int idx = pagedCloudIndex(m_cloudCurrentPage, m_cloudPageSize, i);
  return cloudTaskStatus(idx);
}
QString MultiMachineViewModel::pagedCloudTaskStatusText(int i) const {
  int idx = pagedCloudIndex(m_cloudCurrentPage, m_cloudPageSize, i);
  return cloudTaskStatusText(idx);
}
int MultiMachineViewModel::pagedCloudTaskProgress(int i) const {
  int idx = pagedCloudIndex(m_cloudCurrentPage, m_cloudPageSize, i);
  return cloudTaskProgress(idx);
}
QString MultiMachineViewModel::pagedCloudTaskSendTime(int i) const {
  int idx = pagedCloudIndex(m_cloudCurrentPage, m_cloudPageSize, i);
  return cloudTaskSendTime(idx);
}
QString MultiMachineViewModel::pagedCloudTaskRemaining(int i) const {
  int idx = pagedCloudIndex(m_cloudCurrentPage, m_cloudPageSize, i);
  return cloudTaskRemaining(idx);
}
bool MultiMachineViewModel::pagedCloudTaskSelected(int i) const {
  int idx = pagedCloudIndex(m_cloudCurrentPage, m_cloudPageSize, i);
  return cloudTaskSelected(idx);
}

// ──────────────────────────────────────────────
// Device actions
// ──────────────────────────────────────────────
void MultiMachineViewModel::viewMachine(int index)
{
  // Aligns with upstream EVT_MULTI_DEVICE_VIEW -> jump_to_monitor
  int idx = m_currentPage * m_pageSize + index;
  if (idx >= 0 && idx < m_machineEntries.size()) {
    emit messageRequested(tr("View device: %1").arg(m_machineEntries[idx].name));
  }
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
  // Aligns with upstream clear_page() -> refresh_user_device()
  emit machinesChanged();
  emit localTasksChanged();
  emit cloudTasksChanged();
}

// ──────────────────────────────────────────────
// Local Task actions (aligns with upstream LocalTaskManagerPage)
// ──────────────────────────────────────────────
void MultiMachineViewModel::selectLocalTask(int index)
{
  // Aligns with upstream EVT_MULTI_DEVICE_SELECTED for local tasks
  // Selection only allowed for pending/sending tasks (upstream: state_local_task <= 1)
  if (index >= 0 && index < m_localTasks.size() && m_localTasks[index].status <= 1) {
    m_localTasks[index].selected = !m_localTasks[index].selected;
    emit localTasksChanged();
  }
}

void MultiMachineViewModel::cancelLocalTask(int index)
{
  // Aligns with upstream MultiTaskItem::onCancel
  // Cancel only allowed for pending/sending tasks
  if (index >= 0 && index < m_localTasks.size()) {
    if (m_localTasks[index].status == 0 || m_localTasks[index].status == 1) {
      m_localTasks[index].status = 3; // Sending Cancel
      m_localTasks[index].selected = false;
      emit localTasksChanged();
      emit messageRequested(tr("Task cancelled: %1").arg(m_localTasks[index].projectName));
    }
  }
}

void MultiMachineViewModel::sortDevicesByProgress()
{
  m_sortProgressAsc = !m_sortProgressAsc;
  m_sortField = 3;
  std::sort(m_machineEntries.begin(), m_machineEntries.end(),
    [this](const MachineEntry &a, const MachineEntry &b) {
      return m_sortProgressAsc ? a.progress < b.progress : a.progress > b.progress;
    });
  emit machinesChanged();
}

void MultiMachineViewModel::sortDevicesByTaskName()
{
  m_sortTaskNameAsc = !m_sortTaskNameAsc;
  m_sortField = 4;
  std::sort(m_machineEntries.begin(), m_machineEntries.end(),
    [this](const MachineEntry &a, const MachineEntry &b) {
      int cmp = a.taskName.compare(b.taskName, Qt::CaseInsensitive);
      return m_sortTaskNameAsc ? cmp < 0 : cmp > 0;
    });
  emit machinesChanged();
}

bool MultiMachineViewModel::currentSortAsc() const
{
  switch (m_sortField) {
  case 1: return m_sortNameAsc;
  case 2: return m_sortStatusAsc;
  case 3: return m_sortProgressAsc;
  case 4: return m_sortTaskNameAsc;
  default: return true;
  }
}

void MultiMachineViewModel::recomputePagination()
void MultiMachineViewModel::stopAllLocalTasks()
{
  // Aligns with upstream btn_stop_all -> cancel_all
  for (auto &t : m_localTasks) {
    if (t.selected && (t.status == 0 || t.status == 1)) {
      t.status = 3; // cancel
      t.selected = false;
    }
  }
  emit localTasksChanged();
  emit messageRequested(tr("All selected tasks cancelled"));
}

// ──────────────────────────────────────────────
// Cloud Task actions (aligns with upstream CloudTaskManagerPage)
// ──────────────────────────────────────────────
void MultiMachineViewModel::selectCloudTask(int index)
{
  // Aligns with upstream EVT_MULTI_DEVICE_SELECTED for cloud tasks
  // Selection only allowed for printing tasks (upstream: state_cloud_task == 0)
  if (index >= 0 && index < m_cloudTasks.size() && m_cloudTasks[index].status == 0) {
    m_cloudTasks[index].selected = !m_cloudTasks[index].selected;
    emit cloudTasksChanged();
  }
}

void MultiMachineViewModel::pauseCloudTask(int index)
{
  // Aligns with upstream MultiTaskItem::onPause
  // Only pause if currently printing (status == 0)
  if (index >= 0 && index < m_cloudTasks.size()) {
    if (m_cloudTasks[index].status == 0) {
      m_cloudTasks[index].status = 4; // paused (local display state, mapped to "Paused")
      emit cloudTasksChanged();
      emit messageRequested(tr("Cloud task paused: %1").arg(m_cloudTasks[index].projectName));
    }
  }
}

void MultiMachineViewModel::resumeCloudTask(int index)
{
  // Aligns with upstream MultiTaskItem::onResume
  if (index >= 0 && index < m_cloudTasks.size()) {
    if (m_cloudTasks[index].status == 4) { // paused
      m_cloudTasks[index].status = 0; // back to printing
      emit cloudTasksChanged();
      emit messageRequested(tr("Cloud task resumed: %1").arg(m_cloudTasks[index].projectName));
    }
  }
}

void MultiMachineViewModel::stopCloudTask(int index)
{
  // Aligns with upstream MultiTaskItem::onStop -> command_task_abort
  if (index >= 0 && index < m_cloudTasks.size()) {
    if (m_cloudTasks[index].status == 0 || m_cloudTasks[index].status == 4) {
      m_cloudTasks[index].status = 2; // failed
      m_cloudTasks[index].selected = false;
      emit cloudTasksChanged();
      emit messageRequested(tr("Cloud task stopped: %1").arg(m_cloudTasks[index].projectName));
    }
  }
}

void MultiMachineViewModel::pauseAllCloudTasks()
{
  // Aligns with upstream btn_pause_all
  for (auto &t : m_cloudTasks) {
    if (t.selected && t.status == 0) {
      t.status = 4; // paused
    }
  }
  emit cloudTasksChanged();
  emit messageRequested(tr("All selected cloud tasks paused"));
}

void MultiMachineViewModel::resumeAllCloudTasks()
{
  // Aligns with upstream btn_continue_all
  for (auto &t : m_cloudTasks) {
    if (t.selected && t.status == 4) {
      t.status = 0; // printing
    }
  }
  emit cloudTasksChanged();
  emit messageRequested(tr("All selected cloud tasks resumed"));
}

void MultiMachineViewModel::stopAllCloudTasks()
{
  // Aligns with upstream btn_stop_all
  for (auto &t : m_cloudTasks) {
    if (t.selected && (t.status == 0 || t.status == 4)) {
      t.status = 2; // failed
      t.selected = false;
    }
  }
  emit cloudTasksChanged();
  emit messageRequested(tr("All selected cloud tasks stopped"));
}

// ──────────────────────────────────────────────
// Send task to device (aligns with upstream SendMultiMachinePage)
// ──────────────────────────────────────────────
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
  if (localTaskIndex < 0 || localTaskIndex >= m_localTasks.size())
    return false;

  const QString devName = onlineMachineName(onlineMachineIndex);
  if (devName.isEmpty())
    return false;

  m_localTasks[localTaskIndex].devName = devName;
  m_localTasks[localTaskIndex].status = 1;  // sending
  m_localTasks[localTaskIndex].progress = 0;
  emit localTasksChanged();
  emit messageRequested(QObject::tr("Sending task to %1...").arg(devName));

  // Mock: simulate sending process (2s then becomes printing)
  QTimer::singleShot(2000, this, [this, localTaskIndex, devName]() {
    if (localTaskIndex < m_localTasks.size())
    {
      m_localTasks[localTaskIndex].status = 5;  // printing
      m_localTasks[localTaskIndex].progress = 10;
      emit localTasksChanged();
      emit messageRequested(QObject::tr("Task sent to %1").arg(devName));
    }
  });

  return true;
}

// ──────────────────────────────────────────────
// Timer-based refresh (aligns with upstream m_refresh_timer)
// ──────────────────────────────────────────────
void MultiMachineViewModel::startRefreshTimer()
{
  // Aligns with upstream MultiMachinePage::Show -> m_refresh_timer->Start(2000)
  if (!m_refreshTimer.isActive()) {
    m_refreshTimer.start();
  }
}

void MultiMachineViewModel::stopRefreshTimer()
{
  // Aligns with upstream MultiMachinePage::Show(false) -> m_refresh_timer->Stop()
  m_refreshTimer.stop();
}

void MultiMachineViewModel::simulateMockStateUpdate()
{
  // Aligns with upstream on_timer -> update_page()
  // Simulate state changes for mock data (progress advancement, etc.)
  bool machinesChanged = false;
  bool localChanged = false;
  bool cloudChanged = false;

  // Advance printing device progress
  for (auto &m : m_machineEntries) {
    if (m.statusInt == 3 && m.progress < 100) { // RUNNING
      m.progress = std::min(100, m.progress + 1);
      machinesChanged = true;
    }
    if (m.statusInt == 5) { // PREPARE -> auto transition to RUNNING
      m.statusInt = 3;
      m.progress = 0;
      m.remaining = "02:00:00";
      m.taskName = m.taskName.isEmpty() ? "AutoTask.gcode" : m.taskName;
      machinesChanged = true;
    }
  }

  // Advance local printing tasks
  for (auto &t : m_localTasks) {
    if (t.status == 5 && t.progress < 100) { // printing
      t.progress = std::min(100, t.progress + 1);
      localChanged = true;
    }
    if (t.status == 1 && t.progress < 100) { // sending
      t.progress = std::min(100, t.progress + 5);
      localChanged = true;
    }
  }

  // Advance cloud printing tasks
  for (auto &t : m_cloudTasks) {
    if (t.status == 0 && t.progress < 100) { // printing
      t.progress = std::min(100, t.progress + 1);
      cloudChanged = true;
    }
  }

  if (machinesChanged) emit machinesChanged();
  if (localChanged) emit localTasksChanged();
  if (cloudChanged) emit cloudTasksChanged();
}
