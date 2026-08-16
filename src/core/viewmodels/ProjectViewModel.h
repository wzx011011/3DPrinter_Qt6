#pragma once
#include <QObject>
#include <QVariantList>
#include <QString>
#include <QStringList>

class ProjectServiceMock;

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

  /// Phase 241 (PAGE-02): inject the project data service so the file tree /
  /// details read the REAL loaded project (plates + object source modules)
  /// instead of a hardcoded list. Without a service the tree renders the
  /// honest empty state when no project path is set.
  void setProjectService(ProjectServiceMock *projectService);

  QString currentProjectPath() const { return m_currentProjectPath; }
  bool isDirty() const { return m_isDirty; }
  QVariantList fileTree() const;

  // Individual item accessors - use these from QML to avoid Qt6 V4 VariantAssociationObject crash
  Q_INVOKABLE int fileTreeCount() const;
  Q_INVOKABLE QString fileTreeName(int i) const;
  Q_INVOKABLE bool fileTreeIsDir(int i) const;
  Q_INVOKABLE int fileTreeDepth(int i) const;

  /// Phase 241 (PAGE-02): QFileInfo-derived details for the properties panel
  /// (upstream ProjectPage-style metadata). Empty strings when no project.
  Q_INVOKABLE QString projectFileSizeText() const;
  Q_INVOKABLE QString projectLastModifiedText() const;

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
  /// v5.16 (PLATE-05): real edit flows (object/plate/config mutations via
  /// ProjectServiceMock::projectChanged) mark the project dirty; the guard
  /// dialogs and title indicator consume it. Loading does NOT mark dirty.
  void markDirty()
  {
    if (!m_isDirty) {
      m_isDirty = true;
      emit dirtyChanged();
    }
  }
  /// Phase 241 (PAGE-02): rebuild the resource tree from the live project
  /// service state. Called by the QML page on projectChanged and after
  /// object-level edits so the tree does not go stale.
  Q_INVOKABLE void refreshFileTree();

private:
  void persistRecentProjects();

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
  ProjectServiceMock *m_projectService = nullptr;
};
