#include <QtTest/QtTest>
#include <QProcess>
#include <QTemporaryDir>
#include <QFileInfo>
#include <QDir>
#include <QTemporaryFile>

struct CliResult {
    int exitCode;
    QString stdout_;
    QString stderr_;
};

class CliTests : public QObject
{
    Q_OBJECT

    static QString cliExe()
    {
        return QCoreApplication::applicationDirPath() + QStringLiteral("/owzx-cli");
    }

    static CliResult runCli(const QStringList &args, int timeoutMs = 30000)
    {
        QProcess proc;
        proc.start(cliExe(), args);
        proc.waitForFinished(timeoutMs);
        return {
            proc.exitCode(),
            QString::fromLocal8Bit(proc.readAllStandardOutput()),
            QString::fromLocal8Bit(proc.readAllStandardError())
        };
    }

    static QString modelPath(const QString &relative)
    {
        // QT_TESTCASE_SOURCEDIR is set by CMake to the source root
        return QString::fromUtf8(QT_TESTCASE_SOURCEDIR) +
               QStringLiteral("/third_party/OrcaSlicer/resources/") + relative;
    }

    static QString findGcodeInDir(const QString &dir)
    {
        QDir d(dir);
        const auto entries = d.entryInfoList({QStringLiteral("*.gcode")}, QDir::Files);
        return entries.isEmpty() ? QString() : entries.first().absoluteFilePath();
    }

    static bool validateGcode(const QString &path)
    {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return false;
        const QByteArray data = f.readAll();
        if (data.size() < 100)
            return false;
        // Must contain G1 move commands and comment lines
        return data.contains("G1 ") && data.contains(";");
    }

private slots:
    void testHelpOption()
    {
        auto r = runCli({QStringLiteral("--help")});
        QCOMPARE(r.exitCode, 0);
        QVERIFY(r.stdout_.contains(QLatin1String("--load")));
        QVERIFY(r.stdout_.contains(QLatin1String("--slice")));
        QVERIFY(r.stdout_.contains(QLatin1String("--output-dir")));
        // Phase 241 (CLI-01/02): the transform/export surface is declared.
        QVERIFY(r.stdout_.contains(QLatin1String("--arrange")));
        QVERIFY(r.stdout_.contains(QLatin1String("--orient")));
        QVERIFY(r.stdout_.contains(QLatin1String("--cut")));
        QVERIFY(r.stdout_.contains(QLatin1String("--split")));
        QVERIFY(r.stdout_.contains(QLatin1String("--assemble")));
        QVERIFY(r.stdout_.contains(QLatin1String("--repair")));
        QVERIFY(r.stdout_.contains(QLatin1String("--scale-to-fit")));
        QVERIFY(r.stdout_.contains(QLatin1String("--export-stl")));
        QVERIFY(r.stdout_.contains(QLatin1String("--export-3mf")));
        QVERIFY(r.stdout_.contains(QLatin1String("--export-slicedata")));
    }

    void testNoArgs()
    {
        auto r = runCli({});
        QCOMPARE(r.exitCode, 0);
        QVERIFY(r.stdout_.contains(QLatin1String("--load")));
    }

    void testUnknownArg()
    {
        auto r = runCli({QStringLiteral("--bogus")});
        QVERIFY(r.exitCode != 0);
    }

    void testLoadHotend()
    {
        auto r = runCli({QStringLiteral("--load"), modelPath(QStringLiteral("profiles/hotend.stl"))});
        QCOMPARE(r.exitCode, 0);
        // Phase 241 hygiene: the CLI prints the object table header as
        // "Objects:" (capital O) — the stale lowercase "object" assertion
        // never matched the real output.
        QVERIFY(r.stdout_.contains(QLatin1String("Objects:")) ||
                r.stderr_.contains(QLatin1String("Objects:")));
    }

    void testLoadNonexistent()
    {
        auto r = runCli({QStringLiteral("--load"), QStringLiteral("/nonexistent/bad.stl")});
        QVERIFY(r.exitCode != 0);
    }

