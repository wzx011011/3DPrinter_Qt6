// VisualRegressionTests — X4 visual regression suite (audit 2026-10-10 ④ X4).
//
// The first automated visual-truth gate for a screenshot-driven milestone
// project: renders the key pages (Prepare / Preview / Monitor / Preferences /
// ...) at a FIXED viewport under the offscreen QPA platform + the Qt Quick
// SOFTWARE backend, and compares each grab against the committed baseline PNG
// in tests/visual_baseline/ with a per-pixel tolerance + mismatch-fraction
// budget. ENABLE_VISUAL_REGRESSION (root CMakeLists) gates the target; the
// canonical verify (scripts/auto_verify_with_vcvars.ps1) builds and runs it
// as a blocking suite.
//
// Determinism contract (why this does not flake):
//   1. QT_QPA_PLATFORM=offscreen + QT_QUICK_BACKEND=software: pure-CPU scene
//      graph rasterization — no GPU, no driver, no window manager. (Software
//      rendering also neutralizes the "driver difference" risk the audit
//      names; RHI-backed GLViewport/QQuickRhiItem areas render as a
//      deterministic blank because Qt's software backend does not support
//      QQuickRhiItem — the renderer itself stays covered by the stage bench
//      and owzx-render-bench, not here.)
//   2. Fixed 1440x900 viewport at QT_SCALE_FACTOR=1.
//   3. Stability hooks (the audit's "冻结定时器/固定模型"): after a settle
//      window every Timer/*Animation*/AnimatedImage under the page is
//      stopped, cursor flashing is disabled (QStyleHints::setCursorFlashTime),
//      and the fixed model is the same fresh-BackendContext page wiring
//      QmlSmokeTests uses (temp-dir QSettings, no project loaded).
//   4. A page frame is only accepted after a DOUBLE GRAB matches byte-exact;
//      if the frame keeps changing the row fails with "never stabilized" —
//      that is the signal that new time-dependent UI needs a hook, not a
//      tolerance bump.
//
// Comparison: QImage::Format_RGB32 pixel walk; a pixel mismatches when any
// channel differs by more than kPixelChannelTolerance; the row fails when
// more than kMaxMismatchFraction of pixels mismatch. Size mismatch fails
// outright (viewport or renderer change — a conscious baseline update, never
// a silent tolerance case). Perceptual-hash distance (16x16 average hash) is
// computed for the failure message only, to help triage "layout broken" vs
// "ambient drift".
//
// QML engine messages are deliberately NOT asserted here — that is Q14's
// (QmlSmokeTests) contract with its own allowlist. Pixels are this suite's
// truth: a QML error that changes visuals bakes into the baseline and
// surfaces when it gets fixed (via the baseline-update flow); one that does
// not change visuals is not a visual regression.
//
// Baseline policy (anti "update the baseline and it passes"):
//   - This suite NEVER writes baselines in gate mode; a missing baseline
//     FAILs. Baselines are written only by an explicit developer run with
//     OWZX_VISREG_UPDATE_BASELINE=1 (see tests/visual_baseline/README.md
//     for the full review process) and enter the repo as reviewable PNG
//     diffs that must travel WITH the QML change they bless.
//
// Env knobs:
//   OWZX_VISREG_UPDATE_BASELINE=1  developer baseline (re)capture mode
//   OWZX_VISREG_PAGE=<name>        capture/compare a single page row
//   OWZX_VISREG_PLATFORM=<plugin>  override the offscreen QPA platform
//                                  (debugging only — pixels are then not
//                                  comparable to the committed baselines)
//
// Engine/backend lifetime mirrors QmlSmokeTests: BackendContext, the engine
// and the last page window are intentionally leaked at process exit —
// teardown of a fully-populated QML tree is not under test here.

#include <QtTest>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QSet>
#include <QSettings>
#include <QStyleHints>
#include <QTemporaryDir>
#include <QtGlobal>
#include <array>
#include <cstring>
#include <memory>

#include "core/debug/CrashHandlerWin.h"
#include "qml_gui/BackendContext.h"
#include "qml_gui/CameraImageProvider.h"
#include "qml_gui/Renderer/RhiViewport.h"
#include "qml_gui/Models/ConfigOptionFilterProxy.h"

