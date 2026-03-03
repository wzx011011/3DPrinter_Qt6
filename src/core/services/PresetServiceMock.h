#pragma once

#include <QObject>
#include <QStringList>

class PresetServiceMock final : public QObject
{
  Q_OBJECT

public:
  explicit PresetServiceMock(QObject *parent = nullptr);

  Q_INVOKABLE QStringList presetNames() const;
  Q_INVOKABLE QString defaultPreset() const;
  Q_INVOKABLE double defaultLayerHeight() const;
};
