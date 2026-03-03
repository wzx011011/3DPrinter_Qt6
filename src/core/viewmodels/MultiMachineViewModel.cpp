#include "MultiMachineViewModel.h"

MultiMachineViewModel::MultiMachineViewModel(QObject *parent) : QObject(parent)
{
  // Store as structs to avoid QVariantList member corruption by V4 GC
  m_machineEntries = {
      {"K1C-001", "K1C", "打印中", "01:23:00", "192.168.1.101", true, false, 67},
      {"K1-Max-002", "K1 Max", "空闲", "—", "192.168.1.102", true, false, 0},
      {"Ender-3S1", "Ender-3 S1 Pro", "离线", "—", "—", false, false, 0},
      {"K2-Plus-004", "K2 Plus", "预热中", "—", "192.168.1.104", true, false, 0},
  };
}

QVariantList MultiMachineViewModel::machines() const
{
  QVariantList result;
  result.reserve(m_machineEntries.size());
  for (const auto &e : m_machineEntries)
  {
    result.append(QVariantMap{{"name", e.name}, {"model", e.model}, {"status", e.status}, {"remaining", e.remaining}, {"ip", e.ip}, {"online", e.online}, {"selected", e.selected}, {"progress", e.progress}});
  }
  return result;
}

QVariantList MultiMachineViewModel::taskQueue() const
{
  return {}; // empty task queue for mock
}

int MultiMachineViewModel::machineCount() const { return m_machineEntries.size(); }
QString MultiMachineViewModel::machineName(int i) const { return (i >= 0 && i < m_machineEntries.size()) ? m_machineEntries[i].name : QString{}; }
QString MultiMachineViewModel::machineModel(int i) const { return (i >= 0 && i < m_machineEntries.size()) ? m_machineEntries[i].model : QString{}; }
QString MultiMachineViewModel::machineStatus(int i) const { return (i >= 0 && i < m_machineEntries.size()) ? m_machineEntries[i].status : QString{}; }
bool MultiMachineViewModel::machineOnline(int i) const { return (i >= 0 && i < m_machineEntries.size()) ? m_machineEntries[i].online : false; }
bool MultiMachineViewModel::machineSelected(int i) const { return (i >= 0 && i < m_machineEntries.size()) ? m_machineEntries[i].selected : false; }
int MultiMachineViewModel::machineProgress(int i) const { return (i >= 0 && i < m_machineEntries.size()) ? m_machineEntries[i].progress : 0; }
QString MultiMachineViewModel::machineRemaining(int i) const { return (i >= 0 && i < m_machineEntries.size()) ? m_machineEntries[i].remaining : QString{}; }
QString MultiMachineViewModel::machineIp(int i) const { return (i >= 0 && i < m_machineEntries.size()) ? m_machineEntries[i].ip : QString{}; }
int MultiMachineViewModel::taskQueueCount() const { return 0; }

void MultiMachineViewModel::selectMachine(int index)
{
  for (int i = 0; i < m_machineEntries.size(); ++i)
  {
    m_machineEntries[i].selected = (i == index) ? !m_machineEntries[i].selected : false;
  }
  emit machinesChanged();
}

void MultiMachineViewModel::sendToAll() {}
void MultiMachineViewModel::pauseMachine(int index) { Q_UNUSED(index) }
void MultiMachineViewModel::stopMachine(int index) { Q_UNUSED(index) }
void MultiMachineViewModel::refreshMachines() { emit machinesChanged(); }
