// Phase 171/173 (P1/P3) + S8 (2026-10-14): thin QSettings facade for QML.
// QSettings itself does not expose value()/setValue() as Q_INVOKABLE, so a
// bare QSettings object is call-blind from QML; main.qml's Theme bridge and
// the appearance actions call through this store instead. Backed by the
// dedicated "OWzx/UI" store — the same one main() reads ui/scaleFactor from
// pre-app-creation, so registry seeding pre-launch is honored.
//
// S8: this class moved out of main_qml.cpp (it used to be a cpp-local class
// exposed through a root context property) into the OWzx QML module as a
// QML_SINGLETON named `appSettings`. The engine creates the instance on
// first access; there is deliberately no C++-side singleton handle — main()
// never touches it pre-load. Tests isolate the backing store with
// QSettings::setDefaultFormat(IniFormat) + QSettings::setPath(tempdir) before
// loading QML (see tests/QmlSmokeTests.cpp), so a smoke run can never persist
// a ui/theme or ui/scaleFactor change into the host's real OWzx/UI registry.
#pragma once

#include <QObject>
#include <QSettings>
#include <QtQml/QtQml>

class UiSettingsStore : public QObject
{
  Q_OBJECT
  QML_SINGLETON
  QML_NAMED_ELEMENT(AppSettings)

public:
  explicit UiSettingsStore(QObject *parent = nullptr)
      : QObject(parent)
  {
  }

  Q_INVOKABLE QVariant value(const QString &key, const QVariant &defaultValue = {}) const
  {
    return settings_.value(key, defaultValue);
  }

  Q_INVOKABLE void setValue(const QString &key, const QVariant &val)
  {
    settings_.setValue(key, val);
  }

private:
  QSettings settings_{QStringLiteral("OWzx"), QStringLiteral("UI")};
};
