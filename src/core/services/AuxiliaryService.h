#pragma once

#include <QObject>
#include <QVariantList>
#include <QString>

/// Auxiliary file management service (对齐上游 GUI_AuxiliaryList / AuxiliaryModel)
/// Manages supplementary files (images, notes, references) attached to a print project.
/// Files are organized into folders under a temp basePath on disk.
class AuxiliaryService final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QVariantList files READ files NOTIFY filesChanged)

public:
  explicit AuxiliaryService(QObject *parent = nullptr);

  QVariantList files() const;

  /// Import a file into the auxiliary file store (对齐 upstream do_import_file)
  Q_INVOKABLE void importFile(const QString &path, const QString &folderPath = {});
  /// Create a new folder (对齐 upstream create_new_folder)
  Q_INVOKABLE void createFolder(const QString &name, const QString &parentPath = {});
  /// Delete a file or folder (对齐 upstream AuxiliaryModel::Delete)
  Q_INVOKABLE void deleteItem(const QString &path);
  /// Rename a file or folder (对齐 upstream on_editing_done)
  Q_INVOKABLE void renameItem(const QString &path, const QString &newName);
  /// Open file with system default application (对齐 upstream wxLaunchDefaultApplication)
  Q_INVOKABLE void openFile(const QString &path);
  /// Reload file list from disk (对齐 upstream reload)
  Q_INVOKABLE void refresh();

signals:
  void filesChanged();

private:
  QVariantList files_;
  QString basePath_;

  void loadFiles();
  void ensureBasePathExists();
};
