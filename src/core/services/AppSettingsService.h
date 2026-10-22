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
  /// Creality Cloud region: "CN" (国内) or "INTL" (国际). Selects the
  /// crealitycloud.cn vs crealitycloud.com endpoint family. Default derives
  /// from the system locale (zh_* → CN) at first launch, then persists.
  Q_PROPERTY(QString cloudRegion READ cloudRegion WRITE setCloudRegion NOTIFY cloudSettingsChanged)
  /// Local directory for downloaded cloud models. Default:
  /// <AppDataLocation>/downloads (QStandardPaths::AppDataLocation).
  Q_PROPERTY(QString modelDownloadPath READ modelDownloadPath WRITE setModelDownloadPath NOTIFY cloudSettingsChanged)

public:
  explicit AppSettingsService(QObject *parent = nullptr);
  ~AppSettingsService() override;

  /// Bed configuration.
  double bedWidth() const { return bedWidth_; }
  double bedDepth() const { return bedDepth_; }
  QSizeF bedSize() const { return QSizeF(bedWidth_, bedDepth_); }

  /// Cloud integration settings.
  QString cloudRegion() const { return cloudRegion_; }
  QString modelDownloadPath() const { return modelDownloadPath_; }
  void setCloudRegion(const QString &region);
  void setModelDownloadPath(const QString &path);

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
  /// Batch notification for the cloud group (region, download path).
  void cloudSettingsChanged();

private:
  void loadSettings();
  void saveSettings();
  /// Locale-derived first-launch defaults (not persisted until first save).
  static QString defaultCloudRegion();
  static QString defaultModelDownloadPath();

  QSettings *settings_;
  double bedWidth_ = 220.0;   // Default 220x220.
  double bedDepth_ = 220.0;
  QString cloudRegion_;
  QString modelDownloadPath_;
};
