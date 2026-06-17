#pragma once

#include <QObject>

class ProjectServiceMock;
class SliceService;
class PresetServiceMock;

/// CLI orchestrator — wires services and runs the headless pipeline.
/// Aligns with upstream CLI::run() in CrealityPrint.cpp.
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
    int runSlice();
    int directSlice();
    QHash<QString, QVariant> loadSettingsJson(const QString &path);

    ProjectServiceMock *projectService_ = nullptr;
    SliceService *sliceService_ = nullptr;
    PresetServiceMock *presetService_ = nullptr;

    QStringList loadFiles_;
    QString loadSettingsPath_;
    bool doSlice_ = false;
    int plateIndex_ = 0;
    QString outputDir_;
    bool quiet_ = false;
};
