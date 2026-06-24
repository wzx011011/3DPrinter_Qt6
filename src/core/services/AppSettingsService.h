#pragma once

#include <QObject>
#include <QString>
#include <QSettings>
#include <QSizeF>

/// AppSettingsService - application-level persisted settings service.
///
/// Provides application-scoped settings persistence, including:
/// - Bed dimensions from printer configuration.
/// - User preferences.
/// - UI state.
///
/// Uses QSettings for platform-native storage:
/// - Windows: 注册表 HKEY_CURRENT_USER\Software\OWzx\OWzxSlicer
/// - macOS: ~/Library/Preferences/com.OWzx.OWzxSlicer.plist
/// - Linux: ~/.config/OWzx/OWzxSlicer.conf
class AppSettingsService final : public QObject
{
  Q_OBJECT
  /// Bed width in millimeters.
  Q_PROPERTY(double bedWidth READ bedWidth WRITE setBedWidth NOTIFY bedConfigChanged)
  /// Bed depth in millimeters.
  Q_PROPERTY(double bedDepth READ bedDepth WRITE setBedDepth NOTIFY bedConfigChanged)

public:
  explicit AppSettingsService(QObject *parent = nullptr);
  ~AppSettingsService() override;

  /// Bed configuration.
  double bedWidth() const { return bedWidth_; }
  double bedDepth() const { return bedDepth_; }
  QSizeF bedSize() const { return QSizeF(bedWidth_, bedDepth_); }

  /// Set bed dimensions.
  void setBedWidth(double width);
  void setBedDepth(double depth);
  void setBedSize(const QSizeF &size);

  /// Remove all persisted settings.
  Q_INVOKABLE void clearAll();

  /// Reset persisted values to defaults.
  Q_INVOKABLE void resetToDefaults();

signals:
  void bedConfigChanged();

private:
  void loadSettings();
  void saveSettings();

  QSettings *settings_;
  double bedWidth_ = 220.0;   // Default 220x220.
  double bedDepth_ = 220.0;
};