    void testSliceHotend()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        auto r = runCli({
            QStringLiteral("--load"), modelPath(QStringLiteral("profiles/hotend.stl")),
            QStringLiteral("--slice"),
            QStringLiteral("--output-dir"), tmpDir.path()
        }, 120000);
        QCOMPARE(r.exitCode, 0);
        QString gcode = findGcodeInDir(tmpDir.path());
        QVERIFY2(!gcode.isEmpty(), "No .gcode file found in output dir");
        QVERIFY2(validateGcode(gcode), qPrintable(QStringLiteral("Gcode validation failed: ") + gcode));
    }

    void testSliceBlock20XY()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        auto r = runCli({
            QStringLiteral("--load"), modelPath(QStringLiteral("test_models/Block20XY.stl")),
            QStringLiteral("--slice"),
            QStringLiteral("--output-dir"), tmpDir.path()
        }, 120000);
        QCOMPARE(r.exitCode, 0);
        QString gcode = findGcodeInDir(tmpDir.path());
        QVERIFY2(!gcode.isEmpty(), "No .gcode file found in output dir");
        QVERIFY2(validateGcode(gcode), qPrintable(QStringLiteral("Gcode validation failed: ") + gcode));
    }

    void testSliceNonexistent()
    {
        auto r = runCli({
            QStringLiteral("--load"), QStringLiteral("/nonexistent/bad.stl"),
            QStringLiteral("--slice")
        }, 30000);
        QVERIFY(r.exitCode != 0);
    }

    void testSliceMachineLimitsInGcode()
    {
        // Write a temporary settings JSON with machine limit overrides
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        const QString jsonPath = tmpDir.path() + QStringLiteral("/machine_limits.json");
        {
            QFile f(jsonPath);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("{\"emit_machine_limits_to_gcode\":true,"
                    "\"machine_max_speed_x\":[500],\"machine_max_speed_y\":[500],"
                    "\"machine_max_acceleration_x\":[1000],\"machine_max_acceleration_y\":[1000]}");
        }

        QTemporaryDir outDir;
        QVERIFY(outDir.isValid());
        auto r = runCli({
            QStringLiteral("--load"), modelPath(QStringLiteral("profiles/hotend.stl")),
            QStringLiteral("--load-settings"), jsonPath,
            QStringLiteral("--slice"),
            QStringLiteral("--output-dir"), outDir.path()
        }, 120000);
        QCOMPARE(r.exitCode, 0);

        const QString gcode = findGcodeInDir(outDir.path());
        QVERIFY2(!gcode.isEmpty(), "No .gcode file found in output dir");

        QFile gf(gcode);
        QVERIFY(gf.open(QIODevice::ReadOnly));
        const QString content = QString::fromUtf8(gf.readAll());

        // M201 = max acceleration, M203 = max feedrate/speed
        QVERIFY2(content.contains(QStringLiteral("M201")),
                 "M201 (max acceleration) missing from gcode — machine limits not injected");
        QVERIFY2(content.contains(QStringLiteral("M203")),
                 "M203 (max speed) missing from gcode — machine limits not injected");
    }

    // ── Phase 241 (CLI-01/02): transforms, exports, key overrides ────────

    void testOrientArrangeTransforms()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        auto r = runCli({
            QStringLiteral("--load"), modelPath(QStringLiteral("profiles/hotend.stl")),
            QStringLiteral("--orient"),
            QStringLiteral("--arrange"),
            QStringLiteral("--export-stl"),
            QStringLiteral("--output-dir"), tmpDir.path()
        }, 120000);
        QCOMPARE(r.exitCode, 0);
        QVERIFY(r.stdout_.contains(QLatin1String("Oriented")));
        // Arrange either places every object or honestly keeps the original
        // positions when an item does not fit the bed (upstream ArrangeJob
        // tolerance) — both are successful CLI outcomes.
        QVERIFY(r.stdout_.contains(QLatin1String("Arranged objects"))
                || r.stdout_.contains(QLatin1String("original positions kept")));
        QVERIFY(QFileInfo::exists(tmpDir.path() + QStringLiteral("/hotend.stl")));
    }

    void testCutAndScaleToFitTransforms()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        auto r = runCli({
            QStringLiteral("--load"), modelPath(QStringLiteral("profiles/hotend.stl")),
            QStringLiteral("--cut"), QStringLiteral("z:2"),
            QStringLiteral("--scale-to-fit"), QStringLiteral("50,50,50"),
            QStringLiteral("--export-stl"),
            QStringLiteral("--output-dir"), tmpDir.path()
        }, 120000);
        QCOMPARE(r.exitCode, 0);
        QVERIFY(r.stdout_.contains(QLatin1String("Cut ")));
        QVERIFY(r.stdout_.contains(QLatin1String("Scaled")));
    }

    void testScaleToFitRejectsNonPositiveTarget()
    {
        // Upstream "--scale-to-fit requires a positive volume"
        // (OrcaSlicer.cpp:3591).
        auto r = runCli({
            QStringLiteral("--load"), modelPath(QStringLiteral("profiles/hotend.stl")),
            QStringLiteral("--scale-to-fit"), QStringLiteral("0,0,0"),
            QStringLiteral("--export-stl")
        }, 60000);
        QVERIFY(r.exitCode != 0);
        QVERIFY(r.stderr_.contains(QLatin1String("positive volume")));
    }

    void testCutRejectsBadAxis()
    {
        auto r = runCli({
            QStringLiteral("--load"), modelPath(QStringLiteral("profiles/hotend.stl")),
            QStringLiteral("--cut"), QStringLiteral("q:5"),
            QStringLiteral("--export-stl")
        }, 60000);
        QVERIFY(r.exitCode != 0);
        QVERIFY(r.stderr_.contains(QLatin1String("axis")));
    }

    void testRepairTransform()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        auto r = runCli({
            QStringLiteral("--load"), modelPath(QStringLiteral("profiles/hotend.stl")),
            QStringLiteral("--repair"),
            QStringLiteral("--export-stl"),
            QStringLiteral("--output-dir"), tmpDir.path()
        }, 120000);
        QCOMPARE(r.exitCode, 0);
        QVERIFY(r.stdout_.contains(QLatin1String("Repaired")));
    }

    void testExport3mf()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        auto r = runCli({
            QStringLiteral("--load"), modelPath(QStringLiteral("profiles/hotend.stl")),
            QStringLiteral("--export-3mf"), QStringLiteral("proj.3mf"),
            QStringLiteral("--output-dir"), tmpDir.path()
        }, 120000);
        QCOMPARE(r.exitCode, 0);
        QVERIFY(r.stdout_.contains(QLatin1String("3MF written")));
        QVERIFY(QFileInfo::exists(tmpDir.path() + QStringLiteral("/proj.3mf")));
    }

    void testExportSlicedataRequiresSlice()
    {
        auto r = runCli({
            QStringLiteral("--load"), modelPath(QStringLiteral("profiles/hotend.stl")),
            QStringLiteral("--export-slicedata"), QStringLiteral("sd_out")
        }, 60000);
        QVERIFY(r.exitCode != 0);
        QVERIFY(r.stderr_.contains(QLatin1String("--export-slicedata requires --slice")));
    }

    void testExportSlicedataAfterSlice()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        const QString sdDir = tmpDir.path() + QStringLiteral("/sd");
        auto r = runCli({
            QStringLiteral("--load"), modelPath(QStringLiteral("profiles/hotend.stl")),
            QStringLiteral("--slice"),
            QStringLiteral("--export-slicedata"), sdDir,
            QStringLiteral("--output-dir"), tmpDir.path()
        }, 180000);
        QCOMPARE(r.exitCode, 0);
        QVERIFY(r.stdout_.contains(QLatin1String("Sliced data written")));
        const QString bundle = sdDir + QStringLiteral("/hotend_plate1.gcode.3mf");
        QVERIFY2(QFileInfo::exists(bundle),
                 qPrintable(QStringLiteral("missing sliced-data bundle: ") + bundle));
    }

    void testUnknownConfigKeyRejected()
    {
        // Phase 241 (CLI-02): unknown --key overrides are a hard error
        // (upstream CLI semantics).
        auto r = runCli({
            QStringLiteral("--load"), modelPath(QStringLiteral("profiles/hotend.stl")),
            QStringLiteral("--definitely_not_a_config_option"), QStringLiteral("1")
        }, 60000);
        QVERIFY(r.exitCode != 0);
        QVERIFY(r.stderr_.contains(QLatin1String("unknown option")));
    }

    void testInvalidConfigValueRejected()
    {
        auto r = runCli({
            QStringLiteral("--load"), modelPath(QStringLiteral("profiles/hotend.stl")),
            QStringLiteral("--layer_height"), QStringLiteral("notanumber")
        }, 60000);
        QVERIFY(r.exitCode != 0);
        QVERIFY(r.stderr_.contains(QLatin1String("invalid value")));
    }

    void testConfigOverrideReachesGcode()
    {
        // Phase 241 (CLI-02): a validated --key value override layers onto
        // the merged preset config and reaches the emitted G-code config
        // block ("; wall_loops = 3").
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        auto r = runCli({
            QStringLiteral("--load"), modelPath(QStringLiteral("profiles/hotend.stl")),
            QStringLiteral("--wall_loops"), QStringLiteral("3"),
            QStringLiteral("--slice"),
            QStringLiteral("--output-dir"), tmpDir.path()
        }, 180000);
        QCOMPARE(r.exitCode, 0);
        QVERIFY(r.stdout_.contains(QLatin1String("Config override: wall_loops=3")));
        const QString gcode = findGcodeInDir(tmpDir.path());
        QVERIFY2(!gcode.isEmpty(), "No .gcode file found in output dir");
        QFile gf(gcode);
        QVERIFY(gf.open(QIODevice::ReadOnly));
        const QString content = QString::fromUtf8(gf.readAll());
        QVERIFY2(content.contains(QStringLiteral("; wall_loops = 3")),
                 "the wall_loops override must land in the gcode config block");
    }
};

QTEST_MAIN(CliTests)
#include "CliTests.moc"
