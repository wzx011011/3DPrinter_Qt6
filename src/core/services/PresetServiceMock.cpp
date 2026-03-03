#include "PresetServiceMock.h"

PresetServiceMock::PresetServiceMock(QObject *parent)
    : QObject(parent)
{
}

QStringList PresetServiceMock::presetNames() const
{
  return {
      QStringLiteral("0.20mm Standard @Creality K1C"),
      QStringLiteral("0.16mm Fine"),
      QStringLiteral("0.12mm Ultra")};
}

QString PresetServiceMock::defaultPreset() const
{
  return QStringLiteral("0.20mm Standard @Creality K1C");
}

double PresetServiceMock::defaultLayerHeight() const
{
  return 0.2;
}
