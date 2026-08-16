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
    // Phase 241 (CLI-01): pre-slice transforms (upstream transform loop,
    // OrcaSlicer.cpp:3397-3690; semantic deltas documented in printUsage).
    QCommandLineOption arrangeOption(
        QStringLiteral("arrange"),
        QStringLiteral("Force-arrange all objects on the bed before slicing."));
    QCommandLineOption orientOption(
        QStringLiteral("orient"),
        QStringLiteral("Auto-orient all objects before slicing."));
    QCommandLineOption cutOption(
        QStringLiteral("cut"),
        QStringLiteral("Cut all objects at AXIS:POS (e.g. --cut z:10.5)."),
        QStringLiteral("axis:pos"));
    QCommandLineOption splitOption(
        QStringLiteral("split"),
        QStringLiteral("Split multi-part objects into separate objects."));
    QCommandLineOption assembleOption(
        QStringLiteral("assemble"),
        QStringLiteral("Merge all objects into one multi-part object."));
    QCommandLineOption repairOption(
        QStringLiteral("repair"),
        QStringLiteral("Repair mesh errors on every object."));
    QCommandLineOption scaleToFitOption(
        QStringLiteral("scale-to-fit"),
        QStringLiteral("Uniformly scale objects to fit X,Y,Z mm (e.g. 100,100,100)."),
        QStringLiteral("x,y,z"));
    // Phase 241 (CLI-02): exports (upstream export_stl bool / export_3mf
    // file / export_slicedata dir, PrintConfig.cpp:7006-7029).
    QCommandLineOption exportStlOption(
        QStringLiteral("export-stl"),
        QStringLiteral("Export all objects as one merged STL (upstream export_stl)."));
    QCommandLineOption export3mfOption(
        QStringLiteral("export-3mf"),
        QStringLiteral("Export the project as 3MF."),
        QStringLiteral("file"),
        QStringLiteral("output.3mf"));
    QCommandLineOption exportSlicedataOption(
        QStringLiteral("export-slicedata"),
        QStringLiteral("Directory for sliced data (.gcode.3mf per plate). Requires --slice."),
        QStringLiteral("dir"),
        QStringLiteral("cached_data"));

    parser.addOption(loadOption);
    parser.addOption(loadSettingsOption);
    parser.addOption(sliceOption);
    parser.addOption(plateOption);
    parser.addOption(outputDirOption);
    parser.addOption(quietOption);
    parser.addOption(arrangeOption);
    parser.addOption(orientOption);
    parser.addOption(cutOption);
    parser.addOption(splitOption);
    parser.addOption(assembleOption);
    parser.addOption(repairOption);
    parser.addOption(scaleToFitOption);
    parser.addOption(exportStlOption);
    parser.addOption(export3mfOption);
    parser.addOption(exportSlicedataOption);

    // Phase 241 (CLI-02): arbitrary `--key value` PrintConfig overrides.
    // QCommandLineParser rejects unknown long options, so unrecognized
    // `--key value` tokens are extracted from the raw argv FIRST (upstream
    // boost::program_options allow_unregistered semantics) and validated
    // against the DynamicPrintConfig schema.
    QStringList knownOptions = {
        QStringLiteral("load"), QStringLiteral("load-settings"),
        QStringLiteral("slice"), QStringLiteral("plate"),
        QStringLiteral("output-dir"), QStringLiteral("quiet"),
        QStringLiteral("arrange"), QStringLiteral("orient"),
        QStringLiteral("cut"), QStringLiteral("split"),
        QStringLiteral("assemble"), QStringLiteral("repair"),
        QStringLiteral("scale-to-fit"), QStringLiteral("export-stl"),
        QStringLiteral("export-3mf"), QStringLiteral("export-slicedata"),
        QStringLiteral("help"), QStringLiteral("version"),
        QStringLiteral("h"), QStringLiteral("v"),
    };
    if (!extractConfigOverrides(QCoreApplication::arguments(), knownOptions))
        return CLI_INVALID_PARAMS;

    QStringList filteredArgs = QCoreApplication::arguments();
    // Strip the extracted override tokens so the parser only sees known
    // options (the value token after each override key was consumed too).
    {
        QStringList stripped;
        for (int i = 0; i < filteredArgs.size(); ++i) {
            const QString &arg = filteredArgs.at(i);
            const QString name = arg.startsWith(QStringLiteral("--"))
                                     ? arg.mid(2)
                                     : arg;
            if (configOverrides_.contains(name)) {
                ++i; // skip the value token
                continue;
            }
            stripped.append(arg);
        }
        filteredArgs = stripped;
    }

    if (!parser.parse(filteredArgs)) {
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
    arrange_ = parser.isSet(arrangeOption);
    orient_ = parser.isSet(orientOption);
    split_ = parser.isSet(splitOption);
    assemble_ = parser.isSet(assembleOption);
    repair_ = parser.isSet(repairOption);
    exportStl_ = parser.isSet(exportStlOption);
    // NOTE: --export-3mf / --export-slicedata carry upstream default values
    // ("output.3mf" / "cached_data"), so parser.value() is non-empty even
    // when the flag is absent — gate on isSet() (upstream PrintConfig.cpp:
    // 7006-7029 declares the defaults; the ACTION only runs when set).
    if (parser.isSet(export3mfOption))
        export3mfPath_ = parser.value(export3mfOption);
    if (parser.isSet(exportSlicedataOption))
        exportSlicedataDir_ = parser.value(exportSlicedataOption);
    if (parser.isSet(scaleToFitOption)) {
        const QStringList parts = parser.value(scaleToFitOption).split(
            QLatin1Char(','), Qt::SkipEmptyParts);
        if (parts.size() != 3) {
            QTextStream err(stderr);
            err << "Error: --scale-to-fit expects X,Y,Z (e.g. 100,100,100)\n";
            return CLI_INVALID_PARAMS;
        }
        scaleToFitX_ = parts.at(0).toDouble();
        scaleToFitY_ = parts.at(1).toDouble();
        scaleToFitZ_ = parts.at(2).toDouble();
        if (scaleToFitX_ <= 0 || scaleToFitY_ <= 0 || scaleToFitZ_ <= 0) {
            // Upstream: "--scale-to-fit requires a positive volume"
            // (OrcaSlicer.cpp:3591).
            QTextStream err(stderr);
            err << "Error: --scale-to-fit requires a positive volume\n";
            return CLI_INVALID_PARAMS;
        }
        scaleToFit_ = true;
    }
    if (parser.isSet(cutOption)) {
        // --cut <axis>:<position>; axis x/y/z (0-based service axis 0/1/2).
        const QString value = parser.value(cutOption);
        const int sep = value.indexOf(QLatin1Char(':'));
        if (sep <= 0 || sep + 1 >= value.size()) {
            QTextStream err(stderr);
            err << "Error: --cut expects <axis>:<position> (e.g. z:10.5)\n";
            return CLI_INVALID_PARAMS;
        }
        const QString axis = value.left(sep).toLower();
        const double pos = value.mid(sep + 1).toDouble();
        if (axis == QLatin1String("x"))
            cutAxis_ = 0;
        else if (axis == QLatin1String("y"))
            cutAxis_ = 1;
        else if (axis == QLatin1String("z"))
            cutAxis_ = 2;
        else {
            QTextStream err(stderr);
            err << "Error: --cut axis must be x, y or z\n";
            return CLI_INVALID_PARAMS;
        }
        cutPosition_ = pos;
        cut_ = true;
    }
    if (!exportSlicedataDir_.isEmpty() && !doSlice_) {
        // Sliced data only exists after slicing (upstream pairs
        // export_slicedata with slice, OrcaSlicer.cpp:4643-4650 guards the
        // load/export pairing).
        QTextStream err(stderr);
        err << "Error: --export-slicedata requires --slice\n";
        return CLI_INVALID_PARAMS;
    }

    // No actions specified — show help
    if (loadFiles_.isEmpty() && !doSlice_ && !exportStl_
        && export3mfPath_.isEmpty() && exportSlicedataDir_.isEmpty()) {
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

    // Phase 241 (CLI-01): pre-slice transforms run after loading, before
    // slicing/exporting (upstream order: load -> transforms -> slice/export).
    int transformResult = applyTransforms();
    if (transformResult != CLI_SUCCESS)
        return transformResult;

    // Slice pipeline
    QHash<int, QString> slicedGcodes;
    if (doSlice_) {
        const int sliceResult = runSlice(&slicedGcodes);
        if (sliceResult != CLI_SUCCESS)
            return sliceResult;
    }

    // Phase 241 (CLI-02): exports run post-load (and post-slice for
    // slicedata, which consumes the per-plate gcode paths).
    return runExports(slicedGcodes);
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
        << "  --arrange               Force-arrange all objects\n"
        << "  --orient                Auto-orient all objects\n"
        << "  --cut <axis:pos>        Cut objects at axis x/y/z position (mm)\n"
        << "  --split                 Split multi-part objects\n"
        << "  --assemble              Merge objects into one\n"
        << "  --repair                Repair mesh errors\n"
        << "  --scale-to-fit <x,y,z>  Uniformly scale to fit X,Y,Z mm\n"
        << "  --export-stl            Export merged STL (input basename)\n"
        << "  --export-3mf <file>     Export project 3MF\n"
        << "  --export-slicedata <dir> Per-plate .gcode.3mf (needs --slice)\n"
        << "  --<key> <value>         PrintConfig override (e.g. --layer_height 0.3)\n"
        << "  --quiet                 Suppress progress\n"
        << "  --help                  Show this help\n"
        << "  --version               Show version\n";
}

bool CliRunner::extractConfigOverrides(const QStringList &rawArgs,
                                       const QStringList &knownOptions)
{
#ifdef HAS_LIBSLIC3R
    for (int i = 1; i < rawArgs.size(); ++i) {
        const QString &arg = rawArgs.at(i);
        if (!arg.startsWith(QStringLiteral("--")))
            continue;
        const QString name = arg.mid(2);
        if (name.isEmpty() || knownOptions.contains(name))
            continue;
        if (i + 1 >= rawArgs.size()
            || (rawArgs.at(i + 1).startsWith(QLatin1Char('-'))
                && rawArgs.at(i + 1).length() > 1
                && !rawArgs.at(i + 1).at(1).isDigit())) {
            QTextStream err(stderr);
            err << "Error: option --" << name << " requires a value\n";
            return false;
        }
        const QString value = rawArgs.at(i + 1);
        // Validate against the PrintConfig schema (upstream semantics:
        // unknown option keys are a hard error). NOTE: set_deserialize_strict
        // alone is NOT sufficient — handle_legacy() silently clears unknown
        // keys and set_deserialize_nothrow then "ignores the option"
        // (Config.cpp:538-553). Check the def (including aliases) first,
        // then probe the value through set_deserialize_strict.
        const std::string keyStd = name.toStdString();
        const Slic3r::ConfigOptionDef *optdef = Slic3r::print_config_def.get(keyStd);
        if (!optdef) {
            // Alias resolution mirrors set_deserialize_raw (Config.cpp:585-596).
            for (const auto &opt : Slic3r::print_config_def.options) {
                bool found = false;
                for (const std::string &alias : opt.second.aliases)
                    if (alias == keyStd) { found = true; break; }
                if (found) { optdef = &opt.second; break; }
            }
        }
        if (!optdef) {
            QTextStream err(stderr);
            err << "Error: unknown option: --" << name << "\n";
            return false;
        }
        try {
            Slic3r::DynamicPrintConfig probe;
            probe.set_deserialize_strict(keyStd, value.toStdString());
        } catch (const std::exception &ex) {
            QTextStream err(stderr);
            err << "Error: invalid value for --" << name << ": "
                << ex.what() << "\n";
            return false;
        }
        configOverrides_.insert(name, value);
        ++i; // consume the value token
    }
    return true;
#else
    Q_UNUSED(rawArgs)
    Q_UNUSED(knownOptions)
    return true;
#endif
}

int CliRunner::applyTransforms()
{
    QTextStream out(stdout);
    const int modelCount = projectService_->modelCount();
    if (modelCount <= 0)
        return CLI_SUCCESS; // nothing loaded, nothing to transform

    // Phase 241 (CLI-01): assemble merges every object into one multi-part
    // object (upstream transform "assemble", OrcaSlicer.cpp:3409-3432). With
    // fewer than two objects there is nothing to merge — the service
    // requires >= 2, so a single object is an honest no-op.
    if (assemble_) {
        if (modelCount >= 2) {
            QList<int> indices;
            for (int i = 0; i < modelCount; ++i)
                indices.append(i);
            if (!projectService_->assembleObjects(indices)) {
                QTextStream err(stderr);
                err << "Error: --assemble failed\n";
                return CLI_INVALID_PARAMS;
            }
            if (!quiet_)
                out << "Assembled " << modelCount << " object(s)\n";
        } else if (!quiet_) {
            out << "Assemble: only one object loaded, nothing to merge\n";
        }
    }

    // Orient (upstream "orient" force-flag, OrcaSlicer.cpp:3466-3493).
    if (orient_) {
        const int count = projectService_->modelCount();
        for (int i = 0; i < count; ++i) {
            if (!projectService_->orientObject(i)) {
                QTextStream err(stderr);
                err << "Error: --orient failed on object " << i << "\n";
                return CLI_INVALID_PARAMS;
            }
        }
        if (!quiet_)
            out << "Oriented " << count << " object(s)\n";
    }

    // Cut (upstream REJECTS --cut: "Cut operation is not supported yet",
    // OrcaSlicer.cpp:3599-3600. OWzx has a real cut service
    // (ProjectServiceMock::cutObject), so --cut is wired here — documented
    // superset of upstream).
    if (cut_) {
        int count = projectService_->modelCount();
        // cutObject appends the new part; cut every ORIGINAL object once
        // (keepMode 0 = keep both halves, matching upstream ModelObjectCut-
        // Attribute::KeepUpper|KeepLower).
        for (int i = 0; i < count; ++i) {
            if (projectService_->cutObject(i, cutAxis_, cutPosition_, 0) < 0) {
                QTextStream err(stderr);
                err << "Error: --cut failed on object " << i << "\n";
                return CLI_INVALID_PARAMS;
            }
        }
        if (!quiet_)
            out << "Cut " << count << " object(s) at "
                << (cutAxis_ == 0 ? "x" : cutAxis_ == 1 ? "y" : "z")
                << "=" << cutPosition_ << " mm\n";
    }

    // Split (upstream "split", OrcaSlicer.cpp:3672-3680).
    if (split_) {
        int count = projectService_->modelCount();
        int totalNew = 0;
        for (int i = 0; i < count; ++i) {
            const QList<int> created = projectService_->splitObject(i);
            totalNew += created.size();
        }
        if (!quiet_)
            out << "Split created " << totalNew << " new object(s)\n";
    }

    // Repair (upstream notes models are repaired by default and its
    // --repair is a no-op, OrcaSlicer.cpp:3681-3684; OWzx has a real
    // fixMesh service, so the flag performs an explicit repair pass).
    if (repair_) {
        const int count = projectService_->modelCount();
        int repaired = 0;
        for (int i = 0; i < count; ++i) {
            if (projectService_->fixMesh(i))
                ++repaired;
        }
        if (!quiet_)
            out << "Repaired " << repaired << "/" << count << " object(s)\n";
    }

    // Scale-to-fit (upstream "scale_to_fit" Point3 target, OrcaSlicer.cpp:
    // 3588-3599: uniform min-ratio scale via ModelObject::scale_to_fit).
    if (scaleToFit_) {
        const int count = projectService_->modelCount();
        for (int i = 0; i < count; ++i) {
            // selectionWorldBoundingBox reports GL axes (x, z-up swap):
            // glX = sizeX, glY = sizeZ, glZ = sizeY.
            const QVariantMap box =
                projectService_->selectionWorldBoundingBox(QList<int>{i});
            if (box.isEmpty())
                continue;
            const double sx = box.value(QStringLiteral("maxX")).toDouble()
                              - box.value(QStringLiteral("minX")).toDouble();
            const double sy = box.value(QStringLiteral("maxY")).toDouble()
                              - box.value(QStringLiteral("minY")).toDouble();
            const double sz = box.value(QStringLiteral("maxZ")).toDouble()
                              - box.value(QStringLiteral("minZ")).toDouble();
            if (sx <= 0 || sy <= 0 || sz <= 0)
                continue;
            const double factor = std::min(scaleToFitX_ / sx,
                                           std::min(scaleToFitY_ / sz,
                                                    scaleToFitZ_ / sy));
            if (factor <= 0)
                continue;
            const QVector3D scale = projectService_->objectScale(i);
            projectService_->setObjectScale(
                i, float(scale.x() * factor), float(scale.y() * factor),
                float(scale.z() * factor));
        }
        if (!quiet_)
            out << "Scaled " << count << " object(s) to fit "
                << scaleToFitX_ << "," << scaleToFitY_ << "," << scaleToFitZ_
                << " mm\n";
    }

    // Arrange LAST so the transformed geometry lays out cleanly (upstream
    // runs arrange after the transform loop too, OrcaSlicer.cpp:3693+).
    // An unplaceable item is NOT fatal: arrangeObjects keeps the original
    // coordinates in that case (tolerant vfn, mirrors upstream ArrangeJob
    // which retains positions when bed_idx < 0) — report and continue.
    if (arrange_) {
        // Default printable area matches the GUI default bed (220x220).
        const bool placed = projectService_->arrangeObjects(
            5.0f, /*allowRotation=*/false, /*alignY=*/false,
            QStringLiteral("0,0,220,0,220,220,0,220"));
        if (!quiet_)
            out << (placed ? QStringLiteral("Arranged objects\n")
                           : QStringLiteral(
                                 "Arrange: some objects did not fit the bed, "
                                 "original positions kept\n"));
    }

    return CLI_SUCCESS;
}

int CliRunner::runExports(const QHash<int, QString> &slicedGcodes)
{
    QTextStream out(stdout);
    QString outDir = outputDir_.isEmpty()
                         ? QCoreApplication::applicationDirPath()
                         : outputDir_;
    QDir().mkpath(outDir);

    // --export-stl: one merged STL named after the first input
    // (upstream export_models(IO::STL) writes <input_basename>.stl).
    if (exportStl_) {
        const QString baseName = loadFiles_.isEmpty()
                                     ? QStringLiteral("output")
                                     : QFileInfo(loadFiles_.first()).completeBaseName();
        const QString stlPath = outDir + QStringLiteral("/") + baseName
                                + QStringLiteral(".stl");
        if (!projectService_->exportModel(stlPath, QStringLiteral("stl"))) {
            QTextStream err(stderr);
            err << "Error: --export-stl failed to write: " << stlPath << "\n";
            return CLI_DATA_FILE_ERROR;
        }
        if (!quiet_)
            out << "STL written: " << stlPath << "\n";
    }

    // --export-3mf: full project 3MF (upstream export_3mf, PrintConfig.cpp:
    // 7001-7005).
    if (!export3mfPath_.isEmpty()) {
        QString target = export3mfPath_;
        if (QFileInfo(target).isRelative())
            target = outDir + QStringLiteral("/") + target;
        if (!projectService_->saveProjectAs(target)) {
            QTextStream err(stderr);
            err << "Error: --export-3mf failed to write: " << target << "\n";
            return CLI_DATA_FILE_ERROR;
        }
        if (!quiet_)
            out << "3MF written: " << target << "\n";
    }

    // --export-slicedata: per-plate .gcode.3mf (upstream export_slicedata
    // stores the sliced plate data into a directory; OWzx writes the
    // upstream GCODE_FILE_FORMAT pair: <name>_plate<N>.gcode.3mf per sliced
    // plate via ProjectServiceMock::exportGcode3mf).
    if (!exportSlicedataDir_.isEmpty()) {
        QDir().mkpath(exportSlicedataDir_);
        if (slicedGcodes.isEmpty()) {
            QTextStream err(stderr);
            err << "Error: --export-slicedata found no sliced plates "
                   "(requires --slice)\n";
            return CLI_SLICING_ERROR;
        }
        const QString baseName = loadFiles_.isEmpty()
                                     ? QStringLiteral("output")
                                     : QFileInfo(loadFiles_.first()).completeBaseName();
        for (auto it = slicedGcodes.constBegin(); it != slicedGcodes.constEnd(); ++it) {
            const QString dest = exportSlicedataDir_ + QStringLiteral("/")
                                 + baseName + QStringLiteral("_plate%1.gcode.3mf").arg(it.key() + 1);
            if (!projectService_->exportGcode3mf(it.key(), dest, it.value())) {
                QTextStream err(stderr);
                err << "Error: --export-slicedata failed for plate "
                    << (it.key() + 1) << "\n";
                return CLI_SLICING_ERROR;
            }
            if (!quiet_)
                out << "Sliced data written: " << dest << "\n";
        }
    }

    return CLI_SUCCESS;
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

int CliRunner::runSlice(QHash<int, QString> *slicedGcodes)
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

    // Phase 241 (CLI-02): arbitrary --key value overrides layer on top of
    // everything (values validated at parse time; stored as strings so
    // SliceService's injectPresetConfig deserializes them with the option's
    // own type rules).
    if (!configOverrides_.isEmpty()) {
        for (auto it = configOverrides_.constBegin();
             it != configOverrides_.constEnd(); ++it) {
            mergedConfig[it.key()] = it.value();
            if (!quiet_)
                out << "Config override: " << it.key() << "=" << it.value() << "\n";
        }
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

    // v5.16 (CIRC-07): --plate 0 (the default) now genuinely slices ALL
    // printable plates, one G-code file per plate, matching the declared help
    // (upstream slices every plate by default). --plate N stays 1-based
    // single-plate slicing.
    const int plateCount = projectService_->plateCount();
    QList<int> plateQueue;
    if (plateIndex_ > 0) {
        if (plateIndex_ - 1 >= plateCount) {
            QTextStream err(stderr);
            err << "Error: plate index " << plateIndex_ << " out of range (1.."
                << plateCount << ")\n";
            return CLI_INVALID_PARAMS;
        }
        plateQueue.append(plateIndex_ - 1);
    } else {
        for (int p = 0; p < plateCount; ++p) {
            if (projectService_->isPlatePrintable(p))
                plateQueue.append(p);
            else if (!quiet_)
                out << "Skipping non-printable plate " << (p + 1) << "\n";
        }
    }
    if (plateQueue.isEmpty()) {
        QTextStream err(stderr);
        err << "Error: no printable plates to slice\n";
        return CLI_SLICING_ERROR;
    }

    // Connect progress once for the whole queue
    if (!quiet_) {
        connect(sliceService_, &SliceService::progressUpdated,
                this, [this](int percent, const QString &label) {
            QTextStream out(stdout);
            out << "  [" << percent << "%] " << label << "\n";
            out.flush();
        });
    }

    for (int plate : plateQueue) {
        projectService_->setCurrentPlateIndex(plate);

        if (!quiet_) {
            out << "Slicing plate " << (plate + 1) << " of " << plateCount << "\n";
            out << "  Model count: " << projectService_->modelCount() << "\n";
            out.flush();
        }

        // Fresh event loop per plate; its connections die with it.
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
            err << "Error: slice failed on plate " << (plate + 1) << ": "
                << sliceError << "\n";
            return sliceExitCode;
        }

        // Export G-code to output directory (plate-labeled name when multi-plate)
        const QString gcodeSrc = sliceService_->outputPath();
        if (gcodeSrc.isEmpty()) {
            QTextStream err(stderr);
            err << "Error: no G-code output from slice\n";
            return CLI_SLICING_ERROR;
        }
        const QString gcodeDest = outDir + QStringLiteral("/")
            + sliceService_->defaultExportGCodeFileName(plate);
        // Phase 239 (ENGN-03): the G-code copy runs on a QtConcurrent worker
        // (exportGCodeToPath returns once the export STARTS), so wait for the
        // finished/failed signal in a fresh event loop -- mirrors the slice
        // wait above.
        int exportExitCode = CLI_SLICING_ERROR;
        QString exportError;
        QEventLoop exportLoop;
        connect(sliceService_, &SliceService::exportFinished,
                &exportLoop, [&](const QString &) {
            exportExitCode = CLI_SUCCESS;
            exportLoop.quit();
        });
        connect(sliceService_, &SliceService::exportFailed,
                &exportLoop, [&](const QString &message) {
            exportError = message;
            exportLoop.quit();
        });
        if (!sliceService_->exportGCodeToPath(gcodeDest)) {
            QTextStream err(stderr);
            err << "Error: failed to export G-code to: " << gcodeDest << "\n";
            return CLI_SLICING_ERROR;
        }
        exportLoop.exec();
        if (exportExitCode != CLI_SUCCESS) {
            QTextStream err(stderr);
            err << "Error: failed to export G-code to: " << gcodeDest
                << ": " << exportError << "\n";
            return CLI_SLICING_ERROR;
        }

        if (!quiet_)
            out << "G-code written: " << gcodeDest << "\n";

        // Phase 241 (CLI-02): record the per-plate gcode path so
        // --export-slicedata can bundle them as .gcode.3mf.
        if (slicedGcodes)
            slicedGcodes->insert(plate, gcodeDest);
    }

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
