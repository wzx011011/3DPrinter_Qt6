#pragma once

#include <QSGRendererInterface>
#include <QString>
#include <QStringList>
#include <QVector>

struct RhiBackendAttempt
{
  QString name;
  QSGRendererInterface::GraphicsApi graphicsApi = QSGRendererInterface::Unknown;
  bool success = false;
  QString failureReason;
};

struct RhiBackendSelection
{
  bool enabled = false;
  QString requested;
  QString selectedBackend;
  QSGRendererInterface::GraphicsApi selectedGraphicsApi = QSGRendererInterface::Unknown;
  QVector<RhiBackendAttempt> attempts;
  QStringList failureReasons;
  bool canUseRhi = false;

  QString diagnostics() const;
};

RhiBackendSelection selectRhiBackendFromEnvironment();
