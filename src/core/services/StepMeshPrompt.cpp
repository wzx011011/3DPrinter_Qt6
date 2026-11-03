#include "StepMeshPrompt.h"

#include <QEventLoop>
#include <QSettings>
#include <QThread>
#include <QtConcurrent/QtConcurrentRun>

#include <cmath>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Model.hpp>  // STEP.hpp uses Slic3r::Model undeclared
#include <libslic3r/Format/STEP.hpp>
#endif

namespace {
const char *const kEnableKey = "enableStepMeshSetting";
const char *const kLinearKey = "stepLinearDeflection";
const char *const kAngleKey = "stepAngleDeflection";
const char *const kSplitKey = "stepSplitCompound";

// Upstream stores float_to_string_decimal_point(value, 3 / 2).
double roundTo(double value, int decimals)
{
  const double scale = std::pow(10.0, decimals);
  return std::round(value * scale) / scale;
}
} // namespace

StepMeshPrompt::StepMeshPrompt(QObject *parent)
    : QObject(parent)
{
  connect(&countWatcher_, &QFutureWatcherBase::finished, this, [this]() {
    if (!countWatcher_.future().isFinished() || countWatcher_.future().resultCount() == 0)
      return;
    const CountResult r = countWatcher_.result();
    if (r.generation != countGeneration_ || !active_)
      return;
    // Upstream only publishes a non-zero count (0 = stopped or failed) and
    // otherwise leaves "Calculating, please wait..." up.
    if (r.count == 0)
      return;
    facetCount_ = int(r.count);
    countedLinear_ = r.linear;
    countedAngle_ = r.angle;
    emit facetCountChanged();
  });
}

StepMeshPrompt::~StepMeshPrompt()
{
  cancelPending();
}

void StepMeshPrompt::cancelPending()
{
  stopCount();
  bool wasPending = false;
  {
    QMutexLocker lock(&mutex_);
    if (state_ == State::Pending) {
      state_ = State::Cancelled;
      file_ = nullptr;
      answeredCond_.wakeAll();
      wasPending = true;
    }
  }
  if (active_) {
    active_ = false;
    emit activeChanged();
  }
  if (wasPending)
    emit answered();
}

StepMeshPrompt::Params StepMeshPrompt::storedDefaults()
{
  QSettings s;
  Params p;
  p.linear = s.value(QLatin1String(kLinearKey), 0.003).toDouble();
  if (p.linear <= 0.0)
    p.linear = 0.003;
  p.angle = s.value(QLatin1String(kAngleKey), 0.5).toDouble();
  if (p.angle <= 0.0)
    p.angle = 0.5;
  p.splitCompound = s.value(QLatin1String(kSplitKey), false).toBool();
  return p;
}

bool StepMeshPrompt::promptEnabled()
{
  return QSettings().value(QLatin1String(kEnableKey), true).toBool();
}

bool StepMeshPrompt::linearInRange(double value)
{
  return value >= kLinearMin && value <= kLinearMax;
}

bool StepMeshPrompt::angleInRange(double value)
{
  return value >= kAngleMin && value <= kAngleMax;
}

void StepMeshPrompt::setPresenterAttached(bool attached)
{
  if (presenterAttached_.exchange(attached) == attached)
    return;
  emit presenterAttachedChanged();
  // A presenter that goes away mid-question cannot answer any more.
  if (!attached && active_)
    finish(false, initial_.linear, initial_.angle, initial_.splitCompound, false, false);
}

