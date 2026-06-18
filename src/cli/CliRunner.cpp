#include "CliRunner.h"
#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceService.h"
#include "core/services/PresetServiceMock.h"
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>
#include <QFileInfo>
#include <QEventLoop>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Model.hpp>
#include <libslic3r/Print.hpp>
#include <libslic3r/PrintConfig.hpp>
#include <libslic3r/GCode/GCodeProcessor.hpp>
#endif

// Upstream error codes (libslic3r/Utils.hpp)
#define CLI_SUCCESS                 0
#define CLI_INVALID_PARAMS          -2
#define CLI_FILE_NOTFOUND           -3
#define CLI_CONFIG_FILE_ERROR       -5
#define CLI_DATA_FILE_ERROR         -6
#define CLI_SLICING_ERROR           -100

namespace {
QString formatDuration(double seconds)
{
    const qint64 totalSeconds = std::max<qint64>(0, qRound64(seconds));
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 secs = totalSeconds % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'));
}
}

CliRunner::CliRunner(QObject *parent)
    : QObject(parent)
{
}

int CliRunner::run()
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("OWzx CLI — headless slicing pipeline"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption loadOption(
        QStringLiteral("load"),
        QStringLiteral("Input file (STL/OBJ/3MF). Can be specified multiple times."),
        QStringLiteral("file"));
    QCommandLineOption loadSettingsOption(
        QStringLiteral("load-settings"),
        QStringLiteral("Preset config JSON to overlay."),
        QStringLiteral("json"));
    QCommandLineOption sliceOption(
        QStringLiteral("slice"),
        QStringLiteral("Slice all loaded models."));
    QCommandLineOption plateOption(
        QStringLiteral("plate"),
        QStringLiteral("Plate index to slice (0=all plates)."),
        QStringLiteral("n"),
        QStringLiteral("0"));
    QCommandLineOption outputDirOption(
        QStringLiteral("output-dir"),
        QStringLiteral("Output directory for G-code files."),
        QStringLiteral("dir"));
    QCommandLineOption quietOption(
        QStringLiteral("quiet"),
        QStringLiteral("Suppress progress output."));

    parser.addOption(loadOption);
    parser.addOption(loadSettingsOption);
    parser.addOption(sliceOption);
    parser.addOption(plateOption);
    parser.addOption(outputDirOption);
    parser.addOption(quietOption);

    if (!parser.parse(QCoreApplication::arguments())) {
        QTextStream err(stderr);
        err << "Error: " << parser.errorText() << "\n\n";
        parser.showHelp(CLI_INVALID_PARAMS);
    }

    // --help and --version exit immediately
    if (parser.isSet(QStringLiteral("help"))) {
        parser.showHelp();
        return CLI_SUCCESS;
    }
    if (parser.isSet(QStringLiteral("version"))) {
        parser.showVersion();
        return CLI_SUCCESS;
    }

    // Collect parsed values
    loadFiles_ = parser.values(loadOption);
    loadSettingsPath_ = parser.value(loadSettingsOption);
    doSlice_ = parser.isSet(sliceOption);
    plateIndex_ = parser.value(plateOption).toInt();
    outputDir_ = parser.value(outputDirOption);
    quiet_ = parser.isSet(quietOption);

    // No actions specified — show help
    if (loadFiles_.isEmpty() && !doSlice_) {
        parser.showHelp(CLI_SUCCESS);
    }

    // Slice without any loaded files — error
    if (doSlice_ && loadFiles_.isEmpty()) {
        QTextStream err(stderr);
        err << "Error: --slice requires at least one --load file\n";
        return CLI_INVALID_PARAMS;
    }

    // Instantiate services
    projectService_ = new ProjectServiceMock(this);
    presetService_ = new PresetServiceMock(this);
    sliceService_ = new SliceService(projectService_, this);

    // Connect loadFinished to track async load results
    connect(projectService_, &ProjectServiceMock::loadFinished,
            this, [this](bool success, const QString &message) {
        if (!success) {
            QTextStream err(stderr);
            err << "Error: " << message << "\n";
        }
    });

    // Load models (async)
    int loadResult = loadAndPrintInfo();
    if (loadResult != CLI_SUCCESS)
        return loadResult;

    // Slice pipeline
    if (doSlice_)
        return runSlice();

    return CLI_SUCCESS;
}

void CliRunner::printUsage()
{
    QTextStream out(stdout);
    out << "Usage: owzx-cli [options]\n\n"
        << "Options:\n"
        << "  --load <file>           Input file (STL/OBJ/3MF)\n"
        << "  --load-settings <json>  Preset config JSON\n"
        << "  --slice                 Slice loaded models\n"
        << "  --plate <n>             Plate index (0=all)\n"
        << "  --output-dir <dir>      Output directory\n"
        << "  --quiet                 Suppress progress\n"
        << "  --help                  Show this help\n"
        << "  --version               Show version\n";
}

