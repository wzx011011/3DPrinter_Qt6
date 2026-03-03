#pragma once
#include <QObject>
#include <QVariantList>
#include <QString>

class CalibrationServiceMock;

class CalibrationViewModel : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QVariantList calibrationItems READ calibrationItems CONSTANT)
  Q_PROPERTY(int selectedIndex READ selectedIndex NOTIFY selectionChanged)
  Q_PROPERTY(QString selectedTitle READ selectedTitle NOTIFY selectionChanged)
  Q_PROPERTY(QString selectedDescription READ selectedDescription NOTIFY selectionChanged)
  Q_PROPERTY(QString selectedPreviewLabel READ selectedPreviewLabel NOTIFY selectionChanged)
  Q_PROPERTY(bool isRunning READ isRunning NOTIFY runningChanged)
  Q_PROPERTY(int progress READ progress NOTIFY progressChanged)

public:
  explicit CalibrationViewModel(CalibrationServiceMock *service, QObject *parent = nullptr);

  QVariantList calibrationItems() const;

  // Individual item accessors - use these from QML to avoid Qt6 V4 VariantAssociationObject crash
  Q_INVOKABLE int calibItemCount() const;
  Q_INVOKABLE QString calibItemIcon(int i) const;
  Q_INVOKABLE QString calibItemName(int i) const;
  Q_INVOKABLE QString calibItemDesc(int i) const;

  int selectedIndex() const { return m_selectedIndex; }
  QString selectedTitle() const;
  QString selectedDescription() const;
  QString selectedPreviewLabel() const;
  bool isRunning() const;
  int progress() const;

signals:
  void selectionChanged();
  void runningChanged();
  void progressChanged();

public slots:
  void selectItem(int index);
  void startCalibration();
  void cancelCalibration();
  void resetParameters();

private:
  struct CalibItem
  {
    QString icon, name, desc, longDesc, previewLabel;
  };
  QList<CalibItem> m_calibItems;
  int m_selectedIndex = -1;
  CalibrationServiceMock *m_service = nullptr;
};
