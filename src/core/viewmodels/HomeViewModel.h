#pragma once
#include <QObject>
#include <QVariantList>
#include <QString>

class CloudServiceMock;

class HomeViewModel : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QVariantList recentProjects READ recentProjects NOTIFY recentProjectsChanged)

  /// Cloud account (对齐 upstream NetworkAgent / WebUserLoginDialog)
  Q_PROPERTY(bool cloudLoggedIn READ cloudLoggedIn NOTIFY cloudStateChanged)
  Q_PROPERTY(QString cloudUserName READ cloudUserName NOTIFY cloudStateChanged)
  Q_PROPERTY(QString cloudUserEmail READ cloudUserEmail NOTIFY cloudStateChanged)
  Q_PROPERTY(int cloudBoundDeviceCount READ cloudBoundDeviceCount NOTIFY cloudStateChanged)
  Q_PROPERTY(bool cloudSyncing READ cloudSyncing NOTIFY cloudStateChanged)
  Q_PROPERTY(QString cloudLastSyncTime READ cloudLastSyncTime NOTIFY cloudStateChanged)

public:
  explicit HomeViewModel(CloudServiceMock *cloudService, QObject *parent = nullptr);

  QVariantList recentProjects() const;

  // Individual item accessors - use these from QML to avoid Qt6 V4 VariantAssociationObject crash
  Q_INVOKABLE int recentProjectCount() const;
  Q_INVOKABLE QString recentProjectName(int i) const;
  Q_INVOKABLE QString recentProjectDate(int i) const;
  Q_INVOKABLE QString recentProjectPath(int i) const;

  // Cloud account actions (对齐 upstream NetworkAgent / BindDialog)
  Q_INVOKABLE void cloudLogin(const QString &user, const QString &password);
  Q_INVOKABLE void cloudLogout();
  Q_INVOKABLE void cloudBindDevice(const QString &name, const QString &pin);
  Q_INVOKABLE void cloudUnbindDevice(int index);
  Q_INVOKABLE QVariantMap cloudBoundDeviceAt(int index) const;
  Q_INVOKABLE void cloudSyncPresets();

  bool cloudLoggedIn() const;
  QString cloudUserName() const;
  QString cloudUserEmail() const;
  int cloudBoundDeviceCount() const;
  bool cloudSyncing() const;
  QString cloudLastSyncTime() const;

signals:
  void recentProjectsChanged();
  void cloudStateChanged();
  void cloudLoginFailed(const QString &error);

public slots:
  void openProject(const QString &path);
  void openRecentProject(int index);
  void refreshRecentProjects();

private:
  CloudServiceMock *cloudService_ = nullptr;

  // Stored as plain structs to avoid QVariantList member corruption by V4 GC
  struct ProjectEntry
  {
    QString name, date, path;
  };
  QList<ProjectEntry> m_entries;
};
