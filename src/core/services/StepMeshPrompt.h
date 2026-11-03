#pragma once

#include <QFutureWatcher>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QWaitCondition>

#include <atomic>

namespace Slic3r {
class Step;
}

// Upstream StepMeshDialog (OrcaSlicer src/slic3r/GUI/StepMeshDialog.cpp) as a
// QML-presentable prompt. Model::read_from_step calls its step_mesh_fn after
// the STEP file is loaded and before it is meshed; ProjectService routes that
// callback to ask(), which shows the dialog and blocks until the user answers
// (a nested event loop on the GUI thread, like wxDialog::ShowModal; a wait
// condition when the import runs on a worker thread). While the dialog is up
// the loaded Step is counted for triangles on a pool thread
// (Step::get_triangle_num), exactly as the upstream dialog does.
//
// AppConfig keys (AppConfig.cpp:608-624) map to QSettings:
//   enable_step_mesh_setting -> enableStepMeshSetting (default true)
//   linear_deflection        -> stepLinearDeflection  (default 0.003)
//   angle_deflection         -> stepAngleDeflection   (default 0.5)
//   is_split_compound        -> stepSplitCompound     (default false)
class StepMeshPrompt : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool active READ active NOTIFY activeChanged)
  Q_PROPERTY(QString fileName READ fileName NOTIFY activeChanged)
  Q_PROPERTY(double initialLinear READ initialLinear NOTIFY activeChanged)
  Q_PROPERTY(double initialAngle READ initialAngle NOTIFY activeChanged)
  Q_PROPERTY(bool initialSplitCompound READ initialSplitCompound NOTIFY activeChanged)
  /// -1 while counting ("Calculating, please wait..."), else the facet count.
  Q_PROPERTY(int facetCount READ facetCount NOTIFY facetCountChanged)
  /// Set by the shell once a dialog can be shown. Without a presenter (tests,
  /// headless runs) ask() answers with the stored defaults immediately.
  Q_PROPERTY(bool presenterAttached READ presenterAttached WRITE setPresenterAttached NOTIFY presenterAttachedChanged)

public:
  struct Params
  {
    double linear = 0.003;
    double angle = 0.5;
    bool splitCompound = false;
  };

  static constexpr double kLinearMin = 0.001;
  static constexpr double kLinearMax = 0.1;
  static constexpr double kAngleMin = 0.01;
  static constexpr double kAngleMax = 1.0;

  explicit StepMeshPrompt(QObject *parent = nullptr);
  ~StepMeshPrompt() override;

  /// Stored defaults (upstream Plater::load_files: non-positive -> 0.003 / 0.5).
  static Params storedDefaults();
  static bool promptEnabled();

  /// step_mesh_fn body. Any thread. Returns false when the user cancelled.
  bool ask(Slic3r::Step &file, const QString &fileName, Params &io);
  /// Answers a pending ask() with Cancel (GUI thread; service teardown).
  void cancelPending();

  bool active() const { return active_; }
  QString fileName() const { return fileName_; }
  double initialLinear() const { return initial_.linear; }
  double initialAngle() const { return initial_.angle; }
  bool initialSplitCompound() const { return initial_.splitCompound; }
  int facetCount() const { return facetCount_; }
  bool presenterAttached() const { return presenterAttached_.load(); }
  void setPresenterAttached(bool attached);

  /// Upstream update_mesh_number_text: recount unless the values match the
  /// last successful count.
  Q_INVOKABLE void requestFacetCount(double linear, double angle);
  /// OK / Cancel. OK with out-of-range values is refused (returns false) and
  /// the dialog stays open, as upstream's OK handler does.
  Q_INVOKABLE bool finish(bool accepted, double linear, double angle, bool splitCompound,
                          bool saveAsDefault, bool dontShowAgain);
  Q_INVOKABLE static bool linearInRange(double value);
  Q_INVOKABLE static bool angleInRange(double value);

signals:
  void activeChanged();
  void facetCountChanged();
  void presenterAttachedChanged();
  /// The dialog should be shown (GUI thread).
  void dialogRequested();
  /// Stored defaults / the enable flag changed (Preferences mirrors them).
  void defaultsChanged();
  /// The pending ask() was answered (GUI thread).
  void answered();

private:
  void begin(const QString &fileName, const Params &initial);
  void stopCount();

  enum class State { Idle, Pending, Accepted, Cancelled };
  struct CountResult
  {
    quint64 generation = 0;
    double linear = 0.0;
    double angle = 0.0;
    unsigned int count = 0;
  };

  QMutex mutex_;
  QWaitCondition answeredCond_;
  State state_ = State::Idle;
  Params result_;
  Slic3r::Step *file_ = nullptr;

  bool active_ = false;
  QString fileName_;
  Params initial_;
  int facetCount_ = -1;
  double countedLinear_ = -1.0;
  double countedAngle_ = -1.0;
  quint64 countGeneration_ = 0;
  QFutureWatcher<CountResult> countWatcher_;
  std::atomic<bool> presenterAttached_{false};
};