namespace {

// Fixed viewport every baseline is captured at. Keep in sync with the README
// in tests/visual_baseline/ — a change here invalidates every baseline.
constexpr int kViewportWidth = 1440;
constexpr int kViewportHeight = 900;

// Per-channel absolute difference a pixel may show and still count as equal
// (antialiasing-level slack; software-rasterized same-Qt runs sit at 0-2).
constexpr int kPixelChannelTolerance = 12;

// Fraction of pixels allowed to exceed the channel tolerance (0.2% ≈ 2600
// pixels at 1440x900 — localized layout damage like a displaced 50x50 button
// blows straight through this; a whole-theme drift shifts essentially every
// pixel and fails too).
constexpr double kMaxMismatchFraction = 0.002;

// Settle window after page show before freezing: MonitorPage owns a 1000ms
// repeating refresh timer (MonitorPage.qml:228-234, upstream REFRESH_INTERVAL
// mirror); 1500ms guarantees exactly one refresh tick has fired before the
// frame is taken, on every machine, instead of a timing race.
constexpr int kDefaultSettleMs = 400;
constexpr int kMonitorSettleMs = 1500;

// Stability round-trip wait and round budget for the double-grab loop.
constexpr int kStabilityRoundWaitMs = 120;
constexpr int kMaxStabilityRounds = 6;

bool updateBaselineMode()
{
  return qEnvironmentVariable("OWZX_VISREG_UPDATE_BASELINE") == QLatin1String("1");
}

QString pageFilter()
{
  return qEnvironmentVariable("OWZX_VISREG_PAGE");
}

QString baselineDir()
{
  return QStringLiteral(QT_TESTCASE_SOURCEDIR) + QStringLiteral("/tests/visual_baseline");
}

// aHash (16x16 grayscale average hash) hamming distance — failure-message
// triage only, never a pass criterion.
int averageHashHamming(const QImage &a, const QImage &b)
{
  constexpr int kSide = 16;
  constexpr int kWords = kSide * kSide / 64;
  const auto hash = [](const QImage &image) {
    const QImage gray = image.scaled(kSide, kSide, Qt::IgnoreAspectRatio,
                                     Qt::SmoothTransformation)
                            .convertToFormat(QImage::Format_Grayscale8);
    std::array<quint64, kWords> bits{};
    int sum = 0;
    quint8 lum[kSide * kSide];
    for (int y = 0; y < kSide; ++y) {
      for (int x = 0; x < kSide; ++x) {
        const quint8 v = gray.scanLine(y)[x];
        lum[y * kSide + x] = v;
        sum += v;
      }
    }
    const int mean = sum / (kSide * kSide);
    int bit = 0;
    for (int i = 0; i < kSide * kSide; ++i, ++bit) {
      if (lum[i] > mean)
        bits[bit / 64] |= (quint64(1) << (bit % 64));
    }
    return bits;
  };
  const std::array<quint64, kWords> ha = hash(a);
  const std::array<quint64, kWords> hb = hash(b);
  int distance = 0;
  for (int i = 0; i < kWords; ++i) {
    distance += qPopulationCount(ha[i] ^ hb[i]);
  }
  return distance;
}

struct CompareMetrics
{
  bool sizeOk = false;
  QSize baselineSize;
  QSize actualSize;
  qsizetype mismatchedPixels = 0;
  double mismatchedFraction = 0.0;
  int maxChannelDiff = 0;
  double meanAbsDiff = 0.0;
  int aHashHamming = -1;
};

CompareMetrics compareImages(const QImage &baselineIn, const QImage &actualIn)
{
  CompareMetrics m;
  m.baselineSize = baselineIn.size();
  m.actualSize = actualIn.size();
  m.sizeOk = baselineIn.size() == actualIn.size();
  if (!m.sizeOk)
    return m;

  const QImage baseline = baselineIn.convertToFormat(QImage::Format_RGB32);
  const QImage actual = actualIn.convertToFormat(QImage::Format_RGB32);

  const qsizetype totalPixels = qsizetype(baseline.width()) * baseline.height();
  qsizetype diffAccum = 0;
  for (int y = 0; y < baseline.height(); ++y) {
    const QRgb *rowA = reinterpret_cast<const QRgb *>(baseline.constScanLine(y));
    const QRgb *rowB = reinterpret_cast<const QRgb *>(actual.constScanLine(y));
    for (int x = 0; x < baseline.width(); ++x) {
      const int dr = qRed(rowA[x]) - qRed(rowB[x]);
      const int dg = qGreen(rowA[x]) - qGreen(rowB[x]);
      const int db = qBlue(rowA[x]) - qBlue(rowB[x]);
      const int ad = qAbs(dr), ag = qAbs(dg), ab = qAbs(db);
      const int channelMax = qMax(ad, qMax(ag, ab));
      diffAccum += ad + ag + ab;
      if (channelMax > m.maxChannelDiff)
        m.maxChannelDiff = channelMax;
      if (channelMax > kPixelChannelTolerance)
        ++m.mismatchedPixels;
    }
  }
  m.mismatchedFraction = totalPixels > 0
      ? double(m.mismatchedPixels) / double(totalPixels) : 0.0;
  m.meanAbsDiff = totalPixels > 0
      ? double(diffAccum) / (double(totalPixels) * 3.0) : 0.0;
  m.aHashHamming = averageHashHamming(baseline, actual);
  return m;
}

// Red-marks every mismatching pixel over a dimmed copy of the actual grab.
QImage buildDiffVisualization(const QImage &baselineIn, const QImage &actualIn)
{
  QImage actual = actualIn.convertToFormat(QImage::Format_RGB32);
  const QImage baseline = baselineIn.convertToFormat(QImage::Format_RGB32);
  for (int y = 0; y < actual.height(); ++y) {
    QRgb *rowA = reinterpret_cast<QRgb *>(actual.scanLine(y));
    const QRgb *rowB = reinterpret_cast<const QRgb *>(baseline.constScanLine(y));
    for (int x = 0; x < actual.width(); ++x) {
      const int channelMax = qMax(qAbs(qRed(rowA[x]) - qRed(rowB[x])),
                                  qMax(qAbs(qGreen(rowA[x]) - qGreen(rowB[x])),
                                       qAbs(qBlue(rowA[x]) - qBlue(rowB[x]))));
      if (channelMax > kPixelChannelTolerance)
        rowA[x] = qRgb(255, 32, 32);
      else
        rowA[x] = qRgb(qRed(rowA[x]) / 3, qGreen(rowA[x]) / 3, qBlue(rowA[x]) / 3);
    }
  }
  return actual;
}

// Stability hook (audit X4: "冻结定时器"): stop() every Timer / *Animation*
// under `root`, and stop AnimatedImage playback. Uses QMetaObject::invokeMethod
// by name so no private Qt headers are needed; classes without a stop()
// invokable are skipped gracefully. Returns how many objects were stopped.
int freezeTimersAndAnimations(QObject *root)
{
  int frozen = 0;
  const QList<QObject *> children =
      root->findChildren<QObject *>(QString(), Qt::FindChildrenRecursively);
  for (QObject *object : children) {
    const char *className = object->metaObject()->className();
    const bool isTimer = std::strstr(className, "Timer") != nullptr;
    const bool isAnimation = std::strstr(className, "Animation") != nullptr;
    const bool isAnimatedImage = std::strstr(className, "AnimatedImage") != nullptr;
    if (!isTimer && !isAnimation && !isAnimatedImage)
      continue;
    if (isAnimatedImage) {
      if (object->setProperty("running", false))
        ++frozen;
      continue;
    }
    if (QMetaObject::invokeMethod(object, "stop"))
      ++frozen;
  }
  return frozen;
}

} // namespace

