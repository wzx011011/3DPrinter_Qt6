#include "AuxiliaryService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>
#include <QDesktopServices>
#include <QDebug>

AuxiliaryService::AuxiliaryService(QObject *parent)
    : QObject(parent)
    , basePath_(QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                QStringLiteral("/creality_auxiliary"))
{
  ensureBasePathExists();
  loadFiles();
}

QVariantList AuxiliaryService::files() const
{
  return files_;
}

void AuxiliaryService::importFile(const QString &path, const QString &folderPath)
{
  if (path.isEmpty())
    return;

  const QUrl url(path);
  QString srcPath = url.isLocalFile() ? url.toLocalFile() : path;

  QFileInfo srcInfo(srcPath);
  if (!srcInfo.exists() || !srcInfo.isFile())
  {
    qWarning("[AuxiliaryService] importFile: source does not exist: %s", qPrintable(srcPath));
    return;
  }

  // Destination: folderPath if given, otherwise basePath_
  QString destDir = folderPath.isEmpty() ? basePath_ : folderPath;
  QDir dir(destDir);
  if (!dir.exists())
  {
    if (!dir.mkpath(QStringLiteral(".")))
    {
      qWarning("[AuxiliaryService] importFile: cannot create dir: %s", qPrintable(destDir));
      return;
    }
  }

  const QString destPath = destDir + QStringLiteral("/") + srcInfo.fileName();
  if (QFile::exists(destPath))
  {
    // Avoid overwrite — append numeric suffix
    int counter = 1;
    QString candidate;
    do
    {
      candidate = destDir + QStringLiteral("/") +
                  srcInfo.completeBaseName() +
                  QStringLiteral(" (%1)").arg(counter++) +
                  (srcInfo.suffix().isEmpty() ? QString() : QStringLiteral(".") + srcInfo.suffix());
    } while (QFile::exists(candidate));

    if (!QFile::copy(srcPath, candidate))
    {
      qWarning("[AuxiliaryService] importFile: copy failed to: %s", qPrintable(candidate));
      return;
    }
  }
  else
  {
    if (!QFile::copy(srcPath, destPath))
    {
      qWarning("[AuxiliaryService] importFile: copy failed to: %s", qPrintable(destPath));
      return;
    }
  }

  loadFiles();
}

void AuxiliaryService::createFolder(const QString &name, const QString &parentPath)
{
  QString dirPath = parentPath.isEmpty() ? basePath_ : parentPath;
  if (!name.isEmpty())
    dirPath += QStringLiteral("/") + name;

  QDir dir(dirPath);
  if (!dir.exists())
  {
    if (!dir.mkpath(QStringLiteral(".")))
    {
      qWarning("[AuxiliaryService] createFolder: cannot create: %s", qPrintable(dirPath));
      return;
    }
  }
  loadFiles();
}

void AuxiliaryService::deleteItem(const QString &path)
{
  if (path.isEmpty())
    return;

  QFileInfo info(path);
  if (!info.exists())
    return;

  // Safety: only delete items under basePath_
  if (!path.startsWith(basePath_))
  {
    qWarning("[AuxiliaryService] deleteItem: path outside basePath: %s", qPrintable(path));
    return;
  }

  if (info.isDir())
  {
    QDir dir(path);
    dir.removeRecursively();
  }
  else
  {
    QFile::remove(path);
  }
  loadFiles();
}

void AuxiliaryService::renameItem(const QString &path, const QString &newName)
{
  if (path.isEmpty() || newName.isEmpty())
    return;

  QFileInfo info(path);
  if (!info.exists())
    return;

  if (!path.startsWith(basePath_))
  {
    qWarning("[AuxiliaryService] renameItem: path outside basePath: %s", qPrintable(path));
    return;
  }

  const QString newPath = info.absolutePath() + QStringLiteral("/") + newName;
  if (QFile::rename(path, newPath))
  {
    loadFiles();
  }
  else
  {
    qWarning("[AuxiliaryService] renameItem: rename failed: %s -> %s",
             qPrintable(path), qPrintable(newPath));
  }
}

void AuxiliaryService::openFile(const QString &path)
{
  if (path.isEmpty())
    return;

  QFileInfo info(path);
  if (!info.exists() || info.isDir())
    return;

  // 对齐上游 wxLaunchDefaultApplication
  QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void AuxiliaryService::refresh()
{
  loadFiles();
}

void AuxiliaryService::ensureBasePathExists()
{
  QDir dir(basePath_);
  if (!dir.exists())
    dir.mkpath(QStringLiteral("."));
}

void AuxiliaryService::loadFiles()
{
  files_.clear();

  QDir dir(basePath_);
  if (!dir.exists())
  {
    emit filesChanged();
    return;
  }

  // Folders first, then files — matching upstream tree behavior
  const auto entries = dir.entryInfoList(
      QDir::AllEntries | QDir::NoDotAndDotDot,
      QDir::DirsFirst | QDir::Name);

  for (const QFileInfo &info : entries)
  {
    QVariantMap item;
    item[QStringLiteral("name")] = info.fileName();
    item[QStringLiteral("path")] = info.absoluteFilePath();
    item[QStringLiteral("isFolder")] = info.isDir();
    // Simple icon classification (对齐 upstream AuxiliaryModelNode icon)
    if (info.isDir())
    {
      item[QStringLiteral("icon")] = QStringLiteral("folder");
    }
    else
    {
      const QString suffix = info.suffix().toLower();
      if (suffix == QStringLiteral("png") || suffix == QStringLiteral("jpg") ||
          suffix == QStringLiteral("jpeg") || suffix == QStringLiteral("bmp") ||
          suffix == QStringLiteral("gif") || suffix == QStringLiteral("svg"))
        item[QStringLiteral("icon")] = QStringLiteral("image");
      else if (suffix == QStringLiteral("txt") || suffix == QStringLiteral("md") ||
               suffix == QStringLiteral("doc") || suffix == QStringLiteral("pdf"))
        item[QStringLiteral("icon")] = QStringLiteral("document");
      else
        item[QStringLiteral("icon")] = QStringLiteral("file");
    }
    files_.append(item);
  }

  emit filesChanged();
}
