#pragma once
#include <QObject>
#include <QVariantList>
#include <QString>

class HomeViewModel : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QVariantList recentProjects READ recentProjects NOTIFY recentProjectsChanged)

public:
  explicit HomeViewModel(QObject *parent = nullptr);

  QVariantList recentProjects() const;

  // Individual item accessors - use these from QML to avoid Qt6 V4 VariantAssociationObject crash
  Q_INVOKABLE int recentProjectCount() const;
  Q_INVOKABLE QString recentProjectName(int i) const;
  Q_INVOKABLE QString recentProjectDate(int i) const;
  Q_INVOKABLE QString recentProjectPath(int i) const;

signals:
  void recentProjectsChanged();

public slots:
  void openProject(const QString &path);
  void openRecentProject(int index);
  void refreshRecentProjects();

private:
  // Stored as plain structs to avoid QVariantList member corruption by V4 GC
  struct ProjectEntry
  {
    QString name, date, path;
  };
  QList<ProjectEntry> m_entries;
};