class VisualRegressionTests : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void pagesRender();
  void pagesRender_data();
  void cleanupTestCase();

private:
  QImage captureStableFrame(QQuickWindow *window, bool *stable,
                            QString *diagnostic);
  void failWithMetrics(const QString &page, const QImage &baseline,
                       const QImage &actual, const CompareMetrics &metrics);
  void pinDailyTipHint();

  // Leaked intentionally (see file header). backend_ is the engine-created
  // OWzx module singleton (S8). tempDir_ keeps QSettings isolation alive.
  BackendContext *backend_ = nullptr;
  std::unique_ptr<QTemporaryDir> tempDir_;
  QQmlApplicationEngine *engine_ = nullptr;
  int m_pagesCaptured = 0;
  QStringList m_rewrittenBaselines;
  // Page names of the current data table, filled by pagesRender_data and
  // used by cleanupTestCase's stale-baseline hygiene check.
  QStringList m_pageRows;
};

void VisualRegressionTests::initTestCase()
{
  // Q13 convention: fail-fast crash handler so a crash leaves
  // build/crash_dumps/ evidence instead of a silent nonzero exit.
#ifdef Q_OS_WIN
  CrashHandlerWin::install(QCoreApplication::applicationDirPath() +
                           QStringLiteral("/crash_dumps"));
#endif

  // Same type registrations and bundled-font setup as main_qml.cpp /
  // QmlSmokeTests initTestCase, so the pages resolve the identical OWzx
  // module, singletons and font family as the shipped app.
  qmlRegisterType<RhiViewport>("OWzx", 1, 0, "GLViewport");
  qmlRegisterType<ConfigOptionFilterProxy>("OWzx", 1, 0,
                                           "ConfigOptionFilterProxy");
  QFontDatabase::addApplicationFont(
      QStringLiteral(":/qml/assets/fonts/HarmonyOS_Sans_SC_Regular.ttf"));
  QFontDatabase::addApplicationFont(
      QStringLiteral(":/qml/assets/fonts/HarmonyOS_Sans_SC_Bold.ttf"));

  // Registry isolation, same as QmlSmokeTests: the AppSettings singleton is
  // QSettings-backed; redirect to a temp INI before the engine exists.
  tempDir_ = std::make_unique<QTemporaryDir>();
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                     tempDir_->filePath(QStringLiteral("settings")));

  // Disable cursor blinking BEFORE any page exists: a focused text input's
  // blink phase is wall-clock driven and would make text fields flicker
  // between runs. Flash time 0 keeps the cursor permanently drawn.
  QGuiApplication::styleHints()->setCursorFlashTime(0);

  engine_ = new QQmlApplicationEngine;
  extern void qml_register_types_OWzx();
  qml_register_types_OWzx();

  backend_ = engine_->singletonInstance<BackendContext *>("OWzx", "Backend");
  QVERIFY2(backend_ != nullptr,
           "OWzx module must expose the Backend singleton (BackendContext)");
  backend_->applyStartupPagePreference();
  engine_->addImageProvider(QStringLiteral("camera"),
                            new CameraImageProvider(backend_->cameraService()));

  // The engine-side QTimers (m_hintTimer / m_backupTimer, BackendContext.cpp
  // ~:265/:281) are NOT under any page's item tree, so the per-row window
  // freeze never reaches them. Stop them once here; idempotent, and it keeps
  // a future interval shortening from leaking timing into the pixels.
  freezeTimersAndAnimations(backend_);

  if (updateBaselineMode())
    qWarning("[VisualRegression] UPDATE-BASELINE MODE: captures will OVERWRITE "
             "tests/visual_baseline/*.png. This never runs from the verify "
             "gate; the rewritten PNGs must be committed together with the "
             "QML change they bless (see tests/visual_baseline/README.md).");
}

