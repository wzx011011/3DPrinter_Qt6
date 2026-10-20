#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace owzx {

// Normalize the preset ecosystem's printable_area / bed_exclude_area values
// into flat coordinate pairs. Upstream ConfigOptionPoints serialize as "XxY"
// points — either a QVariantList of per-point strings ("0x0","300x0",...) or
// one flat string ("0x0,300x0,300x300,0x300"); plain "x1,y1,x2,y2" input
// passes through. Returns an empty list when any token is malformed.
//
// This is the shared form of the normalization that used to live inline in
// AppToolRegistry::normalizePrintableArea,
// EditorViewModel::syncBedFromPrinterPreset and
// CliRunner's arrangeBedPolygonFromConfig — each of which originally
// parsed the tokens with plain toFloat()/toDouble() and silently rejected
// every "0x0" point (K1 Max stayed at the 220x220 default bed while its
// STL/texture/gcode are all 300x300).
inline QList<float> flatPointCoords(const QVariant &value)
{
  QStringList tokens;
  if (value.typeId() == QMetaType::QVariantList) {
    const QVariantList points = value.toList();
    tokens.reserve(points.size());
    for (const QVariant &point : points)
      tokens.append(point.toString());
  } else if (value.typeId() == QMetaType::QStringList) {
    tokens = value.toStringList();
  } else {
    const QString raw = value.toString();
    if (!raw.isEmpty())
      tokens = raw.split(QLatin1Char(','), Qt::SkipEmptyParts);
  }

  QList<float> coords;
  coords.reserve(tokens.size() * 2);
  for (const QString &token : tokens) {
    const QString trimmed = token.trimmed();
    bool ok = false;
    const float direct = trimmed.toFloat(&ok);
    if (ok) {  // already a plain coordinate
      coords.append(direct);
      continue;
    }
    const QStringList xy = trimmed.split(QLatin1Char('x'));
    if (xy.size() != 2)
      return {};
    const float x = xy.first().toFloat(&ok);
    if (!ok) return {};
    const float y = xy.last().toFloat(&ok);
    if (!ok) return {};
    coords.append(x);
    coords.append(y);
  }
  return coords;
}

// Same normalization as a "x1,y1,x2,y2,..." string (the form
// ProjectService::arrangeObjects and other polygon consumers parse).
// Empty string when the value carries no usable polygon.
inline QString flatPointCoordsString(const QVariant &value)
{
  const QList<float> coords = flatPointCoords(value);
  QStringList parts;
  parts.reserve(coords.size());
  for (float coord : coords)
    parts.append(QString::number(double(coord)));
  return parts.join(QLatin1Char(','));
}

}  // namespace owzx
