#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>

class QTimer;
class SliceService;
class ProjectServiceMock;

// Calibration step in a wizard flow
struct CalibrationStep
{
    QString id;          // e.g. "start", "preset", "cali", "coarse_save", "fine_cali", "fine_save", "save"
    QString title;       // Display title
    QString description; // Guidance text shown on this step
};

// A calibration type (e.g. Flow Dynamics, Bed Leveling)
struct CalibrationType
{
    QString     id;           // e.g. "flow_dynamics", "bed_leveling"
    QString     name;         // Display name
    QString     icon;         // Icon character
    QString     category;     // "slice" or "hardware"
    QString     description;  // Short description
    QString     longDesc;     // Detailed guidance
    QString     previewLabel; // Preview area label
    QList<CalibrationStep> steps;
    bool        implemented = false;
    bool        startable = false;
    QString     unavailableReason;
    int         calibMode = 0;
    double      calibStart = 0.0;
    double      calibEnd = 0.0;
    double      calibStep = 0.0;
    bool        printNumbers = false;
};

// Status of a calibration type
enum class CalibrationStatus : int
{
    NotStarted  = 0,
    InProgress  = 1,
    Completed   = 2,
    Failed      = 3
};

// Calibration history entry aligned with upstream FlowCalibHeaderView.
struct CalibrationHistoryEntry
{
    QString name;           // Calibration type name
    QString filamentId;     // Filament preset identifier
    float kValue;           // K-value (Pressure Advance); 0 when no machine-readable readback
    float nozzleDiameter;   // Nozzle diameter used
    QString timestamp;      // ISO timestamp
    // Phase 125 (CALIB-03): honest result bookkeeping. hasRealReadback is true
    // only when libslic3r emitted a machine-readable result marker (PA M900 K /
    // SET_PRESSURE_ADVANCE). For FlowRate/TempTower/Vol_speed/VFA/Retraction
    // the calibration outcome is read from the physical print (band/layer
    // inspection) -- upstream CalibUtils never auto-parses those -- so
    // hasRealReadback stays false and notes carries the manual-interpretation
    // guidance instead of a fabricated value.
    bool hasRealReadback = false;
    QString notes;          // Status/manual-interpretation text (no fabricated K)
};

// Mock calibration service - simulates calibration wizard flow
// Upstream reference: CalibrationPanel + CalibrationWizard + CalibrationDialog
// Replace with real CalibrationService when device communication is integrated
class CalibrationServiceMock final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(int currentStepIndex READ currentStepIndex NOTIFY stepChanged)

public:
    explicit CalibrationServiceMock(QObject *parent = nullptr);
    ~CalibrationServiceMock() override;

    /// Inject SliceService for real calibration slices.
    /// It sets calib_params and runs the slice/export pipeline for PA, Flow Rate, and Temp Tower.
    void setSliceService(SliceService *slice);
    /// Phase 197: inject ProjectServiceMock so the four tower modes
    /// (TempTower/Vol_speed/VFA/Retraction) can load a dedicated tower model
    /// from the bundled qrc:/qml/assets/calib/ resources onto the current plate
    /// before slicing -- mirroring upstream Plater::calib_temp/vol_speed/VFA/
    /// retraction (Plater.cpp:9797+), which call add_model() with
    /// resources/calib/<mode>/<tower>.stl. Optional: when null, the tower modes
    /// fall back to the legacy cloneCurrentPlateModel() geometry path.
    void setProjectService(ProjectServiceMock *project);

    // Calibration type list
    Q_INVOKABLE int calibTypeCount() const;
    Q_INVOKABLE QString calibTypeId(int index) const;
    Q_INVOKABLE QString calibTypeName(int index) const;
    Q_INVOKABLE QString calibTypeIcon(int index) const;
    Q_INVOKABLE QString calibTypeCategory(int index) const;
    Q_INVOKABLE QString calibTypeDesc(int index) const;
    Q_INVOKABLE QString calibTypeLongDesc(int index) const;
    Q_INVOKABLE QString calibTypePreviewLabel(int index) const;
    Q_INVOKABLE int calibTypeIndexById(const QString &id) const;
    Q_INVOKABLE bool calibTypeImplemented(int index) const;
    Q_INVOKABLE bool calibTypeStartable(int index) const;
    Q_INVOKABLE QString calibTypeUnavailableReason(int index) const;

    // Steps for a given calibration type
    Q_INVOKABLE int stepCount(int typeIndex) const;
    Q_INVOKABLE QString stepId(int typeIndex, int stepIndex) const;
    Q_INVOKABLE QString stepTitle(int typeIndex, int stepIndex) const;
    Q_INVOKABLE QString stepDesc(int typeIndex, int stepIndex) const;
    /// Step state: 0=pending, 1=active, 2=completed, aligned with upstream StepCtrl.
    Q_INVOKABLE int stepState(int typeIndex, int stepIndex) const;

    // Status of a calibration type
    Q_INVOKABLE int calibStatus(int typeIndex) const;

    // Phase 125 (CALIB-02): per-mode default range readback + user override.
    // The defaults are the Phase 124 hardcoded seeds (buildMockData); the user
    // can override them via setCalibRange before startCalibration. The edited
    // range then flows into SliceService::setCalibParams, replacing the
    // previously hardcoded sweep.
    Q_INVOKABLE double calibTypeStart(int typeIndex) const;
    Q_INVOKABLE double calibTypeEnd(int typeIndex) const;
    Q_INVOKABLE double calibTypeStep(int typeIndex) const;
    /// Apply a user-edited range override for the given mode. Stored on the
    /// CalibrationType so startCalibration dispatches the edited sweep. No-op
    /// when start/end/step are not finite or the index is out of range.
    Q_INVOKABLE void setCalibRange(int typeIndex, double start, double end, double step);

    // Progress
    int progress() const { return m_progress; }
    bool isRunning() const { return m_isRunning; }
    int currentStepIndex() const { return m_currentStepIndex; }

    // Actions
    Q_INVOKABLE void startCalibration(int itemIndex);
    Q_INVOKABLE void cancelCalibration();
    Q_INVOKABLE void goToStep(int stepIndex);
    Q_INVOKABLE void resetCalibration(int itemIndex);

    // History accessors aligned with upstream FlowCalibHeaderView.
    Q_INVOKABLE int historyCount() const;
    Q_INVOKABLE QString historyName(int index) const;
    Q_INVOKABLE QString historyFilamentId(int index) const;
    Q_INVOKABLE float historyKValue(int index) const;
    Q_INVOKABLE float historyNozzleDiameter(int index) const;
    Q_INVOKABLE QString historyTimestamp(int index) const;
    /// Phase 125 (CALIB-03): true when the K-value was read back from a real
    /// machine-readable marker (PA M900 K). False for manual-interpretation modes.
    Q_INVOKABLE bool historyHasRealReadback(int index) const;
    /// Phase 125 (CALIB-03): status/manual-interpretation notes (no fabricated K).
    Q_INVOKABLE QString historyNotes(int index) const;
    Q_INVOKABLE void addHistoryEntry(const QString &name, const QString &filamentId,
                                      float kValue, float nozzleDiameter, const QString &timestamp,
                                      bool hasRealReadback = false, const QString &notes = QString());
    Q_INVOKABLE void clearHistory();