QImage VisualRegressionTests::captureStableFrame(QQuickWindow *window,
                                                 bool *stable,
                                                 QString *diagnostic)
{
  *stable = false;
  const int frozen = freezeTimersAndAnimations(window);

  QImage previous;
  for (int round = 0; round < kMaxStabilityRounds; ++round) {
    QTest::qWait(kStabilityRoundWaitMs);
    QCoreApplication::processEvents();
    const QImage frame = window->grabWindow();
    if (!previous.isNull() && previous == frame) {
      *stable = true;
      return frame;
    }
    previous = frame;
  }
  *diagnostic = QStringLiteral(
                    "frame never stabilized after %1 rounds (freeze stopped %2 "
                    "timers/animations) — new time-dependent UI needs a hook "
                    "before its pixels can be baselined")
                    .arg(kMaxStabilityRounds)
                    .arg(frozen);
  return previous;
}

void VisualRegressionTests::pagesRender_data()
{
  QTest::addColumn<QString>("pageName");
  QTest::addColumn<QString>("qmlPath");
  QTest::addColumn<QVariantMap>("initialProps");
  QTest::addColumn<int>("settleMs");

  auto vm = [this](const char *prop) {
    return QVariant::fromValue(backend_->property(prop).value<QObject *>());
  };

  // Required-property bindings copied verbatim from QmlSmokeTests
  // pagesInstantiate_data (they mirror main.qml:928-1046 / Plater.qml:33-39).
  // Plater.qml is deliberately NOT a row: it hosts the three RHI viewports
  // (Prepare/Preview/Assemble capture their panels standalone; the viewport
  // rectangle is a deterministic blank under the software backend and the
  // renderer is bench-covered elsewhere).
  m_pageRows.clear();
  const auto row = [this](const QString &name, const QString &path,
                          const QVariantMap &props, int settleMs) {
    m_pageRows.append(name);
    QTest::newRow(name.toUtf8().constData()) << name << path << props
                                             << settleMs;
  };
  row(QStringLiteral("HomePage"), QStringLiteral("qrc:/qml/OWzx/pages/HomePage.qml"),
      QVariantMap{{"homeVm", vm("homeViewModel")}}, kDefaultSettleMs);
  row(QStringLiteral("PreparePage"),
      QStringLiteral("qrc:/qml/OWzx/pages/PreparePage.qml"),
      QVariantMap{{"editorVm", vm("editorViewModel")},
                  {"configVm", vm("configViewModel")}},
      kDefaultSettleMs);
  row(QStringLiteral("PreviewPage"),
      QStringLiteral("qrc:/qml/OWzx/pages/PreviewPage.qml"),
      QVariantMap{{"previewVm", vm("previewViewModel")},
                  {"editorVm", vm("editorViewModel")},
                  {"configVm", vm("configViewModel")}},
      kDefaultSettleMs);
  row(QStringLiteral("AssemblePage"),
      QStringLiteral("qrc:/qml/OWzx/pages/AssemblePage.qml"),
      QVariantMap{{"editorVm", vm("editorViewModel")}}, kDefaultSettleMs);
  row(QStringLiteral("MonitorPage"),
      QStringLiteral("qrc:/qml/OWzx/pages/MonitorPage.qml"),
      QVariantMap{{"monitorVm", vm("monitorViewModel")}}, kMonitorSettleMs);
  row(QStringLiteral("MultiMachinePage"),
      QStringLiteral("qrc:/qml/OWzx/pages/MultiMachinePage.qml"),
      QVariantMap{{"multiMachineVm", vm("multiMachineViewModel")}},
      kDefaultSettleMs);
  row(QStringLiteral("ProjectPage"),
      QStringLiteral("qrc:/qml/OWzx/pages/ProjectPage.qml"),
      QVariantMap{{"projectVm", vm("projectViewModel")},
                  {"editorVm", vm("editorViewModel")}},
      kDefaultSettleMs);
  row(QStringLiteral("CalibrationPage"),
      QStringLiteral("qrc:/qml/OWzx/pages/CalibrationPage.qml"),
      QVariantMap{{"calibrationVm", vm("calibrationViewModel")}},
      kDefaultSettleMs);
  row(QStringLiteral("PreferencesPage"),
      QStringLiteral("qrc:/qml/OWzx/pages/PreferencesPage.qml"),
      QVariantMap{{"settingsVm", vm("settingsViewModel")}},
      kDefaultSettleMs);
}