bool StepMeshPrompt::ask(Slic3r::Step &file, const QString &fileName, Params &io)
{
  const Params defaults = storedDefaults();
  io = defaults;
  // enable_step_mesh_setting off -> the stored values are used directly.
  if (!promptEnabled() || !presenterAttached_.load())
    return true;
  {
    QMutexLocker lock(&mutex_);
    if (state_ == State::Pending)
      return true;  // one dialog at a time; a concurrent import keeps the defaults
    state_ = State::Pending;
    file_ = &file;
  }

  if (QThread::currentThread() == thread()) {
    // Synchronous import on the GUI thread: modal loop, like ShowModal().
    begin(fileName, defaults);
    QEventLoop loop;
    connect(this, &StepMeshPrompt::answered, &loop, &QEventLoop::quit);
    bool pending = false;
    {
      QMutexLocker lock(&mutex_);
      pending = state_ == State::Pending;
    }
    if (pending)
      loop.exec(QEventLoop::DialogExec);
  } else {
    QMetaObject::invokeMethod(this, [this, fileName, defaults]() { begin(fileName, defaults); },
                              Qt::QueuedConnection);
    QMutexLocker lock(&mutex_);
    while (state_ == State::Pending)
      answeredCond_.wait(&mutex_);
  }

  QMutexLocker lock(&mutex_);
  const bool accepted = state_ == State::Accepted;
  if (accepted)
    io = result_;
  state_ = State::Idle;
  return accepted;
}

void StepMeshPrompt::begin(const QString &fileName, const Params &initial)
{
  {
    // Cancelled (service teardown) before this queued call ran.
    QMutexLocker lock(&mutex_);
    if (state_ != State::Pending)
      return;
  }
  fileName_ = fileName;
  initial_ = initial;
  active_ = true;
  facetCount_ = -1;
  countedLinear_ = -1.0;
  countedAngle_ = -1.0;
  emit activeChanged();
  emit facetCountChanged();
  if (!presenterAttached_.load()) {
    finish(false, initial.linear, initial.angle, initial.splitCompound, false, false);
    return;
  }
  emit dialogRequested();
  requestFacetCount(initial.linear, initial.angle);
}

void StepMeshPrompt::stopCount()
{
#ifdef HAS_LIBSLIC3R
  if (countWatcher_.isRunning()) {
    Slic3r::Step *file = file_;
    if (file)
      file->m_stop_mesh.store(true);
    countWatcher_.waitForFinished();
    if (file)
      file->m_stop_mesh.store(false);
  }
#endif
  ++countGeneration_;
}

void StepMeshPrompt::requestFacetCount(double linear, double angle)
{
#ifdef HAS_LIBSLIC3R
  if (!active_ || !file_)
    return;
  if (facetCount_ > 0 && linear == countedLinear_ && angle == countedAngle_)
    return;
  stopCount();
  if (facetCount_ != -1) {
    facetCount_ = -1;
    emit facetCountChanged();
  }
  Slic3r::Step *file = file_;
  const quint64 generation = countGeneration_;
  countWatcher_.setFuture(QtConcurrent::run([file, linear, angle, generation]() {
    CountResult r;
    r.generation = generation;
    r.linear = linear;
    r.angle = angle;
    r.count = file->get_triangle_num(linear, angle);
    return r;
  }));
#else
  Q_UNUSED(linear);
  Q_UNUSED(angle);
#endif
}

bool StepMeshPrompt::finish(bool accepted, double linear, double angle, bool splitCompound,
                            bool saveAsDefault, bool dontShowAgain)
{
  {
    QMutexLocker lock(&mutex_);
    if (state_ != State::Pending)
      return false;
  }
  if (accepted && !(linearInRange(linear) && angleInRange(angle)))
    return false;

  stopCount();

  if (accepted && (dontShowAgain || saveAsDefault)) {
    QSettings s;
    if (dontShowAgain)
      s.setValue(QLatin1String(kEnableKey), false);
    if (saveAsDefault) {
      s.setValue(QLatin1String(kSplitKey), splitCompound);
      s.setValue(QLatin1String(kLinearKey), roundTo(linear, 3));
      s.setValue(QLatin1String(kAngleKey), roundTo(angle, 2));
    }
    emit defaultsChanged();
  }

  {
    QMutexLocker lock(&mutex_);
    result_.linear = linear;
    result_.angle = angle;
    result_.splitCompound = splitCompound;
    state_ = accepted ? State::Accepted : State::Cancelled;
    file_ = nullptr;
    answeredCond_.wakeAll();
  }
  active_ = false;
  emit activeChanged();
  emit answered();
  return true;
}
