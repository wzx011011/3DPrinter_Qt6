#pragma once

#include <QObject>

class NetworkServiceMock final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool online READ online NOTIFY networkChanged)
  Q_PROPERTY(int latencyMs READ latencyMs NOTIFY networkChanged)

public:
  explicit NetworkServiceMock(QObject *parent = nullptr);

  bool online() const;
  int latencyMs() const;

  Q_INVOKABLE void probe();

signals:
  void networkChanged();

private:
  bool online_ = true;
  int latencyMs_ = 18;
  int tick_ = 0;
};