// Stability hook — pin the Daily Tips hint to a fixed index before HomePage
// is created. HomePage's Component.onCompleted calls Backend.showDailyTip()
// (HomePage.qml:27), which weighted-random-picks from QRandomGenerator::
// global() (BackendContext.cpp selectNextHint(true)) — real per-run
// nondeterminism the double-grab cannot see (the frame is stable WITHIN a
// run, a different hint BETWEEN runs; the first gate run failed with a
// double-exposed hint banner). QRandomGenerator::global() refuses reseeding
// (qFatal), so this works WITH the selection algorithm instead: drive
// showDailyTip() until every hint EXCEPT the chosen target is in the
// displayed set — selectNextHint then has exactly one candidate left and
// HomePage's pick is forced to that index on every run (the audit's
// "固定模型" hook). The target is the LIGHTEST-weighted hint: starvation
// succeeds per attempt with probability ~exp(-w_target * H / W) — index 0
// (weight 10 of ~155 total) is practically unforceable, the weight-2 hints
// take ~3 attempts. If the walk lands on the target early, it continues to
// full exhaustion (selectNextHint's internal reset clears the displayed set)
// and restarts. The JSON is parsed with the same rules as
// BackendContext::initHintDatabase (objects only, weight default 1) so the
// index mapping matches exactly.
void VisualRegressionTests::pinDailyTipHint()
{
  // Parse the same embedded hints.json the backend loads.
  QFile file(QStringLiteral(":/qml/data/hints.json"));
  QVERIFY2(file.open(QIODevice::ReadOnly),
           "pinDailyTipHint: cannot open :/qml/data/hints.json");
  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
  QVERIFY2(!doc.isNull() && doc.isArray(),
           qPrintable(QStringLiteral("pinDailyTipHint: hints.json parse error: %1")
                          .arg(parseError.errorString())));
  QVector<int> weights;
  const QJsonArray arr = doc.array();
  for (const auto &val : arr) {
    if (!val.isObject())
      continue;
    weights.append(val.toObject().value(QLatin1String("weight")).toInt(1));
  }
  if (weights.size() <= 1)
    return; // nothing to pin

  int targetHint = 0;
  for (int i = 1; i < weights.size(); ++i) {
    if (weights[i] < weights[targetHint])
      targetHint = i;
  }

  constexpr int kMaxPinSteps = 20000;
  QSet<int> displayedMirror; // mirrors BackendContext::m_displayedHintIds
  for (int step = 0; step < kMaxPinSteps; ++step) {
    backend_->showDailyTip();
    const int idx = backend_->currentHintIndex();
    if (displayedMirror.size() == weights.size()) {
      // The step above found no candidates and triggered selectNextHint's
      // internal reset — the mirror restarts from this fresh pick.
      displayedMirror.clear();
    }
    displayedMirror.insert(idx);
    if (!displayedMirror.contains(targetHint) &&
        displayedMirror.size() == weights.size() - 1)
      return; // target is the only candidate left; HomePage's pick is forced
  }
  QFAIL("pinDailyTipHint: could not force the Daily Tips pick to the "
        "lightest-weight hint within the step budget — hint weights or "
        "selection changed?");
}