signals:
    void progressChanged();
    void isRunningChanged();
    void calibrationFinished(bool success);
    void stepChanged();
    void statusChanged(int typeIndex, int status);
    void historyChanged();
    void calibrationSliceRequested(int mode, double start, double end, double step,
                                   bool printNumbers, const QString &projectName);

private slots:
    void onTick();
    /// Receive real progress from SliceService instead of the mock timer.
    void onSliceProgressUpdated(int percent, const QString &label);
    /// Slice finished/failed callbacks.
    void onSliceFinished(const QString &estimatedTime);
    void onSliceFailed(const QString &message);

private:
    void buildMockData();
    void setStatus(int typeIndex, CalibrationStatus status);
    void advanceStep();
    /// Phase 125 (CALIB-03): parse the last PA K-value from a sliced G-code
    /// file. Recognizes the markers emitted by upstream GCodeWriter::
    /// set_pressure_advance (GCodeWriter.cpp:370-392):
    ///   Marlin/BBL   -> "M900 K<value>"
    ///   Klipper      -> "SET_PRESSURE_ADVANCE ADVANCE=<value>"
    ///   RepRap       -> "M572 D0 S<value>"
    ///   Repetier     -> "M233 X<value>"
    /// Returns true + outK when a marker was found, false otherwise. Mirrors
    /// the upstream CalibUtils result-parsing intent (read the value the engine
    /// actually wrote, not a fabricated constant).
    static bool parsePressureAdvanceFromGcode(const QString &gcodePath, float &outK);
    /// Build the manual-interpretation note for non-PA tower modes
    /// (FlowRate/TempTower/Vol_speed/VFA/Retraction). The result of those
    /// calibrations is read by inspecting the physical print (band/layer
    /// quality), so we never fabricate a K-value.
    static QString manualInterpretationNote(const QString &modeName);

    /// Phase 197: map a CalibMode (6=TempTower, 7=Vol_speed, 8=VFA,
    /// 9=Retraction) to its bundled tower-model qrc path under
    /// qrc:/qml/assets/calib/. Returns an empty string for modes without a
    /// dedicated upstream tower (PA=1, FlowRate=5) -- those keep using the
    /// current-plate geometry. Aligned with the per-mode add_model() calls in
    /// upstream Plater.cpp (calib_temp:9804, calib_max_vol_speed:9853,
    /// calib_VFA:9971, calib_retraction:9930).
    static QString towerModelQrcPathForMode(int calibMode);
    /// Phase 197: extract a bundled qrc tower model to a temp file so
    /// libslic3r's Model::read_from_file (which uses plain filesystem I/O, not
    /// Qt's qrc virtual FS) can load it. Returns the temp file path on success
    /// or an empty string on failure (qrc missing / write failed). The caller
    /// owns the temp file (QFile::rename on destruction is not needed; the OS
    /// temp dir is cleaned by the system).
    static QString extractQrcToTempFile(const QString &qrcPath);

    QList<CalibrationType> m_calibTypes;
    QMap<int, CalibrationStatus> m_statusMap; // typeIndex -> status
    QList<CalibrationHistoryEntry> m_history; // Calibration history records
    int m_progress = 0;
    bool m_isRunning = false;
    int m_currentItem = -1;
    int m_currentStepIndex = -1;
    QTimer *m_timer = nullptr;
    /// SliceService reference injected by setSliceService for calibration slices.
    SliceService *m_sliceService = nullptr;
    /// Phase 197: ProjectServiceMock reference injected by setProjectService.
    /// Used by the tower modes to loadFile() a dedicated tower model before the
    /// slice starts (mirrors upstream Plater::add_model). Null when not injected
    /// (tower modes then fall back to the legacy current-plate geometry path).
    ProjectServiceMock *m_projectService = nullptr;
    /// Phase 197: true while a dedicated tower-model loadFile() is in flight.
    /// Gates the one-shot loadFinished connection in startCalibration so a
    /// later user-initiated loadFile is not mistaken for the calibration load.
    bool m_pendingCalibTowerLoad = false;
};
