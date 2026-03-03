#include "NetworkServiceMock.h"

NetworkServiceMock::NetworkServiceMock(QObject *parent)
    : QObject(parent)
{
}

bool NetworkServiceMock::online() const
{
  return online_;
}

int NetworkServiceMock::latencyMs() const
{
  return latencyMs_;
}

void NetworkServiceMock::probe()
{
  tick_++;
  latencyMs_ = 12 + (tick_ % 40);
  online_ = true;
  emit networkChanged();
}
