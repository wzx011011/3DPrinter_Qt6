#pragma once

#include <QPointF>
#include <QMetaType>
#include <QtGlobal>

// Shared C++ contract for Prepare viewport context targeting.
enum class ViewportContextTarget : int
{
  Object = 0,
  Part = 1,
  Plate = 2,
  Empty = 3,
  Suppressed = 4
};

struct ViewportContextHit
{
  ViewportContextTarget target = ViewportContextTarget::Empty;
  int sourceObjectIndex = -1;
  int volumeIndex = -1;
  int instanceIndex = -1;
  int plateIndex = -1;
  QPointF popupPosition;

  bool isMenuRequest() const
  {
    return target != ViewportContextTarget::Suppressed;
  }
};

Q_DECLARE_METATYPE(ViewportContextHit)