int CliRunner::loadAndPrintInfo()
{
    QTextStream out(stdout);

    for (const QString &filePath : loadFiles_) {
        QFileInfo fi(filePath);
        if (!fi.exists()) {
            QTextStream err(stderr);
            err << "Error: file not found: " << filePath << "\n";
            return CLI_FILE_NOTFOUND;
        }

        if (!quiet_) {
            out << "Loading: " << filePath << " ...\n";
            out.flush();
        }

        // loadFile() is async under HAS_LIBSLIC3R — wait for loadFinished
        bool loadOk = false;
        QEventLoop loop;
        connect(projectService_, &ProjectServiceMock::loadFinished,
                &loop, [&](bool ok, const QString &) {
            loadOk = ok;
            loop.quit();
        });

        projectService_->loadFile(fi.absoluteFilePath());
        loop.exec();

        if (!loadOk) {
            QTextStream err(stderr);
            err << "Error: failed to load: " << filePath << "\n";
            return CLI_DATA_FILE_ERROR;
        }
    }

    if (!quiet_) {
        out << "Objects: " << projectService_->modelCount() << "\n";
        out << "Plates:  " << projectService_->plateCount() << "\n";
        const QStringList names = projectService_->objectNames();
        for (int i = 0; i < names.size(); ++i) {
            out << "  [" << i << "] " << names.at(i) << "\n";
        }
    }

    return CLI_SUCCESS;
}

int CliRunner::runSlice()
{
    QTextStream out(stdout);

    // Build merged preset config from default presets (printer → filament → print)
    QHash<QString, QVariant> mergedConfig;

    // Tier 1: Printer preset
    const QString printerPreset = presetService_->defaultPresetForCategory(
        static_cast<int>(PresetServiceMock::PrinterCat));
    if (!printerPreset.isEmpty()) {
        const auto vals = presetService_->presetValues(printerPreset);
        for (auto it = vals.constBegin(); it != vals.constEnd(); ++it)
            mergedConfig[it.key()] = it.value();
    }

    // Tier 2: Filament preset
    const QString filamentPreset = presetService_->defaultPresetForCategory(
        static_cast<int>(PresetServiceMock::FilamentCat));
    if (!filamentPreset.isEmpty()) {
        const auto vals = presetService_->presetValues(filamentPreset);
        for (auto it = vals.constBegin(); it != vals.constEnd(); ++it)
            mergedConfig[it.key()] = it.value();
    }

    // Tier 3: Print preset
    const QString printPreset = presetService_->defaultPresetForCategory(
        static_cast<int>(PresetServiceMock::PrintCat));
    if (!printPreset.isEmpty()) {
        const auto vals = presetService_->presetValues(printPreset);
        for (auto it = vals.constBegin(); it != vals.constEnd(); ++it)
            mergedConfig[it.key()] = it.value();
    }

    // Overlay user-provided settings JSON on top
    if (!loadSettingsPath_.isEmpty()) {
        QHash<QString, QVariant> settings = loadSettingsJson(loadSettingsPath_);
        if (settings.isEmpty()) {
            QTextStream err(stderr);
            err << "Error: failed to load settings: " << loadSettingsPath_ << "\n";
            return CLI_CONFIG_FILE_ERROR;
        }
        for (auto it = settings.constBegin(); it != settings.constEnd(); ++it)
            mergedConfig[it.key()] = it.value();
        if (!quiet_)
            out << "Settings overlay: " << loadSettingsPath_ << "\n";
    }

    if (!mergedConfig.isEmpty()) {
        sliceService_->setMergedPresetConfig(mergedConfig);
        if (!quiet_) {
            out << "Preset config: " << mergedConfig.size() << " keys\n";
            out.flush();
        }
    } else if (!quiet_) {
        out << "Warning: no preset config loaded, using defaults\n";
        out.flush();
    }

    // Resolve output directory
    QString outDir = outputDir_.isEmpty()
                         ? QCoreApplication::applicationDirPath()
                         : outputDir_;
    QDir().mkpath(outDir);

    // Set plate index if specified
    if (plateIndex_ > 0 && projectService_->plateCount() > 0) {
        projectService_->setCurrentPlateIndex(plateIndex_ - 1);
    }

    if (!quiet_) {
        out << "Slicing...\n";
        out << "  Plate index: " << projectService_->currentPlateIndex() << "\n";
        out << "  Model count: " << projectService_->modelCount() << "\n";
        out << "  Plate count: " << projectService_->plateCount() << "\n";
        out.flush();
    }

    // Connect progress
    if (!quiet_) {
        connect(sliceService_, &SliceService::progressUpdated,
                this, [this](int percent, const QString &label) {
            QTextStream out(stdout);
            out << "  [" << percent << "%] " << label << "\n";
            out.flush();
        });
    }

    // Wait for sliceFinished
    int sliceExitCode = CLI_SLICING_ERROR;
    QString sliceError;
    QEventLoop loop;
    connect(sliceService_, &SliceService::sliceFinished,
            &loop, [&](const QString &estimatedTime) {
        sliceExitCode = CLI_SUCCESS;
        if (!quiet_) {
            QTextStream out(stdout);
            out << "Slice complete: " << estimatedTime << "\n";
            out << "  Weight: " << sliceService_->resultWeightLabel() << "\n";
            out << "  Filament: " << sliceService_->resultFilamentLabel() << "\n";
            out << "  Layers: " << sliceService_->resultLayerCount() << "\n";
        }
        loop.quit();
    });
    connect(sliceService_, &SliceService::sliceFailed,
            &loop, [&](const QString &message) {
        sliceError = message;
        loop.quit();
    });

    sliceService_->startSlice(projectService_->projectName());
    loop.exec();

    if (sliceExitCode != CLI_SUCCESS) {
        QTextStream err(stderr);
        err << "Error: slice failed: " << sliceError << "\n";
        return sliceExitCode;
    }

    // Export G-code to output directory
    const QString gcodeSrc = sliceService_->outputPath();
    if (gcodeSrc.isEmpty()) {
        QTextStream err(stderr);
        err << "Error: no G-code output from slice\n";
        return CLI_SLICING_ERROR;
    }

    const QString baseName = QFileInfo(projectService_->sourceFilePath()).completeBaseName();
    const QString gcodeDest = outDir + QStringLiteral("/") + baseName + QStringLiteral(".gcode");
    if (!sliceService_->exportGCodeToPath(gcodeDest)) {
        QTextStream err(stderr);
        err << "Error: failed to export G-code to: " << gcodeDest << "\n";
        return CLI_SLICING_ERROR;
    }

    if (!quiet_)
        out << "G-code written: " << gcodeDest << "\n";

    return CLI_SUCCESS;
}

