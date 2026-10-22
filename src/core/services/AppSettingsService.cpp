#include "AppSettingsService.h"
#include <QDebug>
#include <QLocale>
#include <QStandardPaths>

static const char kGroupBed[] = "Bed";
static const char kKeyBedWidth[] = "Width";
static const char kKeyBedDepth[] = "Depth";
static constexpr double kDefaultBedWidth = 220.0;
static constexpr double kDefaultBedDepth = 220.0;

static const char kGroupCloud[] = "Cloud";
static const char kKeyCloudRegion[] = "Region";
static const char kKeyDownloadPath[] = "ModelDownloadPath";

AppSettingsService::AppSettingsService(QObject *parent)
    : QObject(parent)
{
    // QSettings uses QApplication organization/domain for the key location.
    // These are set in main_qml.cpp; QSettings falls back to QCoreApplication::organizationName().
    settings_ = new QSettings(this);
    loadSettings();
    qDebug("[AppSettings] loaded: bed=%.1fx%.1f", bedWidth_, bedDepth_);
}

AppSettingsService::~AppSettingsService() = default;

void AppSettingsService::loadSettings()
{
    settings_->beginGroup(kGroupBed);
    bedWidth_ = settings_->value(kKeyBedWidth, kDefaultBedWidth).toDouble();
    bedDepth_ = settings_->value(kKeyBedDepth, kDefaultBedDepth).toDouble();
    settings_->endGroup();

    settings_->beginGroup(kGroupCloud);
    cloudRegion_ = settings_->value(kKeyCloudRegion, defaultCloudRegion()).toString();
    modelDownloadPath_ = settings_->value(
        kKeyDownloadPath, defaultModelDownloadPath()).toString();
    settings_->endGroup();
    qDebug("[AppSettings] loaded: bed=%.1fx%.1f, cloudRegion=%s",
           bedWidth_, bedDepth_, qPrintable(cloudRegion_));
}

void AppSettingsService::saveSettings()
{
    settings_->beginGroup(kGroupBed);
    settings_->setValue(kKeyBedWidth, bedWidth_);
    settings_->setValue(kKeyBedDepth, bedDepth_);
    settings_->endGroup();

    settings_->beginGroup(kGroupCloud);
    settings_->setValue(kKeyCloudRegion, cloudRegion_);
    settings_->setValue(kKeyDownloadPath, modelDownloadPath_);
    settings_->endGroup();
    settings_->sync();
    qDebug("[AppSettings] saved: bed=%.1fx%.1f", bedWidth_, bedDepth_);
}

void AppSettingsService::setBedWidth(double width)
{
    if (qFuzzyCompare(bedWidth_, width))
        return;
    bedWidth_ = qBound(50.0, width, 2000.0); // Clamp to a practical printer-bed range.
    saveSettings();
    emit bedConfigChanged();
}

void AppSettingsService::setBedDepth(double depth)
{
    if (qFuzzyCompare(bedDepth_, depth))
        return;
    bedDepth_ = qBound(50.0, depth, 2000.0);
    saveSettings();
    emit bedConfigChanged();
}

void AppSettingsService::setBedSize(const QSizeF &size)
{
    setBedWidth(size.width());
    setBedDepth(size.height());
}

void AppSettingsService::clearAll()
{
    settings_->clear();
    settings_->sync();
    loadSettings();
}

void AppSettingsService::resetToDefaults()
{
    bedWidth_ = kDefaultBedWidth;
    bedDepth_ = kDefaultBedDepth;
    cloudRegion_ = defaultCloudRegion();
    modelDownloadPath_ = defaultModelDownloadPath();
    saveSettings();
    emit bedConfigChanged();
    emit cloudSettingsChanged();
}

void AppSettingsService::setCloudRegion(const QString &region)
{
    const QString normalized = (region == QStringLiteral("INTL")) ? QStringLiteral("INTL")
                                                                 : QStringLiteral("CN");
    if (cloudRegion_ == normalized)
        return;
    cloudRegion_ = normalized;
    saveSettings();
    emit cloudSettingsChanged();
}

void AppSettingsService::setModelDownloadPath(const QString &path)
{
    if (modelDownloadPath_ == path || path.isEmpty())
        return;
    modelDownloadPath_ = path;
    saveSettings();
    emit cloudSettingsChanged();
}

QString AppSettingsService::defaultCloudRegion()
{
    // First-launch default: a zh locale starts on the CN endpoint family,
    // everything else on the international family (mirrors the upstream
    // country_code heuristic).
    return QLocale::system().name().startsWith(QStringLiteral("zh"))
               ? QStringLiteral("CN") : QStringLiteral("INTL");
}

QString AppSettingsService::defaultModelDownloadPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/downloads");
}