void VisualRegressionTests::pagesRender()
{
  QFETCH(QString, pageName);
  QFETCH(QString, qmlPath);
  QFETCH(QVariantMap, initialProps);
  QFETCH(int, settleMs);

  const QString filter = pageFilter();
  if (!filter.isEmpty() && filter != pageName)
    QSKIP("filtered out by OWZX_VISREG_PAGE");

  // HomePage's Component.onCompleted pulls a weighted-RANDOM hint
  // (HomePage.qml:27 -> BackendContext::selectNextHint(true)); pin it to a
  // fixed index first or the card shows a different hint every run.
  if (pageName == QLatin1String("HomePage"))
    pinDailyTipHint();

  QQmlComponent component(engine_, QUrl(qmlPath));
  QVERIFY2(!component.isError(),
           qPrintable(QStringLiteral("%1 failed to compile: %2")
                          .arg(pageName, component.errorString())));

  // Fresh window per row: fixed viewport, software-rendered on demand.
  QQuickWindow *window = new QQuickWindow;
  window->resize(kViewportWidth, kViewportHeight);

  std::unique_ptr<QObject> page(
      component.createWithInitialProperties(initialProps));
  QVERIFY2(page, qPrintable(QStringLiteral("%1 failed to instantiate: %2")
                                .arg(pageName, component.errorString())));
  auto *pageItem = qobject_cast<QQuickItem *>(page.get());
  QVERIFY2(pageItem, qPrintable(QStringLiteral("%1 root must be an Item").arg(pageName)));
  pageItem->setParentItem(window->contentItem());
  page->setParent(window); // lifetime follows the window...
  page.release();          // ...which now owns it (unique_ptr hands off)
  // Page roots are plain Items resized by main.qml's StackLayout at runtime;
  // standalone we size them to the viewport directly (no anchors on page
  // roots, so this cannot collide with anchor layout).
  if (pageItem->width() <= 0)
    pageItem->setWidth(kViewportWidth);
  if (pageItem->height() <= 0)
    pageItem->setHeight(kViewportHeight);

  window->show();
  QCoreApplication::processEvents();
  QTest::qWait(settleMs);
  QCoreApplication::processEvents();

  bool stable = false;
  QString diagnostic;
  const QImage actual = captureStableFrame(window, &stable, &diagnostic);
  QVERIFY2(stable, qPrintable(QStringLiteral("%1: %2")
                                  .arg(pageName, diagnostic)));

  const QString baselinePath =
      baselineDir() + QStringLiteral("/%1.png").arg(pageName);
  const QImage baseline(baselinePath);

  if (updateBaselineMode()) {
    const bool hadBaseline = !baseline.isNull();
    const QImage oldBaseline = baseline;
    QVERIFY2(QFileInfo(baselinePath).dir().mkpath(QStringLiteral(".")),
             qPrintable(QStringLiteral("cannot create %1").arg(baselineDir())));
    QVERIFY2(actual.save(baselinePath, "PNG"),
             qPrintable(QStringLiteral("cannot write baseline %1").arg(baselinePath)));
    m_rewrittenBaselines.append(pageName);
    ++m_pagesCaptured;
    if (hadBaseline) {
      const CompareMetrics drift = compareImages(oldBaseline, actual);
      if (drift.sizeOk && drift.mismatchedFraction > 0.0) {
        qInfo("[VisualRegression] %s baseline rewritten: %lld/%lld pixels "
              "(%.3f%%) beyond tolerance vs old baseline, maxChannelDiff=%d, "
              "meanAbsDiff=%.3f, aHashHamming=%d — quote these numbers in the "
              "baseline-update commit",
              qPrintable(pageName), static_cast<long long>(drift.mismatchedPixels),
              static_cast<long long>(drift.actualSize.width() * drift.actualSize.height()),
              drift.mismatchedFraction * 100.0, drift.maxChannelDiff,
              drift.meanAbsDiff, drift.aHashHamming);
      }
    } else {
      qInfo("[VisualRegression] %s baseline created (%dx%d)",
            qPrintable(pageName), actual.width(), actual.height());
    }
  } else {
    if (baseline.isNull()) {
      QFAIL(qPrintable(
          QStringLiteral("%1: baseline missing at %2\n"
                         "  The gate never auto-creates baselines. Capture one "
                         "with:\n"
                         "    build\\VisualRegressionTests.exe with "
                         "OWZX_VISREG_UPDATE_BASELINE=1 (optionally "
                         "OWZX_VISREG_PAGE=%3)\n"
                         "  review the PNG, and commit it together with the "
                         "change it blesses (tests/visual_baseline/README.md).")
              .arg(pageName, baselinePath, pageName)));
    }
    const CompareMetrics metrics = compareImages(baseline, actual);
    if (!metrics.sizeOk || metrics.mismatchedFraction > kMaxMismatchFraction)
      failWithMetrics(pageName, baseline, actual, metrics);
    ++m_pagesCaptured;
  }

  // Per-row teardown: hide and drop the window so rows stay independent.
  window->hide();
  window->deleteLater();
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  QCoreApplication::processEvents();
}