QHash<QString, QVariant> CliRunner::loadSettingsJson(const QString &path)
{
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isFile())
        return {};

    QFile file(fi.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return {};

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return {};

    if (!doc.isObject())
        return {};

    QHash<QString, QVariant> result;
    const QJsonObject obj = doc.object();
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it)
        result.insert(it.key(), it.value().toVariant());

    return result;
}

#ifdef HAS_LIBSLIC3R
int CliRunner::directSlice()
{
    QTextStream out(stdout);
    out << "Slicing...\n";
    out.flush();

    Slic3r::Model *srcModel = projectService_->rawModel();
    if (!srcModel || srcModel->objects.empty()) {
        QTextStream err(stderr);
        err << "Error: no model loaded\n";
        return CLI_DATA_FILE_ERROR;
    }

    Slic3r::DynamicPrintConfig config = Slic3r::DynamicPrintConfig::full_print_config();
    config.set_key_value("bed_shape", new Slic3r::ConfigOptionPoints{
        {0, 0}, {220, 0}, {220, 220}, {0, 220}
    });
    config.set_key_value("printable_height", new Slic3r::ConfigOptionFloat{250.0});
    config.set_key_value("nozzle_diameter", new Slic3r::ConfigOptionFloats{0.4});

    Slic3r::Print print;

    try {
        print.apply(*srcModel, config);
    } catch (const std::exception &ex) {
        QTextStream err(stderr);
        err << "Error: apply failed: " << ex.what() << "\n";
        return CLI_SLICING_ERROR;
    }

    try {
        print.process();
    } catch (const std::exception &ex) {
        QTextStream err(stderr);
        err << "Error: slice failed: " << ex.what() << "\n";
        return CLI_SLICING_ERROR;
    }

    QString outDir = outputDir_.isEmpty()
                         ? QCoreApplication::applicationDirPath()
                         : outputDir_;
    QDir().mkpath(outDir);
    const QString baseName = QFileInfo(projectService_->sourceFilePath()).completeBaseName();
    const QString gcodePath = outDir + QStringLiteral("/") + baseName + QStringLiteral(".gcode");

    try {
        Slic3r::GCodeProcessorResult result;
        print.export_gcode(gcodePath.toStdString(), &result);
        out << "Slice complete: " << formatDuration(result.print_statistics.modes[0].time) << "\n";
        out << "G-code: " << gcodePath << "\n";
        out.flush();
    } catch (const std::exception &ex) {
        QTextStream err(stderr);
        err << "Error: export failed: " << ex.what() << "\n";
        return CLI_SLICING_ERROR;
    }

    return CLI_SUCCESS;
}
#endif
