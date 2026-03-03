#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTextStream>
#include <QtTest>
#include <QtQml/qqml.h>

#include "qml_gui/BackendContext.h"
#include "GLViewportTestStub.h" // lightweight stand-in, no OpenGL required

class VisualRegressionTests final : public QObject
{
  Q_OBJECT

private slots:
  void main_pages_visual_baseline();

private:
  static double pixelDiffRatio(const QImage &left, const QImage &right);
  static QString resolveReferenceDir();
};

double VisualRegressionTests::pixelDiffRatio(const QImage &left, const QImage &right)
{
  if (left.size() != right.size())
  {
    return 1.0;
  }

  const QImage lhs = left.convertToFormat(QImage::Format_RGBA8888);
  const QImage rhs = right.convertToFormat(QImage::Format_RGBA8888);

  int differentPixels = 0;
  const int totalPixels = lhs.width() * lhs.height();

  for (int y = 0; y < lhs.height(); ++y)
  {
    const QRgb *rowL = reinterpret_cast<const QRgb *>(lhs.constScanLine(y));
    const QRgb *rowR = reinterpret_cast<const QRgb *>(rhs.constScanLine(y));

    for (int x = 0; x < lhs.width(); ++x)
    {
      const int dr = qAbs(qRed(rowL[x]) - qRed(rowR[x]));
      const int dg = qAbs(qGreen(rowL[x]) - qGreen(rowR[x]));
      const int db = qAbs(qBlue(rowL[x]) - qBlue(rowR[x]));
      const int da = qAbs(qAlpha(rowL[x]) - qAlpha(rowR[x]));
      if (dr + dg + db + da > 24)
      {
        differentPixels++;
      }
    }
  }

  return totalPixels > 0 ? static_cast<double>(differentPixels) / static_cast<double>(totalPixels) : 1.0;
}

QString VisualRegressionTests::resolveReferenceDir()
{
  const QString envDir = qEnvironmentVariable("CREALITY_UI_REFERENCE_DIR");
  if (!envDir.trimmed().isEmpty() && QDir(envDir).exists())
  {
    return QDir::cleanPath(envDir);
  }

  const QString root = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
  const QString preferred = root + "/tests/reference";
  if (QDir(preferred).exists())
  {
    return preferred;
  }

  const QString fallback = root + "/tests/baseline";
  if (QDir(fallback).exists())
  {
    return fallback;
  }

  return preferred;
}

void VisualRegressionTests::main_pages_visual_baseline()
{
  // Register GL viewport type (must be before engine.load)
  qmlRegisterType<GLViewport>("CrealityGL", 1, 0, "GLViewport");

  BackendContext backend;

  // Heap-allocate engine and intentionally leak it.
  // Qt 6.10 debug heap corrupts on QQmlApplicationEngine::~QQmlApplicationEngine
  // when it destroys QQmlElement<GLViewport> children (known V4 issue).
  auto *engine = new QQmlApplicationEngine;
  engine->rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
  engine->load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
  QVERIFY2(!engine->rootObjects().isEmpty(), "Failed to load qrc:/qml/main.qml");

  auto *window = qobject_cast<QQuickWindow *>(engine->rootObjects().first());
  QVERIFY2(window != nullptr, "Root object is not a QQuickWindow");

  window->show();
  QTest::qWait(250);

  const QString referenceDir = resolveReferenceDir();
  const QString outputDir = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../tests/output");
  QDir().mkpath(referenceDir);
  QDir().mkpath(outputDir);

  const bool captureOnly = qEnvironmentVariable("CREALITY_UI_CAPTURE_ONLY") == QStringLiteral("1");
  const QString reportPath = outputDir + "/visual_report.txt";
  QFile reportFile(reportPath);
  QVERIFY2(reportFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text),
           qPrintable(QStringLiteral("Failed to open report file: %1").arg(reportPath)));
  QTextStream report(&reportFile);
  report << "Visual Regression Report\n";
  report << "Time: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
  report << "Reference Dir: " << referenceDir << "\n";
  report << "Output Dir: " << outputDir << "\n";
  report << "Capture Only: " << (captureOnly ? "true" : "false") << "\n\n";

  struct PageCase
  {
    int index;
    QString name;
  };

  const QList<PageCase> pages = {
      {1, QStringLiteral("prepare")},
      {2, QStringLiteral("preview")},
      {3, QStringLiteral("monitor")},
  };

  constexpr double maxDiffRatio = 0.020;
  int comparedCount = 0;

  for (const PageCase &page : pages)
  {
    backend.setCurrentPage(page.index);
    QTest::qWait(180);

    QImage shot = window->grabWindow();
    QVERIFY2(!shot.isNull(), "Failed to grab window image");

    const QImage normalized = shot.scaled(720, 430, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    const QString outputPath = outputDir + "/" + page.name + ".png";
    const QString referencePath = referenceDir + "/" + page.name + ".png";

    QVERIFY2(normalized.save(outputPath), qPrintable(QStringLiteral("Failed to save output image: %1").arg(outputPath)));
    report << "Page: " << page.name << "\n";
    report << "  Captured: " << outputPath << "\n";

    if (!QFileInfo::exists(referencePath))
    {
      QVERIFY2(normalized.save(referencePath), qPrintable(QStringLiteral("Failed to create reference image: %1").arg(referencePath)));
      report << "  Reference: created " << referencePath << "\n";
      continue;
    }

    report << "  Reference: " << referencePath << "\n";

    if (captureOnly)
    {
      report << "  Compare: skipped (capture-only mode)\n\n";
      continue;
    }

    const QImage reference(referencePath);
    QVERIFY2(!reference.isNull(), qPrintable(QStringLiteral("Failed to load reference image: %1").arg(referencePath)));
    const QImage normalizedReference = reference.scaled(720, 430, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    comparedCount++;

    const double ratio = pixelDiffRatio(normalized, normalizedReference);
    report << "  DiffRatio: " << QString::number(ratio, 'f', 4)
           << " (threshold " << QString::number(maxDiffRatio, 'f', 4) << ")\n\n";

    QVERIFY2(ratio <= maxDiffRatio,
             qPrintable(QStringLiteral("Visual diff too high for page '%1': %2 > %3")
                            .arg(page.name)
                            .arg(ratio, 0, 'f', 4)
                            .arg(maxDiffRatio, 0, 'f', 4)));
  }

  report << "Compared Pages: " << comparedCount << "\n";
  report << "Status: PASS\n";
  reportFile.close();
}

QTEST_MAIN(VisualRegressionTests)
#include "VisualRegressionTests.moc"
