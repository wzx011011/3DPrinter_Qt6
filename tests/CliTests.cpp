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
        QVERIFY(r.stdout_.contains(QLatin1String("object")) ||
                r.stderr_.contains(QLatin1String("object")));
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
};

QTEST_MAIN(CliTests)
#include "CliTests.moc"
