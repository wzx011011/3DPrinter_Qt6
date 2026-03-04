#pragma once
#include <QObject>
#include <QVariantList>
#include <QString>
#include <QStringList>

class ProjectViewModel : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString currentProjectPath READ currentProjectPath NOTIFY projectChanged)
  Q_PROPERTY(bool isDirty READ isDirty NOTIFY dirtyChanged)
  Q_PROPERTY(QVariantList fileTree READ fileTree NOTIFY projectChanged)
  Q_PROPERTY(QString selectedFile READ selectedFile NOTIFY selectionChanged)
  Q_PROPERTY(QStringList recentProjects READ recentProjects NOTIFY recentChanged)

public:
  explicit ProjectViewModel(QObject *parent = nullptr);

  QString currentProjectPath() const { return m_currentProjectPath; }
  bool isDirty() const { return m_isDirty; }
  QVariantList fileTree() const;

  // Individual item accessors - use these from QML to avoid Qt6 V4 VariantAssociationObject crash
  Q_INVOKABLE int fileTreeCount() const;
  Q_INVOKABLE QString fileTreeName(int i) const;
  Q_INVOKABLE bool fileTreeIsDir(int i) const;
  Q_INVOKABLE int fileTreeDepth(int i) const;

  QString selectedFile() const { return m_selectedFile; }
  QStringList recentProjects() const { return m_recentProjects; }

signals:
  void projectChanged();
  void dirtyChanged();
  void selectionChanged();
  void recentChanged();

public slots:
  void newProject();
  void openProject(const QString &path);
  void saveProject();
  void saveProjectAs(const QString &path);
  void importModel(const QStringList &paths);
  void selectFile(const QString &path);
  void clearRecentProjects();

private:
  struct FileEntry
  {
    QString name;
    bool isDir;
    int depth;
  };
  QList<FileEntry> m_fileEntries;
  QString m_currentProjectPath;
  bool m_isDirty = false;
  QString m_selectedFile;
  QStringList m_recentProjects;
};
