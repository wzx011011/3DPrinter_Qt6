#pragma once

#include <QObject>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariant>

class ProjectServiceMock;
class SliceService;
class PresetServiceMock;

/// CLI orchestrator — wires services and runs the headless pipeline.
/// Aligns with upstream CLI::run() in OrcaSlicer.cpp (BBS CLI surface):
/// pre-slice transforms (arrange/orient/cut/split/assemble/repair/
/// scale-to-fit, OrcaSlicer.cpp:3397-3690), exports (export_stl/export_3mf/
/// export_slicedata, PrintConfig.cpp:7006-7029 + OrcaSlicer.cpp:4660-4710),
/// and arbitrary `--key value` PrintConfig overrides validated against the
/// DynamicPrintConfig option schema (unknown key -> error exit).
class CliRunner : public QObject
{
    Q_OBJECT
public:
    explicit CliRunner(QObject *parent = nullptr);

    /// Parse args and execute. Returns exit code (0=success, <0=error per CLI_* macros).
    int run();

private:
    void printUsage();
    int loadAndPrintInfo();
    /// slicedGcodes (optional) collects plate index -> exported .gcode path
    /// for --export-slicedata.
    int runSlice(QHash<int, QString> *slicedGcodes = nullptr);
    int directSlice();
    QHash<QString, QVariant> loadSettingsJson(const QString &path);

    /// Phase 241 (CLI-01): pre-slice transforms over every loaded object
    /// (upstream transform loop, OrcaSlicer.cpp:3397+). Each flag maps to a
    /// real ProjectServiceMock call; failures return a CLI_* error code.
    int applyTransforms();
    /// Phase 241 (CLI-02): post-load/post-slice exports. slicedGcodes maps
    /// plate index -> exported .gcode path (drives --export-slicedata).
    int runExports(const QHash<int, QString> &slicedGcodes);

    /// Phase 241 (CLI-02): extract `--key value` pairs that are NOT declared
    /// CLI options from the raw argv (upstream boost::program_options
    /// allow_unregistered semantics) and validate each key against the
    /// DynamicPrintConfig schema. Returns false + prints an error on an
    /// unknown key / invalid value.
    bool extractConfigOverrides(const QStringList &rawArgs,
                                 const QStringList &knownOptions);

    ProjectServiceMock *projectService_ = nullptr;
    SliceService *sliceService_ = nullptr;
    PresetServiceMock *presetService_ = nullptr;

    QStringList loadFiles_;
    QString loadSettingsPath_;
    bool doSlice_ = false;
    int plateIndex_ = 0;
    QString outputDir_;
    bool quiet_ = false;

    // Phase 241 (CLI-01): transform flags (upstream OrcaSlicer.cpp transforms).
    bool arrange_ = false;
    bool orient_ = false;
    bool split_ = false;
    bool assemble_ = false;
    bool repair_ = false;
    bool cut_ = false;
    int cutAxis_ = -1;          ///< 0=X, 1=Y, 2=Z
    double cutPosition_ = 0.0;  ///< mm along the axis
    bool scaleToFit_ = false;
    double scaleToFitX_ = 0.0, scaleToFitY_ = 0.0, scaleToFitZ_ = 0.0;

    // Phase 241 (CLI-02): export targets.
    bool exportStl_ = false;
    QString export3mfPath_;
    QString exportSlicedataDir_;

    /// Phase 241 (CLI-02): arbitrary PrintConfig key overrides (QString
    /// values; SliceService's injectPresetConfig deserializes them through
    /// set_deserialize_strict).
    QHash<QString, QString> configOverrides_;
};