void VisualRegressionTests::failWithMetrics(const QString &page,
                                            const QImage &baseline,
                                            const QImage &actual,
                                            const CompareMetrics &metrics)
{
  // Failure artifacts land in cwd (build/ under verify and ctest).
  const QDir failureDir(QDir::current().absoluteFilePath(
      QStringLiteral("visual_failures")));
  failureDir.mkpath(QStringLiteral("."));
  const QString actualPath =
      failureDir.filePath(QStringLiteral("%1__actual.png").arg(page));
  actual.save(actualPath, "PNG");
  QString artifactNote =
      QStringLiteral("\n  actual image: %1").arg(actualPath);
  if (metrics.sizeOk) {
    const QString diffPath =
        failureDir.filePath(QStringLiteral("%1__diff.png").arg(page));
    buildDiffVisualization(baseline, actual).save(diffPath, "PNG");
    artifactNote += QStringLiteral("\n  diff image  : %1 (red = beyond "
                                   "tolerance, dimmed = within)")
                        .arg(diffPath);
  }

  QString core;
  if (!metrics.sizeOk) {
    core = QStringLiteral("size mismatch: baseline %1x%2 vs actual %3x%4 — the "
                          "viewport, scale factor or rendering pipeline "
                          "changed; this is a conscious baseline update, not a "
                          "tolerance case")
               .arg(metrics.baselineSize.width())
               .arg(metrics.baselineSize.height())
               .arg(metrics.actualSize.width())
               .arg(metrics.actualSize.height());
  } else {
    core = QStringLiteral("%1 of %2 pixels (%3%) exceed the per-channel "
                          "tolerance %4 (budget %5%); maxChannelDiff=%6, "
                          "meanAbsDiff=%7, aHashHamming=%8")
               .arg(metrics.mismatchedPixels)
               .arg(qsizetype(metrics.actualSize.width()) *
                    metrics.actualSize.height())
               .arg(metrics.mismatchedFraction * 100.0, 0, 'f', 3)
               .arg(kPixelChannelTolerance)
               .arg(kMaxMismatchFraction * 100.0, 0, 'f', 1)
               .arg(metrics.maxChannelDiff)
               .arg(metrics.meanAbsDiff, 0, 'f', 3)
               .arg(metrics.aHashHamming);
  }

  QFAIL(qPrintable(QStringLiteral("%1: visual regression — %2%3\n"
                                  "  If (and only if) the change is intended, "
                                  "follow tests/visual_baseline/README.md to "
                                  "re-bless the baseline WITH this change.")
                       .arg(page, core, artifactNote)));
}

