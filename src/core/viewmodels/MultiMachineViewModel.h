#pragma once
#include <QObject>
#include <QVariantList>
#include <QTimer>

// Aligns with upstream MultiMachinePage (3 tabs), MultiMachineManagerPage (device list),
// MultiTaskManagerPage (task sending/sent), and SendMultiMachinePage.

class MultiMachineViewModel : public QObject
{
  Q_OBJECT

  // -- Device Manager tab --
  Q_PROPERTY(int machineCount READ machineCount NOTIFY machinesChanged)
  Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY paginationChanged)
  Q_PROPERTY(int totalPages READ totalPages NOTIFY paginationChanged)
  Q_PROPERTY(int pageSize READ pageSize CONSTANT)
  Q_PROPERTY(QString selectedCountText READ selectedCountText NOTIFY machinesChanged)
  Q_PROPERTY(bool hasDevices READ hasDevices NOTIFY machinesChanged)
  Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY machinesChanged)
  Q_PROPERTY(int filteredMachineCount READ filteredMachineCount NOTIFY machinesChanged)

  // -- Task Sending tab --
  Q_PROPERTY(int localTaskCount READ localTaskCount NOTIFY localTasksChanged)
  Q_PROPERTY(bool hasLocalTasks READ hasLocalTasks NOTIFY localTasksChanged)
  Q_PROPERTY(int localSelectedCount READ localSelectedCount NOTIFY localTasksChanged)

  // -- Task Sent tab --
  Q_PROPERTY(int cloudTaskCount READ cloudTaskCount NOTIFY cloudTasksChanged)
  Q_PROPERTY(bool hasCloudTasks READ hasCloudTasks NOTIFY cloudTasksChanged)

public:
  explicit MultiMachineViewModel(QObject *parent = nullptr);

  // ── Device Manager accessors (Q_INVOKABLE for QML safety) ──
  Q_INVOKABLE int machineCount() const;
  Q_INVOKABLE QString machineName(int i) const;
  Q_INVOKABLE QString machineModel(int i) const;
  Q_INVOKABLE QString machineStatus(int i) const;     // upstream state_device mapped to string
  Q_INVOKABLE int machineStatusInt(int i) const;       // 0-idle 1-finish 2-failed 3-printing 4-pause 5-prepare 6-slicing 7-unknown
  Q_INVOKABLE bool machineOnline(int i) const;
  Q_INVOKABLE bool machineSelected(int i) const;
  Q_INVOKABLE int machineProgress(int i) const;
  Q_INVOKABLE QString machineRemaining(int i) const;
  Q_INVOKABLE QString machineIp(int i) const;
  Q_INVOKABLE QString machineTaskName(int i) const;    // upstream subtask_name / "No task"
  Q_INVOKABLE int machinePageCount() const;            // total device count across pages
  Q_INVOKABLE bool hasDevices() const;
  Q_INVOKABLE int filteredMachineCount() const;
  QString searchText() const;
  void setSearchText(const QString &text);

  // Pagination (aligns with upstream m_current_page / m_total_page)
  int currentPage() const;
  void setCurrentPage(int page);
  int totalPages() const;
  int pageSize() const;
  QString selectedCountText() const;

  // ── Local Task (Task Sending) accessors ──
  Q_INVOKABLE int localTaskCount() const;
  Q_INVOKABLE QString localTaskProjectName(int i) const;
  Q_INVOKABLE QString localTaskDevName(int i) const;
  Q_INVOKABLE int localTaskStatus(int i) const;        // 0-pending 1-sending 2-finish 3-cancel 4-failed 5-printing 6-success 7-removed 8-idle
  Q_INVOKABLE QString localTaskStatusText(int i) const;
  Q_INVOKABLE int localTaskProgress(int i) const;
  Q_INVOKABLE QString localTaskSendTime(int i) const;
  Q_INVOKABLE QString localTaskRemaining(int i) const;
  Q_INVOKABLE bool localTaskSelected(int i) const;
  Q_INVOKABLE bool hasLocalTasks() const;
  Q_INVOKABLE int localSelectedCount() const;

  // ── Cloud Task (Task Sent) accessors ──
  Q_INVOKABLE int cloudTaskCount() const;
  Q_INVOKABLE QString cloudTaskProjectName(int i) const;
  Q_INVOKABLE QString cloudTaskDevName(int i) const;
  Q_INVOKABLE int cloudTaskStatus(int i) const;        // 0-printing 1-finish 2-failed
  Q_INVOKABLE QString cloudTaskStatusText(int i) const;
  Q_INVOKABLE int cloudTaskProgress(int i) const;
  Q_INVOKABLE QString cloudTaskSendTime(int i) const;
  Q_INVOKABLE QString cloudTaskRemaining(int i) const;
  Q_INVOKABLE bool hasCloudTasks() const;

  // ── Device actions (aligns with upstream MultiMachineItem EVT_MULTI_DEVICE_VIEW) ──
  Q_INVOKABLE void viewMachine(int index);

  // ── Task actions ──
  Q_INVOKABLE void selectLocalTask(int index);
  Q_INVOKABLE void cancelLocalTask(int index);
  Q_INVOKABLE void stopAllLocalTasks();
  Q_INVOKABLE void pauseCloudTask(int index);
  Q_INVOKABLE void resumeCloudTask(int index);
  Q_INVOKABLE void stopAllCloudTasks();

  // ── Device management (aligns with upstream MultiMachinePickPage "Edit Printers") ──
  Q_INVOKABLE void editPrinters();
  Q_INVOKABLE void addDevice();
  Q_INVOKABLE void removeDevice(int index);
  Q_INVOKABLE void refreshMachines();

  // ── Sort (aligns with upstream SortItem) ──
  Q_INVOKABLE void sortDevicesByName();    // toggles ascending/descending
  Q_INVOKABLE void sortDevicesByStatus();  // toggles ascending/descending

  // ── Send task to device (对齐上游 SendMultiMachinePage / MultiMachinePickPage) ──
  Q_INVOKABLE int onlineMachineCount() const;
  Q_INVOKABLE QString onlineMachineName(int i) const;
  Q_INVOKABLE bool sendTaskToDevice(int localTaskIndex, int onlineMachineIndex);

