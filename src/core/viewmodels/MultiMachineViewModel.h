#pragma once
#include <QObject>
#include <QVariantList>

class MultiMachineViewModel : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QVariantList machines READ machines NOTIFY machinesChanged)
  Q_PROPERTY(QVariantList taskQueue READ taskQueue NOTIFY taskQueueChanged)

public:
  explicit MultiMachineViewModel(QObject *parent = nullptr);

  QVariantList machines() const;
  QVariantList taskQueue() const;

  // Individual item accessors - use these from QML to avoid Qt6 V4 VariantAssociationObject crash
  Q_INVOKABLE int machineCount() const;
  Q_INVOKABLE QString machineName(int i) const;
  Q_INVOKABLE QString machineModel(int i) const;
  Q_INVOKABLE QString machineStatus(int i) const;
  Q_INVOKABLE bool machineOnline(int i) const;
  Q_INVOKABLE bool machineSelected(int i) const;
  Q_INVOKABLE int machineProgress(int i) const;
  Q_INVOKABLE QString machineRemaining(int i) const;
  Q_INVOKABLE QString machineIp(int i) const;
  Q_INVOKABLE int taskQueueCount() const;

signals:
  void machinesChanged();
  void taskQueueChanged();

public slots:
  void selectMachine(int index);
  void sendToAll();
  void pauseMachine(int index);
  void stopMachine(int index);
  void refreshMachines();

private:
  // Store as structs - not QVariantList member to prevent V4 GC destructor crash
  struct MachineEntry
  {
    QString name, model, status, remaining, ip;
    bool online, selected;
    int progress;
  };
  QList<MachineEntry> m_machineEntries;
};