void VisualRegressionTests::cleanupTestCase()
{
  // Stale-baseline hygiene, full runs only (a filtered run must not judge
  // the pages it skipped): a PNG with no matching row is a dead baseline —
  // its page was renamed or removed, and the file must travel out with the
  // same commit.
  if (pageFilter().isEmpty()) {
    const QDir dir(baselineDir());
    const QStringList pngs =
        dir.entryList(QStringList{QStringLiteral("*.png")}, QDir::Files);
    QStringList stale;
    for (const QString &png : pngs) {
      const QString pageName = png.chopped(4); // strip .png
      if (!m_pageRows.contains(pageName))
        stale.append(png);
    }
    if (!stale.isEmpty()) {
      QFAIL(qPrintable(QStringLiteral(
                           "stale baselines in %1 have no page row (page "
                           "renamed/removed?): %2 — delete them in the same "
                           "commit that changed the rows")
                           .arg(baselineDir(), stale.join(", "))));
    }
  }

  if (updateBaselineMode()) {
    qInfo("[VisualRegression] update mode captured %d page(s)%s%s. Commit the "
          "rewritten PNG(s) together with the QML/C++ change that justifies "
          "them, quoting the old-vs-new metrics printed above. Baseline-only "
          "commits need explicit justification in the commit message (see "
          "tests/visual_baseline/README.md).",
          m_pagesCaptured,
          m_rewrittenBaselines.isEmpty() ? QString() : QStringLiteral(": "),
          m_rewrittenBaselines.join(QStringLiteral(", ")).toUtf8().constData());
  } else {
    qInfo("[VisualRegression] %d page(s) compared against committed baselines",
          m_pagesCaptured);
  }
}

int main(int argc, char *argv[])
{
  // Rendering determinism first: offscreen QPA + software scene graph. Unlike
  // QmlSmokeTests (which never renders and can respect an ambient
  // QT_QPA_PLATFORM), this suite's pixels ARE the test — an ambient platform,
  // scale factor or backend must never leak in. OWZX_VISREG_PLATFORM remains
  // as an explicit debugging escape hatch.
  const QByteArray overridePlatform = qgetenv("OWZX_VISREG_PLATFORM");
  if (!overridePlatform.isEmpty())
    qputenv("QT_QPA_PLATFORM", overridePlatform);
  else
    qputenv("QT_QPA_PLATFORM", "offscreen");
  qputenv("QT_QUICK_BACKEND", "software");
  qputenv("QT_SCALE_FACTOR", "1");

  // The test exe runs beside the DEPLOYED Qt6Core.dll in build/ (relocated
  // QLibraryInfo prefix) — point QPA plugin discovery at the Qt install's
  // platforms dir (same mechanism as QmlSmokeTests / OWZX_QT_PLATFORM_PLUGIN_DIR).
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM_PLUGIN_PATH"))
    qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", OWZX_QT_PLATFORM_PLUGIN_DIR);

  QGuiApplication app(argc, argv);
  app.setOrganizationName(QStringLiteral("OWzx"));
  app.setApplicationName(QStringLiteral("OWzxSlicer"));

  VisualRegressionTests tc;
  return QTest::qExec(&tc, argc, argv);
}

#include "VisualRegressionTests.moc"