signals:
  void machinesChanged();
  void paginationChanged();
  void localTasksChanged();
  void cloudTasksChanged();
  void messageRequested(QString text);

private:
  // Machine data stored as structs (avoids QVariantList V4 GC crash)
  struct MachineEntry {
    QString name, model, status, remaining, ip, taskName, sendTime;
    bool online;
    int statusInt;   // 0-idle 1-finish 2-failed 3-printing 4-pause 5-prepare 6-slicing 7-unknown
    int progress;
    bool filteredIn = true;  // search filter state
  };
  QList<MachineEntry> m_machineEntries;

  // Local task data (aligns with upstream MultiTaskItem m_task_type=0)
  struct LocalTaskEntry {
    QString projectName, devName, sendTime, remaining;
    int status;     // 0-pending 1-sending 2-finish 3-cancel 4-failed 5-printing 6-success 7-removed 8-idle
    int progress;
    bool selected;
  };
  QList<LocalTaskEntry> m_localTasks;

  // Cloud task data (aligns with upstream MultiTaskItem m_task_type=1)
  struct CloudTaskEntry {
    QString projectName, devName, sendTime, remaining;
    int status;     // 0-printing 1-finish 2-failed
    int progress;
  };
  QList<CloudTaskEntry> m_cloudTasks;

  // Pagination
  int m_currentPage = 0;
  static const int m_pageSize = 10;

  // Sort state
  bool m_sortNameAsc = true;
  bool m_sortStatusAsc = true;
  int m_sortField = 0;  // 0=none 1=name 2=status

  // Search/filter (aligns with upstream selected_machines / search)
  QString m_searchText;

  void buildMockData();
  void recomputePagination();
  static const MachineEntry *filteredMachineAt(const QList<MachineEntry> &list, int page, int pageSize, int i);
};
